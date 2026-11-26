package com.crestwood.ui;

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import android.util.LruCache;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceResponse;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.HashMap;
import java.util.Map;

import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

/**
 * [Crestwood v47] PAGES PROTEGEES.
 * A la compilation (app/build.gradle.kts, "cwEncryptAssets"), les .html .js .css .json des pages sont
 * chiffres dans l'APK : "CWX1" + IV (16 octets) + AES/CTR. Quelqu'un qui ouvre l'APK ne voit que du bruit.
 * Ici on les dechiffre a la volee pour les WebViews (shouldInterceptRequest) et pour le code Java
 * qui lit un fichier de page (open()). Un fichier NON chiffre (compilation avec
 * -Pcrestwood.protectPages=false) est servi tel quel : rien ne casse.
 */
public final class CwAssets {
    private static final String TAG = "CwAssets";
    private static final String PREFIX = "/android_asset/";
    private static SecretKeySpec sKey;
    private static final LruCache<String, byte[]> sCache = new LruCache<String, byte[]>(12 * 1024 * 1024) {
        @Override protected int sizeOf(String k, byte[] v) { return v.length; }
    };
    private static Context sApp;

    private CwAssets() { }

    private static synchronized SecretKeySpec key() throws Exception {
        if (sKey == null) {
            // meme graine que dans app/build.gradle.kts (cwEncryptAssets)
            String seed = "Crest" + "wood" + ":" + "pa" + "ges" + ":" + "v47" + ":" + "a9F3k" + "Q7zLm2X";
            byte[] d = MessageDigest.getInstance("SHA-256").digest(seed.getBytes(StandardCharsets.UTF_8));
            byte[] k = new byte[16];
            System.arraycopy(d, 0, k, 0, 16);
            sKey = new SecretKeySpec(k, "AES");
        }
        return sKey;
    }

    public static boolean isProtected(byte[] raw) {
        return raw != null && raw.length >= 20 && raw[0] == 'C' && raw[1] == 'W' && raw[2] == 'X' && raw[3] == '1';
    }

    /** contenu en clair (dechiffre si besoin) */
    public static byte[] decode(byte[] raw) {
        if (!isProtected(raw)) return raw;
        try {
            Cipher c = Cipher.getInstance("AES/CTR/NoPadding");
            c.init(Cipher.DECRYPT_MODE, key(), new IvParameterSpec(raw, 4, 16));
            return c.doFinal(raw, 20, raw.length - 20);
        } catch (Exception e) {
            Log.e(TAG, "dechiffrement impossible", e);
            return new byte[0];
        }
    }

    private static byte[] readAll(Context c, String path) throws IOException {
        try (InputStream in = c.getAssets().open(path)) {
            ByteArrayOutputStream out = new ByteArrayOutputStream(Math.max(4096, in.available()));
            byte[] buf = new byte[65536];
            int n;
            while ((n = in.read(buf)) != -1) out.write(buf, 0, n);
            return out.toByteArray();
        }
    }

    /** Pour le code Java qui lit un fichier de page (ex. clothes/south/catalogue.json). */
    public static InputStream open(Context c, String path) throws IOException {
        return new ByteArrayInputStream(decode(readAll(c, path)));
    }

    private static String mime(String ext) {
        switch (ext) {
            case "html": case "htm": return "text/html";
            case "js": return "application/javascript";
            case "css": return "text/css";
            case "json": return "application/json";
            default: return null;
        }
    }

