package com.gta.launcher.ui;

import android.app.Activity;
import android.animation.ObjectAnimator;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.media.MediaPlayer;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.LinearInterpolator;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.gta.game.R;

/**
 * Generique de lancement Crestwood (au moins 20 s) :
 *   1) diaporama cinema des photos Crestwood (Dodge SRT / moto) en fondu enchaine + zoom lent (Ken Burns),
 *      avec bandes noires, legendes et barre de progression fine ;
 *   2) final sur l'affiche CREST WOOD RP + nom du studio + "présente" ;
 *   3) fondu au noir (image + musique), puis ecran de chargement / page JOUER.
 * L'appui pour passer n'est actif qu'apres SKIP_ALLOWED_AT_MS.
 */
public final class IntroOverlay {
    private static final String TAG = "IntroOverlay";

    // ---- Reglages faciles a modifier ----
    /** Photos du diaporama (dans l'ordre). Elles tournent en boucle jusqu'au final. */
    private static final int[] SLIDES = {
            R.drawable.intro_slide_1, R.drawable.intro_slide_2, R.drawable.intro_marina, R.drawable.intro_police,
            R.drawable.intro_slide_1, R.drawable.intro_gang, R.drawable.intro_slide_2, R.drawable.intro_vie };
    /** Legendes affichees sous chaque passage (meme index que le passage). */
    private static final String[][] CAPTIONS = {
            { "LOS SANTOS", "Bienvenue à Crestwood" },
            { "LIBERTÉ", "Roule. Construis. Règne." },
            { "ICI, CHOISIS TON CAMP", "Deviens la loi, ou crée ta loi." },
            { "DEVIENS LA LOI", "Policier, militaire et bien d'autres métiers t'attendent." },
            { "LA RUE", "Ton histoire commence ici" },
            { "CHEF DE GANG", "Crée ta loi, montre ta détermination, domine tous les territoires." },
            { "CRESTWOOD RP", "Le roleplay comme jamais" },
            { "CRESTWOOD RP", "Bien plus que le RP" },
    };
    private static final int SLIDE_PASSES = SLIDES.length; // un passage par photo
    private static final long SLIDE_MS = 4600L;           // duree d'un passage
    private static final long CROSSFADE_MS = 1300L;      // fondu enchaine entre deux photos
    private static final long FINALE_MS = 5600L;          // duree de l'affiche finale (logo)
    private static final long FADE_OUT_MS = 1400L;        // fondu final (image + musique)
    private static final long TOTAL_MS = SLIDE_PASSES * SLIDE_MS + FINALE_MS; // ~40 s
    private static final long SKIP_ALLOWED_AT_MS = 12000L; // le joueur peut passer apres 12 s
    private static final int MUSIC_START_MS = 0;
    private static final float MUSIC_VOLUME = 0.9f;

    private static boolean shownThisProcess;

    private final Activity activity;
    private final Runnable onFinished;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private FrameLayout root;
    private ImageView front, back;
    private LinearLayout captionBox;
    private TextView captionKicker, captionLine;
    private LinearLayout studioBox;
    private TextView presents;
    private TextView skipHint;
    private TextView copyright;
    private View progressFill;
    private ObjectAnimator fadeAnimator;
    private MediaPlayer player;
    private boolean finished;
    private boolean fadingOut;
    private boolean skipAllowed;
    private long startedAt;

    private IntroOverlay(Activity activity, Runnable onFinished) {
        this.activity = activity;
        this.onFinished = onFinished;
    }

    /** Retourne l'intro lancee (onFinished sera appele a la fin), ou null si elle ne s'affiche pas. */
    public static IntroOverlay showIfNeeded(Activity activity, Runnable onFinished) {
        if (shownThisProcess) return null;
        shownThisProcess = true;
        try {
            IntroOverlay intro = new IntroOverlay(activity, onFinished);
            intro.build();
            intro.start();
            return intro;
        } catch (Throwable error) {
            Log.e(TAG, "Generique impossible", error);
            return null;
        }
    }

