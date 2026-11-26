package com.gta.game32;

import android.content.Context;
import android.os.Build;
import android.util.Log;

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.Locale;

/**
 * Journal du client 32 bits, ecrit AVANT le natif (chargement des .so, plantages Java).
 * Fichier : /storage/emulated/0/Android/data/com.crestwoodroleplay/files/crestwood_log.txt
 * (dossier propre a l'appli : toujours accessible en ecriture, sans permission).
 * C'est ce fichier qu'il faut recuperer quand un telephone 32 bits n'arrive pas a jouer.
 */
public final class CrashLog {
    private static final String TAG = "CrestwoodLog";
    private static final String FILE_NAME = "crestwood_log.txt";   // [v22d] 32 ET 64 bits
    private static File sFile;
    private static boolean sHandlerInstalled;
    public static String sLibError = null;     // premiere erreur de chargement d'une bibliotheque native

    private CrashLog() {}

    private static File file() {
        if (sFile != null) return sFile;
        String[] dirs = {
                "/storage/emulated/0/Android/data/com.crestwoodroleplay/files",
                "/sdcard/Android/data/com.crestwoodroleplay/files",
        };
        for (String d : dirs) {
            try {
                File dir = new File(d);
                if (!dir.exists()) dir.mkdirs();
                if (dir.isDirectory() && dir.canWrite()) { sFile = new File(dir, FILE_NAME); return sFile; }
            } catch (Throwable ignored) { }
        }
        return null;
    }

    /** A appeler une fois le contexte connu (chemin exact du dossier de l'appli). */
    private static Context sCtx;
    private static boolean sExitLogged;

    public static void bind(Context ctx) {
        try {
            if (ctx != null) sCtx = ctx.getApplicationContext();
            File dir = ctx.getExternalFilesDir(null);
            if (dir != null) { dir.mkdirs(); sFile = new File(dir, FILE_NAME); }
        } catch (Throwable ignored) { }
    }

    public static synchronized void log(String msg) {
        Log.i(TAG, msg);
        File f = file();
        if (f == null) return;
        try (FileWriter w = new FileWriter(f, true)) {
            String ts = new SimpleDateFormat("dd/MM HH:mm:ss", Locale.ROOT).format(new Date());
            w.write(ts + "  " + msg + "\n");
        } catch (Throwable ignored) { }
    }

    public static void log(String msg, Throwable t) {
        StringWriter sw = new StringWriter();
        t.printStackTrace(new PrintWriter(sw));
        log(msg + "\n" + sw);
    }

    /** Debut de session : infos de l'appareil + capture des plantages Java. */
    public static void startSession(String where) {
        log("===== nouvelle session (" + where + ") =====");
        log("Appareil : " + Build.MANUFACTURER + " " + Build.MODEL + " | Android " + Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + ")");
        log("ABI : " + Arrays.toString(Build.SUPPORTED_ABIS) + " | processus 64 bits = " + android.os.Process.is64Bit()
                + " | memoire Java max = " + (Runtime.getRuntime().maxMemory() / (1024 * 1024)) + " Mo");
        installHandler();
        logPreviousExit();
    }

