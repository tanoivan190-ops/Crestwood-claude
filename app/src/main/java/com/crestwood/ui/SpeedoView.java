package com.crestwood.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.os.SystemClock;
import android.util.AttributeSet;
import android.view.View;

import com.gta.game.R;

/**
 * [Crestwood v36] Compteur de vitesse NATIF (dessine par Android, plus de page HTML dans un iframe).
 * Meme disposition que l'ancien compteur "original-speedometer" :
 *  - grand arc de vitesse (graduations), vitesse en gros au centre + "KM/H"
 *  - arc jaune a gauche = carburant (icone pompe), arc vert a droite = etat du vehicule (icone moteur)
 *  - ceinture rouge quand elle n'est pas attachee
 *  - 5 tuiles en dessous : vie, faim, soif, energie, gilet (remplies selon la valeur)
 * Taille : largeur = 0,555 x hauteur de l'ecran (comme l'ancien), proportions du dessin d'origine (900 x 520).
 */
public class SpeedoView extends View {
    private static final float MAX_KMH = 240f;

    // valeurs cibles (posees par NativeHud) et valeurs affichees (animees)
    private float speed, shownSpeed, fuel = 100, shownFuel = 100, vehHealth = 100, shownVeh = 100;
    private final float[] stat = {100, 100, 100, 100, 0};          // vie, faim, soif, energie, gilet
    private final float[] shownStat = {100, 100, 100, 100, 0};
    private boolean belt = true, engineOn = true;
    private long lastFrame;

    private final Paint track = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint prog = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint glow = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint tick = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint speedText = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint unitText = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint tileBg = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint tileFill = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF rect = new RectF();
    private final Path path = new Path();

    private static final int[] TILE_COLORS = {
            0xFFE5233F,   // vie
            0xFFF07A1E,   // faim
            0xFF2A8CF0,   // soif
            0xFFF2C21D,   // energie
            0xFF7A4CF0    // gilet
    };
    private final Drawable[] tileIcons = new Drawable[5];
    private Drawable icFuel, icEngine, icBelt;

    public SpeedoView(Context c) { this(c, null); }
    public SpeedoView(Context c, AttributeSet a) {
        super(c, a);
        setWillNotDraw(false);
        setClickable(false);
        setFocusable(false);
        track.setStyle(Paint.Style.STROKE);
        track.setStrokeCap(Paint.Cap.ROUND);
        track.setColor(0x8C1E1A16);
        prog.setStyle(Paint.Style.STROKE);
        prog.setStrokeCap(Paint.Cap.ROUND);
        glow.setStyle(Paint.Style.STROKE);
        glow.setStrokeCap(Paint.Cap.ROUND);
        tick.setStyle(Paint.Style.STROKE);
        tick.setStrokeCap(Paint.Cap.ROUND);
        Typeface black = Fonts.black(c);
        speedText.setTypeface(black);
        speedText.setColor(Color.WHITE);
        speedText.setTextAlign(Paint.Align.CENTER);
        speedText.setTextSkewX(-0.18f);
        speedText.setShadowLayer(6f, 0f, 2f, 0x99000000);
        unitText.setTypeface(Fonts.bold(c));
        unitText.setColor(0xFFDCE6F2);
        unitText.setTextAlign(Paint.Align.CENTER);
        unitText.setLetterSpacing(0.35f);
        unitText.setShadowLayer(4f, 0f, 1f, 0x99000000);
        tileBg.setColor(0x8C2A2622);
        int[] ids = {R.drawable.cw_ic_heart, R.drawable.cw_ic_burger, R.drawable.cw_ic_water, R.drawable.cw_ic_bolt, R.drawable.cw_ic_shield};
        for (int i = 0; i < 5; i++) tileIcons[i] = c.getDrawable(ids[i]);
        icFuel = tinted(c, R.drawable.cw_ic_fuel, 0xFFF2C14A);
        icEngine = tinted(c, R.drawable.cw_ic_engine, 0xFF4ADE6B);
        icBelt = tinted(c, R.drawable.cw_ic_belt, 0xFFE5394B);
    }

    private static Drawable tinted(Context c, int id, int color) {
        Drawable d = c.getDrawable(id);
        if (d != null) { d = d.mutate(); d.setTint(color); }
        return d;
    }