    /**
     * A appeler depuis WebViewClient.shouldInterceptRequest pour TOUTES les WebViews de l'appli.
     * Retourne null (chargement normal) pour tout ce qui n'est pas une page chiffree.
     */
    public static WebResourceResponse intercept(Context ctx, WebResourceRequest req) {
        try {
            if (req == null || req.getUrl() == null) return null;
            Uri u = req.getUrl();
            if (!"file".equalsIgnoreCase(u.getScheme())) return null;
            String p = u.getPath();
            if (p == null || !p.startsWith(PREFIX)) return null;
            String rel = p.substring(PREFIX.length());
            while (rel.startsWith("/")) rel = rel.substring(1);   // [v60] file:///android_asset//x
            int dot = rel.lastIndexOf('.');
            if (dot < 0) return null;
            String m = mime(rel.substring(dot + 1).toLowerCase());
            if (m == null) return null;
            if (sApp == null && ctx != null) sApp = ctx.getApplicationContext();
            Context c = sApp != null ? sApp : ctx;
            if (c == null) return null;
            byte[] data = sCache.get(rel);
            if (data == null) {
                byte[] raw;
                try { raw = readAll(c, rel); } catch (IOException missing) { note("[pages] introuvable : " + rel); return null; }
                if (!isProtected(raw)) return null;         // page en clair : chargement normal
                data = decode(raw);
                sCache.put(rel, data);
            }
            Map<String, String> h = new HashMap<>();
            h.put("Access-Control-Allow-Origin", "*");
            h.put("Cache-Control", "no-cache");
            return new WebResourceResponse(m, "UTF-8", 200, "OK", h, new ByteArrayInputStream(data));
        } catch (Throwable t) {
            Log.w(TAG, "intercept", t);
            note("[pages] erreur intercept : " + t);
            return null;
        }
    }

    // ------------------------------------------------------------------ [v60] FILET DE SECURITE
    // Certaines WebView (selon la version d'Android / de Chrome) affichaient la page encore chiffree
    // ("CWX1" + du charabia sur fond noir). A appeler dans onPageFinished : si la page visible commence
    // par CWX1, on la reconstruit en clair (CSS et JS integres) avec loadDataWithBaseURL.
    private static int sNotes;
    private static boolean sBound;
    private static void note(String msg) {
        Log.w(TAG, msg);
        if (sNotes++ > 40) return;
        try { com.gta.game32.CrashLog.log(msg); } catch (Throwable ignored) { }
    }

    /**
     * [v60b] Charge une page d'asset DEJA dechiffree (HTML + CSS/JS integres) : ne depend pas de
     * shouldInterceptRequest. Utilise pour le lanceur et l'ecran de chargement. Repli : loadUrl.
     */
    public static void load(final android.webkit.WebView view, final String url) {
        if (view == null || url == null) return;
        String html = null;
        try { html = buildPage(view.getContext(), url); } catch (Throwable t) { note("[pages] load : " + t); }
        if (html != null && !html.startsWith("CWX1")) view.loadDataWithBaseURL(url, html, "text/html", "UTF-8", url);
        else view.loadUrl(url);
    }

    public static void checkPage(final android.webkit.WebView view, final String url) {
        if (view == null || url == null || !url.startsWith("file://" + PREFIX)) return;
        if (!sBound) { sBound = true; try { com.gta.game32.CrashLog.bind(view.getContext()); } catch (Throwable ignored) { } }
        try {
            view.evaluateJavascript("(function(){try{var e=document.body||document.documentElement;return e?String(e.textContent||'').replace(/^\\s+/,'').slice(0,4):''}catch(x){return ''}})()", v -> {
                if (v == null || !v.contains("CWX1")) return;
                note("[pages] page chiffree affichee, reconstruction : " + url);
                try {
                    String html = buildPage(view.getContext(), url);
                    if (html != null) view.loadDataWithBaseURL(url, html, "text/html", "UTF-8", url);
                } catch (Throwable t) { note("[pages] reconstruction impossible : " + t); }
            });
        } catch (Throwable ignored) { }
    }

    private static String relOf(String url) {
        String u = url;
        int q = u.indexOf('?'); if (q >= 0) u = u.substring(0, q);
        int h = u.indexOf('#'); if (h >= 0) u = u.substring(0, h);
        String pre = "file://" + PREFIX;
        if (!u.startsWith(pre)) return null;
        String rel = u.substring(pre.length());
        while (rel.startsWith("/")) rel = rel.substring(1);
        try { rel = java.net.URLDecoder.decode(rel, "UTF-8"); } catch (Throwable ignored) { }
        return rel;
    }

