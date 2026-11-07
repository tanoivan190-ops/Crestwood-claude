package com.gta.game32;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Rect;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.Looper;
import android.util.Base64;
import android.view.PixelCopy;
import android.view.SurfaceView;
import java.io.ByteArrayOutputStream;
import android.view.View;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.text.InputType;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.webkit.JavascriptInterface;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebChromeClient;
import android.webkit.ConsoleMessage;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceError;
import android.widget.Toast;
import android.view.ViewGroup;
import org.json.JSONObject;
import androidx.constraintlayout.widget.ConstraintLayout;


//import com.google.firebase.crashlytics.FirebaseCrashlytics;
import com.joom.paranoid.Obfuscate;
import com.gta.game.HudWebView;
import com.gta.game.R;
import com.gta.game.WebCompat;
import com.gta.game32.ui.AttachEdit;
import com.gta.game32.ui.CustomKeyboard;
import com.gta.game.ui.LoadingScreen;
import com.gta.game.ui.dialog.DialogManager;
import java.io.UnsupportedEncodingException;

/**
 * Client 32 bits (armeabi-v7a, moteur 2.10) : memes interfaces HTML que le client 64 bits
 * (com.gta.game.SAMP) : HUD + minimap + GPS, connexion, banque, inventaire, panneau admin,
 * reports, boutique, emplois, objectifs, dialogues HTML, telephone...
 * Ce fichier est une copie adaptee de com.gta.game.SAMP : les differences sont marquees "[32]".
 */
@Obfuscate
public class SAMP extends GTASA implements CustomKeyboard.InputListener, HeightProvider.HeightListener {

    private static final String TAG = "SAMP";
    private static SAMP instance;

    private DialogManager mDialog;
    private CustomKeyboard mKeyboard;   // [32] clavier du chat SA-MP (option "androidkeyboard")
    private HeightProvider mHeightProvider;

    private AttachEdit mAttachEdit;
    private LoadingScreen mLoadingScreen;
    // [WebView fluide] deux couches, comme CAMPINA : le HUD reste charge en permanence (hudMain) et les
    // panneaux (banque, garage, admin...) s'ouvrent dans une 2e WebView (hudPanel) au-dessus.
    // hudWebView = la couche active (celle qui recoit loadUrl / evaluateJavascript / modes tactiles).
    private HudWebView hudWebView;
    private HudWebView hudMain;
    private HudWebView hudPanel;
    private String hudMainUrl;
    private static final String HUD_PAGE = "file:///android_asset/hud/index.html";
    // [panneaux] fond = image du jeu reduite + floutee, rafraichie pendant qu'un panneau plein ecran est ouvert
    // (le flou CSS d'une WebView ne voit pas le jeu : il est dessine sur une autre surface, derriere)
    private android.widget.ImageView backdropView;
    private final android.graphics.Bitmap[] backdropBmp = new android.graphics.Bitmap[2];
    private int backdropIdx;
    private boolean backdropRunning;
    private boolean backdropBusy;
    private final android.os.Handler backdropHandler = new android.os.Handler(android.os.Looper.getMainLooper());
    private final Runnable backdropTick = this::captureBackdrop;
    private static final long BACKDROP_INTERVAL_MS = 180;   // ~5 images / s : le jeu "vit" derriere le panneau
    private EditText chatInput;
    private boolean configOpen;
    private boolean pharmacyOpen;
    private boolean bankOpen;
    // Ecran de connexion HTML (assets/login/index.html), pilote par les dialogues "@auth" (ids 9520-9522)
    private boolean authOpen;
    // Inventaire HTML (assets/inventory/index.html), pilote par les dialogues "@inv" (ids 9530-9532)
    private boolean invOpen;
    private volatile boolean invReady;
    private volatile String pendingInvJson;
    private Runnable invWatchdog;
    // Panneau d'administration HTML (assets/admin/index.html), pilote par les dialogues "@admin" (ids 9540-9542)
    private boolean adminOpen;
    private volatile boolean adminReady;
    private volatile String pendingAdminJson;
    private Runnable adminWatchdog;
    // Vestiaire de faction HTML (assets/locker/index.html), pilote par les dialogues "@locker" (ids 9560-9562)
    private boolean lockerOpen;
    private volatile boolean lockerReady;
    private volatile String pendingLockerJson;
    private Runnable lockerWatchdog;
    // Actions de police HTML (assets/police/index.html), pilote par les dialogues "@police" (ids 9570-9572)
    private boolean policeOpen;
    private volatile boolean policeReady;
    private volatile String pendingPoliceJson;
    private Runnable policeWatchdog;
    // Boutique 24/7 HTML (assets/shop/index.html), pilotee par les dialogues "@shop" (ids 9590-9592)
    private boolean shopOpen;
    private volatile boolean shopReady;
    private volatile String pendingShopJson;
    private Runnable shopWatchdog;
    // Contrat de travail / agence pour l'emploi HTML (assets/job/index.html), dialogues "@job" (ids 9600-9602)
    private boolean jobOpen;
    private volatile boolean jobReady;
    private volatile String pendingJobJson;
    private Runnable jobWatchdog;
    // Carte : icones fixes ("@blips", id 9605) et destination GPS ("@gps", id 9606) envoyees par le serveur,
    // gardees ici pour etre rejouees quand la page du HUD ou de la carte est rechargee.
    // Interfaces joueur HTML (assets/ui/index.html) : interaction, vehicule, papiers, permis, tenues ("@ui", 9630-9632)
    private boolean uiOpen;
    // Services HTML (assets/svc/index.html) : station-service, location ("@svc", 9810-9812)
    private boolean svcOpen;
    // Panneau admin mappings / portails / points (assets/mapadmin, "@maps", 9830-9832)
    private boolean mapsOpen;
    // Enceinte JBL (assets/jbl, "@jbl", 9850-9852) + barre rapide du HUD ("@hotbar", 9840)
    private boolean jblOpen;
    private volatile boolean jblReady;
    private volatile String pendingJblJson;
    private Runnable jblWatchdog;
    private volatile String lastHotbarJson = null;
    private volatile boolean mapsReady;
    private volatile String pendingMapsJson;
    private Runnable mapsWatchdog;
    private volatile boolean svcReady;
    private volatile String pendingSvcJson;
    private Runnable svcWatchdog;
    private volatile boolean uiReady;
    private volatile String pendingUiJson;
    private Runnable uiWatchdog;
    // Panneau du gouvernement HTML (assets/gov/index.html) : dossier citoyen, caisse, TVA ("@gov", 9660-9662)
    private boolean govOpen;
    private volatile boolean govReady;
    private volatile String pendingGovJson;
    private Runnable govWatchdog;
    // Panneau de faction HTML (assets/faction/index.html) : tableau de bord, membres, calendrier ("@fac", 9680-9682)
    private boolean facOpen;
    private volatile boolean facReady;
    private volatile String pendingFacJson;
    private Runnable facWatchdog;
    // Panneau des gangs HTML (assets/gang/index.html) : classement, territoires, membres ("@gang", 9720-9722)
    private boolean gangOpen;
    private volatile boolean gangReady;
    private volatile String pendingGangJson;
    private Runnable gangWatchdog;
    // Peche (assets/fish/index.html) : "@fish", 9740-9742
    private boolean fishOpen;
    // Garage des vehicules (assets/garage/index.html) : "@garage", 9750-9752 ; marqueurs 3D "@marks" 9760
    private boolean garageOpen;
    private volatile boolean garageReady;
    private volatile String pendingGarageJson;
    private Runnable garageWatchdog;
    private volatile boolean fishReady;
    private volatile String pendingFishJson;
    private Runnable fishWatchdog;
    // Immobilier : maisons des joueurs (assets/house/index.html) : "@house", 9730-9732
    private boolean houseOpen;
    private volatile boolean houseReady;
    private volatile String pendingHouseJson;
    private Runnable houseWatchdog;
    // Menu RP HTML (assets/rp/index.html) : accueil, animations, factures, annonces, urgences, personnage ("@rp", 9690-9692)
    private boolean rpOpen;
    private long rpClosedAtMs;   // anti-boucle : un "ouvrir" serveur juste apres une fermeture par le joueur est ignore
    // Dialogue de PNJ (assets/npc/index.html) : location de BMX a l'aeroport... ("@npc", 9710 ouvrir / 9712 fermer)
    private boolean npcOpen;
    private boolean npcReady;
    private String pendingNpcJson;
    private volatile boolean rpReady;
    // Telephone (assets/phone/index.html + net.js) : evenements serveur "@phone" 9700, actions /phoneact
    private volatile boolean phoneReady;
    private volatile String pendingRpJson;
    private Runnable rpWatchdog;
    // Support / reports HTML (assets/report/index.html) : "@report" 9640-9642 (joueur), "@reports" 9643 (panneau admin)
    private boolean reportOpen;
    private volatile boolean reportReady;
    private volatile String pendingReportJson;
    private Runnable reportWatchdog;
    // Objectifs de mission (panneau du HUD) : "@obj" 9650, rejoue quand le HUD est recharge
    private volatile String lastObjJson = null;
    private volatile String lastBlipsJson = null;
    private volatile String lastGpsJson = null;      // {"x":..,"y":..,"auto":0|1} ou null
    private volatile float lastPosX = 0f, lastPosY = 0f;
    private volatile boolean hasLastPos = false;
    private volatile boolean authReady;
    // Redemande de l'ecran de connexion au serveur (le 1er dialogue peut arriver avant que le HUD soit pret)
    private boolean authBootstrapDone;
    private int authOpenAttempts;
    private volatile String pendingAuthJson;
    private final Handler authHandler = new Handler(Looper.getMainLooper());
    private Runnable authWatchdog;
    private volatile String pendingBankJson;
    private volatile boolean bankReady;
    private final Handler bankHandler = new Handler(Looper.getMainLooper());
    private Runnable bankWatchdog;
    // Panneau de vetements HTML (assets/clothes/index.html), pilote par le moteur natif (CharacterCreator)
    private boolean clothesPageOpen;
    private volatile boolean clothesReady;
    private volatile String pendingClothesJson;
    private final Handler clothesHandler = new Handler(Looper.getMainLooper());
    private Runnable clothesWatchdog;
    private boolean cameraOpen;
    private volatile int hudHunger = 100;
    private volatile int hudThirst = 100;
    /** [v15] carburant du vehicule (envoye par le serveur, dialogue "@hudv" 9516) */
    private volatile int hudFuel = 100;
    private volatile boolean gameSettingsApplied = false;
    private volatile String lastHudvJson = null;
    private volatile int pharmacyBalance = 0;
    private String lastHudJson = "";
    private long lastHudDispatchMs;
    // true des que le jeu est vivant (dialogue serveur recu ou HUD alimente) : l'ecran de chargement a ete ferme
    private volatile boolean loadingClosedByGame;


    public static SAMP getInstance() {
        return instance;
    }


    private void showLoadingScreen() {
        runOnUiThread(() -> { if (mLoadingScreen != null) mLoadingScreen.show(); });
    }

