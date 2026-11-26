package com.crestwood.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.os.SystemClock;
import android.util.AttributeSet;
import android.view.View;

import com.gta.game.R;

import java.text.NumberFormat;
import java.util.Calendar;
import java.util.Locale;
import java.util.TimeZone;

/**
 * [Crestwood v36] Panneau d'infos NATIF en haut a droite (remplace les pastilles HTML) :
 *   [lieu] [heure] [CrestCoins]
 *                    numero de compte
 *                    argent en poche
 *   etoiles de recherche (si recherche)
 * Tout est dessine dans une seule vue : un seul passage de dessin, seulement quand une valeur change.
 */
public class InfoPanelView extends View {
    private String location = "Los Santos", time = "00:00";
    private int coins = 0, account = 0, money = 0, wanted = 0;
    // [v47b] animations (memes couleurs) : l'argent defile vers la nouvelle valeur avec un petit "pop",
    //        le lieu apparait en fondu, les etoiles de recherche respirent
    private float shownMoney = 0;
    private int moneyFrom = 0;
    private long moneyAt = 0, locAt = 0;
    private boolean moneyInit = false;
    private boolean hasCoins = false;

    private final Paint pill = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint pillStroke = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint sep = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint text = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint small = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF rect = new RectF();
    private final Drawable icPin, icClock, icCoin, icId, icCash, icStar;
    private final NumberFormat nf = NumberFormat.getIntegerInstance(Locale.FRANCE);
    private final Runnable clockTick = new Runnable() {
        @Override public void run() { updateClock(); postDelayed(this, 5000); }
    };

    public InfoPanelView(Context c) { this(c, null); }
    public InfoPanelView(Context c, AttributeSet a) {
        super(c, a);
        setWillNotDraw(false);
        setClickable(false);
        setFocusable(false);
        pill.setColor(0x6614181F);
        pillStroke.setStyle(Paint.Style.STROKE);
        pillStroke.setColor(0x55FFFFFF);
        sep.setColor(0x40FFFFFF);
        text.setTypeface(Fonts.bold(c));
        text.setColor(0xFFFFFFFF);
        text.setShadowLayer(3f, 0, 1f, 0x99000000);
        small.setTypeface(Fonts.bold(c));
        small.setColor(0xFFFFFFFF);
        small.setTextAlign(Paint.Align.RIGHT);
        small.setShadowLayer(3f, 0, 1f, 0xAA000000);
        int teal = 0xFF6FE3D2;
        icPin = tint(c, R.drawable.cw_ic_pin, teal);
        icClock = tint(c, R.drawable.cw_ic_clock, teal);
        icCoin = tint(c, R.drawable.cw_ic_coin, teal);
        icId = tint(c, R.drawable.cw_ic_id, 0xFFDDE3EA);
        icCash = tint(c, R.drawable.cw_ic_cash, 0xFFDDE3EA);
        icStar = tint(c, R.drawable.cw_ic_star, 0xFFFFD23F);
        updateClock();
    }

    private static Drawable tint(Context c, int id, int color) {
        Drawable d = c.getDrawable(id);
        if (d != null) { d = d.mutate(); d.setTint(color); }
        return d;
    }

    @Override protected void onAttachedToWindow() { super.onAttachedToWindow(); removeCallbacks(clockTick); post(clockTick); }
    @Override protected void onDetachedFromWindow() { removeCallbacks(clockTick); super.onDetachedFromWindow(); }

    // ---------------------------------------------------------------- donnees
    public void setMoney(int v) {
        if (!moneyInit) { moneyInit = true; money = v; shownMoney = v; invalidate(); return; }
        if (money != v) { moneyFrom = Math.round(shownMoney); money = v; moneyAt = SystemClock.uptimeMillis(); invalidate(); }
    }
    public void setCoins(int v) { if (!hasCoins || coins != v) { coins = v; hasCoins = true; invalidate(); } }
    public void setAccount(int v) { if (v > 0 && account != v) { account = v; invalidate(); } }
    public void setWanted(int v) { v = Math.max(0, Math.min(6, v)); if (wanted != v) { wanted = v; invalidate(); } }
    public void setLocation(String v) {
        if (v == null || v.isEmpty() || v.equals(location)) return;
        boolean first = "Los Santos".equals(location) && locAt == 0 && !moneyInit;
        location = v;
        if (!first) locAt = SystemClock.uptimeMillis();
        invalidate();
    }

    private void updateClock() {
        Calendar cal = Calendar.getInstance(TimeZone.getTimeZone("Africa/Abidjan"));
        String t = String.format(Locale.FRANCE, "%02d:%02d", cal.get(Calendar.HOUR_OF_DAY), cal.get(Calendar.MINUTE));
        if (!t.equals(time)) { time = t; invalidate(); }
    }

    // ---------------------------------------------------------------- taille
    @Override protected void onMeasure(int wSpec, int hSpec) {
        int h = Math.min(getResources().getDisplayMetrics().heightPixels, getResources().getDisplayMetrics().widthPixels);
        setMeasuredDimension(Math.round(h * 0.9f), Math.round(h * 0.22f));
    }

