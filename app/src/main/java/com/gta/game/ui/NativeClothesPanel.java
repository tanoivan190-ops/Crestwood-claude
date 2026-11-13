package com.gta.game.ui;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.FrameLayout;
import android.widget.GridView;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.load.engine.DiskCacheStrategy;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Studio de style Crestwood : editeur de vetements 100 % natif Android (plus de WebView).
 *
 * Meme protocole que l'ancienne page assets/clothes/index.html :
 *   moteur -> Java : open(json) / setState(json) / close()   (CJavaWrapper::ClothesPanelOpen/State/Close)
 *   Java -> moteur : Callback (option, genre, camera, enregistrer, annuler) -> SAMP.clothesCommand(type, key, value)
 * Les miniatures viennent de assets/clothes/south/ (catalogue.json).
 *
 * Le panneau est pose au-dessus du jeu ; seules ses zones (rail, panneau, camera) captent le tactile,
 * le reste de l'ecran laisse voir (et toucher) le personnage.
 */
public class NativeClothesPanel extends FrameLayout {

    public interface Callback {
        void onOption(String key, int value);
        void onGender(int gender);
        void onCamera(String action, int value);
        void onSave();
        void onCancel();
    }

    private static final String TAG = "CwClothes";

    // onglet, titre, focus camera (0..100), zoom (100..450)
    private static final Object[][] TABS = {
            {"haut", "Hauts", 73, 240},
            {"bas", "Bas", 39, 235},
            {"chaussures", "Chaussures", 8, 155},
            {"sacs", "Sacs", 66, 270},
            {"tete", "Tête", 95, 140},
            {"bijoux", "Bijoux", 85, 175},
            {"cheveux", "Cheveux", 97, 140},
            {"visage", "Visage", 97, 140},
            {"tatouages", "Tatouages", 70, 260},
            {"police", "Uniforme", 70, 260},
    };

    // corps : valeur envoyee au moteur (SkinBodies.h) + libelle
    // [v21] seulement les 2 corps complets (catalogue + textures) : les corps AVZ / 289 sont convertis
    private static final int[] BODY_VALUES = {0, 1};
    private static final String[] BODY_LABELS = {"Homme", "Femme"};

    private static final Map<String, String> LABELS = new HashMap<>();
    static {
        String[][] l = {
                {"skinTone", "Teint"}, {"face", "Visage"}, {"hairStyle", "Coupe"}, {"hairColor", "Couleur des cheveux"},
                {"eyebrowStyle", "Sourcils"}, {"eyebrowColor", "Couleur des sourcils"}, {"beardStyle", "Barbe"},
                {"beardColor", "Couleur de la barbe"}, {"makeupStyle", "Maquillage"}, {"makeupColor", "Teinte du maquillage"},
                {"topStyle", "Haut"}, {"topColor", "Couleur du haut"}, {"bottomStyle", "Bas"}, {"bottomColor", "Couleur du bas"},
                {"shoesStyle", "Chaussures"}, {"shoesColor", "Couleur des chaussures"}, {"hatStyle", "Chapeau"},
                {"hatColor", "Couleur du chapeau"}, {"glasseStyle", "Lunettes"}, {"glasseColor", "Couleur des lunettes"},
                {"maskStyle", "Masque"}, {"maskColor", "Couleur du masque"}, {"earStyle", "Boucles d'oreilles"},
                {"earColor", "Couleur des boucles"}, {"clockStyle", "Montre"}, {"clockColor", "Couleur de la montre"},
                {"necklaceStyle", "Collier"}, {"necklaceColor", "Couleur du collier"}, {"backpackStyle", "Sac à dos"},
                {"backpackColor", "Couleur du sac"}, {"tattooArmLeft", "Tatouage · bras gauche"},
                {"tattooArmRight", "Tatouage · bras droit"}, {"tattooLegLeft", "Tatouage · jambe gauche"},
                {"tattooLegRight", "Tatouage · jambe droite"}, {"tattooBack", "Tatouage · dos"},
                {"tattooFront", "Tatouage · torse"}, {"policeBelt", "Ceinturon"}, {"policeVest", "Gilet"},
                {"policeHat", "Casquette de service"},
                {"southTop", "Hauts"}, {"southBottom", "Bas"}, {"southShoes", "Chaussures"},
                {"southBackpack", "Sacs"}, {"southMask", "Masques"}, {"southEar", "Boucles d'oreilles"},
                {"southClock", "Montres"},
        };
        for (String[] p : l) LABELS.put(p[0], p[1]);
    }

