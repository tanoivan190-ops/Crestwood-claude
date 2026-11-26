package com.crestwood.ui;

import android.app.Activity;
import android.content.Context;
import android.os.CancellationSignal;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.credentials.Credential;
import androidx.credentials.CredentialManager;
import androidx.credentials.CredentialManagerCallback;
import androidx.credentials.CustomCredential;
import androidx.credentials.GetCredentialRequest;
import androidx.credentials.GetCredentialResponse;
import androidx.credentials.exceptions.GetCredentialCancellationException;
import androidx.credentials.exceptions.GetCredentialException;
import androidx.credentials.exceptions.NoCredentialException;

import com.google.android.libraries.identity.googleid.GetSignInWithGoogleOption;
import com.google.android.libraries.identity.googleid.GoogleIdTokenCredential;
import com.google.firebase.FirebaseApp;
import com.google.firebase.FirebaseOptions;
import com.google.firebase.auth.AuthCredential;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.auth.FirebaseUser;
import com.google.firebase.auth.GoogleAuthProvider;

import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

/**
 * [v50] Connexion Google (Firebase Authentication) pour la page de connexion (login/index.html).
 *
 *   1. Google : Credential Manager ("Se connecter avec Google") -> Google ID token
 *   2. Firebase : signInWithCredential -> utilisateur Firebase -> Firebase ID token
 *   3. Service Node du serveur : POST http://host:port/gauth/verify -> ticket court a usage unique
 *   4. L'appelant envoie /authact google <ticket> au serveur SA-MP
 *
 * La configuration Firebase vient des ressources generees par app/build.gradle.kts a partir de
 * app/google-services.json (google_app_id, google_api_key, project_id, default_web_client_id) :
 * pas de plugin google-services (le package du fichier peut differer de l'applicationId).
 * Commun aux versions 32 et 64 bits (code Java pur).
 */
public final class CwGoogleAuth {
    private static final String TAG = "CwGoogleAuth";
    private static final String APP_NAME = "crestwood";
    private static final Executor IO = Executors.newSingleThreadExecutor();
    private static final Handler MAIN = new Handler(Looper.getMainLooper());
    private static volatile boolean busy;

    public interface Callback {
        /** Ticket pret : l'envoyer au serveur avec /authact google <ticket>. */
        void onTicket(String ticket, String email);
        /** Erreur ou annulation (message deja en francais ; cancelled = l'utilisateur a ferme la fenetre). */
        void onError(String message, boolean cancelled);
    }

    /** [v51] connexion seule (ecran JOUER du launcher) : pas d'echange avec le serveur. */
    public interface SignInCallback {
        void onSignedIn(String email, String name);
        void onError(String message, boolean cancelled);
    }

    private interface UserReady { void run(FirebaseUser user); }

    private CwGoogleAuth() { }

    private static String res(Context c, String name) {
        // [v54] le paquet des ressources (namespace com.gta.game) n'est pas l'applicationId
        //       (com.crestwoodroleplay) : on cherche d'abord dans le bon paquet de ressources.
        try {
            String pkg;
            try { pkg = c.getResources().getResourcePackageName(com.gta.game.R.string.google_app_id); }
            catch (Throwable t) { pkg = c.getPackageName(); }
            int id = c.getResources().getIdentifier(name, "string", pkg);
            if (id == 0) id = c.getResources().getIdentifier(name, "string", c.getPackageName());
            return id == 0 ? "" : c.getString(id).trim();
        } catch (Throwable t) {
            return "";
        }
    }

    /** Client OAuth "Web" du projet Firebase (cree quand on active le fournisseur Google). */
    public static String webClientId(Context c) {
        return res(c, "default_web_client_id");
    }

    public static boolean isConfigured(Context c) {
        return !TextUtils.isEmpty(webClientId(c)) && !TextUtils.isEmpty(res(c, "google_app_id"))
                && !TextUtils.isEmpty(res(c, "google_api_key")) && !TextUtils.isEmpty(res(c, "project_id"));
    }

    private static FirebaseAuth auth(Context ctx) {
        Context c = ctx.getApplicationContext();
        FirebaseApp app;
        try {
            app = FirebaseApp.getInstance(APP_NAME);
        } catch (IllegalStateException notYet) {
            FirebaseOptions o = new FirebaseOptions.Builder()
                    .setApiKey(res(c, "google_api_key"))
                    .setApplicationId(res(c, "google_app_id"))
                    .setProjectId(res(c, "project_id"))
                    .setGcmSenderId(res(c, "gcm_defaultSenderId"))
                    .setStorageBucket(res(c, "google_storage_bucket"))
                    .build();
            app = FirebaseApp.initializeApp(c, o, APP_NAME);
        }
        return FirebaseAuth.getInstance(app);
    }

