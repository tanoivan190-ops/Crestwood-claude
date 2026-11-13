package com.gta.game.ui;

import android.content.Context;
import android.graphics.drawable.GradientDrawable;
import android.os.Handler;
import android.os.Looper;
import android.text.InputFilter;
import android.text.InputType;
import android.text.TextUtils;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.GridView;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

/**
 * GARAGE PRO Crestwood : panneau 100 % natif Android (plus de WebView).
 *
 * Serveur -> Java : dialogues "@garage" 9750 (ouvrir) / 9751 (mise a jour) / 9752 (fermer), JSON :
 *   {"garage","at","deliv","cash","bank","maxOut","ok","msg","fee":{rep,svb,svw,fl,i7,i30,imin,paint,plate,sellPct},
 *    "veh":[{id,m,n,k,pl,hp,fu,o,s,p,km,w,in,im,f,c1,c2,sh,ow,lk,en}]}
 * Java -> serveur : Callback.onAction("take 12") -> "/garact take 12" (le serveur re-verifie tout).
 *
 * Mise en page (paysage) :
 *   barre du haut : CRESTWOOD · GARAGE, lieu, portefeuille, compteur, fermer
 *   gauche : fiche du vehicule (silhouette peinte, plaque, statuts) + onglets
 *            APERCU (jauges, actions) | ATELIER (reparation, entretien, plein, fourriere)
 *            STYLE (surnom, plaque, peinture) | PAPIERS (assurance, double des cles, vente)
 *   droite : categories + tri, grille des vehicules (cartes)
 */
public class NativeGaragePanel extends FrameLayout {

    public interface Callback {
        void onAction(String command);   // ex. "take 12", "paint 12 3 1"
        void onClose();
    }

    private static final String TAG = "CwGarage";
    private static final int RED = 0xFFFF5A63, GREEN = 0xFF3DDC97, ORANGE = 0xFFFFB454;
    private static final String[] CAT_KEYS = {"all", "car", "bike", "boat", "truck", "air"};
    private static final String[] CAT_LABELS = {"Tous", "Voitures", "Motos", "Bateaux", "Camions", "Aériens"};
    private static final String[] SORT_LABELS = {"Favoris", "État", "Carburant", "Valeur"};
    private static final String[] TAB_LABELS = {"APERÇU", "ATELIER", "STYLE", "PAPIERS"};
    // couleurs proposees a la peinture (ids SA)
    private static final int[] PAINT_IDS = {0, 1, 8, 14, 26, 9, 24, 36, 3, 17, 43, 58, 6, 61, 65, 46,
            2, 7, 79, 53, 87, 93, 103, 125, 16, 44, 86, 51, 5, 85, 126, 113};

    // ---- donnees ----
    private static final class Veh {
        int id, m, hp, fu, o, s, p, km, w, ins, im, f, c1, c2, ow, lk, en;
        String n = "", k = "", pl = "", sh = "";
    }

    private final Callback cb;
    private final Handler ui = new Handler(Looper.getMainLooper());
    private final List<Veh> vehs = new ArrayList<>();
    private final List<Veh> shown = new ArrayList<>();
    private String garage = "Mes véhicules";
    private boolean at;
    private int deliv = 500, cash, bank, maxOut = 2;
    private int feeRep = 40, feeSvb = 1500, feeSvw = 60, feeFl = 8, feeI7 = 2, feeI30 = 6, feeImin = 500, feePaint = 1500, feePlate = 10000, sellPct = 60;
    private int selId = -1;
    private int cat = 0, sort = 0, tab = 0;
    private boolean busy;
    private int paint1 = -1, paint2 = -1;
    private long sellArmedUntil;
    private boolean open;

    // ---- vues ----
    private TextView placeText, cashText, bankText, countText, toast;
    private LinearLayout catRow, sortRow, tabRow, tabBody, statusRow;
    private TextView vName, vNick, vPlate, vPlateBand;
    private VehicleSilhouetteView vSil;
    private View detailEmpty, detailContent;
    private GridView grid;
    private TextView emptyList;
    private final CardAdapter adapter = new CardAdapter();

    private final Runnable unbusy = () -> { busy = false; render(); };
    private final Runnable hideToast = () -> toast.animate().alpha(0f).setDuration(220).start();

    public NativeGaragePanel(Context c, Callback cb) {
        super(c);
        this.cb = cb;
        setClickable(true);   // plein ecran : le jeu ne recoit pas les touches pendant le garage
        setFocusable(true);
        setFocusableInTouchMode(true);
        setVisibility(GONE);
        // jeu visible derriere, assombri (degrade)
        setBackground(new GradientDrawable(GradientDrawable.Orientation.LEFT_RIGHT, new int[]{0xD9050608, 0xB3050608, 0xD9050608}));
        build();
    }

    // =====================================================================
    // API (thread UI)
    // =====================================================================
    public boolean isOpen() { return open; }

    public void open(String json) {
        open = true;
        tab = 0;
        busy = false;
        sellArmedUntil = 0;
        setVisibility(VISIBLE);
        setAlpha(0f);
        animate().alpha(1f).setDuration(180).start();
        apply(json, true);
    }

    public void update(String json) {
        if (!open) return;
        apply(json, false);
    }

    public void close() {
        open = false;
        hideKeyboard();
        ui.removeCallbacks(unbusy);
        animate().alpha(0f).setDuration(150).withEndAction(() -> setVisibility(GONE)).start();
    }

