package com.gta.game32;

import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.Toast;

import com.bytedance.shadowhook.ShadowHook;
import com.joom.paranoid.Obfuscate;
import com.wardrumstudios.utils.WarMedia;

@Obfuscate
public class GTASA extends WarMedia {
    // public static GTASA gtasaSelf = null;
    static String vmVersion;
    private boolean once = false;

    static {
        // Journal ecrit avant tout le reste : si une bibliotheque manque sur un telephone 32 bits,
        // l'erreur exacte est dans Android/data/com.crestwood.rp/files/crestwood_log.txt
        CrashLog.startSession("chargement des bibliotheques");
        try {
            ShadowHook.init(new ShadowHook.ConfigBuilder()
                    .setMode(ShadowHook.Mode.UNIQUE)
                    .build());
            CrashLog.log("ShadowHook initialise");
        } catch (Throwable t) {
            CrashLog.log("ERREUR ShadowHook.init", t);
        }

        vmVersion = null;
        System.out.println("**** Loading SO's");
        try {
            vmVersion = System.getProperty("java.vm.version");
            System.out.println("vmVersion " + vmVersion);
        } catch (Throwable ignored) {
        }
        CrashLog.loadLibrary("ImmEmulatorJ", false);
        CrashLog.loadLibrary("GTASA", true);
        CrashLog.loadLibrary("bass", true);
        CrashLog.loadLibrary("samp", true);
        if (CrashLog.sLibError != null) {
            // on garde le comportement d'avant (le jeu ne peut pas demarrer) mais avec la cause dans le journal
            throw new UnsatisfiedLinkError("Crestwood : " + CrashLog.sLibError);
        }
    }

    public static void staticEnterSocialClub()
    {
        //  gtasaSelf.EnterSocialClub();
    }

    public static void staticExitSocialClub() {
        //gtasaSelf.ExitSocialClub();
    }

    public void AfterDownloadFunction() {

    }

    public void EnterSocialClub() {

    }

    public void ExitSocialClub() {

    }

    public boolean ServiceAppCommand(String str, String str2)
    {
        return false;
    }

    public int ServiceAppCommandValue(String str, String str2)
    {
        return 0;
    }

    public native void main();

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

        //if(!SignatureChecker.isSignatureValid(this, getPackageName()))
        //{
        //Toast.makeText(this, "Use original launcher! No remake", Toast.LENGTH_LONG).show();
        //return;
        //}

        System.out.println("GTASA onCreate");
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

    public native void setCurrentScreenSize(int i, int i2);
}