    // ---------------------------------------------------------------- dessin (aligne a droite)
    @Override protected void onDraw(Canvas cv) {
        final float W = getWidth(), H = getHeight();
        final float ph = H * 0.30f;              // hauteur d'une pastille
        final float pad = ph * 0.42f, gap = ph * 0.22f, ic = ph * 0.52f;
        text.setTextSize(ph * 0.40f);
        float x = W;
        x = pillAt(cv, x, 0, ph, pad, ic, icCoin, hasCoins ? nf.format(coins) : "0");
        x = pillAt(cv, x - gap, 0, ph, pad, ic, icClock, time);
        long now = SystemClock.uptimeMillis();
        float la = locAt == 0 ? 1f : Math.min(1f, (now - locAt) / 350f);
        if (la < 1f) {
            cv.saveLayerAlpha(0, 0, x, ph, Math.round(255 * la));
            cv.translate(0, (1f - la) * -ph * 0.25f);
            pillAt(cv, x - gap, 0, ph, pad, ic, icPin, location);
            cv.restore();
            postInvalidateOnAnimation();
        } else pillAt(cv, x - gap, 0, ph, pad, ic, icPin, location);

        // lignes de droite : numero de compte, argent
        small.setTextSize(ph * 0.38f);
        float lineY = ph + ph * 0.62f;
        float rx = W - pad * 0.3f;
        if (account > 0) {
            line(cv, rx, lineY, ph * 0.42f, icId, nf.format(account));
            lineY += ph * 0.62f;
        }
        // argent : defile vers la nouvelle valeur (600 ms) + petit pop
        float mk = moneyAt == 0 ? 1f : Math.min(1f, (now - moneyAt) / 600f);
        float ease = 1f - (1f - mk) * (1f - mk) * (1f - mk);
        shownMoney = mk >= 1f ? money : moneyFrom + (money - moneyFrom) * ease;
        float pop = mk < 1f ? 1f + 0.14f * (float) Math.sin(mk * Math.PI) : 1f;
        if (pop != 1f) { cv.save(); cv.scale(pop, pop, rx, lineY - small.getTextSize() * 0.36f); }
        line(cv, rx, lineY, ph * 0.42f, icCash, nf.format(Math.round(shownMoney)) + " $");
        if (pop != 1f) cv.restore();
        if (mk < 1f) postInvalidateOnAnimation();
        lineY += ph * 0.62f;

        // etoiles de recherche
        if (wanted > 0 && icStar != null) {
            float s = ph * 0.46f;
            for (int i = 0; i < wanted; i++) {
                float cx = rx - s / 2f - i * (s * 1.1f);
                icStar.setBounds(Math.round(cx - s / 2f), Math.round(lineY - s * 0.8f), Math.round(cx + s / 2f), Math.round(lineY + s * 0.2f));
                // [v47b] les etoiles respirent (memes couleurs)
                float br = 1f + 0.12f * (float) Math.sin((now / 260.0) + i * 0.6);
                cv.save(); cv.scale(br, br, cx, lineY - s * 0.3f);
                icStar.draw(cv);
                cv.restore();
            }
            postInvalidateDelayed(90);   // [v48] opti : ~11 images/s suffisent pour la respiration des etoiles
        }
    }

    /** pastille ancree par son bord DROIT a x ; retourne le bord gauche */
    private float pillAt(Canvas cv, float right, float top, float h, float pad, float ic, Drawable icon, String label) {
        float tw = text.measureText(label);
        float w = pad + ic + pad * 0.9f + tw + pad;
        float left = right - w;
        float r = h * 0.32f;
        rect.set(left + 1, top + 1, right - 1, top + h - 1);
        cv.drawRoundRect(rect, r, r, pill);
        pillStroke.setStrokeWidth(Math.max(1f, h * 0.035f));
        cv.drawRoundRect(rect, r, r, pillStroke);
        if (icon != null) {
            float cx = left + pad + ic / 2f, cy = top + h / 2f;
            icon.setBounds(Math.round(cx - ic / 2f), Math.round(cy - ic / 2f), Math.round(cx + ic / 2f), Math.round(cy + ic / 2f));
            icon.draw(cv);
            float sx = left + pad + ic + pad * 0.45f;
            cv.drawRect(sx, top + h * 0.25f, sx + Math.max(1f, h * 0.03f), top + h * 0.75f, sep);
        }
        Paint.FontMetrics fm = text.getFontMetrics();
        cv.drawText(label, left + pad + ic + pad * 0.9f, top + h / 2f - (fm.ascent + fm.descent) / 2f, text);
        return left;
    }

    private void line(Canvas cv, float right, float baseline, float ic, Drawable icon, String label) {
        cv.drawText(label, right, baseline, small);
        float tw = small.measureText(label);
        if (icon != null) {
            float cx = right - tw - ic * 0.9f, cy = baseline - small.getTextSize() * 0.36f;
            icon.setBounds(Math.round(cx - ic / 2f), Math.round(cy - ic / 2f), Math.round(cx + ic / 2f), Math.round(cy + ic / 2f));
            icon.draw(cv);
        }
    }

    /** Ville / region de San Andreas d'apres la position (meme decoupage que les zones de ville SA-MP). */
    public static String cityAt(float x, float y) {
        if (x >= 44.6f && x <= 2997.06f && y >= -2892.97f && y <= -768.03f) return "Los Santos";
        if (x >= 869.46f && x <= 2997.06f && y >= 596.35f && y <= 2993.87f) return "Las Venturas";
        if (x >= -2997.47f && x <= -1213.91f && y >= -1115.58f && y <= 1659.68f) return "San Fierro";
        if (x >= -1213.91f && x <= 2997.06f && y >= -768.03f && y <= 596.35f) return "Red County";
        if (x >= -1213.91f && x <= 44.6f && y >= -2892.97f && y <= -768.03f) return "Flint County";
        if (x >= -2997.47f && x <= -1213.91f && y >= -2892.97f && y <= -1115.58f) return "Whetstone";
        if (x >= -480.53f && x <= 869.46f && y >= 596.35f && y <= 2993.87f) return "Bone County";
        if (x >= -2997.47f && x <= -480.53f && y >= 1659.68f && y <= 2993.87f) return "Tierra Robada";
        if (x >= -1213.91f && x <= -242.98f && y >= 596.35f && y <= 1659.68f) return "Tierra Robada";
        return "San Andreas";
    }
}