    // =====================================================================
    // Donnees
    // =====================================================================
    private void apply(String json, boolean first) {
        try {
            JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
            garage = o.optString("garage", garage);
            at = o.optInt("at", 0) == 1;
            deliv = o.optInt("deliv", deliv);
            cash = o.optInt("cash", cash);
            bank = o.optInt("bank", bank);
            maxOut = Math.max(1, o.optInt("maxOut", maxOut));
            JSONObject fee = o.optJSONObject("fee");
            if (fee != null) {
                feeRep = fee.optInt("rep", feeRep); feeSvb = fee.optInt("svb", feeSvb); feeSvw = fee.optInt("svw", feeSvw);
                feeFl = fee.optInt("fl", feeFl); feeI7 = fee.optInt("i7", feeI7); feeI30 = fee.optInt("i30", feeI30);
                feeImin = fee.optInt("imin", feeImin); feePaint = fee.optInt("paint", feePaint); feePlate = fee.optInt("plate", feePlate);
                sellPct = fee.optInt("sellPct", sellPct);
            }
            JSONArray arr = o.optJSONArray("veh");
            if (arr != null) {
                vehs.clear();
                for (int i = 0; i < arr.length(); i++) {
                    JSONObject j = arr.getJSONObject(i);
                    Veh v = new Veh();
                    v.id = j.optInt("id"); v.m = j.optInt("m");
                    v.n = j.optString("n", "Véhicule"); v.k = j.optString("k", "");
                    v.pl = j.optString("pl", j.optString("plate", "")); v.sh = j.optString("sh", "");
                    v.hp = clamp(j.optInt("hp", 100)); v.fu = clamp(j.optInt("fu", j.optInt("fuel", 100)));
                    v.o = j.optInt("o", j.optInt("out", 0)); v.s = j.optInt("s", j.optInt("sell", 0)); v.p = j.optInt("p", 0);
                    v.km = j.optInt("km", 0); v.w = clamp(j.optInt("w", 0)); v.ins = j.optInt("in", 0);
                    v.im = j.optInt("im", 0); v.f = j.optInt("f", 0); v.c1 = j.optInt("c1", 1); v.c2 = j.optInt("c2", 1);
                    v.ow = j.optInt("ow", 1); v.lk = j.optInt("lk", 0); v.en = j.optInt("en", 0);
                    vehs.add(v);
                }
            }
            if (find(selId) == null) selId = vehs.isEmpty() ? -1 : vehs.get(0).id;
            String msg = o.optString("msg", "");
            if (!first || !msg.isEmpty()) {
                busy = false;
                ui.removeCallbacks(unbusy);
            }
            if (!msg.isEmpty()) showToast(msg, o.optInt("ok", 1) == 1);
            Veh sel = find(selId);
            if (sel != null && (paint1 < 0 || first)) { paint1 = sel.c1; paint2 = sel.c2; }
        } catch (Exception e) {
            Log.e(TAG, "JSON garage invalide : " + e.getMessage());
        }
        render();
    }

    private static int clamp(int v) { return Math.max(0, Math.min(100, v)); }

    private Veh find(int id) {
        for (Veh v : vehs) if (v.id == id) return v;
        return null;
    }

    private static String catKeyOf(int model) {
        switch (VehicleSilhouetteView.kindOf(model)) {
            case VehicleSilhouetteView.BIKE: return "bike";
            case VehicleSilhouetteView.BOAT: return "boat";
            case VehicleSilhouetteView.TRUCK: return "truck";
            case VehicleSilhouetteView.AIR: return "air";
            default: return "car";
        }
    }

    /** 1250000 -> "1 250 000" (espace simple : la police n'a pas l'espace fine insecable). */
    private static String num(int v) {
        String s = Integer.toString(Math.abs(v));
        StringBuilder b = new StringBuilder();
        for (int i = 0; i < s.length(); i++) {
            if (i > 0 && (s.length() - i) % 3 == 0) b.append(' ');
            b.append(s.charAt(i));
        }
        return (v < 0 ? "-" : "") + b;
    }

    private String money(int v) { return num(v) + " $"; }

    private int outCount() {
        int n = 0;
        for (Veh v : vehs) if (v.o == 1 && v.ow == 1) n++;
        return n;
    }

    // =====================================================================
    // Construction
    // =====================================================================
    private int dp(float v) { return CwUi.dp(getContext(), v); }

    private LinearLayout.LayoutParams lp(int w, int h, float weight) {
        return new LinearLayout.LayoutParams(w, h, weight);
    }

    private LinearLayout.LayoutParams lpm(int w, int h, int l, int t, int r, int b) {
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(w, h);
        p.setMargins(dp(l), dp(t), dp(r), dp(b));
        return p;
    }

    private LinearLayout row(Context c) {
        LinearLayout l = new LinearLayout(c);
        l.setOrientation(LinearLayout.HORIZONTAL);
        l.setGravity(Gravity.CENTER_VERTICAL);
        return l;
    }

    private LinearLayout col(Context c) {
        LinearLayout l = new LinearLayout(c);
        l.setOrientation(LinearLayout.VERTICAL);
        return l;
    }

