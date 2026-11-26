package com.crestwood.ui;

import android.content.Context;
import android.graphics.Typeface;

import com.gta.game.R;

/** [Crestwood v36] polices du HUD natif (res/font), chargees une seule fois. */
final class Fonts {
    private static Typeface sBlack, sBold, sMedium;
    private Fonts() { }

    static Typeface black(Context c) {
        if (sBlack == null) sBlack = load(c, R.font.montserrat_black, Typeface.DEFAULT_BOLD);
        return sBlack;
    }
    static Typeface bold(Context c) {
        if (sBold == null) sBold = load(c, R.font.montserrat_bold, Typeface.DEFAULT_BOLD);
        return sBold;
    }
    static Typeface medium(Context c) {
        if (sMedium == null) sMedium = load(c, R.font.montserrat_medium, Typeface.DEFAULT);
        return sMedium;
    }
    private static Typeface load(Context c, int id, Typeface fallback) {
        try { Typeface t = c.getResources().getFont(id); return t != null ? t : fallback; }
        catch (Throwable e) { return fallback; }
    }
}
