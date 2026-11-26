package com.crestwood.ui;

import android.content.Context;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * [Crestwood v37] Moteur d'itineraire du GPS en Java (port exact de hud/gps/gps-route.js).
 * Graphe routier de San Andreas (GPS.dat de SA-MP, sens uniques respectes) : assets/hud/gps/roads.bin
 * ("CWRD", n, m, xy int16 x2n (/4), deg uint8 x n, to uint16 x m). Recherche A* : quelques ms.
 */
public final class CrestGps {
    private static final int CELL = 100, HALF = 3000, COLS = (HALF * 2 + CELL - 1) / CELL;
    private static CrestGps sInstance;

    private int n;
    private float[] X, Y;
    private int[] OFF;
    private char[] TO;              // uint16
    private int[] cellStart, cellNodes;
    private float[] gScore;
    private int[] parent, seen, closed;
    private int gen;
    private int[] heapN = new int[4096];
    private float[] heapK = new float[4096];
    private int heapSize;

    /** Itineraire : points x,y aplatis, distances cumulees, longueur ; straight = ligne droite (pas de route) */
    public static final class Route {
        public final float[] pts; public final float[] cum; public final float len; public final boolean straight;
        Route(float[] pts, float[] cum, float len, boolean straight) { this.pts = pts; this.cum = cum; this.len = len; this.straight = straight; }
        public int count() { return pts.length / 2; }
    }

    /** Avancement : segment courant, point projete, ecart au trace, distance restante */
    public static final class Progress { public int idx; public float px, py, off, left; }

    private CrestGps() { }

    /** Charge le graphe une seule fois (a appeler hors du fil UI de preference). null si absent. */
    public static synchronized CrestGps get(Context c) {
        if (sInstance != null) return sInstance;
        try (InputStream in = c.getAssets().open("hud/gps/roads.bin")) {
            ByteArrayOutputStream bo = new ByteArrayOutputStream(260000);
            byte[] buf = new byte[16384];
            int r;
            while ((r = in.read(buf)) > 0) bo.write(buf, 0, r);
            sInstance = fromBytes(bo.toByteArray());
            return sInstance;
        } catch (Throwable t) {
            android.util.Log.e("CrestGps", "roads.bin", t);
            return null;
        }
    }

