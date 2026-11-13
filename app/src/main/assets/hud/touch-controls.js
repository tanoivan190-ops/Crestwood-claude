/* Controles tactiles Crestwood v2.
   Remplacent les boutons natifs de San Andreas (le moteur les masque des que
   "customControls" est envoye, voir cpp/samp et cpp/samp32 : game/Widgets/WidgetGta.cpp).

   - Boutons maintenus  [data-tc-key]    -> AndroidHUD.onHudAction("vehicle:control", {control, pressed})
   - Bouton bascule     [data-tc-toggle] -> idem (pressed = etat de la bascule) : viser
   - Changer d'arme     [data-tc-swap]   -> appui court : suivante, appui long : precedente
   - Moteur             [data-tc-action="engine"] -> AndroidHUD.engineAction("toggle")
   - Joystick flottant  #tcStickZone     -> AndroidHUD.padStick(x, y, actif), x / y de -128 a 128
   Plusieurs doigts en meme temps sont geres (joystick + tir + saut...). */
(() => {
  "use strict";
  const root = document.querySelector(".hud-root");
  if (!root) return;
  const hud = () => window.AndroidHUD;

  const vibrate = (ms) => { if (navigator.vibrate) { try { navigator.vibrate(ms); } catch (_) { /* ignore */ } } };
  const isEditing = () => root.classList.contains("hud-editing");
  const inVehicle = () => root.classList.contains("vehicle-hud-active");
  const syncBounds = () => { try { window.crestwoodSyncTouchBounds?.(); } catch (_) { /* ignore */ } };

  function send(control, pressed) {
    const payload = JSON.stringify({ control, pressed });
    try { hud()?.onHudAction?.("vehicle:control", payload); } catch (_) { /* ignore */ }
  }

  // ------------------------------------------------------------------ touches
  // Une meme touche peut exister sur deux boutons (ex. "fire" a pied et au volant) :
  // on compte les doigts par touche et on n'envoie que les transitions 0 <-> 1.
  const keyCount = new Map();
  const btnPointer = new Map();     // bouton -> pointerId

  function keyDown(key) {
    const n = (keyCount.get(key) || 0) + 1;
    keyCount.set(key, n);
    if (n === 1) send(key, true);
  }
  function keyUp(key) {
    const n = (keyCount.get(key) || 0) - 1;
    if (n <= 0) { keyCount.delete(key); send(key, false); } else keyCount.set(key, n);
  }

  document.querySelectorAll("[data-tc-key]").forEach((btn) => {
    const key = btn.dataset.tcKey;
    btn.addEventListener("pointerdown", (e) => {
      if (isEditing() || btnPointer.has(btn)) return;
      e.preventDefault();
      try { btn.setPointerCapture(e.pointerId); } catch (_) { /* ignore */ }
      btnPointer.set(btn, e.pointerId);
      btn.classList.add("tc-down");
      keyDown(key);
      vibrate(key === "fire" ? 12 : 8);
    });
    const up = (e) => {
      if (btnPointer.get(btn) !== e.pointerId) return;
      btnPointer.delete(btn);
      btn.classList.remove("tc-down");
      keyUp(key);
    };
    btn.addEventListener("pointerup", up);
    btn.addEventListener("pointercancel", up);
    btn.addEventListener("lostpointercapture", up);
    btn.addEventListener("contextmenu", (e) => e.preventDefault());
  });

  // ------------------------------------------------------------ viser (bascule)
  const toggles = new Map();        // nom -> etat
  function setToggle(name, on) {
    on = Boolean(on);
    if ((toggles.get(name) || false) === on) return;
    toggles.set(name, on);
    document.querySelectorAll(`[data-tc-toggle="${name}"]`).forEach((b) => {
      b.classList.toggle("tc-on", on);
      b.setAttribute("aria-pressed", String(on));
    });
    send(name, on);
  }
  document.querySelectorAll("[data-tc-toggle]").forEach((btn) => {
    const name = btn.dataset.tcToggle;
    btn.addEventListener("pointerdown", (e) => {
      if (isEditing()) return;
      e.preventDefault();
      btn.classList.add("tc-down");
      setToggle(name, !toggles.get(name));
      vibrate(10);
    });
    const end = () => btn.classList.remove("tc-down");
    btn.addEventListener("pointerup", end);
    btn.addEventListener("pointercancel", end);
    btn.addEventListener("contextmenu", (e) => e.preventDefault());
  });

  // -------------------------------------------------- changer d'arme (court/long)
  document.querySelectorAll("[data-tc-swap]").forEach((btn) => {
    let timer = 0;
    let longDone = false;
    btn.addEventListener("pointerdown", (e) => {
      if (isEditing()) return;
      e.preventDefault();
      btn.classList.add("tc-down");
      longDone = false;
      clearTimeout(timer);
      timer = setTimeout(() => {
        longDone = true;
        btn.classList.add("tc-long");
        send("weaponPrev", true);
        vibrate([10, 40, 10]);
      }, 450);
    });
    const end = (fire) => {
      clearTimeout(timer);
      btn.classList.remove("tc-down", "tc-long");
      if (fire && !longDone && !isEditing()) { send("weaponNext", true); vibrate(8); }
      longDone = false;
    };
    btn.addEventListener("pointerup", () => end(true));
    btn.addEventListener("pointercancel", () => end(false));
    btn.addEventListener("contextmenu", (e) => e.preventDefault());
  });

  // ------------------------------------------------------------- moteur (tap)
  document.querySelectorAll("[data-tc-action]").forEach((btn) => {
    btn.addEventListener("pointerdown", (e) => {
      if (isEditing()) return;
      e.preventDefault();
      btn.classList.add("tc-down");
    });
    const end = () => btn.classList.remove("tc-down");
    btn.addEventListener("pointerup", () => {
      end();
      if (isEditing()) return;
      if (btn.dataset.tcAction === "engine") {
        try { hud()?.engineAction?.("toggle"); } catch (_) { /* ignore */ }
      }
    });
    btn.addEventListener("pointercancel", end);
    btn.addEventListener("contextmenu", (e) => e.preventDefault());
  });

  // ------------------------------------------------------------------ joystick
  const zone = document.getElementById("tcStickZone");
  const stick = document.getElementById("tcStick");
  const DEADZONE = 0.14;
  const SPRINT_ON = 1.32;           // pousser au-dela de la base -> sprint
  const SPRINT_OFF = 1.12;
  const FOLLOW = 1.7;               // au-dela, la base suit le doigt
  const st = {
    id: null, ox: 0, oy: 0, r: 60,
    x: 0, y: 0, sentX: 999, sentY: 999, sentOn: null,
    sprint: false, raf: 0
  };

  function stickRadius() {
    const base = stick?.querySelector(".tc-stick-base");
    const w = base ? base.getBoundingClientRect().width : 0;
    return Math.max(30, w / 2 || 60);
  }

  // etat visuel du joystick, applique en bloc une fois par image
  const vis = { kx: "0", ky: "0", ang: 0, mag: "0", raf: 0 };
  function paintStick() {
    vis.raf = 0;
    if (!stick) return;
    stick.style.setProperty("--kx", `${vis.kx}px`);
    stick.style.setProperty("--ky", `${vis.ky}px`);
    stick.style.setProperty("--ang", `${vis.ang}deg`);
    stick.style.setProperty("--mag", vis.mag);
  }
  const queuePaint = () => { if (!vis.raf) vis.raf = requestAnimationFrame(paintStick); };

  function placeStick(cx, cy) {
    if (!stick || !zone) return;
    const zr = zone.getBoundingClientRect();
    stick.style.left = `${cx - zr.left}px`;
    stick.style.bottom = `${zr.bottom - cy}px`;
  }
  function restStick() {
    if (!stick) return;
    stick.style.left = "";
    stick.style.bottom = "";
    vis.kx = "0"; vis.ky = "0"; vis.mag = "0";
    paintStick();
    stick.classList.remove("tc-active", "tc-sprinting", "tc-near-sprint");
  }

  function flushStick() {
    st.raf = 0;
    const on = st.id !== null;
    if (on === st.sentOn && Math.abs(st.x - st.sentX) < 2 && Math.abs(st.y - st.sentY) < 2) return;
    st.sentOn = on; st.sentX = st.x; st.sentY = st.y;
    try { hud()?.padStick?.(st.x, st.y, on); } catch (_) { /* ignore */ }
  }
  const queueFlush = () => { if (!st.raf) st.raf = requestAnimationFrame(flushStick); };

  function setSprint(on) {
    if (st.sprint === on) return;
    st.sprint = on;
    stick?.classList.toggle("tc-sprinting", on);
    if (on) { keyDown("sprint"); vibrate(14); } else keyUp("sprint");
  }

  function moveStick(px, py) {
    let dx = px - st.ox;
    let dy = py - st.oy;
    let dist = Math.hypot(dx, dy);
    const r = st.r;
    // Base dynamique : si le doigt part trop loin, la base le suit (on ne "perd" jamais le joystick)
    if (dist > r * FOLLOW) {
      const k = (dist - r * FOLLOW) / dist;
      st.ox += dx * k; st.oy += dy * k;
      placeStick(st.ox, st.oy);
      dx = px - st.ox; dy = py - st.oy; dist = Math.hypot(dx, dy);
    }
    const lim = Math.min(dist, r);
    const nx = dist > 0 ? dx / dist : 0;
    const ny = dist > 0 ? dy / dist : 0;
    vis.kx = (nx * lim).toFixed(1);
    vis.ky = (ny * lim).toFixed(1);

    const norm = lim / r;
    // arc cyan sur le bord de la base, oriente vers la poussee (peint une seule fois par image)
    vis.ang = Math.round(Math.atan2(dx, -dy) * 180 / Math.PI);
    vis.mag = norm > DEADZONE ? norm.toFixed(2) : "0";
    queuePaint();
    const amt = norm <= DEADZONE ? 0 : (norm - DEADZONE) / (1 - DEADZONE);
    st.x = Math.round(nx * amt * 128);
    st.y = Math.round(ny * amt * 128);

    if (!inVehicle()) {
      stick?.classList.toggle("tc-near-sprint", dist > r * 0.95);
      if (!st.sprint && dist > r * SPRINT_ON) setSprint(true);
      else if (st.sprint && dist < r * SPRINT_OFF) setSprint(false);
    } else if (st.sprint) setSprint(false);
    queueFlush();
  }

  function endStick() {
    if (st.id === null) return;
    st.id = null;
    st.x = 0; st.y = 0;
    setSprint(false);
    restStick();
    queueFlush();
  }

  if (zone && stick) {
    zone.addEventListener("pointerdown", (e) => {
      if (isEditing() || st.id !== null) return;
      if (document.querySelector(".floating-chat-composer.open")) return;
      e.preventDefault();
      try { zone.setPointerCapture(e.pointerId); } catch (_) { /* ignore */ }
      st.id = e.pointerId;
      st.r = stickRadius();
      // la base ne deborde pas de l'ecran
      const zr = zone.getBoundingClientRect();
      st.ox = Math.max(zr.left + st.r + 6, Math.min(zr.right - st.r * 0.6, e.clientX));
      st.oy = Math.max(zr.top + st.r * 0.6, Math.min(zr.bottom - st.r - 6, e.clientY));
      placeStick(st.ox, st.oy);
      stick.classList.add("tc-active");
      moveStick(e.clientX, e.clientY);
    });
    zone.addEventListener("pointermove", (e) => {
      if (e.pointerId !== st.id) return;
      e.preventDefault();
      moveStick(e.clientX, e.clientY);
    });
    const up = (e) => { if (e.pointerId === st.id) endStick(); };
    zone.addEventListener("pointerup", up);
    zone.addEventListener("pointercancel", up);
    zone.addEventListener("lostpointercapture", up);
    zone.addEventListener("contextmenu", (e) => e.preventDefault());
    // plus de "faux lien" : pas de selection de texte, pas de glisser-deposer, pas d'appui long,
    // pas de touch -> souris emulee qui donnerait le focus / la surbrillance.
    ["selectstart", "dragstart", "gesturestart"].forEach((n) => zone.addEventListener(n, (e) => e.preventDefault()));
    zone.addEventListener("touchstart", (e) => { if (!isEditing()) e.preventDefault(); }, { passive: false });
    zone.addEventListener("touchmove", (e) => { if (!isEditing()) e.preventDefault(); }, { passive: false });
    zone.addEventListener("pointerdown", () => {
      // petit retour tactile au contact + on retire tout focus/selection laisse par la page
      vibrate(6);
      try { const sel = window.getSelection && window.getSelection(); if (sel && sel.rangeCount) sel.removeAllRanges(); } catch (_) { /* ignore */ }
      try { if (document.activeElement && document.activeElement !== document.body) document.activeElement.blur(); } catch (_) { /* ignore */ }
    }, true);
  }

  // --------------------------------------------- direction : joystick / fleches
  const STEER_KEY = "crestwoodSteerMode";
  const steerBtn = document.getElementById("tcSteerMode");
  function readSteer() { try { return localStorage.getItem(STEER_KEY) === "arrows" ? "arrows" : "stick"; } catch (_) { return "stick"; } }
  function applySteer(mode) {
    root.classList.toggle("tc-steer-arrows", mode === "arrows");
    const label = steerBtn?.querySelector("[data-steer-label]");
    if (label) label.textContent = mode === "arrows" ? "FLÈCHES" : "JOYSTICK";
    steerBtn?.setAttribute("title", mode === "arrows" ? "Direction : flèches (toucher pour le joystick)" : "Direction : joystick (toucher pour les flèches)");
    syncBounds();
  }
  applySteer(readSteer());
  steerBtn?.addEventListener("pointerdown", (e) => { if (!isEditing()) { e.preventDefault(); steerBtn.classList.add("tc-down"); } });
  steerBtn?.addEventListener("pointerup", () => {
    steerBtn.classList.remove("tc-down");
    if (isEditing()) return;
    releaseAll();
    const next = readSteer() === "arrows" ? "stick" : "arrows";
    try { localStorage.setItem(STEER_KEY, next); } catch (_) { /* ignore */ }
    applySteer(next);
    vibrate(10);
  });

  // ------------------------------------------------------ arme portee (hud.js)
  // Armes qui se visent : grenades/molotov (16-18), armes a feu (22-38), bombe aerosol, extincteur, appareil photo (41-43)
  const AIMABLE = (id) => (id >= 16 && id <= 18) || (id >= 22 && id <= 38) || (id >= 41 && id <= 43);
  const fireBtn = document.querySelector(".tc-fire");
  const fireImg = fireBtn?.querySelector(".tc-fire-weapon");
  if (fireImg) {
    fireImg.addEventListener("error", () => { fireImg.hidden = true; fireBtn.classList.remove("tc-has-weapon"); });
    fireImg.addEventListener("load", () => { fireImg.hidden = false; fireBtn.classList.add("tc-has-weapon"); });
  }
  function applyWeapon(id, icon) {
    id = Number(id) || 0;
    icon = icon === undefined ? id : (Number(icon) || 0);
    const gun = AIMABLE(id);
    const before = root.dataset.tcGun === "1";
    root.dataset.tcGun = gun ? "1" : "0";
    if (!gun) setToggle("aim", false);
    if (fireImg) {
      if (icon > 0) fireImg.src = `assets/weapons-white/GTASA_Weapon${icon}.png`;
      else { fireImg.hidden = true; fireImg.removeAttribute("src"); fireBtn.classList.remove("tc-has-weapon"); }
    }
    if (before !== gun) setTimeout(syncBounds, 160);   // le bouton "viser" apparait / disparait
  }
  document.addEventListener("crestwood:weapon", (e) => applyWeapon(e.detail?.weaponid, e.detail?.icon));
  applyWeapon(root.dataset.weapon || 0);

  // ---------------------------------------------------------------- securite
  // On lache tout si le HUD passe en arriere-plan, est recharge, ou si on monte / descend d'un vehicule.
  function releaseAll() {
    btnPointer.forEach((_, btn) => btn.classList.remove("tc-down"));
    btnPointer.clear();
    keyCount.forEach((_, key) => send(key, false));
    keyCount.clear();
    st.sprint = false;
    endStick();
    setToggle("aim", false);
  }
  document.addEventListener("visibilitychange", () => {
    if (document.hidden) releaseAll();
    else send("customControls", true);
  });
  window.addEventListener("pagehide", releaseAll);
  let wasVehicle = inVehicle();
  let wasEditing = isEditing();
  new MutationObserver(() => {
    const v = inVehicle();
    const ed = isEditing();
    if (v !== wasVehicle || ed !== wasEditing) releaseAll();
    if (v !== wasVehicle) setTimeout(syncBounds, 0);
    wasVehicle = v;
    wasEditing = ed;
  }).observe(root, { attributes: true, attributeFilter: ["class"] });

  // Active le mode "controles Crestwood" : le moteur masque et desactive ses boutons tactiles natifs.
  send("customControls", true);
  setTimeout(syncBounds, 0);
})();
