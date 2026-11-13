package com.gta.game.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Shader;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;

import java.io.InputStream;

/**
 * [v21] Logo CRESTWOOD en haut de l'ecran, en jeu : la VRAIE image (assets/login/assets/images/cw-logo.png),
 * plus le texte "C R E S T W O O D" dessine a la main (il etait coupe sur certains ecrans).
 * Un reflet bleu passe sur le logo toutes les 12 s. Ne capte jamais le tactile, natif (pas de WebView).
 */
public class CrestwoodTopBanner extends LinearLayout {

    private static final String LOGO_ASSET = "login/assets/images/cw-logo.png";
    private static Bitmap sLogo;              // decode une seule fois (900 x 300, reduit a la taille d'affichage)

    private final Handler handler = new Handler(Looper.getMainLooper());
    private final ImageView logo;
    private final Paint shinePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private float shine = -1f;                // position du reflet (0..1), -1 = aucun
    private boolean running;

    private final Runnable sweep = new Runnable() {
        @Override public void run() {
            if (!running) return;
            final long start = System.currentTimeMillis();
            handler.post(new Runnable() {
                @Override public void run() {
                    if (!running) { shine = -1f; return; }
                    float p = (System.currentTimeMillis() - start) / 1200f;
                    shine = p > 1f ? -1f : p;
                    logo.invalidate();
                    if (p <= 1f) handler.postDelayed(this, 33L);
                }
            });
            handler.postDelayed(this, 12_000L);
        }
    };

    public CrestwoodTopBanner(Context c) {
        super(c);
        setOrientation(VERTICAL);
        setGravity(Gravity.CENTER_HORIZONTAL);
        setClickable(false);
        setFocusable(false);
        setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_NO);

        logo = new ImageView(c) {
            @Override protected void onDraw(Canvas canvas) {
                super.onDraw(canvas);
                if (shine < 0f) return;
                float w = getWidth();
                float x = -w * 0.4f + shine * w * 1.8f;
                shinePaint.setShader(new LinearGradient(x, 0, x + w * 0.3f, 0,
                        new int[]{0x003B82FF, 0x99DCE8FF, 0x003B82FF}, null, Shader.TileMode.CLAMP));
                shinePaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_ATOP));
                canvas.drawRect(0, 0, w, getHeight(), shinePaint);
            }
        };
        logo.setScaleType(ImageView.ScaleType.FIT_CENTER);
        logo.setAdjustViewBounds(true);
        logo.setLayerType(View.LAYER_TYPE_HARDWARE, null);   // SRC_ATOP : le reflet reste sur le logo
        Bitmap b = loadLogo(c);
        if (b != null) logo.setImageBitmap(b);
        // largeur fixe : 3 x la hauteur (proportions du logo), jamais coupe
        int h = CwUi.dp(c, 30);
        addView(logo, new LayoutParams(h * 3, h));
        setAlpha(0.9f);
    }

    private static synchronized Bitmap loadLogo(Context c) {
        if (sLogo != null) return sLogo;
        try (InputStream in = c.getAssets().open(LOGO_ASSET)) {
            BitmapFactory.Options o = new BitmapFactory.Options();
            o.inSampleSize = 2;                       // 450 x 150 : net et leger en memoire
            sLogo = BitmapFactory.decodeStream(in, null, o);
        } catch (Throwable e) {
            Log.w("CwTopBanner", "logo introuvable : " + e.getMessage());
        }
        return sLogo;
    }

    @Override protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        running = true;
        handler.removeCallbacksAndMessages(null);
        handler.postDelayed(sweep, 3_000L);
        setTranslationY(-CwUi.dp(getContext(), 14));
        animate().translationY(0).setDuration(600).start();
    }

    @Override protected void onDetachedFromWindow() {
        running = false;
        handler.removeCallbacksAndMessages(null);
        super.onDetachedFromWindow();
    }

    /** Parametres de placement : en haut, centre (FrameLayout ou ConstraintLayout). */
    public static android.view.ViewGroup.LayoutParams topCenterParams(android.view.ViewGroup parent, Context c) {
        int top = CwUi.dp(c, 6);
        if (parent instanceof androidx.constraintlayout.widget.ConstraintLayout) {
            androidx.constraintlayout.widget.ConstraintLayout.LayoutParams lp =
                    new androidx.constraintlayout.widget.ConstraintLayout.LayoutParams(-2, -2);
            int id = androidx.constraintlayout.widget.ConstraintLayout.LayoutParams.PARENT_ID;
            lp.topToTop = id; lp.startToStart = id; lp.endToEnd = id;
            lp.topMargin = top;
            return lp;
        }
        if (parent instanceof android.widget.FrameLayout) {
            android.widget.FrameLayout.LayoutParams lp = new android.widget.FrameLayout.LayoutParams(-2, -2, Gravity.TOP | Gravity.CENTER_HORIZONTAL);
            lp.topMargin = top;
            return lp;
        }
        android.view.ViewGroup.MarginLayoutParams lp = new android.view.ViewGroup.MarginLayoutParams(-2, -2);
        lp.topMargin = top;
        return lp;
    }
}
