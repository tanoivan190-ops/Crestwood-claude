package com.gta.game.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Shader;
import android.view.View;

/**
 * Silhouette vectorielle d'un vehicule (voiture, moto, bateau, camion, avion), peinte avec
 * la couleur SA du vehicule (c1) : degrade + reflet + liseré bleu Crestwood + ombre au sol.
 * Dessin dans un repere 100 x 50, mis a l'echelle de la vue.
 */
public class VehicleSilhouetteView extends View {

    public static final int CAR = 0, BIKE = 1, BOAT = 2, TRUCK = 3, AIR = 4;

    private int kind = CAR;
    private int color = 0xFF5A5F68;
    private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint rim = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint glass = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint wheel = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint hub = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint shadow = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path path = new Path();

    public VehicleSilhouetteView(Context c) {
        super(c);
        rim.setStyle(Paint.Style.STROKE);
        rim.setStrokeJoin(Paint.Join.ROUND);
        rim.setStrokeCap(Paint.Cap.ROUND);
        glass.setColor(0xCC0E1116);
        wheel.setColor(0xFF0B0C0F);
        hub.setStyle(Paint.Style.STROKE);
        hub.setColor(0xFF8B93A1);
        shadow.setColor(0x66000000);
    }

    public void set(int kind, int color) {
        this.kind = kind;
        this.color = color | 0xFF000000;
        invalidate();
    }

    public static int kindOf(int model) {
        for (int m : BIKES) if (m == model) return BIKE;
        for (int m : BOATS) if (m == model) return BOAT;
        for (int m : AIRS) if (m == model) return AIR;
        for (int m : TRUCKS) if (m == model) return TRUCK;
        return CAR;
    }

    static final int[] BIKES = {448, 461, 462, 463, 468, 471, 481, 509, 510, 521, 522, 523, 581, 586};
    static final int[] BOATS = {430, 446, 452, 453, 454, 472, 473, 484, 493, 595};
    static final int[] AIRS = {417, 425, 447, 460, 469, 476, 487, 488, 497, 511, 512, 513, 519, 520, 548, 553, 563, 577, 592, 593};
    static final int[] TRUCKS = {403, 406, 407, 408, 413, 414, 416, 423, 427, 428, 431, 433, 437, 440, 443, 455, 456, 459, 482, 483, 498, 499, 508, 514, 515, 524, 525, 528, 532, 544, 552, 573, 578, 582, 588, 601, 609};

    private float sx, sy, ox, oy;

    private float X(float x) { return ox + x * sx; }
    private float Y(float y) { return oy + y * sy; }

    private void poly(float... p) {
        path.reset();
        path.moveTo(X(p[0]), Y(p[1]));
        for (int i = 2; i + 1 < p.length; i += 2) path.lineTo(X(p[i]), Y(p[i + 1]));
        path.close();
    }

    private static int shade(int c, float k) {
        int r = Math.min(255, Math.max(0, Math.round(Color.red(c) * k)));
        int g = Math.min(255, Math.max(0, Math.round(Color.green(c) * k)));
        int b = Math.min(255, Math.max(0, Math.round(Color.blue(c) * k)));
        return Color.argb(255, r, g, b);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        float w = getWidth(), h = getHeight();
        if (w <= 0 || h <= 0) return;
        // repere 100 x 50 centre, proportions conservees
        float s = Math.min(w / 100f, h / 50f);
        sx = s; sy = s;
        ox = (w - 100f * s) / 2f;
        oy = (h - 50f * s) / 2f;

        // ombre au sol
        canvas.drawOval(X(8), Y(42), X(92), Y(48), shadow);

        fill.setShader(new LinearGradient(0, Y(8), 0, Y(42),
                new int[]{shade(color, 1.35f), color, shade(color, 0.55f)}, new float[]{0f, 0.45f, 1f}, Shader.TileMode.CLAMP));
        rim.setStrokeWidth(Math.max(1.2f, s * 0.9f));
        rim.setColor(0xE63B82FF);
        hub.setStrokeWidth(Math.max(1f, s * 0.8f));

        switch (kind) {
            case BIKE: drawBike(canvas, s); break;
            case BOAT: drawBoat(canvas); break;
            case TRUCK: drawTruck(canvas, s); break;
            case AIR: drawAir(canvas); break;
            default: drawCar(canvas, s); break;
        }
    }

    private void wheelAt(Canvas c, float x, float y, float r, float s) {
        c.drawCircle(X(x), Y(y), r * s, wheel);
        c.drawCircle(X(x), Y(y), r * s * 0.55f, hub);
    }

    private void drawCar(Canvas c, float s) {
        poly(4, 36, 7, 28, 20, 25, 33, 15, 63, 14, 77, 24, 92, 27, 97, 33, 97, 38, 4, 38);
        c.drawPath(path, fill);
        c.drawPath(path, rim);
        poly(36, 17, 61, 16.5f, 71, 24, 31, 24.5f);
        c.drawPath(path, glass);
        // reflet
        Paint hl = new Paint(Paint.ANTI_ALIAS_FLAG);
        hl.setColor(0x33FFFFFF);
        poly(10, 29, 22, 27, 88, 28.5f, 92, 30, 10, 31);
        c.drawPath(path, hl);
        wheelAt(c, 24, 38, 7f, s);
        wheelAt(c, 78, 38, 7f, s);
    }

