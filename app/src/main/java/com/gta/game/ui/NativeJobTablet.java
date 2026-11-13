package com.gta.game.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.drawable.GradientDrawable;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONObject;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

/**
 * TABLETTE JOB Crestwood (natif Android, pas de WebView).
 * Ouverte par le dialogue 9890 "@tablet" (9891 = mise a jour, 9892 = fermer).
 * Onglets : EMPLOIS (metiers de la ville + emplois crees par le staff), MON TRAVAIL, QUETES.
 * Les boutons envoient "action [id]" -> SAMP.java -> "/tabact action id" (le serveur re-verifie tout).
 */
public class NativeJobTablet extends FrameLayout {

    public interface Callback {
        void onAction(String cmd);
        void onClose();
    }

    private static final String[] TYPES = {"Livraison", "Transport", "À pied", "Patrouille"};

    private final Callback cb;
    private boolean open;
    private int tab = 0;
    private JSONObject data = new JSONObject();
    private TextView who, cash, clock, toast;
    private final TextView[] tabs = new TextView[3];
    private LinearLayout list;
    private ScrollView scroll;

    public NativeJobTablet(Context c, Callback cb) {
        super(c);
        this.cb = cb;
        setClickable(true);
        setVisibility(GONE);
        setBackgroundColor(0x8C050608);
        build();
    }

    public boolean isOpen() { return open; }

    private int dp(float v) { return CwUi.dp(getContext(), v); }

    private void build() {
        Context c = getContext();
        int sw = c.getResources().getDisplayMetrics().widthPixels, sh = c.getResources().getDisplayMetrics().heightPixels;
        // la tablette : cadre noir (bezel) + ecran
        FrameLayout bezel = new FrameLayout(c);
        GradientDrawable bz = new GradientDrawable();
        bz.setColor(0xF2050608);
        bz.setCornerRadius(dp(16));
        bz.setStroke(Math.max(1, dp(1)), 0x33FFFFFF);
        bezel.setBackground(bz);
        bezel.setPadding(dp(10), dp(10), dp(10), dp(10));
        bezel.setClickable(true);
        LayoutParams blp = new LayoutParams(Math.min(dp(860), Math.round(sw * 0.9f)), Math.min(dp(520), Math.round(sh * 0.9f)), Gravity.CENTER);
        addView(bezel, blp);

        LinearLayout screen = new LinearLayout(c);
        screen.setOrientation(LinearLayout.VERTICAL);
        GradientDrawable sc = new GradientDrawable(GradientDrawable.Orientation.TL_BR, new int[]{0xF20D1017, 0xF2080A0E});
        sc.setCornerRadius(dp(8));
        screen.setBackground(sc);
        screen.setClipToOutline(true);
        bezel.addView(screen, new LayoutParams(-1, -1));
        // camera frontale
        View cam = new View(c);
        GradientDrawable cg = new GradientDrawable();
        cg.setShape(GradientDrawable.OVAL);
        cg.setColor(0xFF1A1D24);
        cam.setBackground(cg);
        LayoutParams camLp = new LayoutParams(dp(5), dp(5), Gravity.TOP | Gravity.CENTER_HORIZONTAL);
        camLp.topMargin = -dp(7.5f);
        bezel.addView(cam, camLp);

        // barre d'etat
        LinearLayout status = new LinearLayout(c);
        status.setOrientation(LinearLayout.HORIZONTAL);
        status.setGravity(Gravity.CENTER_VERTICAL);
        status.setPadding(dp(14), dp(8), dp(10), dp(6));
        TextView app = CwUi.text(c, "CW JOBS", 12f, 0xFFFFFFFF, CwUi.black(c));
        app.setLetterSpacing(0.16f);
        status.addView(app);
        View dot = new View(c);
        dot.setBackgroundColor(CwUi.ACCENT);
        LinearLayout.LayoutParams dlp = new LinearLayout.LayoutParams(dp(6), dp(6));
        dlp.setMargins(dp(6), 0, dp(10), 0);
        status.addView(dot, dlp);
        who = CwUi.text(c, "", 11f, CwUi.MUTED, CwUi.bold(c));
        who.setSingleLine(true);
        who.setEllipsize(TextUtils.TruncateAt.END);
        status.addView(who, new LinearLayout.LayoutParams(0, -2, 1f));
        cash = CwUi.text(c, "", 11.5f, 0xFFFFFFFF, CwUi.black(c));
        status.addView(cash);
        clock = CwUi.text(c, "", 11f, CwUi.MUTED, CwUi.bold(c));
        LinearLayout.LayoutParams clp = new LinearLayout.LayoutParams(-2, -2);
        clp.leftMargin = dp(12);
        status.addView(clock, clp);
        TextView close = CwUi.text(c, "✕", 15f, CwUi.TEXT, CwUi.bold(c));
        close.setGravity(Gravity.CENTER);
        close.setClickable(true);
        close.setOnClickListener(v -> { if (cb != null) cb.onClose(); });
        LinearLayout.LayoutParams xlp = new LinearLayout.LayoutParams(dp(34), dp(30));
        xlp.leftMargin = dp(8);
        status.addView(close, xlp);
        screen.addView(status);

        // onglets
        LinearLayout tabRow = new LinearLayout(c);
        tabRow.setOrientation(LinearLayout.HORIZONTAL);
        tabRow.setPadding(dp(12), 0, dp(12), dp(8));
        String[] names = {"EMPLOIS", "MON TRAVAIL", "QUÊTES"};
        for (int i = 0; i < 3; i++) {
            final int k = i;
            TextView t = CwUi.text(c, names[i], 10.5f, CwUi.TEXT, CwUi.black(c));
            t.setGravity(Gravity.CENTER);
            t.setLetterSpacing(0.12f);
            t.setClickable(true);
            t.setOnClickListener(v -> { tab = k; render(); });
            tabs[i] = t;
            LinearLayout.LayoutParams tl = new LinearLayout.LayoutParams(0, dp(32), 1f);
            if (i > 0) tl.leftMargin = dp(6);
            tabRow.addView(t, tl);
        }
        screen.addView(tabRow);

        scroll = new ScrollView(c);
        scroll.setFillViewport(true);
        scroll.setVerticalScrollBarEnabled(false);
        list = new LinearLayout(c);
        list.setOrientation(LinearLayout.VERTICAL);
        list.setPadding(dp(12), 0, dp(12), dp(10));
        scroll.addView(list, new LayoutParams(-1, -2));
        screen.addView(scroll, new LinearLayout.LayoutParams(-1, 0, 1f));

        toast = CwUi.text(c, "", 11f, 0xFFFFFFFF, CwUi.bold(c));
        toast.setPadding(dp(14), dp(8), dp(14), dp(8));
        toast.setVisibility(GONE);
        screen.addView(toast, new LinearLayout.LayoutParams(-1, -2));
    }