    // ---- donnees ----
    private static final class Field {
        String key, label, tab;
        int min, max, val;
    }

    private static final class Item {
        String name, texture;
    }

    private static final class Group {
        int model;
        String key, tab;
        final List<Item> items = new ArrayList<>();
    }

    private static List<Group> sCatalogue;   // charge une fois (assets/clothes/south/catalogue.json)

    private final Callback cb;
    private final Handler ui = new Handler(Looper.getMainLooper());
    private final List<Field> fields = new ArrayList<>();
    private final Map<String, Integer> pending = new HashMap<>();
    private int model = -1, gender = 0;
    private boolean canSwitchGender, supported;
    private boolean creation, finished, original;
    private String active = "haut";
    private String focusedKey = "";
    private long genderLockUntil;
    private boolean bodyMigrated;   // [v21] ancien corps (Homme II, Femme II...) -> Homme / Femme, une fois par ouverture
    private int zoom = 280;

    // ---- vues ----
    private LinearLayout rail, panel, fieldsBox, bodyRow, selection;
    private ScrollView fieldsScroll;
    private GridView grid;
    private TextView title, statusText, pieceName, pieceNumber, pieceCount, modeCollection, modeSettings, creationBanner, closeX, cancelBtn, saveBtn;
    private final ItemAdapter adapter = new ItemAdapter();
    private final List<Object[]> gridItems = new ArrayList<>();   // {Group, index}

    public NativeClothesPanel(Context c, Callback cb) {
        super(c);
        this.cb = cb;
        setClickable(false);            // le vide laisse passer le tactile vers le jeu
        setFocusable(false);
        setVisibility(GONE);
        build();
        loadCatalogueAsync();
    }

    // =====================================================================
    // API appelee par SAMP (thread UI)
    // =====================================================================
    public void open(String json, boolean creationMode) {
        finished = false;
        bodyMigrated = false;
        creation = creationMode;
        original = false;
        focusedKey = "";
        pending.clear();
        creationBanner.setVisibility(creation ? VISIBLE : GONE);
        closeX.setVisibility(creation ? GONE : VISIBLE);
        cancelBtn.setVisibility(creation ? GONE : VISIBLE);
        setVisibility(VISIBLE);
        setAlpha(0f);
        animate().alpha(1f).setDuration(180).start();
        panel.setTranslationX(CwUi.dp(getContext(), 40));
        panel.animate().translationX(0).setDuration(220).start();
        setState(json);
        focusCamera();
    }

    public boolean isOpen() {
        return getVisibility() == VISIBLE;
    }

    public void close() {
        finished = true;
        if (getVisibility() != VISIBLE) return;
        animate().alpha(0f).setDuration(140).withEndAction(() -> setVisibility(GONE)).start();
    }

    public void setState(String json) {
        if (json == null || json.isEmpty()) return;
        try {
            JSONObject o = new JSONObject(json);
            JSONArray arr = o.optJSONArray("fields");
            if (arr == null) return;
            int nextModel = o.optInt("model", -1);
            if (nextModel != model) pending.clear();
            model = nextModel;
            gender = o.optInt("gender", 0);
            canSwitchGender = o.optBoolean("canSwitchGender", false);
            supported = o.optBoolean("supported", false);
            // [v21] corps retires (2 = Homme original 289, 3 = Femme II, 4 = Homme II) : on passe sur le corps
            // complet du meme genre, sinon les pieces ne s'appliquent pas ("je peux pas changer")
            if (!bodyMigrated && !finished && canSwitchGender && (gender == 2 || gender == 3 || gender == 4)) {
                bodyMigrated = true;
                final int target = gender == 3 ? 1 : 0;
                genderLockUntil = System.currentTimeMillis() + 1600;
                pending.clear();
                gender = target;
                ui.post(() -> { if (!finished && cb != null) cb.onGender(target); });
                ui.postDelayed(this::render, 1650);
            }
            fields.clear();
            for (int i = 0; i < arr.length(); i++) {
                JSONObject f = arr.getJSONObject(i);
                Field x = new Field();
                x.key = f.optString("key");
                x.label = f.optString("label");
                x.tab = f.optString("tab");
                x.min = f.optInt("min", 0);
                x.max = f.optInt("max", 0);
                x.val = f.optInt("val", 0);
                Integer p = pending.get(x.key);
                if (p != null) {
                    if (p == x.val) pending.remove(x.key); else x.val = p;
                }
                fields.add(x);
            }
        } catch (Exception e) {
            Log.e(TAG, "etat invalide : " + e.getMessage());
            return;
        }
        render();
    }

