package com.rockstargames.oswrapper;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.IBinder;

/**
 * [v22] Service de premier plan (type dataSync) : garde le processus en vie pendant le telechargement / l'installation
 * des donnees, meme si l'utilisateur quitte l'application. Affiche une notification avec la progression.
 * Demarre par DataInstaller des que la progression commence, arrete a la fin ou en cas d'erreur.
 */
public class DataDownloadService extends Service {
    private static final String CHANNEL_ID = "crestwood_data";
    private static final int NOTIFICATION_ID = 4711;
    private static long lastNotifyMs;

    public static void start(Context context) {
        try {
            Intent intent = new Intent(context, DataDownloadService.class);
            if (Build.VERSION.SDK_INT >= 26) context.startForegroundService(intent); else context.startService(intent);
        } catch (Throwable ignored) {
            // demarrage refuse (application en arriere-plan, Android 12+) : le telechargement continue quand meme dans l'application
        }
    }

    public static void stop(Context context) {
        try {
            context.stopService(new Intent(context, DataDownloadService.class));
        } catch (Throwable ignored) { }
    }

    public static void update(Context context, int percent, String text) {
        long now = System.currentTimeMillis();
        if (percent < 100 && now - lastNotifyMs < 1000L) return;
        lastNotifyMs = now;
        try {
            NotificationManager manager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
            if (manager != null) manager.notify(NOTIFICATION_ID, build(context, percent, text));
        } catch (Throwable ignored) { }
    }

    private static Notification build(Context context, int percent, String text) {
        NotificationManager manager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        if (Build.VERSION.SDK_INT >= 26 && manager != null && manager.getNotificationChannel(CHANNEL_ID) == null) {
            NotificationChannel channel = new NotificationChannel(CHANNEL_ID, "Installation des données", NotificationManager.IMPORTANCE_LOW);
            channel.setShowBadge(false);
            manager.createNotificationChannel(channel);
        }
        Notification.Builder builder = Build.VERSION.SDK_INT >= 26 ? new Notification.Builder(context, CHANNEL_ID) : new Notification.Builder(context);
        builder.setSmallIcon(android.R.drawable.stat_sys_download)
                .setContentTitle("Crestwood RP")
                .setContentText(text == null || text.isEmpty() ? "Installation des données du jeu…" : text)
                .setOngoing(true)
                .setOnlyAlertOnce(true)
                .setProgress(100, Math.max(0, Math.min(100, percent)), false);
        Intent launch = context.getPackageManager().getLaunchIntentForPackage(context.getPackageName());
        if (launch != null) {
            int flags = PendingIntent.FLAG_UPDATE_CURRENT | (Build.VERSION.SDK_INT >= 23 ? PendingIntent.FLAG_IMMUTABLE : 0);
            builder.setContentIntent(PendingIntent.getActivity(context, 0, launch, flags));
        }
        return builder.build();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Notification notification = build(this, 0, "Installation des données du jeu…");
        try {
            if (Build.VERSION.SDK_INT >= 29) startForeground(NOTIFICATION_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC);
            else startForeground(NOTIFICATION_ID, notification);
        } catch (Throwable ignored) {
            stopSelf();
        }
        return START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) { return null; }
}