    // ---------------- donnees ----------------
    public void open(String json) {
        setData(json);
        tab = data.optJSONObject("me") != null && (data.optJSONObject("me").optInt("cj", 0) > 0 || data.optJSONObject("me").optInt("on", 0) == 1) ? 1 : 0;
        open = true;
        setVisibility(VISIBLE);
        setAlpha(0f);
        setScaleX(0.97f);
        setScaleY(0.97f);
        animate().alpha(1f).scaleX(1f).scaleY(1f).setDuration(180).start();
        render();
    }

    public void update(String json) {
        setData(json);
        render();
    }

    public void close() {
        open = false;
        animate().alpha(0f).setDuration(140).withEndAction(() -> setVisibility(GONE)).start();
    }

    private void setData(String json) {
        try { data = new JSONObject(json == null || json.isEmpty() ? "{}" : json); } catch (Exception e) { data = new JSONObject(); }
        String msg = data.optString("msg", "");
        if (!msg.isEmpty()) {
            toast.setText(msg);
            toast.setBackgroundColor(data.optInt("ok", 0) == 1 ? 0xE61E4FA8 : 0xE6A3242D);
            toast.setVisibility(VISIBLE);
            toast.removeCallbacks(hideToast);
            toast.postDelayed(hideToast, 3200);
        }
    }

    private final Runnable hideToast = () -> toast.setVisibility(GONE);

    private static String money(int v) {
        String s = String.valueOf(Math.abs(v));
        StringBuilder b = new StringBuilder();
        for (int i = 0; i < s.length(); i++) {
            if (i > 0 && (s.length() - i) % 3 == 0) b.append(' ');
            b.append(s.charAt(i));
        }
        return (v < 0 ? "-" : "") + b + " $";
    }