    private void build() {
        Context c = getContext();
        LinearLayout root = col(c);
        root.setPadding(dp(14), dp(10), dp(14), dp(12));
        addView(root, new LayoutParams(-1, -1));

        // ---------------- barre du haut ----------------
        LinearLayout top = row(c);
        LinearLayout brand = col(c);
        TextView eyebrow = CwUi.eyebrow(c, "Crestwood · Garage pro");
        eyebrow.setTextColor(CwUi.ACCENT);
        brand.addView(eyebrow);
        TextView title = CwUi.text(c, "MES VÉHICULES", 21f, CwUi.TEXT, CwUi.black(c));
        title.setLetterSpacing(0.04f);
        brand.addView(title, lpm(-2, -2, 0, 4, 0, 0));
        placeText = CwUi.text(c, "", 10.5f, CwUi.MUTED, CwUi.medium(c));
        brand.addView(placeText, lpm(-2, -2, 0, 4, 0, 0));
        top.addView(brand, lp(0, -2, 1f));

        countText = pill(c, "", false);
        top.addView(countText, lpm(-2, dp(30), 0, 0, 8, 0));
        cashText = walletChip(c, "LIQUIDE");
        top.addView((View) cashText.getTag(), lpm(-2, -2, 0, 0, 8, 0));
        bankText = walletChip(c, "BANQUE");
        top.addView((View) bankText.getTag(), lpm(-2, -2, 0, 0, 10, 0));
        TextView close = CwUi.ghostButton(c, "✕");
        close.setTextSize(16f);
        close.setOnClickListener(v -> { if (cb != null) cb.onClose(); });
        top.addView(close, new LinearLayout.LayoutParams(dp(40), dp(40)));
        root.addView(top);

        // filet bleu
        View rule = new View(c);
        rule.setBackground(new GradientDrawable(GradientDrawable.Orientation.LEFT_RIGHT, new int[]{CwUi.ACCENT, 0x553B82FF, 0x00000000}));
        root.addView(rule, lpm(-1, Math.max(1, dp(1)), 0, 10, 0, 10));

        // ---------------- corps ----------------
        LinearLayout body = row(c);
        body.setGravity(Gravity.TOP);
        root.addView(body, lp(-1, 0, 1f));

        // ---- gauche : fiche ----
        FrameLayout left = new FrameLayout(c);
        left.setBackground(CwUi.box(c, CwUi.BG_PANEL, CwUi.LINE));
        body.addView(left, lp(0, -1, 0.44f));

        TextView empty = CwUi.text(c, "Sélectionne un véhicule", 13f, CwUi.MUTED, CwUi.medium(c));
        empty.setGravity(Gravity.CENTER);
        detailEmpty = empty;
        left.addView(empty, new LayoutParams(-1, -1));

        LinearLayout detail = col(c);
        detail.setPadding(dp(14), dp(12), dp(14), dp(12));
        detailContent = detail;
        left.addView(detail, new LayoutParams(-1, -1));

        LinearLayout head = row(c);
        LinearLayout names = col(c);
        vName = CwUi.text(c, "", 18f, CwUi.TEXT, CwUi.black(c));
        vName.setSingleLine(true);
        vName.setEllipsize(TextUtils.TruncateAt.END);
        names.addView(vName);
        vNick = CwUi.text(c, "", 10.5f, CwUi.ACCENT, CwUi.bold(c));
        vNick.setLetterSpacing(0.08f);
        names.addView(vNick, lpm(-2, -2, 0, 3, 0, 0));
        head.addView(names, lp(0, -2, 1f));
        head.addView(buildPlate(c), new LinearLayout.LayoutParams(-2, dp(26)));
        detail.addView(head);

        vSil = new VehicleSilhouetteView(c);
        detail.addView(vSil, lpm(-1, dp(84), 0, 4, 0, 0));

        HorizontalScrollView stScroll = new HorizontalScrollView(c);
        stScroll.setHorizontalScrollBarEnabled(false);
        statusRow = row(c);
        stScroll.addView(statusRow);
        detail.addView(stScroll, lpm(-1, -2, 0, 2, 0, 0));

        tabRow = row(c);
        detail.addView(tabRow, lpm(-1, dp(32), 0, 10, 0, 0));

        ScrollView tabScroll = new ScrollView(c);
        tabScroll.setVerticalScrollBarEnabled(false);
        tabScroll.setOverScrollMode(OVER_SCROLL_NEVER);
        tabBody = col(c);
        tabScroll.addView(tabBody, new ViewGroup.LayoutParams(-1, -2));
        detail.addView(tabScroll, lpm(-1, 0, 0, 10, 0, 0));
        ((LinearLayout.LayoutParams) tabScroll.getLayoutParams()).weight = 1f;

        // ---- droite : liste ----
        LinearLayout right = col(c);
        LinearLayout.LayoutParams rlp = lp(0, -1, 0.56f);
        rlp.leftMargin = dp(12);
        body.addView(right, rlp);

        HorizontalScrollView catScroll = new HorizontalScrollView(c);
        catScroll.setHorizontalScrollBarEnabled(false);
        catRow = row(c);
        catScroll.addView(catRow);
        right.addView(catScroll);

        LinearLayout sortLine = row(c);
        TextView sortLbl = CwUi.eyebrow(c, "Trier");
        sortLine.addView(sortLbl, lpm(-2, -2, 0, 0, 8, 0));
        sortRow = row(c);
        sortLine.addView(sortRow);
        right.addView(sortLine, lpm(-1, -2, 0, 8, 0, 0));

        FrameLayout listBox = new FrameLayout(c);
        right.addView(listBox, lpm(-1, 0, 0, 8, 0, 0));
        ((LinearLayout.LayoutParams) listBox.getLayoutParams()).weight = 1f;

        grid = new GridView(c);
        grid.setColumnWidth(dp(150));
        grid.setNumColumns(GridView.AUTO_FIT);
        grid.setStretchMode(GridView.STRETCH_COLUMN_WIDTH);
        grid.setHorizontalSpacing(dp(8));
        grid.setVerticalSpacing(dp(8));
        grid.setSelector(new android.graphics.drawable.ColorDrawable(0));
        grid.setVerticalScrollBarEnabled(false);
        grid.setOverScrollMode(OVER_SCROLL_NEVER);
        grid.setAdapter(adapter);
        grid.setOnItemClickListener((p, v, pos, id) -> {
            if (pos < 0 || pos >= shown.size()) return;
            Veh x = shown.get(pos);
            if (x.id != selId) {
                selId = x.id;
                paint1 = x.c1; paint2 = x.c2;
                sellArmedUntil = 0;
                render();
            }
        });
        listBox.addView(grid, new LayoutParams(-1, -1));
        emptyList = CwUi.text(c, "", 12.5f, CwUi.MUTED, CwUi.medium(c));
        emptyList.setGravity(Gravity.CENTER);
        listBox.addView(emptyList, new LayoutParams(-1, -1));

        // ---- toast ----
        toast = CwUi.text(c, "", 12f, CwUi.TEXT, CwUi.bold(c));
        toast.setPadding(dp(16), dp(10), dp(16), dp(10));
        toast.setAlpha(0f);
        LayoutParams tlp = new LayoutParams(-2, -2, Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL);
        tlp.bottomMargin = dp(20);
        addView(toast, tlp);
    }

    private TextView pill(Context c, String s, boolean on) {
        TextView t = CwUi.text(c, s, 10.5f, on ? 0xFFFFFFFF : CwUi.TEXT, CwUi.bold(c));
        t.setGravity(Gravity.CENTER);
        t.setPadding(dp(10), 0, dp(10), 0);
        t.setBackground(CwUi.box(c, on ? CwUi.ACCENT : 0x1AFFFFFF, on ? 0 : CwUi.LINE));
        return t;
    }

    /** Puce portefeuille : renvoie la valeur ; la puce complete est dans getTag(). */
    private TextView walletChip(Context c, String label) {
        LinearLayout box = col(c);
        box.setPadding(dp(10), dp(5), dp(10), dp(5));
        box.setBackground(CwUi.box(c, 0x14FFFFFF, CwUi.LINE));
        TextView l = CwUi.eyebrow(c, label);
        l.setTextSize(8f);
        box.addView(l);
        TextView v = CwUi.text(c, "0 $", 12.5f, CwUi.TEXT, CwUi.black(c));
        box.addView(v, lpm(-2, -2, 0, 2, 0, 0));
        v.setTag(box);
        return v;
    }

