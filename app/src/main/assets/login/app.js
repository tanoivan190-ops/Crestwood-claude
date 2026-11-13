/* =====================================================================
   Crestwood RP · Connexion (v19)
   Pont natif (SAMP.java game + game32) :
     page -> jeu  : SampAuth.action(type, json)   type = login | register | promocheck |
                    requestRecoveryCode | recover | identity        ;  SampAuth.ready()
     jeu -> page  : DXAuth.openFromNative({"screen":"login|register|identity","name":"Prenom_Nom"})
                    DXAuth.handleResponse({"type":"success|error|info|warning","title":"..","message":"..",
                                           "screen":"login","close":1,"promo":{...}})
   Compatibilite : window.RevoltAuth (notify / openScreen / setRememberedConnexion / clearFields).
   ===================================================================== */
(() => {
  "use strict";
  const T = (x) => (window.CwI18n ? window.CwI18n.t(x) : x);   // [v20] langues
  const L = (fr, en, pt) => { const l = window.CwI18n ? window.CwI18n.lang : "fr"; return l === "en" ? en : (l === "pt" ? pt : fr); };
  const $ = (id) => document.getElementById(id);
  const root = $("cwAuth");
  const card = $("cwCard");
  const tabsBox = $("cwTabs");
  const busy = $("cwBusy");
  const NAME_RE = /^[A-Za-z0-9_]{3,24}$/;
  const PASS_OK = (p) => p.length >= 4 && p.length <= 100 && /^[\x21-\x7E]+$/.test(p) && !/[\\/]/.test(p);
  const EMAIL_RE = /^[A-Za-z0-9._+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}$/;
  let currentScreen = "login";
  let busyTimer = 0;
  let lockedName = "";

  // ---------------------------------------------------------------- notifications (boite d'aide GTA SA)
  const stack = $("notificationContainer");
  const SA = { success: "#5cc85f", error: "#e2484c", warning: "#f3c75c", info: "#6f9cff" };
  const DEF_TITLE = { success: "Succès", error: "Erreur", warning: "Attention", info: "" };
  let audioCtx = null;
  function beep(type) {
    try {
      const Ctx = window.AudioContext || window.webkitAudioContext;
      if (!Ctx) return;
      audioCtx = audioCtx || new Ctx();
      if (audioCtx.state === "suspended" && audioCtx.resume) audioCtx.resume().catch(() => {});
      const t = audioCtx.currentTime + 0.01;
      const notes = type === "error" ? [[520, 0], [390, 0.09]] : [[988, 0], [1480, 0.07]];
      notes.forEach(([f, d]) => {
        const o = audioCtx.createOscillator(), g = audioCtx.createGain();
        o.type = "square"; o.frequency.value = f;
        g.gain.setValueAtTime(0.0001, t + d);
        g.gain.exponentialRampToValueAtTime(0.045, t + d + 0.008);
        g.gain.exponentialRampToValueAtTime(0.0001, t + d + 0.07);
        o.connect(g); g.connect(audioCtx.destination);
        o.start(t + d); o.stop(t + d + 0.08);
      });
    } catch (_) {}
  }
  function esc(v) {
    return String(v == null ? "" : v).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
  }
  function notify(message, type = "info", options = {}) {
    if (type === "alert") type = "warning";
    if (type === "danger") type = "error";
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
    root.classList.add("has-note");
    requestAnimationFrame(() => box.classList.add("show"));
    beep(type);
    const duration = Math.max(2500, Math.min(12000, Number(options.duration) || 5200));
    setTimeout(() => {
      box.classList.remove("show");
      setTimeout(() => { box.remove(); if (!stack.children.length) root.classList.remove("has-note"); }, 220);
    }, duration);
    return box;
  }

  // ---------------------------------------------------------------- envoi au jeu
  function emitToGame(type, payload) {
    const name = String(type).replace(/^auth:/, "");
    const json = JSON.stringify(payload || {});
    try {
      if (window.SampAuth && typeof window.SampAuth.action === "function") { window.SampAuth.action(name, json); return true; }
    } catch (e) { console.warn("[Auth] SampAuth.action :", e); }
    const msg = JSON.stringify({ action: "auth:" + name, payload, event: "auth:" + name, data: payload });
    if (window.Android && typeof window.Android.postMessage === "function") { window.Android.postMessage(msg); return true; }
    if (window.cefQuery) { window.cefQuery({ request: msg, onSuccess() {}, onFailure() {} }); return true; }
    console.log("[Crestwood Auth]", name, payload);
    return false;
  }
  function setBusy(on) {
    clearTimeout(busyTimer);
    busy.hidden = !on;
    document.querySelectorAll(".cw-primary").forEach((b) => { b.disabled = !!on; });
    if (on) busyTimer = setTimeout(() => setBusy(false), 7000);
  }

  // ---------------------------------------------------------------- ecrans
  function showScreen(name) {
    const next = $("screen-" + name);
    if (!next) return;
    currentScreen = name;
    document.querySelectorAll(".cw-screen").forEach((s) => s.classList.toggle("active", s === next));
    document.querySelectorAll(".cw-tab").forEach((t) => t.classList.toggle("active", t.dataset.screen === name));
    if (name !== "identity") tabsBox.dataset.active = name;
    card.classList.toggle("identity", name === "identity");
    card.scrollTop = 0;
  }
  document.querySelectorAll(".cw-tab").forEach((t) => t.addEventListener("click", () => showScreen(t.dataset.screen)));
  document.querySelectorAll("[data-go]").forEach((b) => b.addEventListener("click", () => showScreen(b.dataset.go)));
  document.querySelectorAll("[data-eye]").forEach((b) => b.addEventListener("click", () => {
    const input = $(b.dataset.eye);
    if (!input) return;
    input.type = input.type === "password" ? "text" : "password";
    b.classList.toggle("on", input.type === "text");
  }));
  function bad(input, msg) {
    const f = input && input.closest(".cw-field");
    if (f) { f.classList.add("bad"); setTimeout(() => f.classList.remove("bad"), 1800); }
    notify(msg, "error");
    return false;
  }

  // pseudo de jeu impose par le serveur : champ verrouille
  function lockName(name) {
    lockedName = name;
    ["loginUsername", "registerUsername", "recoverUsername"].forEach((id) => {
      const input = $(id);
      if (!input) return;
      input.value = name;
      input.readOnly = !!name;
      input.closest(".cw-field").classList.toggle("locked", !!name);
    });
  }

  // ---------------------------------------------------------------- connexion
  $("loginForm").addEventListener("submit", (e) => {
    e.preventDefault();
    const u = $("loginUsername"), p = $("loginPassword");
    const username = u.value.trim(), password = p.value;
    if (!NAME_RE.test(username)) return bad(u, "Pseudo invalide (3 à 24 lettres, chiffres ou _).");
    if (!password) return bad(p, "Entre ton mot de passe.");
    if (!PASS_OK(password)) return bad(p, "Mot de passe invalide (sans espace, ni / ni \\).");
    window.__cwPending = { username, password, remember: $("rememberConnexion").checked };
    setBusy(true);
    emitToGame("login", { username, password, remember: $("rememberConnexion").checked });
  });

  // ---------------------------------------------------------------- inscription + code promo
  const pw = $("registerPassword"), strength = $("pwStrength");
  pw.addEventListener("input", () => {
    const v = pw.value;
    let s = 0;
    if (v.length >= 4) s++;
    if (v.length >= 8) s++;
    if (/[A-Z]/.test(v) && /[a-z]/.test(v)) s++;
    if (/[0-9]/.test(v) && /[^A-Za-z0-9]/.test(v)) s++;
    strength.dataset.s = v ? String(Math.max(1, s)) : "0";
  });
  const promoInput = $("registerPromo"), promoReward = $("promoReward");
  let promoChecked = "";
  const cleanPromo = (v) => String(v || "").toUpperCase().replace(/[^A-Z0-9]/g, "").slice(0, 15);
  promoInput.addEventListener("input", () => {
    const c = cleanPromo(promoInput.value);
    if (promoInput.value !== c) promoInput.value = c;
    if (c !== promoChecked) { promoReward.hidden = true; promoChecked = ""; }
  });
  $("promoCheck").addEventListener("click", () => {
    const code = cleanPromo(promoInput.value);
    if (code.length < 3) return bad(promoInput, "Entre un code promo (3 à 15 lettres ou chiffres).");
    setBusy(true);
    emitToGame("promocheck", { code });
  });
  $("promoClear").addEventListener("click", () => {
    promoInput.value = ""; promoChecked = ""; promoReward.hidden = true;
  });
  function showPromo(p) {
    if (!p) return;
    if (!p.ok) { promoReward.hidden = true; promoChecked = ""; return; }
    promoChecked = cleanPromo(p.code);
    $("promoRewardTitle").textContent = L(`Code ${promoChecked} · Forfait ${p.pack || ""}`, `Code ${promoChecked} · ${p.pack || ""} pack`, `Código ${promoChecked} · Pacote ${p.pack || ""}`).trim();
    const money = Number(p.money || 0).toLocaleString(L("fr-FR", "en-US", "pt-BR"));
    const veh = p.veh || L("Véhicule", "Vehicle", "Veículo"), days = p.days || 0;
    $("promoRewardText").textContent = L(`${veh} pendant ${days} jours + ${money} $, reçus à ton arrivée en ville.`,
      `${veh} for ${days} days + $${money}, given when you arrive in the city.`,
      `${veh} por ${days} dias + $${money}, entregues quando você chegar na cidade.`);
    promoReward.hidden = false;
  }
  $("registerForm").addEventListener("submit", (e) => {
    e.preventDefault();
    const u = $("registerUsername"), m = $("registerEmail"), c = $("registerConfirmPassword");
    const username = u.value.trim(), email = m.value.trim().toLowerCase(), password = pw.value;
    if (!NAME_RE.test(username)) return bad(u, "Pseudo invalide (3 à 24 lettres, chiffres ou _).");
    if (!EMAIL_RE.test(email) || email.length > 63) return bad(m, "Adresse e-mail invalide.");
    if (!PASS_OK(password)) return bad(pw, "Mot de passe : 4 à 100 caractères, sans espace ni / \\.");
    if (password !== c.value) return bad(c, "Les deux mots de passe ne sont pas identiques.");
    if (!$("acceptTerms").checked) return notify(T("Accepte le règlement pour créer ton compte."), "warning");
    const promo = cleanPromo(promoInput.value);
    if (promo && promo.length < 3) return bad(promoInput, "Code promo trop court.");
    setBusy(true);
    emitToGame("register", { username, email, password, confirmPassword: c.value, promo });
  });

  // ---------------------------------------------------------------- recuperation
  $("requestCodeButton").addEventListener("click", () => {
    const u = $("recoverUsername"), username = u.value.trim();
    if (!NAME_RE.test(username)) return bad(u, "Pseudo invalide.");
    setBusy(true);
    emitToGame("requestRecoveryCode", { username });
  });
  $("recoverForm").addEventListener("submit", (e) => {
    e.preventDefault();
    const u = $("recoverUsername"), k = $("recoverCode"), p = $("recoverNewPassword");
    const username = u.value.trim(), code = k.value.trim(), newPassword = p.value;
    if (!NAME_RE.test(username)) return bad(u, "Pseudo invalide.");
    if (!/^\d{7,18}$/.test(code)) return bad(k, "Le code contient 7 à 18 chiffres.");
    if (!PASS_OK(newPassword)) return bad(p, "Mot de passe : 4 à 100 caractères, sans espace ni / \\.");
    setBusy(true);
    emitToGame("recover", { username, code, newPassword });
  });

  // ---------------------------------------------------------------- identite du personnage
  (() => {
    const MONTHS = ["Janvier", "Février", "Mars", "Avril", "Mai", "Juin", "Juillet", "Août", "Septembre", "Octobre", "Novembre", "Décembre"];
    const ORIGINS = ["Côte d'Ivoire", "Sénégal", "Mali", "Burkina Faso", "Guinée", "Cameroun", "Bénin", "Togo", "Niger", "Gabon", "Congo", "RD Congo",
      "Ghana", "Nigeria", "Maroc", "Algérie", "Tunisie", "France", "Belgique", "Suisse", "Canada", "Haïti", "États-Unis", "Autre"];
    const day = $("idDay"), month = $("idMonth"), year = $("idYear"), origin = $("idOrigin"), other = $("idOther"), otherWrap = $("idOtherWrap");
    let gender = -1;
    const opt = (sel, v, t) => { const o = document.createElement("option"); o.value = v; o.textContent = t; sel.appendChild(o); };
    opt(day, "", "Jour"); for (let d = 1; d <= 31; d++) opt(day, d, d);
    opt(month, "", "Mois"); MONTHS.forEach((m, i) => opt(month, i + 1, m));
    opt(year, "", "Année"); for (let y = 2008; y >= 1940; y--) opt(year, y, y);
    opt(origin, "", "Choisis ton origine"); ORIGINS.forEach((o) => opt(origin, o, o));
    origin.addEventListener("change", () => { otherWrap.hidden = origin.value !== "Autre"; });
    document.querySelectorAll(".cw-gender-btn").forEach((b) => b.addEventListener("click", () => {
      gender = Number(b.dataset.gender);
      document.querySelectorAll(".cw-gender-btn").forEach((x) => { x.classList.toggle("on", x === b); x.setAttribute("aria-checked", String(x === b)); });
    }));
    const ascii = (v) => String(v || "").normalize("NFD").replace(/[̀-ͯ]/g, "").replace(/[^A-Za-z -]/g, "").trim();
    $("identityForm").addEventListener("submit", (e) => {
      e.preventDefault();
      if (gender !== 0 && gender !== 1) return notify("Choisis Homme ou Femme.", "error");
      if (!day.value || !month.value || !year.value) return notify("Indique ta date de naissance.", "error");
      const dim = new Date(Number(year.value), Number(month.value), 0).getDate();
      if (Number(day.value) > dim) return notify("Cette date n'existe pas.", "error");
      const o = origin.value === "Autre" ? ascii(other.value) : ascii(origin.value);
      if (o.length < 2) return notify("Indique ton origine.", "error");
      const birth = String(day.value).padStart(2, "0") + "/" + String(month.value).padStart(2, "0") + "/" + year.value;
      setBusy(true);
      emitToGame("identity", { birth, gender, origin: o.replace(/ /g, "_").slice(0, 22) });
    });
    window.CwIdentity = { open() { showScreen("identity"); } };
  })();

  // ---------------------------------------------------------------- memoire des identifiants
  const STORAGE_USER = "revolt_auth_username";
  const STORAGE_PASS = "revolt_auth_password";
  function setRemembered(username, password) {
    if (!lockedName) $("loginUsername").value = username || "";
    $("loginPassword").value = password || "";
    $("rememberConnexion").checked = !!(username || password);
  }
  function loadRemembered() {
    try {
      const u = localStorage.getItem(STORAGE_USER) || "", p = localStorage.getItem(STORAGE_PASS) || "";
      if (!u && !p) return;
      if (lockedName && u && u.toLowerCase() !== lockedName.toLowerCase()) return;   // autre compte
      setRemembered(u, p);
    } catch (_) {}
  }
  function saveRemembered(u, p) { try { localStorage.setItem(STORAGE_USER, u || ""); localStorage.setItem(STORAGE_PASS, p || ""); } catch (_) {} }
  function clearRemembered() { try { localStorage.removeItem(STORAGE_USER); localStorage.removeItem(STORAGE_PASS); } catch (_) {} }

  // ---------------------------------------------------------------- reponses du serveur
  function handleResponse(raw) {
    let r = raw;
    try { if (typeof raw === "string") r = JSON.parse(raw); } catch (_) { setBusy(false); return; }
    if (!r || typeof r !== "object") return;
    setBusy(false);
    const map = { success: "success", danger: "error", error: "error", warning: "warning", info: "info" };
    const type = map[String(r.type || "info").toLowerCase()] || "info";
    if (r.promo) showPromo(r.promo);
    if (r.message) notify(r.message, type, { title: r.title });
    if (r.screen) showScreen(r.screen);
    const pending = window.__cwPending;
    if (type === "success" && r.close && pending) {
      if (pending.remember) saveRemembered(pending.username, pending.password); else clearRemembered();
      window.__cwPending = null;
    }
    if (type === "success" && r.screen === "login") {
      const n = $("registerUsername").value.trim();
      if (n) $("loginUsername").value = n;
      $("loginPassword").value = "";
      $("loginPassword").focus();
    }
  }

  // ---------------------------------------------------------------- musique (pistes de l'ecran de chargement)
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
  $("musicPrev").addEventListener("click", () => { if (audio.currentTime > 3) { audio.currentTime = 0; return; } loadTrack(trackIndex - 1); play(); });
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
    showConnexion() { showScreen("login"); loadRemembered(); },
    startMusic() { musicStopped = false; if (!audio.src) loadTrack(trackIndex); play(); },
    stopMusic,
    forceCloseFromGame: stopMusic,
    setRememberedConnexion: setRemembered,
    clearRememberedConnexion: clearRemembered,
    openFromNative(raw) {
      let d = raw;
      try { if (typeof raw === "string") d = JSON.parse(raw || "{}"); } catch (_) { d = {}; }
      d = d || {};
      const name = String(d.name || "").trim();
      if (NAME_RE.test(name)) lockName(name);
      setBusy(false);
      if (d.screen === "identity") { showScreen("identity"); return; }
      showScreen(d.screen === "register" || d.screen === "recover" ? d.screen : "login");
      loadRemembered();
      if (d.screen === "register") {
        const nm = name.replace(/_/g, " ");
        notify(L("Bienvenue " + nm + " ! Crée ton compte, et si tu as un code promo, entre-le pour recevoir tes cadeaux.",
          "Welcome " + nm + "! Create your account, and if you have a promo code, enter it to get your gifts.",
          "Bem-vindo " + nm + "! Crie sua conta e, se tiver um código promocional, digite para ganhar seus presentes."),
          "info", { title: L("Nouveau à Crestwood", "New to Crestwood", "Novo em Crestwood"), duration: 8000 });
      }
    }
  };
  window.RevoltAuth = {
    notify,
    openScreen(s) { if (s === "login" || s === "register" || s === "recover") showScreen(s); },
    setRememberedConnexion: setRemembered,
    clearFields() { document.querySelectorAll(".cw-screen input").forEach((i) => { if (i.readOnly) return; if (i.type === "checkbox") i.checked = false; else i.value = ""; }); }
  };

  loadRemembered();
  const ready = () => { try { if (window.SampAuth && typeof window.SampAuth.ready === "function") window.SampAuth.ready(); } catch (_) {} };
  if (document.readyState === "complete") ready(); else window.addEventListener("load", ready);

  // apercu dans un navigateur (sans le jeu)
  if (!window.SampAuth && /[?&]preview=/.test(location.search)) {
    const mode = (location.search.match(/preview=([a-z]+)/) || [])[1];
    window.DXAuth.openFromNative({ screen: mode === "identity" ? "identity" : (mode === "register" ? "register" : "login"), name: "Anzo_Smith" });
    if (mode === "register") showPromo({ ok: 1, code: "ANZO", pack: "Medium", veh: "Sabre", days: 12, money: 500000 });
  }
})();