    private float dp(float value) {
        return TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value,
                activity.getResources().getDisplayMetrics());
    }

    private ImageView newPhoto() {
        ImageView view = new ImageView(activity);
        view.setScaleType(ImageView.ScaleType.CENTER_CROP);
        view.setAlpha(0f);
        view.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        root.addView(view, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        return view;
    }

    private View gradient(int gravity, int heightDp, GradientDrawable.Orientation orientation, int... colors) {
        View view = new View(activity);
        view.setBackground(new GradientDrawable(orientation, colors));
        root.addView(view, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, heightDp < 0 ? ViewGroup.LayoutParams.MATCH_PARENT : (int) dp(heightDp), gravity));
        return view;
    }

    private TextView text(String value, float sp, int color, boolean bold, float spacing) {
        TextView view = new TextView(activity);
        view.setText(value);
        view.setTextColor(color);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        view.setTypeface(bold ? com.gta.game.ui.CwUi.black(activity) : com.gta.game.ui.CwUi.medium(activity));
        view.setLetterSpacing(spacing);
        view.setShadowLayer(dp(6), 0f, dp(1), Color.argb(220, 0, 0, 0));
        view.setIncludeFontPadding(false);
        return view;
    }

    private void build() {
        root = new FrameLayout(activity);
        root.setBackgroundColor(Color.BLACK);
        root.setClickable(true);
        root.setFocusable(true);
        root.setOnClickListener(view -> { if (skipAllowed) fadeOutAndFinish(450L); });

        back = newPhoto();
        front = newPhoto();

        // Vignette + degrades cinema (haut / bas) pour la lisibilite des textes
        gradient(Gravity.BOTTOM, 150, GradientDrawable.Orientation.BOTTOM_TOP,
                Color.argb(235, 0, 0, 0), Color.argb(120, 0, 0, 0), Color.TRANSPARENT);
        gradient(Gravity.TOP, 90, GradientDrawable.Orientation.TOP_BOTTOM,
                Color.argb(200, 0, 0, 0), Color.TRANSPARENT);
        View sideVignette = gradient(Gravity.CENTER, -1, GradientDrawable.Orientation.LEFT_RIGHT,
                Color.argb(150, 0, 0, 0), Color.TRANSPARENT, Color.TRANSPARENT, Color.argb(150, 0, 0, 0));
        sideVignette.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);

        // Bandes noires (format cinema) qui s'ouvrent au debut
        View barTop = new View(activity);
        barTop.setBackgroundColor(Color.BLACK);
        root.addView(barTop, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, (int) dp(22), Gravity.TOP));
        View barBottom = new View(activity);
        barBottom.setBackgroundColor(Color.BLACK);
        root.addView(barBottom, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, (int) dp(22), Gravity.BOTTOM));
        barTop.setTag("barTop");
        barBottom.setTag("barBottom");

        // Legende de chaque photo (bas gauche) : petit sur-titre + phrase
        captionBox = new LinearLayout(activity);
        captionBox.setOrientation(LinearLayout.VERTICAL);
        captionBox.setAlpha(0f);
        View accent = new View(activity);
        accent.setBackgroundColor(Color.WHITE);
        captionBox.addView(accent, new LinearLayout.LayoutParams((int) dp(26), (int) dp(2)));
        captionKicker = text("", 10f, Color.argb(200, 255, 255, 255), false, 0.45f);
        LinearLayout.LayoutParams kp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        kp.topMargin = (int) dp(8);
        captionBox.addView(captionKicker, kp);
        captionLine = text("", 21f, Color.WHITE, true, 0.02f);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        lp.topMargin = (int) dp(5);
        captionBox.addView(captionLine, lp);
        FrameLayout.LayoutParams cp = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM | Gravity.START);
        cp.leftMargin = (int) dp(34);
        cp.bottomMargin = (int) dp(44);
        root.addView(captionBox, cp);

        // Bloc studio (final)
        studioBox = new LinearLayout(activity);
        studioBox.setOrientation(LinearLayout.VERTICAL);
        studioBox.setGravity(Gravity.CENTER_HORIZONTAL);
        studioBox.setAlpha(0f);
        TextView studio = text(activity.getString(R.string.intro_studio_name), 13f, Color.WHITE, true, 0.42f);
        studioBox.addView(studio, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        presents = text(activity.getString(R.string.intro_presents), 10f, Color.argb(210, 255, 255, 255), false, 0.35f);
        presents.setAlpha(0f);
        LinearLayout.LayoutParams pp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        pp.topMargin = (int) dp(5);
        studioBox.addView(presents, pp);
        FrameLayout.LayoutParams sp = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.TOP | Gravity.CENTER_HORIZONTAL);
        sp.topMargin = (int) dp(32);
        root.addView(studioBox, sp);


        copyright = text(activity.getString(R.string.intro_copyright), 8.5f, Color.argb(150, 255, 255, 255), false, 0.08f);
        copyright.setAlpha(0f);
        FrameLayout.LayoutParams crp = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL);
        crp.bottomMargin = (int) dp(30);
        root.addView(copyright, crp);

        skipHint = text(activity.getString(R.string.intro_skip), 9f, Color.argb(150, 255, 255, 255), false, 0.12f);
        skipHint.setAlpha(0f);
        FrameLayout.LayoutParams skp = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM | Gravity.END);
        skp.rightMargin = (int) dp(30);
        skp.bottomMargin = (int) dp(30);
        root.addView(skipHint, skp);

        // Barre de progression fine (en bas, au-dessus de la bande noire)
        FrameLayout track = new FrameLayout(activity);
        track.setBackgroundColor(Color.argb(40, 255, 255, 255));
        FrameLayout.LayoutParams tp = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, Math.max(2, (int) dp(1.5f)), Gravity.BOTTOM);
        tp.bottomMargin = (int) dp(22);
        root.addView(track, tp);
        progressFill = new View(activity);
        progressFill.setBackgroundColor(Color.argb(230, 255, 255, 255));
        progressFill.setPivotX(0f);
        progressFill.setScaleX(0f);
        track.addView(progressFill, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        // tout ce qui n'est pas une photo reste dessine au-dessus des photos
        for (int i = 0; i < root.getChildCount(); i++) {
            View child = root.getChildAt(i);
            if (child != front && child != back) { child.setOutlineProvider(null); child.setTranslationZ(10f); }
        }

        activity.addContentView(root, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    }

    private void start() {
        startedAt = System.currentTimeMillis();
        startMusic();

        progressFill.animate().scaleX(1f).setDuration(TOTAL_MS).setInterpolator(new LinearInterpolator()).start();

        for (int pass = 0; pass < SLIDE_PASSES; pass++) {
            final int index = pass;
            handler.postDelayed(() -> showSlide(SLIDES[index % SLIDES.length], index, false), pass * SLIDE_MS);
        }
        handler.postDelayed(this::showFinale, SLIDE_PASSES * SLIDE_MS);

        handler.postDelayed(() -> {
            if (finished) return;
            skipAllowed = true;
            skipHint.animate().alpha(1f).setDuration(600L).start();
        }, SKIP_ALLOWED_AT_MS);

        handler.postDelayed(() -> fadeOutAndFinish(FADE_OUT_MS), TOTAL_MS - FADE_OUT_MS);
    }

    /** Fondu enchaine vers une nouvelle photo, avec zoom/panoramique lent (sens alterne a chaque passage). */
    private void showSlide(int drawable, int index, boolean finale) {
        if (finished) return;
        ImageView incoming = back;
        back = front;
        front = incoming;
        incoming.animate().cancel();
        if (fadeAnimator != null) fadeAnimator.cancel();
        incoming.setImageResource(drawable);
        // ordre d'affichage par Z : la nouvelle photo passe au-dessus de l'ancienne, l'interface reste au-dessus de tout
        incoming.setTranslationZ(2f);
        back.setTranslationZ(1f);

        long life = (finale ? FINALE_MS : SLIDE_MS) + CROSSFADE_MS;
        boolean zoomIn = finale || index % 2 == 0;
        float shift = dp(finale ? 0 : 14) * (index % 2 == 0 ? 1f : -1f);
        float endScale = zoomIn ? (finale ? 1.06f : 1.14f) : 1.0f;
        incoming.setScaleX(zoomIn ? 1.0f : 1.14f);
        incoming.setScaleY(zoomIn ? 1.0f : 1.14f);
        incoming.setTranslationX(-shift);
        incoming.setAlpha(0f);
        // Ken Burns (zoom + glissement lent) sur toute la vie de la photo
        incoming.animate().scaleX(endScale).scaleY(endScale).translationX(shift)
                .setDuration(life).setInterpolator(new LinearInterpolator()).start();
        // Fondu enchaine (animateur separe pour ne pas interrompre le Ken Burns)
        fadeAnimator = ObjectAnimator.ofFloat(incoming, View.ALPHA, 0f, 1f);
        fadeAnimator.setDuration(index == 0 && !finale ? 1600L : CROSSFADE_MS);
        fadeAnimator.setInterpolator(new AccelerateDecelerateInterpolator());
        fadeAnimator.start();

        if (index == 0 && !finale) openCinemaBars();
        if (!finale) showCaption(index);
    }

    private void openCinemaBars() {
        View top = root.findViewWithTag("barTop");
        View bottom = root.findViewWithTag("barBottom");
        if (top != null) { top.setPivotY(0f); top.setScaleY(2.4f); top.animate().scaleY(1f).setDuration(1800L).start(); }
        if (bottom != null) { bottom.setPivotY(dp(22)); bottom.setScaleY(2.4f); bottom.animate().scaleY(1f).setDuration(1800L).start(); }
    }

    private void showCaption(int index) {
        final String[] caption = CAPTIONS[index % CAPTIONS.length];
        captionBox.animate().cancel();
        captionBox.animate().setStartDelay(0L).alpha(0f).translationX(-dp(8)).setDuration(index == 0 ? 0L : 350L).withEndAction(() -> {
            if (finished) return;
            captionKicker.setText(caption[0]);
            captionLine.setText(caption[1]);
            captionBox.setTranslationX(dp(18));
            captionBox.animate().alpha(1f).translationX(0f).setStartDelay(450L).setDuration(900L)
                    .setInterpolator(new AccelerateDecelerateInterpolator()).withEndAction(() -> {
                        if (finished) return;
                        captionBox.animate().alpha(0f).setStartDelay(Math.max(0L, SLIDE_MS - 2400L)).setDuration(600L).start();
                    }).start();
        }).start();
    }

    private void showFinale() {
        if (finished) return;
        captionBox.animate().cancel();
        captionBox.animate().setStartDelay(0L).alpha(0f).setDuration(400L).start();
        showSlide(R.drawable.crestwood_intro, SLIDE_PASSES, true);
        handler.postDelayed(() -> {
            if (finished) return;
            studioBox.animate().alpha(1f).setDuration(1000L).start();
            copyright.animate().alpha(1f).setDuration(1000L).start();
        }, 900L);
        handler.postDelayed(() -> {
            if (!finished) presents.animate().alpha(1f).setDuration(900L).start();
        }, 1900L);
    }

    private void startMusic() {
        try {
            player = MediaPlayer.create(activity, R.raw.crestwood_intro);
            if (player == null) return;
            player.setVolume(MUSIC_VOLUME, MUSIC_VOLUME);
            player.setLooping(true);
            if (MUSIC_START_MS > 0) player.seekTo(MUSIC_START_MS);
            player.start();
        } catch (Throwable error) {
            Log.e(TAG, "Musique du generique indisponible", error);
            releasePlayer();
        }
    }

    private void fadeOutAndFinish(long durationMs) {
        if (finished || fadingOut) return;
        fadingOut = true;
        root.animate().alpha(0f).setDuration(durationMs).withEndAction(this::finish).start();
        fadeMusic(durationMs);
    }

    private void fadeMusic(final long durationMs) {
        final MediaPlayer current = player;
        if (current == null) return;
        final long fadeStart = System.currentTimeMillis();
        final Runnable step = new Runnable() {
            @Override public void run() {
                if (finished || player != current) return;
                float progress = Math.min(1f, (System.currentTimeMillis() - fadeStart) / (float) durationMs);
                try { current.setVolume(MUSIC_VOLUME * (1f - progress), MUSIC_VOLUME * (1f - progress)); }
                catch (Throwable ignored) { return; }
                if (progress < 1f) handler.postDelayed(this, 40L);
            }
        };
        handler.post(step);
    }

    private void finish() {
        if (finished) return;
        finished = true;
        handler.removeCallbacksAndMessages(null);
        if (fadeAnimator != null) fadeAnimator.cancel();
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
    }

    public void resume() {
        try { if (player != null && !finished && !player.isPlaying()) player.start(); } catch (Throwable ignored) { }
    }

    public void release() {
        if (finished) return;
        finished = true;
        handler.removeCallbacksAndMessages(null);
        releasePlayer();
    }
}
