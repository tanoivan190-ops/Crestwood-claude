package com.crestwood.ui;

import android.app.Activity;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.gta.game.R;

import org.json.JSONObject;

/**
 * [Crestwood v36] HUD natif (etape 1 de la sortie du HTML) : compteur + panneau d'infos.
 * Utilise par les DEUX activites (com.gta.game.SAMP 64 bits et com.gta.game32.SAMP 32 bits).
 *
 * Les valeurs arrivent du fil du jeu : elles sont gardees ici puis UNE seule mise a jour est postee
 * sur le fil de l'interface (pas de JSON, pas de evaluateJavascript a chaque image comme avant).
 * Le HUD natif suit la visibilite de la couche WebView du HUD : il se cache quand un panneau plein
 * ecran (banque, telephone, boutique...) remplace le HUD, et revient avec lui.
 */
public final class NativeHud {
    /** script injecte dans la page HTML du HUD : cache les parties devenues natives */
    public static final String HIDE_HTML_JS =
            "(function(){var d=document.documentElement;if(d.classList.contains('cw-native-hud'))return;"
            + "d.classList.add('cw-native-hud');window.CwNativeHud=1;"
            // [v53] on GARDE notre compteur (speedo2, page HTML) : le compteur natif du dev est desactive
            // [v37] minimap native : la minimap HTML ne calcule plus rien (sinon deux GPS en meme temps)
            + "window.Minimap={update:function(){}};"
            + "window.MinimapBridge={setBlips:function(){},setWaypoint:function(){},clearWaypoint:function(){},setZoom:function(){},distanceLeft:function(){return -1;}};"
            // option "minimap masquee" de la roue -> minimap native
            + "var sync=function(){try{window.SampHud&&SampHud.nativeMinimap&&SampHud.nativeMinimap(!document.body.classList.contains('cw-minimap-off'));}catch(e){}};"
            + "sync();new MutationObserver(sync).observe(document.body,{attributes:true,attributeFilter:['class']});"
            + "try{window.crestwoodSyncTouchBounds&&window.crestwoodSyncTouchBounds();}catch(e){}})();";

    private final Activity act;
    private final View root;
    private final SpeedoView speedo;
    private final InfoPanelView info;
    private final MinimapView minimap;
    private final View hudLayer;
    private final Handler ui = new Handler(Looper.getMainLooper());

    // derniers etats (ecrits par le fil du jeu, lus sur le fil UI)
    private volatile float health = 100, armour, hunger = 100, thirst = 100, energy = 100, speed, vehHealth = 100, posX, posY;
    private volatile float camFx, camFy = 1, heading;
    private volatile boolean minimapOn = true;
    private volatile int fuel = 100, money = Integer.MIN_VALUE, coins = Integer.MIN_VALUE, account, wanted;
    private volatile boolean inVehicle, dead, belt = true, gotState;
    private volatile String forcedLocation;
    private volatile boolean pending;
    private boolean lastShown;

    private final Runnable apply = new Runnable() {
        @Override public void run() { pending = false; applyNow(); }
    };
    private final Runnable watch = new Runnable() {
        @Override public void run() { syncVisibility(); ui.postDelayed(this, 200); }
    };

    private NativeHud(Activity act, View root, View hudLayer) {
        this.act = act;
        this.root = root;
        this.hudLayer = hudLayer;
        this.speedo = root.findViewById(R.id.cwSpeedo);
        this.info = root.findViewById(R.id.cwInfoPanel);
        this.minimap = root.findViewById(R.id.cwMinimap);
        ui.post(watch);
    }

    /** Ajoute le HUD natif dans parent, juste SOUS la couche WebView du HUD (hudLayer). */
    public static NativeHud attach(Activity act, ViewGroup parent, View hudLayer) {
        if (act == null || parent == null) return null;
        try {
            View v = LayoutInflater.from(act).inflate(R.layout.cw_native_hud, parent, false);
            ViewGroup.LayoutParams lp;
            if (parent instanceof ConstraintLayout) {
                ConstraintLayout.LayoutParams c = new ConstraintLayout.LayoutParams(0, 0);
                c.startToStart = ConstraintLayout.LayoutParams.PARENT_ID;
                c.endToEnd = ConstraintLayout.LayoutParams.PARENT_ID;
                c.topToTop = ConstraintLayout.LayoutParams.PARENT_ID;
                c.bottomToBottom = ConstraintLayout.LayoutParams.PARENT_ID;
                lp = c;
            } else {
                lp = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            }
            int at = hudLayer != null ? parent.indexOfChild(hudLayer) : -1;
            if (at < 0) at = parent.getChildCount();
            parent.addView(v, at, lp);
            return new NativeHud(act, v, hudLayer);
        } catch (Throwable t) {
            android.util.Log.e("CwNativeHud", "attach", t);
            return null;
        }
    }