    private View buildPlate(Context c) {
        LinearLayout plate = row(c);
        plate.setBackground(CwUi.box(c, 0xFFF3F4F6, 0xFF1F2937));
        vPlateBand = CwUi.text(c, "CW", 8.5f, 0xFFFFFFFF, CwUi.black(c));
        vPlateBand.setGravity(Gravity.CENTER);
        GradientDrawable band = new GradientDrawable();
        band.setColor(CwUi.ACCENT);
        band.setCornerRadii(new float[]{dp(3), dp(3), 0, 0, 0, 0, dp(3), dp(3)});
        vPlateBand.setBackground(band);
        plate.addView(vPlateBand, new LinearLayout.LayoutParams(dp(18), -1));
        vPlate = CwUi.text(c, "", 12.5f, 0xFF0B0B0D, CwUi.black(c));
        vPlate.setLetterSpacing(0.12f);
        vPlate.setGravity(Gravity.CENTER);
        vPlate.setPadding(dp(8), 0, dp(8), 0);
        plate.addView(vPlate, new LinearLayout.LayoutParams(-2, -1));
        return plate;
    }

    // =====================================================================
    // Rendu
    // =====================================================================
    private void render() {
        if (!open) return;
        Context c = getContext();
        placeText.setText(at ? garage + "  ·  sur place, sortie à l'entrée du garage"
                : "À distance  ·  livraison à côté de toi : " + money(deliv));
        cashText.setText(money(cash));
        bankText.setText(money(bank));
        countText.setText(vehs.size() + " véhicule" + (vehs.size() > 1 ? "s" : "") + "  ·  " + outCount() + " / " + maxOut + " sortis");

        // categories
        catRow.removeAllViews();
        for (int i = 0; i < CAT_KEYS.length; i++) {
            int n = 0;
            for (Veh v : vehs) if (i == 0 || catKeyOf(v.m).equals(CAT_KEYS[i])) n++;
            if (i > 0 && n == 0) continue;
            final int idx = i;
            TextView chip = CwUi.chip(c, CAT_LABELS[i] + "  " + n, cat == i);
            chip.setOnClickListener(v -> { cat = idx; render(); });
            catRow.addView(chip, lpm(-2, dp(32), 0, 0, 6, 0));
        }
        sortRow.removeAllViews();
        for (int i = 0; i < SORT_LABELS.length; i++) {
            final int idx = i;
            TextView chip = CwUi.chip(c, SORT_LABELS[i], sort == i);
            chip.setTextSize(9.5f);
            chip.setPadding(dp(9), dp(5), dp(9), dp(5));
            chip.setOnClickListener(v -> { sort = idx; render(); });
            sortRow.addView(chip, lpm(-2, -2, 0, 0, 5, 0));
        }

        // liste filtree + triee
        shown.clear();
        for (Veh v : vehs) if (cat == 0 || catKeyOf(v.m).equals(CAT_KEYS[cat])) shown.add(v);
        if (sort == 1) Collections.sort(shown, (a, b) -> a.hp - b.hp);
        else if (sort == 2) Collections.sort(shown, (a, b) -> a.fu - b.fu);
        else if (sort == 3) Collections.sort(shown, (a, b) -> b.p - a.p);
        else Collections.sort(shown, (a, b) -> b.f - a.f);
        adapter.notifyDataSetChanged();
        emptyList.setVisibility(shown.isEmpty() ? VISIBLE : GONE);
        emptyList.setText(vehs.isEmpty() ? "Tu n'as encore aucun véhicule.\nPasse chez un concessionnaire." : "Aucun véhicule dans cette catégorie.");

        // fiche
        Veh v = find(selId);
        detailEmpty.setVisibility(v == null ? VISIBLE : GONE);
        detailContent.setVisibility(v == null ? GONE : VISIBLE);
        if (v == null) return;
        vName.setText(v.n);
        vNick.setText(v.k.isEmpty() ? (v.ow == 1 ? "AUCUN SURNOM" : "DOUBLE DES CLÉS") : "« " + v.k.toUpperCase(Locale.FRANCE) + " »");
        vPlate.setText(v.pl);
        vSil.set(VehicleSilhouetteView.kindOf(v.m), VehicleSilhouetteView.saColor(tab == 2 && paint1 >= 0 ? paint1 : v.c1));

        statusRow.removeAllViews();
        if (v.im > 0) status(c, "FOURRIÈRE", RED);
        else status(c, v.o == 1 ? "SORTI" : "AU GARAGE", v.o == 1 ? ORANGE : GREEN);
        if (v.o == 1) status(c, v.lk == 1 ? "VERROUILLÉ" : "OUVERT", v.lk == 1 ? CwUi.ACCENT : CwUi.MUTED);
        status(c, v.ins > 0 ? "ASSURÉ · " + v.ins + " J" : "NON ASSURÉ", v.ins > 0 ? CwUi.ACCENT : ORANGE);
        if (v.w >= 85) status(c, "MOTEUR USÉ", RED);
        if (v.f == 1) status(c, "★ FAVORI", CwUi.ACCENT);
        if (v.ow == 1 && !v.sh.isEmpty()) status(c, "CLÉS : " + v.sh, CwUi.MUTED);

        tabRow.removeAllViews();
        for (int i = 0; i < TAB_LABELS.length; i++) {
            final int idx = i;
            boolean on = tab == i;
            TextView t = CwUi.text(c, TAB_LABELS[i], 10.5f, on ? CwUi.TEXT : CwUi.MUTED, CwUi.bold(c));
            t.setGravity(Gravity.CENTER);
            t.setLetterSpacing(0.14f);
            GradientDrawable under = new GradientDrawable();
            under.setColor(on ? 0x1F3B82FF : 0x00000000);
            under.setStroke(0, 0);
            t.setBackground(new android.graphics.drawable.LayerDrawable(new android.graphics.drawable.Drawable[]{under, underline(on)}));
            t.setClickable(true);
            t.setOnClickListener(x -> { tab = idx; sellArmedUntil = 0; hideKeyboard(); render(); });
            tabRow.addView(t, lp(0, -1, 1f));
        }

        tabBody.removeAllViews();
        switch (tab) {
            case 1: renderWorkshop(c, v); break;
            case 2: renderStyle(c, v); break;
            case 3: renderPapers(c, v); break;
            default: renderOverview(c, v); break;
        }
    }

    private android.graphics.drawable.Drawable underline(boolean on) {
        android.graphics.drawable.ShapeDrawable line = new android.graphics.drawable.ShapeDrawable();
        line.getPaint().setColor(on ? CwUi.ACCENT : 0x1FFFFFFF);
        android.graphics.drawable.InsetDrawable in = new android.graphics.drawable.InsetDrawable(line, 0, 0, 0, 0);
        android.graphics.drawable.LayerDrawable ld = new android.graphics.drawable.LayerDrawable(new android.graphics.drawable.Drawable[]{in});
        ld.setLayerGravity(0, Gravity.BOTTOM);
        ld.setLayerHeight(0, on ? dp(2) : Math.max(1, dp(1)));
        return ld;
    }