    // ---------------------------------------------------------------- donnees
    public void setDriving(float kmh, float fuelPct, float vehPct) {
        speed = Math.max(0, kmh);
        fuel = clamp(fuelPct);
        vehHealth = clamp(vehPct);
        postInvalidateOnAnimation();
    }
    public void setStats(float health, float hunger, float thirst, float energy, float armour) {
        stat[0] = clamp(health); stat[1] = clamp(hunger); stat[2] = clamp(thirst); stat[3] = clamp(energy); stat[4] = clamp(armour);
        postInvalidateOnAnimation();
    }
    public void setBelt(boolean on) { if (belt != on) { belt = on; invalidate(); } }
    public void setEngine(boolean on) { if (engineOn != on) { engineOn = on; invalidate(); } }

    private static float clamp(float v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }

    // ---------------------------------------------------------------- taille
    @Override protected void onMeasure(int wSpec, int hSpec) {
        int screenH = getResources().getDisplayMetrics().heightPixels;
        int screenW = getResources().getDisplayMetrics().widthPixels;
        int h = Math.min(screenH, screenW);      // paysage : hauteur = petit cote
        int w = Math.round(h * 0.555f);
        setMeasuredDimension(w, Math.round(w * 520f / 900f));
    }

    // ---------------------------------------------------------------- dessin
    @Override protected void onDraw(Canvas cv) {
        long now = SystemClock.uptimeMillis();
        float dt = lastFrame == 0 ? 0.016f : Math.min(0.1f, (now - lastFrame) / 1000f);
        lastFrame = now;
        float k = 1f - (float) Math.exp(-dt * 12f);
        boolean moving = false;
        shownSpeed += (speed - shownSpeed) * k; if (Math.abs(speed - shownSpeed) > 0.4f) moving = true; else shownSpeed = speed;
        shownFuel += (fuel - shownFuel) * k; if (Math.abs(fuel - shownFuel) > 0.3f) moving = true; else shownFuel = fuel;
        shownVeh += (vehHealth - shownVeh) * k; if (Math.abs(vehHealth - shownVeh) > 0.3f) moving = true; else shownVeh = vehHealth;
        for (int i = 0; i < 5; i++) {
            shownStat[i] += (stat[i] - shownStat[i]) * k;
            if (Math.abs(stat[i] - shownStat[i]) > 0.3f) moving = true; else shownStat[i] = stat[i];
        }

        final float W = getWidth(), H = getHeight();
        final float u = W / 900f;                // unite : le dessin d'origine fait 900 x 520
        final float cx = W * 0.478f, cy = 330f * u;

        // ---- grand arc de vitesse
        float r = 255f * u;
        rect.set(cx - r, cy - r, cx + r, cy + r);
        float start = 172f, sweep = 196f;
        track.setStrokeWidth(26f * u);
        cv.drawArc(rect, start, sweep, false, track);
        float frac = Math.min(1f, shownSpeed / MAX_KMH);
        if (frac > 0.002f) {
            halo(cv, start, sweep * frac, 26f * u, 0xFFFFFFFF);
            prog.setStrokeWidth(26f * u);
            prog.setColor(0xFFF4F8FF);
            cv.drawArc(rect, start, sweep * frac, false, prog);
        }
        // graduations a l'exterieur de l'arc
        tick.setColor(0xCCFFFFFF);
        for (int i = 0; i <= 8; i++) {
            double a = Math.toRadians(start + sweep * i / 8f);
            boolean major = i == 4;
            float r0 = r + 24f * u, r1 = r + (major ? 50f : 36f) * u;
            tick.setStrokeWidth((major ? 6f : 4f) * u);
            cv.drawLine(cx + (float) Math.cos(a) * r0, cy + (float) Math.sin(a) * r0,
                    cx + (float) Math.cos(a) * r1, cy + (float) Math.sin(a) * r1, tick);
        }

        // ---- carburant (gauche, jaune) et etat du vehicule (droite, vert)
        sideArc(cv, cx, cy, 308f * u, 179f, 43f, shownFuel / 100f, 0xFFFFC23A, true, u);
        sideArc(cv, cx, cy, 308f * u, 361f, -43f, shownVeh / 100f, shownVeh < 30 ? 0xFFFF4D4D : 0xFF4ADE6B, false, u);
        drawIcon(cv, icFuel, cx - 370f * u, cy - 20f * u, 44f * u);
        if (icEngine != null) icEngine.setTint(engineOn ? (shownVeh < 30 ? 0xFFFF4D4D : 0xFF4ADE6B) : 0xFF8A8F98);
        drawIcon(cv, icEngine, cx + 368f * u, cy - 20f * u, 44f * u);

        // ---- vitesse
        unitText.setTextSize(30f * u);
        cv.drawText("KM/H", cx, cy - 135f * u, unitText);
        speedText.setTextSize(140f * u);
        cv.drawText(String.valueOf(Math.round(shownSpeed)), cx, cy - 10f * u, speedText);
        if (!belt) drawIcon(cv, icBelt, cx + 165f * u, cy - 55f * u, 54f * u);

        // ---- tuiles d'etat
        float tw = 115f * u, th = 100f * u, gap = 23f * u;
        float total = 5 * tw + 4 * gap;
        float x0 = cx - total / 2f, ty = cy + 32f * u;
        for (int i = 0; i < 5; i++) {
            float tx = x0 + i * (tw + gap);
            tile(cv, tx, ty, tw, th, shownStat[i] / 100f, TILE_COLORS[i], tileIcons[i], u);
        }

        if (moving) postInvalidateOnAnimation(); else lastFrame = 0;
    }