    private void drawBike(Canvas c, float s) {
        Paint st = new Paint(Paint.ANTI_ALIAS_FLAG);
        st.setStyle(Paint.Style.STROKE);
        st.setStrokeCap(Paint.Cap.ROUND);
        st.setStrokeWidth(3.2f * s);
        st.setColor(color);
        wheelAt(c, 22, 35, 10f, s);
        wheelAt(c, 78, 35, 10f, s);
        path.reset();
        path.moveTo(X(22), Y(35)); path.lineTo(X(42), Y(22)); path.lineTo(X(64), Y(22)); path.lineTo(X(78), Y(35));
        path.moveTo(X(64), Y(22)); path.lineTo(X(71), Y(11));
        c.drawPath(path, st);
        poly(36, 18, 58, 17, 62, 22, 40, 23);
        c.drawPath(path, fill);
        c.drawPath(path, rim);
        Paint bar = new Paint(Paint.ANTI_ALIAS_FLAG);
        bar.setStyle(Paint.Style.STROKE);
        bar.setStrokeWidth(2.2f * s);
        bar.setStrokeCap(Paint.Cap.ROUND);
        bar.setColor(0xFF3B82FF);
        c.drawLine(X(67), Y(11), X(76), Y(10), bar);
    }

    private void drawBoat(Canvas c) {
        poly(26, 14, 58, 14, 66, 26, 22, 26);
        c.drawPath(path, glass);
        poly(3, 24, 97, 24, 86, 40, 14, 40);
        c.drawPath(path, fill);
        c.drawPath(path, rim);
        Paint wave = new Paint(Paint.ANTI_ALIAS_FLAG);
        wave.setStyle(Paint.Style.STROKE);
        wave.setStrokeWidth(Math.max(1f, sx * 0.8f));
        wave.setColor(0x803B82FF);
        c.drawLine(X(8), Y(44), X(92), Y(44), wave);
    }

    private void drawTruck(Canvas c, float s) {
        poly(4, 8, 64, 8, 64, 37, 4, 37);
        c.drawPath(path, fill);
        c.drawPath(path, rim);
        poly(66, 14, 84, 14, 95, 26, 95, 37, 66, 37);
        c.drawPath(path, fill);
        c.drawPath(path, rim);
        poly(70, 17, 82, 17, 89, 26, 70, 26);
        c.drawPath(path, glass);
        wheelAt(c, 18, 39, 6f, s);
        wheelAt(c, 48, 39, 6f, s);
        wheelAt(c, 82, 39, 6f, s);
    }

    private void drawAir(Canvas c) {
        poly(42, 27, 62, 27, 50, 4, 44, 4);
        c.drawPath(path, fill);
        c.drawPath(path, rim);
        poly(42, 29, 62, 29, 50, 47, 44, 47);
        c.drawPath(path, fill);
        c.drawPath(path, rim);
        poly(4, 26, 12, 23, 86, 23, 97, 27, 86, 31, 12, 31);
        c.drawPath(path, fill);
        c.drawPath(path, rim);
        poly(4, 26, 10, 12, 17, 12, 16, 24);
        c.drawPath(path, fill);
        c.drawPath(path, rim);
        poly(80, 24, 90, 25, 90, 27, 80, 27);
        c.drawPath(path, glass);
    }

    // ---- palette SA-MP (couleurs 0..126) ----
    private static final int[] SA = {
            0x000000, 0xF5F5F5, 0x2A77A1, 0x840410, 0x263739, 0x86446E, 0xD78E10, 0x4C75B7, 0xBDBEC6, 0x5E7072,
            0x46597A, 0x656A79, 0x5D7E8D, 0x58595A, 0xD6DAD6, 0x9CA1A3, 0x335F3F, 0x730E1A, 0x7B0A2A, 0x9F9D94,
            0x3B4E78, 0x732E3E, 0x691E3B, 0x96918C, 0x515459, 0x3F3E45, 0xA5A9A7, 0x635C5A, 0x3D4A68, 0x979592,
            0x421F21, 0x5F272B, 0x8494AB, 0x767B7C, 0x646464, 0x5A5752, 0x252527, 0x2D3A35, 0x93A396, 0x6D7A88,
            0x221918, 0x6F675F, 0x7C1C2A, 0x5F0A15, 0x193826, 0x5D1B20, 0x9D9872, 0x7A7560, 0x989586, 0xADB0B0,
            0x848988, 0x304F45, 0x4D6268, 0x162248, 0x272F4B, 0x7D6256, 0x9EA4AB, 0x9C8D71, 0x6D1822, 0x4E6881,
            0x9C9C98, 0x917347, 0x661C26, 0x949D9F, 0xA4A7A5, 0x8E8C46, 0x341A1E, 0x6A7A8C, 0xAAAD8E, 0xAB988F,
            0x851F2E, 0x6F8297, 0x585853, 0x9AA790, 0x601A23, 0x20202C, 0xA4A096, 0xAA9D84, 0x78222B, 0x0E316D,
            0x722A3F, 0x7B715E, 0x741D28, 0x1E2E32, 0x4D322F, 0x7C1B44, 0x2E5B20, 0x395A83, 0x6D2837, 0xA7A28F,
            0xAFB1B1, 0x364155, 0x6D6C6E, 0x0F6A89, 0x204B6B, 0x2B3E57, 0x9B9F9D, 0x6C8495, 0x4D8495, 0xAE9B7F,
            0x406C8F, 0x1F253B, 0xAB9276, 0x134573, 0x96816C, 0x64686A, 0x105082, 0xA19983, 0x385694, 0x525661,
            0x7F6956, 0x8C929A, 0x596E87, 0x473532, 0x44624F, 0x730A27, 0x223457, 0x640D1B, 0xA3ADC6, 0x695853,
            0x9B8B80, 0x620B1C, 0x5B5D5E, 0x624428, 0x731827, 0x1B376D, 0xEC6AAE,
    };

    /** Couleur ARGB approximative d'une couleur SA (0..255). */
    public static int saColor(int id) {
        if (id >= 0 && id < SA.length) return 0xFF000000 | SA[id];
        return 0xFF5A5F68;
    }
}