    private void status(Context c, String s, int color) {
        TextView t = CwUi.text(c, s, 9f, color, CwUi.bold(c));
        t.setLetterSpacing(0.1f);
        t.setPadding(dp(8), dp(4), dp(8), dp(4));
        t.setBackground(CwUi.box(c, CwUi.alpha(color, 0.12f), CwUi.alpha(color, 0.45f)));
        statusRow.addView(t, lpm(-2, -2, 0, 0, 6, 0));
    }

    // ---------------- APERCU ----------------
    private void renderOverview(Context c, Veh v) {
        gauge(c, "État général", v.hp + " %", v.hp, v.hp < 30 ? RED : v.hp < 60 ? ORANGE : CwUi.ACCENT);
        gauge(c, "Carburant", v.fu + " L / 100", v.fu, v.fu < 15 ? RED : v.fu < 35 ? ORANGE : CwUi.ACCENT);
        gauge(c, "Usure moteur", v.w + " %", v.w, v.w >= 85 ? RED : v.w >= 60 ? ORANGE : GREEN);

        LinearLayout facts = row(c);
        fact(c, facts, "COMPTEUR", num(v.km) + " km");
        fact(c, facts, "VALEUR", money(v.p));
        fact(c, facts, "REVENTE", money(v.s));
        tabBody.addView(facts, lpm(-1, -2, 0, 6, 0, 8));

        int out = outCount();
        boolean owner = v.ow == 1;
        if (v.o == 0) {
            String label = v.im > 0 ? "À LA FOURRIÈRE" : at ? "SORTIR LE VÉHICULE" : "SE FAIRE LIVRER · " + money(deliv);
            TextView take = action(c, label, true);
            take.setEnabled(!busy && v.im == 0 && (out < maxOut || !owner));
            take.setOnClickListener(x -> send("take " + v.id));
            tabBody.addView(take, lpm(-1, dp(44), 0, 0, 0, 6));
            if (v.im == 0 && out >= maxOut && owner) hint(c, "Limite atteinte : range d'abord un véhicule (" + maxOut + " sortis max).");
        } else {
            LinearLayout r = row(c);
            TextView store = action(c, "RANGER", true);
            store.setEnabled(!busy);
            store.setOnClickListener(x -> send("store " + v.id));
            r.addView(store, lp(0, dp(44), 1f));
            TextView gps = action(c, "GPS", false);
            gps.setEnabled(!busy);
            gps.setOnClickListener(x -> send("gps " + v.id));
            LinearLayout.LayoutParams g = lp(0, dp(44), 0.6f);
            g.leftMargin = dp(6);
            r.addView(gps, g);
            TextView lock = action(c, v.lk == 1 ? "OUVRIR" : "VERROUILLER", false);
            lock.setEnabled(!busy);
            lock.setOnClickListener(x -> send("lock " + v.id));
            LinearLayout.LayoutParams l = lp(0, dp(44), 0.9f);
            l.leftMargin = dp(6);
            r.addView(lock, l);
            tabBody.addView(r, lpm(-1, -2, 0, 0, 0, 6));
            hint(c, at ? "Ranger : le véhicule doit être garé devant ce garage." : "Ranger à distance : véhicule vide, à moins de 25 m de toi.");
        }
        TextView fav = action(c, v.f == 1 ? "★  RETIRER DES FAVORIS" : "☆  AJOUTER AUX FAVORIS", false);
        fav.setEnabled(!busy);
        fav.setOnClickListener(x -> send("fav " + v.id));
        tabBody.addView(fav, lpm(-1, dp(38), 0, 4, 0, 0));
    }

    private void gauge(Context c, String label, String value, int pct, int color) {
        LinearLayout g = col(c);
        LinearLayout top = row(c);
        top.addView(CwUi.text(c, label, 11f, CwUi.MUTED, CwUi.medium(c)), lp(0, -2, 1f));
        top.addView(CwUi.text(c, value, 11.5f, CwUi.TEXT, CwUi.bold(c)));
        g.addView(top);
        FrameLayout track = new FrameLayout(c);
        track.setBackground(CwUi.box(c, 0x1FFFFFFF, 0));
        View fill = new View(c);
        GradientDrawable fd = new GradientDrawable(GradientDrawable.Orientation.LEFT_RIGHT, new int[]{CwUi.alpha(color, 0.55f), color});
        fd.setCornerRadius(dp(2));
        fill.setBackground(fd);
        track.addView(fill, new LayoutParams(0, -1));
        g.addView(track, lpm(-1, dp(5), 0, 5, 0, 0));
        tabBody.addView(g, lpm(-1, -2, 0, 0, 0, 9));
        final int p = Math.max(0, Math.min(100, pct));
        track.post(() -> {
            ViewGroup.LayoutParams fl = fill.getLayoutParams();
            fl.width = Math.round(track.getWidth() * p / 100f);
            fill.setLayoutParams(fl);
        });
    }

    private void fact(Context c, LinearLayout parent, String label, String value) {
        LinearLayout f = col(c);
        f.setPadding(dp(9), dp(7), dp(9), dp(7));
        f.setBackground(CwUi.box(c, 0x12FFFFFF, CwUi.LINE));
        TextView l = CwUi.eyebrow(c, label);
        l.setTextSize(8f);
        f.addView(l);
        TextView v = CwUi.text(c, value, 11.5f, CwUi.TEXT, CwUi.black(c));
        v.setSingleLine(true);
        v.setEllipsize(TextUtils.TruncateAt.END);
        f.addView(v, lpm(-2, -2, 0, 3, 0, 0));
        LinearLayout.LayoutParams p = lp(0, -2, 1f);
        if (parent.getChildCount() > 0) p.leftMargin = dp(6);
        parent.addView(f, p);
    }

    private TextView action(Context c, String label, boolean primary) {
        TextView t = primary ? CwUi.primaryButton(c, label) : CwUi.ghostButton(c, label);
        t.setTextSize(11.5f);
        t.setSingleLine(true);
        t.setEllipsize(TextUtils.TruncateAt.END);
        t.setPadding(dp(10), 0, dp(10), 0);
        return t;
    }

    private void hint(Context c, String s) {
        TextView t = CwUi.text(c, s, 10f, CwUi.MUTED, CwUi.regular(c));
        t.setLineSpacing(0, 1.15f);
        tabBody.addView(t, lpm(-1, -2, 0, 2, 0, 8));
    }