    // =====================================================================
    // Construction des vues
    // =====================================================================
    private int dp(float v) { return CwUi.dp(getContext(), v); }

    private void build() {
        Context c = getContext();

        // ---- rail des categories (gauche) ----
        ScrollView railScroll = new ScrollView(c);
        railScroll.setVerticalScrollBarEnabled(false);
        railScroll.setOverScrollMode(OVER_SCROLL_NEVER);
        rail = new LinearLayout(c);
        rail.setOrientation(LinearLayout.VERTICAL);
        railScroll.addView(rail, new ViewGroup.LayoutParams(-1, -2));
        LayoutParams rlp = new LayoutParams(dp(88), -1, Gravity.START);
        rlp.setMargins(dp(14), dp(82), 0, dp(14));
        addView(railScroll, rlp);

        // ---- marque en haut a gauche ----
        LinearLayout brand = new LinearLayout(c);
        brand.setOrientation(LinearLayout.VERTICAL);
        // [v21] l'image du logo (plus le texte coupe)
        ImageView logo = new ImageView(c);
        logo.setScaleType(ImageView.ScaleType.FIT_START);
        logo.setAdjustViewBounds(true);
        try (InputStream in = c.getAssets().open("login/assets/images/cw-logo.png")) {
            android.graphics.BitmapFactory.Options bo = new android.graphics.BitmapFactory.Options();
            bo.inSampleSize = 2;
            logo.setImageBitmap(android.graphics.BitmapFactory.decodeStream(in, null, bo));
        } catch (Exception e) { Log.w(TAG, "logo : " + e.getMessage()); }
        TextView sub = CwUi.eyebrow(c, "Studio de style");
        sub.setTextColor(CwUi.ACCENT);
        brand.addView(logo, new LinearLayout.LayoutParams(dp(126), dp(42)));
        LinearLayout.LayoutParams sl = new LinearLayout.LayoutParams(-2, -2);
        sl.topMargin = dp(4);
        brand.addView(sub, sl);
        LayoutParams blp = new LayoutParams(-2, -2, Gravity.START | Gravity.TOP);
        blp.setMargins(dp(16), dp(16), 0, 0);
        addView(brand, blp);

        // ---- camera (bas gauche) ----
        addView(buildCamera(c), cameraParams());

        // ---- panneau principal (droite) ----
        panel = new LinearLayout(c);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setBackground(CwUi.box(c, CwUi.BG_PANEL, CwUi.LINE));
        panel.setPadding(dp(16), dp(14), dp(16), dp(14));
        panel.setClickable(true);   // capte le tactile de toute sa surface
        int w = Math.min(dp(440), Math.round(c.getResources().getDisplayMetrics().widthPixels * 0.44f));
        LayoutParams plp = new LayoutParams(w, -1, Gravity.END);
        plp.setMargins(0, dp(12), dp(12), dp(12));
        addView(panel, plp);

        // en-tete
        LinearLayout head = new LinearLayout(c);
        head.setOrientation(LinearLayout.HORIZONTAL);
        head.setGravity(Gravity.CENTER_VERTICAL);
        LinearLayout headText = new LinearLayout(c);
        headText.setOrientation(LinearLayout.VERTICAL);
        headText.addView(CwUi.eyebrow(c, "Ton style, tes règles"));
        title = CwUi.text(c, "Hauts", 22f, CwUi.TEXT, CwUi.black(c));
        LinearLayout.LayoutParams tl = new LinearLayout.LayoutParams(-2, -2);
        tl.topMargin = dp(5);
        headText.addView(title, tl);
        head.addView(headText, new LinearLayout.LayoutParams(0, -2, 1f));
        closeX = CwUi.ghostButton(c, "✕");
        closeX.setTextSize(15f);
        closeX.setOnClickListener(v -> cancel());
        head.addView(closeX, new LinearLayout.LayoutParams(dp(38), dp(38)));
        panel.addView(head);

        creationBanner = CwUi.text(c, "CRÉATION DU PERSONNAGE — choisis ta tenue de départ, puis enregistre.", 10.5f, 0xFFFFFFFF, CwUi.bold(c));
        creationBanner.setBackground(CwUi.box(c, CwUi.ACCENT, 0));
        creationBanner.setPadding(dp(10), dp(7), dp(10), dp(7));
        creationBanner.setVisibility(GONE);
        panel.addView(creationBanner, marginTop(-1, -2, 10));

        // corps
        HorizontalScrollView bodyScroll = new HorizontalScrollView(c);
        bodyScroll.setHorizontalScrollBarEnabled(false);
        bodyScroll.setOverScrollMode(OVER_SCROLL_NEVER);
        bodyRow = new LinearLayout(c);
        bodyRow.setOrientation(LinearLayout.HORIZONTAL);
        bodyScroll.addView(bodyRow);
        panel.addView(bodyScroll, marginTop(-1, -2, 12));

        // mode Collection / Reglages
        LinearLayout modes = new LinearLayout(c);
        modes.setOrientation(LinearLayout.HORIZONTAL);
        modeCollection = CwUi.chip(c, "COLLECTION", true);
        modeSettings = CwUi.chip(c, "RÉGLAGES", false);
        modeCollection.setOnClickListener(v -> { original = false; render(); });
        modeSettings.setOnClickListener(v -> { original = true; render(); });
        modes.addView(modeCollection, new LinearLayout.LayoutParams(0, dp(34), 1f));
        LinearLayout.LayoutParams ml = new LinearLayout.LayoutParams(0, dp(34), 1f);
        ml.leftMargin = dp(6);
        modes.addView(modeSettings, ml);
        panel.addView(modes, marginTop(-1, -2, 10));

        statusText = CwUi.text(c, "Chargement de ta garde-robe…", 12f, CwUi.MUTED, CwUi.medium(c));
        statusText.setPadding(0, dp(14), 0, dp(4));
        panel.addView(statusText);

        // piece selectionnee
        selection = new LinearLayout(c);
        selection.setOrientation(LinearLayout.HORIZONTAL);
        selection.setGravity(Gravity.CENTER_VERTICAL);
        selection.setBackground(CwUi.box(c, 0x14FFFFFF, CwUi.LINE));
        selection.setPadding(dp(10), dp(8), dp(8), dp(8));
        LinearLayout selText = new LinearLayout(c);
        selText.setOrientation(LinearLayout.VERTICAL);
        pieceNumber = CwUi.eyebrow(c, "Pièce · original");
        pieceNumber.setTextColor(CwUi.ACCENT);
        pieceName = CwUi.text(c, "Tenue actuelle", 14f, CwUi.TEXT, CwUi.bold(c));
        pieceName.setSingleLine(true);
        pieceName.setEllipsize(TextUtils.TruncateAt.END);
        pieceCount = CwUi.text(c, "", 10f, CwUi.MUTED, CwUi.regular(c));
        selText.addView(pieceNumber);
        selText.addView(pieceName, marginTop(-1, -2, 4));
        selText.addView(pieceCount, marginTop(-1, -2, 3));
        selection.addView(selText, new LinearLayout.LayoutParams(0, -2, 1f));
        TextView prev = CwUi.ghostButton(c, "‹");
        TextView next = CwUi.ghostButton(c, "›");
        TextView clear = CwUi.ghostButton(c, "RETIRER");
        prev.setTextSize(20f);
        next.setTextSize(20f);
        clear.setTextSize(9.5f);
        prev.setOnClickListener(v -> step(-1));
        next.setOnClickListener(v -> step(1));
        clear.setOnClickListener(v -> { Group g = currentGroup(); if (g != null) setOption(g.key, 0); });
        selection.addView(clear, new LinearLayout.LayoutParams(dp(64), dp(36)));
        LinearLayout.LayoutParams bl = new LinearLayout.LayoutParams(dp(38), dp(36));
        bl.leftMargin = dp(6);
        selection.addView(prev, bl);
        LinearLayout.LayoutParams bl2 = new LinearLayout.LayoutParams(dp(38), dp(36));
        bl2.leftMargin = dp(4);
        selection.addView(next, bl2);
        panel.addView(selection, marginTop(-1, -2, 10));

        // grille de la collection
        grid = new GridView(c);
        grid.setColumnWidth(dp(86));
        grid.setNumColumns(GridView.AUTO_FIT);
        grid.setStretchMode(GridView.STRETCH_COLUMN_WIDTH);
        grid.setHorizontalSpacing(dp(6));
        grid.setVerticalSpacing(dp(6));
        grid.setSelector(new android.graphics.drawable.ColorDrawable(0));
        grid.setVerticalScrollBarEnabled(false);
        grid.setOverScrollMode(OVER_SCROLL_NEVER);
        grid.setAdapter(adapter);
        grid.setOnItemClickListener((p, v, pos, id) -> {
            if (pos < 0 || pos >= gridItems.size()) return;
            Group g = (Group) gridItems.get(pos)[0];
            int idx = (Integer) gridItems.get(pos)[1];
            focusedKey = g.key;
            setOption(g.key, idx + 1);
        });
        LinearLayout.LayoutParams gl = new LinearLayout.LayoutParams(-1, 0, 1f);
        gl.topMargin = dp(10);
        panel.addView(grid, gl);

        // reglages fins
        fieldsScroll = new ScrollView(c);
        fieldsScroll.setVerticalScrollBarEnabled(false);
        fieldsScroll.setOverScrollMode(OVER_SCROLL_NEVER);
        fieldsBox = new LinearLayout(c);
        fieldsBox.setOrientation(LinearLayout.VERTICAL);
        fieldsScroll.addView(fieldsBox, new ViewGroup.LayoutParams(-1, -2));
        LinearLayout.LayoutParams fl = new LinearLayout.LayoutParams(-1, 0, 1f);
        fl.topMargin = dp(10);
        panel.addView(fieldsScroll, fl);

        // pied
        TextView hint = CwUi.text(c, "Les changements s'affichent en direct sur ton personnage.", 10f, CwUi.MUTED, CwUi.regular(c));
        panel.addView(hint, marginTop(-1, -2, 10));
        LinearLayout foot = new LinearLayout(c);
        foot.setOrientation(LinearLayout.HORIZONTAL);
        cancelBtn = CwUi.ghostButton(c, "ANNULER");
        cancelBtn.setOnClickListener(v -> cancel());
        saveBtn = CwUi.primaryButton(c, "ENREGISTRER LA TENUE");
        saveBtn.setOnClickListener(v -> save());
        foot.addView(cancelBtn, new LinearLayout.LayoutParams(0, dp(44), 1f));
        LinearLayout.LayoutParams sv = new LinearLayout.LayoutParams(0, dp(44), 1.8f);
        sv.leftMargin = dp(8);
        foot.addView(saveBtn, sv);
        panel.addView(foot, marginTop(-1, -2, 10));
    }