    private static String dist(int m) {
        if (m < 0) return "—";
        if (m < 1000) return m + " m";
        return String.format(Locale.FRANCE, "%.1f km", m / 1000f);
    }

    private void send(String cmd) { if (cb != null) cb.onAction(cmd); }

    // ---------------- rendu ----------------
    private void render() {
        Context c = getContext();
        JSONObject me = data.optJSONObject("me");
        if (me == null) me = new JSONObject();
        who.setText(me.optString("n", "") + "  ·  " + me.optString("jn", "Aucun"));
        cash.setText(money(me.optInt("cash", 0)));
        clock.setText(new SimpleDateFormat("HH:mm", Locale.FRANCE).format(new Date()));
        for (int i = 0; i < 3; i++) {
            boolean on = i == tab;
            tabs[i].setTextColor(on ? 0xFFFFFFFF : CwUi.MUTED);
            GradientDrawable g = new GradientDrawable();
            g.setColor(on ? CwUi.ACCENT : 0x14FFFFFF);
            g.setCornerRadius(dp(3));
            tabs[i].setBackground(g);
        }
        list.removeAllViews();
        if (tab == 0) renderOffers(c, me);
        else if (tab == 1) renderWork(c, me);
        else renderQuests(c, me);
        scroll.scrollTo(0, 0);
    }

    private LinearLayout card(Context c) {
        LinearLayout l = new LinearLayout(c);
        l.setOrientation(LinearLayout.HORIZONTAL);
        l.setGravity(Gravity.CENTER_VERTICAL);
        l.setPadding(dp(10), dp(9), dp(10), dp(9));
        GradientDrawable g = new GradientDrawable();
        g.setColor(0x0FFFFFFF);
        g.setCornerRadius(dp(3));
        g.setStroke(Math.max(1, dp(1)), 0x1AFFFFFF);
        l.setBackground(g);
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(-1, -2);
        p.topMargin = dp(6);
        list.addView(l, p);
        return l;
    }

    private void section(Context c, String s) {
        TextView t = CwUi.eyebrow(c, s);
        t.setTextColor(CwUi.ACCENT);
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(-2, -2);
        p.topMargin = dp(10);
        list.addView(t, p);
    }

    private View icon(Context c, int model) {
        FrameLayout f = new FrameLayout(c);
        GradientDrawable g = new GradientDrawable();
        g.setColor(0x1A3B82FF);
        g.setCornerRadius(dp(3));
        f.setBackground(g);
        if (model >= 400) {
            VehicleSilhouetteView v = new VehicleSilhouetteView(c);
            v.set(VehicleSilhouetteView.kindOf(model), 0xFF3B82FF);
            LayoutParams lp = new LayoutParams(-1, -1);
            lp.setMargins(dp(5), dp(8), dp(5), dp(8));
            f.addView(v, lp);
        } else {
            f.addView(new Walker(c), new LayoutParams(-1, -1));
        }
        return f;
    }

    private LinearLayout texts(Context c, LinearLayout parent, String title, String sub) {
        LinearLayout t = new LinearLayout(c);
        t.setOrientation(LinearLayout.VERTICAL);
        TextView a = CwUi.text(c, title, 13f, 0xFFFFFFFF, CwUi.black(c));
        a.setSingleLine(true);
        a.setEllipsize(TextUtils.TruncateAt.END);
        t.addView(a);
        TextView b = CwUi.text(c, sub, 10.5f, CwUi.MUTED, CwUi.medium(c));
        b.setSingleLine(true);
        b.setEllipsize(TextUtils.TruncateAt.END);
        LinearLayout.LayoutParams bp = new LinearLayout.LayoutParams(-2, -2);
        bp.topMargin = dp(4);
        t.addView(b, bp);
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(0, -2, 1f);
        p.setMargins(dp(10), 0, dp(8), 0);
        parent.addView(t, p);
        return t;
    }