    /** chemin d'asset resolu depuis le dossier de "fromRel" (gere ../), ou null si externe */
    private static String resolve(String fromRel, String href) {
        if (href == null) return null;
        String hl = href.trim().toLowerCase(java.util.Locale.ROOT);
        if (hl.isEmpty() || hl.startsWith("data:") || hl.startsWith("http:") || hl.startsWith("https:") || hl.startsWith("#") || hl.startsWith("about:")) return null;
        if (hl.startsWith("file://")) return relOf(href.trim());
        String h = href.trim();
        int q = h.indexOf('?'); if (q >= 0) h = h.substring(0, q);
        int f = h.indexOf('#'); if (f >= 0) h = h.substring(0, f);
        java.util.ArrayList<String> parts = new java.util.ArrayList<>();
        int slash = fromRel.lastIndexOf('/');
        if (!h.startsWith("/") && slash > 0) for (String x : fromRel.substring(0, slash).split("/")) if (!x.isEmpty()) parts.add(x);
        for (String x : h.split("/")) {
            if (x.isEmpty() || x.equals(".")) continue;
            if (x.equals("..")) { if (!parts.isEmpty()) parts.remove(parts.size() - 1); continue; }
            parts.add(x);
        }
        return android.text.TextUtils.join("/", parts);
    }

    private static String text(Context c, String rel) {
        try { return new String(decode(readAll(c, rel)), StandardCharsets.UTF_8); } catch (Throwable t) { return null; }
    }

    private static final java.util.regex.Pattern CSS_URL = java.util.regex.Pattern.compile("url\\(\\s*(['\"]?)([^'\")]+)\\1\\s*\\)");
    private static final java.util.regex.Pattern LINK = java.util.regex.Pattern.compile("<link\\b[^>]*?href\\s*=\\s*['\"]([^'\"]+\\.css)['\"][^>]*>", java.util.regex.Pattern.CASE_INSENSITIVE);
    private static final java.util.regex.Pattern SCRIPT = java.util.regex.Pattern.compile("<script\\b([^>]*?)\\bsrc\\s*=\\s*['\"]([^'\"]+\\.js)['\"]([^>]*)>\\s*</script>", java.util.regex.Pattern.CASE_INSENSITIVE);

    /** CSS : les url(...) relatives sont rendues absolues (la feuille est integree dans la page) */
    private static String fixCss(String css, String cssRel) {
        java.util.regex.Matcher m = CSS_URL.matcher(css);
        StringBuffer out = new StringBuffer();
        while (m.find()) {
            String r = resolve(cssRel, m.group(2));
            String rep = r == null ? m.group(0) : "url(\"file://" + PREFIX + r + "\")";
            m.appendReplacement(out, java.util.regex.Matcher.quoteReplacement(rep));
        }
        m.appendTail(out);
        return out.toString();
    }

    static String buildPage(Context c, String url) {
        String rel = relOf(url);
        if (rel == null) return null;
        String html = text(c, rel);
        if (html == null) return null;
        // feuilles de style
        java.util.regex.Matcher m = LINK.matcher(html);
        StringBuffer out = new StringBuffer();
        while (m.find()) {
            String r = resolve(rel, m.group(1));
            String css = r == null ? null : text(c, r);
            String rep = css == null ? m.group(0) : "<style>" + fixCss(css, r).replace("</style", "<\\/style") + "</style>";
            m.appendReplacement(out, java.util.regex.Matcher.quoteReplacement(rep));
        }
        m.appendTail(out);
        html = out.toString();
        // scripts
        m = SCRIPT.matcher(html);
        out = new StringBuffer();
        while (m.find()) {
            String r = resolve(rel, m.group(2));
            String js = r == null ? null : text(c, r);
            String rep = js == null ? m.group(0) : "<script" + m.group(1) + m.group(3) + ">" + js.replace("</script", "<\\/script") + "</script>";
            m.appendReplacement(out, java.util.regex.Matcher.quoteReplacement(rep));
        }
        m.appendTail(out);
        return out.toString();
    }
}