    private LayoutParams cameraParams() {
        LayoutParams lp = new LayoutParams(dp(250), -2, Gravity.START | Gravity.BOTTOM);
        lp.setMargins(dp(114), 0, 0, dp(16));
        return lp;
    }

    private View buildCamera(Context c) {
        LinearLayout cam = new LinearLayout(c);
        cam.setOrientation(LinearLayout.VERTICAL);
        cam.setBackground(CwUi.box(c, CwUi.BG_PANEL, CwUi.LINE));
        cam.setPadding(dp(12), dp(10), dp(12), dp(10));
        cam.setClickable(true);
        cam.addView(CwUi.eyebrow(c, "Aperçu 360°"));
        SeekBar rot = new SeekBar(c);
        rot.setMax(360);
        rot.setProgressTintList(android.content.res.ColorStateList.valueOf(CwUi.ACCENT));
        rot.setThumbTintList(android.content.res.ColorStateList.valueOf(CwUi.TEXT));
        rot.setProgressBackgroundTintList(android.content.res.ColorStateList.valueOf(0x40FFFFFF));
        rot.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar s, int p, boolean user) { if (user) camera("rotate", p); }
            @Override public void onStartTrackingTouch(SeekBar s) {}
            @Override public void onStopTrackingTouch(SeekBar s) {}
        });
        cam.addView(rot, marginTop(-1, dp(30), 4));
        LinearLayout row = new LinearLayout(c);
        row.setOrientation(LinearLayout.HORIZONTAL);
        TextView out = CwUi.ghostButton(c, "−");
        TextView in = CwUi.ghostButton(c, "+");
        TextView reset = CwUi.ghostButton(c, "RECENTRER");
        out.setTextSize(17f);
        in.setTextSize(17f);
        reset.setTextSize(9.5f);
        out.setOnClickListener(v -> { zoom = Math.min(450, zoom + 25); camera("zoom", zoom); });
        in.setOnClickListener(v -> { zoom = Math.max(100, zoom - 25); camera("zoom", zoom); });
        reset.setOnClickListener(v -> { rot.setProgress(0); camera("rotate", 0); focusCamera(); });
        row.addView(out, new LinearLayout.LayoutParams(dp(40), dp(32)));
        LinearLayout.LayoutParams il = new LinearLayout.LayoutParams(dp(40), dp(32));
        il.leftMargin = dp(6);
        row.addView(in, il);
        LinearLayout.LayoutParams rl = new LinearLayout.LayoutParams(0, dp(32), 1f);
        rl.leftMargin = dp(6);
        row.addView(reset, rl);
        cam.addView(row, marginTop(-1, -2, 4));
        return cam;
    }

    private LinearLayout.LayoutParams marginTop(int w, int h, int topDp) {
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(w, h);
        lp.topMargin = dp(topDp);
        return lp;
    }

    // =====================================================================
    // Logique (identique a l'ancien clothes.js)
    // =====================================================================
    private boolean hasTab(String tab) {
        for (Field f : fields) if (tab.equals(f.tab)) return true;
        return false;
    }

    private Field field(String key) {
        for (Field f : fields) if (f.key.equals(key)) return f;
        return null;
    }

    private List<Group> groups() {
        List<Group> out = new ArrayList<>();
        List<Group> cat = sCatalogue;
        if (cat == null) return out;
        int m = (model == 9 || model == 12) ? 9 : 7;
        for (Group g : cat) {
            if (g.model != m) continue;
            Field f = field(g.key);
            if (f != null && active.equals(f.tab)) out.add(g);
        }
        return out;
    }

    private Group currentGroup() {
        List<Group> gs = groups();
        for (Group g : gs) if (g.key.equals(focusedKey)) return g;
        return gs.isEmpty() ? null : gs.get(0);
    }

    private Object[] tabInfo(String tab) {
        for (Object[] t : TABS) if (t[0].equals(tab)) return t;
        return null;
    }

    private void setOption(String key, int value) {
        if (finished) return;
        Field f = field(key);
        if (f == null) return;
        value = Math.max(f.min, Math.min(f.max, value));
        f.val = value;
        pending.put(key, value);
        if (cb != null) cb.onOption(key, value);
        render();
    }

    private void step(int dir) {
        Group g = currentGroup();
        if (g == null || g.items.isEmpty()) return;
        Field f = field(g.key);
        if (f == null) return;
        int n = g.items.size();
        int v = f.val + dir;
        if (v < 1) v = n;
        if (v > n) v = 1;
        focusedKey = g.key;
        setOption(g.key, v);
    }

    private void camera(String action, int value) {
        if (!finished && cb != null) cb.onCamera(action, value);
    }

    private void focusCamera() {
        Object[] t = tabInfo(active);
        if (t == null) return;
        zoom = (Integer) t[3];
        camera("focus", (Integer) t[2]);
        camera("zoom", zoom);
    }

    private void cancel() {
        if (finished || creation) return;
        finished = true;
        if (cb != null) cb.onCancel();
    }

    private void save() {
        if (finished || !supported || fields.isEmpty()) return;
        finished = true;
        if (cb != null) cb.onSave();
    }

    private static String pretty(String name) {
        if (name == null) return "";
        String[] parts = name.replace('_', ' ').trim().split("\\s+");
        StringBuilder b = new StringBuilder();
        for (String p : parts) {
            if (p.isEmpty()) continue;
            if (b.length() > 0) b.append(' ');
            b.append(Character.toUpperCase(p.charAt(0))).append(p.substring(1));
        }
        return b.toString();
    }

    private String labelOf(Field f) {
        String l = LABELS.get(f.key);
        return l != null ? l : (f.label == null || f.label.isEmpty() ? f.key : f.label);
    }

    private void render() {
        Context c = getContext();
        if (!fields.isEmpty() && !hasTab(active)) active = fields.get(0).tab;

        // rail
        rail.removeAllViews();
        for (Object[] t : TABS) {
            final String tab = (String) t[0];
            if (!hasTab(tab)) continue;
            boolean on = tab.equals(active);
            TextView b = CwUi.text(c, ((String) t[1]).toUpperCase(), 10f, on ? 0xFF0B0B0D : CwUi.TEXT, CwUi.bold(c));
            b.setGravity(Gravity.CENTER);
            b.setLetterSpacing(0.08f);
            b.setBackground(on ? CwUi.button(c, CwUi.TEXT, 0xFFD6D8DC, 0) : CwUi.button(c, CwUi.BG_PANEL, 0xE6202227, CwUi.LINE));
            b.setClickable(true);
            b.setOnClickListener(v -> { active = tab; focusedKey = ""; render(); focusCamera(); });
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, dp(46));
            lp.bottomMargin = dp(6);
            rail.addView(b, lp);
        }

        Object[] info = tabInfo(active);
        title.setText(info != null ? (String) info[1] : "Ta tenue");

        // corps
        bodyRow.removeAllViews();
        boolean locked = !canSwitchGender || System.currentTimeMillis() < genderLockUntil;
        for (int i = 0; i < BODY_VALUES.length; i++) {
            final int value = BODY_VALUES[i];
            boolean on = value == gender;
            TextView chip = CwUi.chip(c, BODY_LABELS[i], on);
            chip.setEnabled(!locked || on);
            chip.setOnClickListener(v -> {
                if (finished || value == gender || !canSwitchGender || System.currentTimeMillis() < genderLockUntil) return;
                genderLockUntil = System.currentTimeMillis() + 1600;
                pending.clear();
                gender = value;
                if (cb != null) cb.onGender(value);
                render();
                ui.postDelayed(this::render, 1650);
            });
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-2, dp(32));
            if (i > 0) lp.leftMargin = dp(6);
            bodyRow.addView(chip, lp);
        }

        // modes
        List<Group> gs = groups();
        boolean catalogue = !original && !gs.isEmpty();
        styleChip(modeCollection, !original);
        styleChip(modeSettings, original);

        boolean empty = fields.isEmpty();
        statusText.setVisibility(empty || !supported ? VISIBLE : GONE);
        statusText.setText(empty ? "Chargement de ta garde-robe…" : "Ce personnage n'est pas personnalisable. Choisis un autre corps ci-dessus.");
        saveBtn.setEnabled(supported && !empty);

        selection.setVisibility(catalogue ? VISIBLE : GONE);
        grid.setVisibility(catalogue ? VISIBLE : GONE);
        fieldsScroll.setVisibility(catalogue ? GONE : VISIBLE);

        if (catalogue) {
            Group chosen = currentGroup();
            Field cf = chosen != null ? field(chosen.key) : null;
            int sel = cf != null ? cf.val : 0;
            Item piece = (chosen != null && sel >= 1 && sel <= chosen.items.size()) ? chosen.items.get(sel - 1) : null;
            pieceName.setText(piece != null ? pretty(piece.name) : "Tenue actuelle");
            pieceNumber.setText(piece != null ? String.format(java.util.Locale.FRANCE, "PIÈCE · N° %03d", sel) : "PIÈCE · ORIGINAL");
            pieceCount.setText(chosen != null ? chosen.items.size() + " pièces · " + labelOfKey(chosen.key) : "");
            gridItems.clear();
            for (Group g : gs) for (int i = 0; i < g.items.size(); i++) gridItems.add(new Object[]{g, i});
            adapter.notifyDataSetChanged();
        } else {
            renderFields(c);
        }
    }

    private String labelOfKey(String key) {
        String l = LABELS.get(key);
        return l != null ? l : key;
    }

    private void styleChip(TextView t, boolean on) {
        Context c = getContext();
        t.setTextColor(on ? 0xFF0B0B0D : CwUi.TEXT);
        t.setBackground(on ? CwUi.button(c, CwUi.TEXT, 0xFFD6D8DC, 0) : CwUi.button(c, CwUi.BG_CARD, CwUi.BG_CARD_ON, CwUi.LINE));
    }

    private void renderFields(Context c) {
        fieldsBox.removeAllViews();
        for (final Field f : fields) {
            if (!active.equals(f.tab) || f.key.startsWith("south")) continue;
            LinearLayout row = new LinearLayout(c);
            row.setOrientation(LinearLayout.HORIZONTAL);
            row.setGravity(Gravity.CENTER_VERTICAL);
            row.setBackground(CwUi.box(c, 0x10FFFFFF, CwUi.LINE));
            row.setPadding(dp(12), dp(6), dp(6), dp(6));
            TextView label = CwUi.text(c, labelOf(f), 12f, CwUi.TEXT, CwUi.medium(c));
            row.addView(label, new LinearLayout.LayoutParams(0, -2, 1f));
            TextView minus = CwUi.ghostButton(c, "−");
            TextView plus = CwUi.ghostButton(c, "+");
            minus.setTextSize(16f);
            plus.setTextSize(16f);
            minus.setEnabled(f.val > f.min);
            plus.setEnabled(f.val < f.max);
            TextView value = CwUi.text(c, f.val + " / " + f.max, 11.5f, CwUi.ACCENT, CwUi.bold(c));
            value.setGravity(Gravity.CENTER);
            minus.setOnClickListener(v -> setOption(f.key, f.val - 1));
            plus.setOnClickListener(v -> setOption(f.key, f.val + 1));
            row.addView(minus, new LinearLayout.LayoutParams(dp(36), dp(34)));
            row.addView(value, new LinearLayout.LayoutParams(dp(62), -2));
            row.addView(plus, new LinearLayout.LayoutParams(dp(36), dp(34)));
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, -2);
            lp.bottomMargin = dp(6);
            fieldsBox.addView(row, lp);
        }
        if (fieldsBox.getChildCount() == 0) {
            TextView t = CwUi.text(c, "Aucun réglage pour cette catégorie.", 12f, CwUi.MUTED, CwUi.regular(c));
            t.setPadding(0, dp(10), 0, 0);
            fieldsBox.addView(t);
        }
    }

    // =====================================================================
    // Grille des pieces
    // =====================================================================
    private final class ItemAdapter extends BaseAdapter {
        @Override public int getCount() { return gridItems.size(); }
        @Override public Object getItem(int p) { return gridItems.get(p); }
        @Override public long getItemId(int p) { return p; }

        @Override
        public View getView(int pos, View convert, ViewGroup parent) {
            Context c = parent.getContext();
            LinearLayout cell;
            ImageView img;
            TextView num;
            if (convert instanceof LinearLayout) {
                cell = (LinearLayout) convert;
                img = (ImageView) cell.getChildAt(0);
                num = (TextView) cell.getChildAt(1);
            } else {
                cell = new LinearLayout(c);
                cell.setOrientation(LinearLayout.VERTICAL);
                cell.setPadding(dp(4), dp(4), dp(4), dp(5));
                img = new ImageView(c);
                img.setScaleType(ImageView.ScaleType.FIT_CENTER);
                cell.addView(img, new LinearLayout.LayoutParams(-1, dp(72)));
                num = CwUi.text(c, "", 9f, CwUi.MUTED, CwUi.bold(c));
                num.setGravity(Gravity.CENTER);
                LinearLayout.LayoutParams nl = new LinearLayout.LayoutParams(-1, -2);
                nl.topMargin = dp(3);
                cell.addView(num, nl);
            }
            Object[] e = gridItems.get(pos);
            Group g = (Group) e[0];
            int idx = (Integer) e[1];
            Item it = g.items.get(idx);
            Field f = field(g.key);
            boolean selected = f != null && f.val == idx + 1;
            cell.setBackground(CwUi.box(c, selected ? 0x2E3B82FF : 0x12FFFFFF, selected ? CwUi.ACCENT : CwUi.LINE));
            num.setText(String.format(java.util.Locale.FRANCE, "N° %03d", idx + 1));
            num.setTextColor(selected ? CwUi.ACCENT : CwUi.MUTED);
            try {
                Glide.with(c)
                        .load("file:///android_asset/clothes/south/" + it.texture)
                        .override(dp(96))
                        .diskCacheStrategy(DiskCacheStrategy.RESOURCE)
                        .dontAnimate()
                        .into(img);
            } catch (Throwable t) {
                img.setImageDrawable(null);
            }
            return cell;
        }
    }

    // =====================================================================
    // Catalogue (assets/clothes/south/catalogue.json)
    // =====================================================================
    private void loadCatalogueAsync() {
        if (sCatalogue != null) return;
        final Context app = getContext().getApplicationContext();
        new Thread(() -> {
            List<Group> out = new ArrayList<>();
            try (InputStream in = app.getAssets().open("clothes/south/catalogue.json")) {
                ByteArrayOutputStream buf = new ByteArrayOutputStream();
                byte[] b = new byte[16384];
                int n;
                while ((n = in.read(b)) > 0) buf.write(b, 0, n);
                JSONArray arr = new JSONArray(buf.toString("UTF-8"));
                for (int i = 0; i < arr.length(); i++) {
                    JSONObject o = arr.getJSONObject(i);
                    Group g = new Group();
                    g.model = o.optInt("model");
                    g.key = o.optString("key");
                    g.tab = o.optString("tab");
                    JSONArray items = o.optJSONArray("items");
                    if (items != null) {
                        for (int j = 0; j < items.length(); j++) {
                            JSONObject io = items.getJSONObject(j);
                            Item it = new Item();
                            it.name = io.optString("name");
                            it.texture = io.optString("texture");
                            g.items.add(it);
                        }
                    }
                    out.add(g);
                }
            } catch (Exception e) {
                Log.e(TAG, "catalogue illisible : " + e.getMessage());
            }
            ui.post(() -> {
                sCatalogue = out;
                if (isOpen()) render();
            });
        }, "cw-clothes-catalogue").start();
    }
}