    private void section(Context c, String s) {
        TextView t = CwUi.eyebrow(c, s);
        t.setTextColor(CwUi.ACCENT);
        tabBody.addView(t, lpm(-1, -2, 0, tabBody.getChildCount() == 0 ? 0 : 10, 0, 6));
    }

    /** Ligne de service : titre, detail, prix, bouton. */
    private void service(Context c, String title, String detail, String price, String button, boolean enabled, Runnable run) {
        LinearLayout r = row(c);
        r.setPadding(dp(10), dp(8), dp(8), dp(8));
        r.setBackground(CwUi.box(c, 0x10FFFFFF, CwUi.LINE));
        LinearLayout txt = col(c);
        txt.addView(CwUi.text(c, title, 12.5f, CwUi.TEXT, CwUi.bold(c)));
        TextView d = CwUi.text(c, detail, 10f, CwUi.MUTED, CwUi.regular(c));
        txt.addView(d, lpm(-2, -2, 0, 3, 0, 0));
        r.addView(txt, lp(0, -2, 1f));
        if (price != null) {
            TextView p = CwUi.text(c, price, 11.5f, CwUi.ACCENT, CwUi.black(c));
            r.addView(p, lpm(-2, -2, 6, 0, 8, 0));
        }
        TextView b = action(c, button, enabled);
        b.setEnabled(enabled && !busy);
        b.setOnClickListener(x -> run.run());
        r.addView(b, new LinearLayout.LayoutParams(dp(96), dp(34)));
        tabBody.addView(r, lpm(-1, -2, 0, 0, 0, 6));
    }

    // ---------------- ATELIER ----------------
    private void renderWorkshop(Context c, Veh v) {
        boolean workshop = v.o == 0 || at;
        section(c, "Atelier " + (at ? "· " + garage : "· à distance"));
        if (!workshop) hint(c, "Véhicule sorti : gare-le devant un garage et ouvre le garage sur place pour l'atelier.");
        int dmg = 100 - v.hp;
        service(c, "Réparation complète", dmg > 0 ? "Carrosserie + moteur · " + dmg + " % de dégâts" : "Aucun dégât",
                dmg > 0 ? money(dmg * feeRep) : null, "RÉPARER", workshop && dmg > 0 && v.im == 0, () -> send("repair " + v.id));
        service(c, "Entretien", v.w > 0 ? "Vidange, filtres, freins · usure " + v.w + " %" : "Entretien à jour",
                v.w > 0 ? money(feeSvb + v.w * feeSvw) : null, "ENTRETIEN", workshop && v.w > 0 && v.im == 0, () -> send("service " + v.id));
        int liters = 100 - v.fu;
        service(c, "Plein d'essence", liters > 0 ? liters + " L à remettre · " + feeFl + " $ / L" : "Réservoir plein",
                liters > 0 ? money(liters * feeFl) : null, "FAIRE LE PLEIN", workshop && liters > 0 && v.im == 0, () -> send("refuel " + v.id));
        if (v.im > 0) {
            section(c, "Fourrière");
            service(c, "Sortie de fourrière", "Véhicule détruit sans assurance", money(v.im), "PAYER", true, () -> send("unimpound " + v.id));
        }
        hint(c, "Paiement en liquide d'abord, sinon par la banque. L'usure augmente de 1 % tous les 15 km ; au-delà de 85 % le moteur peut caler.");
    }

    // ---------------- STYLE ----------------
    private void renderStyle(Context c, Veh v) {
        boolean owner = v.ow == 1;
        if (!owner) { hint(c, "Seul le propriétaire peut personnaliser ce véhicule."); return; }
        section(c, "Surnom");
        final EditText nick = input(c, v.k, "Ex. Bébé noir", 16, false);
        tabBody.addView(inputRow(c, nick, "ENREGISTRER", () -> {
            String s = nick.getText().toString().trim().replaceAll("[^A-Za-z0-9 \\-]", "").replace(' ', '_');
            send("nick " + v.id + (s.isEmpty() ? "" : " " + s));
        }), lpm(-1, -2, 0, 0, 0, 4));
        hint(c, "Gratuit. Lettres, chiffres, espaces (16 max). Vide = retirer.");

        section(c, "Plaque personnalisée · " + money(feePlate));
        final EditText plate = input(c, "", v.pl, 8, true);
        tabBody.addView(inputRow(c, plate, "COMMANDER", () -> {
            String s = plate.getText().toString().toUpperCase(Locale.FRANCE).replaceAll("[^A-Z0-9\\-]", "");
            if (s.length() < 3) { showToast("Plaque : 3 à 8 caractères (A-Z, 0-9, -).", false); return; }
            send("plate " + v.id + " " + s);
        }), lpm(-1, -2, 0, 0, 0, 4));
        hint(c, "Unique sur le serveur, posée à la prochaine sortie du véhicule.");

        section(c, "Peinture · " + money(feePaint));
        swatches(c, "Couleur principale", true);
        swatches(c, "Couleur secondaire", false);
        boolean changed = paint1 >= 0 && paint2 >= 0 && (paint1 != v.c1 || paint2 != v.c2);
        TextView apply = action(c, changed ? "APPLIQUER LA PEINTURE · " + money(feePaint) : "CHOISIS DES COULEURS", true);
        apply.setEnabled(changed && !busy && (v.o == 0 || at));
        apply.setOnClickListener(x -> send("paint " + v.id + " " + paint1 + " " + paint2));
        tabBody.addView(apply, lpm(-1, dp(42), 0, 6, 0, 4));
        if (v.o == 1 && !at) hint(c, "Véhicule sorti : la peinture se fait devant un garage.");
    }

    private void swatches(Context c, String label, final boolean primary) {
        TextView l = CwUi.text(c, label, 10.5f, CwUi.MUTED, CwUi.medium(c));
        tabBody.addView(l, lpm(-1, -2, 0, 2, 0, 5));
        LinearLayout grid = col(c);
        LinearLayout line = null;
        int perLine = 16;
        for (int i = 0; i < PAINT_IDS.length; i++) {
            if (i % perLine == 0) { line = row(c); grid.addView(line, lpm(-1, -2, 0, 0, 0, 5)); }
            final int id = PAINT_IDS[i];
            boolean on = primary ? paint1 == id : paint2 == id;
            View sw = new View(c);
            GradientDrawable d = new GradientDrawable();
            d.setColor(VehicleSilhouetteView.saColor(id));
            d.setCornerRadius(dp(2));
            d.setStroke(on ? dp(2) : Math.max(1, dp(1)), on ? CwUi.ACCENT : 0x40FFFFFF);
            sw.setBackground(d);
            sw.setClickable(true);
            sw.setOnClickListener(x -> {
                if (primary) paint1 = id; else paint2 = id;
                render();
            });
            LinearLayout.LayoutParams p = lp(0, dp(20), 1f);
            p.rightMargin = dp(4);
            line.addView(sw, p);
        }
        tabBody.addView(grid);
    }

