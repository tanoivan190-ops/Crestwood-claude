package com.gta.game.ui;

import android.app.Activity;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.webkit.JavascriptInterface;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.gta.game.R;
import com.rockstargames.oswrapper.DataInstaller;

public class LoadingScreen {
    private final Activity activity;
    private ConstraintLayout mainLayout;
    private WebView loadingWebView;
    private Handler handler;
    private boolean completed;
    private boolean hiding;
    // Filet de securite : si le jeu ne ferme pas l'ecran (dialogue, spawn...), on le ferme tout seul apres 30 s.
    private static final long MAX_WAIT_MS = 30_000L;
    private static final long FADE_MS = 350L;

    public LoadingScreen(Activity activity) {
        this.activity = activity;
        mainLayout = (ConstraintLayout) activity.getLayoutInflater().inflate(R.layout.loadingscreen, null);
        activity.addContentView(mainLayout, new ConstraintLayout.LayoutParams(
                ConstraintLayout.LayoutParams.MATCH_PARENT,
                ConstraintLayout.LayoutParams.MATCH_PARENT));
        initializeViews();
    }

    private void initializeViews() {
        handler = new Handler(Looper.getMainLooper());
        loadingWebView = mainLayout.findViewById(R.id.loadingWebView);
        if (loadingWebView == null) return;
        // sans ca la WebView peint un fond BLANC tant que la page n'est pas affichee (l'"ecran blanc" du 32 bits)
        loadingWebView.setBackgroundColor(0xFF050506);
        loadingWebView.setWebViewClient(new WebViewClient() {
            // [v19] moteur de rendu perdu (memoire) : sans ce "return true", Android ferme toute l'application
            @Override public boolean onRenderProcessGone(WebView view, android.webkit.RenderProcessGoneDetail detail) {
                android.util.Log.e("LoadingScreen", "[webview] moteur de rendu perdu : ecran de chargement retire");
                runOnUiThread(() -> {
                    try {
                        android.view.ViewGroup parent = (android.view.ViewGroup) view.getParent();
                        if (parent != null) parent.removeView(view);
                        view.destroy();
                    } catch (Throwable ignored) { }
                    if (loadingWebView == view) loadingWebView = null;
                    if (handler != null) handler.removeCallbacksAndMessages(null);
                    if (mainLayout != null) mainLayout.setVisibility(View.GONE);
                });
                return true;
            }
            @Override public void onPageFinished(WebView view, String url) {
                // La data peut etre deja prete avant la fin du chargement de la page : on remet l'etat "connexion".
                if (completed) evaluate("window.LoadingBridge&&LoadingBridge.connecting()");
            }
        });
        WebSettings settings = loadingWebView.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);   // [v21] langue + reglement memorises (sinon affiches a chaque lancement)
        settings.setAllowFileAccess(true);
        settings.setAllowContentAccess(false);
        settings.setBuiltInZoomControls(false);
        settings.setDisplayZoomControls(false);
        settings.setLoadWithOverviewMode(true);
        settings.setUseWideViewPort(true);
        settings.setMediaPlaybackRequiresUserGesture(false); // musique de l'écran de chargement
        loadingWebView.addJavascriptInterface(new LoadingBridge(), "AndroidLoading");
        loadingWebView.loadUrl("file:///android_asset/loading/index.html");