    /**
     * [v25] Pourquoi la session precedente s'est arretee (Android 11+) : plantage natif (avec la trace),
     * manque de memoire (le telephone a tue le jeu), ANR, fermeture normale...
     * Indispensable : un manque de memoire ne laisse AUCUNE trace dans le jeu lui-meme.
     */
    public static void logPreviousExit() {
        if (sExitLogged || sCtx == null || Build.VERSION.SDK_INT < 30) return;
        sExitLogged = true;
        try {
            android.app.ActivityManager am = (android.app.ActivityManager) sCtx.getSystemService(Context.ACTIVITY_SERVICE);
            java.util.List<android.app.ApplicationExitInfo> list = am.getHistoricalProcessExitReasons(sCtx.getPackageName(), 0, 3);
            if (list == null || list.isEmpty()) return;
            android.app.ApplicationExitInfo e = list.get(0);
            String why;
            switch (e.getReason()) {
                case android.app.ApplicationExitInfo.REASON_LOW_MEMORY: why = "MANQUE DE MEMOIRE (Android a tue le jeu)"; break;
                case android.app.ApplicationExitInfo.REASON_CRASH_NATIVE: why = "PLANTAGE NATIF (C++)"; break;
                case android.app.ApplicationExitInfo.REASON_CRASH: why = "PLANTAGE JAVA"; break;
                case android.app.ApplicationExitInfo.REASON_ANR: why = "BLOCAGE (ANR)"; break;
                case android.app.ApplicationExitInfo.REASON_SIGNALED: why = "TUE PAR UN SIGNAL (souvent memoire)"; break;
                case android.app.ApplicationExitInfo.REASON_EXIT_SELF: why = "fermeture par le jeu"; break;
                case android.app.ApplicationExitInfo.REASON_USER_REQUESTED: why = "fermee par le joueur"; break;
                case android.app.ApplicationExitInfo.REASON_EXCESSIVE_RESOURCE_USAGE: why = "TROP DE RESSOURCES"; break;
                default: why = "raison " + e.getReason(); break;
            }
            String ts = new SimpleDateFormat("dd/MM HH:mm:ss", Locale.ROOT).format(new Date(e.getTimestamp()));
            log("Session precedente terminee le " + ts + " : " + why + " | importance " + e.getImportance()
                    + " | memoire " + (e.getPss() / 1024) + " Mo PSS / " + (e.getRss() / 1024) + " Mo RSS | " + e.getDescription());
            if (e.getReason() == android.app.ApplicationExitInfo.REASON_CRASH_NATIVE || e.getReason() == android.app.ApplicationExitInfo.REASON_ANR) {
                try (java.io.InputStream in = e.getTraceInputStream()) {
                    if (in != null) {
                        byte[] buf = new byte[24000];
                        int n = 0, r;
                        while (n < buf.length && (r = in.read(buf, n, buf.length - n)) > 0) n += r;
                        String trace = new String(buf, 0, n, java.nio.charset.StandardCharsets.ISO_8859_1);
                        // les tombstones sont binaires (protobuf) depuis Android 12 : on garde les lignes lisibles
                        StringBuilder sb = new StringBuilder();
                        for (String line : trace.split("[\\n\\r]")) {
                            String l = line.replaceAll("[^\\x20-\\x7E]", " ").trim();
                            if (l.length() >= 6 && (l.contains("lib") || l.contains("pc ") || l.contains("signal") || l.contains("Abort") || l.contains("fault"))) sb.append("    ").append(l.length() > 200 ? l.substring(0, 200) : l).append("\n");
                            if (sb.length() > 6000) break;
                        }
                        if (sb.length() > 0) log("Trace du plantage :\n" + sb);
                    }
                } catch (Throwable ignored) { }
            }
        } catch (Throwable t) {
            log("Raison de la fin de session illisible : " + t);
        }
    }

    public static void installHandler() {
        if (sHandlerInstalled) return;
        sHandlerInstalled = true;
        final Thread.UncaughtExceptionHandler prev = Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler((thread, e) -> {
            log("PLANTAGE JAVA dans le thread " + thread.getName(), e);
            if (prev != null) prev.uncaughtException(thread, e);
        });
    }

    /** Charge une bibliotheque native en notant le resultat (au lieu d'un arret silencieux). */
    public static boolean loadLibrary(String name, boolean required) {
        try {
            System.loadLibrary(name);
            log("lib" + name + ".so chargee");
            return true;
        } catch (Throwable t) {
            log((required ? "ERREUR" : "info") + " : lib" + name + ".so non chargee : " + t);
            if (required && sLibError == null) sLibError = "lib" + name + ".so : " + t.getMessage();
            return false;
        }
    }

    /** Liste les .so reellement installes (pour voir s'il manque une bibliotheque armeabi-v7a dans l'APK). */
    public static void logNativeDir(Context ctx) {
        try {
            String dir = ctx.getApplicationInfo().nativeLibraryDir;
            File d = new File(dir);
            String[] names = d.list();
            log("Bibliotheques natives (" + dir + ") : " + (names == null ? "aucune / illisible" : Arrays.toString(names)));
        } catch (Throwable t) {
            log("nativeLibraryDir illisible : " + t);
        }
    }
}
