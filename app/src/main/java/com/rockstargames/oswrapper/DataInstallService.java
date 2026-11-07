package com.rockstargames.oswrapper;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.IBinder;
import android.os.PowerManager;

import androidx.core.content.ContextCompat;

/**
 * Service de premier plan qui garde le processus en vie pendant le telechargement des donnees,
 * meme si l'utilisateur quitte l'application. Une notification affiche la progression.
 */
public class DataInstallService extends Service {
    private static final String CHANNEL_ID = "crestwood_data";
    private static final int NOTIFICATION_ID = 4201;
    private static volatile boolean running;
    private static volatile Context appContext;
    private static volatile long lastNotifyMs;
    private static volatile int lastPercent = -1;
    private PowerManager.WakeLock wakeLock;

    public static void start(Context context) {
        try {
            appContext = context.getApplicationContext();
            ContextCompat.startForegroundService(appContext, new Intent(appContext, DataInstallService.class));
        } catch (Throwable error) {
            android.util.Log.w("DataInstallService", "Demarrage du service impossible", error);
        }
    }

    public static void stop() {
        Context context = appContext;
        if (context == null) return;
        try { context.stopService(new Intent(context, DataInstallService.class)); } catch (Throwable ignored) { }
    }

    /** Met a jour la notification (limite a ~1 fois/s). */
    public static void notifyProgress(int percent, String phase) {
        Context context = appContext;
        if (context == null || !running) return;
        long now = android.os.SystemClock.elapsedRealtime();
        if (percent == lastPercent && now - lastNotifyMs < 1000L) return;
        if (now - lastNotifyMs < 1000L && percent < 100) return;
        lastNotifyMs = now;
        lastPercent = percent;
        try {
            NotificationManager manager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
            if (manager != null) manager.notify(NOTIFICATION_ID, build(context, percent, phase));
        } catch (Throwable ignored) { }
    }

    private static Notification build(Context context, int percent, String phase) {
        Notification.Builder builder = Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
                ? new Notification.Builder(context, CHANNEL_ID)
                : new Notification.Builder(context);
        builder.setSmallIcon(android.R.drawable.stat_sys_download)
                .setContentTitle("Crestwood RP")
                .setContentText(phase == null || phase.isEmpty() ? "Téléchargement des données du jeu..." : phase)
                .setOngoing(true)
                .setOnlyAlertOnce(true);
        if (percent >= 0 && percent < 100) builder.setProgress(100, percent, false);
        else builder.setProgress(0, 0, true);
        Intent open = context.getPackageManager().getLaunchIntentForPackage(context.getPackageName());
        if (open != null) {
            int flags = android.app.PendingIntent.FLAG_UPDATE_CURRENT
                    | (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ? android.app.PendingIntent.FLAG_IMMUTABLE : 0);
            builder.setContentIntent(android.app.PendingIntent.getActivity(context, 0, open, flags));
        }
        return builder.build();
    }

    @Override public int onStartCommand(Intent intent, int flags, int startId) {
        NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && manager != null) {
            manager.createNotificationChannel(new NotificationChannel(
                    CHANNEL_ID, "Téléchargement des données", NotificationManager.IMPORTANCE_LOW));
        }
        Notification notification = build(this, Math.max(0, lastPercent), "Téléchargement des données du jeu...");
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(NOTIFICATION_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC);
        } else {
            startForeground(NOTIFICATION_ID, notification);
        }
        running = true;
        if (wakeLock == null) {
            PowerManager power = (PowerManager) getSystemService(Context.POWER_SERVICE);
            if (power != null) {
                wakeLock = power.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "crestwood:data-install");
                wakeLock.setReferenceCounted(false);
                wakeLock.acquire(3L * 60L * 60L * 1000L);
            }
        }
        return START_NOT_STICKY;
    }

    /** Android 15+ : delai maximal des services "dataSync" atteint. */
    @Override public void onTimeout(int startId, int fgsType) {
        stopSelf();
    }

    @Override public void onDestroy() {
        running = false;
        lastPercent = -1;
        if (wakeLock != null && wakeLock.isHeld()) wakeLock.release();
        wakeLock = null;
        super.onDestroy();
    }

    @Override public IBinder onBind(Intent intent) { return null; }
}