    // ---------------- PAPIERS ----------------
    private void renderPapers(Context c, Veh v) {
        boolean owner = v.ow == 1;
        section(c, "Assurance tous risques");
        if (v.ins > 0) hint(c, "Active encore " + v.ins + " jour" + (v.ins > 1 ? "s" : "") + " : remorquage et réparation gratuits si le véhicule est détruit.");
        else hint(c, "Sans assurance, un véhicule détruit part à la fourrière (forfait + 5 % de sa valeur).");
        int i7 = Math.max(feeImin, v.p / 100 * feeI7), i30 = Math.max(feeImin, v.p / 100 * feeI30);
        service(c, "Formule 7 jours", v.ins > 0 ? "Prolonge la couverture" : "Couverture immédiate", money(i7), "SOUSCRIRE", owner, () -> send("insure " + v.id + " 7"));
        service(c, "Formule 30 jours", "La plus avantageuse (-25 %)", money(i30), "SOUSCRIRE", owner, () -> send("insure " + v.id + " 30"));

        section(c, "Double des clés");
        if (!owner) { hint(c, "Ce véhicule t'a été confié : tu peux le sortir, le ranger, le localiser et l'entretenir."); }
        else {
            hint(c, v.sh.isEmpty() ? "Personne n'a de double. Donne-le à un joueur connecté (son ID)." : "Double actuellement chez " + v.sh + ".");
            final EditText target = input(c, "", "ID du joueur", 4, false);
            target.setInputType(InputType.TYPE_CLASS_NUMBER);
            tabBody.addView(inputRow(c, target, "DONNER", () -> {
                String s = target.getText().toString().replaceAll("[^0-9]", "");
                if (s.isEmpty()) { showToast("Indique l'ID du joueur.", false); return; }
                send("keys " + v.id + " " + s);
            }), lpm(-1, -2, 0, 0, 0, 4));
            if (!v.sh.isEmpty()) {
                TextView back = action(c, "REPRENDRE LE DOUBLE", false);
                back.setEnabled(!busy);
                back.setOnClickListener(x -> send("keys " + v.id));
                tabBody.addView(back, lpm(-1, dp(36), 0, 2, 0, 4));
            }
        }

        if (!owner) return;
        section(c, "Vendre à un joueur");
        hint(c, "L'acheteur doit être à côté de toi ; il reçoit la fiche du véhicule et accepte (60 s). Véhicule rangé obligatoire.");
        LinearLayout r = row(c);
        final EditText buyer = input(c, "", "ID", 4, false);
        buyer.setInputType(InputType.TYPE_CLASS_NUMBER);
        r.addView(buyer, lp(0, dp(38), 0.35f));
        final EditText price = input(c, "", "Prix en dollars", 9, false);
        price.setInputType(InputType.TYPE_CLASS_NUMBER);
        LinearLayout.LayoutParams pp = lp(0, dp(38), 0.65f);
        pp.leftMargin = dp(6);
        r.addView(price, pp);
        TextView offer = action(c, "PROPOSER", true);
        offer.setEnabled(!busy && v.o == 0);
        offer.setOnClickListener(x -> {
            String id = buyer.getText().toString().replaceAll("[^0-9]", "");
            String pr = price.getText().toString().replaceAll("[^0-9]", "");
            if (id.isEmpty() || pr.isEmpty() || pr.length() > 8) { showToast("Indique l'ID de l'acheteur et un prix.", false); return; }
            send("offer " + v.id + " " + id + " " + pr);
        });
        LinearLayout.LayoutParams op = new LinearLayout.LayoutParams(dp(96), dp(38));
        op.leftMargin = dp(6);
        r.addView(offer, op);
        tabBody.addView(r, lpm(-1, -2, 0, 0, 0, 6));

        section(c, "Revendre au concessionnaire");
        final boolean armed = System.currentTimeMillis() < sellArmedUntil;
        TextView sell = action(c, armed ? "CONFIRMER : VENDRE POUR " + money(v.s) : "REVENDRE · " + money(v.s) + " (" + sellPct + " %)", false);
        sell.setTextColor(RED);
        sell.setBackground(CwUi.button(c, armed ? 0x33FF5A63 : 0x00000000, 0x4DFF5A63, 0x80FF5A63));
        sell.setEnabled(!busy && v.o == 0);
        sell.setOnClickListener(x -> {
            if (System.currentTimeMillis() < sellArmedUntil) { sellArmedUntil = 0; send("sell " + v.id); }
            else { sellArmedUntil = System.currentTimeMillis() + 4000; render(); ui.postDelayed(this::render, 4100); }
        });
        tabBody.addView(sell, lpm(-1, dp(42), 0, 0, 0, 4));
        hint(c, v.o == 1 ? "Range d'abord le véhicule pour le vendre." : "Définitif : l'argent est versé en liquide.");
    }