    private void btn(Context c, LinearLayout parent, String label, boolean primary, String cmd) {
        TextView b = primary ? CwUi.primaryButton(c, label) : CwUi.ghostButton(c, label);
        b.setTextSize(10.5f);
        b.setPadding(dp(12), 0, dp(12), 0);
        b.setOnClickListener(v -> send(cmd));
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(-2, dp(32));
        p.leftMargin = dp(6);
        parent.addView(b, p);
    }

    private void badge(Context c, LinearLayout parent, String s) {
        TextView t = CwUi.text(c, s, 10f, CwUi.TEXT, CwUi.bold(c));
        t.setPadding(dp(8), dp(4), dp(8), dp(4));
        GradientDrawable g = new GradientDrawable();
        g.setColor(0x14FFFFFF);
        g.setCornerRadius(dp(2));
        t.setBackground(g);
        parent.addView(t);
    }

    private void renderOffers(Context c, JSONObject me) {
        int myJob = me.optInt("j", 0);
        JSONArray b = data.optJSONArray("b");
        section(c, "Métiers de la ville");
        if (b != null) for (int i = 0; i < b.length(); i++) {
            JSONArray r = b.optJSONArray(i);
            if (r == null) continue;
            int id = r.optInt(0), model = r.optInt(2), bonus = r.optInt(3), d = r.optInt(4, -1);
            LinearLayout k = card(c);
            k.addView(icon(c, model), new LinearLayout.LayoutParams(dp(54), dp(40)));
            texts(c, k, r.optString(1), (model >= 400 ? "Véhicule de service" : "À pied") + " · prime " + money(bonus) + (id == myJob ? " · TON CONTRAT" : ""));
            badge(c, k, dist(d));
            btn(c, k, "GPS", false, "bgps " + id);
            if (id != myJob && myJob == 0) btn(c, k, "SIGNER", true, "bsign " + id);
            else if (id == myJob) btn(c, k, "SERVICE", true, "bstart");
        }
        JSONArray s = data.optJSONArray("s");
        section(c, "Emplois Crestwood (staff)");
        if (s == null || s.length() == 0) {
            TextView e = CwUi.text(c, "Aucun emploi publié pour l'instant. Le staff les crée depuis le panel admin.", 11f, CwUi.MUTED, CwUi.medium(c));
            LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(-1, -2);
            p.topMargin = dp(6);
            list.addView(e, p);
        } else for (int i = 0; i < s.length(); i++) {
            JSONArray r = s.optJSONArray(i);
            if (r == null) continue;
            int id = r.optInt(0), type = r.optInt(2), model = r.optInt(3), pay = r.optInt(4), d = r.optInt(5, -1), stops = r.optInt(6), runs = r.optInt(7);
            int lvl = Math.min(5, runs / 5) + 1;
            LinearLayout k = card(c);
            k.addView(icon(c, model), new LinearLayout.LayoutParams(dp(54), dp(40)));
            texts(c, k, r.optString(1), TYPES[Math.max(0, Math.min(3, type))] + " · " + stops + " arrêts · " + money(pay) + "/arrêt · niv. " + lvl);
            badge(c, k, dist(d));
            btn(c, k, "GPS", false, "sgps " + id);
            btn(c, k, "COMMENCER", true, "sstart " + id);
        }
    }

    private void renderWork(Context c, JSONObject me) {
        int job = me.optInt("j", 0);
        section(c, "Contrat");
        LinearLayout k = card(c);
        if (job > 0) {
            int routes = me.optInt("r", 0), level = Math.min(5, routes / 10 + 1);
            texts(c, k, me.optString("jn"), "Niveau " + level + " · " + routes + " tournées · " + money(me.optInt("e", 0)) + " gagnés");
            if (me.optInt("on", 0) == 1) badge(c, k, "EN SERVICE · " + money(me.optInt("sh", 0)));
            else btn(c, k, "PRENDRE LE SERVICE", true, "bstart");
            btn(c, k, "GPS", false, "bgps " + job);
            btn(c, k, "DÉMISSIONNER", false, "bresign");
            list.addView(progress(c, (routes % 10) / 10f, level >= 5 ? "Niveau maximum" : "Prochain niveau : " + (10 - routes % 10) + " tournée(s)"));
        } else {
            texts(c, k, "Aucun contrat", "Signe un contrat au siège d'un métier (onglet EMPLOIS → GPS).");
        }
        section(c, "Service Crestwood (staff)");
        LinearLayout s = card(c);
        if (me.optInt("cj", 0) > 0) {
            int cs = me.optInt("cs", 0), ct = Math.max(1, me.optInt("ct", 1));
            texts(c, s, me.optString("cjn"), "Arrêt " + Math.min(cs + 1, ct) + "/" + ct + " · " + money(me.optInt("cm", 0)) + " ce service");
            btn(c, s, "ARRÊTER", false, "sstop");
            list.addView(progress(c, cs / (float) ct, "Suis le GPS jusqu'au prochain arrêt"));
        } else {
            texts(c, s, "Aucun service en cours", "Choisis un emploi dans l'onglet EMPLOIS, va au siège et touche COMMENCER.");
        }
        if (me.optInt("q", 0) > 0) {
            section(c, "Quête en cours");
            LinearLayout q = card(c);
            texts(c, q, "Une quête est en cours", "Termine-la ou /abandonner avant de prendre un service.");
        }
    }

