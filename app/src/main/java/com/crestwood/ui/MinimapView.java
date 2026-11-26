package com.crestwood.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.DashPathEffect;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RadialGradient;
import android.graphics.RectF;
import android.graphics.Shader;
import android.graphics.drawable.Drawable;
import android.os.SystemClock;
import android.util.AttributeSet;
import android.view.View;

import com.gta.game.R;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * [Crestwood v37] Minimap NATIVE (port exact de hud/minimap.js + hud/hud-ring.js) :
 *  - carte ronde qui tourne avec la camera (tuiles hud/radar/tiles, 12 x 12, 500 unites par tuile)
 *  - icones du serveur (hud/radar/blips), destination GPS + itineraire bleu sur les routes (CrestGps)
 *  - fleche du joueur, repere du nord, distance restante
 *  - jauges en arc autour (vie, gilet, soif, faim, energie) avec leurs icones
 * Toucher la carte = grande carte. Le dessin s'arrete quand rien ne bouge (~30 images/s max).
 */
public class MinimapView extends View {
    public interface Listener {
        void onMinimapTap();
        void onGpsRoute(String data);   // itineraire -> chevrons 3D du moteur ("tx,ty;x,y;...")
        void onGpsArrived();
    }

    private static final int GRID = 12, TILE = 500, HALF = 3000, MAX_TILES = 40;
    private static final float FOOT_SPAN = 260, VEH_SPAN = 260, SPEED_GAIN = 1.6f;
    private static final int SEA = 0xFF0B0B0D, ROUTE = 0xFF3B82FF, ROUTE_EDGE = 0xEB060E20, ROUTE_CORE = 0xF2BED7FF;
    private static final float OFF_ROUTE = 32, ARRIVED = 30;
    private static final long RECALC_MS = 1500, MIN_FRAME_MS = 33;

    // jauges (ordre, angles en degres depuis midi, sens horaire) : comme hud-ring.js
    private static final int[] ARC_COLORS = {0xFFFF4054, 0xFFDFE8FF, 0xFF36A6FF, 0xFFFFA526, 0xFFFFD84A};
    private static final float START = 32, END = 214, GAP = 5;

    private final float dp;
    private Listener listener;

    // etat camera / joueur
    private float tx, ty, tfx = 0, tfy = 1, thx = 0, thy = 1, tspan = FOOT_SPAN;
    private float cx, cy, cfx = 0, cfy = 1, chx = 0, chy = 1, cspan = FOOT_SPAN;
    private boolean hasData;
    private long lastFrame;
    private float manualSpan = 0;

    // jauges : vie, gilet, soif, faim, energie
    private final float[] stat = {100, 0, 100, 100, 100};
    private final float[] shown = {100, 0, 100, 100, 100};

    // blips / GPS
    private static final class Blip { float x, y; int icon; }
    private final ArrayList<Blip> blips = new ArrayList<>();
    private float[] waypoint;      // x, y
    private boolean waypointAuto;
    private CrestGps.Route route;
    private int routeIdx;
    private float routeLeft;
    private final CrestGps.Progress prog = new CrestGps.Progress();
    private boolean hasProj;
    private long routeAt;
    private volatile boolean routePending;
    private String gpsSentKey = "";

    // images
    private final LinkedHashMap<Integer, Bitmap> tiles = new LinkedHashMap<>(48, 0.75f, true);
    private final HashMap<Integer, Bitmap> blipIcons = new HashMap<>();
    private final HashSet<String> loading = new HashSet<>();
    private final HashSet<String> failed = new HashSet<>();
    private static final ExecutorService IO = Executors.newSingleThreadExecutor(r -> {
        Thread t = new Thread(r, "cw-minimap"); t.setPriority(Thread.MIN_PRIORITY); return t;
    });

    private final Paint bmpPaint = new Paint(Paint.FILTER_BITMAP_FLAG | Paint.ANTI_ALIAS_FLAG);
    private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint stroke = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint text = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint vignette = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path path = new Path();
    private final Path clip = new Path();
    private final Matrix mtx = new Matrix();
    private final float[] mv = new float[9];
    private final RectF rect = new RectF();
    private final Drawable[] arcIcons = new Drawable[5];
    private int vignetteFor = -1;