    private EditText input(Context c, String value, String hintText, int max, boolean caps) {
        EditText e = new EditText(c);
        e.setText(value);
        e.setHint(hintText);
        e.setHintTextColor(0x66FFFFFF);
        e.setTextColor(CwUi.TEXT);
        e.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.5f);
        e.setTypeface(CwUi.bold(c));
        e.setSingleLine(true);
        e.setPadding(dp(10), 0, dp(10), 0);
        e.setBackground(CwUi.box(c, 0x14FFFFFF, CwUi.LINE));
        e.setFilters(new InputFilter[]{new InputFilter.LengthFilter(max)});
        e.setInputType(caps ? (InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS) : InputType.TYPE_CLASS_TEXT);
        e.setImeOptions(EditorInfo.IME_ACTION_DONE | EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_FLAG_NO_FULLSCREEN);
        return e;
    }

    private View inputRow(Context c, final EditText e, String button, final Runnable run) {
        LinearLayout r = row(c);
        r.addView(e, lp(0, dp(38), 1f));
        TextView b = action(c, button, true);
        b.setEnabled(!busy);
        b.setOnClickListener(x -> { hideKeyboard(); run.run(); });
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(dp(110), dp(38));
        p.leftMargin = dp(6);
        r.addView(b, p);
        e.setOnEditorActionListener((tv, actionId, ev) -> {
            if (actionId == EditorInfo.IME_ACTION_DONE) { hideKeyboard(); run.run(); return true; }
            return false;
        });
        return r;
    }

    private void hideKeyboard() {
        try {
            InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            if (imm != null) imm.hideSoftInputFromWindow(getWindowToken(), 0);
        } catch (Throwable ignored) { }
    }

    // =====================================================================
    // Actions
    // =====================================================================
    private void send(String command) {
        if (busy || cb == null) return;
        busy = true;
        render();
        ui.removeCallbacks(unbusy);
        ui.postDelayed(unbusy, 4000);   // filet : le serveur n'a pas repondu
        cb.onAction(command);
    }

    private void showToast(String msg, boolean ok) {
        Context c = getContext();
        toast.setText((ok ? "✓  " : "✕  ") + msg);
        toast.setTextColor(ok ? CwUi.TEXT : 0xFFFFD0D3);
        toast.setBackground(CwUi.box(c, ok ? 0xF20F1B33 : 0xF2361114, ok ? CwUi.ACCENT : RED));
        toast.animate().cancel();
        toast.setTranslationY(dp(8));
        toast.animate().alpha(1f).translationY(0).setDuration(160).start();
        ui.removeCallbacks(hideToast);
        ui.postDelayed(hideToast, 3200);
    }

    // =====================================================================
    // Cartes des vehicules
    // =====================================================================
    private final class CardAdapter extends BaseAdapter {
        @Override public int getCount() { return shown.size(); }
        @Override public Object getItem(int p) { return shown.get(p); }
        @Override public long getItemId(int p) { return shown.get(p).id; }

        @Override
        public View getView(int pos, View convert, ViewGroup parent) {
            Context c = parent.getContext();
            Holder h;
            if (convert != null && convert.getTag() instanceof Holder) {
                h = (Holder) convert.getTag();
            } else {
                h = new Holder(c);
                h.root.setTag(h);
            }
            h.bind(shown.get(pos));
            return h.root;
        }
    }

    private final class Holder {
        final LinearLayout root;
        final TextView name, nick, plate, state, star;
        final VehicleSilhouetteView sil;
        final View hpBar, fuBar;

        Holder(Context c) {
            root = col(c);
            root.setPadding(dp(10), dp(8), dp(10), dp(9));
            LinearLayout top = row(c);
            name = CwUi.text(c, "", 12.5f, CwUi.TEXT, CwUi.black(c));
            name.setSingleLine(true);
            name.setEllipsize(TextUtils.TruncateAt.END);
            top.addView(name, lp(0, -2, 1f));
            star = CwUi.text(c, "★", 12f, CwUi.ACCENT, CwUi.bold(c));
            top.addView(star);
            root.addView(top);
            nick = CwUi.text(c, "", 9f, CwUi.MUTED, CwUi.bold(c));
            nick.setSingleLine(true);
            nick.setEllipsize(TextUtils.TruncateAt.END);
            nick.setLetterSpacing(0.08f);
            root.addView(nick, lpm(-1, -2, 0, 2, 0, 0));
            sil = new VehicleSilhouetteView(c);
            root.addView(sil, lpm(-1, dp(50), 0, 2, 0, 2));
            LinearLayout meta = row(c);
            plate = CwUi.text(c, "", 9f, 0xFF0B0B0D, CwUi.black(c));
            plate.setLetterSpacing(0.1f);
            plate.setPadding(dp(5), dp(2), dp(5), dp(2));
            plate.setBackground(CwUi.box(c, 0xFFE5E7EB, 0));
            meta.addView(plate);
            state = CwUi.text(c, "", 9f, GREEN, CwUi.bold(c));
            state.setGravity(Gravity.END);
            meta.addView(state, lp(0, -2, 1f));
            root.addView(meta);
            hpBar = bar(c);
            fuBar = bar(c);
        }

        private View bar(Context c) {
            FrameLayout track = new FrameLayout(c);
            track.setBackground(CwUi.box(c, 0x1FFFFFFF, 0));
            View fill = new View(c);
            track.addView(fill, new LayoutParams(0, -1));
            root.addView(track, lpm(-1, dp(3), 0, 5, 0, 0));
            return fill;
        }

        private void setBar(View fill, int pct, int color) {
            fill.setBackgroundColor(color);
            final View track = (View) fill.getParent();
            final int p = Math.max(0, Math.min(100, pct));
            track.post(() -> {
                ViewGroup.LayoutParams l = fill.getLayoutParams();
                l.width = Math.round(track.getWidth() * p / 100f);
                fill.setLayoutParams(l);
            });
        }

        void bind(Veh v) {
            Context c = root.getContext();
            boolean sel = v.id == selId;
            root.setBackground(CwUi.box(c, sel ? 0x263B82FF : CwUi.BG_PANEL, sel ? CwUi.ACCENT : CwUi.LINE));
            name.setText(v.n);
            nick.setText(!v.k.isEmpty() ? "« " + v.k.toUpperCase(Locale.FRANCE) + " »" : v.ow == 1 ? catLabel(v.m) : "DOUBLE DES CLÉS");
            star.setVisibility(v.f == 1 ? VISIBLE : GONE);
            sil.set(VehicleSilhouetteView.kindOf(v.m), VehicleSilhouetteView.saColor(v.c1));
            plate.setText(v.pl);
            if (v.im > 0) { state.setText("● FOURRIÈRE"); state.setTextColor(RED); }
            else if (v.o == 1) { state.setText("● SORTI"); state.setTextColor(ORANGE); }
            else { state.setText("● AU GARAGE"); state.setTextColor(GREEN); }
            setBar(hpBar, v.hp, v.hp < 30 ? RED : CwUi.ACCENT);
            setBar(fuBar, v.fu, v.fu < 15 ? RED : 0xFF8B93A1);
        }
    }

    private static String catLabel(int model) {
        switch (VehicleSilhouetteView.kindOf(model)) {
            case VehicleSilhouetteView.BIKE: return "MOTO";
            case VehicleSilhouetteView.BOAT: return "BATEAU";
            case VehicleSilhouetteView.TRUCK: return "UTILITAIRE";
            case VehicleSilhouetteView.AIR: return "AÉRIEN";
            default: return "VOITURE";
        }
    }
}
