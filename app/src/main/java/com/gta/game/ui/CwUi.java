package com.gta.game.ui;

import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.StateListDrawable;
import android.util.TypedValue;
import android.view.Gravity;
import android.widget.TextView;

import androidx.core.content.res.ResourcesCompat;

import com.gta.game.R;

/**
 * Charte graphique native Crestwood (panneaux Android sans WebView) :
 * verre sombre un peu transparent, coins presque carres, filets fins, accent bleu.
 * Partagee par le client 64 bits (com.gta.game) et 32 bits (com.gta.game32).
 */
public final class CwUi {
    private CwUi() {}

    public static final int BG_PANEL   = 0xD60A0B0E;   // verre sombre (84 %)
    public static final int BG_CARD    = 0x1FFFFFFF;
    public static final int BG_CARD_ON = 0x33FFFFFF;
    public static final int LINE       = 0x26FFFFFF;
    public static final int TEXT       = 0xFFF3F4F6;
    public static final int MUTED      = 0xFF9AA0A8;
    public static final int ACCENT       = 0xFF3B82FF;
    public static final int ACCENT_DARK  = 0xFF1E4FA8;
    public static final int RADIUS_DP  = 12;   // [v21] coins doux (avant 3 : "trop carre")

    private static Typeface sBold, sBlack, sMedium, sRegular;

    public static int dp(Context c, float v) {
        return Math.round(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, v, c.getResources().getDisplayMetrics()));
    }

    private static Typeface font(Context c, int id, Typeface fallback) {
        try {
            Typeface t = ResourcesCompat.getFont(c, id);
            return t != null ? t : fallback;
        } catch (Throwable e) {
            return fallback;
        }
    }

    public static Typeface bold(Context c) {
        if (sBold == null) sBold = font(c, R.font.montserrat_bold, Typeface.DEFAULT_BOLD);
        return sBold;
    }

    public static Typeface black(Context c) {
        if (sBlack == null) sBlack = font(c, R.font.montserrat_black, Typeface.DEFAULT_BOLD);
        return sBlack;
    }

    public static Typeface medium(Context c) {
        if (sMedium == null) sMedium = font(c, R.font.montserrat_medium, Typeface.DEFAULT);
        return sMedium;
    }

    public static Typeface regular(Context c) {
        if (sRegular == null) sRegular = font(c, R.font.montserrat_regular, Typeface.DEFAULT);
        return sRegular;
    }

    public static GradientDrawable box(Context c, int fill, int stroke) {
        GradientDrawable g = new GradientDrawable();
        g.setColor(fill);
        g.setCornerRadius(dp(c, RADIUS_DP));
        if (stroke != 0) g.setStroke(Math.max(1, dp(c, 1)), stroke);
        return g;
    }

    /** Fond de bouton avec etat appuye. */
    public static StateListDrawable button(Context c, int fill, int pressed, int stroke) {
        StateListDrawable s = new StateListDrawable();
        s.addState(new int[]{android.R.attr.state_pressed}, box(c, pressed, stroke));
        s.addState(new int[]{-android.R.attr.state_enabled}, box(c, 0x0FFFFFFF, 0x14FFFFFF));
        s.addState(new int[]{}, box(c, fill, stroke));
        return s;
    }

    public static TextView text(Context c, String s, float sp, int color, Typeface tf) {
        TextView t = new TextView(c);
        t.setText(s);
        t.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        t.setTextColor(color);
        t.setTypeface(tf);
        t.setIncludeFontPadding(false);
        return t;
    }

    /** Petit sur-titre en capitales espacees ("CRESTWOOD · STUDIO"). */
    public static TextView eyebrow(Context c, String s) {
        TextView t = text(c, s.toUpperCase(), 9.5f, MUTED, bold(c));
        t.setLetterSpacing(0.28f);
        return t;
    }

    public static TextView chip(Context c, String s, boolean on) {
        TextView t = text(c, s, 11f, on ? 0xFF0B0B0D : TEXT, bold(c));
        t.setGravity(Gravity.CENTER);
        t.setLetterSpacing(0.06f);
        int px = dp(c, 12), py = dp(c, 8);
        t.setPadding(px, py, px, py);
        t.setBackground(on ? button(c, TEXT, 0xFFD6D8DC, 0) : button(c, BG_CARD, BG_CARD_ON, LINE));
        t.setClickable(true);
        return t;
    }

    public static TextView primaryButton(Context c, String s) {
        TextView t = text(c, s, 12.5f, 0xFFFFFFFF, black(c));
        t.setGravity(Gravity.CENTER);
        t.setLetterSpacing(0.12f);
        t.setBackground(button(c, ACCENT, 0xFF2F6BD6, 0));
        t.setClickable(true);
        return t;
    }

    public static TextView ghostButton(Context c, String s) {
        TextView t = text(c, s, 12f, TEXT, bold(c));
        t.setGravity(Gravity.CENTER);
        t.setLetterSpacing(0.1f);
        t.setBackground(button(c, 0x00000000, BG_CARD_ON, 0x40FFFFFF));
        t.setClickable(true);
        return t;
    }

    public static int alpha(int color, float a) {
        return Color.argb(Math.round(Math.max(0f, Math.min(1f, a)) * 255f), Color.red(color), Color.green(color), Color.blue(color));
    }
}