    public MinimapView(Context c) { this(c, null); }
    public MinimapView(Context c, AttributeSet a) {
        super(c, a);
        dp = c.getResources().getDisplayMetrics().density;
        setWillNotDraw(false);
        setClickable(true);
        setFocusable(false);
        setOnClickListener(v -> { if (listener != null) listener.onMinimapTap(); });
        stroke.setStyle(Paint.Style.STROKE);
        stroke.setStrokeCap(Paint.Cap.ROUND);
        stroke.setStrokeJoin(Paint.Join.ROUND);
        text.setTypeface(Fonts.bold(c));
        text.setTextAlign(Paint.Align.CENTER);
        text.setColor(0xFFFFFFFF);
        int[] ids = {R.drawable.cw_ic_heart, R.drawable.cw_ic_shield, R.drawable.cw_ic_water, R.drawable.cw_ic_burger, R.drawable.cw_ic_bolt};
        for (int i = 0; i < 5; i++) arcIcons[i] = c.getDrawable(ids[i]);
        // le graphe routier se charge en arriere-plan des le depart (premier itineraire instantane)
        IO.execute(() -> CrestGps.get(getContext().getApplicationContext()));
    }

    public void setListener(Listener l) { listener = l; }

    // ---------------------------------------------------------------- taille / place
    /** diametre de la carte = min(34 % de la hauteur, 16 % de la largeur), comme le HUD HTML */
    private float mapDiameter() {
        int w = getResources().getDisplayMetrics().widthPixels, h = getResources().getDisplayMetrics().heightPixels;
        int W = Math.max(w, h), H = Math.min(w, h);
        return Math.min(H * 0.34f, W * 0.16f);
    }
    private float pad() { return 3.5f * dp + 14f * dp; }

    @Override protected void onMeasure(int wSpec, int hSpec) {
        int s = Math.round(mapDiameter() + pad() * 2);
        setMeasuredDimension(s, s);
        // la carte touche le bord gauche et presque le haut de l'ecran ; les jauges debordent dans la marge
        setTranslationX(-pad());
        setTranslationY(-pad() + 2f * dp);
    }

    // ---------------------------------------------------------------- donnees
    public void setPlayer(float posX, float posY, float camFx, float camFy, float heading, boolean inVehicle, float speed) {
        if (Float.isNaN(posX) || Float.isNaN(posY)) return;
        tx = posX; ty = posY;
        float l = (float) Math.hypot(camFx, camFy);
        if (l > 1e-4f) { tfx = camFx / l; tfy = camFy / l; }
        if (!Float.isNaN(heading)) { thx = (float) -Math.sin(heading); thy = (float) Math.cos(heading); }
        tspan = inVehicle ? VEH_SPAN + Math.min(Math.max(0, speed), 200) * SPEED_GAIN : FOOT_SPAN;
        if (!hasData) {
            cx = tx; cy = ty; cfx = tfx; cfy = tfy; chx = thx; chy = thy; cspan = tspan;
            hasData = true;
        } else if (Math.hypot(tx - cx, ty - cy) > 120) {   // teleportation
            cx = tx; cy = ty;
            if (waypoint != null) requestRoute(true);
        }
        if (waypoint != null) {
            if (waypointAuto && Math.hypot(waypoint[0] - tx, waypoint[1] - ty) < ARRIVED) {
                waypoint = null;
                clearRoute();
                if (listener != null) listener.onGpsArrived();
            } else {
                trackRoute();
            }
        }
        postInvalidateOnAnimation();
    }

    public void setStats(float health, float armour, float thirst, float hunger, float energy) {
        stat[0] = clamp(health); stat[1] = clamp(armour); stat[2] = clamp(thirst); stat[3] = clamp(hunger); stat[4] = clamp(energy);
        postInvalidateOnAnimation();
    }

