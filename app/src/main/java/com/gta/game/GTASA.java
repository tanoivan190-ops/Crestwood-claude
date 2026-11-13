package com.gta.game;

import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.KeyEvent;


import com.joom.paranoid.Obfuscate;

import com.rockstargames.oswrapper.GameActivityBase;

@Obfuscate
public abstract class GTASA extends GameActivityBase {
    // public static GTASA gtasaSelf = null;
    static String vmVersion;
    private boolean once = false;

    public boolean ServiceAppCommand(String str, String str2)
    {
        return false;
    }

    public int ServiceAppCommandValue(String str, String str2)
    {
        return 0;
    }


    public void onActivityResult(int i, int i2, Intent intent)
    {
        super.onActivityResult(i, i2, intent);
    }

    public void onConfigurationChanged(Configuration configuration)
    {
        super.onConfigurationChanged(configuration);
    }

    public void onCreate(Bundle bundle)
    {

        if(!once)
        {
            once = true;
        }
        System.out.println("GTASA onCreate");
        // [v22d] journal des plantages aussi en 64 bits : Android/data/com.crestwood.rp/files/crestwood_log.txt
        try {
            com.gta.game32.CrashLog.bind(this);
            com.gta.game32.CrashLog.startSession("jeu 64 bits");
            android.app.ActivityManager am = (android.app.ActivityManager) getSystemService(ACTIVITY_SERVICE);
            if (am != null) {
                android.app.ActivityManager.MemoryInfo mi = new android.app.ActivityManager.MemoryInfo();
                am.getMemoryInfo(mi);
                com.gta.game32.CrashLog.log("RAM totale " + (mi.totalMem / (1024 * 1024)) + " Mo, libre " + (mi.availMem / (1024 * 1024)) + " Mo, memoryClass " + am.getMemoryClass());
            }
        } catch (Throwable ignored) { }
        //  gtasaSelf = this;
        //  wantsAccelerometer = true;

        super.onCreate(bundle);


    }

    public void onDestroy()
    {
        System.out.println("GTASA onDestroy");
        super.onDestroy();
    }

    public boolean onKeyDown(int i, KeyEvent keyEvent)
    {
        return super.onKeyDown(i, keyEvent);
    }

    public void onPause()
    {
        System.out.println("GTASA onPause");
        super.onPause();
    }

    public void onRestart()
    {
        System.out.println("GTASA onRestart");
        super.onRestart();
    }

    public void onResume()
    {
        System.out.println("GTASA onResume");
        super.onResume();
    }

    public void onStart()
    {
        System.out.println("GTASA onStart");
        super.onStart();
    }

    public void onStop()
    {
        System.out.println("GTASA onStop");
        super.onStop();
    }
}