    private void sideArc(Canvas cv, float cx, float cy, float r, float start, float sweep, float frac, int color, boolean left, float u) {
        rect.set(cx - r, cy - r, cx + r, cy + r);
        track.setStrokeWidth(22f * u);
        cv.drawArc(rect, start, sweep, false, track);
        if (frac > 0.01f) {
            // la jauge se remplit depuis le bas de l'arc
            float sw = sweep * frac;
            halo(cv, start, sw, 22f * u, color);
            prog.setStrokeWidth(22f * u);
            prog.setColor(color);
            cv.drawArc(rect, start, sw, false, prog);
        }
        // petites graduations a l'interieur
        tick.setStrokeWidth(4f * u);
        tick.setColor(left ? 0x88FFFFFF : 0x99FF5A5A);
        for (int i = 1; i < 4; i++) {
            double a = Math.toRadians(start + sweep * i / 4f);
            float r0 = r - 30f * u, r1 = r - 42f * u;
            cv.drawLine(cx + (float) Math.cos(a) * r0, cy + (float) Math.sin(a) * r0,
                    cx + (float) Math.cos(a) * r1, cy + (float) Math.sin(a) * r1, tick);
        }
    }

    /** lueur douce autour d'un arc (2 traits larges transparents : pas de flou, rapide sur le GPU) */
    private void halo(Canvas cv, float start, float sweep, float width, int color) {
        int rgb = color & 0x00FFFFFF;
        glow.setStrokeWidth(width * 1.9f);
        glow.setColor(rgb | 0x1A000000);
        cv.drawArc(rect, start, sweep, false, glow);
        glow.setStrokeWidth(width * 1.4f);
        glow.setColor(rgb | 0x33000000);
        cv.drawArc(rect, start, sweep, false, glow);
    }

    private void tile(Canvas cv, float x, float y, float w, float h, float frac, int color, Drawable icon, float u) {
        // tuile legerement penchee (parallelogramme arrondi), fond sombre, remplissage depuis le bas
        float skew = 10f * u, rad = 22f * u;
        cv.save();
        cv.skew(-0.12f, 0f);
        float sx = x + (y + h) * 0.12f;
        rect.set(sx, y, sx + w, y + h);
        cv.drawRoundRect(rect, rad, rad, tileBg);
        if (frac > 0.005f) {
            cv.save();
            cv.clipRect(sx, y + h * (1f - frac), sx + w, y + h);
            tileFill.setColor(color);
            cv.drawRoundRect(rect, rad, rad, tileFill);
            cv.restore();
        }
        cv.restore();
        drawIcon(cv, icon, x + w / 2f + skew * 0.2f, y + h / 2f, h * 0.42f);
    }

    private static void drawIcon(Canvas cv, Drawable d, float cx, float cy, float size) {
        if (d == null) return;
        int iw = d.getIntrinsicWidth(), ih = d.getIntrinsicHeight();
        float sc = size / Math.max(1, Math.max(iw, ih));
        int w = Math.round(iw * sc), h = Math.round(ih * sc);
        d.setBounds(Math.round(cx - w / 2f), Math.round(cy - h / 2f), Math.round(cx + w / 2f), Math.round(cy + h / 2f));
        d.draw(cv);
    }
}