    /** [{x,y,icon}, ...] (JSON du serveur, dialogue 9605) */
    public void setBlips(String json) {
        ArrayList<Blip> next = new ArrayList<>();
        try {
            JSONArray a = new JSONArray(json == null || json.isEmpty() ? "[]" : json);
            for (int i = 0; i < a.length(); i++) {
                JSONObject o = a.optJSONObject(i);
                if (o == null || !o.has("x") || !o.has("y")) continue;
                Blip b = new Blip();
                b.x = (float) o.optDouble("x", 0); b.y = (float) o.optDouble("y", 0); b.icon = o.optInt("icon", 0);
                next.add(b);
            }
        } catch (Exception ignored) { }
        blips.clear();
        blips.addAll(next);
        invalidate();
    }

    public void setWaypoint(float x, float y, boolean auto) {
        boolean same = waypoint != null && Math.abs(waypoint[0] - x) < 0.5f && Math.abs(waypoint[1] - y) < 0.5f;
        waypoint = new float[]{x, y};
        waypointAuto = auto;
        if (!same) { route = null; routeIdx = 0; hasProj = false; }
        requestRoute(true);
        invalidate();
    }

    public void clearWaypoint() { waypoint = null; clearRoute(); }

    public void setZoom(float span) { manualSpan = span > 0 ? Math.max(80, Math.min(1200, span)) : 0; invalidate(); }

    private static float clamp(float v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }

    // ---------------------------------------------------------------- itineraire
    private void requestRoute(boolean force) {
        if (waypoint == null || !hasData || routePending) return;
        long now = SystemClock.uptimeMillis();
        if (!force && now - routeAt < RECALC_MS) return;
        routePending = true;
        routeAt = now;
        final float sx = tx, sy = ty, ex = waypoint[0], ey = waypoint[1];
        final Context app = getContext().getApplicationContext();
        IO.execute(() -> {
            CrestGps g = CrestGps.get(app);
            CrestGps.Route r = g != null ? g.route(sx, sy, ex, ey) : null;
            if (r == null) r = CrestGps.straight(sx, sy, ex, ey);   // interieur, ile... : ligne droite
            final CrestGps.Route done = r;
            post(() -> {
                routePending = false;
                if (waypoint == null) return;
                route = done;
                routeIdx = 0;
                routeAt = SystemClock.uptimeMillis();
                trackRoute();
                sendGpsWorld();
                invalidate();
            });
        });
    }

    private void trackRoute() {
        if (waypoint == null) return;
        if (route == null) { requestRoute(true); return; }
        CrestGps.progress(route, routeIdx, tx, ty, prog);
        hasProj = true;
        routeIdx = prog.idx;
        routeLeft = prog.left + (prog.off > 8 ? prog.off : 0);
        if (!route.straight && prog.off > OFF_ROUTE) requestRoute(false);
        else if (route.straight && Math.hypot(route.pts[0] - tx, route.pts[1] - ty) > 80) requestRoute(false);
    }

    private void clearRoute() {
        route = null; routeIdx = 0; routeLeft = 0; hasProj = false;
        sendGpsWorld();
        invalidate();
    }

    private void sendGpsWorld() {
        String data = "";
        if (waypoint != null) {
            StringBuilder sb = new StringBuilder();
            sb.append(fmt(waypoint[0])).append(',').append(fmt(waypoint[1]));
            if (route != null && !route.straight) {
                float[] p = route.pts; int n = p.length / 2, parts = 1;
                float lx = 1e9f, ly = 1e9f;
                for (int i = 0; i < n && parts < 3000; i++) {
                    float x = p[i * 2], y = p[i * 2 + 1];
                    if (Math.hypot(x - lx, y - ly) < 3 && i < n - 1) continue;
                    sb.append(';').append(fmt(x)).append(',').append(fmt(y));
                    lx = x; ly = y; parts++;
                }
            }
            data = sb.toString();
        }
        if (data.equals(gpsSentKey)) return;
        gpsSentKey = data;
        if (listener != null) listener.onGpsRoute(data);
    }
    private static String fmt(float v) { return String.format(java.util.Locale.US, "%.1f", v); }

