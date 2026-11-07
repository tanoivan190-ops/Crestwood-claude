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
 * Fichier : /storage/emulated/0/Android/data/com.crestwood.rp/files/crestwood_32_log.txt
 * (dossier propre a l'appli : toujours accessible en ecriture, sans permission).
 * Une copie est aussi ecrite dans /storage/emulated/0/data/crestwood_32_log.txt (a cote de logcat.txt) si ce dossier est accessible.
 * C'est ce fichier qu'il faut recuperer quand un telephone 32 bits n'arrive pas a jouer.
 */
public final class CrashLog {
    private static final String TAG = "CrestwoodLog";
    private static final String FILE_NAME = "crestwood_32_log.txt";
    private static File sFile;
    private static File sMirror;
    private static boolean sHandlerInstalled;
    public static String sLibError = null;     // premiere erreur de chargement d'une bibliotheque native

    private CrashLog() {}

    private static File file() {
        if (sFile != null) return sFile;
        String[] dirs = {
                "/storage/emulated/0/Android/data/com.crestwood.rp/files",
                "/sdcard/Android/data/com.crestwood.rp/files",
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
    public static void bind(Context ctx) {
        try {
            File dir = ctx.getExternalFilesDir(null);
            if (dir != null) { dir.mkdirs(); sFile = new File(dir, FILE_NAME); }
        } catch (Throwable ignored) { }
    }

    /** Copie du journal dans le dossier des donnees du jeu (accessible au gestionnaire de fichiers, a cote de logcat.txt). */
    private static File mirrorFile() {
        if (sMirror != null) return sMirror;
        try {
            File dir = new File("/storage/emulated/0/data");
            if (!dir.exists()) dir.mkdirs();
            if (dir.isDirectory() && dir.canWrite()) { sMirror = new File(dir, FILE_NAME); return sMirror; }
        } catch (Throwable ignored) { }
        return null;
    }

    public static synchronized void log(String msg) {
        Log.i(TAG, msg);
        String line = new SimpleDateFormat("dd/MM HH:mm:ss", Locale.ROOT).format(new Date()) + "  " + msg + "\n";
        File f = file();
        if (f != null) {
            try (FileWriter w = new FileWriter(f, true)) { w.write(line); } catch (Throwable ignored) { }
        }
        File m = mirrorFile();
        if (m != null) {
            try (FileWriter w = new FileWriter(m, true)) { w.write(line); } catch (Throwable ignored) { }
        }
    }

    public static void log(String msg, Throwable t) {
        StringWriter sw = new StringWriter();
        t.printStackTrace(new PrintWriter(sw));
        log(msg + "\n" + sw);
    }

    /** Debut de session : infos de l'appareil + capture des plantages Java. */
    public static void startSession(String where) {
        log("===== nouvelle session 32 bits (" + where + ") =====");
        log("Appareil : " + Build.MANUFACTURER + " " + Build.MODEL + " | Android " + Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + ")");
        log("ABI : " + Arrays.toString(Build.SUPPORTED_ABIS) + " | processus 64 bits = " + android.os.Process.is64Bit());
        installHandler();
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