    private void hideLoadingScreen() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mLoadingScreen != null) mLoadingScreen.hide();
            }
        });
    }

    public void setPauseState(boolean pause) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (pause) {
                    mDialog.hideWithoutReset();
                    mAttachEdit.hideWithoutReset();
                } else {
                    if (mDialog.isShow)
                        mDialog.showWithOldContent();
                    if (mAttachEdit.isShow)
                        mAttachEdit.showWithoutReset();
                }

            }
        });
    }

    public void exitGame() {
        //FirebaseCrashlytics.getInstance().setCrashlyticsCollectionEnabled(false);
        finishAndRemoveTask();
        System.exit(0);
    }

    /** Dernier etat d'interaction recu du serveur : rejoue quand la page du HUD est rechargee (retour de la banque, du telephone...). */
    private volatile String lastInteractJson = null;

    public void showDialog(int dialogId, int dialogTypeId, byte[] bArr, byte[] bArr2, byte[] bArr3, byte[] bArr4) {
        // Textes du serveur : UTF-8 s'ils sont valides, sinon Windows-1252 (accents francais Latin-1 des .pwn)
        final String caption = decodeSamp(bArr);
        final String content = decodeSamp(bArr2);
        final String leftBtnText = decodeSamp(bArr3);
        final String rightBtnText = decodeSamp(bArr4);
        // Interaction tactile : le serveur signale qu'un point d'interaction est proche (id 9510, titre
        // "@interact", contenu JSON {"show":1,"id":"bank","label":"Banque"} ou {"show":0}).
        // Rien n'est affiche en dialogue : l'icone de main du HUD apparait ou disparait.
        // [v15] infos HUD du serveur (id 9516, titre "@hudv", JSON {"fuel":72,"gz":1,"gzn":"Mairie","belt":0,"veh":1})
        if (dialogId == 9516) {
            final String hv = content == null ? "{}" : content.replace("\u0000", "").trim();
            try {
                JSONObject o = new JSONObject(hv.isEmpty() ? "{}" : hv);
                if (o.has("fuel")) hudFuel = Math.max(0, Math.min(100, o.optInt("fuel", 100)));
            } catch (Exception ignored) { }
            if (looksJson(hv)) {
                lastHudvJson = hv;
                hudJs("window.HudApp && HudApp.updateHudInfo && HudApp.updateHudInfo(" + hv + ")");
            }
            return;
        }
        // [v15] joueurs a cacher (admins invisibles) : id 9517, titre "@hide", contenu "3,7" (vide = personne)
        if (dialogId == 9517) {
            final String ids = content == null ? "" : content.replace("\u0000", "").replaceAll("[^0-9,]", "");
            try { setHiddenPlayers(ids); } catch (Throwable t) { Log.w(TAG, "setHiddenPlayers indisponible", t); }
            return;
        }
        if (dialogId == 9510) {
            final String capInteract = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            if (capInteract.startsWith("@interact")) {
                final String interactJson = content == null ? "{}" : content.replace("\u0000", "").trim();
                lastInteractJson = interactJson;
                hudJs("window.HudBridge && HudBridge.setInteract && HudBridge.setInteract(" + JSONObject.quote(interactJson) + ")");
                return;
            }
        }
        // Boutique : ids 9590 (ouvrir), 9591 (mise a jour), 9592 (fermer), titre "@shop", contenu JSON.
        if (dialogId >= 9590 && dialogId <= 9592) {
            final String capShop = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            final boolean shopJson = looksJson(content);
            if (capShop.startsWith("@shop") || shopJson) {
                handleShopDialog(dialogId, content);
                return;
            }
        }
        // Emplois : ids 9600 (ouvrir), 9601 (mise a jour), 9602 (fermer), titre "@job", contenu JSON.
        if (dialogId >= 9600 && dialogId <= 9602) {
            final String capJob = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            final boolean jobJson = looksJson(content);
            if (capJob.startsWith("@job") || jobJson) {
                handleJobDialog(dialogId, content);
                return;
            }
        }
        // PNJ : ids 9710 (ouvrir) / 9712 (fermer), titre "@npc", contenu JSON (nom, texte, choix).
        if (dialogId == 9710 || dialogId == 9712) {
            handleNpcDialog(dialogId, content);
            return;
        }
        // Telephone : id 9700, titre "@phone", contenu JSON {"ev":...} (appels, SMS, Wave, urgences)
        if (dialogId == 9700) {
            handlePhoneDialog(content);
            return;
        }
        // Menu RP : ids 9690 (ouvrir), 9691 (mise a jour), 9692 (fermer), titre "@rp", contenu JSON.
        if (dialogId >= 9690 && dialogId <= 9692) {
            final String capRp = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            if (capRp.startsWith("@rp") || looksJson(content)) {
                handleRpDialog(dialogId, content);
                return;
            }
        }
        // Immobilier : maisons des joueurs : ids 9730 (ouvrir), 9731 (mise a jour), 9732 (fermer), titre "@house", contenu JSON.
        if (dialogId >= 9730 && dialogId <= 9732) {
            final String capHouse = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            if (capHouse.startsWith("@house") || looksJson(content)) {
                handleHouseDialog(dialogId, content);
                return;
            }
        }
        // Garage : ids 9750 (ouvrir), 9751 (mise a jour), 9752 (fermer), titre "@garage", contenu JSON.
        if (dialogId >= 9750 && dialogId <= 9752) {
            final String capGar = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            if (capGar.startsWith("@garage") || looksJson(content)) {
                handleGarageDialog(dialogId, content);
                return;
            }
        }
        // Marqueurs 3D flottants : id 9760, titre "@marks", liste JSON [{"t":1,"x":..,"y":..,"z":..,"n":"..."}]
        if (dialogId == 9760 || dialogId == 9761) {
            handleWorldMarkers(content, dialogId == 9761);   // 9761 = suite de la liste (envoi en plusieurs morceaux)
            return;
        }
        // Peche : ids 9740 (ouvrir), 9741 (mise a jour), 9742 (fermer), titre "@fish", contenu JSON.
        if (dialogId >= 9740 && dialogId <= 9742) {
            final String capFish = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            if (capFish.startsWith("@fish") || looksJson(content)) {
                handleFishDialog(dialogId, content);
                return;
            }
        }
        // Panneau des gangs : ids 9720 (ouvrir), 9721 (mise a jour), 9722 (fermer), titre "@gang", contenu JSON.
        if (dialogId >= 9720 && dialogId <= 9722) {
            final String capGang = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            if (capGang.startsWith("@gang") || looksJson(content)) {
                handleGangDialog(dialogId, content);
                return;
            }
        }
        // Panneau de faction : ids 9680 (ouvrir), 9681 (mise a jour), 9682 (fermer), titre "@fac", contenu JSON.
        if (dialogId >= 9680 && dialogId <= 9682) {
            final String capFac = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            if (capFac.startsWith("@fac") || looksJson(content)) {
                handleFacDialog(dialogId, content);
                return;
            }
        }
        // Panneau du gouvernement : ids 9660 (ouvrir), 9661 (mise a jour), 9662 (fermer), titre "@gov", contenu JSON.
        if (dialogId >= 9660 && dialogId <= 9662) {
            final String capGov = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            if (capGov.startsWith("@gov") || looksJson(content)) {
                handleGovDialog(dialogId, content);
                return;
            }
        }
        // Services : ids 9810 (ouvrir), 9811 (mise a jour), 9812 (fermer), titre "@svc", contenu JSON.
        if (dialogId >= 9810 && dialogId <= 9812) {
            handleSvcDialog(dialogId, content);
            return;
        }
        // Barre d'objets rapides du HUD : id 9840, titre "@hotbar", contenu JSON (jamais affiche comme dialogue).
        if (dialogId == 9840) {
            final String hot = extractJson(content);
            lastHotbarJson = hot;
            hudJs("window.CwHotbar && CwHotbar.set(" + JSONObject.quote(hot) + ")");
            return;
        }
        // Enceinte JBL : ids 9850 (ouvrir), 9851 (mise a jour), 9852 (fermer), titre "@jbl".
        if (dialogId >= 9850 && dialogId <= 9852) {
            handleJblDialog(dialogId, content);
            return;
        }
        // Panneau admin mappings / portails / points : ids 9830 (ouvrir), 9831 (mise a jour), 9832 (fermer), titre "@maps".
        if (dialogId >= 9830 && dialogId <= 9832) {
            handleMapsDialog(dialogId, content);
            return;
        }
        // Interfaces joueur : ids 9630 (ouvrir), 9631 (mise a jour), 9632 (fermer), titre "@ui", contenu JSON.
        if (dialogId >= 9630 && dialogId <= 9632) {
            final String capUi = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            final boolean uiJson = looksJson(content);
            if (capUi.startsWith("@ui") || uiJson) {
                handleUiDialog(dialogId, content);
                return;
            }
        }
        // Support : 9640 (ouvrir), 9641 (mise a jour), 9642 (fermer) "@report" ; 9643 "@reports" (panneau admin) ;
        // 9644 "@note" (notification du HUD) ; 9650 "@obj" (objectifs de mission). Contenu JSON, rien n'est affiche en dialogue.
        if ((dialogId >= 9640 && dialogId <= 9644) || dialogId == 9650) {
            final String capRep = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            final boolean repJson = looksJson(content);
            if (capRep.startsWith("@report") || capRep.startsWith("@note") || capRep.startsWith("@obj") || repJson) {
                handleSupportDialog(dialogId, content);
                return;
            }
        }
        // Carte : icones fixes (9605 "@blips", liste JSON) et GPS (9606 "@gps", {"x","y"} ou {"clear":1}).
        if (dialogId == 9605 || dialogId == 9606) {
            final String capMap = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            if (capMap.startsWith("@blips") || capMap.startsWith("@gps")) {
                handleMapDialog(dialogId, content == null ? "" : content.replace("\u0000", "").trim());
                return;
            }
        }
        // Connexion / inscription : dialogues speciaux ids 9520 (ouvrir), 9521 (reponse du serveur), 9522 (fermer),
        // titre "@auth", contenu JSON. Rien n'est affiche en dialogue : on pilote assets/login/index.html.
        if (dialogId >= 9520 && dialogId <= 9522) {
            final String capAuth = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            final boolean authJson = looksJson(content);
            if (capAuth.startsWith("@auth") || authJson) {
                CrashLog.log("[auth] dialogue " + dialogId + " intercepte");
                handleAuthDialog(dialogId, content);
                return;
            }
            CrashLog.log("[auth] dialogue " + dialogId + " NON intercepte, titre='" + caption + "'");
        }
        // Administration : ids 9540 (ouvrir), 9541 (mise a jour), 9542 (fermer), titre "@admin", contenu JSON.
        if (dialogId >= 9540 && dialogId <= 9542) {
            final String capAdm = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            final boolean admJson = looksJson(content);
            if (capAdm.startsWith("@admin") || admJson) {
                handleAdminDialog(dialogId, content);
                return;
            }
        }
        // Actions de police : ids 9570 (ouvrir), 9571 (mise a jour), 9572 (fermer), titre "@police", contenu JSON.
        if (dialogId >= 9570 && dialogId <= 9572) {
            final String capPolice = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            final boolean policeJson = looksJson(content);
            if (capPolice.startsWith("@police") || policeJson) {
                handlePoliceDialog(dialogId, content);
                return;
            }
        }
        // Vestiaire de faction : ids 9560 (ouvrir), 9561 (mise a jour), 9562 (fermer), titre "@locker", contenu JSON.
        if (dialogId >= 9560 && dialogId <= 9562) {
            final String capLocker = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            final boolean lockerJson = looksJson(content);
            if (capLocker.startsWith("@locker") || lockerJson) {
                handleLockerDialog(dialogId, content);
                return;
            }
        }
        // Vetements : ids 9550 (appliquer l'apparence sauvegardee), 9551 (ouvrir l'editeur), 9552 (fermer),
        // titre "@clothes", contenu JSON. Rien n'est affiche en dialogue : le panneau natif prend le relais.
        if (dialogId >= 9550 && dialogId <= 9554) {
            final String capCloth = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            final boolean clothJson = looksJson(content);
            if (capCloth.startsWith("@clothes") || clothJson) {
                String rawCloth = content == null ? "{}" : content.replace("\u0000", "").trim();
                int cs = rawCloth.indexOf('{');
                int ce = rawCloth.lastIndexOf('}');
                final String clothPayload = (cs >= 0 && ce > cs) ? rawCloth.substring(cs, ce + 1) : "{}";
                final int clothId = dialogId;
                try {
                    onClothesDialog(clothId, clothPayload);
                } catch (UnsatisfiedLinkError e) {
                    Log.e(TAG, "[clothes] natif indisponible : " + e.getMessage());
                }
                return;
            }
        }
        // Inventaire : ids 9530 (ouvrir), 9531 (mise a jour), 9532 (fermer), titre "@inv", contenu JSON.
        if (dialogId >= 9530 && dialogId <= 9532) {
            final String capInv = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            final boolean invJson = looksJson(content);
            if (capInv.startsWith("@inv") || invJson) {
                handleInvDialog(dialogId, content);
                return;
            }
        }
        // Banque : le serveur envoie l'etat en JSON dans un dialogue special (ids 9500-9502,
        // titre "@bank"). On ne l'affiche pas : il va a la page assets/bank/index.html.
        if (dialogId >= 9500 && dialogId <= 9502) {
            // titre nettoye (codes couleur, caracteres nuls, espaces) + secours : contenu JSON
            final String capClean = caption.replace("\u0000", "").replaceAll("\\{[0-9A-Fa-f]{6}\\}", "").trim();
            final boolean looksJson = looksJson(content);
            if (capClean.startsWith("@bank") || looksJson) {
                Log.i(TAG, "[bank] dialogue " + dialogId + " intercepte, titre='" + capClean + "', "
                        + (content == null ? 0 : content.length()) + " car.");
                handleBankDialog(dialogId, content);
                return;
            }
            Log.w(TAG, "[bank] dialogue " + dialogId + " NON intercepte, titre='" + caption + "'");
        }
        // Le dialogue de connexion du serveur (login/inscription) s'affiche DERRIERE l'ecran de chargement :
        // sans cette ligne le joueur ne pouvait jamais le voir, donc jamais arriver au spawn.
        loadingClosedByGame = true;
        hideLoadingScreen();
        runOnUiThread(() -> { this.mDialog.show(dialogId, dialogTypeId, caption, content, leftBtnText, rightBtnText); });
    }
    /** Contenu JSON d'un dialogue special ? (un texte qui commence par un code couleur SA-MP {RRGGBB} n'en est pas un) */
    private static boolean looksJson(String c) {
        if (c == null) return false;
        String t = c.replace("\u0000", "").trim();
        return t.startsWith("{") && !t.matches("(?s)^\\{[0-9A-Fa-f]{6}\\}.*");
    }

    private static String decodeSamp(byte[] b) {
        if (b == null) return "";
        try {
            return java.nio.charset.StandardCharsets.UTF_8.newDecoder()
                    .onMalformedInput(java.nio.charset.CodingErrorAction.REPORT)
                    .onUnmappableCharacter(java.nio.charset.CodingErrorAction.REPORT)
                    .decode(java.nio.ByteBuffer.wrap(b)).toString();
        } catch (Exception e) {
            try {
                return new String(b, "windows-1252");
            } catch (Exception e2) {
                return new String(b);
            }
        }
    }

    private void showEditObject() {
        runOnUiThread(() -> mAttachEdit.show());
    }

    private void hideEditObject() {
        runOnUiThread(() -> mAttachEdit.hide());
    }


    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "**** onCreate");
        CrashLog.bind(this);
        CrashLog.logNativeDir(this);
        CrashLog.log("Dossier des donnees du jeu : '" + GetGameBaseDirectory() + "'");
        super.onCreate(savedInstanceState);
        // Vocal de proximite : on demande le micro des le lancement du jeu (sinon SampVoice ne peut pas enregistrer)
        requestMicPermission();
        CrashLog.log("Activite du jeu creee (moteur 2.10)");

        // [32] la vue du jeu (SurfaceView) est creee par NvEventQueueActivity.onCreate : le HUD HTML
        // est ajoute par-dessus, puis les dialogues et l'ecran de chargement au-dessus du HUD.
        instance = this;
        initializeHudWebView();
        mKeyboard = new CustomKeyboard(this);
        mDialog     = new DialogManager(this);
        mAttachEdit = new AttachEdit(this);
        mLoadingScreen = new LoadingScreen(this);
        initializeChatInput();

        try {
            initializeSAMP();
            CrashLog.log("SA-MP initialise");
        } catch (UnsatisfiedLinkError e5) {
            Log.e(TAG, e5.getMessage());
            CrashLog.log("ERREUR initializeSAMP : " + e5);
        }
        try {
            setHtmlHud(true);
        } catch (UnsatisfiedLinkError e6) {
            Log.e(TAG, "setHtmlHud indisponible : " + e6.getMessage());
        }
    }

    private native void initializeSAMP();
    // [32] envoi au serveur (commande "/..." ou message), texte deja en Windows-1252
    private native void sendChatBytes(byte[] input);
    // [Crestwood] vocal SampVoice : true tant que le bouton micro du HUD est maintenu
    private native void setVoiceTalk(boolean talk);
    // [v15] parametres en jeu -> moteur (game/CwSettings.cpp) et joueurs caches (admins invisibles)
    private native void applyGameSetting(int key, int value);
    private native void setHiddenPlayers(String data);
    /** [Crestwood] touches du HUD -> touches SA-MP (1 gauche 2 droite 3 accel 4 frein 5 frein a main 6 klaxon 7 sprint 8 saut 9 accroupi 10 monter/descendre, 0 = tout relacher) */
    private native void setVehicleControl(int control, boolean pressed);
    /** [Crestwood] joystick du HUD : x / y de -128 a 128, active = doigt pose sur le joystick */
    private native void setHudStick(int x, int y, boolean active);
    /** [Crestwood] marqueurs 3D flottants : "type,x,y,z,libelle;..." (game/WorldMarkers.cpp) */
    private native void setWorldMarkers(String data);
    private volatile boolean hudStickUnavailable;
    // [32] ouvre la saisie du chat SA-MP natif
    private native void openChatNative();
    // [32] le HUD HTML remplace le HUD de GTA (radar, vie, argent) : le moteur ne le dessine plus
    private native void setHtmlHud(boolean enabled);
    private native void onInputEnd(byte[] str);
    public native void onEventBackPressed();
    // Les vetements (CharacterCreator) n'existent que dans le client 64 bits : appels proteges.
    private native void openClothesTest();
    private native void onClothesDialog(int dialogId, String json);
    private native void clothesCommand(int type, String key, int value);

    /** [32] meme role que le natif sendChatInput du client 64 bits. */
    private void sendChatInput(String input) {
        if (input == null) return;
        String text = input.trim();
        if (text.isEmpty()) return;
        if (text.length() > 127) text = text.substring(0, 127);
        byte[] bytes;
        try {
            bytes = text.getBytes("windows-1252");
        } catch (UnsupportedEncodingException e) {
            bytes = text.getBytes();
        }
        try {
            sendChatBytes(bytes);
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "sendChatBytes indisponible : " + e.getMessage());
        }
    }

    private void openChatInput() {
        try {
            openChatNative();
        } catch (UnsatisfiedLinkError e) {
            showChatInput();   // secours : champ de saisie Android
        }
    }

    // ---- [32] appels du code natif (CJavaWrapper du client 32 bits) ----
    private void showTab() { }
    private void hideTab() { }
    private void setTab(int id, String name, int score, int ping) { }
    private void clearTab() { }

    private void showKeyboard() {
        runOnUiThread(() -> { if (mKeyboard != null) mKeyboard.ShowInputLayout(); });
    }

    private void hideKeyboard() {
        runOnUiThread(() -> { if (mKeyboard != null) mKeyboard.HideInputLayout(); });
    }

    @Override
    public void OnInputEnd(String str) {
        byte[] toReturn = null;
        try {
            toReturn = str.getBytes("windows-1252");
        } catch (UnsupportedEncodingException e) {
            toReturn = str.getBytes();
        }
        try {
            onInputEnd(toReturn);
        } catch (UnsatisfiedLinkError e5) {
            Log.e(TAG, e5.getMessage());
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            onBackPressed();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onStart() {
        Log.i(TAG, "**** onStart");
        super.onStart();
    }

    @Override
    public void onRestart() {
        Log.i(TAG, "**** onRestart");
        super.onRestart();
    }

    @Override
    public void onResume() {
        Log.i(TAG, "**** onResume");
        super.onResume();
        if (hudPanel != null) hudPanel.onResume();
        if (hudMain != null) hudMain.onResume();
        if (hudPanel != null && hudWebView == hudPanel && hudPanel.isFullscreenPage()) startBackdrop();
        if (hudWebView != null) {
            hudWebView.setVisibility(View.VISIBLE);
            hudWebView.evaluateJavascript("window.HudBridge && HudBridge.setVisible(true)", null);
        }
    }

    @Override
    public void onPause() {
        Log.i(TAG, "**** onPause");
        if (hudWebView != null) {
            hudWebView.evaluateJavascript("window.HudBridge && HudBridge.setVisible(false)", null);
            if (hudPanel != null) hudPanel.onPause();
            if (hudMain != null) hudMain.onPause();
            stopBackdrop();
        }
        super.onPause();
    }

    @Override
    public void onStop() {
        Log.i(TAG, "**** onStop");
        super.onStop();
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "**** onDestroy");
        if (hudWebView != null) {
            stopBackdrop();
            for (HudWebView v : new HudWebView[] { hudPanel, hudMain }) {
                if (v == null) continue;
                v.setMirror(null);
                v.loadUrl("about:blank");
                v.removeJavascriptInterface("AndroidHUD");
                v.removeJavascriptInterface("AndroidClothes");
                v.destroy();
            }
            hudWebView = null; hudMain = null; hudPanel = null;
        }
        super.onDestroy();
    }

    private int dp(float value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private void initializeChatInput() {
        chatInput = new EditText(this);
        chatInput.setSingleLine(true);
        chatInput.setHint("Digite sua mensagem...");
        chatInput.setTextColor(Color.WHITE);
        chatInput.setHintTextColor(Color.LTGRAY);
        chatInput.setTextSize(16);
        chatInput.setPadding(dp(14), 0, dp(14), 0);
        chatInput.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_CAP_SENTENCES);
        chatInput.setImeOptions(EditorInfo.IME_ACTION_SEND | EditorInfo.IME_FLAG_NO_EXTRACT_UI);
        chatInput.setVisibility(View.GONE);
        chatInput.setOnEditorActionListener((v, actionId, event) -> {
            boolean enter = event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER && event.getAction() == KeyEvent.ACTION_DOWN;
            if (actionId == EditorInfo.IME_ACTION_SEND || enter) {
                submitChatInput();
                return true;
            }
            return false;
        });
        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(-1, dp(52), Gravity.BOTTOM);
        params.leftMargin = dp(18);
        params.rightMargin = dp(18);
        params.bottomMargin = dp(12);
        addContentView(chatInput, params);
    }

    private void showChatInput() {
        runOnUiThread(() -> {
            if (chatInput == null) return;
            chatInput.setVisibility(View.VISIBLE);
            chatInput.requestFocus();
            InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
            if (imm != null) imm.showSoftInput(chatInput, InputMethodManager.SHOW_IMPLICIT);
        });
    }

    private void submitChatInput() {
        if (chatInput == null) return;
        String value = chatInput.getText().toString().trim();
        chatInput.setText("");
        chatInput.setVisibility(View.GONE);
        InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
        if (imm != null) imm.hideSoftInputFromWindow(chatInput.getWindowToken(), 0);
        if (!value.isEmpty()) sendChatInput(value);
    }

    private void initializeHudWebView() {
        // [32] pas de layout game.xml : les WebView sont creees ici, plein ecran, au-dessus du jeu
        hudMain = buildWebLayer();
        hudPanel = buildWebLayer();
        hudPanel.setMirror(hudMain);
        hudPanel.setVisibility(View.GONE);
        hudPanel.loadUrl("about:blank");   // prechauffe le moteur de rendu du panneau
        hudWebView = hudMain;
        navigate(HUD_PAGE);
    }

    /** Une couche WebView transparente, accelere materiellement, avec tous les ponts JS. */
    @SuppressLint("SetJavaScriptEnabled")
    private HudWebView buildWebLayer() {
        final HudWebView hudWebView = new HudWebView(this);
        addContentView(hudWebView, new FrameLayout.LayoutParams(-1, -1));
        hudWebView.setBackgroundColor(Color.TRANSPARENT);
        WebCompat.apply(hudWebView);   // materiel, ou logiciel si le rendu a deja plante sur ce telephone

        // Configuration WebView indispensable pour le rendu des pages HTML (login, HUD, etc.)
        WebSettings ws = hudWebView.getSettings();
        ws.setJavaScriptEnabled(true);
        ws.setDomStorageEnabled(true);
        ws.setAllowFileAccessFromFileURLs(true);
        ws.setAllowUniversalAccessFromFileURLs(true);
        ws.setMediaPlaybackRequiresUserGesture(false);
        ws.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);
        ws.setCacheMode(WebSettings.LOAD_DEFAULT);
        // supprimé : API AppCache retirée du SDK

        hudWebView.setWebChromeClient(new WebChromeClient() {
            @Override public boolean onConsoleMessage(ConsoleMessage m) {
                // [fluide] seules les erreurs / avertissements JS vont dans le journal (avant : chaque console.log)
                ConsoleMessage.MessageLevel lvl = m.messageLevel();
                if (lvl == ConsoleMessage.MessageLevel.ERROR || lvl == ConsoleMessage.MessageLevel.WARNING) {
                    Log.w("HUD-JS", m.message() + " (" + m.sourceId() + ":" + m.lineNumber() + ")");
                    CrashLog.log("[js] " + m.message() + " (" + m.sourceId() + ":" + m.lineNumber() + ")");
                }
                return true;
            }
        });
        hudWebView.setWebViewClient(new WebViewClient() {
            @Override public boolean onRenderProcessGone(WebView view, android.webkit.RenderProcessGoneDetail detail) {
                if (detail != null && detail.didCrash()) {
                    WebCompat.onRendererCrash(SAMP.this);
                    CrashLog.log("[webview] rendu logiciel active pour les prochaines couches");
                }
                CrashLog.log("[webview] processus de rendu perdu (crash=" + (detail != null && detail.didCrash()) + ")");
                runOnUiThread(() -> {
                    rpOpen = false; rpReady = false; reportOpen = false; reportReady = false; configOpen = false;
                    // une WebView dont le moteur de rendu est mort n'est plus utilisable : on recree les deux couches
                    rebuildWebLayers();
                });
                return true;
            }
            @Override public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error) {
                CrashLog.log("[webview] erreur " + (request != null ? request.getUrl() : "?")
                        + " : " + (error != null ? error.getDescription() : "?"));
            }
            @Override public void onPageFinished(WebView view, String url) {
                CrashLog.log("[webview] page chargee : " + url);
                String invJson = pendingInvJson;
                if (invJson != null && url != null && url.endsWith("inventory/index.html")) {
                    pendingInvJson = null;
                    view.evaluateJavascript("window.InvApp && InvApp.openFromNative(" + JSONObject.quote(invJson) + ")", null);
                }
                String admJson = pendingAdminJson;
                if (admJson != null && url != null && url.endsWith("admin/index.html")) {
                    pendingAdminJson = null;
                    view.evaluateJavascript("window.AdminApp && AdminApp.openFromNative(" + JSONObject.quote(admJson) + ")", null);
                }
                String policeJson = pendingPoliceJson;
                if (policeJson != null && url != null && url.endsWith("police/index.html")) {
                    pendingPoliceJson = null;
                    view.evaluateJavascript("window.PoliceApp && PoliceApp.openFromNative(" + JSONObject.quote(policeJson) + ")", null);
                }
                String shopJson = pendingShopJson;
                if (shopJson != null && url != null && url.endsWith("shop/index.html")) {
                    pendingShopJson = null;
                    view.evaluateJavascript("window.ShopApp && ShopApp.openFromNative(" + JSONObject.quote(shopJson) + ")", null);
                }
                String jobJson = pendingJobJson;
                if (jobJson != null && url != null && url.endsWith("job/index.html")) {
                    pendingJobJson = null;
                    view.evaluateJavascript("window.JobApp && JobApp.openFromNative(" + JSONObject.quote(jobJson) + ")", null);
                }
                String facJsonPending = pendingFacJson;
                if (facJsonPending != null && url != null && url.endsWith("faction/index.html")) {
                    pendingFacJson = null;
                    view.evaluateJavascript("window.FacApp && FacApp.openFromNative(" + JSONObject.quote(facJsonPending) + ")", null);
                }
                String houseJsonPending = pendingHouseJson;
                if (houseJsonPending != null && url != null && url.endsWith("house/index.html")) {
                    pendingHouseJson = null;
                    view.evaluateJavascript("window.HouseApp && HouseApp.openFromNative(" + JSONObject.quote(houseJsonPending) + ")", null);
                }
                String garageJsonPending = pendingGarageJson;
                if (garageJsonPending != null && url != null && url.endsWith("garage/index.html")) {
                    pendingGarageJson = null;
                    view.evaluateJavascript("window.GarageApp && GarageApp.openFromNative(" + JSONObject.quote(garageJsonPending) + ")", null);
                }
                String fishJsonPending = pendingFishJson;
                if (fishJsonPending != null && url != null && url.endsWith("fish/index.html")) {
                    pendingFishJson = null;
                    view.evaluateJavascript("window.FishApp && FishApp.openFromNative(" + JSONObject.quote(fishJsonPending) + ")", null);
                }
                String gangJsonPending = pendingGangJson;
                if (gangJsonPending != null && url != null && url.endsWith("gang/index.html")) {
                    pendingGangJson = null;
                    view.evaluateJavascript("window.GangApp && GangApp.openFromNative(" + JSONObject.quote(gangJsonPending) + ")", null);
                }
                String govJsonPending = pendingGovJson;
                if (govJsonPending != null && url != null && url.endsWith("gov/index.html")) {
                    pendingGovJson = null;
                    view.evaluateJavascript("window.GovApp && GovApp.openFromNative(" + JSONObject.quote(govJsonPending) + ")", null);
                }
                String mapsJsonPending = pendingMapsJson;
                if (mapsJsonPending != null && url != null && url.endsWith("mapadmin/index.html")) {
                    pendingMapsJson = null;
                    view.evaluateJavascript("window.MapAdmin && MapAdmin.openFromNative(" + JSONObject.quote(mapsJsonPending) + ")", null);
                }
                String svcJsonPending = pendingSvcJson;
                if (svcJsonPending != null && url != null && url.endsWith("svc/index.html")) {
                    pendingSvcJson = null;
                    view.evaluateJavascript("window.SvcApp && SvcApp.openFromNative(" + JSONObject.quote(svcJsonPending) + ")", null);
                }
                String uiJsonPending = pendingUiJson;
                if (uiJsonPending != null && url != null && url.endsWith("ui/index.html")) {
                    pendingUiJson = null;
                    view.evaluateJavascript("window.UiApp && UiApp.openFromNative(" + JSONObject.quote(uiJsonPending) + ")", null);
                }
                if (url != null && (url.endsWith("hud/index.html") || url.endsWith("map/index.html"))) {
                    replayMapState();
                }
                String repJson = pendingReportJson;
                if (repJson != null && url != null && url.endsWith("report/index.html")) {
                    pendingReportJson = null;
                    view.evaluateJavascript("window.ReportApp && ReportApp.openFromNative(" + JSONObject.quote(repJson) + ")", null);
                }
                final String hotJson = lastHotbarJson;
                if (hotJson != null && url != null && url.endsWith("hud/index.html")) {
                    view.evaluateJavascript("window.CwHotbar && CwHotbar.set(" + JSONObject.quote(hotJson) + ")", null);
                }
                String jblJsonPending = pendingJblJson;
                if (jblJsonPending != null && url != null && url.endsWith("jbl/index.html")) {
                    pendingJblJson = null;
                    view.evaluateJavascript("window.JblApp && JblApp.openFromNative(" + JSONObject.quote(jblJsonPending) + ")", null);
                }
                final String objJson = lastObjJson;
                if (objJson != null && url != null && url.endsWith("hud/index.html")) {
                    view.evaluateJavascript("window.ObjTracker && ObjTracker.set(" + JSONObject.quote(objJson) + ")", null);
                }
                String lockerJson = pendingLockerJson;
                if (lockerJson != null && url != null && url.endsWith("locker/index.html")) {
                    pendingLockerJson = null;
                    view.evaluateJavascript("window.LockerApp && LockerApp.openFromNative(" + JSONObject.quote(lockerJson) + ")", null);
                }
                String authJson = pendingAuthJson;
                if (authJson != null && url != null && url.endsWith("login/index.html")) {
                    pendingAuthJson = null;
                    view.evaluateJavascript("window.DXAuth && DXAuth.openFromNative(" + JSONObject.quote(authJson) + ")", null);
                }
                String json = pendingBankJson;
                if (json != null && url != null && url.endsWith("bank/index.html")) {
                    pendingBankJson = null;
                    view.evaluateJavascript("window.BankApp && BankApp.openFromNative(" + JSONObject.quote(json) + ")", null);
                }
            }
        });
        WebSettings settings = hudWebView.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setAllowFileAccess(true);
        settings.setAllowFileAccessFromFileURLs(true);
        settings.setAllowUniversalAccessFromFileURLs(false);
        settings.setAllowContentAccess(false);
        settings.setBuiltInZoomControls(false);
        settings.setDisplayZoomControls(false);
        settings.setLoadWithOverviewMode(false);
        settings.setUseWideViewPort(false);
        // La musique de l'ecran de connexion doit pouvoir demarrer sans toucher l'ecran
        settings.setMediaPlaybackRequiresUserGesture(false);
        // [fluide] taille de texte fixe (le zoom systeme cassait la mise en page) + moteur de rendu prioritaire
        settings.setTextZoom(100);
        if (android.os.Build.VERSION.SDK_INT >= 26) {
            hudWebView.setRendererPriorityPolicy(WebView.RENDERER_PRIORITY_IMPORTANT, true);
        }
        hudWebView.addJavascriptInterface(new HudAndroidBridge(hudWebView), "AndroidHUD");
        hudWebView.addJavascriptInterface(new SampAuthBridge(), "SampAuth");
        hudWebView.addJavascriptInterface(new SampInvBridge(), "SampInv");
        hudWebView.addJavascriptInterface(new SampAdminBridge(), "SampAdmin");
        hudWebView.addJavascriptInterface(new SampLockerBridge(), "SampLocker");
        hudWebView.addJavascriptInterface(new SampPoliceBridge(), "SampPolice");
        hudWebView.addJavascriptInterface(new SampShopBridge(), "SampShop");
        hudWebView.addJavascriptInterface(new SampJobBridge(), "SampJob");
        hudWebView.addJavascriptInterface(new SampUiBridge(), "SampUi");
        hudWebView.addJavascriptInterface(new SampSvcBridge(), "SampSvc");
        hudWebView.addJavascriptInterface(new SampMapsBridge(), "SampMaps");
        hudWebView.addJavascriptInterface(new SampJblBridge(), "SampJbl");
        hudWebView.addJavascriptInterface(new SampGovBridge(), "SampGov");
        hudWebView.addJavascriptInterface(new SampFactionBridge(), "SampFaction");
        hudWebView.addJavascriptInterface(new SampGangBridge(), "SampGang");
        hudWebView.addJavascriptInterface(new SampFishBridge(), "SampFish");
        hudWebView.addJavascriptInterface(new SampGarageBridge(), "SampGarage");
        hudWebView.addJavascriptInterface(new SampHouseBridge(), "SampHouse");
        hudWebView.addJavascriptInterface(new SampRpBridge(), "SampRp");
        hudWebView.addJavascriptInterface(new SampNpcBridge(), "SampNpc");
        hudWebView.addJavascriptInterface(new SampPhoneBridge(), "SampPhone");
        hudWebView.addJavascriptInterface(new SampReportBridge(), "SampReport");
        hudWebView.addJavascriptInterface(new SampHudBridge(hudWebView), "SampHud");
        hudWebView.addJavascriptInterface(new SampBankBridge(), "SampBank");
        hudWebView.addJavascriptInterface(new SampClothesBridge(hudWebView), "AndroidClothes");
        hudWebView.setVisibility(View.VISIBLE);
        return hudWebView;
    }

    /**
     * Navigation entre le HUD et les panneaux SANS recharger le HUD (avant : chaque fermeture de panneau
     * rechargeait hud/index.html = ecran vide + minimap reconstruite = saccade).
     */
    private void navigate(String url) {
        if (hudMain == null || url == null) return;
        if (url.equals(HUD_PAGE)) {
            boolean fromPanel = hudWebView != hudMain;
            hudWebView = hudMain;
            if (hudPanel != null && hudPanel.getVisibility() != View.GONE) {
                hudPanel.setVisibility(View.GONE);
                hudPanel.resetHudMode();
                hudPanel.setTouchBounds(null);
                hudPanel.loadUrl("about:blank");   // libere la page (musique, timers) ; le HUD reste intact
            }
            hudMain.resetHudMode();
            hudMain.setVisibility(View.VISIBLE);
            stopBackdrop();
            if (HUD_PAGE.equals(hudMainUrl)) {
                if (fromPanel) onHudShownAgain();
                return;
            }
            hudMainUrl = HUD_PAGE;
            hudMain.loadUrl(HUD_PAGE);
            return;
        }
        if (url.startsWith("about:")) { navigate(url); return; }
        if (hudPanel == null) { hudMainUrl = url; hudMain.loadUrl(url); return; }
        if (hudWebView == hudMain) {
            // les modes (plein ecran, telephone, camera) viennent d'etre regles sur la couche HUD : on les deplace
            hudPanel.copyModeFrom(hudMain);
            hudMain.resetHudMode();
        }
        hudWebView = hudPanel;
        hudPanel.setVisibility(View.VISIBLE);
        hudPanel.loadUrl(url);
        hudMain.setVisibility(View.INVISIBLE);   // reste charge (pas de rechargement a la fermeture)
        // fond flou seulement pour les panneaux plein ecran (pas les vetements / la camera / le telephone)
        if (hudPanel.isFullscreenPage()) startBackdrop(); else stopBackdrop();
    }

    // ---------------- fond flou des panneaux ----------------
    private void ensureBackdrop() {
        if (backdropView != null || hudMain == null) return;
        android.view.ViewParent parent = hudMain.getParent();
        if (!(parent instanceof android.view.ViewGroup)) return;
        android.view.ViewGroup group = (android.view.ViewGroup) parent;
        backdropView = new android.widget.ImageView(this);
        backdropView.setScaleType(android.widget.ImageView.ScaleType.FIT_XY);
        backdropView.setClickable(false);
        backdropView.setFocusable(false);
        backdropView.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        backdropView.setVisibility(View.GONE);
        // assombrit legerement le jeu pour que le texte des panneaux reste lisible
        backdropView.setColorFilter(0x66000000, android.graphics.PorterDuff.Mode.SRC_ATOP);
        if (android.os.Build.VERSION.SDK_INT >= 31) {
            backdropView.setRenderEffect(android.graphics.RenderEffect.createBlurEffect(
                    18f, 18f, android.graphics.Shader.TileMode.CLAMP));
        }
        // juste SOUS le HUD (donc sous les panneaux), au-dessus de la surface du jeu
        group.addView(backdropView, group.indexOfChild(hudMain), new FrameLayout.LayoutParams(-1, -1));
    }

    private void startBackdrop() {
        ensureBackdrop();
        if (backdropView == null) return;
        if (!backdropRunning) {
            backdropRunning = true;
            backdropHandler.removeCallbacks(backdropTick);
            backdropHandler.post(backdropTick);
        }
    }

    private void stopBackdrop() {
        backdropRunning = false;
        backdropHandler.removeCallbacks(backdropTick);
        if (backdropView != null) {
            backdropView.setVisibility(View.GONE);
            backdropView.setImageDrawable(null);
        }
    }

    private void captureBackdrop() {
        if (!backdropRunning || backdropView == null) return;
        if (backdropBusy) { backdropHandler.postDelayed(backdropTick, BACKDROP_INTERVAL_MS); return; }
        View gv = GetSurfaceView();
        if (!(gv instanceof SurfaceView) || gv.getWidth() <= 0 || gv.getHeight() <= 0
                || !((SurfaceView) gv).getHolder().getSurface().isValid()) {
            backdropHandler.postDelayed(backdropTick, 500);
            return;
        }
        // image 10x plus petite : copie rapide, et l'agrandissement la rend deja floue (anciens Android)
        final int div = android.os.Build.VERSION.SDK_INT >= 31 ? 6 : 10;
        final int w = Math.max(16, gv.getWidth() / div), h = Math.max(9, gv.getHeight() / div);
        final int slot = backdropIdx ^ 1;   // double tampon : on n'ecrit jamais dans l'image affichee
        android.graphics.Bitmap bmp = backdropBmp[slot];
        if (bmp == null || bmp.getWidth() != w || bmp.getHeight() != h) {
            bmp = android.graphics.Bitmap.createBitmap(w, h, android.graphics.Bitmap.Config.ARGB_8888);
            backdropBmp[slot] = bmp;
        }
        final android.graphics.Bitmap target = bmp;
        backdropBusy = true;
        try {
            android.view.PixelCopy.request((SurfaceView) gv, target, result -> {
                backdropBusy = false;
                if (!backdropRunning || backdropView == null) return;
                if (result == android.view.PixelCopy.SUCCESS) {
                    backdropIdx = slot;
                    backdropView.setImageBitmap(target);
                    if (backdropView.getVisibility() != View.VISIBLE) backdropView.setVisibility(View.VISIBLE);
                }
                backdropHandler.postDelayed(backdropTick, BACKDROP_INTERVAL_MS);
            }, backdropHandler);
        } catch (Throwable error) {
            backdropBusy = false;
            Log.w(TAG, "[panneau] capture du jeu impossible : " + error);
            backdropRunning = false;
        }
    }

    private void rebuildWebLayers() {
        if (isFinishing()) return;
        for (HudWebView v : new HudWebView[] { hudPanel, hudMain }) {
            if (v == null) continue;
            try {
                v.setMirror(null);
                android.view.ViewParent parent = v.getParent();
                if (parent instanceof android.view.ViewGroup) ((android.view.ViewGroup) parent).removeView(v);
                v.destroy();
            } catch (Throwable error) {
                CrashLog.log("[webview] destruction", error);
            }
        }
        stopBackdrop();
        if (backdropView != null && backdropView.getParent() instanceof android.view.ViewGroup) {
            ((android.view.ViewGroup) backdropView.getParent()).removeView(backdropView);
        }
        backdropView = null;
        hudWebView = null; hudMain = null; hudPanel = null; hudMainUrl = null;
        initializeHudWebView();
    }

    /** Le HUD redevient visible apres un panneau : on lui renvoie l'etat qui a pu changer. */
    private void onHudShownAgain() {
        if (hudMain == null) return;
        replayMapState();
        final String hot = lastHotbarJson;
        if (hot != null) hudMain.evaluateJavascript("window.CwHotbar && CwHotbar.set(" + JSONObject.quote(hot) + ")", null);
        final String obj = lastObjJson;
        if (obj != null) hudMain.evaluateJavascript("window.ObjTracker && ObjTracker.set(" + JSONObject.quote(obj) + ")", null);
        hudMain.evaluateJavascript("window.HudBridge && HudBridge.setVisible && HudBridge.setVisible(true)", null);
    }

    private String payloadValue(String payload) {
        if (payload == null) return "";
        String marker = "\"value\":\"";
        int start = payload.indexOf(marker);
        if (start < 0) return "";
        start += marker.length();
        int end = payload.indexOf('\"', start);
        return end > start ? payload.substring(start, end) : "";
    }

    private void applyGameSurfaceLayout(int bufferWidth, int bufferHeight) {
        // [32] la resolution du moteur 2.10 est geree par NvEventQueueActivity : reglage ignore ici.
        Log.i(TAG, "resolution demandee " + bufferWidth + "x" + bufferHeight + " (non geree en 32 bits)");
    }

    private void applyResolution(String value) {
        try {
            String[] parts = value.toLowerCase().split("x");
            if (parts.length != 2) return;
            int width = Integer.parseInt(parts[0].trim());
            int height = Integer.parseInt(parts[1].trim());
            if (width >= 640 && height >= 360) {
                applyGameSurfaceLayout(width, height);
                getSharedPreferences("display_settings", MODE_PRIVATE).edit()
                        .putString("resolution", value).apply();
            }
        } catch (Exception ignored) { }
    }

    private void applyResolutionScale(String value) {
        try {
            int percent = Math.max(50, Math.min(100, Integer.parseInt(value.trim())));
            android.util.DisplayMetrics metrics = getResources().getDisplayMetrics();
            int width = Math.max(640, Math.round(metrics.widthPixels * percent / 100f));
            int height = Math.max(360, Math.round(metrics.heightPixels * percent / 100f));
            applyGameSurfaceLayout(width, height);
            getSharedPreferences("display_settings", MODE_PRIVATE).edit()
                    .putInt("resolution_scale", percent).apply();
        } catch (Exception ignored) { }
    }

    private void applyAspect(String value) {
        try {
            String[] parts = value.split(":");
            if (parts.length != 2) return;
            float ratio = Float.parseFloat(parts[0]) / Float.parseFloat(parts[1]);
            android.util.DisplayMetrics metrics = getResources().getDisplayMetrics();
            int screenWidth = metrics.widthPixels;
            int screenHeight = metrics.heightPixels;
            int width = screenWidth;
            int height = Math.round(width / ratio);
            if (height > screenHeight) {
                height = screenHeight;
                width = Math.round(height * ratio);
            }
            applyGameSurfaceLayout(width, height);
            getSharedPreferences("display_settings", MODE_PRIVATE).edit()
                    .putString("aspect", value).apply();
        } catch (Exception ignored) { }
    }

    private void openConfig() {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            configOpen = true;
            hudWebView.setWheelOpen(false);
            hudWebView.setFullscreenPage(true);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/config/index.html?s=" + android.net.Uri.encode(gameSettingsJson()));
        });
    }

    private void openInteractionPage(final String assetPath) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            configOpen = true;
            pharmacyOpen = false;
            hudWebView.setWheelOpen(false);
            hudWebView.setPhonePage(false);
            hudWebView.setFullscreenPage(true);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/" + assetPath);
        });
    }

    private void openPhonePage() {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            phoneReady = false;
            configOpen = true;
            pharmacyOpen = false;
            hudWebView.setWheelOpen(false);
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(true);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/phone/index.html");
        });
    }

    public void openPharmacy(final int balance, final int hunger, final int thirst) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            pharmacyOpen = true;
            configOpen = false;
            pharmacyBalance = Math.max(0, balance);
            hudHunger = Math.max(0, Math.min(100, hunger));
            hudThirst = Math.max(0, Math.min(100, thirst));
            hudWebView.setWheelOpen(false);
            hudWebView.setFullscreenPage(true);
            hudWebView.setVisibility(View.VISIBLE);
            String url = "file:///android_asset/pharmacy/index.html?balance=" + Math.max(0, balance) + "&hunger=" + Math.max(0, hunger) + "&thirst=" + Math.max(0, thirst);
            navigate(url);
        });
    }

    private void closePharmacy() {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            pharmacyOpen = false;
            hudWebView.setFullscreenPage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            sendChatInput("/farmclose");
        });
    }

    // ---- Inventaire (page assets/inventory/index.html) ----
    private void handleInvDialog(final int dialogId, final String rawJson) {
        String tmp = rawJson == null ? "{}" : rawJson;
        int js = tmp.indexOf('{');
        int je = tmp.lastIndexOf('}');
        final String json = (js >= 0 && je > js) ? tmp.substring(js, je + 1) : "{}";
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9532) {            // fermeture demandee par le serveur
                closeInvPage(false);
            } else if (dialogId == 9530) {     // ouverture
                openInvPage(json);
            } else if (invOpen) {              // 9531 : mise a jour (objets, message)
                hudWebView.evaluateJavascript("window.InvApp && InvApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openInvPage(String json) {
        if (hudWebView == null || authOpen) return;
        if (invOpen && invReady) {
            hudWebView.evaluateJavascript("window.InvApp && InvApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        Log.i(TAG, "[inv] ouverture de l'inventaire");
        invOpen = true;
        invReady = false;
        configOpen = true;
        bankOpen = false;
        pharmacyOpen = false;
        pendingInvJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/inventory/index.html");
        if (invWatchdog != null) authHandler.removeCallbacks(invWatchdog);
        invWatchdog = () -> {
            if (invOpen && !invReady) {
                Log.e(TAG, "[inv] la page ne s'est pas affichee a temps -> fermeture de secours");
                closeInvPage(true);
            }
        };
        authHandler.postDelayed(invWatchdog, 5000);
    }

    private void closeInvPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = invOpen;
            if (invWatchdog != null) { authHandler.removeCallbacks(invWatchdog); invWatchdog = null; }
            invOpen = false;
            invReady = false;
            configOpen = false;
            pendingInvJson = null;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/invact close x 0");
        });
    }

    private final class SampInvBridge {
        // JS -> serveur : /invact <use|drop> <objet> <quantite>
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                String item = o.optString("item", "").replaceAll("[^a-z_]", "");
                long qty = Math.max(1L, Math.min(99L, o.optLong("qty", 1)));
                if (item.length() > 23) return;
                switch (t) {
                    case "refresh":   // [v2] inventaire : actualiser (sol / joueurs proches)
                        sendHudCommand("/invact refresh");
                        return;
                    case "pick": {    // ramasser l'objet au sol n
                        long gid = Math.max(0L, Math.min(9999L, o.optLong("gid", -1)));
                        sendHudCommand("/invact pick " + gid);
                        return;
                    }
                    case "slot": {    // emplacement rapide 1..5 <- objet (ou vide)
                        long slot = Math.max(1L, Math.min(5L, o.optLong("slot", 1)));
                        sendHudCommand("/invact slot " + slot + " " + (item.isEmpty() ? "none" : item));
                        return;
                    }
                    case "give": {    // donner a un joueur proche
                        long to = Math.max(0L, Math.min(999L, o.optLong("to", -1)));
                        if (item.isEmpty()) return;
                        sendHudCommand("/invact give " + item + " " + qty + " " + to);
                        return;
                    }
                    case "use": case "drop":
                        if (item.isEmpty()) return;
                        sendHudCommand("/invact " + t + " " + item + " " + qty);
                        return;
                    default:
                        return;
                }
            } catch (Exception e) {
                Log.e(TAG, "inv action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeInvPage(true);
        }

        @JavascriptInterface public void ready() {
            invReady = true;
            Log.i(TAG, "[inv] page affichee");
        }
    }

    // ---- Administration (page assets/admin/index.html) ----
    private void handleAdminDialog(final int dialogId, final String rawJson) {
        String tmp = rawJson == null ? "{}" : rawJson;
        int js = tmp.indexOf('{');
        int je = tmp.lastIndexOf('}');
        final String json = (js >= 0 && je > js) ? tmp.substring(js, je + 1) : "{}";
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9542) {            // fermeture demandee par le serveur
                closeAdminPage(false);
            } else if (dialogId == 9540) {     // ouverture (/adminpanel)
                openAdminPage(json);
            } else if (adminOpen) {            // 9541 : mise a jour (joueurs, message)
                hudWebView.evaluateJavascript("window.AdminApp && AdminApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openAdminPage(String json) {
        if (hudWebView == null || authOpen) return;
        if (adminOpen && adminReady) {
            hudWebView.evaluateJavascript("window.AdminApp && AdminApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        Log.i(TAG, "[admin] ouverture du panneau admin");
        adminOpen = true;
        adminReady = false;
        configOpen = true;
        invOpen = false;
        bankOpen = false;
        pharmacyOpen = false;
        pendingAdminJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/admin/index.html");
        if (adminWatchdog != null) authHandler.removeCallbacks(adminWatchdog);
        adminWatchdog = () -> {
            if (adminOpen && !adminReady) {
                Log.e(TAG, "[admin] la page ne s'est pas affichee a temps -> fermeture de secours");
                closeAdminPage(true);
            }
        };
        authHandler.postDelayed(adminWatchdog, 5000);
    }

    private void closeAdminPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = adminOpen;
            if (adminWatchdog != null) { authHandler.removeCallbacks(adminWatchdog); adminWatchdog = null; }
            adminOpen = false;
            adminReady = false;
            configOpen = false;
            pendingAdminJson = null;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/adminact close 0");
        });
    }

    private final class SampAdminBridge {
        // JS -> serveur : /adminact <action> <id> <texte>. Le serveur re-verifie le niveau a chaque action.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                if (t.isEmpty() || t.length() > 12) return;
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                long id = Math.max(0L, Math.min(9999L, o.optLong("id", 0)));
                String arg = o.optString("arg", "")
                        .replaceAll("[\\p{Cntrl}\"\\\\%]", " ")
                        .trim();
                if (arg.length() > 90) arg = arg.substring(0, 90);
                sendHudCommand("/adminact " + t + " " + id + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "admin action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeAdminPage(true);
        }

        @JavascriptInterface public void ready() {
            adminReady = true;
            Log.i(TAG, "[admin] page affichee");
        }
    }

    // ---- Actions de police (page assets/police/index.html) ----
    // Le bouton "Police" du HUD (icone bouclier) apparait quand un policier est a cote d'un joueur ; un appui
    // envoie /interagir, le serveur ouvre alors cette page (dialogue 9570) avec la cible la plus proche.
    private void handlePoliceDialog(final int dialogId, final String rawJson) {
        String tmp = rawJson == null ? "{}" : rawJson;
        int js = tmp.indexOf('{');
        int je = tmp.lastIndexOf('}');
        final String json = (js >= 0 && je > js) ? tmp.substring(js, je + 1) : "{}";
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9572) {            // fermeture demandee par le serveur (cible perdue, menotte...)
                closePolicePage(false);
            } else if (dialogId == 9570) {     // ouverture
                openPolicePage(json);
            } else if (policeOpen) {           // 9571 : mise a jour (etat de la cible, vue, message)
                hudWebView.evaluateJavascript("window.PoliceApp && PoliceApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openPolicePage(String json) {
        if (hudWebView == null || authOpen || adminOpen || lockerOpen) return;
        if (policeOpen && policeReady) {
            hudWebView.evaluateJavascript("window.PoliceApp && PoliceApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        Log.i(TAG, "[police] ouverture du menu d'actions");
        policeOpen = true;
        policeReady = false;
        configOpen = true;
        invOpen = false;
        bankOpen = false;
        pharmacyOpen = false;
        pendingPoliceJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/police/index.html");
        if (policeWatchdog != null) authHandler.removeCallbacks(policeWatchdog);
        policeWatchdog = () -> {
            if (policeOpen && !policeReady) {
                Log.e(TAG, "[police] la page ne s'est pas affichee a temps -> fermeture de secours");
                closePolicePage(true);
            }
        };
        authHandler.postDelayed(policeWatchdog, 5000);
    }

    private void closePolicePage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = policeOpen;
            if (policeWatchdog != null) { authHandler.removeCallbacks(policeWatchdog); policeWatchdog = null; }
            policeOpen = false;
            policeReady = false;
            configOpen = false;
            pendingPoliceJson = null;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/policeact close");
        });
    }

    private final class SampPoliceBridge {
        // JS -> serveur : /policeact <action> [arguments numeriques]. Le serveur re-verifie faction, grade,
        // distance et etat de la cible a chaque action : seule une liste blanche d'actions est transmise.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                switch (t) {
                    case "refresh": case "id": case "papers": case "search":
                    case "cuff": case "uncuff": case "drag": case "undrag":
                    case "licwd": case "licret": case "charge": case "fine": case "arrest":
                        break;
                    default:
                        return;
                }
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                String arg = o.optString("arg", "").replaceAll("[^0-9 ]", "").trim();
                if (arg.length() > 20) arg = arg.substring(0, 20);
                sendHudCommand("/policeact " + t + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "police action", e);
            }
        }

        @JavascriptInterface public void close() {
            closePolicePage(true);
        }

        @JavascriptInterface public void ready() {
            policeReady = true;
            runOnUiThread(() -> { if (policeWatchdog != null) { authHandler.removeCallbacks(policeWatchdog); policeWatchdog = null; } });
            Log.i(TAG, "[police] page affichee");
        }
    }

    // ---- Carte : icones fixes + GPS (minimap du HUD et page assets/map/index.html) ----
    private void handleMapDialog(final int dialogId, final String raw) {
        if (dialogId == 9605) {
            int s0 = raw.indexOf('['), s1 = raw.lastIndexOf(']');
            try {
                lastBlipsJson = new org.json.JSONArray((s0 >= 0 && s1 > s0) ? raw.substring(s0, s1 + 1) : "[]").toString();
            } catch (Exception e) {
                Log.e(TAG, "[blips] JSON invalide", e);
                return;
            }
        } else {
            try {
                int s0 = raw.indexOf('{'), s1 = raw.lastIndexOf('}');
                JSONObject o = new JSONObject((s0 >= 0 && s1 > s0) ? raw.substring(s0, s1 + 1) : "{}");
                if (o.has("x") && o.has("y")) {
                    JSONObject g = new JSONObject();
                    g.put("x", o.getDouble("x"));
                    g.put("y", o.getDouble("y"));
                    g.put("auto", 0);
                    lastGpsJson = g.toString();
                } else {
                    lastGpsJson = null;
                }
            } catch (Exception e) {
                Log.e(TAG, "[gps] JSON invalide", e);
                return;
            }
        }
        replayMapState();
    }

    /** Pousse les icones, le GPS et la position connus vers la page affichee (HUD ou carte). */
    private void replayMapState() {
        final String blips = lastBlipsJson;
        final String gps = lastGpsJson;
        StringBuilder js = new StringBuilder();
        if (blips != null) {
            js.append("window.MinimapBridge && MinimapBridge.setBlips(").append(blips).append(");");
            js.append("window.MapApp && MapApp.setBlips && MapApp.setBlips(").append(blips).append(");");
        }
        if (gps != null) {
            js.append("(function(g){window.MinimapBridge && MinimapBridge.setWaypoint(g.x, g.y, g.auto === 1);")
              .append("window.MapApp && MapApp.setGps && MapApp.setGps(g.x, g.y);})(").append(gps).append(");");
        } else {
            js.append("window.MinimapBridge && MinimapBridge.clearWaypoint();");
            js.append("window.MapApp && MapApp.clearGps && MapApp.clearGps();");
        }
        if (hasLastPos) {
            js.append("window.MapApp && MapApp.setPlayer(").append(lastPosX).append(",").append(lastPosY).append(");");
        }
        final String script = js.toString();
        runOnUiThread(() -> {
            if (hudWebView != null && !isFinishing()) hudWebView.evaluateJavascript(script, null);
        });
    }

    // ---- Support / reports (page assets/report/index.html + onglet Reports du panneau admin) ----
    private void handleSupportDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        if (dialogId == 9650) {                        // objectifs de mission
            boolean show = json.contains("\"show\":1");
            lastObjJson = show ? json : null;
            hudJs("window.ObjTracker && ObjTracker.set(" + JSONObject.quote(json) + ")");
            return;
        }
        if (dialogId == 9644) {                        // notification en bas de l'ecran
            hudJs("window.HudApp && HudApp.receiveGameNotification && HudApp.receiveGameNotification(" + JSONObject.quote(json) + ")");
            hudJs("window.ReportApp && ReportApp.onNote && ReportApp.onNote(" + JSONObject.quote(json) + ")");
            return;
        }
        if (dialogId == 9643) {                        // panneau admin : liste des reports + conversation
            runOnUiThread(() -> {
                if (adminOpen && hudWebView != null && !isFinishing()) {
                    hudWebView.evaluateJavascript("window.AdminApp && AdminApp.onReports && AdminApp.onReports(" + JSONObject.quote(json) + ")", null);
                }
            });
            return;
        }
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9642) {
                closeReportPage(false);
            } else if (dialogId == 9640) {
                openReportPage(json);
            } else if (reportOpen) {
                hudWebView.evaluateJavascript("window.ReportApp && ReportApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openReportPage(String json) {
        if (hudWebView == null || authOpen) return;
        if (reportOpen && reportReady) {
            hudWebView.evaluateJavascript("window.ReportApp && ReportApp.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        if (reportOpen) {                              // page en cours de chargement : on garde le dernier etat
            pendingReportJson = json;
            return;
        }
        Log.i(TAG, "[report] ouverture du support");
        reportOpen = true;
        reportReady = false;
        adminOpen = false;
        uiOpen = false;
        shopOpen = false;
        jobOpen = false;
        lockerOpen = false;
        policeOpen = false;
        configOpen = true;
        invOpen = false;
        bankOpen = false;
        pharmacyOpen = false;
        pendingReportJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/report/index.html");
        if (reportWatchdog != null) authHandler.removeCallbacks(reportWatchdog);
        reportWatchdog = () -> {
            if (reportOpen && !reportReady) {
                Log.e(TAG, "[report] la page ne s'est pas affichee a temps -> fermeture de secours");
                closeReportPage(true);
            }
        };
        authHandler.postDelayed(reportWatchdog, 5000);
    }

    private void closeReportPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = reportOpen;
            if (reportWatchdog != null) { authHandler.removeCallbacks(reportWatchdog); reportWatchdog = null; }
            reportOpen = false;
            reportReady = false;
            configOpen = false;
            pendingReportJson = null;
            hudWebView.hideKeyboard();
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/repact hide");
        });
    }

    /** Texte libre d'un message de support : une ligne, sans guillemet / antislash / %, 100 caracteres max. */
    private static String cleanSupportText(String value) {
        if (value == null) return "";
        String v = value.replaceAll("[\\p{Cntrl}\"\\\\%]", " ").replaceAll("\\s+", " ").trim();
        return v.length() > 100 ? v.substring(0, 100).trim() : v;
    }

    private final class SampReportBridge {
        // JS -> serveur : /repact new <categorie> <id> <texte> | msg <texte> | cancel | open
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                switch (t) {
                    case "new": {
                        long cat = Math.max(0L, Math.min(4L, o.optLong("cat", 4)));
                        long target = Math.max(-1L, Math.min(999L, o.optLong("target", -1)));
                        String text = cleanSupportText(o.optString("text", ""));
                        if (text.isEmpty()) return;
                        sendHudCommand("/repact new " + cat + " " + target + " " + text);
                        break;
                    }
                    case "msg": {
                        String text = cleanSupportText(o.optString("text", ""));
                        if (!text.isEmpty()) sendHudCommand("/repact msg " + text);
                        break;
                    }
                    case "cancel":
                    case "open":
                        sendHudCommand("/repact " + t);
                        break;
                    default:
                        break;
                }
            } catch (Exception e) {
                Log.e(TAG, "report action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeReportPage(true);
        }

        @JavascriptInterface public void showKeyboard() {
            runOnUiThread(() -> { if (hudWebView != null) hudWebView.showKeyboard(); });
        }

        @JavascriptInterface public void hideKeyboard() {
            runOnUiThread(() -> { if (hudWebView != null) hudWebView.hideKeyboard(); });
        }

        @JavascriptInterface public void ready() {
            reportReady = true;
            runOnUiThread(() -> {
                if (reportWatchdog != null) { authHandler.removeCallbacks(reportWatchdog); reportWatchdog = null; }
                // etat arrive pendant le chargement de la page
                String pending = pendingReportJson;
                pendingReportJson = null;
                if (pending != null && reportOpen && hudWebView != null) {
                    hudWebView.evaluateJavascript("window.ReportApp && ReportApp.openFromNative(" + JSONObject.quote(pending) + ")", null);
                }
            });
            Log.i(TAG, "[report] page affichee");
        }
    }

    // ---- Boutique 24/7 (page assets/shop/index.html) ----
    private static String extractJson(String raw) {
        String tmp = raw == null ? "{}" : raw;
        int js = tmp.indexOf('{');
        int je = tmp.lastIndexOf('}');
        return (js >= 0 && je > js) ? tmp.substring(js, je + 1) : "{}";
    }

    private void handleShopDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9592) {
                closeShopPage(false);
            } else if (dialogId == 9590) {
                openShopPage(json);
            } else if (shopOpen) {
                hudWebView.evaluateJavascript("window.ShopApp && ShopApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openShopPage(String json) {
        if (hudWebView == null || authOpen || adminOpen) return;
        if (shopOpen && shopReady) {
            hudWebView.evaluateJavascript("window.ShopApp && ShopApp.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        Log.i(TAG, "[shop] ouverture de la boutique");
        shopOpen = true;
        shopReady = false;
        jobOpen = false;
        lockerOpen = false;
        policeOpen = false;
        configOpen = true;
        invOpen = false;
        bankOpen = false;
        pharmacyOpen = false;
        pendingShopJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/shop/index.html");
        if (shopWatchdog != null) authHandler.removeCallbacks(shopWatchdog);
        shopWatchdog = () -> {
            if (shopOpen && !shopReady) {
                Log.e(TAG, "[shop] la page ne s'est pas affichee a temps -> fermeture de secours");
                closeShopPage(true);
            }
        };
        authHandler.postDelayed(shopWatchdog, 5000);
    }

    private void closeShopPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = shopOpen;
            if (shopWatchdog != null) { authHandler.removeCallbacks(shopWatchdog); shopWatchdog = null; }
            shopOpen = false;
            shopReady = false;
            configOpen = false;
            pendingShopJson = null;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/shopact close");
        });
    }

    private final class SampShopBridge {
        // JS -> serveur : /shopact buy <article> <quantite> <cash|card> | refresh. Le serveur re-verifie tout.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                if (!t.equals("buy") && !t.equals("refresh")) return;
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                String arg = o.optString("arg", "").replaceAll("[^a-z0-9_ ]", "").trim();
                if (arg.length() > 48) arg = arg.substring(0, 48);
                sendHudCommand("/shopact " + t + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "shop action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeShopPage(true);
        }

        @JavascriptInterface public void ready() {
            shopReady = true;
            Log.i(TAG, "[shop] page affichee");
        }
    }

    // ---- Enceinte JBL (page assets/jbl/index.html) ----
    private void handleJblDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9852) {
                closeJblPage(false);
            } else if (dialogId == 9850) {
                openJblPage(json);
            } else if (jblOpen) {
                hudWebView.evaluateJavascript("window.JblApp && JblApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openJblPage(String json) {
        if (hudWebView == null || authOpen || adminOpen) return;
        String curUrl = hudWebView.getUrl();
        if (jblOpen && jblReady && curUrl != null && curUrl.endsWith("jbl/index.html")) {
            hudWebView.evaluateJavascript("window.JblApp && JblApp.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        jblOpen = true;
        jblReady = false;
        mapsOpen = false;
        svcOpen = false;
        uiOpen = false;
        shopOpen = false;
        invOpen = false;
        bankOpen = false;
        configOpen = true;
        pendingJblJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/jbl/index.html");
        if (jblWatchdog != null) authHandler.removeCallbacks(jblWatchdog);
        jblWatchdog = () -> { if (jblOpen && !jblReady) closeJblPage(true); };
        authHandler.postDelayed(jblWatchdog, 5000);
    }

    private void closeJblPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = jblOpen;
            if (jblWatchdog != null) { authHandler.removeCallbacks(jblWatchdog); jblWatchdog = null; }
            jblOpen = false;
            jblReady = false;
            configOpen = false;
            pendingJblJson = null;
            hudWebView.hideKeyboard();
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/jblact close");
        });
    }

    private final class SampJblBridge {
        // JS -> serveur : /jblact take | play <lien> | stop | drop | store. Le serveur verifie l'objet et le lien.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                if (!t.equals("take") && !t.equals("play") && !t.equals("stop") && !t.equals("drop") && !t.equals("store")) return;
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                String arg = o.optString("arg", "").replaceAll("[^A-Za-z0-9:/._?=&%~+#@!,;-]", "");
                if (arg.length() > 110) return;   // trop long pour une commande SA-MP
                sendHudCommand("/jblact " + t + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "jbl action", e);
            }
        }
        @JavascriptInterface public void close() { closeJblPage(true); }
        @JavascriptInterface public void showKeyboard() {
            runOnUiThread(() -> { if (hudWebView != null) hudWebView.showKeyboard(); });
        }
        @JavascriptInterface public void hideKeyboard() {
            runOnUiThread(() -> { if (hudWebView != null) hudWebView.hideKeyboard(); });
        }
        @JavascriptInterface public void ready() {
            jblReady = true;
            runOnUiThread(() -> { if (jblWatchdog != null) { authHandler.removeCallbacks(jblWatchdog); jblWatchdog = null; } });
        }
    }

    // ---- Panneau admin : mappings, portails, points (page assets/mapadmin/index.html) ----
    private void handleMapsDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9832) {
                closeMapsPage(false);
            } else if (dialogId == 9830) {
                openMapsPage(json);
            } else if (mapsOpen) {
                hudWebView.evaluateJavascript("window.MapAdmin && MapAdmin.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openMapsPage(String json) {
        if (hudWebView == null || authOpen || adminOpen) return;
        String curUrl = hudWebView.getUrl();
        if (mapsOpen && mapsReady && curUrl != null && curUrl.endsWith("mapadmin/index.html")) {
            hudWebView.evaluateJavascript("window.MapAdmin && MapAdmin.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        mapsOpen = true;
        mapsReady = false;
        svcOpen = false;
        uiOpen = false;
        shopOpen = false;
        invOpen = false;
        bankOpen = false;
        configOpen = true;
        pendingMapsJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/mapadmin/index.html");
        if (mapsWatchdog != null) authHandler.removeCallbacks(mapsWatchdog);
        mapsWatchdog = () -> { if (mapsOpen && !mapsReady) closeMapsPage(true); };
        authHandler.postDelayed(mapsWatchdog, 5000);
    }

    private void closeMapsPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = mapsOpen;
            if (mapsWatchdog != null) { authHandler.removeCallbacks(mapsWatchdog); mapsWatchdog = null; }
            mapsOpen = false;
            mapsReady = false;
            configOpen = false;
            pendingMapsJson = null;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/mapact close");
        });
    }

    private final class SampMapsBridge {
        // JS -> serveur : /mapact <action> <arguments>. Le serveur verifie le niveau admin et tout le reste.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                switch (t) {
                    case "view": case "maptp": case "maparr": case "pnew": case "pout": case "pin": case "ptpout":
                    case "ptpin": case "pdel": case "ptnew": case "ptdel": case "pttp":
                        break;
                    default:
                        return;
                }
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                String arg = o.optString("arg", "").replaceAll("[^A-Za-z0-9 ./-]", "").trim();
                if (arg.length() > 48) arg = arg.substring(0, 48);
                sendHudCommand("/mapact " + t + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "maps action", e);
            }
        }
        @JavascriptInterface public void close() { closeMapsPage(true); }
        @JavascriptInterface public void showKeyboard() {
            runOnUiThread(() -> { if (hudWebView != null) hudWebView.showKeyboard(); });
        }
        @JavascriptInterface public void hideKeyboard() {
            runOnUiThread(() -> { if (hudWebView != null) hudWebView.hideKeyboard(); });
        }
        @JavascriptInterface public void ready() {
            mapsReady = true;
            runOnUiThread(() -> { if (mapsWatchdog != null) { authHandler.removeCallbacks(mapsWatchdog); mapsWatchdog = null; } });
        }
    }

    // ---- Services : station-service, location (page assets/svc/index.html) ----
    private void handleSvcDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9812) {
                closeSvcPage(false);
            } else if (dialogId == 9810) {
                openSvcPage(json);
            } else if (svcOpen) {
                hudWebView.evaluateJavascript("window.SvcApp && SvcApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openSvcPage(String json) {
        if (hudWebView == null || authOpen || adminOpen) return;
        String curUrl = hudWebView.getUrl();
        if (svcOpen && svcReady && curUrl != null && curUrl.endsWith("svc/index.html")) {
            hudWebView.evaluateJavascript("window.SvcApp && SvcApp.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        svcOpen = true;
        svcReady = false;
        uiOpen = false;
        shopOpen = false;
        invOpen = false;
        bankOpen = false;
        configOpen = true;
        pendingSvcJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/svc/index.html");
        if (svcWatchdog != null) authHandler.removeCallbacks(svcWatchdog);
        svcWatchdog = () -> { if (svcOpen && !svcReady) closeSvcPage(true); };
        authHandler.postDelayed(svcWatchdog, 5000);
    }

    private void closeSvcPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = svcOpen;
            if (svcWatchdog != null) { authHandler.removeCallbacks(svcWatchdog); svcWatchdog = null; }
            svcOpen = false;
            svcReady = false;
            configOpen = false;
            pendingSvcJson = null;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/svcact close");
        });
    }

    private final class SampSvcBridge {
        // JS -> serveur : /svcact <fill PCT cash|bank | rent MODELE cash|bank | rentend>. Le serveur re-verifie tout.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                if (!t.equals("fill") && !t.equals("rent") && !t.equals("rentend")) return;
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                String arg = o.optString("arg", "").replaceAll("[^a-z0-9 ]", "").trim();
                if (arg.length() > 20) arg = arg.substring(0, 20);
                sendHudCommand("/svcact " + t + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "svc action", e);
            }
        }
        @JavascriptInterface public void close() { closeSvcPage(true); }
        @JavascriptInterface public void ready() { svcReady = true; }
    }

    // ---- Interfaces joueur (page assets/ui/index.html) ----
    private void handleUiDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9632) {
                closeUiPage(false);
            } else if (dialogId == 9630) {
                openUiPage(json);
            } else if (uiOpen) {
                hudWebView.evaluateJavascript("window.UiApp && UiApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openUiPage(String json) {
        if (hudWebView == null || authOpen || adminOpen) return;
        String curUrl = hudWebView.getUrl();
        if (uiOpen && uiReady && curUrl != null && curUrl.endsWith("ui/index.html")) {
            hudWebView.evaluateJavascript("window.UiApp && UiApp.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        Log.i(TAG, "[ui] ouverture d'une interface joueur");
        uiOpen = true;
        uiReady = false;
        govOpen = false;
        facOpen = false;
        gangOpen = false;
        shopOpen = false;
        jobOpen = false;
        lockerOpen = false;
        policeOpen = false;
        configOpen = true;
        invOpen = false;
        bankOpen = false;
        pharmacyOpen = false;
        pendingUiJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/ui/index.html");
        if (uiWatchdog != null) authHandler.removeCallbacks(uiWatchdog);
        uiWatchdog = () -> {
            if (uiOpen && !uiReady) {
                Log.e(TAG, "[ui] la page ne s'est pas affichee a temps -> fermeture de secours");
                closeUiPage(true);
            }
        };
        authHandler.postDelayed(uiWatchdog, 5000);
    }

    private void closeUiPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = uiOpen;
            if (uiWatchdog != null) { authHandler.removeCallbacks(uiWatchdog); uiWatchdog = null; }
            uiOpen = false;
            uiReady = false;
            configOpen = false;
            pendingUiJson = null;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/uiact close");
        });
    }

    private final class SampUiBridge {
        // JS -> serveur : /uiact <action>. Le serveur re-verifie distance, faction et vehicule.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z_]", "");
                if (t.isEmpty() || t.length() > 16) return;
                sendHudCommand("/uiact " + t);
            } catch (Exception e) {
                Log.e(TAG, "ui action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeUiPage(true);
        }

        @JavascriptInterface public void ready() {
            uiReady = true;
            Log.i(TAG, "[ui] page affichee");
        }
    }

    // ---- Panneau du gouvernement (page assets/gov/index.html) ----
    private void handleGovDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9662) {
                closeGovPage(false);
            } else if (dialogId == 9660) {
                openGovPage(json);
            } else if (govOpen) {
                hudWebView.evaluateJavascript("window.GovApp && GovApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openGovPage(String json) {
        if (hudWebView == null || authOpen || adminOpen) return;
        String curUrl = hudWebView.getUrl();
        if (govOpen && govReady && curUrl != null && curUrl.endsWith("gov/index.html")) {
            hudWebView.evaluateJavascript("window.GovApp && GovApp.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        Log.i(TAG, "[gov] ouverture du panneau du gouvernement");
        govOpen = true;
        govReady = false;
        facOpen = false;
        gangOpen = false;
        uiOpen = false;
        shopOpen = false;
        jobOpen = false;
        lockerOpen = false;
        policeOpen = false;
        configOpen = true;
        invOpen = false;
        bankOpen = false;
        pharmacyOpen = false;
        pendingGovJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/gov/index.html");
        if (govWatchdog != null) authHandler.removeCallbacks(govWatchdog);
        govWatchdog = () -> {
            if (govOpen && !govReady) {
                Log.e(TAG, "[gov] la page ne s'est pas affichee a temps -> fermeture de secours");
                closeGovPage(true);
            }
        };
        authHandler.postDelayed(govWatchdog, 5000);
    }

    private void closeGovPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = govOpen;
            if (govWatchdog != null) { authHandler.removeCallbacks(govWatchdog); govWatchdog = null; }
            govOpen = false;
            govReady = false;
            pendingGovJson = null;
            String curUrl = hudWebView.getUrl();
            // une autre page a pu s'ouvrir entre-temps : on ne la ferme pas
            if (!wasOpen && (curUrl == null || !curUrl.endsWith("gov/index.html"))) return;
            configOpen = false;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/govact close");
        });
    }

    private final class SampGovBridge {
        // JS -> serveur : /govact <action> [argument]. Liste blanche d'actions ; le serveur re-verifie
        // faction, grade, distance du citoyen et montants a chaque action.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                String arg;
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                switch (t) {
                    case "refresh":
                        arg = "";
                        break;
                    case "licgive": case "lictake": case "subsidy": case "tax":
                        arg = o.optString("arg", "").replaceAll("[^0-9]", "");
                        if (arg.length() > 9) arg = arg.substring(0, 9);
                        break;
                    case "announce":
                        arg = o.optString("arg", "").replaceAll("[\\r\\n\\t%\"\\\\{}]", " ").replaceAll("\\s+", " ").trim();
                        if (arg.length() > 100) arg = arg.substring(0, 100);
                        break;
                    default:
                        return;
                }
                sendHudCommand("/govact " + t + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "gov action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeGovPage(true);
        }

        @JavascriptInterface public void ready() {
            govReady = true;
            runOnUiThread(() -> { if (govWatchdog != null) { authHandler.removeCallbacks(govWatchdog); govWatchdog = null; } });
            Log.i(TAG, "[gov] page affichee");
        }
    }

    // ---- Telephone : evenements du serveur (dialogue 9700 "@phone") ----
    private void handlePhoneDialog(final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            String url = hudWebView.getUrl();
            if (phoneReady && url != null && url.contains("phone/index.html")) {
                hudWebView.evaluateJavascript("window.PhoneNet && PhoneNet.onNative(" + JSONObject.quote(json) + ")", null);
                return;
            }
            // telephone ferme : notification sur le HUD (appel entrant avec decrocher / refuser, SMS, Wave recu)
            try {
                JSONObject o = new JSONObject(json);
                String ev = o.optString("ev", "");
                JSONObject n = new JSONObject();
                if ("call".equals(ev) && "incoming".equals(o.optString("st"))) {
                    n.put("type", "call"); n.put("status", "incoming");
                    n.put("name", o.optString("name", "")); n.put("from", o.optString("num", ""));
                } else if ("sms".equals(ev)) {
                    n.put("type", "message"); n.put("name", o.optString("name", ""));
                    n.put("from", o.optString("from", "")); n.put("text", o.optString("text", ""));
                } else if ("wave".equals(ev) && "in".equals(o.optString("st"))) {
                    String nm = o.optString("name", "");
                    n.put("type", "bank_txn"); n.put("label", nm.isEmpty() ? o.optString("num", "") : nm); n.put("amount", o.optInt("amount", 0));
                } else {
                    return;
                }
                if (url != null && url.contains("hud/index.html")) {
                    hudWebView.evaluateJavascript("window.HudApp && HudApp.showPhoneNotification && HudApp.showPhoneNotification(" + n + ")", null);
                }
            } catch (Exception e) {
                Log.e(TAG, "phone event", e);
            }
        });
    }

    /** Texte libre du telephone : ni retour a la ligne ni guillemets droits (JSON / SQL du serveur). */
    private static String phoneText(String s, int max) {
        if (s == null) return "";
        String t = s.replaceAll("[\\r\\n\\t]", " ").replace('"', '”').replace('\'', '’').replace('\\', '/')
                .replace('{', '(').replace('}', ')').replaceAll("\\s+", " ").trim();
        return t.length() > max ? t.substring(0, max) : t;
    }

    private final class SampPhoneBridge {
        // JS -> serveur : /phoneact <action> [arguments]. Le serveur re-verifie (telephone possede, numero, solde...).
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                String num = o.optString("num", "").replaceAll("[^0-9]", "");
                if (num.length() > 9) num = num.substring(0, 9);
                String cmd;
                switch (t) {
                    case "sync": case "convs": case "recents": case "accept": case "decline": case "hangup": case "soscancel":
                        cmd = t;
                        break;
                    case "conv": case "call":
                        if (num.isEmpty()) return;
                        cmd = t + " " + num;
                        break;
                    case "sms": {
                        String text = phoneText(o.optString("text", ""), 96);
                        if (num.isEmpty() || text.isEmpty()) return;
                        cmd = "sms " + num + " " + text;
                        break;
                    }
                    case "addc": {
                        String name = phoneText(o.optString("name", ""), 24);
                        if (num.isEmpty() || name.isEmpty()) return;
                        cmd = "addc " + num + " " + name;
                        break;
                    }
                    case "delc": {
                        String id = o.optString("id", "").replaceAll("[^0-9]", "");
                        if (id.isEmpty() || id.length() > 9) return;
                        cmd = "delc " + id;
                        break;
                    }
                    case "sos": {
                        String svc = o.optString("svc", "").replaceAll("[^a-z]", "");
                        String text = phoneText(o.optString("text", ""), 64);
                        if (svc.isEmpty() || svc.length() > 8 || text.isEmpty()) return;
                        cmd = "sos " + svc + " " + text;
                        break;
                    }
                    case "taxi": {
                        String text = phoneText(o.optString("text", ""), 64);
                        if (text.isEmpty()) return;
                        cmd = "taxi " + text;
                        break;
                    }
                    case "wave": {
                        String amount = o.optString("amount", "").replaceAll("[^0-9]", "");
                        if (num.isEmpty() || amount.isEmpty() || amount.length() > 7) return;
                        cmd = "wave " + num + " " + amount;
                        break;
                    }
                    case "camera":
                        openPhoneCamera();
                        return;
                    case "close":
                        closeConfig();
                        return;
                    default:
                        return;
                }
                sendHudCommand("/phoneact " + cmd);
            } catch (Exception e) {
                Log.e(TAG, "phone action", e);
            }
        }

        @JavascriptInterface public void ready() {
            phoneReady = true;
            sendHudCommand("/phoneact sync");
        }
    }

    // ---- Menu RP (page assets/rp/index.html) ----
    private void handleRpDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9692) {
                closeRpPage(false);
            } else if (dialogId == 9690) {
                // anti-figeage : si le joueur vient de fermer la page RP, on ignore une reouverture automatique du serveur
                if (!rpOpen && System.currentTimeMillis() - rpClosedAtMs < 2000) {
                    Log.i(TAG, "[rp] reouverture serveur ignoree (page fermee par le joueur)");
                    return;
                }
                openRpPage(json);
            } else if (rpOpen) {
                hudWebView.evaluateJavascript("window.RpApp && RpApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openRpPage(String json) {
        if (hudWebView == null || authOpen || adminOpen) return;
        String curUrl = hudWebView.getUrl();
        if (rpOpen && rpReady && curUrl != null && curUrl.endsWith("rp/index.html")) {
            hudWebView.evaluateJavascript("window.RpApp && RpApp.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        if (rpOpen && !rpReady && curUrl != null && curUrl.endsWith("rp/index.html")) {
            pendingRpJson = json; // la page charge encore : elle prendra ce JSON a ready()
            return;
        }
        Log.i(TAG, "[rp] ouverture du menu RP");
        rpOpen = true;
        rpReady = false;
        facOpen = false;
        gangOpen = false;
        govOpen = false;
        uiOpen = false;
        shopOpen = false;
        jobOpen = false;
        lockerOpen = false;
        policeOpen = false;
        configOpen = true;
        invOpen = false;
        bankOpen = false;
        pharmacyOpen = false;
        pendingRpJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/rp/index.html");
        if (rpWatchdog != null) authHandler.removeCallbacks(rpWatchdog);
        rpWatchdog = () -> {
            if (rpOpen && !rpReady) {
                Log.e(TAG, "[rp] la page ne s'est pas affichee a temps -> fermeture de secours");
                closeRpPage(true);
            }
        };
        authHandler.postDelayed(rpWatchdog, 5000);
    }

    private void closeRpPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = rpOpen;
            rpClosedAtMs = System.currentTimeMillis();
            if (rpWatchdog != null) { authHandler.removeCallbacks(rpWatchdog); rpWatchdog = null; }
            rpOpen = false;
            rpReady = false;
            pendingRpJson = null;
            String curUrl = hudWebView.getUrl();
            // une autre page a pu s'ouvrir entre-temps : on ne la ferme pas
            if (curUrl == null || !curUrl.endsWith("rp/index.html")) {
                if (notifyServer && wasOpen) sendChatInput("/rpact close");
                return;
            }
            configOpen = false;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/rpact close");
        });
    }

    // ---- Dialogue de PNJ (page assets/npc/index.html) ----
    // La camera du jeu est deja sur le visage du PNJ (serveur) : la page est transparente,
    // seule la bulle de dialogue en bas de l'ecran est dessinee.
    private void handleNpcDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9712) closeNpcPage(false);
            else openNpcPage(json);
        });
    }

    private void openNpcPage(String json) {
        if (hudWebView == null) return;
        if (authOpen || adminOpen) {
            // Le serveur a deja fige le joueur pour ce dialogue : on lui rend la main au lieu de rester silencieux.
            sendHudCommand("/npcact close");
            return;
        }
        String curUrl = hudWebView.getUrl();
        if (npcOpen && curUrl != null && curUrl.endsWith("npc/index.html")) {
            if (npcReady) hudWebView.evaluateJavascript("window.NpcApp && NpcApp.open(" + JSONObject.quote(json) + ")", null);
            else pendingNpcJson = json;
            return;
        }
        Log.i(TAG, "[npc] ouverture du dialogue");
        npcOpen = true;
        npcReady = false;
        pendingNpcJson = json;
        configOpen = true;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/npc/index.html");
        // Securite : si la page ne s'affiche pas, on rend la main au joueur (sinon il resterait fige).
        authHandler.postDelayed(() -> {
            if (npcOpen && !npcReady) {
                Log.e(TAG, "[npc] la page ne s'est pas affichee a temps -> fermeture");
                closeNpcPage(true);
            }
        }, 5000);
    }

    private void closeNpcPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = npcOpen;
            npcOpen = false;
            npcReady = false;
            pendingNpcJson = null;
            String curUrl = hudWebView.getUrl();
            if (curUrl != null && curUrl.endsWith("npc/index.html")) {
                configOpen = false;
                hudWebView.setFullscreenPage(false);
                hudWebView.setPhonePage(false);
                hudWebView.setVisibility(View.VISIBLE);
                navigate("file:///android_asset/hud/index.html");
            }
            if (notifyServer && wasOpen) sendHudCommand("/npcact close");
        });
    }

    private final class SampNpcBridge {
        // JS -> serveur : /npcact <choix>. Le serveur re-verifie tout (distance, argent...).
        @JavascriptInterface public void choose(String id) {
            String c = id == null ? "" : id.replaceAll("[^a-z0-9_]", "");
            if (c.isEmpty() || c.length() > 16) c = "cancel";
            final String cmd = "/npcact " + c;
            runOnUiThread(() -> {
                // la page se ferme tout de suite ; le serveur rend la camera et peut en rouvrir une autre
                npcOpen = false;
                npcReady = false;
                String curUrl = hudWebView == null ? null : hudWebView.getUrl();
                if (hudWebView != null && curUrl != null && curUrl.endsWith("npc/index.html")) {
                    configOpen = false;
                    hudWebView.setFullscreenPage(false);
                    navigate("file:///android_asset/hud/index.html");
                }
                sendHudCommand(cmd);
            });
        }

        @JavascriptInterface public void close() {
            closeNpcPage(true);
        }

        @JavascriptInterface public void ready() {
            npcReady = true;
            final String pend = pendingNpcJson;
            pendingNpcJson = null;
            runOnUiThread(() -> {
                if (pend != null && hudWebView != null) hudWebView.evaluateJavascript("window.NpcApp && NpcApp.open(" + JSONObject.quote(pend) + ")", null);
            });
        }
    }

    private final class SampRpBridge {
        // JS -> serveur : /rpact <action> [argument]. Liste blanche d'actions ; le serveur re-verifie tout
        // (distance, argent, faction, service...).
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                String arg;
                switch (t) {
                    case "refresh": case "animstop": case "soscancel": case "descoff":
                        arg = "";
                        break;
                    case "view": case "anim":
                        arg = o.optString("arg", "").replaceAll("[^a-z0-9]", "");
                        if (arg.length() > 12) arg = arg.substring(0, 12);
                        break;
                    case "pay":
                        arg = o.optString("arg", "").replaceAll("[^a-z0-9 ]", "").trim();
                        if (arg.length() > 20) arg = arg.substring(0, 20);
                        break;
                    case "refuse": case "cancel": case "addel": case "sostake": case "sosdone": case "fight":
                        arg = o.optString("arg", "").replaceAll("[^0-9]", "");
                        if (arg.length() > 9) arg = arg.substring(0, 9);
                        break;
                    case "lawbackup": case "lawbackupend": case "lawbarrier": case "lawbarrierdel":
                        arg = "";
                        break;
                    case "lawclear": case "lawgps": case "lawnotedel":
                        arg = o.optString("arg", "").replaceAll("[^0-9]", "");
                        if (arg.length() > 9) arg = arg.substring(0, 9);
                        break;
                    case "lawwanted": case "lawnote": case "lawalert":
                        // centrale des forces de l'ordre : texte libre nettoye (le serveur re-verifie la faction)
                        arg = o.optString("arg", "").replaceAll("[\\r\\n\\t%\"\\\\{}']", " ").replaceAll("\\s+", " ").trim();
                        if (arg.length() > 100) arg = arg.substring(0, 100);
                        break;
                    case "promouse": case "promoadd": case "promodel":
                        // codes promo : lettres, chiffres, _ et espaces seulement
                        arg = o.optString("arg", "").replaceAll("[^A-Za-z0-9_ ]", "").replaceAll("\\s+", " ").trim();
                        if (arg.length() > 60) arg = arg.substring(0, 60);
                        break;
                    case "bill": case "ad": case "sos": case "desc":
                        arg = o.optString("arg", "").replaceAll("[\\r\\n\\t%\"\\\\{}']", " ").replaceAll("\\s+", " ").trim();
                        if (arg.length() > 100) arg = arg.substring(0, 100);
                        break;
                    case "go":
                        arg = o.optString("arg", "").replaceAll("[^a-z]", "");
                        if (arg.length() > 8) arg = arg.substring(0, 8);
                        // Raccourcis geres par l'application (pas besoin du serveur)
                        if (arg.equals("map") || arg.equals("phone") || arg.equals("settings")) {
                            final String local = arg;
                            runOnUiThread(() -> {
                                closeRpPage(true);
                                if (local.equals("map")) openInteractionPage("map/index.html");
                                else if (local.equals("phone")) openPhonePage();
                                else openConfig();
                            });
                            return;
                        }
                        if (arg.equals("report")) {
                            runOnUiThread(() -> { rpOpen = false; openReportPage("{\"loading\":1}"); sendHudCommand("/repact open"); sendHudCommand("/rpact close"); });
                            return;
                        }
                        // l'autre page va s'ouvrir : on libere la page RP sans recharger le HUD
                        runOnUiThread(() -> { rpOpen = false; rpReady = false; });
                        break;
                    default:
                        return;
                }
                sendHudCommand("/rpact " + t + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "rp action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeRpPage(true);
        }

        @JavascriptInterface public void ready() {
            rpReady = true;
            final String pend = pendingRpJson;
            pendingRpJson = null;
            runOnUiThread(() -> {
                if (rpWatchdog != null) { authHandler.removeCallbacks(rpWatchdog); rpWatchdog = null; }
                if (pend != null && hudWebView != null) hudWebView.evaluateJavascript("window.RpApp && RpApp.openFromNative(" + JSONObject.quote(pend) + ")", null);
            });
            Log.i(TAG, "[rp] page affichee");
        }
    }

    // ---- Panneau de faction (page assets/faction/index.html) ----
    private void handleFacDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9682) {
                closeFacPage(false);
            } else if (dialogId == 9680) {
                openFacPage(json);
            } else if (facOpen) {
                hudWebView.evaluateJavascript("window.FacApp && FacApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openFacPage(String json) {
        if (hudWebView == null || authOpen || adminOpen) return;
        String curUrl = hudWebView.getUrl();
        if (facOpen && facReady && curUrl != null && curUrl.endsWith("faction/index.html")) {
            hudWebView.evaluateJavascript("window.FacApp && FacApp.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        Log.i(TAG, "[faction] ouverture du panneau de faction");
        facOpen = true;
        facReady = false;
        gangOpen = false;
        govOpen = false;
        uiOpen = false;
        shopOpen = false;
        jobOpen = false;
        lockerOpen = false;
        policeOpen = false;
        configOpen = true;
        invOpen = false;
        bankOpen = false;
        pharmacyOpen = false;
        pendingFacJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/faction/index.html");
        if (facWatchdog != null) authHandler.removeCallbacks(facWatchdog);
        facWatchdog = () -> {
            if (facOpen && !facReady) {
                Log.e(TAG, "[faction] la page ne s'est pas affichee a temps -> fermeture de secours");
                closeFacPage(true);
            }
        };
        authHandler.postDelayed(facWatchdog, 5000);
    }

    private void closeFacPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = facOpen;
            if (facWatchdog != null) { authHandler.removeCallbacks(facWatchdog); facWatchdog = null; }
            facOpen = false;
            facReady = false;
            pendingFacJson = null;
            String curUrl = hudWebView.getUrl();
            // une autre page a pu s'ouvrir entre-temps : on ne la ferme pas
            if (!wasOpen && (curUrl == null || !curUrl.endsWith("faction/index.html"))) return;
            configOpen = false;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/facact close");
        });
    }

    private final class SampFactionBridge {
        // JS -> serveur : /facact <action> [argument]. Liste blanche d'actions ; le serveur re-verifie
        // faction, grade, distance du citoyen et montants a chaque action.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                String arg;
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                switch (t) {
                    case "refresh":
                        arg = "";
                        break;
                    case "view":
                        arg = o.optString("arg", "").replaceAll("[^a-z]", "");
                        if (arg.length() > 8) arg = arg.substring(0, 8);
                        break;
                    case "deposit": case "withdraw": case "evdel":
                        arg = o.optString("arg", "").replaceAll("[^0-9]", "");
                        if (arg.length() > 9) arg = arg.substring(0, 9);
                        break;
                    case "promote": case "demote": case "kick":
                        arg = o.optString("arg", "").replaceAll("[^A-Za-z0-9_\\[\\].$@=()]", "");
                        if (arg.length() > 24) arg = arg.substring(0, 24);
                        break;
                    case "evadd":
                        arg = o.optString("arg", "").replaceAll("[\\r\\n\\t%\"\\\\{}']", " ").replaceAll("\\s+", " ").trim();
                        if (arg.length() > 70) arg = arg.substring(0, 70);
                        break;
                    default:
                        return;
                }
                sendHudCommand("/facact " + t + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "faction action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeFacPage(true);
        }

        @JavascriptInterface public void ready() {
            facReady = true;
            runOnUiThread(() -> { if (facWatchdog != null) { authHandler.removeCallbacks(facWatchdog); facWatchdog = null; } });
            Log.i(TAG, "[faction] page affichee");
        }
    }

    // ---- Immobilier : maisons des joueurs (page assets/house/index.html) ----
    private void handleHouseDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9732) {
                closeHousePage(false);
            } else if (dialogId == 9730) {
                openHousePage(json);
            } else if (houseOpen) {
                hudWebView.evaluateJavascript("window.HouseApp && HouseApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openHousePage(String json) {
        if (hudWebView == null || authOpen || adminOpen) return;
        String curUrl = hudWebView.getUrl();
        if (houseOpen && houseReady && curUrl != null && curUrl.endsWith("house/index.html")) {
            hudWebView.evaluateJavascript("window.HouseApp && HouseApp.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        if (houseOpen && !houseReady && curUrl != null && curUrl.endsWith("house/index.html")) {
            pendingHouseJson = json;
            return;
        }
        Log.i(TAG, "[house] ouverture de la page");
        houseOpen = true;
        houseReady = false;
        facOpen = false;
        gangOpen = false;
        govOpen = false;
        uiOpen = false;
        shopOpen = false;
        jobOpen = false;
        lockerOpen = false;
        policeOpen = false;
        configOpen = true;
        invOpen = false;
        bankOpen = false;
        pharmacyOpen = false;
        pendingHouseJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/house/index.html");
        if (houseWatchdog != null) authHandler.removeCallbacks(houseWatchdog);
        houseWatchdog = () -> {
            if (houseOpen && !houseReady) {
                Log.e(TAG, "[house] la page ne s'est pas affichee a temps -> fermeture de secours");
                closeHousePage(true);
            }
        };
        authHandler.postDelayed(houseWatchdog, 5000);
    }

    private void closeHousePage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = houseOpen;
            if (houseWatchdog != null) { authHandler.removeCallbacks(houseWatchdog); houseWatchdog = null; }
            houseOpen = false;
            houseReady = false;
            pendingHouseJson = null;
            String curUrl = hudWebView.getUrl();
            if (!wasOpen && (curUrl == null || !curUrl.endsWith("house/index.html"))) return;
            configOpen = false;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/houseact close");
        });
    }

    private final class SampHouseBridge {
        // JS -> serveur : /houseact <action> [argument]. Liste blanche ; le serveur re-verifie tout.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                String arg;
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                switch (t) {
                    case "refresh": case "buy": case "sell": case "lock": case "enter":
                        arg = "";
                        break;
                    case "view":
                        arg = o.optString("arg", "").replaceAll("[^a-z]", "");
                        if (arg.length() > 10) arg = arg.substring(0, 10);
                        break;
                    case "gps": case "deposit": case "withdraw": case "bid":
                        arg = o.optString("arg", "").replaceAll("[^0-9]", "");
                        if (arg.length() > 9) arg = arg.substring(0, 9);
                        break;
                    default:
                        return;
                }
                sendHudCommand("/houseact " + t + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "house action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeHousePage(true);
        }

        @JavascriptInterface public void ready() {
            houseReady = true;
            final String pend = pendingHouseJson;
            runOnUiThread(() -> {
                if (houseWatchdog != null) { authHandler.removeCallbacks(houseWatchdog); houseWatchdog = null; }
                if (pend != null && hudWebView != null) {
                    pendingHouseJson = null;
                    hudWebView.evaluateJavascript("window.HouseApp && HouseApp.openFromNative(" + JSONObject.quote(pend) + ")", null);
                }
            });
            Log.i(TAG, "[house] page affichee");
        }
    }

    // ---- Peche (page assets/fish/index.html) ----
    private void handleFishDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9742) {
                closeFishPage(false);
            } else if (dialogId == 9740) {
                openFishPage(json);
            } else if (fishOpen) {
                hudWebView.evaluateJavascript("window.FishApp && FishApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openFishPage(String json) {
        if (hudWebView == null || authOpen || adminOpen) return;
        String curUrl = hudWebView.getUrl();
        if (fishOpen && fishReady && curUrl != null && curUrl.endsWith("fish/index.html")) {
            hudWebView.evaluateJavascript("window.FishApp && FishApp.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        if (fishOpen && !fishReady && curUrl != null && curUrl.endsWith("fish/index.html")) {
            pendingFishJson = json;
            return;
        }
        Log.i(TAG, "[fish] ouverture de la page");
        fishOpen = true;
        fishReady = false;
        facOpen = false;
        gangOpen = false;
        govOpen = false;
        uiOpen = false;
        shopOpen = false;
        jobOpen = false;
        lockerOpen = false;
        policeOpen = false;
        configOpen = true;
        invOpen = false;
        bankOpen = false;
        pharmacyOpen = false;
        pendingFishJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/fish/index.html");
        if (fishWatchdog != null) authHandler.removeCallbacks(fishWatchdog);
        fishWatchdog = () -> {
            if (fishOpen && !fishReady) {
                Log.e(TAG, "[fish] la page ne s'est pas affichee a temps -> fermeture de secours");
                closeFishPage(true);
            }
        };
        authHandler.postDelayed(fishWatchdog, 5000);
    }

    private void closeFishPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = fishOpen;
            if (fishWatchdog != null) { authHandler.removeCallbacks(fishWatchdog); fishWatchdog = null; }
            fishOpen = false;
            fishReady = false;
            pendingFishJson = null;
            String curUrl = hudWebView.getUrl();
            if (!wasOpen && (curUrl == null || !curUrl.endsWith("fish/index.html"))) return;
            configOpen = false;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/fishact close");
        });
    }

    private final class SampFishBridge {
        // JS -> serveur : /fishact <action> [argument]. Liste blanche ; le serveur re-verifie tout.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                String arg;
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                switch (t) {
                    case "refresh": case "buyrod": case "cast": case "cancel": case "sell":
                        arg = "";
                        break;
                    case "hook":
                        arg = o.optString("arg", "").replaceAll("[^0-9]", "");
                        if (arg.length() > 9) arg = arg.substring(0, 9);
                        break;
                    default:
                        return;
                }
                sendHudCommand("/fishact " + t + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "fish action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeFishPage(true);
        }

        @JavascriptInterface public void ready() {
            fishReady = true;
            final String pend = pendingFishJson;
            runOnUiThread(() -> {
                if (fishWatchdog != null) { authHandler.removeCallbacks(fishWatchdog); fishWatchdog = null; }
                if (pend != null && hudWebView != null) {
                    pendingFishJson = null;
                    hudWebView.evaluateJavascript("window.FishApp && FishApp.openFromNative(" + JSONObject.quote(pend) + ")", null);
                }
            });
            Log.i(TAG, "[fish] page affichee");
        }
    }

    // ---- Garage (page assets/garage/index.html) ----
    private void handleGarageDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9752) {
                closeGaragePage(false);
            } else if (dialogId == 9750) {
                openGaragePage(json);
            } else if (garageOpen) {
                hudWebView.evaluateJavascript("window.GarageApp && GarageApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openGaragePage(String json) {
        if (hudWebView == null || authOpen || adminOpen) return;
        String curUrl = hudWebView.getUrl();
        if (garageOpen && garageReady && curUrl != null && curUrl.endsWith("garage/index.html")) {
            hudWebView.evaluateJavascript("window.GarageApp && GarageApp.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        if (garageOpen && !garageReady && curUrl != null && curUrl.endsWith("garage/index.html")) {
            pendingGarageJson = json;
            return;
        }
        Log.i(TAG, "[garage] ouverture de la page");
        garageOpen = true;
        garageReady = false;
        fishOpen = false;
        facOpen = false;
        gangOpen = false;
        govOpen = false;
        uiOpen = false;
        shopOpen = false;
        jobOpen = false;
        lockerOpen = false;
        policeOpen = false;
        configOpen = true;
        invOpen = false;
        bankOpen = false;
        pharmacyOpen = false;
        pendingGarageJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/garage/index.html");
        if (garageWatchdog != null) authHandler.removeCallbacks(garageWatchdog);
        garageWatchdog = () -> {
            if (garageOpen && !garageReady) {
                Log.e(TAG, "[garage] la page ne s'est pas affichee a temps -> fermeture de secours");
                closeGaragePage(true);
            }
        };
        authHandler.postDelayed(garageWatchdog, 5000);
    }

    private void closeGaragePage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = garageOpen;
            if (garageWatchdog != null) { authHandler.removeCallbacks(garageWatchdog); garageWatchdog = null; }
            garageOpen = false;
            garageReady = false;
            pendingGarageJson = null;
            String curUrl = hudWebView.getUrl();
            if (!wasOpen && (curUrl == null || !curUrl.endsWith("garage/index.html"))) return;
            configOpen = false;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/garact close");
        });
    }

    private final class SampGarageBridge {
        // JS -> serveur : /garact <action> [argument]. Liste blanche ; le serveur re-verifie tout.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                String arg;
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                switch (t) {
                    case "refresh":
                        arg = "";
                        break;
                    case "take": case "store": case "sell": case "gps":
                        arg = o.optString("arg", "").replaceAll("[^0-9]", "");
                        if (arg.isEmpty() || arg.length() > 9) return;
                        break;
                    default:
                        return;
                }
                sendHudCommand("/garact " + t + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "garage action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeGaragePage(true);
        }

        @JavascriptInterface public void ready() {
            garageReady = true;
            final String pend = pendingGarageJson;
            runOnUiThread(() -> {
                if (garageWatchdog != null) { authHandler.removeCallbacks(garageWatchdog); garageWatchdog = null; }
                if (pend != null && hudWebView != null) {
                    pendingGarageJson = null;
                    hudWebView.evaluateJavascript("window.GarageApp && GarageApp.openFromNative(" + JSONObject.quote(pend) + ")", null);
                }
            });
            Log.i(TAG, "[garage] page affichee");
        }
    }

    // ---- Marqueurs 3D flottants (dessines par le moteur : game/WorldMarkers.cpp) ----
    private final StringBuilder worldMarkersAll = new StringBuilder();

    private void handleWorldMarkers(final String rawJson, final boolean append) {
        try {
            String raw = rawJson == null ? "[]" : rawJson.trim();
            int i = raw.indexOf('[');
            int j = raw.lastIndexOf(']');
            org.json.JSONArray arr = new org.json.JSONArray(i >= 0 && j > i ? raw.substring(i, j + 1) : "[]");
            StringBuilder sb = new StringBuilder();
            for (int k = 0; k < arr.length() && k < 512; k++) {
                JSONObject m = arr.optJSONObject(k);
                if (m == null) continue;
                String label = m.optString("n", "").replaceAll("[,;\\r\\n]", " ");
                if (label.length() > 30) label = label.substring(0, 30);
                sb.append(m.optInt("t", 0)).append(',')
                  .append((float) m.optDouble("x", 0)).append(',')
                  .append((float) m.optDouble("y", 0)).append(',')
                  .append((float) m.optDouble("z", 0)).append(',')
                  .append(label).append(';');
            }
            synchronized (worldMarkersAll) {
                if (!append) worldMarkersAll.setLength(0);
                worldMarkersAll.append(sb);
                setWorldMarkers(worldMarkersAll.toString());
            }
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "setWorldMarkers indisponible : " + e.getMessage());
        } catch (Exception e) {
            Log.e(TAG, "marqueurs 3D", e);
        }
    }

    // ---- Gangs + guerre des territoires (page assets/gang/index.html) ----
    private void handleGangDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9722) {
                closeGangPage(false);
            } else if (dialogId == 9720) {
                openGangPage(json);
            } else if (gangOpen) {
                hudWebView.evaluateJavascript("window.GangApp && GangApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openGangPage(String json) {
        if (hudWebView == null || authOpen || adminOpen) return;
        String curUrl = hudWebView.getUrl();
        if (gangOpen && gangReady && curUrl != null && curUrl.endsWith("gang/index.html")) {
            hudWebView.evaluateJavascript("window.GangApp && GangApp.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        if (gangOpen && !gangReady && curUrl != null && curUrl.endsWith("gang/index.html")) {
            pendingGangJson = json; // la page charge encore
            return;
        }
        Log.i(TAG, "[gang] ouverture du panneau des gangs");
        gangOpen = true;
        gangReady = false;
        facOpen = false;
        govOpen = false;
        uiOpen = false;
        shopOpen = false;
        jobOpen = false;
        lockerOpen = false;
        policeOpen = false;
        configOpen = true;
        invOpen = false;
        bankOpen = false;
        pharmacyOpen = false;
        pendingGangJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/gang/index.html");
        if (gangWatchdog != null) authHandler.removeCallbacks(gangWatchdog);
        gangWatchdog = () -> {
            if (gangOpen && !gangReady) {
                Log.e(TAG, "[gang] la page ne s'est pas affichee a temps -> fermeture de secours");
                closeGangPage(true);
            }
        };
        authHandler.postDelayed(gangWatchdog, 5000);
    }

    private void closeGangPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = gangOpen;
            if (gangWatchdog != null) { authHandler.removeCallbacks(gangWatchdog); gangWatchdog = null; }
            gangOpen = false;
            gangReady = false;
            pendingGangJson = null;
            String curUrl = hudWebView.getUrl();
            // une autre page a pu s'ouvrir entre-temps : on ne la ferme pas
            if (!wasOpen && (curUrl == null || !curUrl.endsWith("gang/index.html"))) return;
            configOpen = false;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/gangact close");
        });
    }

    private final class SampGangBridge {
        // JS -> serveur : /gangact <action> [argument]. Liste blanche ; le serveur re-verifie gang, grade et position.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                String arg;
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                switch (t) {
                    case "refresh": case "quit":
                        arg = "";
                        break;
                    case "gps": case "attack": case "hq": case "invite":
                        arg = o.optString("arg", "").replaceAll("[^0-9]", "");
                        if (arg.length() > 4) arg = arg.substring(0, 4);
                        break;
                    case "promote": case "demote": case "kick":
                        arg = o.optString("arg", "").replaceAll("[^A-Za-z0-9_\\[\\].$@=()]", "");
                        if (arg.length() > 24) arg = arg.substring(0, 24);
                        if (arg.isEmpty()) return;
                        break;
                    default:
                        return;
                }
                sendHudCommand("/gangact " + t + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "gang action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeGangPage(true);
        }

        @JavascriptInterface public void ready() {
            gangReady = true;
            final String pend = pendingGangJson;
            runOnUiThread(() -> {
                if (gangWatchdog != null) { authHandler.removeCallbacks(gangWatchdog); gangWatchdog = null; }
                if (pend != null && hudWebView != null) {
                    pendingGangJson = null;
                    hudWebView.evaluateJavascript("window.GangApp && GangApp.openFromNative(" + JSONObject.quote(pend) + ")", null);
                }
            });
            Log.i(TAG, "[gang] page affichee");
        }
    }

    // ---- Emplois : contrat de travail + agence (page assets/job/index.html) ----
    private void handleJobDialog(final int dialogId, final String rawJson) {
        final String json = extractJson(rawJson);
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9602) {
                closeJobPage(false);
            } else if (dialogId == 9600) {
                openJobPage(json);
            } else if (jobOpen) {
                hudWebView.evaluateJavascript("window.JobApp && JobApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openJobPage(String json) {
        if (hudWebView == null || authOpen || adminOpen) return;
        if (jobOpen && jobReady) {
            hudWebView.evaluateJavascript("window.JobApp && JobApp.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        Log.i(TAG, "[job] ouverture du contrat / de l'agence");
        jobOpen = true;
        jobReady = false;
        shopOpen = false;
        lockerOpen = false;
        policeOpen = false;
        configOpen = true;
        invOpen = false;
        bankOpen = false;
        pharmacyOpen = false;
        pendingJobJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/job/index.html");
        if (jobWatchdog != null) authHandler.removeCallbacks(jobWatchdog);
        jobWatchdog = () -> {
            if (jobOpen && !jobReady) {
                Log.e(TAG, "[job] la page ne s'est pas affichee a temps -> fermeture de secours");
                closeJobPage(true);
            }
        };
        authHandler.postDelayed(jobWatchdog, 5000);
    }

    private void closeJobPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = jobOpen;
            if (jobWatchdog != null) { authHandler.removeCallbacks(jobWatchdog); jobWatchdog = null; }
            jobOpen = false;
            jobReady = false;
            configOpen = false;
            pendingJobJson = null;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/jobact close");
        });
    }

    private final class SampJobBridge {
        // JS -> serveur : /jobact <sign|start|stop|resign|gps|list|refresh> [id]. Le serveur re-verifie tout.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                if (!t.equals("sign") && !t.equals("start") && !t.equals("stop") && !t.equals("resign")
                        && !t.equals("gps") && !t.equals("list") && !t.equals("refresh")) return;
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                String arg = o.optString("arg", "").replaceAll("[^0-9]", "");
                if (arg.length() > 3) arg = arg.substring(0, 3);
                sendHudCommand("/jobact " + t + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "job action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeJobPage(true);
        }

        @JavascriptInterface public void ready() {
            jobReady = true;
            Log.i(TAG, "[job] page affichee");
        }
    }

    // ---- Vestiaire de faction (page assets/locker/index.html) ----
    private void handleLockerDialog(final int dialogId, final String rawJson) {
        String tmp = rawJson == null ? "{}" : rawJson;
        int js = tmp.indexOf('{');
        int je = tmp.lastIndexOf('}');
        final String json = (js >= 0 && je > js) ? tmp.substring(js, je + 1) : "{}";
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) return;
            if (dialogId == 9562) {            // fermeture demandee par le serveur
                closeLockerPage(false);
            } else if (dialogId == 9560) {     // ouverture (icone de main "Vestiaire" ou /vestiaire)
                openLockerPage(json);
            } else if (lockerOpen) {           // 9561 : mise a jour (tenue, message)
                hudWebView.evaluateJavascript("window.LockerApp && LockerApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openLockerPage(String json) {
        if (hudWebView == null || authOpen || adminOpen) return;
        if (lockerOpen && lockerReady) {
            hudWebView.evaluateJavascript("window.LockerApp && LockerApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        Log.i(TAG, "[locker] ouverture du vestiaire");
        lockerOpen = true;
        lockerReady = false;
        configOpen = true;
        invOpen = false;
        bankOpen = false;
        pharmacyOpen = false;
        pendingLockerJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/locker/index.html");
        if (lockerWatchdog != null) authHandler.removeCallbacks(lockerWatchdog);
        lockerWatchdog = () -> {
            if (lockerOpen && !lockerReady) {
                Log.e(TAG, "[locker] la page ne s'est pas affichee a temps -> fermeture de secours");
                closeLockerPage(true);
            }
        };
        authHandler.postDelayed(lockerWatchdog, 5000);
    }

    private void closeLockerPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = lockerOpen;
            if (lockerWatchdog != null) { authHandler.removeCallbacks(lockerWatchdog); lockerWatchdog = null; }
            lockerOpen = false;
            lockerReady = false;
            configOpen = false;
            pendingLockerJson = null;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/lockeract close");
        });
    }

    private final class SampLockerBridge {
        // JS -> serveur : /lockeract <wear|civil|refresh> [femme ceinture gilet casquette]. Le serveur re-verifie faction et distance.
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z]", "");
                if (!t.equals("wear") && !t.equals("civil") && !t.equals("refresh")) return;
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                String arg = o.optString("arg", "").replaceAll("[^01 ]", "").trim();
                if (arg.length() > 12) arg = arg.substring(0, 12);
                sendHudCommand("/lockeract " + t + (arg.isEmpty() ? "" : " " + arg));
            } catch (Exception e) {
                Log.e(TAG, "locker action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeLockerPage(true);
        }

        @JavascriptInterface public void ready() {
            lockerReady = true;
            Log.i(TAG, "[locker] page affichee");
        }
    }

    // ---- Connexion (page assets/login/index.html) ----
    private void handleAuthDialog(final int dialogId, final String rawJson) {
        String tmp = rawJson == null ? "{}" : rawJson;
        int js = tmp.indexOf('{');
        int je = tmp.lastIndexOf('}');
        final String json = (js >= 0 && je > js) ? tmp.substring(js, je + 1) : "{}";
        // Le serveur parle : l'ecran de chargement ne doit plus masquer la page de connexion
        loadingClosedByGame = true;
        hideLoadingScreen();
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) {
                CrashLog.log("[auth] WebView indisponible, ouverture impossible");
                return;
            }
            if (dialogId == 9522) {            // fermeture demandee par le serveur (connexion reussie)
                closeAuthPage();
            } else if (dialogId == 9520) {     // ouverture
                openAuthPage(json);
            } else if (authOpen) {             // 9521 : reponse (succes / erreur / changement d'onglet)
                hudWebView.evaluateJavascript("window.DXAuth && DXAuth.handleResponse(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openAuthPage(String json) {
        if (hudWebView == null) return;
        if (authOpen && authReady) {
            // deja affichee : on reapplique seulement l'onglet / le pseudo
            hudWebView.evaluateJavascript("window.DXAuth && DXAuth.openFromNative(" + JSONObject.quote(json) + ")", null);
            return;
        }
        CrashLog.log("[auth] ouverture de la page de connexion");
        authOpen = true;
        authReady = false;
        configOpen = true;
        bankOpen = false;
        pharmacyOpen = false;
        pendingAuthJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/login/index.html");

        // Garde-fou : si la page ne s affiche pas en 6 s, on la ferme pour ne pas laisser un ecran vide
        // (le serveur peut la rouvrir : voir Auth_OnRequestClass).
        cancelAuthWatchdog();
        authWatchdog = () -> {
            if (authOpen && !authReady) {
                CrashLog.log("[auth] la page ne s'est pas affichee a temps -> fermeture de secours");
                Toast.makeText(SAMP.this, "Ecran de connexion indisponible, reessaie.", Toast.LENGTH_SHORT).show();
                closeAuthPage();
                requestAuthOpenLater(2000);
            }
        };
        authHandler.postDelayed(authWatchdog, 6000);
    }

    /** Si la page de connexion n'est pas ouverte, demande au serveur (/authact open) de l'envoyer ; 5 essais max. */
    private void requestAuthOpenLater(long delayMs) {
        authHandler.postDelayed(() -> {
            if (authOpen || authOpenAttempts >= 5 || isFinishing()) return;
            authOpenAttempts++;
            CrashLog.log("[auth] ecran de connexion absent -> demande au serveur (essai " + authOpenAttempts + "/5)");
            sendHudCommand("/authact open");
            requestAuthOpenLater(4000);
        }, delayMs);
    }

    private void cancelAuthWatchdog() {
        if (authWatchdog != null) {
            authHandler.removeCallbacks(authWatchdog);
            authWatchdog = null;
        }
    }

    private void closeAuthPage() {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            cancelAuthWatchdog();
            authOpen = false;
            authReady = false;
            configOpen = false;
            pendingAuthJson = null;
            hudWebView.hideKeyboard();
            hudWebView.setInteractive(false);   // anti-figeage : ne laisse aucun etat tactile de la page de connexion
            hudWebView.setTouchBounds(null);
            hudWebView.setWheelOpen(false);
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            // recharger le HUD arrete aussi la musique de la page de connexion
            navigate("file:///android_asset/hud/index.html");
        });
    }

    private final class SampAuthBridge {
        // JS -> serveur : /authact <type> <arguments...>
        // Les champs sont deja valides cote page (pseudo, e-mail, mot de passe sans espace ni / \).
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.toLowerCase().replaceAll("[^a-z]", "");
                if (t.isEmpty() || t.equals("openlink")) return;
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                String user = cleanAuthToken(o.optString("username", ""), 24);
                String command;
                switch (t) {
                    case "login":
                        command = "/authact login " + user + " " + cleanAuthToken(o.optString("password", ""), 100);
                        break;
                    case "register":
                        command = "/authact register " + user + " " + cleanAuthToken(o.optString("email", ""), 63)
                                + " " + cleanAuthToken(o.optString("password", ""), 100);
                        break;
                    case "requestrecoverycode":
                        command = "/authact code " + user;
                        break;
                    case "identity": {
                        // [v13] identite du personnage : /authact identity jj/mm/aaaa 0|1 Origine_Avec_Tirets_Bas
                        String birth = o.optString("birth", "").replaceAll("[^0-9/]", "");
                        if (birth.length() > 10) birth = birth.substring(0, 10);
                        String gender = String.valueOf(o.optInt("gender", -1));
                        if (!gender.equals("0") && !gender.equals("1")) return;
                        String origin = o.optString("origin", "").replaceAll("[^A-Za-z_-]", "");
                        if (origin.length() > 22) origin = origin.substring(0, 22);
                        if (birth.isEmpty() || origin.isEmpty()) return;
                        command = "/authact identity " + birth + " " + gender + " " + origin;
                        break;
                    }
                    case "recover":
                        command = "/authact recover " + user + " " + cleanAuthToken(o.optString("code", ""), 18)
                                + " " + cleanAuthToken(o.optString("newPassword", ""), 100);
                        break;
                    default:
                        return;
                }
                // Le chat SA-MP limite une commande a 128 caracteres : au-dela le serveur la tronquerait
                if (command.length() > 127) {
                    runOnUiThread(() -> Toast.makeText(SAMP.this, "Donnees trop longues (pseudo + mot de passe).", Toast.LENGTH_SHORT).show());
                    return;
                }
                sendHudCommand(command);
            } catch (Exception e) {
                Log.e(TAG, "auth action", e);
            }
        }

        // Appele par login/app.js quand la page est bien affichee (annule le garde-fou)
        @JavascriptInterface public void ready() {
            authReady = true;
            runOnUiThread(() -> cancelAuthWatchdog());
            CrashLog.log("[auth] page affichee");
        }
    }

    /** Un seul argument de commande : ni espace ni caractere de controle, longueur bornee. */
    private static String cleanAuthToken(String value, int max) {
        if (value == null) return "";
        String v = value.replaceAll("[\\s\\x00-\\x1F\\x7F]", "");
        return v.length() > max ? v.substring(0, max) : v;
    }

    // ---- Banque (page assets/bank/index.html) ----
    private void handleBankDialog(final int dialogId, final String rawJson) {
        String tmp = rawJson == null ? "{}" : rawJson;
        int js = tmp.indexOf('{');
        int je = tmp.lastIndexOf('}');
        final String json = (js >= 0 && je > js) ? tmp.substring(js, je + 1) : "{}";
        runOnUiThread(() -> {
            if (hudWebView == null || isFinishing()) {
                Log.e(TAG, "[bank] WebView indisponible, ouverture impossible");
                return;
            }
            if (dialogId == 9502) {            // fermeture demandee par le serveur
                closeBankPage(false);
            } else if (dialogId == 9500) {     // ouverture
                openBankPage(json);
            } else if (bankOpen) {             // 9501 : mise a jour (solde, historique, message)
                hudWebView.evaluateJavascript("window.BankApp && BankApp.onUpdateFromNative(" + JSONObject.quote(json) + ")", null);
            }
        });
    }

    private void openBankPage(String json) {
        if (hudWebView == null) return;
        Log.i(TAG, "[bank] ouverture de la page banque");
        bankOpen = true;
        bankReady = false;
        configOpen = true;
        pharmacyOpen = false;
        pendingBankJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        hudWebView.setFullscreenPage(true);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/bank/index.html");

        // Garde-fou : si la page n'a pas confirme son affichage en 5 s, on la ferme et on previent le
        // serveur (/bankact close) pour que le joueur ne reste JAMAIS fige (TogglePlayerControllable).
        cancelBankWatchdog();
        bankWatchdog = () -> {
            if (bankOpen && !bankReady) {
                Log.e(TAG, "[bank] la page ne s'est pas affichee a temps -> fermeture de secours");
                Toast.makeText(SAMP.this, "Banque indisponible, reessaie.", Toast.LENGTH_SHORT).show();
                closeBankPage(true);
            }
        };
        bankHandler.postDelayed(bankWatchdog, 5000);
    }

    private void cancelBankWatchdog() {
        if (bankWatchdog != null) {
            bankHandler.removeCallbacks(bankWatchdog);
            bankWatchdog = null;
        }
    }

    private void closeBankPage(final boolean notifyServer) {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            boolean wasOpen = bankOpen;
            cancelBankWatchdog();
            bankOpen = false;
            bankReady = false;
            configOpen = false;
            pendingBankJson = null;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
            if (notifyServer && wasOpen) sendChatInput("/bankact close 0 0 0");
        });
    }

    private final class SampBankBridge {
        // JS -> serveur : /bankact <type> <pin> <montant> <cible>
        @JavascriptInterface public void action(String type, String json) {
            try {
                String t = type == null ? "" : type.replaceAll("[^a-z_]", "");
                if (t.isEmpty()) return;
                JSONObject o = new JSONObject(json == null || json.isEmpty() ? "{}" : json);
                long pin = Math.max(0L, Math.min(9999L, o.optLong("pin", 0)));
                long amount = Math.max(0L, Math.min(2000000000L, o.optLong("amount", 0)));
                long target = Math.max(0L, Math.min(1000L, o.optLong("target", 0)));
                sendHudCommand("/bankact " + t + " " + pin + " " + amount + " " + target);
            } catch (Exception e) {
                Log.e(TAG, "bank action", e);
            }
        }

        @JavascriptInterface public void close() {
            closeBankPage(true);
        }

        // Appele par bank.js quand la page est bien affichee (annule le garde-fou)
        @JavascriptInterface public void ready() {
            bankReady = true;
            runOnUiThread(() -> cancelBankWatchdog());
            Log.i(TAG, "[bank] page affichee");
        }
    }

    // ---- Vetements (page assets/clothes/index.html) ----
    // Appelees par le code natif (CJavaWrapper::ClothesPanelOpen / State / Close).
    /** Extract packaged models only into this application's private cache. */
    // [v14] modeles d'objets remplaces au demarrage (assets/models/<nom>/model.dff|txd|col), appele par le natif
    public synchronized String prepareCustomModel(String name) {
        if (name == null || !name.matches("[a-z0-9_]{1,24}")) return null;
        java.io.File dir = new java.io.File(getCacheDir(), "crestwood-models-v1/" + name);
        if (!dir.isDirectory() && !dir.mkdirs()) return null;
        try {
            for (String file : new String[]{"model.dff", "model.col", "materials.txt"}) {
                java.io.File target = new java.io.File(dir, file);
                if (target.isFile() && target.length() > 0) continue;
                java.io.File temp = new java.io.File(dir, file + ".tmp");
                if (file.equals("model.col")) {   // collision facultative
                    try { getAssets().open("models/" + name + "/" + file).close(); } catch (java.io.IOException missing) { continue; }
                }
                try (java.io.InputStream input = getAssets().open("models/" + name + "/" + file);
                     java.io.FileOutputStream output = new java.io.FileOutputStream(temp)) {
                    byte[] buffer = new byte[65536]; int count;
                    while ((count = input.read(buffer)) != -1) output.write(buffer, 0, count);
                }
                if (!temp.renameTo(target)) throw new java.io.IOException("Cannot install model " + file);
            }
            return dir.getAbsolutePath();
        } catch (java.io.IOException e) {
            Log.e(TAG, "[models] Echec de preparation de " + name, e);
            return null;
        }
    }

    public synchronized String prepareClothesModel(String gender) {
        if (!"male".equals(gender) && !"female".equals(gender)) return null;
        java.io.File dir = new java.io.File(getCacheDir(), "crestwood-clothes-v1/" + gender + "skin");
        if (!dir.isDirectory() && !dir.mkdirs()) return null;
        try {
            for (String extension : new String[]{"dff", "txd"}) {
                String name = gender + "." + extension;
                java.io.File target = new java.io.File(dir, name);
                // Asset version is part of the cache directory; never touch the shared game cache.
                if (target.isFile() && target.length() > 0) continue;
                java.io.File temp = new java.io.File(dir, name + ".tmp");
                try (java.io.InputStream input = getAssets().open("clothes/south/" + gender + "skin/" + name);
                     java.io.FileOutputStream output = new java.io.FileOutputStream(temp)) {
                    byte[] buffer = new byte[65536]; int count;
                    while ((count = input.read(buffer)) != -1) output.write(buffer, 0, count);
                }
                if (!temp.renameTo(target)) throw new java.io.IOException("Cannot install clothing model");
            }
            return dir.getAbsolutePath();
        } catch (java.io.IOException e) {
            Log.e(TAG, "[clothes] Falha ao preparar modelo", e);
            return null;
        }
    }

    public byte[] loadClothesTexture(String path) {
        if (path == null || !(path.startsWith("south/") || path.startsWith("models/")) || path.contains("..") || path.contains("\\")) return null;
        // [v14] "models/..." = textures des modeles d'objets (assets/models), le reste = vetements (assets/clothes)
        try (java.io.InputStream input = getAssets().open(path.startsWith("models/") ? path : "clothes/" + path)) {
            android.graphics.BitmapFactory.Options options = new android.graphics.BitmapFactory.Options();
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;
            Bitmap bitmap = android.graphics.BitmapFactory.decodeStream(input, null, options);
            if (bitmap == null) return null;
            int w = bitmap.getWidth(), h = bitmap.getHeight();
            if (w > 512 || h > 512) {
                float scale = 512f / Math.max(w, h);
                Bitmap resized = Bitmap.createScaledBitmap(bitmap, Math.max(1, Math.round(w * scale)), Math.max(1, Math.round(h * scale)), true);
                if (resized != bitmap) bitmap.recycle();
                bitmap = resized; w = bitmap.getWidth(); h = bitmap.getHeight();
            }
            int[] pixels = new int[w * h];
            bitmap.getPixels(pixels, 0, w, 0, 0, w, h);
            bitmap.recycle();
            byte[] rgba = new byte[4 + pixels.length * 4];
            rgba[0] = (byte) w; rgba[1] = (byte) (w >> 8);
            rgba[2] = (byte) h; rgba[3] = (byte) (h >> 8);
            for (int i = 0, p = 4; i < pixels.length; i++) {
                int color = pixels[i];
                rgba[p++] = (byte) (color >> 16); rgba[p++] = (byte) (color >> 8);
                rgba[p++] = (byte) color; rgba[p++] = (byte) (color >>> 24);
            }
            return rgba;
        } catch (java.io.IOException | RuntimeException e) {
            Log.e(TAG, "[clothes] Falha ao carregar " + path, e);
            return null;
        }
    }

    public void clothesOpen(final String json) {
        runOnUiThread(() -> openClothesPage(json));
    }

    public void clothesState(final String json) {
        runOnUiThread(() -> {
            if (!clothesPageOpen || hudWebView == null) return;
            if (!clothesReady) {
                pendingClothesJson = json;
                return;
            }
            hudWebView.evaluateJavascript("window.ClothesApp && ClothesApp.onState(" + JSONObject.quote(json) + ")", null);
        });
    }

    public void clothesClose() {
        runOnUiThread(this::closeClothesPage);
    }

    private void openClothesPage(String json) {
        if (hudWebView == null || isFinishing()) return;
        Log.i(TAG, "[clothes] ouverture du panneau HTML");
        clothesPageOpen = true;
        clothesReady = false;
        configOpen = true;
        pendingClothesJson = json;
        hudWebView.setWheelOpen(false);
        hudWebView.setPhonePage(false);
        // Pas de page plein ecran : seul le panneau capte le tactile (zones envoyees par la page),
        // le reste de l'ecran laisse passer vers le jeu pour voir le personnage.
        hudWebView.setFullscreenPage(false);
        hudWebView.setInteractive(false);
        hudWebView.setTouchBounds(null);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/clothes/index.html");

        // Garde-fou : si la page ne s'affiche pas en 5 s, on la ferme et le moteur retombe sur le panneau natif.
        cancelClothesWatchdog();
        clothesWatchdog = () -> {
            if (clothesPageOpen && !clothesReady) {
                Log.e(TAG, "[clothes] la page ne s'est pas affichee a temps -> panneau natif");
                try {
                    clothesCommand(4, "", 0);
                } catch (UnsatisfiedLinkError e) {
                    Log.e(TAG, "[clothes] natif indisponible : " + e.getMessage());
                }
                closeClothesPage();
            }
        };
        clothesHandler.postDelayed(clothesWatchdog, 5000);
    }

    private void cancelClothesWatchdog() {
        if (clothesWatchdog != null) {
            clothesHandler.removeCallbacks(clothesWatchdog);
            clothesWatchdog = null;
        }
    }

    private void closeClothesPage() {
        if (!clothesPageOpen) return;
        cancelClothesWatchdog();
        clothesPageOpen = false;
        clothesReady = false;
        pendingClothesJson = null;
        configOpen = false;
        if (hudWebView == null) return;
        hudWebView.setTouchBounds(null);
        hudWebView.setFullscreenPage(false);
        hudWebView.setPhonePage(false);
        hudWebView.setVisibility(View.VISIBLE);
        navigate("file:///android_asset/hud/index.html");
    }

    private void sendClothesCommand(int type, String key, int value) {
        try {
            clothesCommand(type, key == null ? "" : key, value);
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "[clothes] natif indisponible : " + e.getMessage());
        }
    }

    private final class SampClothesBridge {
        private final HudWebView owner;   // couche WebView qui a cree ce pont
        SampClothesBridge(HudWebView owner) { this.owner = owner; }
        @JavascriptInterface public void camera(String action, int value) {
            if (!clothesPageOpen) return;
            if (!"rotate".equals(action) && !"focus".equals(action) && !"zoom".equals(action)) return;
            sendClothesCommand(5, action, value);
        }

        // JS -> moteur : change une option ("topStyle", "topColor", ...)
        @JavascriptInterface public void setOption(String key, int value) {
            if (key == null || !key.matches("[A-Za-z]{1,24}")) return;
            sendClothesCommand(0, key, value);
        }

        // JS -> moteur : 0 homme / 1 femme
        @JavascriptInterface public void setGender(int gender) {
            sendClothesCommand(1, "", (gender >= 0 && gender <= 4) ? gender : 0);   // 0 homme, 1 femme, 2 homme 2 (289), 3 femme 2 (12), 4 homme 3 (7)
        }

        @JavascriptInterface public void save() {
            sendClothesCommand(2, "", 0);
            runOnUiThread(() -> closeClothesPage());
        }

        @JavascriptInterface public void cancel() {
            sendClothesCommand(3, "", 0);
            runOnUiThread(() -> closeClothesPage());
        }

        // Zones tactiles du panneau ("viewport=W,H;panel=l,t,r,b") : le reste de l'ecran reste jouable.
        @JavascriptInterface public void setTouchBounds(String raw) {
            if (owner != null && clothesPageOpen) owner.setTouchBounds(raw);
        }

        // Appele par la page quand elle est prete : annule le garde-fou et envoie l'etat en attente.
        @JavascriptInterface public void ready() {
            clothesReady = true;
            runOnUiThread(() -> {
                cancelClothesWatchdog();
                String json = pendingClothesJson;
                pendingClothesJson = null;
                if (json != null && hudWebView != null && clothesPageOpen) {
                    hudWebView.evaluateJavascript("window.ClothesApp && ClothesApp.onState(" + JSONObject.quote(json) + ")", null);
                }
            });
            Log.i(TAG, "[clothes] page affichee");
        }
    }

    private void closeConfig() {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            if (authOpen) return;
            clothesPageOpen = false;
            invOpen = false;
            adminOpen = false;
            lockerOpen = false;
            policeOpen = false;
            shopOpen = false;
            jobOpen = false;
            uiOpen = false;
            govOpen = false;
            facOpen = false;
            gangOpen = false;
            fishOpen = false;
            garageOpen = false;
            houseOpen = false;
            rpOpen = false;
            reportOpen = false;
            configOpen = false;
            pharmacyOpen = false;
            bankOpen = false;
            cameraOpen = false;
            hudWebView.setFullscreenPage(false);
            hudWebView.setPhonePage(false);
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html");
        });
    }

    @Override
    public void onBackPressed() {
        // Dialogue SA-MP ouvert (page assets/dialog) : retour = bouton droit
        if (mDialog != null && mDialog.onBackPressed()) {
            return;
        }
        if (authOpen) {
            // la connexion est obligatoire : le bouton retour ne ferme pas la page
            return;
        }
        if (adminOpen) {
            closeAdminPage(true);
            return;
        }
        if (reportOpen) {
            closeReportPage(true);
            return;
        }
        if (policeOpen) {
            closePolicePage(true);
            return;
        }
        if (lockerOpen) {
            closeLockerPage(true);
            return;
        }
        if (shopOpen) {
            closeShopPage(true);
            return;
        }
        if (jobOpen) {
            closeJobPage(true);
            return;
        }
        if (uiOpen) {
            closeUiPage(true);
            return;
        }
        if (svcOpen) {
            closeSvcPage(true);
            return;
        }
        if (mapsOpen) {
            closeMapsPage(true);
            return;
        }
        if (jblOpen) {
            closeJblPage(true);
            return;
        }
        if (govOpen) {
            closeGovPage(true);
            return;
        }
        if (houseOpen) {
            closeHousePage(true);
            return;
        }
        if (fishOpen) {
            closeFishPage(true);
            return;
        }
        if (garageOpen) {
            closeGaragePage(true);
            return;
        }
        if (gangOpen) {
            closeGangPage(true);
            return;
        }
        if (facOpen) {
            closeFacPage(true);
            return;
        }
        if (rpOpen) {
            closeRpPage(true);
            return;
        }
        if (invOpen) {
            closeInvPage(true);
            return;
        }
        if (bankOpen) {
            closeBankPage(true);
            return;
        }
        if (clothesPageOpen) {
            sendClothesCommand(3, "", 0); // retour = ANNULER
            closeClothesPage();
            return;
        }
        if (cameraOpen) {
            closePhoneCamera();
            return;
        }
        if (pharmacyOpen) {
            closePharmacy();
            return;
        }
        if (configOpen) {
            closeConfig();
            return;
        }
        // [32] comportement d'origine : le moteur gere le retour (menu pause, clavier...)
        try {
            onEventBackPressed();
        } catch (UnsatisfiedLinkError ignored) { }
    }

    public void updateHudState(float health, float armour, float thirst, float hunger, float energy, boolean dead, boolean running, boolean exhausted, boolean inVehicle, float speed, float vehHealth, boolean handbrake,
                               float posX, float posY, float camFx, float camFy, float heading) {
        lastPosX = posX;
        lastPosY = posY;
        hasLastPos = true;
        String json = "{\"health\":" + health + ",\"armour\":" + armour + ",\"thirst\":" + hudThirst + ",\"hunger\":" + hudHunger + ",\"energy\":" + energy + ",\"dead\":" + dead + ",\"running\":" + running + ",\"exhausted\":" + exhausted + ",\"inVehicle\":" + inVehicle + ",\"speed\":" + speed + ",\"vehHealth\":" + vehHealth + ",\"fuel\":" + hudFuel + ",\"handbrake\":" + handbrake
                + ",\"posX\":" + posX + ",\"posY\":" + posY
                + ",\"camFx\":" + camFx + ",\"camFy\":" + camFy + ",\"heading\":" + heading + "}";
        updateHudState(json);
    }

    /** Argent en poche (appele par le natif a chaque changement). */
    private volatile int hudMoney = Integer.MIN_VALUE;
    public void updateHudMoney(final int money) {
        hudMoney = money;
        hudJs("window.HudApp && HudApp.updateHudInfo && HudApp.updateHudInfo({money:" + money + "})");
    }

    public void updateHudState(final String json) {
        if (hudWebView == null || json == null || isFinishing()) return;
        // Premier etat du joueur recu du moteur = le jeu tourne : on ferme l'ecran de chargement (une seule fois)
        if (!loadingClosedByGame) {
            loadingClosedByGame = true;
            hideLoadingScreen();
        }
        if (!gameSettingsApplied) {          // [v15] le jeu tourne : on applique les parametres enregistres
            gameSettingsApplied = true;
            applySavedGameSettings();
        }
        final long now = SystemClock.uptimeMillis();
        if (json.equals(lastHudJson) && now - lastHudDispatchMs < 250L) return;
        lastHudJson = json;
        lastHudDispatchMs = now;
        runOnUiThread(() -> {
            if (hudWebView != null && !isFinishing()) {
                hudWebView.evaluateJavascript("(function(j){window.HudBridge && HudBridge.setPlayerState(j);"
                        + "if (window.MapApp && j.posX !== undefined) MapApp.setPlayer(j.posX, j.posY);})(" + json + ")", null);
            }
        });
    }

    public void updateHudNeeds(final int hunger, final int thirst) {
        hudHunger = Math.max(0, Math.min(100, hunger));
        hudThirst = Math.max(0, Math.min(100, thirst));
        String json = "{\"hunger\":" + hudHunger + ",\"thirst\":" + hudThirst + "}";
        updateHudState(json);
        runOnUiThread(() -> {
            if (pharmacyOpen && hudWebView != null) {
                hudWebView.evaluateJavascript("window.PharmacyBridge && PharmacyBridge.setState(" + json.replace("hunger", "hunger").replace("thirst", "thirst") + ")", null);
            }
        });
    }

    public void updatePharmacyState(final int balance, final int hunger, final int thirst) {
        pharmacyBalance = Math.max(0, balance);
        hudHunger = Math.max(0, Math.min(100, hunger));
        hudThirst = Math.max(0, Math.min(100, thirst));
        runOnUiThread(() -> {
            if (!pharmacyOpen || hudWebView == null) return;
            String json = "{\"balance\":" + Math.max(0, balance) + ",\"hunger\":" + Math.max(0, hunger) + ",\"thirst\":" + Math.max(0, thirst) + "}";
            hudWebView.evaluateJavascript("window.PharmacyBridge && PharmacyBridge.setState(" + json + ")", null);
        });
    }

    private String payloadField(final String payload, final String field) {
        try {
            JSONObject object = new JSONObject(payload == null ? "{}" : payload);
            return object.optString(field, "");
        } catch (Exception ignored) {
            return "";
        }
    }

    private final class HudAndroidBridge {
        private final HudWebView owner;
        HudAndroidBridge(HudWebView owner) { this.owner = owner; }
        /** [Crestwood] joystick du HUD (appel direct, sans JSON : jusqu'a 60 fois / seconde). */
        @JavascriptInterface
        public void padStick(int x, int y, boolean active) {
            if (hudStickUnavailable) return;
            try {
                setHudStick(x, y, active);
            } catch (UnsatisfiedLinkError e) {
                hudStickUnavailable = true;
                Log.e(TAG, "setHudStick indisponible : " + e.getMessage());
            } catch (Throwable t) {
                Log.e(TAG, "padStick", t);
            }
        }

        @JavascriptInterface
        public void onHudAction(final String action, final String payload) {
            if ("chat:open".equals(action)) {
                runOnUiThread(() -> openChatInput());
                return;
            }
            if ("config:open".equals(action)) {
                openConfig();
                return;
            }
            if ("phone:open".equals(action)) {
                openPhonePage();
                return;
            }
            if ("camera:open".equals(action)) {
                openPhoneCamera();
                return;
            }
            if ("blazer:open".equals(action)) {
                openInteractionPage("blazer/index.html");
                return;
            }
            if ("map:open".equals(action)) {
                openInteractionPage("map/index.html");
                return;
            }
            if ("map:gps".equals(action)) {
                // payload {"x":..,"y":..} = point choisi sur la carte (efface a l'arrivee), {"clear":1} = effacer
                try {
                    JSONObject o = new JSONObject(payload == null || payload.isEmpty() ? "{}" : payload);
                    if (o.has("x") && o.has("y")) {
                        JSONObject g = new JSONObject();
                        g.put("x", o.getDouble("x"));
                        g.put("y", o.getDouble("y"));
                        g.put("auto", 1);
                        lastGpsJson = g.toString();
                    } else {
                        lastGpsJson = null;
                        sendHudCommand(CMD_MAP);   // /gps : le serveur remet le GPS de la mission en cours s'il y en a une
                    }
                } catch (Exception e) {
                    Log.e(TAG, "map:gps", e);
                }
                closeConfig();
                return;
            }
            if ("gps:arrived".equals(action)) {
                lastGpsJson = null;
                return;
            }
            if ("settings:close".equals(action)) {
                closeConfig();
                return;
            }
            if ("settings:aspect".equals(action)) {
                applyAspect(payloadValue(payload));
                return;
            }
            if ("settings:resolution".equals(action)) {
                applyResolution(payloadValue(payload));
                return;
            }
            if ("settings:resolution-scale".equals(action)) {
                applyResolutionScale(payloadValue(payload));
                return;
            }
            if ("settings:fps".equals(action) || "settings:toggle".equals(action) || "settings:slider".equals(action)) {
                getSharedPreferences("display_settings", MODE_PRIVATE).edit()
                        .putString(action, payload == null ? "" : payload).apply();
                return;
            }
            if ("pharmacy:close".equals(action)) {
                closePharmacy();
                return;
            }
            if ("pharmacy:buy".equals(action)) {
                String item = payloadField(payload, "item");
                if (!item.isEmpty()) runOnUiThread(() -> sendChatInput("/farmbuy " + item));
                return;
            }
            if ("vehicle:mode".equals(action)) {
                boolean active = payload != null && payload.contains("\"active\":true");
                if (owner != null) owner.setVehicleMode(active);
                return;
            }
            if ("vehicle:control".equals(action)) {
                dispatchVehicleControl(payload);
                return;
            }
            if ("settings:set".equals(action)) {      // [v15] page Parametres (style GTA V)
                applyGameSettingPref(payloadField(payload, "key"), payloadField(payload, "value"), true);
                return;
            }
            if ("settings:reset".equals(action)) {
                android.util.DisplayMetrics resetMetrics = getResources().getDisplayMetrics();
                applyGameSurfaceLayout(Math.max(640, resetMetrics.widthPixels), Math.max(360, resetMetrics.heightPixels));
                getSharedPreferences("display_settings", MODE_PRIVATE).edit().clear().apply();
                for (String k : GAME_SETTING_KEYS) applyGameSettingPref(k, String.valueOf(gameSettingDefault(k)), false);
                return;
            }
            if (owner != null) {
                if ("wheel:open".equals(action)) owner.setWheelOpen(true);
                if ("wheel:close".equals(action) || "wheel:select".equals(action)) owner.setWheelOpen(false);
            }
            Log.d(TAG, "HUD action=" + action + " payload=" + payload);
        }
    }


    // ------------------------------------------------------------------
    // Caméra du téléphone Crestwood (capture de l'écran de jeu -> galerie Photos du téléphone)
    // ------------------------------------------------------------------
    private void openPhoneCamera() {
        runOnUiThread(() -> {
            if (hudWebView == null) return;
            configOpen = true;
            pharmacyOpen = false;
            cameraOpen = true;
            hudWebView.setWheelOpen(false);
            hudWebView.setPhonePage(false);
            hudWebView.setFullscreenPage(false);
            hudWebView.setCameraPage(true); // viseur transparent : le tactile passe au jeu (glisser pour regarder)
            hudWebView.setVisibility(View.VISIBLE);
            navigate("file:///android_asset/hud/index.html?camera=phone");
        });
    }

    /** Ferme la caméra et revient sur le téléphone (la page Photos s'ouvre si une photo a été prise). */
    private void closePhoneCamera() {
        runOnUiThread(() -> {
            cameraOpen = false;
            sendHudCommand("/phoneact cam off");   // remet la camera du jeu derriere le joueur (fin du selfie)
            openPhonePage();
        });
    }

    /** Appele par le moteur (Chat::addMessage) : message du chat vers le chat HTML du HUD. */
    public void hudChatMessage(final byte[] nickBytes, final byte[] textBytes, final int rgb) {
        try {
            final String nick = nickBytes == null ? "" : new String(nickBytes, java.nio.charset.StandardCharsets.UTF_8);
            final String text = textBytes == null ? "" : new String(textBytes, java.nio.charset.StandardCharsets.UTF_8);
            JSONObject o = new JSONObject();
            o.put("n", nick);
            o.put("t", text);
            o.put("c", String.format("#%06X", rgb & 0xFFFFFF));
            hudJs("window.HudApp && HudApp.addChatMessage && HudApp.addChatMessage(" + JSONObject.quote(o.toString()) + ")");
        } catch (Exception ignored) { }
    }

    // ------------------------------------------------------------------
    // [v15] Parametres du jeu (page assets/config) : enregistres dans "display_settings" et appliques
    // au moteur (game/CwSettings.cpp) a chaque changement et au demarrage de la partie.
    // ------------------------------------------------------------------
    private static final String[] GAME_SETTING_KEYS = {"fps_counter", "fps_limit", "draw_dist", "post_fx", "voice_btn", "cw_buttons", "res_scale", "gfx_preset", "hud_scale"};

    private int gameSettingDefault(String key) {
        switch (key) {
            case "fps_counter": return 0;
            case "fps_limit": return 120;
            case "draw_dist": return 100;
            case "post_fx": return 1;
            case "voice_btn": return 1;
            case "cw_buttons": return 1;
            case "res_scale": return 100;
            case "gfx_preset": return 2;
            case "hud_scale": return 100;
            default: return 0;
        }
    }

    private String gameSettingsJson() {
        SharedPreferences sp = getSharedPreferences("display_settings", MODE_PRIVATE);
        JSONObject o = new JSONObject();
        try {
            for (String k : GAME_SETTING_KEYS) o.put(k, sp.getInt("cw_" + k, gameSettingDefault(k)));
            o.put("aspect", sp.getString("aspect", "16:9"));
            android.util.DisplayMetrics m = getResources().getDisplayMetrics();
            o.put("screen", m.widthPixels + "x" + m.heightPixels);
        } catch (Exception ignored) { }
        return o.toString();
    }

    private void applyGameSettingPref(String key, String rawValue, boolean save) {
        if (key == null || key.isEmpty()) return;
        int value;
        try { value = Integer.parseInt(rawValue == null ? "" : rawValue.trim()); } catch (Exception e) { return; }
        if (save) getSharedPreferences("display_settings", MODE_PRIVATE).edit().putInt("cw_" + key, value).apply();
        try {
            switch (key) {
                case "fps_counter": applyGameSetting(1, value); break;
                case "fps_limit": applyGameSetting(2, value); break;
                case "draw_dist": applyGameSetting(3, value); break;
                case "post_fx": applyGameSetting(4, value); break;
                case "voice_btn": applyGameSetting(5, value); break;
                case "cw_buttons": applyGameSetting(6, value); break;
                case "res_scale": { final String v = String.valueOf(value); runOnUiThread(() -> applyResolutionScale(v)); break; }
                case "hud_scale": hudJs("window.HudApp && HudApp.setHudScale && HudApp.setHudScale(" + value + ")"); break;
                default: break;
            }
        } catch (Throwable t) {
            Log.w(TAG, "[settings] " + key + " non applique", t);
        }
    }

    /** Applique les parametres enregistres (une fois, quand la partie tourne). */
    private void applySavedGameSettings() {
        SharedPreferences sp = getSharedPreferences("display_settings", MODE_PRIVATE);
        for (String k : GAME_SETTING_KEYS) {
            if ("gfx_preset".equals(k)) continue;
            // la limite FPS / la distance / l'echelle ne sont forcees que si le joueur les a changees
            if (("fps_limit".equals(k) || "draw_dist".equals(k) || "res_scale".equals(k) || "post_fx".equals(k)) && !sp.contains("cw_" + k)) continue;
            applyGameSettingPref(k, String.valueOf(sp.getInt("cw_" + k, gameSettingDefault(k))), false);
        }
    }

    private void hudJs(final String script) {
        runOnUiThread(() -> {
            if (hudWebView != null && !isFinishing()) hudWebView.evaluateJavascript(script, null);
        });
    }

    private void cameraStatus(String status, String url, String message) {
        hudJs("window.HudApp && HudApp.rgCameraNativeStatus("
                + JSONObject.quote(status) + "," + JSONObject.quote(url == null ? "" : url) + ","
                + JSONObject.quote(message == null ? "" : message) + ")");
    }

    /** Copie la zone demandée (coordonnées CSS du HUD) depuis la SurfaceView du jeu. */
    private void captureGameRegion(final int left, final int top, final int width, final int height, final int innerW, final int innerH) {
        runOnUiThread(() -> {
            View gameView = GetSurfaceView();   // [32]
            if (!(gameView instanceof SurfaceView) || hudWebView == null || innerW <= 0 || innerH <= 0 || width <= 0 || height <= 0) {
                cameraStatus("error", "", "Impossible de capturer l'image.");
                return;
            }
            final SurfaceView game = (SurfaceView) gameView;
            Rect frame = game.getHolder().getSurfaceFrame();
            int surfaceW = frame.width();
            int surfaceH = frame.height();
            if (surfaceW <= 0 || surfaceH <= 0 || game.getWidth() <= 0 || game.getHeight() <= 0) {
                cameraStatus("error", "", "Le jeu n'est pas prêt.");
                return;
            }
            float sx = hudWebView.getWidth() / (float) innerW;
            float sy = hudWebView.getHeight() / (float) innerH;
            float fx0 = (left * sx - game.getLeft()) / game.getWidth();
            float fy0 = (top * sy - game.getTop()) / game.getHeight();
            float fx1 = ((left + width) * sx - game.getLeft()) / game.getWidth();
            float fy1 = ((top + height) * sy - game.getTop()) / game.getHeight();
            int x0 = Math.max(0, Math.min(surfaceW - 1, Math.round(fx0 * surfaceW)));
            int y0 = Math.max(0, Math.min(surfaceH - 1, Math.round(fy0 * surfaceH)));
            int x1 = Math.max(x0 + 2, Math.min(surfaceW, Math.round(fx1 * surfaceW)));
            int y1 = Math.max(y0 + 2, Math.min(surfaceH, Math.round(fy1 * surfaceH)));
            final Rect src = new Rect(x0, y0, x1, y1);
            int outW = Math.min(src.width(), 640);
            int outH = Math.max(1, Math.round(outW * (src.height() / (float) src.width())));
            final Bitmap bitmap = Bitmap.createBitmap(outW, outH, Bitmap.Config.ARGB_8888);
            cameraStatus("capturing", "", "");
            try {
                PixelCopy.request(game, src, bitmap, result -> {
                    if (result != PixelCopy.SUCCESS) {
                        bitmap.recycle();
                        cameraStatus("error", "", "Capture impossible, réessaie.");
                        return;
                    }
                    new Thread(() -> {
                        try {
                            ByteArrayOutputStream out = new ByteArrayOutputStream();
                            bitmap.compress(Bitmap.CompressFormat.JPEG, 72, out);
                            bitmap.recycle();
                            String url = "data:image/jpeg;base64," + Base64.encodeToString(out.toByteArray(), Base64.NO_WRAP);
                            cameraStatus("captured", url, "");
                        } catch (Throwable t) {
                            cameraStatus("error", "", "Impossible d'enregistrer la photo.");
                        }
                    }, "hud-photo").start();
                }, new Handler(Looper.getMainLooper()));
            } catch (Throwable t) {
                Log.e(TAG, "PixelCopy failed", t);
                cameraStatus("error", "", "Capture impossible sur cet appareil.");
            }
        });
    }

    // ------------------------------------------------------------------
    // Pont natif du HUD Crestwood (window.SampHud côté JavaScript)
    // ------------------------------------------------------------------

    /**
     * Commandes de chat envoyées au serveur par les boutons du HUD.
     * ADAPTE CES VALEURS aux commandes réelles de ton gamemode PAWN ("" = ne rien envoyer).
     */
    private static final String CMD_ENGINE = "/moteur";
    private static final String CMD_INTERACT = "/interagir";
    private static final String CMD_VEHICLE_LOCK = "/lock";
    private static final String CMD_ANIMATIONS = "/anim";
    private static final String CMD_INVENTORY = "/inventaire";
    private static final String CMD_MAP = "/gps";
    private static final String CMD_STORE = "/boutique";
    private static final String CMD_AUDIO = "";
    private static final String CMD_CALL_ACCEPT = "/phoneact accept";
    private static final String CMD_CALL_REJECT = "/phoneact decline";
    /** Les actions "système" (boutique, objectifs, documents, Faggio...) passent par "/hudaction <action> <données>". */
    private static final String CMD_SYSTEM = "/hudaction";

    private void sendHudCommand(final String command) {
        if (command == null || command.trim().isEmpty()) return;
        runOnUiThread(() -> sendChatInput(command.trim()));
    }

    private SharedPreferences identityPrefs() {
        return getSharedPreferences("crestwood_hud_identity", MODE_PRIVATE);
    }

    // ---- Micro (vocal SampVoice) : autorisation Android RECORD_AUDIO ----
    private boolean hasMicPermission() {
        if (android.os.Build.VERSION.SDK_INT < 23) return true;
        return checkSelfPermission(android.Manifest.permission.RECORD_AUDIO) == android.content.pm.PackageManager.PERMISSION_GRANTED;
    }

    private void requestMicPermission() {
        if (android.os.Build.VERSION.SDK_INT < 23 || hasMicPermission()) return;
        try {
            requestPermissions(new String[] { android.Manifest.permission.RECORD_AUDIO }, 7701);
        } catch (Exception e) {
            Log.e(TAG, "micro", e);
        }
    }

    private final class SampHudBridge {
        private final HudWebView owner;
        SampHudBridge(HudWebView owner) { this.owner = owner; }
        // Bouton micro du HUD (vocal de proximite) : appuyer = parler, relacher = couper
        @JavascriptInterface public void voiceTalk(boolean on) {
            if (on && !hasMicPermission()) {
                runOnUiThread(() -> requestMicPermission());
                return;
            }
            try { setVoiceTalk(on); } catch (Throwable e) { Log.e(TAG, "voiceTalk", e); }
        }

        @JavascriptInterface public boolean voiceAllowed() {
            return hasMicPermission();
        }

        @JavascriptInterface public void ready() {
            SharedPreferences launcher = getSharedPreferences("crestwood_launcher", MODE_PRIVATE);
            String nick = launcher.getString("saved_nickname", "");
            JSONObject info = new JSONObject();
            try {
                if (nick != null && !nick.isEmpty()) info.put("name", nick);
                info.put("ready", 1);
            } catch (Exception ignored) { }
            hudJs("window.HudApp && HudApp.openFromNative(" + info + ")");
            if (hudMoney != Integer.MIN_VALUE) {
                hudJs("window.HudApp && HudApp.updateHudInfo && HudApp.updateHudInfo({money:" + hudMoney + "})");
            }
            if (!authBootstrapDone) {
                authBootstrapDone = true;
                runOnUiThread(() -> requestAuthOpenLater(2500));
            }
            final String objNow = lastObjJson;
            if (objNow != null) hudJs("window.ObjTracker && ObjTracker.set(" + JSONObject.quote(objNow) + ")");
            final String hotNow = lastHotbarJson;
            if (hotNow != null) hudJs("window.CwHotbar && CwHotbar.set(" + JSONObject.quote(hotNow) + ")");
            final String hudvNow = lastHudvJson;   // [v15] zone verte / ceinture / carburant
            if (hudvNow != null) hudJs("window.HudApp && HudApp.updateHudInfo && HudApp.updateHudInfo(" + hudvNow + ")");
            final String pendingInteract = lastInteractJson;
            if (pendingInteract != null) {
                hudJs("window.HudBridge && HudBridge.setInteract && HudBridge.setInteract(" + JSONObject.quote(pendingInteract) + ")");
            }
        }

        @JavascriptInterface public void setTouchBounds(String raw) {
            if (owner != null) owner.setTouchBounds(raw);
        }

        @JavascriptInterface public void setInteractionState(boolean chatOpen, boolean menuOpen) {
            if (owner != null) owner.setInteractive(chatOpen || menuOpen);
        }

        @JavascriptInterface public void setHudEditorOpen(boolean open) {
            if (owner != null) owner.setInteractive(open);
        }

        @JavascriptInterface public void setIdentityOpen(boolean open) {
            if (owner != null) owner.setInteractive(open);
        }

        @JavascriptInterface public void setIdentitySigning(boolean signing) {
            if (owner != null) owner.setInteractive(signing);
        }

        @JavascriptInterface public void showAndroidKeyboard() {
            runOnUiThread(() -> { if (owner != null) owner.showKeyboard(); });
        }

        @JavascriptInterface public void hideAndroidKeyboard() {
            runOnUiThread(() -> { if (owner != null) owner.hideKeyboard(); });
        }

        @JavascriptInterface public void sendChatMessage(String message) {
            if (message == null) return;
            final String clean = message.trim();
            if (!clean.isEmpty()) runOnUiThread(() -> sendChatInput(clean));
        }

        @JavascriptInterface public void menuAction(String action) {
            if (action == null) return;
            switch (action) {
                case "phone": openPhonePage(); break;
                case "settings": openConfig(); break;
                case "garage": sendHudCommand("/garage"); break;   // [v15b] roue -> garage (partout)
                case "inventory": sendHudCommand(CMD_INVENTORY); break;
                case "map": openInteractionPage("map/index.html"); break;
                case "audio": sendHudCommand(CMD_AUDIO); break;
                // casque du menu rotatif : support / report (la page s'ouvre tout de suite, le serveur la remplit)
                case "report": runOnUiThread(() -> { openReportPage("{\"loading\":1}"); sendHudCommand("/repact open"); }); break;
                case "store": sendHudCommand(CMD_STORE); break;
                case "records": openInteractionPage("blazer/index.html"); break;
                // bouton RP du HUD : ouvre la tablette RP (le serveur la remplit)
                case "rp": runOnUiThread(() -> { openRpPage("{\"v\":\"hub\",\"loading\":1}"); sendHudCommand("/rpact open"); }); break;
                case "clothes": runOnUiThread(() -> { try { openClothesTest(); } catch (UnsatisfiedLinkError e) { Toast.makeText(SAMP.this, "Vêtements indisponibles sur cette version.", Toast.LENGTH_SHORT).show(); } }); break;
                default: Log.d(TAG, "menuAction inconnue: " + action);
            }
        }

        @JavascriptInterface public void systemAction(String action, String payload) {
            if (action == null || action.isEmpty()) return;
            if ("close".equals(action)) return;
            sendHudCommand(CMD_SYSTEM + " " + action + (payload == null || payload.isEmpty() ? "" : " " + payload));
        }

        @JavascriptInterface public void engineAction(String action) { sendHudCommand(CMD_ENGINE); }
        /** Tap sur l'icone de main : le serveur choisit lui-meme la cible la plus proche (banque, vehicule...). */
        @JavascriptInterface public void interactAction() { sendHudCommand(CMD_INTERACT); }
        @JavascriptInterface public void vehicleLockAction() { sendHudCommand(CMD_VEHICLE_LOCK); }
        @JavascriptInterface public void openAnimations() { sendHudCommand(CMD_ANIMATIONS); }

        @JavascriptInterface public void radioAction(String action, String value) {
            if (action == null || action.isEmpty()) return;
            sendHudCommand("/radioact " + action + (value == null || value.isEmpty() ? "" : " " + value));
        }

        /** Fleches de l'arme du HUD : arme precedente / suivante (touches SA-MP natives, voir game/pad.cpp). */
        @JavascriptInterface public void weaponAction(String action) {
            if ("next".equals(action)) dispatchVehicleControl("{\"control\":\"weaponNext\",\"pressed\":true}");
            else if ("prev".equals(action)) dispatchVehicleControl("{\"control\":\"weaponPrev\",\"pressed\":true}");
        }

        @JavascriptInterface public void phoneCallAction(String action, String phone) {
            // notification d'appel du HUD : decrocher ouvre le telephone sur l'ecran d'appel (rendu par la synchro)
            boolean accept = "accept".equals(action);
            sendHudCommand(accept ? CMD_CALL_ACCEPT : CMD_CALL_REJECT);
            if (accept) openPhonePage();
        }

        /** Camera du telephone : back (camera normale), selfie, left/right (tourner), in/out (zoom). */
        @JavascriptInterface public void phoneCamMode(String mode) {
            String m = mode == null ? "" : mode.replaceAll("[^a-z]", "");
            if (m.isEmpty() || m.length() > 7) return;
            sendHudCommand("/phoneact cam " + m);
        }

        // ---- Carte d'identité : stockage local (SharedPreferences) ----
        @JavascriptInterface public void saveIdentityPhoto(String value) {
            identityPrefs().edit().putString("photo", value == null ? "" : value).apply();
        }

        @JavascriptInterface public void saveIdentitySignature(String value) {
            identityPrefs().edit().putString("signature", value == null ? "" : value).apply();
        }

        @JavascriptInterface public void requestIdentityAssets() {
            SharedPreferences prefs = identityPrefs();
            String photo = prefs.getString("photo", "");
            String signature = prefs.getString("signature", "");
            try {
                if (!photo.isEmpty()) {
                    JSONObject event = new JSONObject().put("type", "photo").put("value", photo);
                    hudJs("window.HudApp && HudApp.receiveIdentityEvent(" + event + ")");
                }
                if (!signature.isEmpty()) {
                    hudJs("window.HudApp && HudApp.receiveIdentityEvent(" + new JSONObject().put("type", "signature_start").put("chunks", 1) + ")");
                    hudJs("window.HudApp && HudApp.receiveIdentityEvent(" + new JSONObject().put("type", "signature_chunk").put("idx", 0).put("value", signature) + ")");
                    hudJs("window.HudApp && HudApp.receiveIdentityEvent(" + new JSONObject().put("type", "signature_end") + ")");
                }
            } catch (Exception ignored) { }
        }

        // ---- Caméra (carte d'identité et téléphone) ----
        @JavascriptInterface public void openRgCamera() { }
        @JavascriptInterface public void closeRgCamera() { }

        @JavascriptInterface public void captureRgPhoto(int left, int top, int width, int height, int innerWidth, int innerHeight) {
            captureGameRegion(left, top, width, height, innerWidth, innerHeight);
        }

        /** Le déplacement du cadre pilote la caméra du jeu : non branché côté moteur natif pour l'instant. */
        @JavascriptInterface public void adjustRgCamera(int dx, int dy) { }

        /** Appelé par le HUD quand la caméra du téléphone se ferme (photo prise ou annulée). */
        @JavascriptInterface public void phoneCameraDone(boolean saved) { closePhoneCamera(); }
    }

    private void dispatchVehicleControl(String payload) {
        try {
            JSONObject data = new JSONObject(payload == null ? "{}" : payload);
            String control = data.optString("control", "");
            boolean pressed = data.optBoolean("pressed", false);
            int id;
            switch (control) {
                case "left": id = 1; break;
                case "right": id = 2; break;
                case "accelerate": id = 3; break;
                case "brake": id = 4; break;
                case "handbrake": id = 5; break;
                case "horn": id = 6; break;
                case "sprint": id = 7; break;
                case "jump": id = 8; break;
                case "crouch": id = 9; break;
                case "enter": id = 10; break;
                case "fire": id = 11; break;
                case "aim": id = 12; break;
                case "weaponNext": id = 13; break;
                case "weaponPrev": id = 14; break;
                case "passenger": id = 15; break;
                case "customControls": id = 20; break;
                case "release": id = 0; break;
                default: return;
            }
            setVehicleControl(id, pressed);
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "setVehicleControl indisponible : " + e.getMessage());
        } catch (Throwable t) {
            Log.e(TAG, "dispatchVehicleControl", t);
        }
    }

    @Override
    public void onHeightChanged(int orientation, int height) { }
}