    // ---------------------------------------------------------------- entrees (n'importe quel fil)
    public void setPlayer(float health, float armour, float hunger, float thirst, float energy, boolean dead, boolean exhausted,
                          boolean inVehicle, float speed, float vehHealth, float posX, float posY,
                          float camFx, float camFy, float heading) {
        this.camFx = camFx;
        this.camFy = camFy;
        this.heading = heading;
        this.health = dead ? 0 : health;
        this.armour = armour;
        this.hunger = hunger;
        this.thirst = thirst;
        this.energy = exhausted ? 0 : energy;
        this.dead = dead;
        this.inVehicle = inVehicle;
        this.speed = speed;
        this.vehHealth = vehHealth;
        this.posX = posX;
        this.posY = posY;
        this.gotState = true;
        schedule();
    }

    public void setNeeds(int hunger, int thirst) { this.hunger = hunger; this.thirst = thirst; schedule(); }
    public void setFuel(int fuel) { this.fuel = fuel; schedule(); }
    public void setMoney(int money) { this.money = money; schedule(); }

    /** infos du serveur ("@hudv" 9516 et autres) : fuel, belt, veh, hunger, thirst, coin, account, wanted, location */
    public void setInfo(JSONObject o) {
        if (o == null) return;
        if (o.has("fuel")) fuel = o.optInt("fuel", fuel);
        if (o.has("belt")) belt = o.optInt("belt", 1) != 0 || o.optInt("veh", 1) == 0;
        if (o.has("hunger")) hunger = o.optInt("hunger", (int) hunger);
        if (o.has("thirst")) thirst = o.optInt("thirst", (int) thirst);
        if (o.has("coin")) coins = o.optInt("coin", 0);
        if (o.has("account")) account = o.optInt("account", account);
        if (o.has("accountId")) account = o.optInt("accountId", account);
        if (o.has("wanted")) wanted = o.optInt("wanted", 0);
        if (o.has("location")) forcedLocation = o.optString("location", "").replace('_', ' ');
        if (o.has("money")) money = o.optInt("money", money);
        schedule();
    }

    // ---------------------------------------------------------------- minimap (fil UI)
    public void setMinimapListener(final MinimapView.Listener l) { ui.post(() -> { if (minimap != null) minimap.setListener(l); }); }
    public void setBlips(final String json) { ui.post(() -> { if (minimap != null) minimap.setBlips(json); }); }
    public void setWaypoint(final float x, final float y, final boolean auto) { ui.post(() -> { if (minimap != null) minimap.setWaypoint(x, y, auto); }); }
    public void clearWaypoint() { ui.post(() -> { if (minimap != null) minimap.clearWaypoint(); }); }
    public void setMinimapOn(boolean on) { minimapOn = on; schedule(); }

    private void schedule() {
        if (pending) return;
        pending = true;
        ui.post(apply);
    }

    // ---------------------------------------------------------------- fil UI
    private void applyNow() {
        if (speedo != null) {
            speedo.setStats(health, hunger, thirst, energy, armour);
            speedo.setDriving(speed, fuel, vehHealth);
            speedo.setBelt(belt);
        }
        if (minimap != null && gotState) {
            minimap.setStats(health, armour, thirst, hunger, energy);
            minimap.setPlayer(posX, posY, camFx, camFy, heading, inVehicle, speed);
        }
        if (info != null) {
            if (money != Integer.MIN_VALUE) info.setMoney(money);
            if (coins != Integer.MIN_VALUE) info.setCoins(coins);
            info.setAccount(account);
            info.setWanted(wanted);
            String loc = forcedLocation;
            info.setLocation(loc != null && !loc.isEmpty() ? loc : InfoPanelView.cityAt(posX, posY));
        }
        syncVisibility();
    }

    private void syncVisibility() {
        boolean hudVisible = hudLayer == null || (hudLayer.getVisibility() == View.VISIBLE && hudLayer.isAttachedToWindow());
        boolean show = gotState && hudVisible && !act.isFinishing();
        if (show != lastShown) {
            lastShown = show;
            root.setVisibility(show ? View.VISIBLE : View.GONE);
        }
        if (minimap != null) {
            int mw = minimapOn ? View.VISIBLE : View.GONE;
            if (minimap.getVisibility() != mw) minimap.setVisibility(mw);
        }
        if (speedo != null) {
            int want = View.GONE;   // [v53] compteur natif remplace par notre speedo2
            if (speedo.getVisibility() != want) speedo.setVisibility(want);
        }
    }

    public void release() {
        ui.removeCallbacksAndMessages(null);
        try { ((ViewGroup) root.getParent()).removeView(root); } catch (Throwable ignored) { }
    }
}