    // ---------------------------------------------------------------- images (fil d'arriere-plan)
    private Bitmap tile(int index) {
        Bitmap b = tiles.get(index);
        if (b == null) load("hud/radar/tiles/radar" + index + ".png", index, true);
        return b;
    }
    private Bitmap blipIcon(int icon) {
        Bitmap b = blipIcons.get(icon);
        if (b == null) load("hud/radar/blips/blip_" + icon + ".png", icon, false);
        return b;
    }
    private void load(final String asset, final int key, final boolean isTile) {
        if (loading.contains(asset) || failed.contains(asset)) return;
        loading.add(asset);
        final Context app = getContext().getApplicationContext();
        IO.execute(() -> {
            Bitmap bmp = null;
            try (InputStream in = app.getAssets().open(asset)) { bmp = BitmapFactory.decodeStream(in); } catch (Throwable ignored) { }
            final Bitmap res = bmp;
            post(() -> {
                loading.remove(asset);
                if (res == null) { failed.add(asset); return; }
                if (isTile) {
                    tiles.put(key, res);
                    if (tiles.size() > MAX_TILES) {
                        Iterator<Map.Entry<Integer, Bitmap>> it = tiles.entrySet().iterator();
                        while (tiles.size() > MAX_TILES && it.hasNext()) { it.next(); it.remove(); }
                    }
                } else blipIcons.put(key, res);
                invalidate();
            });
        });
    }

    // ---------------------------------------------------------------- dessin
    private boolean step(float dt) {
        float k = 1f - (float) Math.exp(-dt * 12f);
        float before = Math.abs(tx - cx) + Math.abs(ty - cy) + Math.abs(tfx - cfx) + Math.abs(tfy - cfy) + Math.abs(thx - chx) + Math.abs(thy - chy);
        cx += (tx - cx) * k; cy += (ty - cy) * k;
        float fx = cfx + (tfx - cfx) * k, fy = cfy + (tfy - cfy) * k, l = (float) Math.hypot(fx, fy);
        if (l > 1e-4f) { cfx = fx / l; cfy = fy / l; } else { cfx = tfx; cfy = tfy; }
        float hx = chx + (thx - chx) * k, hy = chy + (thy - chy) * k; l = (float) Math.hypot(hx, hy);
        if (l > 1e-4f) { chx = hx / l; chy = hy / l; } else { chx = thx; chy = thy; }
        float goal = manualSpan > 0 ? manualSpan : tspan, gap = goal - cspan;
        cspan += gap * (1f - (float) Math.exp(-dt * 3f));
        boolean anim = false;
        for (int i = 0; i < 5; i++) {
            shown[i] += (stat[i] - shown[i]) * k;
            if (Math.abs(stat[i] - shown[i]) > 0.3f) anim = true; else shown[i] = stat[i];
        }
        return before > 0.02f || Math.abs(gap) > 0.5f || anim;
    }

    @Override protected void onDraw(Canvas cv) {
        long now = SystemClock.uptimeMillis();
        float dt = lastFrame == 0 ? 0.016f : Math.min(0.1f, Math.max(0.001f, (now - lastFrame) / 1000f));
        lastFrame = now;
        boolean moving = hasData && step(dt);

        final float P = pad(), D = mapDiameter(), R = D / 2f;
        final float mx = P + R, my = P + R;   // centre de la carte dans la vue
        if (hasData) {
            final float s = D / cspan;
            cv.save();
            clip.reset();
            clip.addCircle(mx, my, R, Path.Direction.CW);
            cv.clipPath(clip);
            fill.setColor(SEA);
            cv.drawCircle(mx, my, R, fill);
            drawTiles(cv, mx, my, s);
            drawRoute(cv, mx, my, s);
            if (vignetteFor != Math.round(D)) {
                vignetteFor = Math.round(D);
                vignette.setShader(new RadialGradient(mx, my, D * 0.52f, new int[]{0x00000000, 0x00000000, 0x6B000000},
                        new float[]{0f, 0.3f / 0.52f, 1f}, Shader.TileMode.CLAMP));
            }
            cv.drawCircle(mx, my, R, vignette);
            drawMarkers(cv, mx, my, R, s);
            cv.restore();
        } else {
            fill.setColor(0x2E0A0B0E);
            cv.drawCircle(mx, my, R, fill);
        }
        // liseré blanc + ombre (comme le cadre HTML)
        stroke.setPathEffect(null);
        stroke.setStrokeWidth(1f * dp);
        stroke.setColor(0x59000000);
        cv.drawCircle(mx, my, R + 1f * dp, stroke);
        stroke.setColor(0x9EFFFFFF);
        cv.drawCircle(mx, my, R - 0.5f * dp, stroke);

        drawRing(cv, mx, my, R);

        if (moving) {
            long wait = Math.max(0, MIN_FRAME_MS - (SystemClock.uptimeMillis() - now));
            if (wait > 4) postInvalidateDelayed(wait); else postInvalidateOnAnimation();
        } else lastFrame = 0;
    }

