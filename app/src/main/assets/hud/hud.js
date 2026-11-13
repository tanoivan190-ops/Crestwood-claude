(() => {
  const root = document.querySelector(".hud-root");
  const cornerAction = document.querySelector(".corner-action");
  const chatAction = document.querySelector(".chat-action");
  const clothesTestAction = document.getElementById("cwClothesBtn");   // bouton TENUE (systeme de vetements du RAR)
  const chatComposer = document.querySelector(".floating-chat-composer");
  const chatInput = chatComposer?.querySelector("input");
  const radialMenu = document.querySelector(".radial-menu");
  const gameNotificationStack = document.querySelector(".game-notification-stack");
  const speedFineBanner = document.querySelector(".speed-fine-banner");
  const speedFineSpeed = document.querySelector("[data-speed-fine-speed]");
  const radarBotAlert = document.querySelector(".radarbot-alert");
  const radarBotLimit = document.querySelector("[data-radarbot-limit]");
  const radarBotDistance = document.querySelector("[data-radarbot-distance]");
  const phoneNotificationLayer = document.querySelector(".phone-notification-layer");
  const phoneNotificationSound = document.querySelector("#phoneNotificationSound");
  const idLayer = document.querySelector(".id-layer");
  const idCameraLayer = document.querySelector(".id-camera-layer");
  const idSignLayer = document.querySelector(".id-sign-layer");
  const idPhotoButton = document.querySelector("#idPhotoButton");
  const idPhoto = document.querySelector("#idPhoto");
  const idPhotoHint = document.querySelector("#idPhotoHint");
  const idName = document.querySelector("#idName");
  const idRg = document.querySelector("#idRg");
  const idSex = document.querySelector("#idSex");
  const idBirth = document.querySelector("#idBirth");
  const idExpiry = document.querySelector("#idExpiry");
  const idIssued = document.querySelector("#idIssued");
  const idSignButton = document.querySelector("#idSignButton");
  const idSignatureImage = document.querySelector("#idSignatureImage");
  const idSignatureHint = document.querySelector("#idSignatureHint");
  const idCameraFrame = document.querySelector("#idCameraFrame");
  const idCameraStatus = document.querySelector("#idCameraStatus");
  const idCameraCancel = document.querySelector("#idCameraCancel");
  const idCameraShot = document.querySelector("#idCameraShot");
  const idSignCanvas = document.querySelector("#idSignCanvas");
  const idSignClear = document.querySelector("#idSignClear");
  const idSignCancel = document.querySelector("#idSignCancel");
  const idSignSave = document.querySelector("#idSignSave");
  const firstStepsPanel = document.querySelector("#firstStepsPanel");
  const firstStepsProgress = document.querySelector("#firstStepsProgress");
  const firstStepsToggle = document.querySelector("#firstStepsToggle");
  const firstStepsStart = document.querySelector("#firstStepsStart");
  const firstStepsHint = document.querySelector("#firstStepsHint");
  const firstStepsTrackFill = document.querySelector("#firstStepsTrackFill");
  const capitalFlowLayer = document.querySelector("#capitalFlowLayer");
  const capitalFlowCard = capitalFlowLayer?.querySelector(".capital-flow-card");
  const capitalFlowContent = document.querySelector("#capitalFlowContent");
  const infoFields = new Map(
    Array.from(document.querySelectorAll("[data-info]")).map((item) => [item.dataset.info, item])
  );
  const wantedStars = Array.from(document.querySelectorAll(".wanted-star"));
  const weaponIcon = document.querySelector(".weapon-icon");
  const statusItems = new Map();
  Array.from(document.querySelectorAll("[data-status]")).forEach((item) => {
    const name = item.dataset.status;
    if (!statusItems.has(name)) statusItems.set(name, []);
    statusItems.get(name).push(item);
  });
  const radioButton = document.querySelector(".radio-talk-button");
  const engineButton = document.querySelector(".engine-toggle-button");
  const vehicleLockButton = document.querySelector(".vehicle-lock-button");
  const vehicleLockIcon = vehicleLockButton?.querySelector("img");
  const animationsButton = document.querySelector(".animations-toggle-button");
  const voiceButton = document.querySelector(".voice-talk-button");
  const interactButton = document.querySelector("#interactTap");
  const interactLabel = document.querySelector("#interactTapLabel");
  // Libelles (avec accents) des cibles connues ; sinon on affiche le libelle envoye par le serveur.
  const INTERACT_LABELS = {
    bank: "Banque",
    vehicle_lock: "Verrouiller",
    vehicle_unlock: "Déverrouiller",
    police: "Police",
    shop: "Boutique",
    clothshop: "Vêtements",
    job: "Contrat",
    locker: "Vestiaire",
    enter: "Entrer",
    exit: "Sortir",
    player: "Joueur",
    house: "Maison",
    fish: "Pêcher",
    garage: "Garage",
    clothes: "Vêtements"
  };
  const radioFrequencyText = document.querySelector("[data-radio-frequency]");
  const radioFrequencyLayer = document.querySelector(".radio-frequency-layer");
  const radioFrequencyInput = document.querySelector("[data-radio-input]");
  const radioKeypad = document.querySelector("[data-radio-keypad]");
  const radioFrequencyConfirm = document.querySelector("[data-radio-confirm]");
  const radioFrequencyCancel = document.querySelector("[data-radio-cancel]");
  const hudEditorLayer = document.querySelector(".hud-editor-layer");
  const hudEditorSave = document.querySelector("[data-hud-save]");
  const hudEditorReset = document.querySelector("[data-hud-reset]");
  const hudScaleInput = document.querySelector("[data-hud-scale]");
  const movableItems = Array.from(document.querySelectorAll("[data-hud-key]"));
  const statusBars = document.querySelector(".literal-status-bars");
  const vehicleSpeedometer = document.querySelector(".vehicle-speedometer");
  const vehicleSpeedometerFrame = document.querySelector(".literal-speedometer-frame");

  const weaponNames = {
    0: "Sans arme",
    1: "Poing américain",
    3: "Matraque",
    4: "Couteau",
    16: "Grenade",
    22: "Colt 45",
    23: "Pistolet silencieux",
    24: "Desert Eagle",
    25: "Shotgun",
    27: "Combat Shotgun",
    28: "Micro Uzi",
    29: "MP5",
    30: "AK-47",
    31: "M4",
    32: "Tec-9",
    34: "Sniper",
    43: "Appareil photo",
    46: "Parachute"
  };
  const weaponImageIds = new Set(Object.keys(weaponNames).map(Number));
  const bridge = window.SampHud || null;
  const state = {
    health: 100,
    hunger: 100,
    thirst: 100,
    energy: 100,
    armour: 0,
    inVehicle: false,
    speed: 0,
    vehicleHealth: 100,
    fuel: 100,
    gz: false,          // [v15] zone verte (serveur, "@hudv")
    gzn: "",
    belt: false,        // [v15] ceinture (/ceinture)
    time: "00:00",
    diamonds: 0,
    account: 0,
    phone: "000-000",
    money: 0,
    bank: 0,
    location: "Los Santos",
    wanted: 0,
    weaponid: 0,
    ammoClip: 0,
    ammoTotal: 0,
    playerName: "Joueur",
    gender: "male",
    registered: "",
    chatOpen: false,
    menuOpen: false,
    idOpen: false,
    idCameraOpen: false,
    idSigning: false,
    radioVisible: false,
    radioFrequency: 0,
    radioTalking: false,
    engineVisible: false,
    engineOn: false,
    vehicleKeyVisible: false,
    vehicleKeyLocked: false,
    vehicleKeyVehicle: 0,
    hudEditing: false,
    identityPhoto: "",
    identitySignature: "",
    firstStepsMask: 0,
    firstStepsClaimed: true,
    firstStepsClaimPending: false,
    firstStepsExpanded: false,
    selectedFirstStep: 0,
    activeFirstStep: 0,
    flowView: "",
    flowData: {},
    shopCart: {},
    shopSearch: "",
    shopPending: false,
    shopNotice: "",
    shopNoticeKind: ""
  };

  const departmentStoreProducts = [
    { id: "burger", name: "Hamburger", description: "Repas rapide pour calmer la faim.", image: "assets/store/burger.png", priceKey: "burgerprice", fallbackPrice: 40, stackable: true },
    { id: "radio", name: "Radio", description: "Communication par fréquence avec les autres joueurs.", image: "assets/store/radio.png", priceKey: "radioprice", fallbackPrice: 500, unique: true, ownedKey: "ownradio" },
    { id: "water", name: "Eau", description: "Bouteille d'eau pour étancher la soif.", image: "assets/store/water.png", priceKey: "waterprice", fallbackPrice: 20, stackable: true }
  ];
  let phoneNotificationTimer = 0;
  let gameNotificationId = 0;
  let speedFineTimer = 0;
  let radarBotTimer = 0;
  let radarBotCountdownTimer = 0;
  let radarBotEstimate = 0;
  let radarBotLastStamp = 0;
  let radarBotSpeedMps = 0;
  let radarBotLiveVisible = false;
  let signatureDrawing = false;
  let signatureHasInk = false;
  let signatureCanvasReady = false;
  let signatureStrokes = [];
  let activeSignatureStroke = null;
  let identitySignatureChunks = [];
  let idCameraDragging = false;
  let idCameraLastX = 0;
  let idCameraLastY = 0;

  function clampPercent(value) {
    const number = Number(value);
    if (!Number.isFinite(number)) return 0;
    return Math.max(0, Math.min(100, Math.round(number)));
  }

  function clampInt(value, min = 0, max = Number.MAX_SAFE_INTEGER) {
    const number = Number(value);
    if (!Number.isFinite(number)) return min;
    return Math.max(min, Math.min(max, Math.trunc(number)));
  }

  function pad2(value) {
    return String(clampInt(value, 0, 99)).padStart(2, "0");
  }

  function formatNumber(value) {
    return clampInt(value).toLocaleString("fr-FR");
  }

  function formatMoney(value) {
    return `${formatNumber(value)} $`;
  }

  function formatBrasiliaTime() {
    const now = new Date();
    try {
      const formatted = new Intl.DateTimeFormat("fr-FR", {
        timeZone: "Africa/Abidjan",
        hour: "2-digit",
        minute: "2-digit",
        hour12: false
      }).format(now);
      return formatted.replace("24:", "00:");
    } catch (_) {
      const utcMs = now.getTime() + now.getTimezoneOffset() * 60000;
      const brasilia = new Date(utcMs);
      return `${pad2(brasilia.getUTCHours())}:${pad2(brasilia.getUTCMinutes())}`;
    }
  }

  let hudClockTimer = 0;

  function tickBrasiliaClock(force = false) {
    const nextTime = formatBrasiliaTime();
    if (!force && state.time === nextTime) return;
    state.time = nextTime;
    setInfo("time", state.time);
  }

  function startBrasiliaClock() {
    tickBrasiliaClock(true);
    if (hudClockTimer) window.clearInterval(hudClockTimer);
    hudClockTimer = window.setInterval(() => tickBrasiliaClock(false), 1000);
  }

  function formatBankMoney(value) {
    const cents = clampInt(value, 0);
    return `${cents.toLocaleString("fr-FR")} $`;
  }

  function isValidPhone(value) {
    return /^\d{3}-\d{3}$/.test(String(value || "")) && String(value) !== "000-000";
  }

  function setStatus(name, value) {
    const items = statusItems.get(name);
    if (!items) return;
    const level = clampPercent(value);
    // Palier d'alerte : sert au CSS (pulsation + valeur affichee quand c'est bas).
    // Le gilet n'a pas d'alerte, il est simplement masque quand il est vide.
    const tier = name === "armour"
      ? (level <= 0 ? "empty" : "ok")
      : (level <= 15 ? "critical" : level <= 30 ? "low" : "ok");
    items.forEach((item) => {
      item.style.setProperty("--status-fill", `${level}%`);
      item.setAttribute("aria-valuenow", String(level));
      item.dataset.level = tier;
      item.dataset.value = String(level);
    });
  }

  // [v15] regime moteur simule (0-100) : monte dans chaque rapport puis retombe au passage du suivant
  const RPM_GEARS = [0, 28, 55, 88, 125, 170, 250];
  let rpmSmooth = 0;
  function vehicleRpm(speed) {
    const v = Math.max(0, Number(speed) || 0);
    let target = 14;                       // ralenti
    if (v >= 1) {
      let g = 1;
      while (g < RPM_GEARS.length - 1 && v > RPM_GEARS[g]) g++;
      const lo = RPM_GEARS[g - 1], hi = RPM_GEARS[g];
      target = 22 + 76 * Math.min(1, (v - lo) / Math.max(1, hi - lo));
    }
    if (!state.inVehicle) target = 0;
    rpmSmooth += (target - rpmSmooth) * 0.55;
    return Math.round(Math.max(0, Math.min(100, rpmSmooth)));
  }

  // [v15] bandeau "Zone verte" (zones publiques protegees, envoyees par le serveur)
  function renderGreenZone() {
    let box = document.getElementById("cwGreenZone");
    if (!box) {
      box = document.createElement("div");
      box.id = "cwGreenZone";
      box.className = "cw-greenzone";
      // [v15c] petit ecusson vert (style badge de police) avec une coche blanche
      box.innerHTML = '<svg viewBox="0 0 24 28" aria-hidden="true"><path class="sh" d="M12 1.5 2.5 5v7.6c0 6.3 4 10.9 9.5 13.9 5.5-3 9.5-7.6 9.5-13.9V5L12 1.5Z"/><path class="ck" d="m7.4 14.2 3.2 3.2 6.2-6.6"/></svg>';
      document.body.appendChild(box);
    }
    box.title = "Zone verte" + (state.gzn ? " : " + state.gzn : "");
    box.classList.toggle("on", !!state.gz);
  }

  function renderDrivingHud() {
    const showBothForEditor = state.hudEditing;
    if (statusBars) statusBars.hidden = state.inVehicle && !showBothForEditor;
    if (vehicleSpeedometer) {
      vehicleSpeedometer.hidden = !state.inVehicle && !showBothForEditor;
    }
    vehicleSpeedometerFrame?.contentWindow?.postMessage({
      hud: true,
      vehicle: state.inVehicle || showBothForEditor,
      speed: clampInt(state.speed, 0, 999),
      fuel: clampPercent(state.fuel),
      engine: clampPercent(state.vehicleHealth),
      rpm: vehicleRpm(state.speed) * 5,   // [v15] la barre blanche suit l'acceleration (regime simule par rapport)
      health: clampPercent(state.health),
      armour: clampPercent(state.armour),
      hunger: clampPercent(state.hunger),
      thirst: clampPercent(state.thirst),
      stress: clampPercent(state.energy),
      oxigen: 0,
      showbelt: state.inVehicle,
      seatbelt: state.belt,
      direction: "",
      street: ""
    }, "*");
    const wasDriving = root?.classList.contains("vehicle-hud-active");
    root?.classList.toggle("vehicle-hud-active", state.inVehicle);
    // Les boutons de conduite apparaissent/disparaissent : on met à jour les zones tactiles.
    if (wasDriving !== state.inVehicle) syncTouchBounds();
  }

  vehicleSpeedometerFrame?.addEventListener("load", renderDrivingHud);

  function setInfo(name, value) {
    const item = infoFields.get(name);
    if (item) item.textContent = value;
  }

  function updateWantedStars() {
    wantedStars.forEach((star, index) => {
      star.classList.toggle("active", index < state.wanted);
    });
  }

  function updateWeaponDisplay() {
    const weaponid = weaponImageIds.has(state.weaponid) ? state.weaponid : 0;
    // Controles tactiles : le bouton de tir affiche l'arme et le bouton "viser" n'apparait qu'avec une arme a feu
    if (root && root.dataset.weapon !== String(state.weaponid)) {
      root.dataset.weapon = String(state.weaponid);
      document.dispatchEvent(new CustomEvent("crestwood:weapon", { detail: { weaponid: state.weaponid, icon: weaponid } }));
    }
    if (weaponIcon) {
      weaponIcon.src = `assets/weapons/GTASA_Weapon${weaponid}.png`;
      weaponIcon.alt = weaponNames[weaponid] || "Arme";
    }

    const ammoReserve = Math.max(0, state.ammoTotal - state.ammoClip);
    // Rond d'arme de la minimap (hud-ring.js)
    document.dispatchEvent(new CustomEvent("crestwood:ammo", { detail: { weaponid: state.weaponid, icon: weaponid, clip: state.ammoClip, reserve: ammoReserve } }));
    setInfo("ammoClip", String(state.ammoClip));
    setInfo("ammoReserve", String(ammoReserve));
  }

  function renderInfoPanel() {
    setInfo("time", state.time);
    setInfo("coin", formatNumber(state.diamonds));
    setInfo("account", formatNumber(state.account));
    setInfo("phone", state.phone);
    setInfo("money", formatMoney(state.money));
    setInfo("bank", formatBankMoney(state.bank));
    setInfo("location", state.location || "Los Santos");
    updateWantedStars();
    updateWeaponDisplay();
  }

  function updateBridgeInteraction() {
    bridge?.setInteractionState?.(state.chatOpen, state.menuOpen);
  }

  function setHudEditor(open) {
    state.hudEditing = Boolean(open);
    root?.classList.toggle("hud-editing", state.hudEditing);
    if (hudEditorLayer) hudEditorLayer.hidden = !state.hudEditing;
    if (radarBotAlert) {
      radarBotAlert.classList.toggle("hud-edit-preview", state.hudEditing);
      if (state.hudEditing) {
        radarBotAlert.hidden = false;
        radarBotAlert.classList.add("visible");
        radarBotAlert.classList.remove("danger");
        if (radarBotLimit) radarBotLimit.textContent = radarBotLimit.textContent || "80";
        if (radarBotDistance) radarBotDistance.textContent = radarBotDistance.textContent || "45";
      } else if (!radarBotLiveVisible) {
        radarBotAlert.classList.remove("visible", "danger");
        radarBotAlert.hidden = true;
      }
    }
    bridge?.setHudEditorOpen?.(state.hudEditing);
    renderDrivingHud();
    reframeHud();
    syncTouchBounds();
    renderRadio();
    renderEngine();
    renderVehicleLock();
  }

  function closeInteractive() {
    const hadChat = state.chatOpen;
    const hadMenu = state.menuOpen;
    const hadIdentity = state.idOpen || state.idCameraOpen || state.idSigning;
    state.chatOpen = false;
    state.menuOpen = false;
    chatComposer?.classList.remove("open");
    chatAction?.classList.remove("chat-open");
    chatAction?.setAttribute("aria-label", "Ouvrir le chat");
    radialMenu?.classList.remove("open");
    cornerAction?.classList.remove("menu-open");
    cornerAction?.setAttribute("aria-label", "Ouvrir le menu rapide");
    if (hadIdentity) hideIdentity();
    if (hadChat) hideKeyboard();
    if (hadChat || hadMenu) updateBridgeInteraction();
    syncTouchBounds();
  }

  function scheduleHudSync() {
    applyHudLayout();
    reframeHud();
    renderDrivingHud();
    renderRadio();
    renderEngine();
    renderVehicleLock();
    syncTouchBounds();
    [0, 80, 180, 420, 800].forEach((delay) => {
      window.setTimeout(() => {
        if (!hudDrag) { applyHudLayout(); reframeHud(); }
        syncTouchBounds();
      }, delay);
    });
  }

  // v4 descarta coordenadas absolutas gravadas antes da viewport adaptativa.
  // Elas eram a causa de o HUD menor continuar nas posicoes antigas da tela.
  const hudLayoutKey = "crestwood.hud.layout.v1";
  let hudDrag = null;
  let activeHudItem = movableItems[0] || null;

  function clampScale(value) {
    const number = Number(value);
    if (!Number.isFinite(number)) return 1;
    return Math.max(0.7, Math.min(1.3, number));
  }

  function hudItemScale(item) {
    return clampScale(item?.dataset.hudScale || item?.style.getPropertyValue("--hud-item-scale") || 1);
  }

  function setHudItemScale(item, scale) {
    if (!item) return;
    const next = clampScale(scale);
    item.dataset.hudScale = String(next);
    item.style.setProperty("--hud-item-scale", String(next));
  }

  function selectHudItem(item) {
    if (!item) return;
    activeHudItem?.classList.remove("hud-edit-selected");
    activeHudItem = item;
    activeHudItem.classList.add("hud-edit-selected");
    if (hudScaleInput) hudScaleInput.value = String(Math.round(hudItemScale(activeHudItem) * 100));
  }

  function loadHudLayout() {
    try {
      const data = JSON.parse(localStorage.getItem(hudLayoutKey) || "{}") || {};
      if (data.radial) {
        delete data.radial;
        localStorage.setItem(hudLayoutKey, JSON.stringify(data));
      }
      return data;
    } catch (_) {
      return {};
    }
  }

  function saveHudLayout() {
    const data = {};
    const viewportWidth = Math.max(1, window.innerWidth);
    const viewportHeight = Math.max(1, window.innerHeight);
    movableItems.forEach((item) => {
      const key = item.dataset.hudKey;
      if (!key) return;
      const rect = item.getBoundingClientRect();
      data[key] = {
        xRatio: Number((rect.left / viewportWidth).toFixed(6)),
        yRatio: Number((rect.top / viewportHeight).toFixed(6)),
        scale: Number(hudItemScale(item).toFixed(2))
      };
    });
    localStorage.setItem(hudLayoutKey, JSON.stringify(data));
    syncTouchBounds();
  }

  function applyHudLayout() {
    const data = loadHudLayout();
    movableItems.forEach((item) => {
      const key = item.dataset.hudKey;
      const pos = data[key];
      if (!pos) return;
      const xRatio = Number(pos.xRatio);
      const yRatio = Number(pos.yRatio);
      if (!Number.isFinite(xRatio) || !Number.isFinite(yRatio)) return;
      const rect = item.getBoundingClientRect();
      const maxX = Math.max(0, window.innerWidth - rect.width);
      const maxY = Math.max(0, window.innerHeight - rect.height);
      const x = Math.max(0, Math.min(maxX, xRatio * window.innerWidth));
      const y = Math.max(0, Math.min(maxY, yRatio * window.innerHeight));
      item.style.left = `${x}px`;
      item.style.top = `${y}px`;
      item.style.right = "auto";
      item.style.bottom = "auto";
      item.style.transform = "none";
      item.style.translate = "";
      setHudItemScale(item, pos.scale || 1);
    });
    selectHudItem(activeHudItem || movableItems[0]);
    window.setTimeout(syncTouchBounds, 0);
  }

  function resetHudLayout() {
    localStorage.removeItem(hudLayoutKey);
    movableItems.forEach((item) => {
      item.style.left = "";
      item.style.top = "";
      item.style.right = "";
      item.style.bottom = "";
      item.style.transform = "";
      item.style.translate = "";
      item.dataset.hudScale = "1";
      item.style.removeProperty("--hud-item-scale");
    });
    selectHudItem(movableItems[0]);
    reframeHud();
    syncTouchBounds();
  }

  // Recadrage : garde chaque element du HUD entierement visible dans l'ecran (marge de securite),
  // quelle que soit la taille / le ratio / l'encoche de l'appareil. Utilise la propriete CSS "translate"
  // (independante de transform et scale) et ne touche pas aux positions enregistrees par l'editeur.
  const REFRAME_MARGIN = 6;
  function reframeHud() {
    if (state.hudEditing) return;
    const viewW = Math.max(1, window.innerWidth);
    const viewH = Math.max(1, window.innerHeight);
    movableItems.forEach((item) => {
      if (item.dataset.hudKey === "speedometer") return; // sa zone transparente deborde volontairement du bas
      if (item.dataset.hudKey === "quick") { item.style.translate = ""; return; } // [v19] collee a la carte (bord gauche)
      if (item.style.left) return; // position choisie dans l'editeur : deja bornee par applyHudLayout()
      item.style.translate = "";
      if (item.hidden) return;
      const rect = item.getBoundingClientRect();
      if (rect.width < 2 || rect.height < 2) return;
      let dx = 0;
      let dy = 0;
      if (rect.left < REFRAME_MARGIN) dx = REFRAME_MARGIN - rect.left;
      else if (rect.right > viewW - REFRAME_MARGIN) dx = viewW - REFRAME_MARGIN - rect.right;
      if (rect.top < REFRAME_MARGIN) dy = REFRAME_MARGIN - rect.top;
      else if (rect.bottom > viewH - REFRAME_MARGIN) dy = viewH - REFRAME_MARGIN - rect.bottom;
      if (dx || dy) item.style.translate = `${Math.round(dx)}px ${Math.round(dy)}px`;
    });
  }

  function syncTouchBounds() {
    if (!bridge?.setTouchBounds) return;
    const entries = [`viewport=${Math.round(window.innerWidth)},${Math.round(window.innerHeight)}`];
    const add = (name, element) => {
      if (!element || element.hidden) return;
      const rect = element.getBoundingClientRect();
      if (rect.width < 2 || rect.height < 2) return;
      entries.push(`${name}=${Math.round(rect.left)},${Math.round(rect.top)},${Math.round(rect.right)},${Math.round(rect.bottom)}`);
    };
    add("radio", radioButton);
    add("engine", engineButton);
    add("vehicleLock", vehicleLockButton);
    add("animations", animationsButton);
    add("voice", voiceButton);
    add("interact", interactButton);
    document.querySelectorAll(".tc-btn").forEach((btn, i) => add(`drive${i}`, btn));
    // Joystick flottant (touch-controls.js) : toute sa zone capte le tactile
    document.querySelectorAll(".tc-touch-zone").forEach((zone, i) => add(`stick${i}`, zone));
    add("quick", document.querySelector(".quick-actions"));
    add("weapon", document.querySelector(".weapon-panel"));
    add("steps", firstStepsPanel);
    if (!document.body.classList.contains("cw-minimap-off")) add("minimap", document.querySelector(".minimap-frame"));
    add("chatpin", document.querySelector("#cwChatPin"));
    add("mmWeapon", document.querySelector("#mmWeapon"));
    add("hotbar", document.querySelector("#cwHotbar"));
    if (state.chatOpen) { add("chat", chatComposer); add("chatlog", document.querySelector("#cwChat")); add("kb", document.querySelector("#cwKb")); }
    if (phoneCameraMode) {
      add("pcTop", document.querySelector("#pcTop"));
      add("pcBottom", document.querySelector("#pcBottom"));
      add("pcSelfie", document.querySelector("#pcSelfie"));   // pave selfie / drone
      add("pcModes", document.querySelector("#pcModes"));
    }
    add("peek", document.querySelector("#cwPeek.show"));
    add("note", document.querySelector(".cw-note.show .cw-note-card"));
    add("animpanel", document.querySelector("#cwAnimMini:not([hidden])"));
    bridge.setTouchBounds(entries.join(";"));
  }
  // touch-controls.js recalcule les zones quand ses boutons apparaissent / disparaissent
  window.crestwoodSyncTouchBounds = syncTouchBounds;

  function eventPoint(event) {
    const touch = event.changedTouches && event.changedTouches[0] ? event.changedTouches[0] : null;
    return {
      x: touch ? touch.clientX : event.clientX,
      y: touch ? touch.clientY : event.clientY
    };
  }

  function hideKeyboard() {
    chatInput?.blur();
    bridge?.hideAndroidKeyboard?.();
  }

  function focusChatInput() {
    if (!chatInput || !state.chatOpen) return;
    if (window.CwKb?.isOpen?.()) return;   // clavier de jeu : pas de focus (sinon le clavier Android s'ouvre)
    try {
      chatInput.focus({ preventScroll: true });
    } catch (_) {
      chatInput.focus();
    }
    const end = chatInput.value.length;
    try {
      chatInput.setSelectionRange(end, end);
    } catch (_) {}
  }

  function showKeyboard() {
    bridge?.showAndroidKeyboard?.();
  }

  function setRadialMenu(open) {
    if (Boolean(open) !== state.menuOpen && typeof cw19ShowFamily === "function") {
      try { cw19ShowFamily(""); if (open) cw19SyncToggles(); } catch (_) {}
    }
    state.menuOpen = Boolean(open);
    radialMenu?.classList.toggle("open", state.menuOpen);
    cornerAction?.classList.toggle("menu-open", state.menuOpen);
    cornerAction?.setAttribute("aria-label", state.menuOpen ? "Fermer le menu rapide" : "Ouvrir le menu rapide");
    updateBridgeInteraction();
  }

  function setChatComposer(open) {
    state.chatOpen = Boolean(open);
    chatComposer?.classList.toggle("open", state.chatOpen);
    document.body.classList.toggle("cw-chat-typing", state.chatOpen);
    if (state.chatOpen && cwChat) cwChat.scrollTop = cwChat.scrollHeight;
    chatAction?.classList.toggle("chat-open", state.chatOpen);
    document.querySelector("#cwChatPin")?.classList.toggle("chat-open", state.chatOpen);
    chatAction?.setAttribute("aria-label", state.chatOpen ? "Fermer le chat" : "Ouvrir le chat");
    updateBridgeInteraction();

    if (state.chatOpen) {
      setRadialMenu(false);
      // [v18] clavier de jeu Crestwood (hud-kb.js) ; le clavier Android ne sert que si on le demande (touche ⌨)
      if (!(window.CwKb && window.CwKb.open())) showKeyboard();
    } else {
      window.CwKb?.close?.();
      hideKeyboard();
    }
    window.setTimeout(syncTouchBounds, 40);
  }
  window.CwHudChat = { close: () => setChatComposer(false), open: () => setChatComposer(true) };

  function renderRadio() {
    if (!radioButton) return;
    radioButton.hidden = !(state.radioVisible || state.hudEditing);
    radioButton.classList.toggle("talking", state.radioTalking);
    if (radioFrequencyText) {
      radioFrequencyText.textContent = state.radioFrequency > 0 ? String(state.radioFrequency) : "OFF";
    }
    window.setTimeout(syncTouchBounds, 0);
    window.setTimeout(syncTouchBounds, 120);
    window.setTimeout(syncTouchBounds, 420);
  }

  function renderEngine() {
    if (!engineButton) return;
    engineButton.hidden = !(state.engineVisible || state.hudEditing);
    engineButton.classList.toggle("engine-on", state.engineOn);
    engineButton.setAttribute("aria-label", state.engineOn ? "Éteindre le moteur" : "Démarrer le moteur");
    window.setTimeout(syncTouchBounds, 0);
    window.setTimeout(syncTouchBounds, 120);
  }

  function renderVehicleLock() {
    if (!vehicleLockButton) return;
    vehicleLockButton.hidden = !(state.vehicleKeyVisible || state.hudEditing);
    vehicleLockButton.classList.toggle("locked", state.vehicleKeyLocked);
    vehicleLockButton.setAttribute("aria-label", state.vehicleKeyLocked ? "Déverrouiller le véhicule" : "Verrouiller le véhicule");
    if (vehicleLockIcon) {
      vehicleLockIcon.src = state.vehicleKeyLocked
        ? "assets/vehicle-locked.png"
        : "assets/vehicle-unlocked.png";
    }
    window.setTimeout(syncTouchBounds, 0);
    window.setTimeout(syncTouchBounds, 120);
  }

  function openRadioFrequency() {
    if (!radioFrequencyLayer) return;
    radioFrequencyLayer.hidden = false;
    radioFrequencyInput.value = state.radioFrequency > 0 ? String(state.radioFrequency) : "";
    window.setTimeout(() => radioFrequencyInput?.focus(), 0);
    bridge?.setHudEditorOpen?.(true);
    syncTouchBounds();
  }

  function closeRadioFrequency() {
    if (radioFrequencyLayer) radioFrequencyLayer.hidden = true;
    bridge?.setHudEditorOpen?.(state.hudEditing);
    syncTouchBounds();
  }

  function sendRadioAction(action, value = "") {
    if (bridge?.radioAction) {
      bridge.radioAction(action, String(value || ""));
      return;
    }
    sendMessage(`/radioact ${action}${value ? ` ${value}` : ""}`);
  }

  // Icone de main : le serveur (dialogue "@interact") dit si un point d'interaction est a portee.
  function setInteract(payload) {
    if (!interactButton) return;
    let data = payload;
    if (typeof data === "string") {
      try { data = JSON.parse(data); } catch (_) { data = null; }
    }
    // [Crestwood] les points fixes ont leur icone 3D + la main dessinees dans le monde par le moteur
    // (WorldMarkers) : ici seulement joueur / police / vehicule, et sans texte.
    const HTML_HAND_IDS = ["player", "police", "vehicle_lock", "vehicle_unlock"];
    const show = !!(data && Number(data.show) === 1) && HTML_HAND_IDS.includes(String(data.id || ""));
    if (show) {
      const id = String(data.id || "");
      const label = INTERACT_LABELS[id] || String(data.label || "Interagir");
      if (interactLabel) interactLabel.textContent = label;
      interactButton.setAttribute("aria-label", label);
      interactButton.dataset.target = id;
    }
    const wasHidden = interactButton.hidden;
    interactButton.hidden = !show;
    if (wasHidden !== interactButton.hidden) syncTouchBounds();
    else if (show) window.setTimeout(syncTouchBounds, 0);
  }

  // ---------------------------------------------------------------------------
  // [Crestwood] Chat HTML (messages du serveur et des joueurs) - masquable
  // ---------------------------------------------------------------------------
  const cwChat = document.querySelector("#cwChat");
  const cwChatToggle = document.querySelector("#cwChatToggle");
  const CW_CHAT_MAX = 60;
  const CW_CHAT_FADE_MS = 14000;
  let cwChatOn = true;
  try { cwChatOn = window.localStorage.getItem("cw.chat.on") !== "0"; } catch (_) { cwChatOn = true; }

  function cwChatApply() {
    document.body.classList.toggle("cw-chat-off", !cwChatOn);
    if (cwChatToggle) {
      cwChatToggle.classList.toggle("off", !cwChatOn);
      cwChatToggle.setAttribute("aria-pressed", String(cwChatOn));
    }
    document.querySelector("#cwChatPin")?.classList.toggle("off", !cwChatOn);
    document.querySelectorAll('[data-slot="chat"]').forEach((b) => b.classList.toggle("off", !cwChatOn));
  }

  function cwChatSpans(target, text, baseColor) {
    // codes couleur SA-MP {RRGGBB}
    const re = /\{([0-9a-fA-F]{6})\}/g;
    let last = 0, color = baseColor, m;
    while ((m = re.exec(text)) !== null) {
      if (m.index > last) {
        const span = document.createElement("span");
        span.textContent = text.slice(last, m.index);
        if (color) span.style.color = color;
        target.appendChild(span);
      }
      color = "#" + m[1];
      last = re.lastIndex;
    }
    if (last < text.length) {
      const span = document.createElement("span");
      span.textContent = text.slice(last);
      if (color) span.style.color = color;
      target.appendChild(span);
    }
  }

  function addChatMessage(payload) {
    if (!cwChat) return;
    let d = payload;
    if (typeof d === "string") { try { d = JSON.parse(d); } catch (_) { d = { t: String(payload) }; } }
    if (!d) return;
    const line = document.createElement("div");
    line.className = "cw-chat-line";
    const nick = String(d.n || "");
    const color = /^#[0-9A-Fa-f]{6}$/.test(String(d.c || "")) ? String(d.c) : "#FFFFFF";
    if (nick) {
      const n = document.createElement("b");
      n.textContent = nick.replace(/_/g, " ");
      n.style.color = color;
      line.appendChild(n);
      line.appendChild(document.createTextNode(" : "));
      cwChatSpans(line, String(d.t || ""), "");
    } else {
      cwChatSpans(line, String(d.t || ""), color === "#FFFFFF" ? "" : color);
    }
    cwChat.appendChild(line);
    while (cwChat.childElementCount > CW_CHAT_MAX) cwChat.removeChild(cwChat.firstElementChild);
    cwChat.scrollTop = cwChat.scrollHeight;
    window.setTimeout(() => line.classList.add("old"), CW_CHAT_FADE_MS);
  }

  function clearChat() {
    if (cwChat) cwChat.replaceChildren();
  }

  cwChatToggle?.addEventListener("click", (event) => {
    event.preventDefault();
    cwChatOn = !cwChatOn;
    try { window.localStorage.setItem("cw.chat.on", cwChatOn ? "1" : "0"); } catch (_) {}
    cwChatApply();
  });
  cwChatApply();

  function sendInteract() {
    if (state.hudEditing || !interactButton || interactButton.hidden) return;
    if (navigator.vibrate) { try { navigator.vibrate(18); } catch (_) { /* ignore */ } }
    if (bridge?.interactAction) {
      bridge.interactAction();
      return;
    }
    sendMessage("/interagir");
  }

  function sendEngineAction(action = "toggle") {
    if (state.hudEditing || !state.engineVisible) return;
    if (bridge?.engineAction) {
      bridge.engineAction(action);
      return;
    }
    sendMessage("/motor");
  }

  function startRadioTalk() {
    if (state.hudEditing || !state.radioVisible) return;
    if (state.radioTalking) return;
    if (state.radioFrequency < 1) {
      openRadioFrequency();
      return;
    }
    state.radioTalking = true;
    renderRadio();
    sendRadioAction("start");
  }

  function stopRadioTalk() {
    if (!state.radioTalking) return;
    state.radioTalking = false;
    renderRadio();
    sendRadioAction("stop");
  }

  function updateRadioFrequencyInput(key) {
    if (!radioFrequencyInput) return;
    const current = String(radioFrequencyInput.value || "");
    if (key === "clear") {
      radioFrequencyInput.value = "";
      return;
    }
    if (key === "back") {
      radioFrequencyInput.value = current.slice(0, -1);
      return;
    }
    const next = `${current}${key}`.replace(/^0+/, "").slice(0, 3);
    radioFrequencyInput.value = next;
  }

  function saveRadioFrequency() {
    const rawValue = String(radioFrequencyInput?.value || "").trim();
    const parsedValue = Number(rawValue);
    if (!rawValue || !Number.isFinite(parsedValue)) return;
    const value = clampInt(parsedValue, 1, 300);
    state.radioFrequency = value;
    renderRadio();
    sendRadioAction("freq", value);
    closeRadioFrequency();
  }

  function beginHudDrag(event, element) {
    if (!state.hudEditing) return;
    selectHudItem(element);
    const point = eventPoint(event);
    const rect = element.getBoundingClientRect();
    hudDrag = {
      element,
      offsetX: point.x - rect.left,
      offsetY: point.y - rect.top
    };
    element.setPointerCapture?.(event.pointerId);
    event.preventDefault();
  }

  function moveHudDrag(event) {
    if (!hudDrag) return;
    const point = eventPoint(event);
    const rect = hudDrag.element.getBoundingClientRect();
    const x = Math.max(0, Math.min(window.innerWidth - rect.width, point.x - hudDrag.offsetX));
    const y = Math.max(0, Math.min(window.innerHeight - rect.height, point.y - hudDrag.offsetY));
    hudDrag.element.style.left = `${x}px`;
    hudDrag.element.style.top = `${y}px`;
    hudDrag.element.style.right = "auto";
    hudDrag.element.style.bottom = "auto";
    hudDrag.element.style.transform = "none";
    hudDrag.element.style.translate = "";
    syncTouchBounds();
    event.preventDefault();
  }

  function endHudDrag(event) {
    if (!hudDrag) return;
    hudDrag.element.releasePointerCapture?.(event.pointerId);
    hudDrag = null;
    syncTouchBounds();
  }

  function parsePayload(payload) {
    if (!payload) return {};
    if (typeof payload === "object") return payload;

    const text = String(payload).trim();
    if (!text) return {};

    try {
      return JSON.parse(text);
    } catch (_) {
      const data = {};
      text.split(/[;&\s]+/).forEach((entry) => {
        const [key, value] = entry.split("=");
        if (!key || value === undefined) return;
        data[key.trim()] = value.trim();
      });
      return data;
    }
  }

  function decodePayload(value) {
    const raw = String(value || "");
    const normalized = raw.replace(/#([0-9A-Fa-f]{2})/g, "%$1");
    try { return decodeURIComponent(normalized); } catch (_) {}
    try { return decodeURIComponent(raw); } catch (_) {}
    return raw;
  }

  function escapeText(value) {
    return String(value ?? "")
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }

  function previewText(value) {
    const text = String(value || "").replace(/\s+/g, " ").trim();
    if (text.length <= 58) return text;
    return `${text.slice(0, 56)}...`;
  }

  function firstName(value) {
    return String(value || "Joueur").trim().split(/\s+/)[0] || "Joueur";
  }

  function normalizeName(value) {
    return String(value || "Joueur").replace(/_/g, " ").replace(/\s+/g, " ").trim() || "Joueur";
  }

  function getStoredIdValue(suffix) {
    if (suffix === "photo") return state.identityPhoto || "";
    if (suffix === "signature") return state.identitySignature || "";
    return "";
  }

  function setStoredIdValue(suffix, value) {
    const cleanValue = String(value || "");
    if (suffix === "photo") {
      state.identityPhoto = cleanValue;
      bridge?.saveIdentityPhoto?.(cleanValue);
      return;
    }
    if (suffix === "signature") {
      state.identitySignature = cleanValue;
      bridge?.saveIdentitySignature?.(cleanValue);
    }
  }

  function formatIdentityDate(value) {
    const raw = decodePayload(value || "").trim();
    if (/^\d{2}\/\d{2}\/\d{4}$/.test(raw)) return raw;
    const iso = raw.match(/^(\d{4})-(\d{2})-(\d{2})/);
    if (iso) return `${iso[3]}/${iso[2]}/${iso[1]}`;
    return new Date().toLocaleDateString("fr-FR");
  }

  function genderLetter(value) {
    return /^f/i.test(String(value || "")) ? "F" : "M";
  }

  function setBridgeIdentityOpen(open) {
    bridge?.setIdentityOpen?.(Boolean(open));
  }

  function renderIdentity() {
    const name = normalizeName(state.playerName);
    const registered = formatIdentityDate(state.registered);
    const photoUrl = getStoredIdValue("photo");
    const signatureUrl = signatureToImageSrc(getStoredIdValue("signature"));

    if (idName) idName.textContent = name;
    if (idRg) idRg.textContent = state.account > 0 ? String(state.account).padStart(3, "0") : "000";
    if (idSex) idSex.textContent = genderLetter(state.gender);
    if (idBirth) idBirth.textContent = registered;
    if (idExpiry) idExpiry.textContent = "31/12/2099";
    if (idIssued) idIssued.textContent = `Délivrée le ${registered}`;

    if (idPhoto) {
      idPhoto.src = photoUrl || "assets/rg/boneco.png";
      idPhotoButton?.classList.toggle("has-photo", Boolean(photoUrl));
    }
    if (idPhotoHint) idPhotoHint.textContent = photoUrl ? "Mettre à jour la photo" : "Touche pour ajouter une photo";

    if (idSignatureImage) {
      if (signatureUrl) {
        idSignatureImage.src = signatureUrl;
        idSignatureImage.hidden = false;
      } else {
        idSignatureImage.hidden = true;
        idSignatureImage.removeAttribute("src");
      }
    }
    if (idSignatureHint) {
      idSignatureHint.textContent = signatureUrl ? "Mettre à jour la signature" : "Signer la carte d'identité";
      idSignatureHint.hidden = Boolean(signatureUrl);
    }
  }

  function showIdentity(requestAssets = true) {
    closeSignature();
    closeIdCamera(false);
    setChatComposer(false);
    setRadialMenu(false);
    state.idOpen = true;
    renderIdentity();
    if (idLayer) idLayer.hidden = false;
    setBridgeIdentityOpen(true);
    if (requestAssets) bridge?.requestIdentityAssets?.();
  }

  function hideIdentity() {
    closeSignature();
    closeIdCamera(true);
    state.idOpen = false;
    if (idLayer) idLayer.hidden = true;
    setBridgeIdentityOpen(false);
  }

  let phoneCameraMode = false;
  const phoneCameraHint = "Cadre ta photo puis appuie sur « Prendre la photo »";

  // [v20] vignette de la derniere photo sur le bouton galerie de l'appareil
  function pcRefreshThumb(url) {
    const img = document.querySelector("#pcThumb");
    if (!img) return;
    let u = url || "";
    if (!u) { try { const l = JSON.parse(localStorage.getItem("crestwoodPhonePhotos") || "[]"); u = Array.isArray(l) && l[0] ? l[0] : ""; } catch (_) { u = ""; } }
    if (u) { img.src = u; img.hidden = false; } else { img.removeAttribute("src"); img.hidden = true; }
  }

  function savePhonePhoto(url) {
    try {
      const key = "crestwoodPhonePhotos";
      let list = [];
      try { list = JSON.parse(localStorage.getItem(key) || "[]"); } catch (_) { list = []; }
      if (!Array.isArray(list)) list = [];
      list.unshift(url);
      while (list.length > 24) list.pop();
      for (;;) {
        try {
          localStorage.setItem(key, JSON.stringify(list));
          break;
        } catch (_) {
          if (list.length <= 1) break;
          list.pop();
        }
      }
      localStorage.setItem("crestwoodPhoneOpenApp", "photos");
    } catch (_) {}
    pcRefreshThumb(url);
  }

  // ---- Caméra du téléphone (cadre téléphone, viseur transparent) ----
  const phoneCam = document.querySelector("#phoneCam");
  const pcView = document.querySelector("#pcView");
  const pcStage = document.querySelector("#pcStage");
  const pcPill = document.querySelector("#pcPill");
  const pcCount = document.querySelector("#pcCount");
  const pcFlash = document.querySelector("#pcFlash");
  const pcHand = document.querySelector("#pcHand");
  const PC_RATIOS = [{ label: "Max", r: 0 }, { label: "3:4", r: 3 / 4 }, { label: "1:1", r: 1 }, { label: "9:16", r: 9 / 16 }];
  const PC_TIMERS = [0, 3, 10];
  let pcRatioIndex = 0;
  let pcTimerIndex = 0;
  let pcCountdown = 0;
  let pcBusy = false;
  let pcClockTimer = 0;
  let pcHandTimer = 0;
  let pcSelfie = false;
  let pcMode = "photo";
  function pcSetMode(mode, send) {
    const m = mode === "selfie" || mode === "drone" ? mode : "photo";
    pcMode = m;
    pcSelfie = m !== "photo";
    phoneCam?.classList.toggle("mode-selfie", m === "selfie");
    phoneCam?.classList.toggle("mode-drone", m === "drone");
    document.querySelectorAll("#pcModes [data-mode]").forEach((b) => b.classList.toggle("on", b.dataset.mode === m));
    document.querySelector("#pcFlip")?.classList.toggle("on", m === "selfie");
    const bar = document.querySelector("#pcSelfie");
    if (bar) bar.hidden = m === "photo";
    pcSetStatus(m === "selfie" ? "Selfie" : (m === "drone" ? "Drone" : "Ambiance"));
    if (send) bridge?.phoneCamMode?.(m === "photo" ? "back" : m);
    window.setTimeout(() => { pcLayout(); scheduleHudSync(); }, 60);
  }

  function pcSetStatus(text) {
    if (pcPill) pcPill.textContent = window.CwI18n ? CwI18n.t(text || "Ambiance") : (text || "Ambiance");
  }

  function pcLayout() {
    if (!pcView || !pcStage) return;
    const sw = pcStage.clientWidth;
    const sh = pcStage.clientHeight;
    const r = PC_RATIOS[pcRatioIndex].r;
    let w = sw;
    let h = sh;
    if (r) {
      w = Math.min(sw, sh * r);
      h = w / r;
    }
    pcView.style.width = Math.round(w) + "px";
    pcView.style.height = Math.round(h) + "px";
    const label = document.querySelector("#pcRatio");
    if (label) label.textContent = PC_RATIOS[pcRatioIndex].label;
  }

  function pcTickClock() {
    const el = document.querySelector("#pcClock");
    if (!el) return;
    const d = new Date();
    el.textContent = String(d.getHours()).padStart(2, "0") + ":" + String(d.getMinutes()).padStart(2, "0");
  }

  function pcShoot() {
    if (pcBusy || !pcView) return;
    pcBusy = true;
    if (pcFlash) {
      pcFlash.classList.remove("go");
      void pcFlash.offsetWidth;
      pcFlash.classList.add("go");
    }
    const rect = pcView.getBoundingClientRect();
    pcSetStatus("Capture en cours...");
    bridge?.captureRgPhoto?.(
      Math.round(rect.left),
      Math.round(rect.top),
      Math.round(rect.width),
      Math.round(rect.height),
      Math.round(window.innerWidth),
      Math.round(window.innerHeight)
    );
    window.setTimeout(() => { pcBusy = false; }, 4000);
  }

  function pcCancelCountdown() {
    if (!pcCountdown) return false;
    window.clearInterval(pcCountdown);
    pcCountdown = 0;
    if (pcCount) pcCount.hidden = true;
    pcSetStatus("Ambiance");
    return true;
  }

  function pcPressShutter() {
    if (pcCancelCountdown()) return;
    const seconds = PC_TIMERS[pcTimerIndex];
    if (!seconds) { pcShoot(); return; }
    let left = seconds;
    if (pcCount) { pcCount.textContent = String(left); pcCount.hidden = false; }
    pcSetStatus("Touche pour annuler");
    pcCountdown = window.setInterval(() => {
      left -= 1;
      if (left <= 0) {
        window.clearInterval(pcCountdown);
        pcCountdown = 0;
        if (pcCount) pcCount.hidden = true;
        pcShoot();
      } else if (pcCount) {
        pcCount.textContent = String(left);
      }
    }, 1000);
  }

  function pcOpenGallery() {
    try { localStorage.setItem("crestwoodPhoneOpenApp", "photos"); } catch (_) {}
    bridge?.phoneCameraDone?.(true);
  }

  function pcInit() {
    const on = (id, fn) => document.querySelector(id)?.addEventListener("click", (event) => { event.preventDefault(); fn(); });
    on("#pcBack", () => closeIdCamera(true));
    on("#pcNavBack", () => closeIdCamera(true));
    on("#pcNavHome", () => closeIdCamera(true));
    on("#pcNavRecent", pcOpenGallery);
    on("#pcGallery", pcOpenGallery);
    on("#pcShutter", pcPressShutter);
    on("#pcGrid", () => {
      const lines = document.querySelector("#pcGridLines");
      if (lines) lines.hidden = !lines.hidden;
    });
    on("#pcRatio", () => { pcRatioIndex = (pcRatioIndex + 1) % PC_RATIOS.length; pcLayout(); });
    // [v21] trois modes : Photo (camera du jeu), Selfie (camera devant le visage), Drone (camera volante).
    // Le serveur place la camera ; les boutons du pave la deplacent (appui long = repetition).
    on("#pcFlip", () => pcSetMode(pcMode === "selfie" ? "photo" : "selfie", true));
    document.querySelectorAll("#pcModes [data-mode]").forEach((b) => b.addEventListener("click", (event) => {
      event.preventDefault();
      pcSetMode(b.dataset.mode, true);
    }));
    document.querySelectorAll("#pcSelfie [data-cam]").forEach((b) => {
      let rep = 0;
      const stop = () => { window.clearInterval(rep); rep = 0; b.classList.remove("down"); };
      b.addEventListener("pointerdown", (event) => {
        event.preventDefault();
        stop();
        b.classList.add("down");
        bridge?.phoneCamMode?.(b.dataset.cam);
        if (b.dataset.cam !== "land") rep = window.setInterval(() => bridge?.phoneCamMode?.(b.dataset.cam), 220);
      });
      ["pointerup", "pointercancel", "pointerleave"].forEach((ev) => b.addEventListener(ev, stop));
      b.addEventListener("click", (event) => event.preventDefault());
    });
    on("#pcTimer", () => {
      pcTimerIndex = (pcTimerIndex + 1) % PC_TIMERS.length;
      const lbl = document.querySelector("#pcTimerLbl");
      const s = PC_TIMERS[pcTimerIndex];
      if (lbl) lbl.textContent = s ? s + "s" : "";
    });
    window.addEventListener("resize", () => { if (phoneCameraMode) pcLayout(); });
  }
  pcInit();

  function openPhoneCamera() {
    if (!phoneCam) return;
    phoneCameraMode = true;
    pcSetMode("photo", false);
    document.body.classList.add("phone-camera-mode");
    state.idCameraOpen = true;
    phoneCam.hidden = false;
    pcBusy = false;
    pcSetStatus("Ambiance");
    pcLayout();
    pcTickClock();
    window.clearInterval(pcClockTimer);
    pcClockTimer = window.setInterval(pcTickClock, 20000);
    if (pcHand) {
      pcHand.classList.remove("gone");
      window.clearTimeout(pcHandTimer);
      pcHandTimer = window.setTimeout(() => pcHand.classList.add("gone"), 5000);
    }
    window.setTimeout(pcLayout, 120);
    pcRefreshThumb();
    scheduleHudSync();
    bridge?.openRgCamera?.();
  }

  function openIdCamera() {
    if (!state.idOpen || !idCameraLayer) return;
    state.idCameraOpen = true;
    idCameraLayer.hidden = false;
    if (idLayer) {
      idLayer.classList.add("camera-active");
      idLayer.hidden = true;
    }
    if (idCameraStatus) idCameraStatus.textContent = "Glisse vers le bas pour cadrer le visage";
    setBridgeIdentityOpen(true);
    bridge?.openRgCamera?.();
  }

  function closeIdCamera(notifyNative = true) {
    if (phoneCameraMode) {
      if (notifyNative) bridge?.phoneCameraDone?.(false);
      return;
    }
    if (!state.idCameraOpen && idCameraLayer?.hidden !== false) return;
    state.idCameraOpen = false;
    if (idCameraLayer) idCameraLayer.hidden = true;
    if (idLayer) {
      idLayer.classList.remove("camera-active");
      if (state.idOpen) idLayer.hidden = false;
    }
    if (idCameraStatus) idCameraStatus.textContent = "Glisse vers le bas pour cadrer le visage";
    if (notifyNative) bridge?.closeRgCamera?.();
  }

  function captureIdPhoto() {
    if (!idCameraFrame) return;
    const rect = idCameraFrame.getBoundingClientRect();
    if (idCameraStatus) idCameraStatus.textContent = "Capture en cours...";
    bridge?.captureRgPhoto?.(
      Math.round(rect.left),
      Math.round(rect.top),
      Math.round(rect.width),
      Math.round(rect.height),
      Math.round(window.innerWidth),
      Math.round(window.innerHeight)
    );
  }

  function adjustIdCamera(dx, dy) {
    bridge?.adjustRgCamera?.(Math.round(dx), Math.round(dy));
  }

  function setupSignatureCanvas() {
    if (!idSignCanvas) return null;
    const rect = idSignCanvas.getBoundingClientRect();
    const dpr = Math.max(1, Math.min(2, window.devicePixelRatio || 1));
    const width = Math.max(320, Math.round(rect.width * dpr));
    const height = Math.max(110, Math.round(rect.height * dpr));
    if (idSignCanvas.width !== width || idSignCanvas.height !== height) {
      idSignCanvas.width = width;
      idSignCanvas.height = height;
    }
    const ctx = idSignCanvas.getContext("2d");
    if (!ctx) return null;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, rect.width, rect.height);
    ctx.lineWidth = Math.max(2.4, rect.width * 0.006);
    ctx.lineCap = "round";
    ctx.lineJoin = "round";
    ctx.strokeStyle = "#132238";
    signatureCanvasReady = true;
    signatureHasInk = false;
    signatureStrokes = [];
    activeSignatureStroke = null;
    return ctx;
  }

  function pointerOnCanvas(event) {
    const rect = idSignCanvas.getBoundingClientRect();
    return {
      x: Math.max(0, Math.min(rect.width, event.clientX - rect.left)),
      y: Math.max(0, Math.min(rect.height, event.clientY - rect.top))
    };
  }

  function openSignature() {
    if (!idSignLayer) return;
    state.idSigning = true;
    idSignLayer.hidden = false;
    bridge?.setIdentitySigning?.(true);
    window.requestAnimationFrame(setupSignatureCanvas);
    setBridgeIdentityOpen(true);
  }

  function closeSignature() {
    state.idSigning = false;
    signatureDrawing = false;
    if (idSignLayer) idSignLayer.hidden = true;
    bridge?.setIdentitySigning?.(false);
  }

  function saveSignature() {
    if (!idSignCanvas || !signatureHasInk) {
      closeSignature();
      return;
    }
    setStoredIdValue("signature", buildSignaturePayload());
    closeSignature();
    renderIdentity();
  }

  function buildSignaturePayload() {
    if (!idSignCanvas || !signatureStrokes.length) return "";
    const rect = idSignCanvas.getBoundingClientRect();
    const width = Math.max(320, Math.round(rect.width));
    const height = Math.max(110, Math.round(rect.height));
    const strokes = signatureStrokes
      .filter((stroke) => stroke.length > 1)
      .slice(-6)
      .map((stroke) => {
        const step = Math.max(1, Math.ceil(stroke.length / 28));
        return stroke
          .filter((_, index) => index % step === 0 || index === stroke.length - 1)
          .slice(0, 34)
          .map((point) => `${Math.round(point.x)},${Math.round(point.y)}`)
          .join(".");
      })
      .filter(Boolean)
      .join(";");
    return strokes ? `sigv1:${width}:${height}:${strokes}` : "";
  }

  function signatureToImageSrc(value) {
    const raw = String(value || "");
    if (!raw) return "";
    if (!raw.startsWith("sigv1:")) return raw;

    const parts = raw.split(":");
    const width = clampInt(parts[1], 1, 2000) || 480;
    const height = clampInt(parts[2], 1, 1000) || 160;
    const strokes = parts.slice(3).join(":").split(";").map((stroke) => stroke.trim()).filter(Boolean);
    const paths = strokes
      .map((stroke) => {
        const points = stroke.split(".")
          .filter((point) => /^\d+,\d+$/.test(point))
          .join(" ");
        if (!points) return "";
        return `<polyline points="${points}" fill="none" stroke="#132238" stroke-width="3.2" stroke-linecap="round" stroke-linejoin="round"/>`;
      })
      .join("");
    const svg = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${width} ${height}">${paths}</svg>`;
    return `data:image/svg+xml,${encodeURIComponent(svg)}`;
  }

  function rgCameraNativeStatus(status, url, message) {
    const cleanStatus = String(status || "");
    if (phoneCameraMode) {
      if (cleanStatus === "capturing") pcSetStatus("Capture en cours...");
      else if (cleanStatus === "uploading") pcSetStatus("Enregistrement de la photo...");
      else if (message) pcSetStatus(message);
    } else if (idCameraStatus) {
      if (cleanStatus === "capturing") idCameraStatus.textContent = "Capture en cours...";
      else if (cleanStatus === "uploading") idCameraStatus.textContent = "Enregistrement de la photo...";
      else if (message) idCameraStatus.textContent = message;
    }

    if (cleanStatus === "captured" && url && phoneCameraMode) {
      savePhonePhoto(url);
      bridge?.phoneCameraDone?.(true);
      return;
    }
    if (cleanStatus === "captured" && url) {
      setStoredIdValue("photo", url);
      closeIdCamera(false);
      renderIdentity();
    } else if (cleanStatus === "error") {
      pcBusy = false;
      if (phoneCameraMode) pcSetStatus(message || "Impossible d'enregistrer.");
      else if (idCameraStatus) idCameraStatus.textContent = message || "Impossible d'enregistrer.";
      window.setTimeout(() => {
        if (phoneCameraMode) pcSetStatus("Ambiance");
        else if (state.idCameraOpen && idCameraStatus) idCameraStatus.textContent = "Glisse vers le bas pour cadrer le visage";
      }, 1800);
    } else if (cleanStatus === "closed") {
      closeIdCamera(false);
    }
  }

  function renderFirstSteps() {
    if (!firstStepsPanel) return;
    firstStepsPanel.hidden = state.firstStepsClaimed;
    if (state.firstStepsClaimed) {
      syncTouchBounds();
      return;
    }
    const bits = [1, 2, 4, 8, 16];
    const completed = bits.filter((bit) => (state.firstStepsMask & bit) === bit).length;
    const allCompleted = completed === bits.length;
    const firstIncomplete = bits.find((bit) => (state.firstStepsMask & bit) !== bit) || 0;
    if (!state.selectedFirstStep || (state.firstStepsMask & state.selectedFirstStep) === state.selectedFirstStep) {
      state.selectedFirstStep = firstIncomplete;
    }
    if (state.activeFirstStep && (state.firstStepsMask & state.activeFirstStep) === state.activeFirstStep) {
      state.activeFirstStep = 0;
    }

    firstStepsPanel.classList.toggle("expanded", state.firstStepsExpanded);
    firstStepsPanel.setAttribute("aria-expanded", String(state.firstStepsExpanded));
    firstStepsToggle?.setAttribute("aria-expanded", String(state.firstStepsExpanded));
    if (firstStepsProgress) firstStepsProgress.textContent = `${completed}/${bits.length}`;
    if (firstStepsTrackFill) {
      const progress = completed <= 0 ? 0 : ((completed - 1) / (bits.length - 1)) * 100;
      firstStepsTrackFill.style.width = `${progress}%`;
    }

    firstStepsPanel.querySelectorAll("[data-step-node]").forEach((node) => {
      const bit = Number(node.dataset.stepNode) || 0;
      node.classList.toggle("done", (state.firstStepsMask & bit) === bit);
      node.classList.toggle("current", bit === state.selectedFirstStep && (state.firstStepsMask & bit) !== bit);
    });

    firstStepsPanel.querySelectorAll("[data-first-step]").forEach((button) => {
      const bit = Number(button.dataset.firstStep) || 0;
      const done = (state.firstStepsMask & bit) === bit;
      const selected = bit === state.selectedFirstStep && !done;
      button.classList.toggle("done", done);
      button.classList.toggle("selected", selected);
      button.classList.toggle("active", bit === state.activeFirstStep && !done);
      button.classList.toggle("recommended", bit === firstIncomplete && !done);
      button.setAttribute("aria-selected", String(selected));
      button.setAttribute("aria-disabled", String(done));
    });

    const selectedCard = firstStepsPanel.querySelector(`[data-first-step="${state.selectedFirstStep}"] .first-step-copy strong`);
    if (firstStepsHint) {
      firstStepsHint.textContent = allCompleted
        ? "Tout est terminé. Récupère ta récompense."
        : selectedCard
          ? `${selectedCard.textContent}. Lance l'objectif pour marquer la destination.`
          : "Sélectionne le prochain objectif.";
    }
    if (firstStepsStart) {
      const hasSelection = Boolean(state.selectedFirstStep) && (state.firstStepsMask & state.selectedFirstStep) !== state.selectedFirstStep;
      firstStepsStart.disabled = state.firstStepsClaimPending || (!allCompleted && !hasSelection);
      const label = firstStepsStart.querySelector("b");
      if (label) {
        label.textContent = allCompleted
          ? (state.firstStepsClaimPending ? "RESGATANDO..." : "RÉCUPÉRER LA RÉCOMPENSE")
          : state.activeFirstStep === state.selectedFirstStep ? "OBJECTIF EN COURS" : "LANCER L'OBJECTIF";
      }
    }

    syncTouchBounds();
    [120, 300, 520].forEach((delay) => window.setTimeout(syncTouchBounds, delay));
  }

  function flowIcon(kind) {
    if (kind === "documents") return `<svg viewBox="0 0 24 24"><path d="M6 3h9l3 3v15H6z"/><path d="M15 3v4h4M9 11h6M9 15h6"/></svg>`;
    if (kind === "phone_shop") return `<svg viewBox="0 0 24 24"><rect x="7" y="2.5" width="10" height="19" rx="2"/><path d="M10 5h4M11 18.5h2"/></svg>`;
    if (kind === "bike") return `<svg viewBox="0 0 24 24"><circle cx="6" cy="17" r="4"/><circle cx="18" cy="17" r="4"/><path d="m6 17 4-8h4l4 8M9 12h7M11 7h3"/></svg>`;
    return `<svg viewBox="0 0 24 24"><path d="m12 3 2.6 5.2 5.7.8-4.1 4 1 5.7-5.2-2.8-5.2 2.8 1-5.7-4.1-4 5.7-.8z"/></svg>`;
  }

  function storeCartIcon() {
    return `<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M3 4h2l2.1 10.1a2 2 0 0 0 2 1.6h7.8a2 2 0 0 0 2-1.6L20 7H6M9 20h.01M17 20h.01"/></svg>`;
  }

  function storePrice(product) {
    return Math.max(0, clampInt(state.flowData?.[product.priceKey] ?? product.fallbackPrice));
  }

  function storeOwned(product) {
    return Boolean(product.unique && clampInt(state.flowData?.[product.ownedKey], 0, 1));
  }

  function storeCartEntries() {
    return departmentStoreProducts
      .map((product) => ({ product, quantity: clampInt(state.shopCart[product.id], 0, 99) }))
      .filter((entry) => entry.quantity > 0);
  }

  function storeCartTotal() {
    return storeCartEntries().reduce((total, entry) => total + storePrice(entry.product) * entry.quantity, 0);
  }

  function storeCartPayload() {
    return storeCartEntries().map((entry) => `${entry.product.id}:${entry.quantity}`).join(",");
  }

  function setStoreQuantity(itemId, nextQuantity) {
    const product = departmentStoreProducts.find((item) => item.id === itemId);
    if (!product) return;
    if (storeOwned(product)) {
      state.shopNotice = `Tu possèdes déjà ${product.name.toLowerCase()}.`;
      state.shopNoticeKind = "error";
      renderCapitalFlow();
      return;
    }
    const maximum = product.unique ? 1 : 99;
    const quantity = clampInt(nextQuantity, 0, maximum);
    if (quantity <= 0) delete state.shopCart[itemId];
    else state.shopCart[itemId] = quantity;
    state.shopNotice = "";
    state.shopNoticeKind = "";
    renderCapitalFlow();
  }

  function renderDepartmentStore() {
    const search = String(state.shopSearch || "").trim().toLocaleLowerCase("fr-FR");
    const products = departmentStoreProducts.filter((product) => !search || product.name.toLocaleLowerCase("fr-FR").includes(search));
    const entries = storeCartEntries();
    const itemCount = entries.reduce((total, entry) => total + entry.quantity, 0);
    const total = storeCartTotal();
    const cash = clampInt(state.flowData?.cash ?? state.money);
    const productMarkup = products.length ? products.map((product) => {
      const owned = storeOwned(product);
      const inCart = clampInt(state.shopCart[product.id], 0, 99);
      return `
        <button class="store-product${owned ? " owned" : ""}${inCart ? " selected" : ""}" type="button" data-shop-add="${product.id}" ${owned ? 'aria-disabled="true"' : ""}>
          <span class="store-product-price">${formatNumber(storePrice(product))} $</span>
          <img src="${product.image}" alt="">
          <strong>${product.name}</strong>
          <small>${product.description}</small>
          <em>${owned ? "DÉJÀ POSSÉDÉ" : inCart ? `${inCart} DANS LE PANIER` : "AJOUTER"}</em>
        </button>`;
    }).join("") : `<div class="store-empty-products">Aucun article trouvé.</div>`;

    const cartMarkup = entries.length ? entries.map(({ product, quantity }) => `
      <article class="store-cart-row">
        <img src="${product.image}" alt="">
        <div class="store-cart-copy"><strong>${product.name}</strong><small>${formatNumber(storePrice(product))} $ l'unité</small></div>
        <div class="store-quantity" aria-label="Quantité de ${product.name}">
          <button type="button" data-shop-qty="${product.id}" data-delta="-1" ${product.unique ? "disabled" : ""}>−</button>
          <span>Qté : ${quantity}</span>
          <button type="button" data-shop-qty="${product.id}" data-delta="1" ${product.unique || quantity >= 99 ? "disabled" : ""}>+</button>
        </div>
        <b>${formatNumber(storePrice(product) * quantity)} $</b>
        <button class="store-trash" type="button" data-shop-remove="${product.id}" aria-label="Retirer ${product.name}">
          <svg viewBox="0 0 24 24"><path d="M4 7h16M9 7V4h6v3m3 0-1 14H7L6 7m4 4v6m4-6v6"/></svg>
        </button>
      </article>`).join("") : `
      <div class="store-cart-empty">
        <span>${storeCartIcon()}</span>
        <strong>Ton panier est vide</strong>
        <small>Sélectionne les produits que tu veux acheter.</small>
      </div>`;

    return `
      <div class="department-store">
        <section class="store-panel store-catalog-panel">
          <header class="store-panel-heading">
            <span>${storeCartIcon()}</span>
            <div><strong>BOUTIQUE</strong><small>Catalogue des articles disponibles.</small></div>
            <em>EN POCHE : ${formatNumber(cash)} $</em>
          </header>
          <div class="store-catalog-copy"><span>Catalogue des articles disponibles.</span><label><svg viewBox="0 0 24 24"><circle cx="11" cy="11" r="7"/><path d="m20 20-4-4"/></svg><input id="departmentStoreSearch" type="search" value="${escapeText(state.shopSearch)}" placeholder="Rechercher un article..." autocomplete="off"></label></div>
          <div class="store-product-grid">${productMarkup}</div>
        </section>
        <section class="store-panel store-cart-panel">
          <header class="store-panel-heading">
            <span>${storeCartIcon()}</span>
            <div><strong>MON PANIER</strong><small>Articles sélectionnés.</small></div>
          </header>
          <div class="store-cart-count">${itemCount} ${itemCount === 1 ? "article" : "articles"} dans le panier</div>
          <div class="store-cart-list">${cartMarkup}</div>
          ${state.shopNotice ? `<div class="store-notice ${state.shopNoticeKind === "error" ? "error" : "success"}">${escapeText(state.shopNotice)}</div>` : ""}
          <footer class="store-checkout">
            <div><span>${storeCartIcon()} Total</span><strong>${formatNumber(total)} $</strong></div>
            <button type="button" data-shop-buy ${!entries.length || state.shopPending ? "disabled" : ""}><span>✓</span>${state.shopPending ? "TRAITEMENT..." : "ACHETER"}</button>
          </footer>
        </section>
      </div>`;
  }

  function closeCapitalFlow(notify = false) {
    state.flowView = "";
    state.flowData = {};
    state.shopCart = {};
    state.shopSearch = "";
    state.shopPending = false;
    state.shopNotice = "";
    state.shopNoticeKind = "";
    if (capitalFlowLayer) capitalFlowLayer.hidden = true;
    capitalFlowCard?.classList.remove("store-flow-card");
    setBridgeIdentityOpen(false);
    if (notify) bridge?.systemAction?.("close", "");
  }

  function renderCapitalFlow() {
    if (!capitalFlowLayer || !capitalFlowContent) return;
    const view = state.flowView;
    const data = state.flowData || {};
    if (!view) {
      capitalFlowLayer.hidden = true;
      return;
    }
    capitalFlowCard?.classList.toggle("store-flow-card", view === "phone_shop");

    let body = "";
    if (view === "documents") {
      body = `
        <div class="flow-heading"><span>${flowIcon(view)}</span><div><small>CITOYENNETÉ</small><h2>Établissement de la carte d'identité</h2></div></div>
        <div class="document-sheet">
          <b>Demande d'établissement de document</b>
          <p>Confirme tes informations pour établir ta carte d'identité Crestwood. Le document reste lié définitivement à ton compte.</p>
          <dl><div><dt>Demandeur</dt><dd>${escapeText(normalizeName(state.playerName))}</dd></div><div><dt>Document</dt><dd>Carte d'identité</dd></div><div><dt>Frais</dt><dd>${formatNumber(data.price || 200)} $</dd></div></dl>
        </div>
        <button class="flow-primary" type="button" data-capital-action="documents_issue">Établir le document</button>`;
    } else if (view === "phone_shop") {
      body = renderDepartmentStore();
    } else if (view === "bike") {
      const active = Number(data.active) > 0;
      body = `
        <div class="flow-heading"><span>${flowIcon(view)}</span><div><small>MOBILITÉ</small><h2>${active ? "Retour du Faggio" : "Location de Faggio"}</h2></div></div>
        <div class="bike-service"><div>${flowIcon(view)}</div><h3>${active ? "Faggio en cours d'utilisation" : "Faggio urbain"}</h3><p>${active ? "Tu peux rendre ton Faggio à n'importe quelle station de la ville." : "Le Faggio est récupéré automatiquement après 5 minutes sans conducteur."}</p>${active ? "" : `<strong>${formatNumber(data.price || 50)} $</strong>`}</div>
        <button class="flow-primary" type="button" data-capital-action="${active ? "bike_return" : "bike_rent"}">${active ? "Rendre le Faggio" : "Louer le Faggio"}</button>`;
    } else if (view === "reward") {
      body = `
        <div class="flow-heading reward"><span>${flowIcon(view)}</span><div><small>MISSION ACCOMPLIE</small><h2>Récompenses des Premiers Pas</h2></div></div>
        <p class="reward-copy">Tu as découvert les services essentiels de Crestwood et récupéré ta récompense.</p>
        <div class="reward-list"><strong>+ ${formatNumber(data.money || 7000)} $</strong><strong>+ ${formatNumber(data.xp || 10)} XP</strong></div>
        <button class="flow-primary" type="button" data-capital-flow-close>Continuer</button>`;
    }
    capitalFlowContent.innerHTML = body;
    capitalFlowLayer.hidden = false;
    setBridgeIdentityOpen(true);
  }

  function receiveIdentityEvent(payload) {
    const data = parsePayload(payload);
    const type = String(data.type || "");
    if (type === "vehicle_key") {
      updateHudInfo({
        vehicleKeyVisible: data.visible,
        vehicleKeyLocked: data.locked,
        vehicleKeyVehicle: data.vehicle
      });
      return;
    }
    if (type === "vehicle_lock_sound") return;
    if (type === "steps") {
      state.firstStepsMask = clampInt(data.mask, 0, 31);
      state.firstStepsClaimed = clampInt(data.claimed, 0, 1) === 1;
      state.firstStepsClaimPending = false;
      renderFirstSteps();
      return;
    }
    if (type === "flow") {
      state.flowView = String(data.view || "");
      state.flowData = data;
      if (state.flowView === "phone_shop") {
        state.shopCart = {};
        state.shopSearch = "";
        state.shopPending = false;
        state.shopNotice = "";
        state.shopNoticeKind = "";
      }
      renderCapitalFlow();
      return;
    }
    if (type === "shop_result") {
      state.shopPending = false;
      state.flowData = { ...state.flowData, ...data };
      state.shopNotice = decodePayload(data.message || "");
      state.shopNoticeKind = String(data.status || "") === "ok" ? "success" : "error";
      if (state.shopNoticeKind === "success") state.shopCart = {};
      renderCapitalFlow();
      return;
    }
    if (type === "flow_close") {
      closeCapitalFlow(false);
      return;
    }
    if (type === "missing") {
      hideIdentity();
      closeCapitalFlow(false);
      receiveGameNotification("type=denied;title=DOCUMENTO;message=Emita#20sua#20identidade#20na#20Central#20de#20Documentos;duration=5000");
      return;
    }
    if (type === "data") {
      if (data.name !== undefined) state.playerName = decodePayload(data.name);
      if (data.rg !== undefined) state.account = clampInt(data.rg);
      if (data.sex !== undefined) state.gender = /^f/i.test(String(data.sex)) ? "female" : "male";
      if (data.birth !== undefined) state.registered = decodePayload(data.birth);
      if (data.photo !== undefined) state.identityPhoto = decodePayload(data.photo);
      closeCapitalFlow(false);
      renderIdentity();
      if (clampInt(data.show) > 0) showIdentity(false);
      return;
    }
    if (type === "photo") {
      state.identityPhoto = decodePayload(data.value || "");
      renderIdentity();
      return;
    }
    if (type === "signature_start") {
      identitySignatureChunks = new Array(clampInt(data.chunks, 0, 100));
      if (identitySignatureChunks.length === 0) state.identitySignature = "";
      return;
    }
    if (type === "signature_chunk") {
      const index = clampInt(data.idx, 0, 100);
      if (index < identitySignatureChunks.length) {
        identitySignatureChunks[index] = decodePayload(data.value || "");
      }
      return;
    }
    if (type === "signature_end") {
      state.identitySignature = identitySignatureChunks.join("");
      identitySignatureChunks = [];
      renderIdentity();
    }
  }

  function playPhoneNotificationSound() {
    if (!phoneNotificationSound) return;
    try {
      phoneNotificationSound.currentTime = 0;
      const result = phoneNotificationSound.play();
      if (result && typeof result.catch === "function") result.catch(() => {});
    } catch (_) {}
  }

  function isPhoneNotificationAllowed(app) {
    try {
      const prefs = JSON.parse(localStorage.getItem("crestwoodPhoneNotificationPrefs") || "{}");
      return !prefs || prefs[app] !== false;
    } catch (_) {
      return true;
    }
  }

  function isGameNotificationAllowed() {
    try {
      const prefs = JSON.parse(localStorage.getItem("crestwoodPhoneNotificationPrefs") || "{}");
      return !prefs || prefs.game !== false;
    } catch (_) {
      return true;
    }
  }

  // [v21] le haut du telephone sort du bas de l'ecran (message WhatsApp, virement) ; toucher = ouvrir l'appli
  let peekTimer = 0;
  function showPhonePeek(icon, app, title, text, openApp) {
    let el = document.querySelector("#cwPeek");
    if (!el) {
      el = document.createElement("div");
      el.id = "cwPeek";
      el.setAttribute("role", "status");
      document.body.appendChild(el);
      el.addEventListener("click", () => {
        const target = el.dataset.app || "";
        try { if (target) localStorage.setItem("crestwoodPhoneOpenApp", target); } catch (_) {}
        el.classList.remove("show");
        bridge?.menuAction?.("phone");
        window.setTimeout(scheduleHudSync, 60);
      });
    }
    const d = new Date();
    const hh = String(d.getHours()).padStart(2, "0") + ":" + String(d.getMinutes()).padStart(2, "0");
    el.dataset.app = openApp || "";
    el.innerHTML = `<div class="cw-peek-phone"><i class="cw-peek-island"></i>
      <div class="cw-peek-status"><span>${hh}</span><span>5G ▮▮▮</span></div>
      <div class="cw-peek-banner"><img src="${icon}" alt=""><div><small><em>${escapeText(app)}</em><em>${escapeText(window.CwI18n ? CwI18n.t("maintenant") : "maintenant")}</em></small>
      <b>${escapeText(title)}</b><span>${escapeText(text)}</span></div></div>
      <div class="cw-peek-hint">${escapeText(window.CwI18n ? CwI18n.t("Touche pour ouvrir") : "Touche pour ouvrir")}</div></div>`;
    el.querySelectorAll("small em").forEach((e) => { e.style.fontStyle = "normal"; });
    window.requestAnimationFrame(() => { el.classList.add("show"); scheduleHudSync(); });
    playPhoneNotificationSound();
    window.clearTimeout(peekTimer);
    peekTimer = window.setTimeout(() => { el.classList.remove("show"); window.setTimeout(scheduleHudSync, 480); }, 6500);
  }

  function showPhoneNotification(payload) {
    const data = parsePayload(payload);
    if (!phoneNotificationLayer || data.direction === "out") return;

    if (data.type === "call" && data.status === "incoming") {
      if (!isPhoneNotificationAllowed("whatsapp")) return;
      const callerName = decodePayload(data.name || data.display || "");
      const callerPhone = decodePayload(data.from || data.phone || "");
      const title = callerName || callerPhone || "Appel";
      if (!callerPhone) return;

      window.clearTimeout(phoneNotificationTimer);
      phoneNotificationLayer.innerHTML = `
        <article class="phone-toast phone-call-toast" role="status">
          <span class="phone-toast-icon"><img src="assets/whatsapp.png" alt="" /></span>
          <span class="phone-toast-body">
            <strong class="phone-toast-title">${escapeText(title)}</strong>
            <span class="phone-toast-text">Décrocher ?</span>
          </span>
          <span class="phone-toast-actions">
            <button class="phone-toast-reject" type="button" data-call-action="reject" data-phone="${escapeText(callerPhone)}" aria-label="Refuser"></button>
            <button class="phone-toast-accept" type="button" data-call-action="accept" data-phone="${escapeText(callerPhone)}" aria-label="Répondre"></button>
          </span>
        </article>
      `;
      const toast = phoneNotificationLayer.querySelector(".phone-toast");
      phoneNotificationTimer = window.setTimeout(() => {
        toast?.classList.add("leaving");
        window.setTimeout(() => {
          if (phoneNotificationLayer.contains(toast)) phoneNotificationLayer.innerHTML = "";
        }, 260);
      }, 30000);
      return;
    }

    if (data.type === "bank_txn") {
      if (!isPhoneNotificationAllowed("bank")) return;
      const sender = firstName(decodePayload(data.label || data.name || "Joueur"));
      const amount = formatBankMoney(data.amount || 0);
      const message = previewText(`Tu as reçu ${amount} de ${sender}`);
      showPhonePeek("assets/bancologo.png", "Wave", "Virement reçu", message, "");
      return;

      window.clearTimeout(phoneNotificationTimer);
      phoneNotificationLayer.innerHTML = `
        <article class="phone-toast" role="status">
          <span class="phone-toast-icon"><img src="assets/bancologo.png" alt="" /></span>
          <span class="phone-toast-body">
            <strong class="phone-toast-title">Virement reçu</strong>
            <span class="phone-toast-text">${escapeText(message)}</span>
          </span>
        </article>
      `;
      const toast = phoneNotificationLayer.querySelector(".phone-toast");
      playPhoneNotificationSound();
      phoneNotificationTimer = window.setTimeout(() => {
        toast?.classList.add("leaving");
        window.setTimeout(() => {
          if (phoneNotificationLayer.contains(toast)) phoneNotificationLayer.innerHTML = "";
        }, 260);
      }, 6000);
      return;
    }

    if (data.type !== "message") return;
    if (!isPhoneNotificationAllowed("whatsapp")) return;

    const senderName = decodePayload(data.name || data.display || "");
    const senderPhone = decodePayload(data.from || data.phone || "");
    let message = decodePayload(data.text || "");
    if (data.kind === "image") message = "a envoyé une photo";
    if (data.kind === "audio") message = "a envoyé un audio";
    if (data.kind === "location") message = "a envoyé une localisation";
    message = previewText(message);
    const title = senderName || senderPhone || "Message";
    if (!message) return;
    showPhonePeek("assets/whatsapp.png", "WhatsApp", title, message, "whatsapp");
    return;

    window.clearTimeout(phoneNotificationTimer);
    phoneNotificationLayer.innerHTML = `
      <article class="phone-toast" role="status">
        <span class="phone-toast-icon"><img src="assets/whatsapp.png" alt="" /></span>
        <span class="phone-toast-body">
          <strong class="phone-toast-title">${escapeText(title)}</strong>
          <span class="phone-toast-text">${escapeText(message)}</span>
        </span>
      </article>
    `;
    const toast = phoneNotificationLayer.querySelector(".phone-toast");
    phoneNotificationTimer = window.setTimeout(() => {
      toast?.classList.add("leaving");
      window.setTimeout(() => {
        if (phoneNotificationLayer.contains(toast)) phoneNotificationLayer.innerHTML = "";
      }, 260);
    }, 6000);
  }

  // =====================================================================
  //  [v19] SA HELP : notifications du jeu facon boite d'aide de GTA San Andreas
  //  (celle qui apparait quand on prend un BMX) : noir translucide en haut a gauche,
  //  texte blanc, mots en couleur avec les codes GTA (~g~ ~r~ ~y~ ~b~ ~p~ ~o~ ~w~ ~n~)
  //  ou SA-MP ({RRGGBB}), petit bip. Une seule boite a la fois, les suivantes attendent.
  //  Serveur : dialogue 9644 "@note" {"type":"..","title":"..","message":"..","duration":6000,"sound":1}
  // =====================================================================
  const SA_COLORS = { r: "#e2484c", g: "#5cc85f", b: "#6f9cff", y: "#f3c75c", p: "#b98cff", o: "#ff9d3c", l: "#9aa0a8" };
  const SA_TYPE_COLOR = { success: "#2fb768", denied: "#e2484c", warning: "#e0a526", admin: "#8b6cf0", restart: "#e2484c", system: "#3b82ff" };
  const gameNotifyQueue = [];
  let gameNotifyCurrent = null;
  let gameNotifyShownAt = 0;
  let gameNotifyTimer = 0;
  let gameNotifyAudio = null;

  function normalizeGameNotificationType(type) {
    if (type === "success" || type === "denied" || type === "warning" || type === "system" || type === "admin" || type === "restart") return type;
    if (type === "ok") return "success";
    if (type === "error" || type === "fail" || type === "negado") return "denied";
    if (type === "info" || type === "help") return "system";
    return "system";
  }

  // Pas de titre par defaut : comme dans le jeu, la boite peut n'avoir que le texte.
  function gameNotificationTitle(type, title) {
    return decodePayload(title || "").replace(/\s+/g, " ").trim().slice(0, 40);
  }

  function saMarkup(text) {
    let out = "";
    let open = false;
    String(text || "").split(/(~[a-zA-Z]~|\{[0-9A-Fa-f]{6}\})/).forEach((part) => {
      if (!part) return;
      let m = /^~([a-zA-Z])~$/.exec(part);
      if (m) {
        const k = m[1].toLowerCase();
        if (k === "n") { out += "<br>"; return; }
        if (k === "h" || k === "k") return;
        if (open) out += "</span>";
        open = false;
        if (SA_COLORS[k]) { out += `<span style="color:${SA_COLORS[k]}">`; open = true; }
        return;
      }
      m = /^\{([0-9A-Fa-f]{6})\}$/.exec(part);
      if (m) {
        if (open) out += "</span>";
        const hex = m[1].toUpperCase();
        open = hex !== "FFFFFF";
        if (open) out += `<span style="color:#${hex}">`;
        return;
      }
      out += escapeText(part);
    });
    if (open) out += "</span>";
    return out;
  }

  // petit "bip" de l'aide du jeu (synthetise : aucun fichier a charger)
  function playHelpBeep() {
    try {
      if (localStorage.getItem("crestwoodHelpBeep") === "0") return;
      const Ctx = window.AudioContext || window.webkitAudioContext;
      if (!Ctx) return;
      gameNotifyAudio = gameNotifyAudio || new Ctx();
      const ctx = gameNotifyAudio;
      if (ctx.state === "suspended" && ctx.resume) ctx.resume().catch(() => {});
      const t = ctx.currentTime + 0.01;
      [[988, 0], [1480, 0.07]].forEach(([freq, delay]) => {
        const osc = ctx.createOscillator();
        const gain = ctx.createGain();
        osc.type = "square";
        osc.frequency.value = freq;
        gain.gain.setValueAtTime(0.0001, t + delay);
        gain.gain.exponentialRampToValueAtTime(0.045, t + delay + 0.008);
        gain.gain.exponentialRampToValueAtTime(0.0001, t + delay + 0.065);
        osc.connect(gain);
        gain.connect(ctx.destination);
        osc.start(t + delay);
        osc.stop(t + delay + 0.075);
      });
    } catch (_) {}
  }

  // [v22] icone du rond selon le type : i (info), coche (succes), croix (refus), ! (attention)
  const NOTE_ICON = {
    system: '<svg viewBox="0 0 24 24"><circle cx="12" cy="6.2" r="1.9"/><path d="M10.2 10h3.6v9.5h-3.6z"/></svg>',
    admin: '<svg viewBox="0 0 24 24"><circle cx="12" cy="6.2" r="1.9"/><path d="M10.2 10h3.6v9.5h-3.6z"/></svg>',
    success: '<svg viewBox="0 0 24 24"><path d="m4.5 12.5 5 5 10-11" fill="none" stroke-width="3.2" stroke-linecap="round" stroke-linejoin="round"/></svg>',
    denied: '<svg viewBox="0 0 24 24"><path d="m6 6 12 12M18 6 6 18" fill="none" stroke-width="3.2" stroke-linecap="round"/></svg>',
    restart: '<svg viewBox="0 0 24 24"><path d="m6 6 12 12M18 6 6 18" fill="none" stroke-width="3.2" stroke-linecap="round"/></svg>',
    warning: '<svg viewBox="0 0 24 24"><path d="M10.2 4h3.6l-.5 10.5h-2.6z"/><circle cx="12" cy="19" r="2"/></svg>'
  };
  function showGameNotify(item) {
    if (!gameNotificationStack) return;
    gameNotifyCurrent = item;
    gameNotifyShownAt = Date.now();
    const accent = SA_TYPE_COLOR[item.type] || SA_TYPE_COLOR.system;
    gameNotificationStack.innerHTML = `
      <article class="sa-help cw-note sa-help-${item.type}" data-game-notify="${item.id}" role="status" style="--sa-accent:${accent};--dur:${item.duration}ms">
        <span class="cw-note-dot" aria-hidden="true">${NOTE_ICON[item.type] || NOTE_ICON.system}<em class="cw-note-more" ${gameNotifyQueue.length ? "" : "hidden"}>+${gameNotifyQueue.length}</em></span>
        <div class="cw-note-card">
          <span class="cw-note-count" ${item.count > 1 ? "" : "hidden"}>×${item.count || 1}</span>
          ${item.title ? `<b class="sa-help-title">${saMarkup(item.title)}</b>` : ""}
          <p class="sa-help-text">${saMarkup(item.message)}</p>
          <i class="cw-note-bar"></i>
        </div>
      </article>`;
    const box = gameNotificationStack.firstElementChild;
    window.requestAnimationFrame(() => { if (box) box.classList.add("show"); syncTouchBounds(); });
    // [v21] toucher la notification = passer a la suivante
    box?.querySelector(".cw-note-card")?.addEventListener("pointerdown", (e) => { e.stopPropagation(); hideGameNotify(item.id); }, { once: true });
    if (item.sound) playHelpBeep();
    window.clearTimeout(gameNotifyTimer);
    gameNotifyTimer = window.setTimeout(() => hideGameNotify(item.id), gameNotifyQueue.length ? Math.min(item.duration, 2600) : item.duration);
  }

  function hideGameNotify(id) {
    if (!gameNotificationStack || !gameNotifyCurrent || gameNotifyCurrent.id !== id) return;
    gameNotificationStack.querySelector(".sa-help")?.classList.remove("show");
    window.clearTimeout(gameNotifyTimer);
    gameNotifyTimer = window.setTimeout(() => {
      gameNotifyCurrent = null;
      gameNotificationStack.innerHTML = "";
      const next = gameNotifyQueue.shift();
      if (next) showGameNotify(next);
      else syncTouchBounds();
    }, 210);
  }

  function receiveGameNotification(payload) {
    if (!isGameNotificationAllowed() || !gameNotificationStack) return;
    const data = parsePayload(payload);
    const type = normalizeGameNotificationType(data.type || data.status);
    let message = decodePayload(data.message || data.text || data.msg || "").replace(/\s+/g, " ").trim();
    if (!message) return;
    if (message.length > 260) message = `${message.slice(0, 258)}…`;
    const item = {
      id: ++gameNotificationId,
      type,
      title: gameNotificationTitle(type, data.title),
      message,
      duration: clampInt(data.duration, 2500, 12000) || 6000,
      sound: String(data.sound) !== "0",
      count: 1
    };
    // [v21] temps de lecture : au moins ~45 ms par caractere (titre + texte), max 10 s
    item.duration = Math.min(10000, Math.max(item.duration, 2200 + (item.message.length + item.title.length) * 45));
    const urgent = type === "denied" || type === "admin" || type === "restart";
    if (type === "restart") gameNotifyQueue.length = 0;
    if (!gameNotifyCurrent) { showGameNotify(item); return; }
    // meme texte deja affiche : on prolonge seulement
    if (gameNotifyCurrent.message === item.message && gameNotifyCurrent.title === item.title) {
      // [v21] meme notification : compteur x2, x3... sur la carte
      gameNotifyCurrent.count = (gameNotifyCurrent.count || 1) + 1;
      const badge = gameNotificationStack.querySelector(".cw-note-count");
      if (badge) { badge.hidden = false; badge.textContent = "×" + gameNotifyCurrent.count; badge.classList.remove("bump"); void badge.offsetWidth; badge.classList.add("bump"); }
      window.clearTimeout(gameNotifyTimer);
      gameNotifyTimer = window.setTimeout(() => hideGameNotify(gameNotifyCurrent.id), item.duration);
      return;
    }
    const twin = gameNotifyQueue.find((q) => q.message === item.message && q.title === item.title);
    if (twin) { twin.count = (twin.count || 1) + 1; return; }
    // [v21] refus / staff / redemarrage passent devant les simples infos
    if (urgent) gameNotifyQueue.unshift(item); else gameNotifyQueue.push(item);
    while (gameNotifyQueue.length > 4) gameNotifyQueue.splice(urgent ? gameNotifyQueue.length - 1 : 0, 1);
    const more = gameNotificationStack.querySelector(".cw-note-more");
    if (more) { more.hidden = false; more.textContent = "+" + gameNotifyQueue.length; }
    // l'aide affichee laisse la place apres 1,6 s minimum
    const current = gameNotifyCurrent;
    window.clearTimeout(gameNotifyTimer);
    gameNotifyTimer = window.setTimeout(() => hideGameNotify(current.id), Math.max(0, 1600 - (Date.now() - gameNotifyShownAt)));
  }

  function showSpeedFineNotice(payload) {
    if (!speedFineBanner) return;
    const data = parsePayload(payload);
    const speed = clampInt(data.speed || data.velocity || data.velocidade, 0, 999);
    const duration = clampInt(data.duration, 1800, 8000) || 3500;
    if (speedFineSpeed) speedFineSpeed.textContent = String(speed);

    window.clearTimeout(speedFineTimer);
    speedFineBanner.hidden = false;
    speedFineBanner.classList.remove("leaving");
    speedFineBanner.classList.add("visible");

    speedFineTimer = window.setTimeout(() => {
      speedFineBanner.classList.add("leaving");
      speedFineBanner.classList.remove("visible");
      window.setTimeout(() => {
        if (speedFineBanner.classList.contains("leaving")) {
          speedFineBanner.hidden = true;
          speedFineBanner.classList.remove("leaving");
        }
      }, 220);
    }, duration);
  }

  function stopRadarBotCountdown() {
    window.clearInterval(radarBotCountdownTimer);
    radarBotCountdownTimer = 0;
    radarBotSpeedMps = 0;
  }

  function renderRadarBotDistance() {
    if (!radarBotDistance) return;
    radarBotDistance.textContent = String(Math.max(0, Math.ceil(radarBotEstimate)));
  }

  function startRadarBotCountdown(distance, speedKmh) {
    radarBotEstimate = Math.max(0, Number(distance) || 0);
    radarBotLastStamp = performance.now();
    radarBotSpeedMps = Math.max(0, (Number(speedKmh) || 0) / 3.6);
    renderRadarBotDistance();
    window.clearInterval(radarBotCountdownTimer);
    if (radarBotSpeedMps <= 0.1) return;
    radarBotCountdownTimer = window.setInterval(() => {
      const now = performance.now();
      const elapsed = Math.max(0, (now - radarBotLastStamp) / 1000);
      radarBotLastStamp = now;
      radarBotEstimate = Math.max(0, radarBotEstimate - (radarBotSpeedMps * elapsed));
      renderRadarBotDistance();
    }, 45);
  }

  function showRadarBotAlert(payload) {
    if (!radarBotAlert) return;
    const data = parsePayload(payload);
    const shouldShow = String(data.show ?? "1") !== "0";
    window.clearTimeout(radarBotTimer);

    if (!shouldShow) {
      radarBotLiveVisible = false;
      stopRadarBotCountdown();
      if (state.hudEditing) {
        radarBotAlert.classList.add("hud-edit-preview", "visible");
        radarBotAlert.classList.remove("danger");
        radarBotAlert.hidden = false;
        return;
      }
      radarBotAlert.classList.remove("visible", "danger");
      radarBotAlert.hidden = true;
      return;
    }

    const limit = clampInt(data.limit, 1, 999) || 80;
    const distance = clampInt(data.distance || data.dist, 0, 999) || 0;
    const speed = clampInt(data.speed, 0, 999) || 0;
    const danger = String(data.danger || "0") === "1";
    radarBotLiveVisible = true;
    if (radarBotLimit) radarBotLimit.textContent = String(limit);
    startRadarBotCountdown(distance, speed);

    radarBotAlert.hidden = false;
    radarBotAlert.classList.remove("hud-edit-preview");
    radarBotAlert.classList.toggle("danger", danger);
    radarBotAlert.classList.add("visible");
    radarBotTimer = window.setTimeout(() => {
      radarBotLiveVisible = false;
      stopRadarBotCountdown();
      if (state.hudEditing) {
        radarBotAlert.classList.add("hud-edit-preview", "visible");
        radarBotAlert.classList.remove("danger");
        radarBotAlert.hidden = false;
        return;
      }
      radarBotAlert.classList.remove("visible", "danger");
      radarBotAlert.hidden = true;
    }, 900);
  }

  let hudInfoCalls = 0;
  function updateHudInfo(payload) {
    const data = parsePayload(payload);
    let statusDirty = false;
    let infoDirty = false;
    let radioDirty = false;
    let engineDirty = false;
    let vehicleKeyDirty = false;
    let drivingDirty = false;

    // [v19] l'etat arrive 10 fois/s : on ne redessine que ce qui a VRAIMENT change
    // (avant, chaque envoi relancait le rendu des jauges et du compteur meme sans changement)
    const forceAll = ++hudInfoCalls <= 3;
    const chg = (key, value) => {
      if (!forceAll && state[key] === value) return false;
      state[key] = value;
      return true;
    };
    if (data.health !== undefined && chg("health", clampPercent(data.health))) statusDirty = true;
    if (data.hunger !== undefined && chg("hunger", clampPercent(data.hunger))) statusDirty = true;
    if (data.thirst !== undefined && chg("thirst", clampPercent(data.thirst))) statusDirty = true;
    if ((data.energy !== undefined || data.oxygen !== undefined) && chg("energy", clampPercent(data.energy ?? data.oxygen))) statusDirty = true;
    if ((data.armour !== undefined || data.armor !== undefined) && chg("armour", clampPercent(data.armour ?? data.armor))) statusDirty = true;
    if ((data.inVehicle !== undefined || data.vehicle !== undefined) && chg("inVehicle", clampInt(data.inVehicle ?? data.vehicle) > 0)) drivingDirty = true;
    if ((data.speed !== undefined || data.speedKmh !== undefined) && chg("speed", clampInt(data.speed ?? data.speedKmh, 0, 999))) drivingDirty = true;
    if ((data.vehicleHealth !== undefined || data.carHealth !== undefined) && chg("vehicleHealth", clampPercent(data.vehicleHealth ?? data.carHealth))) drivingDirty = true;
    if ((data.fuel !== undefined || data.gas !== undefined) && chg("fuel", clampPercent(data.fuel ?? data.gas))) drivingDirty = true;
    if (data.time !== undefined || data.hour !== undefined || data.minute !== undefined) tickBrasiliaClock(true);
    if (data.coin !== undefined || data.diamonds !== undefined || data.diamond !== undefined || data.diamante !== undefined) {
      state.diamonds = clampInt(data.coin ?? data.diamonds ?? data.diamond ?? data.diamante);
      infoDirty = true;
    }
    if (data.account !== undefined) {
      const account = clampInt(data.account);
      if (account > 0) { state.account = account; infoDirty = true; }
    }
    if (data.accountId !== undefined) {
      const account = clampInt(data.accountId);
      if (account > 0) { state.account = account; infoDirty = true; }
    }
    if (data.phone !== undefined) {
      const nextPhone = String(data.phone || "");
      if (isValidPhone(nextPhone)) { state.phone = nextPhone; infoDirty = true; }
    }
    if (data.name !== undefined) { state.playerName = decodePayload(data.name); infoDirty = true; }
    if (data.playerName !== undefined) { state.playerName = decodePayload(data.playerName); infoDirty = true; }
    if (data.gender !== undefined) state.gender = decodePayload(data.gender);
    if (data.registered !== undefined) { state.registered = decodePayload(data.registered); infoDirty = true; }
    if (data.created !== undefined) { state.registered = decodePayload(data.created); infoDirty = true; }
    if (data.createdAt !== undefined) { state.registered = decodePayload(data.createdAt); infoDirty = true; }
    if (data.money !== undefined || data.cash !== undefined) {
      state.money = clampInt(data.money ?? data.cash);
      infoDirty = true;
    }
    if (data.bank !== undefined || data.bankBalance !== undefined) {
      state.bank = clampInt(data.bank ?? data.bankBalance);
      infoDirty = true;
    }
    if (data.location !== undefined || data.zone !== undefined) {
      state.location = decodePayload(data.location ?? data.zone).replace(/_/g, " ") || "Los Santos";
      infoDirty = true;
    }
    if (data.wanted !== undefined) { state.wanted = clampInt(data.wanted, 0, 6); infoDirty = true; }
    if (data.weaponid !== undefined) { state.weaponid = clampInt(data.weaponid, 0, 99); infoDirty = true; }
    if (data.ammoClip !== undefined) { state.ammoClip = clampInt(data.ammoClip); infoDirty = true; }
    else if (data.ammo !== undefined) { state.ammoClip = clampInt(data.ammo); infoDirty = true; }
    if (data.ammoTotal !== undefined) { state.ammoTotal = clampInt(data.ammoTotal); infoDirty = true; }
    else if (data.ammoinclip !== undefined) { state.ammoTotal = clampInt(data.ammoinclip); infoDirty = true; }
    if (data.radio !== undefined) { state.radioVisible = clampInt(data.radio) > 0; radioDirty = true; }
    if (data.freq !== undefined || data.radioFrequency !== undefined) {
      state.radioFrequency = clampInt(data.freq ?? data.radioFrequency, 0, 300);
      radioDirty = true;
    }
    if (data.engineVisible !== undefined || data.motorVisible !== undefined) {
      state.engineVisible = clampInt(data.engineVisible ?? data.motorVisible) > 0;
      engineDirty = true;
    }
    if (data.engine !== undefined || data.motor !== undefined) {
      state.engineOn = clampInt(data.engine ?? data.motor) > 0;
      engineDirty = true;
    }
    if (data.vehicleKeyVisible !== undefined) {
      state.vehicleKeyVisible = clampInt(data.vehicleKeyVisible) > 0;
      vehicleKeyDirty = true;
    }
    if (data.vehicleKeyLocked !== undefined) {
      state.vehicleKeyLocked = clampInt(data.vehicleKeyLocked) > 0;
      vehicleKeyDirty = true;
    }
    if (data.vehicleKeyVehicle !== undefined) {
      state.vehicleKeyVehicle = clampInt(data.vehicleKeyVehicle);
      vehicleKeyDirty = true;
    }
    if (data.edit !== undefined && clampInt(data.edit) > 0) setHudEditor(true);
    if (data.gz !== undefined) {
      state.gz = clampInt(data.gz) > 0;
      state.gzn = data.gzn !== undefined ? decodePayload(data.gzn) : "";
      renderGreenZone();
    }
    if (data.belt !== undefined) { state.belt = clampInt(data.belt) > 0; drivingDirty = true; }

    if (statusDirty) {
      setStatus("health", state.health);
      setStatus("hunger", state.hunger);
      setStatus("thirst", state.thirst);
      setStatus("energy", state.energy);
      setStatus("armour", state.armour);
    }
    if (drivingDirty || statusDirty) renderDrivingHud();
    if (infoDirty) renderInfoPanel();
    if (radioDirty) renderRadio();
    if (engineDirty) renderEngine();
    if (vehicleKeyDirty) renderVehicleLock();
  }

  function notifyNativeReady() {
    try {
      bridge?.ready?.();
    } catch (_) {}
  }

  function sendMessage(message) {
    const clean = String(message || "").trim();
    if (!clean) return;
    bridge?.sendChatMessage?.(clean);
  }

  function holdHudTouch(event) {
    if (state.hudEditing) return;
    event.stopPropagation();
  }

  [cornerAction, chatAction, clothesTestAction, chatComposer].forEach((element) => {
    element?.addEventListener("pointerdown", holdHudTouch);
    element?.addEventListener("touchstart", holdHudTouch, { passive: false });
    element?.addEventListener("mousedown", holdHudTouch);
  });

  cornerAction?.addEventListener("click", (event) => {
    event.preventDefault();
    event.stopPropagation();
    if (state.hudEditing) {
      selectHudItem(cornerAction.closest("[data-hud-key]") || cornerAction);
      return;
    }
    const willOpen = !state.menuOpen;
    if (willOpen) setChatComposer(false);
    setRadialMenu(willOpen);
  });

  chatAction?.addEventListener("click", (event) => {
    event.preventDefault();
    event.stopPropagation();
    if (state.hudEditing) {
      selectHudItem(chatAction.closest("[data-hud-key]") || chatAction);
      return;
    }
    setChatComposer(!state.chatOpen);
  });

  // Bouton TENUE : ouvre l'editeur de vetements (comme dans le RAR)
  clothesTestAction?.addEventListener("click", (event) => {
    event.preventDefault();
    event.stopPropagation();
    if (state.hudEditing) return;
    setRadialMenu(false);
    setChatComposer(false);
    bridge?.menuAction?.("clothes");
  });

  // Bouton RP : ouvre la tablette RP (animations, factures, annonces, urgences, personnage...)
  document.querySelector(".rp-action")?.addEventListener("click", (event) => {
    event.preventDefault();
    event.stopPropagation();
    if (state.hudEditing) return;
    setRadialMenu(false);
    setChatComposer(false);
    bridge?.menuAction?.("rp");
  });

  document.querySelectorAll("[data-weapon-action]").forEach((button) => {
    button.addEventListener("click", () => {
      bridge?.weaponAction?.(button.dataset.weaponAction || "");
    });
  });

  chatComposer?.addEventListener("submit", (event) => {
    event.preventDefault();
    sendMessage(chatInput?.value);
    if (chatInput) chatInput.value = "";
    setChatComposer(false);
  });

  chatInput?.addEventListener("touchstart", (event) => {
    event.stopPropagation();
  });

  phoneNotificationLayer?.addEventListener("click", (event) => {
    const button = event.target.closest("[data-call-action]");
    if (!button) return;
    const phone = button.dataset.phone || "";
    const action = button.dataset.callAction === "accept" ? "accept" : "reject";
    bridge?.phoneCallAction?.(action, phone);
    const toast = button.closest(".phone-toast");
    toast?.classList.add("leaving");
    window.setTimeout(() => {
      if (toast && phoneNotificationLayer.contains(toast)) phoneNotificationLayer.innerHTML = "";
    }, 220);
  });

  chatInput?.addEventListener("pointerdown", (event) => {
    event.stopPropagation();
    window.setTimeout(focusChatInput, 0);
  });

  chatInput?.addEventListener("compositionend", () => {
    window.setTimeout(focusChatInput, 0);
  });

  chatInput?.addEventListener("input", () => {
    if (state.chatOpen && document.activeElement !== chatInput) {
      window.setTimeout(focusChatInput, 0);
    }
  });

  radioButton?.addEventListener("pointerdown", (event) => {
    event.preventDefault();
    startRadioTalk();
  });
  radioButton?.addEventListener("touchstart", (event) => {
    event.preventDefault();
    startRadioTalk();
  }, { passive: false });
  radioButton?.addEventListener("mousedown", (event) => {
    event.preventDefault();
    startRadioTalk();
  });
  ["pointerup", "pointercancel", "pointerleave"].forEach((type) => {
    radioButton?.addEventListener(type, stopRadioTalk);
  });
  ["touchend", "touchcancel", "mouseup"].forEach((type) => {
    radioButton?.addEventListener(type, stopRadioTalk);
  });
  document.addEventListener("pointerup", stopRadioTalk);
  document.addEventListener("touchend", stopRadioTalk);
  document.addEventListener("mouseup", stopRadioTalk);
  radioFrequencyConfirm?.addEventListener("click", saveRadioFrequency);
  radioFrequencyCancel?.addEventListener("click", closeRadioFrequency);
  radioFrequencyInput?.addEventListener("keydown", (event) => {
    if (event.key === "Enter") saveRadioFrequency();
  });
  radioKeypad?.addEventListener("click", (event) => {
    const key = event.target?.dataset?.key;
    if (!key) return;
    updateRadioFrequencyInput(key);
  });
  interactButton?.addEventListener("click", (event) => {
    event.preventDefault();
    event.stopPropagation();
    sendInteract();
  });
  interactButton?.addEventListener("pointerdown", (event) => {
    event.preventDefault();
  });
  engineButton?.addEventListener("click", (event) => {
    event.preventDefault();
    sendEngineAction("toggle");
  });
  engineButton?.addEventListener("pointerdown", (event) => {
    if (state.hudEditing) return;
    event.preventDefault();
  });
  vehicleLockButton?.addEventListener("click", (event) => {
    event.preventDefault();
    event.stopPropagation();
    if (state.hudEditing) {
      selectHudItem(vehicleLockButton);
      return;
    }
    if (!state.vehicleKeyVisible) return;
    bridge?.vehicleLockAction?.();
  });
  vehicleLockButton?.addEventListener("pointerdown", (event) => {
    if (state.hudEditing) return;
    event.preventDefault();
  });
  animationsButton?.addEventListener("click", (event) => {
    event.preventDefault();
    event.stopPropagation();
    if (state.hudEditing) {
      selectHudItem(animationsButton);
      return;
    }
    // [v22] mini page des animations a droite (anim-mini.js) ; secours : ancienne commande
    if (window.CwAnim) window.CwAnim.toggle();
    else bridge?.openAnimations?.();
  });
  animationsButton?.addEventListener("pointerdown", (event) => {
    if (state.hudEditing) return;
    event.preventDefault();
  });
  // ---- Vocal de proximite (SampVoice) : maintenir = parler ; double appui rapide = micro verrouille ----
  let voiceTalking = false;
  let voiceLocked = false;
  let voiceLastTap = 0;
  let voicePointer = null;
  function setVoiceTalk(on) {
    on = Boolean(on);
    if (on === voiceTalking) return;
    voiceTalking = on;
    voiceButton?.classList.toggle("talking", on);
    voiceButton?.setAttribute("aria-pressed", on ? "true" : "false");
    try { bridge?.voiceTalk?.(on); } catch (_) { /* ignore */ }
    if (on && navigator.vibrate) { try { navigator.vibrate(12); } catch (_) { /* ignore */ } }
  }
  function setVoiceLocked(on) {
    voiceLocked = Boolean(on);
    voiceButton?.classList.toggle("locked", voiceLocked);
  }
  voiceButton?.addEventListener("pointerdown", (event) => {
    if (state.hudEditing) { selectHudItem(voiceButton); return; }
    event.preventDefault();
    try { voiceButton.setPointerCapture(event.pointerId); } catch (_) { /* ignore */ }
    voicePointer = event.pointerId;
    try {
      if (bridge?.voiceAllowed && !bridge.voiceAllowed()) {
        voiceButton.classList.add("denied");
        bridge.voiceTalk?.(true);   // ouvre la demande d'autorisation du micro
        return;
      }
    } catch (_) { /* ignore */ }
    voiceButton.classList.remove("denied");
    const now = Date.now();
    if (voiceLocked) { setVoiceLocked(false); setVoiceTalk(false); voiceLastTap = 0; return; }
    if (now - voiceLastTap < 320) { setVoiceLocked(true); setVoiceTalk(true); voiceLastTap = 0; return; }
    voiceLastTap = now;
    setVoiceTalk(true);
  });
  const voiceRelease = (event) => {
    if (voicePointer !== null && event.pointerId !== voicePointer) return;
    voicePointer = null;
    if (!voiceLocked) setVoiceTalk(false);
  };
  voiceButton?.addEventListener("pointerup", voiceRelease);
  voiceButton?.addEventListener("pointercancel", voiceRelease);
  voiceButton?.addEventListener("lostpointercapture", voiceRelease);
  voiceButton?.addEventListener("contextmenu", (event) => event.preventDefault());
  // l'application passe en arriere-plan : on coupe le micro
  document.addEventListener("visibilitychange", () => { if (document.hidden) { setVoiceLocked(false); setVoiceTalk(false); } });
  // la page du HUD est rechargee (ouverture d'un panneau) : ne jamais laisser le micro ouvert
  window.addEventListener("pagehide", () => { try { bridge?.voiceTalk?.(false); } catch (_) { /* ignore */ } });

  hudEditorSave?.addEventListener("click", () => {
    saveHudLayout();
    setHudEditor(false);
  });
  hudEditorReset?.addEventListener("click", resetHudLayout);
  hudScaleInput?.addEventListener("input", () => {
    if (!activeHudItem) return;
    hudDefaultResetPending = false;
    setHudItemScale(activeHudItem, Number(hudScaleInput.value || 100) / 100);
    syncTouchBounds();
  });
  movableItems.forEach((item) => {
    item.addEventListener("click", () => {
      if (state.hudEditing) selectHudItem(item);
    });
    item.addEventListener("pointerdown", (event) => beginHudDrag(event, item), true);
    item.addEventListener("pointermove", moveHudDrag);
    item.addEventListener("pointerup", endHudDrag);
    item.addEventListener("pointercancel", endHudDrag);
  });

  radialMenu?.addEventListener("click", (event) => {
    if (event.target === radialMenu) setRadialMenu(false);
  });

  firstStepsPanel?.addEventListener("click", (event) => {
    if (event.target.closest?.("#firstStepsToggle")) {
      state.firstStepsExpanded = !state.firstStepsExpanded;
      renderFirstSteps();
      return;
    }

    const target = event.target.closest?.("[data-first-step]");
    if (target && !target.classList.contains("done")) {
      state.selectedFirstStep = Number(target.dataset.firstStep) || 0;
      renderFirstSteps();
      return;
    }

    if (event.target.closest?.("#firstStepsStart") && state.firstStepsMask === 31 && !state.firstStepsClaimed) {
      state.firstStepsClaimPending = true;
      bridge?.systemAction?.("steps_claim", "");
      renderFirstSteps();
      window.setTimeout(() => {
        if (!state.firstStepsClaimed) {
          state.firstStepsClaimPending = false;
          renderFirstSteps();
        }
      }, 2500);
      return;
    }

    if (event.target.closest?.("#firstStepsStart") && state.selectedFirstStep) {
      state.activeFirstStep = state.selectedFirstStep;
      bridge?.systemAction?.("steps_target", `step=${state.selectedFirstStep}`);
      renderFirstSteps();
    }
  });

  capitalFlowLayer?.addEventListener("click", (event) => {
    const addProduct = event.target.closest?.("[data-shop-add]");
    if (addProduct) {
      const product = departmentStoreProducts.find((item) => item.id === addProduct.dataset.shopAdd);
      if (!product) return;
      if (storeOwned(product)) {
        state.shopNotice = `Tu possèdes déjà ${product.name.toLowerCase()}.`;
        state.shopNoticeKind = "error";
        renderCapitalFlow();
        return;
      }
      const current = clampInt(state.shopCart[product.id], 0, 99);
      if (product.unique && current >= 1) {
        state.shopNotice = `${product.name} n'autorise qu'une seule unité.`;
        state.shopNoticeKind = "error";
        renderCapitalFlow();
        return;
      }
      setStoreQuantity(product.id, current + 1);
      return;
    }
    const quantityButton = event.target.closest?.("[data-shop-qty]");
    if (quantityButton) {
      const itemId = quantityButton.dataset.shopQty;
      const current = clampInt(state.shopCart[itemId], 0, 99);
      setStoreQuantity(itemId, current + Number(quantityButton.dataset.delta || 0));
      return;
    }
    const removeProduct = event.target.closest?.("[data-shop-remove]");
    if (removeProduct) {
      setStoreQuantity(removeProduct.dataset.shopRemove, 0);
      return;
    }
    if (event.target.closest?.("[data-shop-buy]")) {
      const payload = storeCartPayload();
      if (!payload || state.shopPending) return;
      state.shopPending = true;
      state.shopNotice = "";
      state.shopNoticeKind = "";
      renderCapitalFlow();
      bridge?.systemAction?.("shop_buy", payload);
      window.setTimeout(() => {
        if (!state.shopPending) return;
        state.shopPending = false;
        state.shopNotice = "La boutique met trop de temps à répondre. Réessaie.";
        state.shopNoticeKind = "error";
        renderCapitalFlow();
      }, 5000);
      return;
    }
    const confirm = event.target.closest?.("[data-capital-confirm]");
    if (confirm) {
      state.flowView = confirm.dataset.capitalConfirm || state.flowView;
      renderCapitalFlow();
      return;
    }
    const back = event.target.closest?.("[data-capital-back]");
    if (back) {
      state.flowView = back.dataset.capitalBack || "phone_shop";
      renderCapitalFlow();
      return;
    }
    const action = event.target.closest?.("[data-capital-action]");
    if (action) {
      action.disabled = true;
      bridge?.systemAction?.(action.dataset.capitalAction || "", "");
      return;
    }
    if (event.target.closest?.("[data-capital-flow-close]")) closeCapitalFlow(true);
  });

  capitalFlowLayer?.addEventListener("input", (event) => {
    if (event.target?.id !== "departmentStoreSearch") return;
    state.shopSearch = event.target.value || "";
    const cursor = event.target.selectionStart;
    renderCapitalFlow();
    const searchInput = document.querySelector("#departmentStoreSearch");
    if (searchInput) {
      searchInput.focus({ preventScroll: true });
      try { searchInput.setSelectionRange(cursor, cursor); } catch (_) {}
    }
  });

  // Toucher la minimap ouvre la grande carte (la meme que celle du menu rotatif).
  const minimapFrame = document.querySelector(".minimap-frame");
  minimapFrame?.addEventListener("click", (event) => {
    if (state.hudEditing) return;
    event.preventDefault();
    bridge?.menuAction?.("map");
  });

  // =====================================================================
  //  [v19b] CW19 ROUE : petite roue de 6 familles ; toucher une famille ouvre ses sous-menus
  //  (panneau a droite). Les entrees gardent data-slot -> meme actions qu'avant (bridge.menuAction),
  //  "cmd:/xxx" = commande envoyee au serveur (SAMP.java game + game32, liste blanche).
  // =====================================================================
  const cw18Label = document.querySelector("#cw18Label");
  const cw19Panel = document.querySelector("#cw19Panel");
  const cw19Fams = Array.from(document.querySelectorAll(".cw19-fam"));
  let cw19Current = "";
  const wheelLabel = (txt) => { if (cw18Label) cw18Label.textContent = txt || "Menu"; };
  function cw19ShowFamily(fam) {
    cw19Current = fam || "";
    cw19Fams.forEach((b) => b.classList.toggle("on", b.dataset.fam === cw19Current));
    document.querySelectorAll(".cw19-group").forEach((g) => { g.hidden = g.dataset.fam !== cw19Current; });
    if (cw19Panel) {
      cw19Panel.hidden = !cw19Current;
      cw19Panel.classList.remove("in"); void cw19Panel.offsetWidth;
      if (cw19Current) cw19Panel.classList.add("in");
    }
    const btn = cw19Fams.find((b) => b.dataset.fam === cw19Current);
    wheelLabel(btn ? btn.getAttribute("aria-label") : "");
    document.querySelector(".cw19-wheel")?.classList.toggle("has-panel", Boolean(cw19Current));
  }
  function cw19SyncToggles() {
    document.querySelectorAll('.cw19-item[data-slot="minimap"]').forEach((b) => { b.classList.toggle("off", !cw18MinimapOn); });
    document.querySelectorAll('.cw19-item[data-slot="chat"]').forEach((b) => { b.classList.toggle("off", !cwChatOn); });
    document.querySelectorAll(".cw19-toggle .cw19-state").forEach((e) => { const v = e.closest(".off") ? "Masqué" : "Affiché"; e.textContent = window.CwI18n ? CwI18n.t(v) : v; });
  }
  cw19Fams.forEach((b) => b.addEventListener("click", (event) => {
    event.stopPropagation();
    cw19ShowFamily(cw19Current === b.dataset.fam ? "" : b.dataset.fam);
  }));
  document.querySelector("#cw19Hub")?.addEventListener("click", () => { setRadialMenu(false); });
  document.querySelectorAll(".wheel-slot, .cw19-item").forEach((slot) => {
    slot.addEventListener("pointerdown", () => { slot.classList.add("hot"); });
    ["pointerup", "pointerleave", "pointercancel"].forEach((ev) => slot.addEventListener(ev, () => slot.classList.remove("hot")));
    slot.addEventListener("click", () => {
      const action = slot.dataset.slot || "";
      if (action === "id") {
        showIdentity(true);
      } else if (action === "minimap") {
        cw18SetMinimap(!cw18MinimapOn);
        cw19SyncToggles();
        return;                                   // interrupteur : le menu reste ouvert
      } else if (action === "chat") {
        cw18SetChatFeed(!cwChatOn);
        cw19SyncToggles();
        return;
      } else {
        bridge?.menuAction?.(action);
      }
      setRadialMenu(false);
      window.setTimeout(() => setRadialMenu(false), 90);
    });
  });

  // [v18] minimap masquable (roue -> Minimap), mémorisé sur le téléphone
  let cw18MinimapOn = true;
  try { cw18MinimapOn = window.localStorage.getItem("cw.minimap.on") !== "0"; } catch (_) { cw18MinimapOn = true; }
  function cw18SetMinimap(on) {
    cw18MinimapOn = Boolean(on);
    try { window.localStorage.setItem("cw.minimap.on", cw18MinimapOn ? "1" : "0"); } catch (_) {}
    document.body.classList.toggle("cw-minimap-off", !cw18MinimapOn);
    document.querySelectorAll('[data-slot="minimap"]').forEach((b) => b.classList.toggle("off", !cw18MinimapOn));
    cw18Tip(cw18MinimapOn ? "Minimap affichée" : "Minimap masquée");
    window.setTimeout(syncTouchBounds, 30);
  }
  function cw18SetChatFeed(on) {
    cwChatOn = Boolean(on);
    try { window.localStorage.setItem("cw.chat.on", cwChatOn ? "1" : "0"); } catch (_) {}
    cwChatApply();
    cw18Tip(cwChatOn ? "Chat affiché" : "Chat masqué · maintiens le bouton pour le réafficher");
  }
  let cw18TipTimer = 0;
  function cw18Tip(text) {
    const t = document.querySelector("#cw18Tip");
    if (!t) return;
    t.textContent = text;
    t.hidden = false;
    t.classList.remove("show"); void t.offsetWidth; t.classList.add("show");
    window.clearTimeout(cw18TipTimer);
    cw18TipTimer = window.setTimeout(() => { t.classList.remove("show"); t.hidden = true; }, 2200);
  }

  // [v18] petit bouton chat près de la carte : toucher = écrire / envoyer, maintenir = masquer / afficher le chat
  const cw18Pin = document.querySelector("#cwChatPin");
  if (cw18Pin) {
    let holdTimer = 0, held = false;
    const startHold = (event) => {
      holdHudTouch(event);
      if (state.hudEditing) return;
      held = false;
      cw18Pin.classList.add("holding");
      window.clearTimeout(holdTimer);
      holdTimer = window.setTimeout(() => {
        held = true;
        cw18Pin.classList.remove("holding");
        if (navigator.vibrate) { try { navigator.vibrate(28); } catch (_) {} }
        if (state.chatOpen) setChatComposer(false);
        cw18SetChatFeed(!cwChatOn);
      }, 520);
    };
    const endHold = () => { window.clearTimeout(holdTimer); cw18Pin.classList.remove("holding"); };
    cw18Pin.addEventListener("pointerdown", startHold);
    cw18Pin.addEventListener("touchstart", (e) => holdHudTouch(e), { passive: false });
    ["pointerup", "pointerleave", "pointercancel"].forEach((ev) => cw18Pin.addEventListener(ev, endHold));
    cw18Pin.addEventListener("click", (event) => {
      event.preventDefault();
      event.stopPropagation();
      if (state.hudEditing) { selectHudItem(cw18Pin); return; }
      if (held) { held = false; return; }
      if (!cwChatOn) cw18SetChatFeed(true);
      setChatComposer(!state.chatOpen);
    });
  }
  // [v18] rangée rapide : paramètres, téléphone
  [["#cwQaSettings", "settings"], ["#cwQaPhone", "phone"]].forEach(([sel, action]) => {
    const b = document.querySelector(sel);
    if (!b) return;
    ["pointerdown", "mousedown"].forEach((ev) => b.addEventListener(ev, holdHudTouch));
    b.addEventListener("touchstart", holdHudTouch, { passive: false });
    b.addEventListener("click", (event) => {
      event.preventDefault();
      event.stopPropagation();
      if (state.hudEditing) return;
      setRadialMenu(false);
      setChatComposer(false);
      bridge?.menuAction?.(action);
    });
  });
  document.body.classList.toggle("cw-minimap-off", !cw18MinimapOn);
  document.querySelectorAll('[data-slot="minimap"]').forEach((b) => b.classList.toggle("off", !cw18MinimapOn));

  document.querySelectorAll("[data-id-close]").forEach((item) => {
    item.addEventListener("click", hideIdentity);
  });

  idPhotoButton?.addEventListener("click", openIdCamera);
  idCameraCancel?.addEventListener("click", () => closeIdCamera(true));
  idCameraShot?.addEventListener("click", captureIdPhoto);
  idCameraFrame?.addEventListener("pointerdown", (event) => {
    if (!state.idCameraOpen) return;
    idCameraDragging = true;
    idCameraLastX = event.clientX;
    idCameraLastY = event.clientY;
    idCameraFrame.setPointerCapture?.(event.pointerId);
    event.preventDefault();
  });
  idCameraFrame?.addEventListener("pointermove", (event) => {
    if (!idCameraDragging) return;
    const dx = event.clientX - idCameraLastX;
    const dy = event.clientY - idCameraLastY;
    idCameraLastX = event.clientX;
    idCameraLastY = event.clientY;
    adjustIdCamera(dx, dy);
    event.preventDefault();
  });
  ["pointerup", "pointercancel", "pointerleave"].forEach((type) => {
    idCameraFrame?.addEventListener(type, (event) => {
      if (!idCameraDragging) return;
      idCameraDragging = false;
      idCameraFrame.releasePointerCapture?.(event.pointerId);
    });
  });
  idSignButton?.addEventListener("click", openSignature);
  idSignCancel?.addEventListener("click", closeSignature);
  idSignClear?.addEventListener("click", setupSignatureCanvas);
  idSignSave?.addEventListener("click", saveSignature);

  idSignCanvas?.addEventListener("pointerdown", (event) => {
    if (!signatureCanvasReady) setupSignatureCanvas();
    const ctx = idSignCanvas.getContext("2d");
    if (!ctx) return;
    signatureDrawing = true;
    signatureHasInk = true;
    idSignCanvas.setPointerCapture?.(event.pointerId);
    const point = pointerOnCanvas(event);
    activeSignatureStroke = [point];
    signatureStrokes.push(activeSignatureStroke);
    ctx.beginPath();
    ctx.moveTo(point.x, point.y);
    event.preventDefault();
  });

  idSignCanvas?.addEventListener("pointermove", (event) => {
    if (!signatureDrawing) return;
    const ctx = idSignCanvas.getContext("2d");
    if (!ctx) return;
    const point = pointerOnCanvas(event);
    activeSignatureStroke?.push(point);
    ctx.lineTo(point.x, point.y);
    ctx.stroke();
    event.preventDefault();
  });

  ["pointerup", "pointercancel", "pointerleave"].forEach((type) => {
    idSignCanvas?.addEventListener(type, (event) => {
      if (!signatureDrawing) return;
      signatureDrawing = false;
      activeSignatureStroke = null;
      idSignCanvas.releasePointerCapture?.(event.pointerId);
    });
  });

  document.addEventListener("pointerdown", (event) => {
    if (state.chatOpen) {
      if (chatComposer?.contains(event.target) || chatAction?.contains(event.target)) return;
      setChatComposer(false);
      return;
    }

    if (state.menuOpen) {
      const radialWheel = radialMenu?.querySelector(".radial-wheel");
      if (radialWheel?.contains(event.target) || cornerAction?.contains(event.target)) return;
      setRadialMenu(false);
    }
  });

  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape") {
      if (!radioFrequencyLayer?.hidden) {
        closeRadioFrequency();
        return;
      }
      if (state.hudEditing) {
        applyHudLayout(true);
        setHudEditor(false);
        return;
      }
      if (state.idSigning) {
        closeSignature();
        return;
      }
      if (state.idCameraOpen) {
        closeIdCamera(true);
        return;
      }
      if (state.idOpen) {
        hideIdentity();
        return;
      }
      setChatComposer(false);
      setRadialMenu(false);
    }
  });

  window.HudApp = {
    openFromNative(payload) {
      document.body.classList.add("hud-ready");
      root?.classList.add("ready");
      updateHudInfo(payload);
      scheduleHudSync();
    },
    closeFromNative() {
      closeInteractive();
      root?.classList.remove("ready");
      document.body.classList.remove("hud-ready");
    },
    closeInteractiveFromNative: closeInteractive,
    updateHudInfo,
    setStatus,
    focusChatInput,
    showChat() {
      setChatComposer(true);
    },
    clearChat,
    addChatMessage,
    showPhoneNotification,
    receiveGameNotification,
    showSpeedFineNotice,
    showRadarBotAlert,
    rgCameraNativeStatus,
    receiveIdentityEvent,
    radioNativeDown: startRadioTalk,
    radioNativeUp: stopRadioTalk,
    updateEngine(payload) {
      updateHudInfo(payload);
    },
    openHudEditorFromNative() {
      document.body.classList.add("hud-ready");
      root?.classList.add("ready");
      setHudEditor(true);
      scheduleHudSync();
    },
    refreshFromNative() {
      document.body.classList.add("hud-ready");
      root?.classList.add("ready");
      scheduleHudSync();
    }
  };

  // Compatibilité avec l'ancien pont natif (HudBridge.setPlayerState / setVisible)
  window.HudBridge = {
    setPlayerState(next) {
      if (typeof next === "string") {
        try { next = JSON.parse(next); } catch (_) { return; }
      }
      if (!next || typeof next !== "object") return;
      const mapped = {};
      if (next.health !== undefined) mapped.health = next.dead ? 0 : next.health;
      if (next.armour !== undefined) mapped.armour = next.armour;
      if (next.thirst !== undefined) mapped.thirst = next.thirst;
      if (next.hunger !== undefined) mapped.hunger = next.hunger;
      if (next.energy !== undefined) mapped.energy = next.exhausted ? 0 : next.energy;
      if (next.inVehicle !== undefined) mapped.inVehicle = next.inVehicle ? 1 : 0;
      if (next.speed !== undefined) mapped.speed = next.speed;
      if (next.vehHealth !== undefined) mapped.vehicleHealth = next.vehHealth;
      if (next.fuel !== undefined) mapped.fuel = next.fuel;
      if (next.posX !== undefined && window.Minimap) window.Minimap.update(next);
      updateHudInfo(mapped);
    },
    setVisible(visible) { if (root) root.style.display = visible === false ? "none" : ""; },
    setInteract,
    setWheelOpen() {},
    setVehicleMode() {},
    reset() {}
  };

  startBrasiliaClock();
  window.addEventListener("focus", () => tickBrasiliaClock(true));
  document.addEventListener("visibilitychange", () => tickBrasiliaClock(true));
  window.addEventListener("resize", () => { reframeHud(); syncTouchBounds(); });
  window.addEventListener("orientationchange", () => window.setTimeout(() => { reframeHud(); syncTouchBounds(); }, 200));
  scheduleHudSync();
  updateHudInfo(state);
  tickBrasiliaClock(true);
  updateBridgeInteraction();
  notifyNativeReady();

  if (new URLSearchParams(window.location.search).get("camera") === "phone") {
    document.body.classList.add("hud-ready");
    root?.classList.add("ready");
    openPhoneCamera();
  }

  const previewMode = new URLSearchParams(window.location.search).get("preview");
  if (previewMode) {
    window.setTimeout(() => {
      const vehiclePreview = previewMode === "vehicle"
        ? ";energy=71;armour=46;inVehicle=1;speed=127;vehicleHealth=63;fuel=100"
        : ";energy=84;armour=46;inVehicle=0;speed=0;vehicleHealth=100;fuel=100";
      window.HudApp.openFromNative(`health=92;hunger=78;thirst=64;money=2485;coin=0;accountId=316;name=Trevor_Vivo;location=Los_Santos;hour=16;minute=4${vehiclePreview}`);
      if (previewMode === "vehicle" || previewMode === "status") return;
      window.HudApp.receiveIdentityEvent(`type=steps;mask=${previewMode === "steps" ? 1 : 31};done=0;claimed=${previewMode === "claimed" ? 1 : 0};phone=0;bank=0;document=0`);
      if (previewMode === "steps" || previewMode === "claim") {
        state.firstStepsExpanded = true;
        renderFirstSteps();
      }
      const notificationPreviews = {
        "notify-success": "type=success;title=Sedex;message=Voce#20colocou#20uma#20caixa#20no#20bau.#20Faltam#209.;duration=9000",
        "notify-denied": "type=denied;title=Negado;message=Voce#20nao#20tem#20permissao.;duration=9000",
        "notify-warning": "type=warning;title=Aviso;message=Veiculos#20sem#20motorista#20serao#20respawnados#20em#201#20minuto.;duration=9000",
        "notify-system": "type=system;title=Sistema;message=Siga#20a#20missao#20de#20Primeiros#20Passos.;duration=9000",
        "notify-admin": "type=admin;title=Atendimento;message=Um#20novo#20atendimento#20foi#20solicitado.;duration=9000"
      };
      window.HudApp.receiveGameNotification(notificationPreviews[previewMode] || notificationPreviews["notify-system"]);
      if (previewMode === "documents") window.HudApp.receiveIdentityEvent("type=flow;view=documents;price=200");
      if (previewMode === "phone") window.HudApp.receiveIdentityEvent("type=flow;view=phone_shop;phoneprice=500;radioprice=500;burgerprice=40;waterprice=20;ownphone=0;ownradio=0;cash=1500");
      if (previewMode === "bike") window.HudApp.receiveIdentityEvent("type=flow;view=bike;price=50;active=0");
      if (previewMode === "reward") window.HudApp.receiveIdentityEvent("type=flow;view=reward;money=7000;xp=10");
    }, 60);
  }
})();
