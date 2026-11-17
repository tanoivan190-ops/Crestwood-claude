package com.gta.launcher.ui;

import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.app.Activity;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.media.MediaPlayer;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.LinearInterpolator;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.gta.game.R;

/**
 * [v25] Ecran de demarrage Crestwood (remplace l'ancien generique de 24 s) :
 *   photo du joueur assis sur la Dodge (meme image que la page JOUER), zoom lent,
 *   logo CREST WOOD RP qui apparait, barre de chargement bleue + etapes, ~5 s, touchable pour passer.
 * Plus aucune mention de studio. API inchangee : showIfNeeded / pause / resume / release.
 */
public final class IntroOverlay {
    private static final String TAG = "IntroOverlay";

    private static final long TOTAL_MS = 5200L;          // duree du chargement affiche
    private static final long SKIP_ALLOWED_AT_MS = 1200L;
    private static final long FADE_OUT_MS = 550L;
    private static final float MUSIC_VOLUME = 0.7f;
    private static final int BLUE = Color.rgb(59, 130, 255);
    private static final String[] STEPS = {
            "Chargement de Los Santos",
            "Préparation des données du jeu",
            "Réglages de ton téléphone",
            "Connexion à Crestwood"
    };

    private static boolean shownThisProcess;

    private final Activity activity;
    private final Runnable onFinished;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private FrameLayout root;
    private ImageView photo, logo;
    private View barFill;
    private TextView step, percent, hint;
    private ValueAnimator progress;
    private MediaPlayer player;
    private boolean finished, fadingOut, skipAllowed;

    private IntroOverlay(Activity activity, Runnable onFinished) {
        this.activity = activity;
        this.onFinished = onFinished;
    }

    /** Retourne l'ecran lance (onFinished sera appele a la fin), ou null s'il ne s'affiche pas. */
    public static IntroOverlay showIfNeeded(Activity activity, Runnable onFinished) {
        if (shownThisProcess) return null;
        shownThisProcess = true;
        try {
            IntroOverlay intro = new IntroOverlay(activity, onFinished);
            intro.build();
            intro.start();
            return intro;
        } catch (Throwable error) {
            Log.e(TAG, "Ecran de demarrage impossible", error);
            return null;
        }
    }