    /** repere monde (X, V=-Y) -> ecran, haut de la carte = direction de la camera */
    private void worldMatrix(float mx, float my, float s) {
        float fx = cfx, fy = cfy, rx = fy, ry = -fx;
        float a = s * rx, b = -s * fx, c = -s * ry, d = s * fy;
        float e = mx - s * (rx * cx + ry * cy), f = my + s * (fx * cx + fy * cy);
        mv[0] = a; mv[1] = c; mv[2] = e; mv[3] = b; mv[4] = d; mv[5] = f; mv[6] = 0; mv[7] = 0; mv[8] = 1;
        mtx.setValues(mv);
    }

    private void drawTiles(Canvas cv, float mx, float my, float s) {
        float radius = cspan * 0.75f, padW = radius * 0.35f;
        int col0 = cl((int) Math.floor((cx - radius - padW + HALF) / TILE)), col1 = cl((int) Math.floor((cx + radius + padW + HALF) / TILE));
        int row0 = cl((int) Math.floor((HALF - (cy + radius + padW)) / TILE)), row1 = cl((int) Math.floor((HALF - (cy - radius - padW)) / TILE));
        int vc0 = cl((int) Math.floor((cx - radius + HALF) / TILE)), vc1 = cl((int) Math.floor((cx + radius + HALF) / TILE));
        int vr0 = cl((int) Math.floor((HALF - (cy + radius)) / TILE)), vr1 = cl((int) Math.floor((HALF - (cy - radius)) / TILE));
        worldMatrix(mx, my, s);
        cv.save();
        cv.concat(mtx);
        for (int row = row0; row <= row1; row++) {
            for (int col = col0; col <= col1; col++) {
                Bitmap b = tile(row * GRID + col);    // demande aussi les tuiles voisines (prechargement)
                boolean visible = col >= vc0 && col <= vc1 && row >= vr0 && row <= vr1;
                if (!visible || b == null) continue;
                float x = col * TILE - HALF, y = row * TILE - HALF;
                rect.set(x, y, x + TILE + 0.6f, y + TILE + 0.6f);
                cv.drawBitmap(b, null, rect, bmpPaint);
            }
        }
        cv.restore();
    }
    private static int cl(int v) { return Math.max(0, Math.min(GRID - 1, v)); }

    private void drawRoute(Canvas cv, float mx, float my, float s) {
        if (route == null || waypoint == null) return;
        float[] pts = route.pts; int n = pts.length / 2;
        if (n < 2) return;
        float reach = cspan * 0.9f;
        path.reset();
        path.moveTo(cx, -cy);
        if (hasProj && !route.straight) path.lineTo(prog.px, -prog.py);
        for (int i = Math.min(n - 1, routeIdx + 1); i < n; i++) {
            path.lineTo(pts[i * 2], -pts[i * 2 + 1]);
            if (Math.abs(pts[i * 2] - cx) > reach * 2.5f || Math.abs(pts[i * 2 + 1] - cy) > reach * 2.5f) break;
        }
        float px = 1f / s;   // 1 pixel en unites du monde
        worldMatrix(mx, my, s);
        cv.save();
        cv.concat(mtx);
        stroke.setPathEffect(null);
        stroke.setColor(ROUTE_EDGE);
        stroke.setStrokeWidth(8.5f * dp * px);
        cv.drawPath(path, stroke);
        stroke.setColor(route.straight ? 0xCC3B82FF : ROUTE);
        stroke.setStrokeWidth(5.5f * dp * px);
        if (route.straight) stroke.setPathEffect(new DashPathEffect(new float[]{10 * dp * px, 7 * dp * px}, 0));
        cv.drawPath(path, stroke);
        if (!route.straight) {
            stroke.setColor(ROUTE_CORE);
            stroke.setStrokeWidth(1.3f * dp * px);
            stroke.setPathEffect(new DashPathEffect(new float[]{6 * dp * px, 9 * dp * px}, 0));
            cv.drawPath(path, stroke);
        }
        stroke.setPathEffect(null);
        cv.restore();
    }