    /** Compte Google deja connecte sur ce telephone (JSON {email,name,photo}) ou "" si aucun. */
    public static String currentAccount(Context ctx) {
        try {
            if (!isConfigured(ctx)) return "{\"ready\":0}";
            FirebaseUser u = auth(ctx).getCurrentUser();
            JSONObject o = new JSONObject();
            o.put("ready", 1);
            if (u != null) {
                o.put("email", u.getEmail() == null ? "" : u.getEmail());
                o.put("name", u.getDisplayName() == null ? "" : u.getDisplayName());
                o.put("photo", u.getPhotoUrl() == null ? "" : u.getPhotoUrl().toString());
            }
            return o.toString();
        } catch (Throwable t) {
            Log.w(TAG, "currentAccount", t);
            return "{\"ready\":0}";
        }
    }

    public static void signOut(Context ctx) {
        try { auth(ctx).signOut(); } catch (Throwable t) { Log.w(TAG, "signOut", t); }
        try {
            CredentialManager.create(ctx).clearCredentialStateAsync(
                    new androidx.credentials.ClearCredentialStateRequest(), new CancellationSignal(), IO,
                    new CredentialManagerCallback<Void, androidx.credentials.exceptions.ClearCredentialException>() {
                        @Override public void onResult(Void r) { }
                        @Override public void onError(@NonNull androidx.credentials.exceptions.ClearCredentialException e) { }
                    });
        } catch (Throwable ignored) { }
    }

    /**
     * Lance la connexion. Peut etre appele depuis n'importe quel thread ; les callbacks arrivent
     * sur le thread principal.
     * @param chooseAccount true = toujours afficher le choix du compte Google (bouton "Changer de compte")
     */
    public static void start(final Activity act, final String host, final int port, final String playerName,
                             final boolean chooseAccount, final Callback cb) {
        MAIN.post(() -> {
            if (busy) return;
            if (act == null || act.isFinishing()) return;
            if (!isConfigured(act)) {
                cb.onError("Connexion Google pas encore configurée sur cette version (Firebase).", false);
                return;
            }
            busy = true;
            final Callback done = new Callback() {
                @Override public void onTicket(String t, String e) { busy = false; cb.onTicket(t, e); }
                @Override public void onError(String m, boolean c) { busy = false; cb.onError(m, c); }
            };
            try {
                FirebaseAuth fa = auth(act);
                FirebaseUser u = fa.getCurrentUser();
                if (u != null && !chooseAccount) {
                    exchange(u, host, port, playerName, done);
                    return;
                }
                if (u != null) fa.signOut();
                pickGoogleAccount(act, fa, user -> exchange(user, host, port, playerName, done), done);
            } catch (Throwable t) {
                Log.e(TAG, "start", t);
                done.onError("Connexion Google impossible sur ce téléphone.", false);
            }
        });
    }

    /** [v51] Google -> Firebase sans echange serveur. Callbacks sur le thread principal. */
    public static void signIn(final Activity act, final boolean chooseAccount, final SignInCallback cb) {
        MAIN.post(() -> {
            if (busy || act == null || act.isFinishing()) return;
            if (!isConfigured(act)) {
                cb.onError("Connexion Google pas encore configurée sur cette version (Firebase).", false);
                return;
            }
            busy = true;
            final Callback err = new Callback() {
                @Override public void onTicket(String t, String e) { busy = false; }
                @Override public void onError(String m, boolean c) { busy = false; cb.onError(m, c); }
            };
            try {
                FirebaseAuth fa = auth(act);
                FirebaseUser u = fa.getCurrentUser();
                if (u != null && !chooseAccount) {
                    busy = false;
                    cb.onSignedIn(u.getEmail() == null ? "" : u.getEmail(), u.getDisplayName() == null ? "" : u.getDisplayName());
                    return;
                }
                if (u != null) fa.signOut();
                pickGoogleAccount(act, fa, user -> {
                    busy = false;
                    cb.onSignedIn(user.getEmail() == null ? "" : user.getEmail(), user.getDisplayName() == null ? "" : user.getDisplayName());
                }, err);
            } catch (Throwable t) {
                Log.e(TAG, "signIn", t);
                err.onError("Connexion Google impossible sur ce téléphone.", false);
            }
        });
    }