    private View progress(Context c, float f, String label) {
        LinearLayout box = new LinearLayout(c);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setPadding(dp(2), dp(8), dp(2), 0);
        FrameLayout bar = new FrameLayout(c);
        bar.setBackgroundColor(0x1AFFFFFF);
        View fill = new View(c);
        fill.setBackgroundColor(CwUi.ACCENT);
        bar.addView(fill, new LayoutParams(0, -1));
        bar.post(() -> { LayoutParams lp = (LayoutParams) fill.getLayoutParams(); lp.width = Math.round(bar.getWidth() * Math.max(0f, Math.min(1f, f))); fill.setLayoutParams(lp); });
        box.addView(bar, new LinearLayout.LayoutParams(-1, dp(4)));
        TextView t = CwUi.text(c, label, 10f, CwUi.MUTED, CwUi.medium(c));
        LinearLayout.LayoutParams tp = new LinearLayout.LayoutParams(-2, -2);
        tp.topMargin = dp(5);
        box.addView(t, tp);
        return box;
    }

    private void renderQuests(Context c, JSONObject me) {
        section(c, "Quêtes des habitants");
        JSONArray q = data.optJSONArray("qs");
        String[] hints = {"", "Le guide te prête un scooter : 5 lieux à visiter.", "École de pilotage : quiz puis vol en Dodo, atterrissage sous 45 km/h.", "Le coursier te confie des colis chronométrés : enchaîne pour un bonus."};
        if (q != null) for (int i = 0; i < q.length(); i++) {
            JSONArray r = q.optJSONArray(i);
            if (r == null) continue;
            int id = r.optInt(0), done = r.optInt(2), best = r.optInt(3);
            LinearLayout k = card(c);
            texts(c, k, r.optString(1), id > 0 && id < hints.length ? hints[id] : "");
            badge(c, k, done > 0 ? (best > 0 ? String.format(Locale.FRANCE, "✓ %d:%02d", best / 60, best % 60) : "✓ TERMINÉE") : "À DÉCOUVRIR");
            btn(c, k, "GPS PNJ", true, "qgps " + id);
        }
    }

    /** petit pictogramme "a pied" pour les emplois sans vehicule */
    private static final class Walker extends View {
        private final Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        Walker(Context c) { super(c); }
        @Override protected void onDraw(Canvas cv) {
            float w = getWidth(), h = getHeight(), s = Math.min(w, h);
            float cx = w / 2f, top = h / 2f - s * 0.38f;
            p.setColor(CwUi.ACCENT);
            p.setStyle(Paint.Style.FILL);
            cv.drawCircle(cx, top + s * 0.1f, s * 0.09f, p);
            p.setStyle(Paint.Style.STROKE);
            p.setStrokeCap(Paint.Cap.ROUND);
            p.setStrokeWidth(s * 0.08f);
            cv.drawLine(cx, top + s * 0.24f, cx, top + s * 0.5f, p);
            cv.drawLine(cx, top + s * 0.5f, cx - s * 0.14f, top + s * 0.76f, p);
            cv.drawLine(cx, top + s * 0.5f, cx + s * 0.14f, top + s * 0.76f, p);
            cv.drawLine(cx - s * 0.16f, top + s * 0.36f, cx + s * 0.16f, top + s * 0.3f, p);
        }
    }
}