    private final float[] placed = new float[3];
    private void place(float wx, float wy, float s, float half, float mx, float my) {
        float dx = wx - cx, dy = wy - cy;
        float sx = (dx * cfy + dy * -cfx) * s, sy = -(dx * cfx + dy * cfy) * s;
        float m = (float) Math.hypot(sx, sy), limit = half - 9 * dp;
        boolean edge = false;
        if (m > limit) { sx *= limit / m; sy *= limit / m; edge = true; }
        placed[0] = mx + sx; placed[1] = my + sy; placed[2] = edge ? 1 : 0;
    }

    private void drawMarkers(Canvas cv, float mx, float my, float half, float s) {
        for (Blip b : blips) {
            place(b.x, b.y, s, half, mx, my);
            Bitmap ic = blipIcon(b.icon);
            if (ic == null) continue;
            float sz = 16 * dp;
            bmpPaint.setAlpha(placed[2] > 0 ? 191 : 255);
            rect.set(placed[0] - sz / 2, placed[1] - sz / 2, placed[0] + sz / 2, placed[1] + sz / 2);
            cv.drawBitmap(ic, null, rect, bmpPaint);
        }
        bmpPaint.setAlpha(255);

        if (waypoint != null) {
            place(waypoint[0], waypoint[1], s, half, mx, my);
            drawWaypoint(cv, placed[0], placed[1], placed[2] > 0);
        }

        // fleche du joueur
        float rx = cfy, ry = -cfx;
        double angle = Math.atan2(chx * rx + chy * ry, chx * cfx + chy * cfy);
        cv.save();
        cv.translate(mx, my);
        cv.rotate((float) Math.toDegrees(angle));
        path.reset();
        path.moveTo(0, -8 * dp);
        path.lineTo(5.8f * dp, 6.5f * dp);
        path.lineTo(0, 3.4f * dp);
        path.lineTo(-5.8f * dp, 6.5f * dp);
        path.close();
        fill.setColor(0xFFFFFFFF);
        fill.setShadowLayer(3 * dp, 0, 0, 0x8C000000);
        cv.drawPath(path, fill);
        fill.clearShadowLayer();
        stroke.setColor(ROUTE);
        stroke.setStrokeWidth(1.6f * dp);
        cv.drawPath(path, stroke);
        cv.restore();

        // nord
        float t = half - 8 * dp, nx = mx - cfx * t, ny = my - cfy * t;
        fill.setColor(0xEB111419);
        cv.drawCircle(nx, ny, 6.2f * dp, fill);
        stroke.setColor(ROUTE);
        stroke.setStrokeWidth(1.2f * dp);
        cv.drawCircle(nx, ny, 6.2f * dp, stroke);
        text.setTextSize(8 * dp);
        Paint.FontMetrics fm = text.getFontMetrics();
        cv.drawText("N", nx, ny - (fm.ascent + fm.descent) / 2f, text);

        // distance restante
        if (waypoint != null && route != null) {
            String label = CrestGps.formatDistance(routeLeft);
            if (!label.isEmpty()) {
                text.setTextSize(9.5f * dp);
                float tw = text.measureText(label) + 14 * dp, th = 15 * dp;
                float bx = mx - tw / 2, by = my + half - th - 7 * dp;
                fill.setColor(ROUTE);
                rect.set(bx, by, bx + tw, by + th);
                cv.drawRoundRect(rect, 2 * dp, 2 * dp, fill);
                stroke.setColor(0x99FFFFFF);
                stroke.setStrokeWidth(1 * dp);
                rect.set(bx + 1.5f * dp, by + 1.5f * dp, bx + tw - 1.5f * dp, by + th - 1.5f * dp);
                cv.drawRoundRect(rect, 1 * dp, 1 * dp, stroke);
                fm = text.getFontMetrics();
                cv.drawText(label, mx, by + th / 2f - (fm.ascent + fm.descent) / 2f, text);
            }
        }
    }