    private static void pickGoogleAccount(final Activity act, final FirebaseAuth fa, final UserReady ok, final Callback cb) {
        GetSignInWithGoogleOption option = new GetSignInWithGoogleOption.Builder(webClientId(act)).build();
        GetCredentialRequest req = new GetCredentialRequest.Builder().addCredentialOption(option).build();
        CredentialManager cm = CredentialManager.create(act);
        cm.getCredentialAsync(act, req, new CancellationSignal(), act.getMainExecutor(),
                new CredentialManagerCallback<GetCredentialResponse, GetCredentialException>() {
                    @Override public void onResult(GetCredentialResponse r) {
                        try {
                            Credential c = r.getCredential();
                            if (!(c instanceof CustomCredential)
                                    || !GoogleIdTokenCredential.TYPE_GOOGLE_ID_TOKEN_CREDENTIAL.equals(c.getType())) {
                                cb.onError("Réponse Google inattendue.", false);
                                return;
                            }
                            String googleIdToken = GoogleIdTokenCredential.createFrom(((CustomCredential) c).getData()).getIdToken();
                            AuthCredential fc = GoogleAuthProvider.getCredential(googleIdToken, null);
                            fa.signInWithCredential(fc).addOnCompleteListener(act, task -> {
                                if (!task.isSuccessful() || fa.getCurrentUser() == null) {
                                    Log.w(TAG, "Firebase signIn", task.getException());
                                    cb.onError("Firebase a refusé la connexion Google (empreinte SHA-1 ou fournisseur Google à vérifier).", false);
                                    return;
                                }
                                ok.run(fa.getCurrentUser());
                            });
                        } catch (Throwable t) {
                            Log.e(TAG, "credential", t);
                            cb.onError("Compte Google illisible.", false);
                        }
                    }

                    @Override public void onError(@NonNull GetCredentialException e) {
                        Log.w(TAG, "getCredential: " + e.getClass().getSimpleName() + " " + e.getMessage());
                        if (e instanceof GetCredentialCancellationException) cb.onError("", true);
                        else if (e instanceof NoCredentialException) cb.onError("Aucun compte Google sur ce téléphone. Ajoute-en un dans les paramètres Android.", false);
                        else cb.onError("Google indisponible : " + (e.getMessage() == null ? e.getType() : e.getMessage()), false);
                    }
                });
    }

    /** Firebase ID token -> service Node du serveur -> ticket. */
    private static void exchange(final FirebaseUser u, final String host, final int port, final String playerName, final Callback cb) {
        final String email = u.getEmail() == null ? "" : u.getEmail();
        u.getIdToken(true).addOnCompleteListener(t -> {
            if (!t.isSuccessful() || t.getResult() == null || t.getResult().getToken() == null) {
                Log.w(TAG, "getIdToken", t.getException());
                cb.onError("Session Google expirée. Touche à nouveau Continuer avec Google.", false);
                try { FirebaseAuth.getInstance(FirebaseApp.getInstance(APP_NAME)).signOut(); } catch (Throwable ignored) { }
                return;
            }
            final String idToken = t.getResult().getToken();
            IO.execute(() -> {
                String err = "Serveur de connexion Google injoignable.";
                String ticket = null;
                HttpURLConnection con = null;
                try {
                    JSONObject body = new JSONObject();
                    body.put("token", idToken);
                    body.put("name", playerName == null ? "" : playerName);
                    URL url = new URL("http://" + host + ":" + port + "/gauth/verify");
                    con = (HttpURLConnection) url.openConnection();
                    con.setConnectTimeout(6000);
                    con.setReadTimeout(9000);
                    con.setRequestMethod("POST");
                    con.setDoOutput(true);
                    con.setRequestProperty("Content-Type", "application/json; charset=utf-8");
                    byte[] out = body.toString().getBytes(StandardCharsets.UTF_8);
                    con.setFixedLengthStreamingMode(out.length);
                    try (OutputStream os = con.getOutputStream()) { os.write(out); }
                    int code = con.getResponseCode();
                    InputStream is = code >= 400 ? con.getErrorStream() : con.getInputStream();
                    String txt = read(is);
                    JSONObject r = new JSONObject(txt.isEmpty() ? "{}" : txt);
                    if (code == 200 && r.optInt("ok") == 1) ticket = r.optString("ticket", "").replaceAll("[^A-Za-z0-9]", "");
                    else if (r.has("err")) err = r.optString("err");
                } catch (Throwable e) {
                    Log.w(TAG, "verify " + host + ":" + port, e);
                } finally {
                    if (con != null) con.disconnect();
                }
                final String tk = ticket, er = err;
                MAIN.post(() -> {
                    if (tk != null && tk.length() == 20) cb.onTicket(tk, email);
                    else cb.onError(er, false);
                });
            });
        });
    }

    private static String read(InputStream is) throws java.io.IOException {
        if (is == null) return "";
        try (InputStream in = is) {
            ByteArrayOutputStream bo = new ByteArrayOutputStream();
            byte[] b = new byte[2048];
            int n;
            while ((n = in.read(b)) > 0 && bo.size() < 16384) bo.write(b, 0, n);
            return bo.toString("UTF-8");
        }
    }
}