    private int dp(float v) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, v, activity.getResources().getDisplayMetrics());
    }

    private TextView text(String value, float sp, int color, boolean bold, float spacing) {
        TextView view = new TextView(activity);
        view.setText(value);
        view.setTextColor(color);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        view.setTypeface(bold ? com.gta.game.ui.CwUi.black(activity) : com.gta.game.ui.CwUi.medium(activity));
        view.setLetterSpacing(spacing);
        view.setShadowLayer(dp(4), 0f, dp(1), Color.argb(200, 0, 0, 0));
        view.setIncludeFontPadding(false);
        return view;
    }

    private void build() {
        root = new FrameLayout(activity);
        root.setBackgroundColor(Color.BLACK);
        root.setClickable(true);
        root.setFocusable(true);
        root.setOnClickListener(v -> { if (skipAllowed) fadeOutAndFinish(); });

        // photo plein ecran (le joueur sur la Dodge)
        photo = new ImageView(activity);
        photo.setScaleType(ImageView.ScaleType.CENTER_CROP);
        photo.setImageResource(R.drawable.intro_slide_1);
        photo.setAlpha(0f);
        root.addView(photo, new FrameLayout.LayoutParams(-1, -1));

        // assombrissement : gauche (lecture du logo) + bas (barre)
        View left = new View(activity);
        left.setBackground(new GradientDrawable(GradientDrawable.Orientation.LEFT_RIGHT,
                new int[] { Color.argb(235, 4, 6, 10), Color.argb(150, 4, 6, 10), Color.argb(30, 4, 6, 10) }));
        root.addView(left, new FrameLayout.LayoutParams(-1, -1));
        View bottom = new View(activity);
        bottom.setBackground(new GradientDrawable(GradientDrawable.Orientation.BOTTOM_TOP,
                new int[] { Color.argb(240, 0, 0, 0), Color.argb(0, 0, 0, 0) }));
        root.addView(bottom, new FrameLayout.LayoutParams(-1, dp(150), Gravity.BOTTOM));

        // bloc gauche : logo + accroche
        LinearLayout block = new LinearLayout(activity);
        block.setOrientation(LinearLayout.VERTICAL);
        block.setGravity(Gravity.START);
        FrameLayout.LayoutParams bl = new FrameLayout.LayoutParams(-2, -2, Gravity.START | Gravity.CENTER_VERTICAL);
        bl.leftMargin = dp(44);
        bl.bottomMargin = dp(36);
        root.addView(block, bl);

        logo = new ImageView(activity);
        logo.setImageResource(R.drawable.cw_boot_logo);
        logo.setAdjustViewBounds(true);
        logo.setScaleType(ImageView.ScaleType.FIT_START);
        logo.setAlpha(0f);
        block.addView(logo, new LinearLayout.LayoutParams(dp(290), -2));

        TextView tag = text("Ta ville. Ton histoire.", 13f, Color.argb(225, 255, 255, 255), false, 0.04f);
        LinearLayout.LayoutParams tl = new LinearLayout.LayoutParams(-2, -2);
        tl.topMargin = dp(12);
        tl.leftMargin = dp(4);
        tag.setAlpha(0f);
        block.addView(tag, tl);
        tag.animate().alpha(1f).setStartDelay(900).setDuration(700).start();

        // bas : etape + pourcentage + barre bleue
        LinearLayout foot = new LinearLayout(activity);
        foot.setOrientation(LinearLayout.VERTICAL);
        FrameLayout.LayoutParams fl = new FrameLayout.LayoutParams(-1, -2, Gravity.BOTTOM);
        fl.leftMargin = dp(44);
        fl.rightMargin = dp(44);
        fl.bottomMargin = dp(26);
        root.addView(foot, fl);

        LinearLayout row = new LinearLayout(activity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.BOTTOM);
        foot.addView(row, new LinearLayout.LayoutParams(-1, -2));
        step = text(STEPS[0], 11.5f, Color.argb(235, 255, 255, 255), false, 0.02f);
        row.addView(step, new LinearLayout.LayoutParams(0, -2, 1f));
        percent = text("0 %", 15f, Color.WHITE, true, 0.02f);
        row.addView(percent, new LinearLayout.LayoutParams(-2, -2));

        FrameLayout track = new FrameLayout(activity);
        GradientDrawable trackBg = new GradientDrawable();
        trackBg.setColor(Color.argb(60, 255, 255, 255));
        trackBg.setCornerRadius(dp(3));
        track.setBackground(trackBg);
        LinearLayout.LayoutParams trl = new LinearLayout.LayoutParams(-1, dp(4));
        trl.topMargin = dp(9);
        foot.addView(track, trl);
        barFill = new View(activity);
        GradientDrawable fillBg = new GradientDrawable(GradientDrawable.Orientation.LEFT_RIGHT, new int[] { Color.rgb(36, 99, 235), BLUE, Color.rgb(140, 185, 255) });
        fillBg.setCornerRadius(dp(3));
        barFill.setBackground(fillBg);
        barFill.setPivotX(0f);
        barFill.setScaleX(0f);
        track.addView(barFill, new FrameLayout.LayoutParams(-1, -1));

        hint = text("Touchez pour passer", 9f, Color.argb(140, 255, 255, 255), false, 0.06f);
        hint.setAlpha(0f);
        FrameLayout.LayoutParams hl = new FrameLayout.LayoutParams(-2, -2, Gravity.TOP | Gravity.END);
        hl.topMargin = dp(18);
        hl.rightMargin = dp(24);
        root.addView(hint, hl);

        activity.addContentView(root, new ViewGroup.LayoutParams(-1, -1));
    }

    private void start() {
        // photo : fondu + zoom lent
        photo.setScaleX(1.0f);
        photo.setScaleY(1.0f);
        photo.animate().alpha(1f).setDuration(700).start();
        photo.animate().scaleX(1.08f).scaleY(1.08f).setDuration(TOTAL_MS + 800).setInterpolator(new LinearInterpolator()).start();
        // logo : glisse de la gauche
        logo.setTranslationX(-dp(24));
        logo.animate().alpha(1f).translationX(0f).setStartDelay(300).setDuration(800).setInterpolator(new DecelerateInterpolator()).start();

        progress = ValueAnimator.ofFloat(0f, 1f);
        progress.setDuration(TOTAL_MS);
        progress.setInterpolator(new DecelerateInterpolator(1.3f));
        progress.addUpdateListener(a -> {
            float p = (float) a.getAnimatedValue();
            barFill.setScaleX(p);
            percent.setText(Math.round(p * 100f) + " %");
            int i = Math.min(STEPS.length - 1, (int) (p * STEPS.length));
            if (!STEPS[i].contentEquals(step.getText())) step.setText(STEPS[i]);
        });
        progress.start();

        handler.postDelayed(() -> { skipAllowed = true; hint.animate().alpha(1f).setDuration(400).start(); }, SKIP_ALLOWED_AT_MS);
        handler.postDelayed(this::fadeOutAndFinish, TOTAL_MS + 150);

        try {
            player = MediaPlayer.create(activity, R.raw.crestwood_intro);
            if (player != null) { player.setVolume(MUSIC_VOLUME, MUSIC_VOLUME); player.start(); }
        } catch (Throwable error) {
            Log.w(TAG, "Musique indisponible", error);
            player = null;
        }
    }

    private void fadeOutAndFinish() {
        if (finished || fadingOut) return;
        fadingOut = true;
        ObjectAnimator fade = ObjectAnimator.ofFloat(root, "alpha", 1f, 0f);
        fade.setDuration(FADE_OUT_MS);
        fade.start();
        final MediaPlayer current = player;
        final long t0 = System.currentTimeMillis();
        handler.post(new Runnable() {
            @Override public void run() {
                if (finished || current == null || player != current) return;
                float k = Math.min(1f, (System.currentTimeMillis() - t0) / (float) FADE_OUT_MS);
                try { current.setVolume(MUSIC_VOLUME * (1f - k), MUSIC_VOLUME * (1f - k)); } catch (Throwable ignored) { return; }
                if (k < 1f) handler.postDelayed(this, 40L);
            }
        });
        handler.postDelayed(this::finish, FADE_OUT_MS + 30);
    }

    private void finish() {
        if (finished) return;
        finished = true;
        handler.removeCallbacksAndMessages(null);
        if (progress != null) progress.cancel();
        releasePlayer();
        try {
            ViewGroup parent = root == null ? null : (ViewGroup) root.getParent();
            if (parent != null) parent.removeView(root);
        } catch (Throwable ignored) { }
        if (onFinished != null) onFinished.run();
    }

    private void releasePlayer() {
        MediaPlayer current = player;
        player = null;
        if (current == null) return;
        try { current.stop(); } catch (Throwable ignored) { }
        try { current.release(); } catch (Throwable ignored) { }
    }

    public void pause() {
        try { if (player != null && player.isPlaying()) player.pause(); } catch (Throwable ignored) { }
        if (progress != null && progress.isRunning()) progress.pause();
    }

    public void resume() {
        try { if (player != null && !finished && !player.isPlaying()) player.start(); } catch (Throwable ignored) { }
        if (progress != null && progress.isPaused()) progress.resume();
    }

    public void release() {
        if (finished) return;
        finished = true;
        handler.removeCallbacksAndMessages(null);
        if (progress != null) progress.cancel();
        releasePlayer();
    }
}