    private void drawWaypoint(Canvas cv, float x, float y, boolean edge) {
        float r = (edge ? 5 : 6.5f) * dp;
        if (!edge) {
            path.reset();
            path.moveTo(x - r * 0.62f, y - r * 0.9f);
            path.lineTo(x + r * 0.62f, y - r * 0.9f);
            path.lineTo(x, y + 0.2f * dp);
            path.close();
            fill.setColor(ROUTE);
            cv.drawPath(path, fill);
            y -= r * 1.45f;
        }
        fill.setColor(ROUTE_EDGE); cv.drawCircle(x, y, r + 1.6f * dp, fill);
        fill.setColor(ROUTE); cv.drawCircle(x, y, r, fill);
        stroke.setColor(0xE6FFFFFF); stroke.setStrokeWidth(1.1f * dp); cv.drawCircle(x, y, r * 0.7f, stroke);
        fill.setColor(0xFFFFFFFF); cv.drawCircle(x, y, r * 0.3f, fill);
    }

    /** jauges en arc autour de la carte (vie, gilet, soif, faim, energie) + leurs icones */
    private void drawRing(Canvas cv, float mx, float my, float R) {
        float sw = 3.5f * dp, r = R + 3 * dp + sw / 2f;
        float span = (END - START - GAP * 4) / 5f;
        rect.set(mx - r, my - r, mx + r, my + r);
        long now = SystemClock.uptimeMillis();
        boolean pulse = false;
        for (int i = 0; i < 5; i++) {
            float a0 = START + i * (span + GAP);
            float androidStart = a0 - 90f;       // 0 = midi -> 0 = 3 h pour Android
            int col = ARC_COLORS[i];
            float v = shown[i];
            boolean critical = v <= 15 && !(i == 1);
            float alphaMul = (i == 1 && v <= 0) ? 0.35f : 1f;
            if (critical) {
                pulse = true;
                float ph = (now % 900) / 900f;
                alphaMul *= 0.35f + 0.65f * (float) Math.abs(Math.cos(ph * Math.PI));
            }
            stroke.setPathEffect(null);
            stroke.setStrokeWidth(sw);
            stroke.setColor(withAlpha(col, 0.16f * (i == 1 && v <= 0 ? 0.35f : 1f)));
            cv.drawArc(rect, androidStart, span, false, stroke);
            if (v > 0.2f) {
                float sweep = span * v / 100f;
                stroke.setStrokeWidth(sw * 2.6f);              // halo (drop-shadow du HTML)
                stroke.setColor(withAlpha(col, 0.22f * alphaMul));
                cv.drawArc(rect, androidStart, sweep, false, stroke);
                stroke.setStrokeWidth(sw);
                stroke.setColor(withAlpha(col, 0.95f * alphaMul));
                cv.drawArc(rect, androidStart, sweep, false, stroke);
            }
            // icone au debut de l'arc
            double a = Math.toRadians(a0 + 2 - 90);
            float ix = mx + (float) Math.cos(a) * r, iy = my + (float) Math.sin(a) * r;
            float isz = 15 * dp;
            fill.setColor(withAlpha(0xFF06090F, 0.9f * (critical ? alphaMul : 1f)));
            cv.drawCircle(ix, iy, isz / 2f + 1 * dp, fill);
            Drawable d = arcIcons[i];
            if (d != null) {
                float in = isz - 4 * dp;
                int iw = d.getIntrinsicWidth(), ih = d.getIntrinsicHeight();
                float sc = in / Math.max(1, Math.max(iw, ih));
                int w = Math.round(iw * sc), h = Math.round(ih * sc);
                d.setBounds(Math.round(ix - w / 2f), Math.round(iy - h / 2f), Math.round(ix + w / 2f), Math.round(iy + h / 2f));
                d.setAlpha(critical ? Math.round(255 * alphaMul) : 255);
                d.draw(cv);
            }
        }
        if (pulse) postInvalidateDelayed(MIN_FRAME_MS);
    }

    private static int withAlpha(int color, float a) {
        int al = Math.max(0, Math.min(255, Math.round(255 * a)));
        return (color & 0x00FFFFFF) | (al << 24);
    }
}
