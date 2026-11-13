package com.gta.game.ui;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.drawable.GradientDrawable;
import android.text.InputFilter;
import android.text.InputType;
import android.util.Base64;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.util.Locale;

/**
 * Etat civil de la REPUBLIQUE DE CRESTWOOD : formulaire de carte d'identite + signature au doigt.
 * Natif Android (pas de WebView). Ouvert par le dialogue serveur 9880 "@idform".
 *   1) le formulaire est pre-rempli (nom, prenom, sexe) ; le joueur regle l'age et le lieu de naissance ;
 *   2) il signe dans le cadre (le papier officiel) ;
 *   3) VALIDER -> "/idsign <age> <lieu>" ; la signature (PNG) est gardee sur le telephone et affichee
 *      sur sa propre carte (SampUi.signature(key) dans assets/ui).
 */
public class NativeIdPanel extends FrameLayout {

    public interface Callback {
        void onSubmit(int age, String origin);
        void onClose();
    }

    public static final String PREFS = "cw_signatures";

    private final Callback cb;
    private boolean open;
    private String key = "";
    private int age = 25;
    private TextView title, sub, nameVal, firstVal, sexVal, ageVal, feeText, validate, step;
    private EditText origin;
    private SignatureView sig;

    public NativeIdPanel(Context c, Callback cb) {
        super(c);
        this.cb = cb;
        setClickable(true);
        setFocusable(true);
        setFocusableInTouchMode(true);
        setVisibility(GONE);
        setBackground(new GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM, new int[]{0xCC050608, 0xE6050608}));
        build();
    }

    public boolean isOpen() { return open; }

    private int dp(float v) { return CwUi.dp(getContext(), v); }

    /** Signature PNG (data URL) enregistree pour ce compte, ou "" */
    public static String signatureFor(Context c, String key) {
        if (c == null || key == null || key.isEmpty()) return "";
        try {
            return c.getSharedPreferences(PREFS, Context.MODE_PRIVATE).getString(key.toLowerCase(Locale.ROOT), "");
        } catch (Throwable t) {
            return "";
        }
    }

    private LinearLayout.LayoutParams lpm(int w, int h, int l, int t, int r, int b) {
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(w, h);
        p.setMargins(dp(l), dp(t), dp(r), dp(b));
        return p;
    }

    private void build() {
        Context c = getContext();
        // feuille officielle (papier clair) au centre
        LinearLayout sheet = new LinearLayout(c);
        sheet.setOrientation(LinearLayout.HORIZONTAL);
        GradientDrawable paper = new GradientDrawable(GradientDrawable.Orientation.TL_BR, new int[]{0xFFF7F6F1, 0xFFE9E7DE});
        paper.setCornerRadius(dp(3));
        sheet.setBackground(paper);
        sheet.setPadding(dp(18), dp(14), dp(18), dp(14));
        sheet.setClickable(true);
        int w = Math.min(dp(760), Math.round(c.getResources().getDisplayMetrics().widthPixels * 0.86f));
        LayoutParams slp = new LayoutParams(w, LayoutParams.WRAP_CONTENT, Gravity.CENTER);
        addView(sheet, slp);

        // ---- colonne gauche : en-tete + formulaire ----
        LinearLayout left = new LinearLayout(c);
        left.setOrientation(LinearLayout.VERTICAL);
        sheet.addView(left, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        LinearLayout head = new LinearLayout(c);
        head.setOrientation(LinearLayout.HORIZONTAL);
        head.setGravity(Gravity.CENTER_VERTICAL);
        head.addView(new Seal(c), new LinearLayout.LayoutParams(dp(44), dp(44)));
        LinearLayout ht = new LinearLayout(c);
        ht.setOrientation(LinearLayout.VERTICAL);
        TextView rep = CwUi.text(c, "RÉPUBLIQUE DE CRESTWOOD", 13f, 0xFF0F2A5F, CwUi.black(c));
        rep.setLetterSpacing(0.12f);
        ht.addView(rep);
        TextView motto = CwUi.text(c, "Unité · Travail · Prospérité", 9f, 0xFF5A6270, CwUi.medium(c));
        ht.addView(motto, lpm(-2, -2, 0, 3, 0, 0));
        LinearLayout.LayoutParams hlp = new LinearLayout.LayoutParams(0, -2, 1f);
        hlp.leftMargin = dp(10);
        head.addView(ht, hlp);
        TextView close = CwUi.text(c, "✕", 16f, 0xFF5A6270, CwUi.bold(c));
        close.setGravity(Gravity.CENTER);
        close.setClickable(true);
        close.setOnClickListener(v -> { hideKeyboard(); if (cb != null) cb.onClose(); });
        head.addView(close, new LinearLayout.LayoutParams(dp(36), dp(36)));
        left.addView(head);

        // bandeau tricolore
        View band = new View(c);
        band.setBackground(new GradientDrawable(GradientDrawable.Orientation.LEFT_RIGHT, new int[]{0xFF0F2A5F, 0xFF3B82FF, 0xFFE9E7DE, 0xFF0F2A5F}));
        left.addView(band, lpm(-1, dp(3), 0, 10, 0, 10));

        title = CwUi.text(c, "DEMANDE DE CARTE NATIONALE D'IDENTITÉ", 12f, 0xFF111418, CwUi.black(c));
        title.setLetterSpacing(0.04f);
        left.addView(title);
        sub = CwUi.text(c, "Formulaire CERFA-CW 01 · Service de l'état civil", 9.5f, 0xFF6B7280, CwUi.medium(c));
        left.addView(sub, lpm(-2, -2, 0, 3, 0, 8));

        nameVal = field(c, left, "NOM");
        firstVal = field(c, left, "PRÉNOM(S)");
        sexVal = field(c, left, "SEXE");

        // age : stepper
        LinearLayout ageRow = new LinearLayout(c);
        ageRow.setOrientation(LinearLayout.HORIZONTAL);
        ageRow.setGravity(Gravity.CENTER_VERTICAL);
        ageRow.addView(label(c, "ÂGE"), new LinearLayout.LayoutParams(dp(120), -2));
        TextView minus = stepBtn(c, "−");
        TextView plus = stepBtn(c, "+");
        ageVal = CwUi.text(c, "25 ans", 13f, 0xFF111418, CwUi.black(c));
        ageVal.setGravity(Gravity.CENTER);
        minus.setOnClickListener(v -> setAge(age - 1));
        plus.setOnClickListener(v -> setAge(age + 1));
        ageRow.addView(minus, new LinearLayout.LayoutParams(dp(34), dp(30)));
        ageRow.addView(ageVal, new LinearLayout.LayoutParams(dp(76), -2));
        ageRow.addView(plus, new LinearLayout.LayoutParams(dp(34), dp(30)));
        left.addView(ageRow, lpm(-1, -2, 0, 6, 0, 0));

        // lieu de naissance
        LinearLayout oriRow = new LinearLayout(c);
        oriRow.setOrientation(LinearLayout.HORIZONTAL);
        oriRow.setGravity(Gravity.CENTER_VERTICAL);
        oriRow.addView(label(c, "NÉ(E) À"), new LinearLayout.LayoutParams(dp(120), -2));
        origin = new EditText(c);
        origin.setSingleLine(true);
        origin.setTextColor(0xFF111418);
        origin.setHintTextColor(0x88111418);
        origin.setHint("Ville de naissance");
        origin.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13f);
        origin.setTypeface(CwUi.bold(c));
        origin.setPadding(dp(8), 0, dp(8), 0);
        GradientDrawable line = new GradientDrawable();
        line.setColor(0x14000000);
        line.setCornerRadius(dp(2));
        line.setStroke(Math.max(1, dp(1)), 0x330F2A5F);
        origin.setBackground(line);
        origin.setFilters(new InputFilter[]{new InputFilter.LengthFilter(24)});
        origin.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_CAP_WORDS);
        origin.setImeOptions(EditorInfo.IME_ACTION_DONE | EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_FLAG_NO_FULLSCREEN);
        origin.addTextChangedListener(new android.text.TextWatcher() {
            @Override public void beforeTextChanged(CharSequence q, int a, int b, int d) {}
            @Override public void onTextChanged(CharSequence q, int a, int b, int d) {}
            @Override public void afterTextChanged(android.text.Editable q) { if (validate != null) refresh(); }
        });
        oriRow.addView(origin, new LinearLayout.LayoutParams(0, dp(34), 1f));
        left.addView(oriRow, lpm(-1, -2, 0, 6, 0, 0));

        // villes rapides
        LinearLayout cities = new LinearLayout(c);
        cities.setOrientation(LinearLayout.HORIZONTAL);
        for (String city : new String[]{"Los Santos", "San Fierro", "Las Venturas", "Abidjan", "Dakar"}) {
            TextView t = CwUi.text(c, city, 9.5f, 0xFF0F2A5F, CwUi.bold(c));
            t.setPadding(dp(8), dp(5), dp(8), dp(5));
            GradientDrawable g = new GradientDrawable();
            g.setColor(0x1A3B82FF);
            g.setCornerRadius(dp(2));
            t.setBackground(g);
            t.setClickable(true);
            t.setOnClickListener(v -> origin.setText(city));
            LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(-2, -2);
            p.rightMargin = dp(5);
            cities.addView(t, p);
        }
        left.addView(cities, lpm(-1, -2, 120, 6, 0, 0));

        feeText = CwUi.text(c, "", 9.5f, 0xFF6B7280, CwUi.medium(c));
        left.addView(feeText, lpm(-1, -2, 0, 10, 0, 0));

        // ---- colonne droite : signature ----
        LinearLayout right = new LinearLayout(c);
        right.setOrientation(LinearLayout.VERTICAL);
        LinearLayout.LayoutParams rlp = new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 0.9f);
        rlp.leftMargin = dp(18);
        sheet.addView(right, rlp);

        step = CwUi.text(c, "SIGNATURE DU DEMANDEUR", 10f, 0xFF0F2A5F, CwUi.black(c));
        step.setLetterSpacing(0.1f);
        right.addView(step);
        TextView hint = CwUi.text(c, "Signe avec ton doigt dans le cadre ci-dessous.", 9.5f, 0xFF6B7280, CwUi.medium(c));
        right.addView(hint, lpm(-2, -2, 0, 3, 0, 6));

        FrameLayout pad = new FrameLayout(c);
        GradientDrawable pb = new GradientDrawable();
        pb.setColor(0xFFFFFFFF);
        pb.setCornerRadius(dp(2));
        pb.setStroke(Math.max(1, dp(1)), 0x660F2A5F);
        pad.setBackground(pb);
        sig = new SignatureView(c);
        pad.addView(sig, new LayoutParams(-1, -1));
        TextView x = CwUi.text(c, "×", 16f, 0x660F2A5F, CwUi.bold(c));
        LayoutParams xlp = new LayoutParams(-2, -2, Gravity.BOTTOM | Gravity.START);
        xlp.setMargins(dp(8), 0, 0, dp(18));
        pad.addView(x, xlp);
        View base = new View(c);
        base.setBackgroundColor(0x330F2A5F);
        LayoutParams blp = new LayoutParams(-1, Math.max(1, dp(1)), Gravity.BOTTOM);
        blp.setMargins(dp(22), 0, dp(12), dp(22));
        pad.addView(base, blp);
        right.addView(pad, new LinearLayout.LayoutParams(-1, dp(150)));

        LinearLayout actions = new LinearLayout(c);
        actions.setOrientation(LinearLayout.HORIZONTAL);
        TextView clear = CwUi.text(c, "EFFACER", 11f, 0xFF0F2A5F, CwUi.bold(c));
        clear.setGravity(Gravity.CENTER);
        GradientDrawable cg = new GradientDrawable();
        cg.setColor(0x00000000);
        cg.setCornerRadius(dp(3));
        cg.setStroke(Math.max(1, dp(1)), 0x660F2A5F);
        clear.setBackground(cg);
        clear.setClickable(true);
        clear.setOnClickListener(v -> { sig.clear(); refresh(); });
        actions.addView(clear, new LinearLayout.LayoutParams(0, dp(42), 0.8f));
        validate = CwUi.primaryButton(c, "VALIDER ET SIGNER");
        validate.setOnClickListener(v -> submit());
        LinearLayout.LayoutParams vlp = new LinearLayout.LayoutParams(0, dp(42), 1.6f);
        vlp.leftMargin = dp(8);
        actions.addView(validate, vlp);
        right.addView(actions, lpm(-1, -2, 0, 10, 0, 0));

        TextView legal = CwUi.text(c, "Je certifie sur l'honneur l'exactitude des renseignements ci-dessus. Toute fausse déclaration est passible de poursuites (Code civil de Crestwood, art. 12).", 8.5f, 0xFF8A8F98, CwUi.regular(c));
        legal.setLineSpacing(0, 1.15f);
        right.addView(legal, lpm(-1, -2, 0, 8, 0, 0));
    }

    private TextView label(Context c, String s) {
        TextView t = CwUi.text(c, s, 9.5f, 0xFF6B7280, CwUi.bold(c));
        t.setLetterSpacing(0.1f);
        return t;
    }

    private TextView field(Context c, LinearLayout parent, String name) {
        LinearLayout row = new LinearLayout(c);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.addView(label(c, name), new LinearLayout.LayoutParams(dp(120), -2));
        TextView v = CwUi.text(c, "", 13f, 0xFF111418, CwUi.black(c));
        row.addView(v, new LinearLayout.LayoutParams(0, -2, 1f));
        parent.addView(row, lpm(-1, -2, 0, 6, 0, 0));
        return v;
    }

    private TextView stepBtn(Context c, String s) {
        TextView t = CwUi.text(c, s, 16f, 0xFF0F2A5F, CwUi.black(c));
        t.setGravity(Gravity.CENTER);
        GradientDrawable g = new GradientDrawable();
        g.setColor(0x1A3B82FF);
        g.setCornerRadius(dp(2));
        t.setBackground(g);
        t.setClickable(true);
        return t;
    }

    private void setAge(int a) {
        age = Math.max(16, Math.min(90, a));
        ageVal.setText(age + " ans");
    }

    private void refresh() {
        boolean ok = sig.hasInk() && origin.getText().toString().trim().length() >= 2;
        validate.setEnabled(ok);
        validate.setAlpha(ok ? 1f : 0.45f);
    }

    public void open(String json) {
        try {
            JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
            nameVal.setText(o.optString("nom", "—").toUpperCase(Locale.FRANCE));
            firstVal.setText(o.optString("prenom", "—"));
            sexVal.setText("F".equals(o.optString("sexe", "M")) ? "Féminin (F)" : "Masculin (M)");
            setAge(o.optInt("age", 25));
            origin.setText(o.optString("origine", "Los Santos"));
            key = o.optString("key", "");
            boolean renew = o.optInt("renew", 0) == 1;
            int fee = o.optInt("fee", 0);
            title.setText(renew ? "RENOUVELLEMENT DE LA CARTE NATIONALE D'IDENTITÉ" : "DEMANDE DE CARTE NATIONALE D'IDENTITÉ");
            feeText.setText(fee > 0 ? "Timbre fiscal : " + fee + " $ (liquide ou banque) · validité 10 ans" : "Première carte : gratuite · validité 10 ans");
        } catch (Exception ignored) { }
        sig.clear();
        open = true;
        setVisibility(VISIBLE);
        setAlpha(0f);
        animate().alpha(1f).setDuration(180).start();
        refresh();
    }

    public void close() {
        open = false;
        hideKeyboard();
        animate().alpha(0f).setDuration(150).withEndAction(() -> setVisibility(GONE)).start();
    }

    private void submit() {
        // accents retires (les commandes SA-MP passent en latin-1) : "Séguéla" -> "Seguela"
        String ori = java.text.Normalizer.normalize(origin.getText().toString().trim(), java.text.Normalizer.Form.NFD)
                .replaceAll("\\p{M}", "").replaceAll("[^A-Za-z \\-]", "").trim();
        if (!sig.hasInk() || ori.length() < 2 || cb == null) return;
        // signature gardee sur le telephone (affichee sur sa propre carte)
        try {
            String png = sig.toDataUrl();
            if (!png.isEmpty() && !key.isEmpty())
                getContext().getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().putString(key.toLowerCase(Locale.ROOT), png).apply();
        } catch (Throwable ignored) { }
        hideKeyboard();
        cb.onSubmit(age, ori.replace(' ', '_'));
    }

    private void hideKeyboard() {
        try {
            InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            if (imm != null) imm.hideSoftInputFromWindow(getWindowToken(), 0);
        } catch (Throwable ignored) { }
    }

    // ---- sceau de la Republique (bouclier bleu, "CR") ----
    private static final class Seal extends View {
        private final Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Path path = new Path();
        Seal(Context c) { super(c); }

        @Override protected void onDraw(Canvas c) {
            float w = getWidth(), h = getHeight();
            path.reset();
            path.moveTo(w * 0.5f, h * 0.04f);
            path.lineTo(w * 0.92f, h * 0.18f);
            path.lineTo(w * 0.86f, h * 0.62f);
            path.lineTo(w * 0.5f, h * 0.96f);
            path.lineTo(w * 0.14f, h * 0.62f);
            path.lineTo(w * 0.08f, h * 0.18f);
            path.close();
            p.setStyle(Paint.Style.FILL);
            p.setColor(0xFF0F2A5F);
            c.drawPath(path, p);
            p.setStyle(Paint.Style.STROKE);
            p.setStrokeWidth(w * 0.05f);
            p.setColor(0xFF3B82FF);
            c.drawPath(path, p);
            p.setStyle(Paint.Style.FILL);
            p.setColor(0xFFFFFFFF);
            p.setTextAlign(Paint.Align.CENTER);
            p.setTextSize(h * 0.34f);
            p.setFakeBoldText(true);
            c.drawText("CR", w * 0.5f, h * 0.6f, p);
        }
    }

    // ---- zone de signature au doigt ----
    private final class SignatureView extends View {
        private final Paint ink = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Path path = new Path();
        private float lx, ly;
        private int strokes;

        SignatureView(Context c) {
            super(c);
            ink.setColor(0xFF0B1F4D);
            ink.setStyle(Paint.Style.STROKE);
            ink.setStrokeJoin(Paint.Join.ROUND);
            ink.setStrokeCap(Paint.Cap.ROUND);
            ink.setStrokeWidth(CwUi.dp(c, 2.6f));
        }

        boolean hasInk() { return strokes > 0; }

        void clear() { path.reset(); strokes = 0; invalidate(); }

        @Override protected void onDraw(Canvas c) { c.drawPath(path, ink); }

        @Override public boolean onTouchEvent(MotionEvent e) {
            float x = e.getX(), y = e.getY();
            switch (e.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    getParent().requestDisallowInterceptTouchEvent(true);
                    path.moveTo(x, y); lx = x; ly = y;
                    return true;
                case MotionEvent.ACTION_MOVE:
                    for (int i = 0; i < e.getHistorySize(); i++) {
                        float hx = e.getHistoricalX(i), hy = e.getHistoricalY(i);
                        path.quadTo(lx, ly, (hx + lx) / 2f, (hy + ly) / 2f);
                        lx = hx; ly = hy;
                    }
                    path.quadTo(lx, ly, (x + lx) / 2f, (y + ly) / 2f);
                    lx = x; ly = y;
                    invalidate();
                    return true;
                case MotionEvent.ACTION_UP:
                    path.lineTo(x, y);
                    strokes++;
                    invalidate();
                    refresh();
                    return true;
                default:
                    return true;
            }
        }

        String toDataUrl() {
            int w = getWidth(), h = getHeight();
            if (w <= 0 || h <= 0) return "";
            float s = Math.min(1f, 360f / w);
            Bitmap bmp = Bitmap.createBitmap(Math.max(1, Math.round(w * s)), Math.max(1, Math.round(h * s)), Bitmap.Config.ARGB_8888);
            Canvas cv = new Canvas(bmp);
            cv.scale(s, s);
            cv.drawPath(path, ink);
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            bmp.compress(Bitmap.CompressFormat.PNG, 100, out);
            bmp.recycle();
            return "data:image/png;base64," + Base64.encodeToString(out.toByteArray(), Base64.NO_WRAP);
        }
    }
}