    /** graphe depuis le contenu de roads.bin (null si invalide) */
    public static CrestGps fromBytes(byte[] data) {
        CrestGps g = new CrestGps();
        return g.decode(ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)) ? g : null;
    }

    private boolean decode(ByteBuffer b) {
        if (b.get() != 'C' || b.get() != 'W' || b.get() != 'R' || b.get() != 'D') return false;
        n = b.getInt();
        int m = b.getInt();
        X = new float[n]; Y = new float[n]; OFF = new int[n + 1];
        for (int i = 0; i < n; i++) { X[i] = b.getShort() / 4f; Y[i] = b.getShort() / 4f; }
        for (int i = 0; i < n; i++) OFF[i + 1] = OFF[i] + (b.get() & 0xFF);
        if (OFF[n] != m) return false;
        TO = new char[m];
        for (int i = 0; i < m; i++) TO[i] = b.getChar();
        int cells = COLS * COLS;
        int[] count = new int[cells + 1];
        int[] cellOf = new int[n];
        for (int i = 0; i < n; i++) { int c = cellIndex(X[i], Y[i]); cellOf[i] = c; count[c + 1]++; }
        for (int c = 0; c < cells; c++) count[c + 1] += count[c];
        cellStart = count;
        cellNodes = new int[n];
        int[] fill = new int[cells];
        System.arraycopy(count, 0, fill, 0, cells);
        for (int i = 0; i < n; i++) cellNodes[fill[cellOf[i]]++] = i;
        gScore = new float[n]; parent = new int[n]; seen = new int[n]; closed = new int[n];
        return true;
    }

    private static int cellIndex(float x, float y) {
        int cx = Math.max(0, Math.min(COLS - 1, (int) Math.floor((x + HALF) / CELL)));
        int cy = Math.max(0, Math.min(COLS - 1, (int) Math.floor((y + HALF) / CELL)));
        return cy * COLS + cx;
    }

    public int nearest(float x, float y) {
        int cx = Math.max(0, Math.min(COLS - 1, (int) Math.floor((x + HALF) / CELL)));
        int cy = Math.max(0, Math.min(COLS - 1, (int) Math.floor((y + HALF) / CELL)));
        int best = -1; float bestD = Float.MAX_VALUE;
        for (int r = 0; r < COLS; r++) {
            if (best >= 0 && (r - 1) * CELL > Math.sqrt(bestD)) break;
            int x0 = cx - r, x1 = cx + r, y0 = cy - r, y1 = cy + r;
            for (int gy = y0; gy <= y1; gy++) {
                if (gy < 0 || gy >= COLS) continue;
                boolean edgeRow = gy == y0 || gy == y1;
                int stepX = edgeRow ? 1 : Math.max(1, x1 - x0);
                for (int gx = x0; gx <= x1; gx += stepX) {
                    if (gx < 0 || gx >= COLS) continue;
                    int c = gy * COLS + gx;
                    for (int k = cellStart[c]; k < cellStart[c + 1]; k++) {
                        int i = cellNodes[k];
                        float dx = X[i] - x, dy = Y[i] - y, d = dx * dx + dy * dy;
                        if (d < bestD) { bestD = d; best = i; }
                    }
                }
            }
        }
        return best;
    }

    private void heapPush(int node, float k) {
        if (heapSize >= heapN.length) {
            int[] nn = new int[heapN.length * 2]; System.arraycopy(heapN, 0, nn, 0, heapN.length); heapN = nn;
            float[] kk = new float[heapK.length * 2]; System.arraycopy(heapK, 0, kk, 0, heapK.length); heapK = kk;
        }
        int i = heapSize++;
        while (i > 0) {
            int p = (i - 1) >> 1;
            if (heapK[p] <= k) break;
            heapN[i] = heapN[p]; heapK[i] = heapK[p];
            i = p;
        }
        heapN[i] = node; heapK[i] = k;
    }

    private int heapPop() {
        int top = heapN[0];
        int node = heapN[--heapSize]; float k = heapK[heapSize];
        int i = 0;
        for (;;) {
            int c = i * 2 + 1;
            if (c >= heapSize) break;
            if (c + 1 < heapSize && heapK[c + 1] < heapK[c]) c++;
            if (heapK[c] >= k) break;
            heapN[i] = heapN[c]; heapK[i] = heapK[c];
            i = c;
        }
        heapN[i] = node; heapK[i] = k;
        return top;
    }

    private int[] astar(int s, int t) {
        gen++;
        if (gen > 0x7ffffff0) { gen = 1; java.util.Arrays.fill(seen, 0); java.util.Arrays.fill(closed, 0); }
        heapSize = 0;
        float tx = X[t], ty = Y[t];
        gScore[s] = 0; parent[s] = -1; seen[s] = gen;
        heapPush(s, (float) Math.hypot(X[s] - tx, Y[s] - ty));
        int guard = 0;
        while (heapSize > 0) {
            int u = heapPop();
            if (closed[u] == gen) continue;
            closed[u] = gen;
            if (u == t) break;
            if (++guard > n * 3) return null;
            float gu = gScore[u], ux = X[u], uy = Y[u];
            for (int k = OFF[u]; k < OFF[u + 1]; k++) {
                int v = TO[k];
                if (closed[v] == gen) continue;
                float ng = gu + (float) Math.hypot(X[v] - ux, Y[v] - uy);
                if (seen[v] != gen || ng < gScore[v]) {
                    seen[v] = gen; gScore[v] = ng; parent[v] = u;
                    heapPush(v, ng + (float) Math.hypot(X[v] - tx, Y[v] - ty));
                }
            }
        }
        if (closed[t] != gen) return null;
        int len = 0;
        for (int v = t; v != -1; v = parent[v]) len++;
        int[] path = new int[len];
        for (int v = t, i = len - 1; v != -1; v = parent[v], i--) path[i] = v;
        return path;
    }

    private static boolean[] simplify(float[] xs, float[] ys, int n, float eps) {
        boolean[] keep = new boolean[n];
        if (n < 3) { for (int i = 0; i < n; i++) keep[i] = true; return keep; }
        keep[0] = keep[n - 1] = true;
        int[] stack = new int[n * 2 + 4];
        int sp = 0;
        stack[sp++] = 0; stack[sp++] = n - 1;
        while (sp > 0) {
            int b = stack[--sp], a = stack[--sp];
            float ax = xs[a], ay = ys[a], dx = xs[b] - ax, dy = ys[b] - ay;
            float len2 = dx * dx + dy * dy; if (len2 == 0) len2 = 1e-6f;
            int far = -1; float farD = eps * eps;
            for (int i = a + 1; i < b; i++) {
                float t = Math.max(0, Math.min(1, ((xs[i] - ax) * dx + (ys[i] - ay) * dy) / len2));
                float ex = ax + t * dx - xs[i], ey = ay + t * dy - ys[i], d = ex * ex + ey * ey;
                if (d > farD) { farD = d; far = i; }
            }
            if (far >= 0) {
                keep[far] = true;
                if (sp + 4 > stack.length) { int[] ns = new int[stack.length * 2]; System.arraycopy(stack, 0, ns, 0, sp); stack = ns; }
                stack[sp++] = a; stack[sp++] = far; stack[sp++] = far; stack[sp++] = b;
            }
        }
        return keep;
    }

    private static Route build(float[] xs, float[] ys, int n) {
        boolean[] keep = simplify(xs, ys, n, 1.5f);
        int k = 0;
        for (int i = 0; i < n; i++) if (keep[i]) k++;
        float[] pts = new float[k * 2], cum = new float[k];
        int j = 0;
        for (int i = 0; i < n; i++) {
            if (!keep[i]) continue;
            pts[j * 2] = xs[i]; pts[j * 2 + 1] = ys[i];
            if (j > 0) cum[j] = cum[j - 1] + (float) Math.hypot(pts[j * 2] - pts[j * 2 - 2], pts[j * 2 + 1] - pts[j * 2 - 1]);
            j++;
        }
        return new Route(pts, cum, k > 0 ? cum[k - 1] : 0, false);
    }

    public static Route straight(float x0, float y0, float x1, float y1) {
        float len = (float) Math.hypot(x1 - x0, y1 - y0);
        return new Route(new float[]{x0, y0, x1, y1}, new float[]{0, len}, len, true);
    }

    public synchronized Route route(float x0, float y0, float x1, float y1) {
        int s = nearest(x0, y0), t = nearest(x1, y1);
        if (s < 0 || t < 0) return null;
        int[] path = s != t ? astar(s, t) : new int[]{s};
        if (path == null) return null;
        int cnt = path.length + 2;
        float[] xs = new float[cnt], ys = new float[cnt];
        xs[0] = x0; ys[0] = y0;
        for (int i = 0; i < path.length; i++) { xs[i + 1] = X[path[i]]; ys[i + 1] = Y[path[i]]; }
        xs[cnt - 1] = x1; ys[cnt - 1] = y1;
        return build(xs, ys, cnt);
    }

    public static Progress progress(Route r, int from, float px, float py, Progress out) {
        if (out == null) out = new Progress();
        float[] pts = r.pts; int n = pts.length / 2;
        if (n < 2) { out.idx = 0; out.px = pts[0]; out.py = pts[1]; out.off = (float) Math.hypot(pts[0] - px, pts[1] - py); out.left = 0; return out; }
        int best = Math.max(0, Math.min(n - 2, from)); float bestD = Float.MAX_VALUE, bestT = 0;
        int end = Math.min(n - 2, best + 25);
        for (int i = Math.max(0, best - 1); i <= end; i++) {
            float ax = pts[i * 2], ay = pts[i * 2 + 1], dx = pts[i * 2 + 2] - ax, dy = pts[i * 2 + 3] - ay;
            float len2 = dx * dx + dy * dy; if (len2 == 0) len2 = 1e-6f;
            float t = Math.max(0, Math.min(1, ((px - ax) * dx + (py - ay) * dy) / len2));
            float ex = ax + t * dx - px, ey = ay + t * dy - py, d = ex * ex + ey * ey;
            if (d < bestD - 0.01f) { bestD = d; best = i; bestT = t; }
        }
        float ax = pts[best * 2], ay = pts[best * 2 + 1];
        out.idx = best;
        out.px = ax + (pts[best * 2 + 2] - ax) * bestT;
        out.py = ay + (pts[best * 2 + 3] - ay) * bestT;
        float seg = r.cum[best + 1] - r.cum[best];
        out.left = Math.max(0, r.len - (r.cum[best] + seg * bestT));
        out.off = (float) Math.sqrt(bestD);
        return out;
    }

    /** "350 m" / "1,2 km" */
    public static String formatDistance(float d) {
        if (Float.isNaN(d) || Float.isInfinite(d)) return "";
        if (d < 1000) return Math.max(0, Math.round(d / 10f) * 10) + " m";
        String s = d < 10000 ? String.format(java.util.Locale.FRANCE, "%.1f", d / 1000f) : String.valueOf(Math.round(d / 1000f));
        return s + " km";
    }
}
