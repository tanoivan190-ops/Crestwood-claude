package com.gta.game;

import android.content.Context;
import android.content.SharedPreferences;
import android.view.View;

/**
 * Compatibilite WebView : sur certains telephones (GPU PowerVR/Mali de milieu de gamme, ex. TECNO), le moteur
 * de rendu de la WebView plante en boucle avec une couche materielle transparente posee sur le jeu OpenGL.
 * Apres un premier plantage du moteur de rendu, on passe en rendu logiciel, et on s'en souvient pour les
 * lancements suivants (plus lent, mais stable).
 */
public final class WebCompat {
    private static final String PREFS = "crestwood_webcompat";
    private static final String KEY = "software_layer";
    private static volatile Boolean software;

    private WebCompat() {}

    public static boolean useSoftware(Context context) {
        if (software == null) {
            boolean value = false;
            try {
                SharedPreferences prefs = context.getApplicationContext().getSharedPreferences(PREFS, Context.MODE_PRIVATE);
                value = prefs.getBoolean(KEY, false);
            } catch (Throwable ignored) { }
            software = value;
        }
        return software;
    }

    /** A appeler quand le moteur de rendu de la WebView vient de planter (didCrash). */
    public static void onRendererCrash(Context context) {
        software = Boolean.TRUE;
        try {
            context.getApplicationContext().getSharedPreferences(PREFS, Context.MODE_PRIVATE)
                    .edit().putBoolean(KEY, true).apply();
        } catch (Throwable ignored) { }
    }

    /** Applique le type de couche adapte (materiel par defaut, logiciel apres un plantage du rendu). */
    public static void apply(View view) {
        view.setLayerType(useSoftware(view.getContext()) ? View.LAYER_TYPE_SOFTWARE : View.LAYER_TYPE_HARDWARE, null);
    }
}
