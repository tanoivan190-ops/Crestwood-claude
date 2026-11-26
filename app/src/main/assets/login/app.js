/* =====================================================================
   Crestwood RP v51 · Arrivee en ville (connexion Google uniquement)
   Pont natif (SAMP.java game + game32) :
     page -> jeu : SampAuth.action(type, json)  type = create | link | identity | promocheck |
                   gpreview | introskip | enter | back | spawnsel | spawn
                   SampAuth.google(json) (connexion Google -> ticket -> /authact google)
                   SampAuth.googleAccount() / SampAuth.ready()
     jeu -> page : DXAuth.openFromNative({"screen":"google|intro|create|link|identity|lobby|spawn",...})
                   DXAuth.handleResponse({"type":"success|error|info|warning","title":"..","message":"..",
                                          "screen":"..","retry":1,"promo":{...}})
                   DXAuth.googleStep(email) / DXAuth.googleError(message, annule)
   ===================================================================== */
(() => {
  "use strict";
  const T = (x) => (window.CwI18n ? window.CwI18n.t(x) : x);
  const $ = (id) => document.getElementById(id);
  const root = $("cwAuth");
  const busy = $("cwBusy");
  let screen = "google";
  let busyTimer = 0;
  let playerName = "";
  let googlePort = 7790;

  // ---------------------------------------------------------------- notifications
  const stack = $("notificationContainer");
  const SA = { success: "#5cc85f", error: "#e2484c", warning: "#f3c75c", info: "#6f9cff" };
  const DEF_TITLE = { success: "Succès", error: "Erreur", warning: "Attention", info: "" };
  const esc = (v) => String(v == null ? "" : v).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
  function notify(message, type = "info", options = {}) {
    if (type === "danger") type = "error";
    if (type === "alert") type = "warning";
    if (!SA[type]) type = "info";
    const text = String(message || "").trim();
    if (!text || !stack) return null;
    const title = options.title != null ? String(options.title) : DEF_TITLE[type];
    const box = document.createElement("article");
    box.className = "sa-help";
    box.style.setProperty("--sa", SA[type]);
    box.innerHTML = (title ? `<b>${esc(title)}</b>` : "") + esc(text);
    stack.appendChild(box);
    while (stack.children.length > 2) stack.firstElementChild.remove();
    requestAnimationFrame(() => box.classList.add("show"));
    const duration = Math.max(2500, Math.min(12000, Number(options.duration) || 5200));
    setTimeout(() => { box.classList.remove("show"); setTimeout(() => box.remove(), 220); }, duration);
    return box;
  }

  // ---------------------------------------------------------------- envoi au jeu
  function emit(type, payload) {
    const json = JSON.stringify(payload || {});
    try { if (window.SampAuth && typeof window.SampAuth.action === "function") { window.SampAuth.action(type, json); return true; } } catch (e) { console.warn(e); }
    console.log("[Crestwood Auth]", type, json);
    return false;
  }
  function setBusy(on, ms) {
    clearTimeout(busyTimer);
    busy.hidden = !on;
    document.querySelectorAll(".cw-cta").forEach((b) => { b.disabled = !!on; });
    if (on) busyTimer = setTimeout(() => setBusy(false), ms || 8000);
  }

  // ---------------------------------------------------------------- ecrans
  function show(name) {
    const next = $("screen-" + name);
    if (!next) return;
    if (screen === "intro" && name !== "intro") filmStop();
    screen = name;
    root.dataset.screen = name;
    document.querySelectorAll(".cw-screen").forEach((s) => s.classList.toggle("active", s === next));
  }
  document.querySelectorAll("[data-go]").forEach((b) => b.addEventListener("click", () => show(b.dataset.go)));

  // ---------------------------------------------------------------- connexion Google
  const gStatus = $("gStatus"), gMail = $("gMail"), gProgress = $("gProgress"), gButton = $("gButton"), gRetry = $("gRetry");
  let googleTimer = 0, googleRunning = false;
  function maskEmail(e) {
    const m = /^([^@]+)@(.+)$/.exec(String(e || ""));
    if (!m) return "";
    const keep = m[1].length >= 3 ? 2 : 1;
    return m[1].slice(0, keep) + "•".repeat(Math.min(4, Math.max(1, m[1].length - keep))) + "@" + m[2];
  }
  function googleAccount() {
    try { if (window.SampAuth && window.SampAuth.googleAccount) return JSON.parse(window.SampAuth.googleAccount() || "{}") || {}; } catch (_) {}
    return {};
  }
  function googleUi(state, text) {
    // state : run | idle | error
    gProgress.classList.toggle("off", state !== "run");
    gButton.hidden = state === "run" || state === "error";
    gRetry.hidden = state !== "error";
    if (text) gStatus.textContent = text;
  }
  function googleStart(choose) {
    if (googleRunning) return;
    if (!window.SampAuth || typeof window.SampAuth.google !== "function") {
      googleUi("error", T("Mets à jour l'application."));
      return;
    }
    const a = googleAccount();
    gMail.textContent = a.email ? maskEmail(a.email) : "";
    googleRunning = true;
    googleUi("run", a.email ? T("Connexion à ton compte Google…") : T("Choisis ton compte Google…"));
    clearTimeout(googleTimer);
    googleTimer = setTimeout(() => { googleRunning = false; googleUi("error", T("Pas de réponse. Réessaie.")); }, 40000);
    try { window.SampAuth.google(JSON.stringify({ name: playerName, port: googlePort, promo: "", choose: choose ? 1 : 0 })); }
    catch (e) { googleRunning = false; googleUi("error", T("Connexion Google impossible.")); }
  }
  gButton.addEventListener("click", () => googleStart(true));
  gRetry.addEventListener("click", () => googleStart(false));
  function googleStep(email) {
    if (email) gMail.textContent = maskEmail(email);
    gStatus.textContent = T("Vérification de ton compte…");
    clearTimeout(googleTimer);
    googleTimer = setTimeout(() => { googleRunning = false; googleUi("error", T("Le serveur ne répond pas. Réessaie.")); }, 15000);
  }
  function googleError(message, cancelled) {
    clearTimeout(googleTimer);
    googleRunning = false;
    if (cancelled) { googleUi("idle", T("Connecte-toi avec Google pour jouer.")); return; }
    googleUi("error", T("Connexion impossible."));
    if (message) notify(String(message), "error", { title: "Google" });
  }

  // ---------------------------------------------------------------- cinematique
  const CAPTIONS = [
    ["Los Santos · 20 h 40", "Une ville qui ne dort jamais."],
    ["Centre-ville", "Des milliers de vies."],
    ["Grove Street", "Ici, personne ne te connaît."],
    ["Vinewood", "Pas encore."],
    ["Mairie de Los Santos", "Ton histoire commence ici."]
  ];
  const fBlack = $("filmBlack"), fCap = $("filmCaption"), fSmall = $("filmSmall"), fBig = $("filmBig"), fEnd = $("filmEnd"), fCount = $("filmCount");
  let filmStep = -1, filmEndTimer = 0;
  function filmStep_(step, n) {
    if (step === filmStep) return;
    filmStep = step;
    setTimeout(() => fBlack.classList.add("off"), 500);
    const c = CAPTIONS[Math.max(0, Math.min(CAPTIONS.length - 1, step))];
    fCap.classList.remove("show");
    void fCap.offsetWidth;
    fSmall.textContent = T(c[0]);
    fBig.textContent = T(c[1]);
    fCap.classList.add("show");
    fCount.textContent = String(step + 1).padStart(2, "0") + " / " + String(n || CAPTIONS.length).padStart(2, "0");
    clearTimeout(filmEndTimer);
    if (step >= (n || CAPTIONS.length) - 1) filmEndTimer = setTimeout(() => { fCap.classList.remove("show"); fEnd.classList.add("on"); }, 3200);
  }
  function filmStop() {
    clearTimeout(filmEndTimer);
    filmStep = -1;
    fEnd.classList.remove("on");
    fBlack.classList.remove("off");
    fCap.classList.remove("show");
  }
  $("filmSkip").addEventListener("click", () => emit("introskip", {}));

  // ---------------------------------------------------------------- formulaires d'identite
  const MONTHS = ["Janvier", "Février", "Mars", "Avril", "Mai", "Juin", "Juillet", "Août", "Septembre", "Octobre", "Novembre", "Décembre"];
  const ORIGINS = ["Côte d'Ivoire", "Sénégal", "Mali", "Burkina Faso", "Guinée", "Cameroun", "Bénin", "Togo", "Niger", "Gabon", "Congo", "RD Congo",
    "Ghana", "Nigeria", "Maroc", "Algérie", "Tunisie", "France", "Belgique", "Suisse", "Canada", "Haïti", "États-Unis", "Autre"];
  const opt = (sel, v, t) => { const o = document.createElement("option"); o.value = v; o.textContent = t; sel.appendChild(o); };
  const ascii = (v) => String(v || "").normalize("NFD").replace(/[̀-ͯ]/g, "").replace(/[^A-Za-z -]/g, "").trim();
  function fillDate(d, m, y) {
    opt(d, "", "JJ"); for (let i = 1; i <= 31; i++) opt(d, i, i);
    opt(m, "", "Mois"); MONTHS.forEach((x, i) => opt(m, i + 1, T(x)));
    opt(y, "", "AAAA"); for (let i = 2008; i >= 1940; i--) opt(y, i, i);
  }
  function fillOrigin(o, other, wrap) {
    opt(o, "", T("Choisis"));
    ORIGINS.forEach((x) => opt(o, x, T(x)));
    o.addEventListener("change", () => { wrap.hidden = o.value !== "Autre"; });
  }
  function readIdentity(d, m, y, o, other) {
    if (!d.value || !m.value || !y.value) return { err: "Indique ta date de naissance." };
    const dim = new Date(Number(y.value), Number(m.value), 0).getDate();
    if (Number(d.value) > dim) return { err: "Cette date n'existe pas." };
    const origin = o.value === "Autre" ? ascii(other.value) : ascii(o.value);
    if (origin.length < 2) return { err: "Indique ton origine." };
    return { birth: String(d.value).padStart(2, "0") + "/" + String(m.value).padStart(2, "0") + "/" + y.value, origin: origin.replace(/ /g, "_").slice(0, 22) };
  }
  function bad(el, msg) {
    const f = el && el.closest(".cw-in");
    if (f) { f.classList.add("bad"); setTimeout(() => f.classList.remove("bad"), 1600); }
    notify(T(msg), "error");
    return false;
  }
  function segBind(seg, onPick) {
    seg.querySelectorAll("button").forEach((b) => b.addEventListener("click", () => {
      seg.querySelectorAll("button").forEach((x) => { x.classList.toggle("on", x === b); x.setAttribute("aria-checked", String(x === b)); });
      onPick(Number(b.dataset.gender));
    }));
  }
  function segSet(seg, g) {
    seg.querySelectorAll("button").forEach((x) => { const on = Number(x.dataset.gender) === g; x.classList.toggle("on", on); x.setAttribute("aria-checked", String(on)); });
  }

  // creation
  const cFirst = $("cFirst"), cLast = $("cLast"), cSeg = document.querySelector("#screen-create .cw-seg");
  const cDay = $("cDay"), cMonth = $("cMonth"), cYear = $("cYear"), cOrigin = $("cOrigin"), cOther = $("cOther"), cPromo = $("cPromo"), cReward = $("cPromoReward");
  let cGender = 0, previewTimer = 0;
  fillDate(cDay, cMonth, cYear);
  fillOrigin(cOrigin, cOther, $("cOtherWrap"));
  const namePart = (v) => { const s = ascii(v).replace(/[^A-Za-z]/g, ""); return s ? s.charAt(0).toUpperCase() + s.slice(1).toLowerCase() : ""; };
  [cFirst, cLast].forEach((i) => i.addEventListener("input", () => { const v = namePart(i.value); if (i.value !== v) i.value = v; }));
  segBind(cSeg, (g) => {
    cGender = g;
    clearTimeout(previewTimer);
    previewTimer = setTimeout(() => emit("gpreview", { gender: g }), 150);
  });
  const cleanPromo = (v) => String(v || "").toUpperCase().replace(/[^A-Z0-9]/g, "").slice(0, 15);
  cPromo.addEventListener("input", () => { const c = cleanPromo(cPromo.value); if (cPromo.value !== c) cPromo.value = c; cReward.hidden = true; });
  $("cPromoCheck").addEventListener("click", () => {
    const code = cleanPromo(cPromo.value);
    if (code.length < 3) return bad(cPromo, "Entre un code promo (3 à 15 lettres ou chiffres).");
    setBusy(true);
    emit("promocheck", { code });
  });
  $("createForm").addEventListener("submit", (e) => {
    e.preventDefault();
    const first = namePart(cFirst.value), last = namePart(cLast.value);
    if (first.length < 2 || first.length > 12) return bad(cFirst, "Prénom : 2 à 12 lettres.");
    if (last.length < 2 || last.length > 12) return bad(cLast, "Nom : 2 à 12 lettres.");
    const id = readIdentity(cDay, cMonth, cYear, cOrigin, cOther);
    if (id.err) return notify(T(id.err), "error");
    setBusy(true);
    emit("create", { name: first + "_" + last, birth: id.birth, gender: cGender, origin: id.origin, promo: cleanPromo(cPromo.value) });
  });
  $("suggestBox").addEventListener("click", () => { $("lName").value = $("suggestName").textContent; show("link"); setTimeout(() => $("lPass").focus(), 200); });

  // ancien personnage
  $("linkForm").addEventListener("submit", (e) => {
    e.preventDefault();
    const n = $("lName"), p = $("lPass");
    const user = n.value.trim(), pass = p.value;
    if (!/^[A-Za-z0-9_\[\]\.$@=()]{3,24}$/.test(user)) return bad(n, "Pseudo invalide.");
    if (!pass || /\s/.test(pass)) return bad(p, "Entre ton ancien mot de passe.");
    setBusy(true);
    emit("link", { username: user, password: pass });
  });

  // identite (ancien compte)
  const iSeg = $("idSeg");
  let iGender = -1;
  fillDate($("iDay"), $("iMonth"), $("iYear"));
  fillOrigin($("iOrigin"), $("iOther"), $("iOtherWrap"));
  segBind(iSeg, (g) => { iGender = g; });
  $("identityForm").addEventListener("submit", (e) => {
    e.preventDefault();
    if (iGender !== 0 && iGender !== 1) return notify(T("Choisis Homme ou Femme."), "error");
    const id = readIdentity($("iDay"), $("iMonth"), $("iYear"), $("iOrigin"), $("iOther"));
    if (id.err) return notify(T(id.err), "error");
    setBusy(true);
    emit("identity", { birth: id.birth, gender: iGender, origin: id.origin });
  });
  window.CwIdentity = { open() { show("identity"); } };

  // ---------------------------------------------------------------- salon
  function fillLobby(d) {
    const parts = String(d.name || playerName || "").split("_");
    $("lbFirst").textContent = parts[0] || "";
    $("lbLast").textContent = parts.slice(1).join(" ") || "";
    $("lbBirth").textContent = d.birth || "Non indiqué";
    $("lbOrigin").textContent = d.origin || "Non indiqué";
    $("lbCash").textContent = d.cash != null && d.cash !== "" ? d.cash + " $" : "Non indiqué";
    $("lbBank").textContent = d.bank != null && d.bank !== "" ? d.bank + " $" : "Non indiqué";
    const a = googleAccount();
    $("lbMail").textContent = d.email || (a.email ? maskEmail(a.email) : T("Compte Google"));
  }
  $("lbEnter").addEventListener("click", () => { setBusy(true, 3000); emit("enter", {}); });

  // ---------------------------------------------------------------- point d'apparition
  const PLACES = [
    { tag: "Centre-ville", name: "Mairie de Los Santos", desc: "City Hall : papiers, emplois et premiers pas." },
    { tag: "El Corona", name: "Gare Unity Station", desc: "Trains, bus et taxis : toute la ville à portée." },
    { tag: "Sud de la ville", name: "Aéroport de Los Santos", desc: "Tu viens d'atterrir." },
    { tag: "Bord de mer", name: "Plage de Santa Maria", desc: "Le sable, la jetée et le soleil." },
    { tag: "Ganton", name: "Grove Street", desc: "Le quartier. Tout le monde se connaît." },
    { tag: "Market", name: "Hôpital All Saints", desc: "Les urgences à deux pas." }
  ];
  const LAST = { tag: "Dernière position", name: "Là où tu t'es arrêté", desc: "Reprends exactement où tu étais." };
  const strip = $("spStrip"), place = $("spPlace");
  let spSel = 0, spCount = PLACES.length, spLast = false, spSelTimer = 0;
  function placeOf(i) { return i === spCount ? LAST : PLACES[i] || PLACES[0]; }
  function renderPlace() {
    const p = placeOf(spSel);
    $("spTag").textContent = T(p.tag);
    $("spName").textContent = T(p.name);
    $("spDesc").textContent = T(p.desc);
    place.classList.remove("swap"); void place.offsetWidth; place.classList.add("swap");
    strip.querySelectorAll(".cw-card").forEach((c) => c.classList.toggle("on", Number(c.dataset.i) === spSel));
    const on = strip.querySelector(".cw-card.on");
    if (on && on.scrollIntoView) on.scrollIntoView({ inline: "center", block: "nearest", behavior: "smooth" });
  }
  function buildStrip() {
    strip.innerHTML = "";
    const order = [];
    if (spLast) order.push(spCount);
    for (let i = 0; i < spCount; i++) order.push(i);
    order.forEach((i, k) => {
      const p = placeOf(i);
      const b = document.createElement("button");
      b.type = "button";
      b.className = "cw-card" + (i === spCount ? " last" : "");
      b.dataset.i = String(i);
      b.innerHTML = `<small>${String(k + 1).padStart(2, "0")}</small><b>${esc(T(p.name))}</b>`;
      b.addEventListener("click", () => {
        if (spSel === i) return;
        spSel = i;
        renderPlace();
        clearTimeout(spSelTimer);
        spSelTimer = setTimeout(() => emit("spawnsel", { n: i }), 120);
      });
      strip.appendChild(b);
    });
  }
  $("spGo").addEventListener("click", () => { setBusy(true, 6000); emit("spawn", { n: spSel }); });
  $("spBack").addEventListener("click", () => emit("back", {}));

  // ---------------------------------------------------------------- reponses du serveur
  function showPromo(p) {
    if (!p || !p.ok) { cReward.hidden = true; return; }
    const money = Number(p.money || 0).toLocaleString("fr-FR");
    cReward.textContent = `✓ ${p.code} · ${p.veh || "Véhicule"} ${p.days || 0} jours + ${money} $ à ton arrivée`;
    cReward.hidden = false;
  }
  function handleResponse(raw) {
    let r = raw;
    try { if (typeof raw === "string") r = JSON.parse(raw); } catch (_) { setBusy(false); return; }
    if (!r || typeof r !== "object") return;
    setBusy(false);
    const map = { success: "success", danger: "error", error: "error", warning: "warning", info: "info" };
    const type = map[String(r.type || "info").toLowerCase()] || "info";
    if (r.promo) showPromo(r.promo);
    if (r.message) notify(r.message, type, { title: r.title });
    if (screen === "google" && (r.retry || type === "error")) { googleRunning = false; clearTimeout(googleTimer); googleUi("error", T("Connexion impossible.")); }
    if (r.screen && $("screen-" + r.screen)) show(r.screen);
  }

  function openFromNative(raw) {
    let d = raw;
    try { if (typeof raw === "string") d = JSON.parse(raw || "{}"); } catch (_) { d = {}; }
    d = d || {};
    setBusy(false);
    if (d.name && /^[A-Za-z0-9_\[\]\.$@=()]{3,24}$/.test(String(d.name))) playerName = String(d.name);
    const gp = Number(d.gp);
    if (gp > 0 && gp < 65536) googlePort = gp;
    const s = String(d.screen || "google");
    if (s === "intro") { show("intro"); filmStep_(Number(d.step) || 0, Number(d.n) || 5); return; }
    if (s === "create") {
      const sug = String(d.suggest || "");
      $("suggestBox").hidden = !sug;
      $("suggestName").textContent = sug;
      cGender = Number(d.gender) === 1 ? 1 : 0;
      segSet(cSeg, cGender);
      show("create");
      return;
    }
    if (s === "lobby") { fillLobby(d); show("lobby"); return; }
    if (s === "spawn") {
      spCount = Number(d.count) || PLACES.length;
      spLast = Number(d.last) === 1;
      spSel = Number(d.sel) || 0;
      if (spSel === spCount && !spLast) spSel = 0;
      buildStrip();
      show("spawn");
      renderPlace();
      return;
    }
    if (s === "identity") { show("identity"); return; }
    if (s === "link") { show("link"); return; }
    show("google");
    googleStart(false);
  }

  // ---------------------------------------------------------------- musique
  const TRACKS = [
    { src: "../loading/music/track1.mp3", title: "SPRINTER · Central Cee x Dave" },
    { src: "../loading/music/track2.mp3", title: "PILIER · Ninho" },
    { src: "../loading/music/track3.mp3", title: "PACMAN DE PANICAT · MC Ryan SP" },
    { src: "../loading/music/track4.mp3", title: "EU PREFIRO A RUA · Pacman de Panicat" },
    { src: "../loading/music/track5.mp3", title: "BASE AUTOMOTIVA · Pacman de Panicat" },
    { src: "../loading/music/track6.mp3", title: "MERIDIAN · Tiakola x Dave" }
  ];
  const audio = new Audio();
  audio.preload = "auto";
  let trackIndex = Math.floor(Math.random() * TRACKS.length), musicStopped = false, musicErrors = 0;
  const icon = $("musicIcon"), vol = $("musicVolume");
  const ICON_PAUSE = "M7 5h4v14H7zM13 5h4v14h-4z", ICON_PLAY = "M8 5v14l11-7z";
  function loadTrack(i) { trackIndex = (i + TRACKS.length) % TRACKS.length; audio.src = TRACKS[trackIndex].src; $("musicTitle").textContent = TRACKS[trackIndex].title; }
  function play() { if (musicStopped) return; const p = audio.play(); if (p && p.catch) p.catch(() => {}); }
  function setVol(v) { v = Math.max(0, Math.min(100, Number(v) || 0)); audio.volume = v / 100; vol.style.setProperty("--v", v + "%"); }
  audio.addEventListener("play", () => icon.setAttribute("d", ICON_PAUSE));
  audio.addEventListener("pause", () => icon.setAttribute("d", ICON_PLAY));
  audio.addEventListener("playing", () => { musicErrors = 0; });
  audio.addEventListener("ended", () => { loadTrack(trackIndex + 1); play(); });
  audio.addEventListener("error", () => { if (musicStopped || ++musicErrors > TRACKS.length) return; setTimeout(() => { loadTrack(trackIndex + 1); play(); }, 300); });
  $("musicToggle").addEventListener("click", () => { if (audio.paused) play(); else audio.pause(); });
  $("musicNext").addEventListener("click", () => { loadTrack(trackIndex + 1); play(); });
  vol.addEventListener("input", () => setVol(vol.value));
  try { const saved = localStorage.getItem("cw_login_volume"); if (saved !== null) vol.value = saved; } catch (_) {}
  vol.addEventListener("change", () => { try { localStorage.setItem("cw_login_volume", vol.value); } catch (_) {} });
  setVol(vol.value);
  icon.setAttribute("d", ICON_PLAY);
  loadTrack(trackIndex);
  play();
  ["pointerdown", "touchstart", "keydown"].forEach((ev) => window.addEventListener(ev, (e) => {
    if (e.target && e.target.closest && e.target.closest(".cw-player")) return;
    if (audio.paused && !musicStopped) play();
  }, { capture: true, once: true }));
  function stopMusic() { musicStopped = true; try { audio.pause(); audio.removeAttribute("src"); audio.load(); } catch (_) {} }

  // ---------------------------------------------------------------- API appelee par le jeu
  window.DXAuth = {
    handleResponse,
    openFromNative,
    googleStep,
    googleError,
    showConnexion() { show("google"); },
    startMusic() { musicStopped = false; if (!audio.src) loadTrack(trackIndex); play(); },
    stopMusic,
    forceCloseFromGame: stopMusic,
    setRememberedConnexion() {},
    clearRememberedConnexion() {}
  };
  window.RevoltAuth = { notify, openScreen(s) { if ($("screen-" + s)) show(s); }, setRememberedConnexion() {}, clearFields() {} };

  const ready = () => { try { if (window.SampAuth && typeof window.SampAuth.ready === "function") window.SampAuth.ready(); } catch (_) {} };
  if (document.readyState === "complete") ready(); else window.addEventListener("load", ready);

  // apercu dans un navigateur (sans le jeu) : ?preview=google|intro|create|link|identity|lobby|spawn
  if (!window.SampAuth && /[?&]preview=/.test(location.search)) {
    root.classList.add("preview");
    const mode = (location.search.match(/preview=([a-z]+)/) || [])[1] || "google";
    if (mode === "lobby") openFromNative({ screen: "lobby", name: "Anzo_Smith", birth: "12/03/1998", origin: "Cote d'Ivoire", cash: "12 500", bank: "248 900", email: "an••••@gmail.com" });
    else if (mode === "spawn") openFromNative({ screen: "spawn", sel: 6, last: 1, count: 6 });
    else if (mode === "intro") { openFromNative({ screen: "intro", step: 1, n: 5 }); }
    else if (mode === "create") openFromNative({ screen: "create", suggest: "Anzo_Smith", gender: 0 });
    else if (mode === "google") { show("google"); gMail.textContent = "an••••@gmail.com"; googleUi("run", T("Connexion à ton compte Google…")); }
    else openFromNative({ screen: mode });
  }
})();