        DataInstaller.setListener(new DataInstaller.Listener() {
            @Override public void onProgress(final int percent, final String phase, final long done, final long total) {
                runOnUiThread(() -> evaluate("window.LoadingBridge&&LoadingBridge.setProgress(" + percent + "," + quote(phase) + "," + done + "," + total + ")"));
            }
            @Override public void onComplete(java.io.File root) {
                runOnUiThread(() -> {
                    completed = true;
                    // Data prete : on reste affiche (connexion au serveur) jusqu'a ce que le jeu appelle hideLoadingScreen()
                    // (dialogue de connexion, ecran de personnage, spawn...). Filet de securite : fermeture forcee apres MAX_WAIT_MS.
                    evaluate("window.LoadingBridge&&LoadingBridge.connecting()");
                    if (handler != null) {
                        handler.removeCallbacksAndMessages(null);
                        handler.postDelayed(LoadingScreen.this::hide, MAX_WAIT_MS);
                    }
                });
            }
            @Override public void onError(final String message, Throwable error) {
                runOnUiThread(() -> evaluate("window.LoadingBridge&&LoadingBridge.error(" + quote(message) + ")"));
            }
        });
        DataInstaller.prepare(activity);
    }

    private void runOnUiThread(Runnable action) {
        if (activity == null || action == null) return;
        activity.runOnUiThread(action);
    }

    private void evaluate(String script) {
        if (loadingWebView != null && mainLayout != null && mainLayout.getVisibility() == View.VISIBLE) {
            loadingWebView.evaluateJavascript(script, null);
        }
    }

    private static String quote(String value) {
        if (value == null) return "\"\"";
        return "\"" + value.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", " ") + "\"";
    }

    public void hide() {
        if (handler != null) handler.removeCallbacksAndMessages(null);
        // Deja cache (ou en train de se cacher) : rien a faire
        if (mainLayout == null || mainLayout.getVisibility() != View.VISIBLE || hiding) return;
        hiding = true;
        if (loadingWebView != null) loadingWebView.evaluateJavascript("window.LoadingBridge&&LoadingBridge.stopMusic&&LoadingBridge.stopMusic()", null);
        // Fondu de 350 ms puis l'ecran disparait et le joueur peut toucher le jeu
        mainLayout.animate().alpha(0f).setDuration(FADE_MS).withEndAction(() -> {
            hiding = false;
            if (mainLayout != null) {
                mainLayout.setVisibility(View.GONE);
                mainLayout.setAlpha(1f);
            }
            // [v19] libere la page (images du diaporama + musique) pendant le jeu : show() la recharge
            if (loadingWebView != null) loadingWebView.loadUrl("about:blank");
        }).start();
    }

    public void show() {
        if (mainLayout == null || loadingWebView == null) return;
        if (hiding) { // un fondu est en cours : on l'annule et on reste affiche
            mainLayout.animate().cancel();
            hiding = false;
            mainLayout.setAlpha(1f);
        }
        if (mainLayout.getVisibility() == View.VISIBLE) return;
        mainLayout.setVisibility(View.VISIBLE);
        if (loadingWebView != null) loadingWebView.loadUrl("file:///android_asset/loading/index.html");
        if (completed && handler != null) {
            handler.postDelayed(() -> evaluate("window.LoadingBridge&&LoadingBridge.connecting()"), 600L);
            handler.postDelayed(this::hide, MAX_WAIT_MS);
        }
    }

    public void destroy() {
        hide();
        if (loadingWebView != null) {
            loadingWebView.stopLoading();
            loadingWebView.removeJavascriptInterface("AndroidLoading");
            loadingWebView.destroy();
            loadingWebView = null;
        }
        handler = null;
        mainLayout = null;
    }

    private final class LoadingBridge {
        @JavascriptInterface public void start() { DataInstaller.prepare(activity); }
        @JavascriptInterface public void retry() { DataInstaller.repair(activity); }
        // Bouton "Entrer en jeu" de l'ecran de chargement : ferme l'ecran sur le thread UI
        @JavascriptInterface public void skip() { runOnUiThread(LoadingScreen.this::hide); }
        // [v21] memoire native du premier lancement (cw.lang / cw.rules) : survit a tout
        @JavascriptInterface public String getPref(String key) {
            if (key == null || !key.matches("cw\\.[a-z]{1,12}")) return "";
            return activity.getSharedPreferences("crestwood_welcome", android.content.Context.MODE_PRIVATE).getString(key, "");
        }
        @JavascriptInterface public void setPref(String key, String value) {
            if (key == null || !key.matches("cw\\.[a-z]{1,12}") || value == null || value.length() > 32) return;
            activity.getSharedPreferences("crestwood_welcome", android.content.Context.MODE_PRIVATE).edit().putString(key, value).apply();
        }
    }
}
