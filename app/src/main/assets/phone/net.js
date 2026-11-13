/* ============================================================
   Crestwood RP — téléphone relié au serveur
   ------------------------------------------------------------
   Remplace les données de démonstration de index.html par les vraies :
   numéro, contacts, appels entre joueurs, SMS (Messages + WhatsApp),
   numéros d'urgence (17 / 15 / 18), Uber = taxis en service, Wave.
   Serveur -> page : PhoneNet.onNative(json)   (dialogue "@phone" 9700, SAMP.java)
   Page -> serveur : SampPhone.action(type, json) -> /phoneact ...
   ============================================================ */
(function () {
  "use strict";
  const B = window.SampPhone || null;
  const ST = { num: "", me: "", own: 1, has: 1, unread: 0, cash: 0, bank: 0, contacts: [], convs: [], recents: [], conv: { num: "", msgs: [] }, waveLog: [], taxi: null };
  const SVC_NUM = { "17": ["police", "Police secours"], "15": ["medic", "SAMU · Médecins"], "18": ["mech", "Dépannage · Mécano"] };

  function send(type, o) {
    if (!B || typeof B.action !== "function") { console.log("[phone]", type, o || {}); return; }
    try { B.action(type, JSON.stringify(o || {})); } catch (_) {}
  }
  const digits = (s) => String(s || "").replace(/\D/g, "").slice(0, 9);
  const fmt = (n) => { n = digits(n); return n.length === 9 ? n.replace(/(\d{2})(\d{3})(\d{2})(\d{2})/, "$1 $2 $3 $4") : n; };
  const esc = (s) => String(s == null ? "" : s).replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
  const money = (v) => (Math.max(0, Math.floor(Number(v) || 0))).toLocaleString("fr-FR") + " $";
  function nameOf(num) { const c = ST.contacts.find((x) => x[2] === num); return c ? c[1] : ""; }
  function label(num) { return nameOf(num) || fmt(num); }
  function hhmm(ts) { const d = new Date(Number(ts) * 1000); return String(d.getHours()).padStart(2, "0") + ":" + String(d.getMinutes()).padStart(2, "0"); }
  function when(ts) {
    const d = new Date(Number(ts) * 1000), now = new Date();
    if (d.toDateString() === now.toDateString()) return hhmm(ts);
    const y = new Date(now); y.setDate(now.getDate() - 1);
    if (d.toDateString() === y.toDateString()) return "Hier";
    return d.toLocaleDateString("fr-FR", { day: "2-digit", month: "2-digit" });
  }

  /* ---------------- Verrou : pas d'iPhone = pas de téléphone ---------------- */
  // Le serveur (dialogue @phone, ev "sync") confirme que l'iPhone est dans le sac.
  // Tant qu'il n'a pas répondu la page reste invisible ; sans iPhone (own = 0) ou sans réponse, elle se ferme.
  let phoneConfirmed = false, lockTimer = 0;
  function revealPhone() {
    phoneConfirmed = true;
    clearTimeout(lockTimer);
    document.documentElement.style.visibility = "";
  }
  function closePhone() {
    clearTimeout(lockTimer);
    document.documentElement.style.visibility = "hidden";
    if (B && typeof B.action === "function") { try { B.action("close", "{}"); } catch (_) {} }
  }
  if (B && typeof B.ready === "function") {
    document.documentElement.style.visibility = "hidden";
    lockTimer = setTimeout(() => { if (!phoneConfirmed) closePhone(); }, 4000);
  }

  // WhatsApp utilise directement le numéro du personnage (pas d'inscription factice)
  try { waRegistered = true; } catch (_) {}

  /* ---------------- Applications installées (gardées d'une ouverture à l'autre) ---------------- */
  try {
    const saved = JSON.parse(localStorage.getItem("crestwoodPhoneApps") || "[]");
    if (Array.isArray(saved)) saved.forEach((a) => Set.prototype.add.call(installedApps, a));
  } catch (_) {}
  ["uber", "messages", "wave"].forEach((a) => Set.prototype.add.call(installedApps, a));
  installedApps.add = function (v) {
    Set.prototype.add.call(this, v);
    try { localStorage.setItem("crestwoodPhoneApps", JSON.stringify(Array.from(this))); } catch (_) {}
    return this;
  };
  // Application Wave (transfert d'argent)
  S.wave = '<div style="width:100%;height:100%;display:flex;align-items:center;justify-content:center;background:#1dc4ff"><svg viewBox="0 0 48 48" width="40" height="40"><path fill="#fff" d="M6 20c4-6 8-6 12 0s8 6 12 0 8-6 12 0v6c-4 6-8 6-12 0s-8-6-12 0-8 6-12 0z"/></svg></div>';
  if (!apps.some((a) => a.id === "wave")) apps.splice(5, 0, { id: "wave", bg: "bg-wave", lbl: "Wave", icon: "wave" });

  /* badges (non lus) sur les icônes */
  const _renderHome = renderHome;
  window.renderHome = function () {
    _renderHome();
    if (!ST.unread) return;
    document.querySelectorAll("#homeGrid .app, #dock .app").forEach((d) => {
      const l = d.querySelector(".lbl");
      if (!l || (l.textContent !== "WhatsApp" && l.textContent !== "Messages")) return;
      const ic = d.querySelector(".icon");
      if (!ic || ic.querySelector(".pnBadge")) return;
      const b = document.createElement("span");
      b.className = "pnBadge";
      b.textContent = ST.unread > 99 ? "99+" : String(ST.unread);
      b.style.cssText = "position:absolute;top:-4px;right:-4px;min-width:20px;height:20px;padding:0 5px;border-radius:10px;background:#ff3b30;color:#fff;font-size:11px;font-weight:700;display:flex;align-items:center;justify-content:center;box-shadow:0 0 0 2px rgba(0,0,0,.35);z-index:3";
      ic.style.overflow = "visible";
      ic.appendChild(b);
    });
  };
  renderHome();

  /* ---------------- Bandeau « pas de téléphone » ---------------- */
  // [v20] carte "pas de puce" : illustration de la carte SIM + GPS vers le 24/7 ; "simnew" = puce activee
  const SIM_SVG = '<svg viewBox="0 0 64 80" width="46" height="58" aria-hidden="true"><path d="M8 2h34l20 20v52a4 4 0 0 1-4 4H8a4 4 0 0 1-4-4V6a4 4 0 0 1 4-4Z" fill="#1d2330" stroke="#3b82ff" stroke-width="2.5"/>'
    + '<rect x="16" y="30" width="32" height="34" rx="4" fill="#c9cfd8"/><path d="M16 41h32M16 53h32M32 30v34" stroke="#1d2330" stroke-width="2"/></svg>';
  function simCardBase(id) {
    let el = document.getElementById(id);
    if (el) return el;
    el = document.createElement("div");
    el.id = id;
    el.style.cssText = "position:absolute;left:12px;right:12px;top:50px;z-index:150;padding:14px;border-radius:14px;background:rgba(17,20,25,.96);"
      + "border:1px solid rgba(255,255,255,.1);color:#eef1f5;font-size:13px;line-height:1.4;box-shadow:0 12px 30px rgba(0,0,0,.5);display:flex;gap:12px;align-items:flex-start";
    (document.getElementById("screen") || document.body).appendChild(el);
    return el;
  }
  function renderNoPhone() {
    const el0 = document.getElementById("pnNoPhone");
    if (ST.has) { if (el0) el0.remove(); return; }
    if (el0) return;
    const el = simCardBase("pnNoPhone");
    el.innerHTML = SIM_SVG + "<div style='flex:1;min-width:0'><b style='display:block;font-size:15px;margin-bottom:3px'>Aucune carte SIM</b>"
      + "Achète une puce au 24/7 (500 $) : elle s'active à la caisse et te donne ton numéro."
      + "<div style='display:flex;gap:8px;margin-top:10px'><button type='button' data-sim='gps' style='flex:1;height:34px;border:0;border-radius:8px;background:#3b82ff;color:#fff;font-weight:700'>GPS vers le 24/7</button>"
      + "<button type='button' data-sim='close' style='height:34px;padding:0 12px;border:1px solid rgba(255,255,255,.16);border-radius:8px;background:transparent;color:#d3d8df'>Plus tard</button></div></div>";
    el.addEventListener("click", (e) => {
      const b = e.target.closest("[data-sim]");
      if (!b) return;
      if (b.dataset.sim === "gps") { send("shopgps"); toast("GPS : 24/7 le plus proche"); }
      el.remove();
    });
  }
  function showSimActivated(num) {
    const old = document.getElementById("pnNoPhone");
    if (old) old.remove();
    const el = simCardBase("pnSimNew");
    el.innerHTML = SIM_SVG + "<div style='flex:1;min-width:0'><b style='display:block;font-size:15px;margin-bottom:3px'>Puce activée</b>Ton numéro :"
      + "<div style='font-size:22px;font-weight:800;letter-spacing:.5px;color:#fff;margin-top:2px'>" + esc(fmt(num)) + "</div>"
      + "<small style='color:#8c94a0'>Donne-le à tes contacts pour qu'ils t'appellent.</small></div>";
    el.onclick = () => el.remove();
    window.setTimeout(() => { try { el.remove(); } catch (_) {} }, 9000);
  }

  /* ---------------- Téléphone : contacts, récents, appels ---------------- */
  function syncPhoneData() {
    phoneContacts.length = 0;
    ST.contacts.forEach((c) => phoneContacts.push({ id: c[0], name: c[1], number: fmt(c[2]), fav: true }));
    phoneRecents = ST.recents.map((r) => ({ name: label(r[0]), number: fmt(r[0]), type: r[1] === 0 ? "out" : (r[1] === 1 ? "in" : "missed"), time: when(r[2]) }));
  }

  const _phoneTabBodyHTML = phoneTabBodyHTML;
  window.phoneTabBodyHTML = function () {
    let h = _phoneTabBodyHTML();
    if ((phoneTab === "contacts" || phoneTab === "favoris") && ST.num) {
      h = `<div style="display:flex;align-items:center;gap:10px;background:#1c1c1e;border-radius:14px;padding:10px 12px;margin:6px 0 4px">
        <div style="font-size:20px">📇</div><div style="flex:1"><div style="font-size:12px;color:#8e8e93">Mon numéro</div><div style="font-weight:700;font-size:16px;letter-spacing:.5px">${fmt(ST.num)}</div></div></div>` + h;
    }
    if (phoneTab === "clavier") {
      h += `<div style="text-align:center;color:#8e8e93;font-size:12px;margin-top:14px">Urgences : <b style="color:#fff">17</b> Police · <b style="color:#fff">15</b> SAMU · <b style="color:#fff">18</b> Dépannage</div>`;
    }
    return h;
  };

  window.phoneAddContact = function () {
    const name = (document.getElementById("phoneNewName").value || "").trim();
    const number = digits(document.getElementById("phoneNewNumber").value);
    if (!name || number.length < 2) { toast("Nom et numéro requis"); return; }
    send("addc", { num: number, name: name });
    toast("Contact enregistré");
  };

  function emergencyPrompt(num) {
    const s = SVC_NUM[num];
    let el = document.getElementById("pnSos");
    if (el) el.remove();
    el = document.createElement("div");
    el.id = "pnSos";
    el.style.cssText = "position:absolute;inset:0;z-index:210;display:flex;align-items:flex-end;background:rgba(0,0,0,.55)";
    el.innerHTML = `<div style="width:100%;background:#1c1c1e;border-radius:22px 22px 0 0;padding:18px 16px 26px;color:#fff">
      <div style="font-size:12px;color:#ff453a;font-weight:700;letter-spacing:.5px">APPEL D'URGENCE · ${esc(num)}</div>
      <div style="font-size:20px;font-weight:800;margin:4px 0 12px">${esc(s[1])}</div>
      <input id="pnSosTxt" maxlength="60" placeholder="Décris la situation (accident, blessé, panne…)" style="width:100%;background:#2c2c2e;border:none;color:#fff;padding:12px;border-radius:12px;font-size:14px;outline:none;box-sizing:border-box">
      <div style="display:flex;gap:10px;margin-top:12px">
        <button class="btn" style="flex:1;background:#3a3a3c" id="pnSosNo">Annuler</button>
        <button class="btn" style="flex:1;background:#ff3b30" id="pnSosGo">Appeler</button>
      </div>
      <div style="color:#8e8e93;font-size:11.5px;margin-top:10px">Ta position est envoyée aux agents en service.</div></div>`;
    (document.getElementById("screen") || document.body).appendChild(el);
    el.querySelector("#pnSosNo").onclick = () => el.remove();
    el.querySelector("#pnSosGo").onclick = () => {
      const t = el.querySelector("#pnSosTxt").value.trim();
      if (t.length < 3) { toast("Décris la situation"); return; }
      send("sos", { svc: s[0], text: t });
      el.remove();
    };
    setTimeout(() => { const i = el.querySelector("#pnSosTxt"); if (i) i.focus(); }, 60);
  }

  function startTimer() {
    clearInterval(phoneCallTimerInt);
    phoneCallTimerInt = setInterval(() => {
      if (!phoneCallActive || phoneCallActive.status !== "connected") return;
      phoneCallActive.seconds++;
      const d = document.getElementById("callTimer");
      if (d) { const s = phoneCallActive.seconds; d.textContent = String(Math.floor(s / 3600)).padStart(2, "0") + ":" + String(Math.floor(s / 60) % 60).padStart(2, "0") + ":" + String(s % 60).padStart(2, "0"); }
    }, 1000);
  }

  window.phoneStartCall = function (name, number) {
    const n = digits(number);
    if (!n) return;
    if (!ST.has) { toast("Pas de carte SIM"); return; }
    if (SVC_NUM[n]) { phoneDialed = ""; phoneRender(); emergencyPrompt(n); return; }
    if (n === ST.num) { toast("C'est ton propre numéro"); return; }
    phoneCallActive = { name: label(n), number: fmt(n), raw: n, seconds: 0, status: "ringing" };
    phoneDialed = "";
    if (!document.getElementById("scr-call")) openApp("call");
    phoneRender();
    send("call", { num: n });
  };
  window.phoneCallDialed = function () { if (phoneDialed) window.phoneStartCall("", phoneDialed); };
  window.phoneEndCall = function () {
    send("hangup");
    phoneCallActive = null;
    clearInterval(phoneCallTimerInt);
    phoneRender();
  };
  window.phoneAcceptIncoming = function () {
    const p = phonePendingIncoming;
    document.getElementById("incomingCallOverlay").style.display = "none";
    phonePendingIncoming = null;
    if (!p) return;
    phoneCallActive = { name: p.name, number: p.number, raw: digits(p.number), seconds: 0, status: "connecting", incoming: true };
    send("accept");
    openApp("call");
    phoneRender();
  };
  window.phoneDeclineIncoming = function () {
    document.getElementById("incomingCallOverlay").style.display = "none";
    phonePendingIncoming = null;
    send("decline");
  };
  window.phoneCallScreenHTML = function () {
    const c = phoneCallActive;
    const connected = c.status === "connected";
    const st = c.status === "ringing" ? "Appel en cours…" : (c.status === "connecting" ? "Connexion…" : "");
    return `<div class="callBg" style="color:#fff;height:100%;display:flex;flex-direction:column">
      <div style="text-align:center;padding:56px 0 18px">
        <div class="callAvatarWrap">
          ${connected ? "" : `<div class="ringWave"></div><div class="ringWave w2"></div><div class="ringWave w3"></div>`}
          <div class="callAvatar" style="background:linear-gradient(150deg,${phoneAvatarColor(c.name)},#1a1a1e)">${phoneInitial(c.name)}</div>
        </div>
        <div class="callName">${escapeHtml(c.name)}</div>
        <div class="callSub">${esc(c.number)}</div>
        ${connected ? `<div class="callStatus" id="callTimer">00:00:00</div>` : `<div class="callStatus ringing">${st}</div>`}
      </div>
      ${connected ? `<div style="margin:0 26px;padding:10px 12px;border-radius:14px;background:rgba(255,255,255,.08);font-size:12.5px;line-height:1.4;text-align:center;color:#d1d1d6">💬 Écris dans le chat du jeu : ton correspondant reçoit tes messages.<br><span style="color:#8e8e93">Les joueurs proches t'entendent parler.</span></div>` : ""}
      <div style="margin-top:auto;margin-bottom:46px;display:flex;justify-content:center">
        <div style="text-align:center"><div class="callActionBtn hangupBtn" onclick="phoneEndCall()">📵</div><div class="callActionLbl">Raccrocher</div></div>
      </div>
    </div>`;
  };

  /* ---------------- Messages & WhatsApp ---------------- */
  const COLORS = ["#4f5bd5", "#f59d0c", "#3f7c5f", "#e34f9a", "#1f70d0", "#c04a2a", "#2b8a3e", "#8e44ad"];
  function colorOf(s) { let h = 0; for (const c of String(s)) h += c.charCodeAt(0); return COLORS[h % COLORS.length]; }
  function convEntry(num, last, me) {
    const n = label(num);
    return { id: num, letter: (n.replace(/[^A-Za-zÀ-ÿ0-9]/g, "")[0] || "#").toUpperCase(), name: n, color: colorOf(num), msgs: [[me ? "m" : "t", last || ""]] };
  }
  function syncChats() {
    ["whatsapp", "messages"].forEach((app) => {
      const list = CHAT[app].contacts;
      const open = (curChat && curChat.appId === app) ? list.find((c) => c.id === curChat.id) : null;
      list.length = 0;
      ST.convs.forEach((c) => { const e = convEntry(c[0], c[1], c[3]); e.unread = c[4]; list.push(e); WA_TIMES[c[0]] = when(c[2]); });
      if (open && !list.some((c) => c.id === open.id)) list.unshift(open);
      CHAT[app].replies = [];
    });
    renderChatLists();
  }
  function listRowHTML(app, c) {
    const last = c.msgs.length ? c.msgs[c.msgs.length - 1][1] : "";
    const unread = Number(c.unread) || 0;
    return `<div onclick="openChat('${app}','${c.id}')" style="display:flex;gap:12px;align-items:center;padding:10px 8px;border-radius:12px;cursor:pointer">
      ${avatar(esc(c.letter), c.color)}
      <div style="flex:1;min-width:0">
        <div style="display:flex;justify-content:space-between;gap:8px"><span style="font-weight:700;color:#fff">${esc(c.name)}</span><span style="color:${unread ? "#25D366" : "#8e8e93"};font-size:12px;flex:none">${WA_TIMES[c.id] || ""}</span></div>
        <div style="display:flex;gap:8px;align-items:center"><div style="flex:1;color:#8e8e93;font-size:13px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis">${esc(last)}</div>${unread ? `<span style="min-width:20px;height:20px;border-radius:10px;background:#25D366;color:#000;font-size:11px;font-weight:800;display:flex;align-items:center;justify-content:center;padding:0 6px">${unread}</span>` : ""}</div>
      </div>
    </div>`;
  }
  function newChatBtn(app) {
    return `<div onclick="PhoneNet.compose('${app}')" style="position:absolute;right:16px;bottom:${app === "whatsapp" ? 76 : 18}px;width:52px;height:52px;border-radius:16px;background:${app === "whatsapp" ? "#00a884" : "#34c759"};display:flex;align-items:center;justify-content:center;font-size:24px;color:#fff;box-shadow:0 8px 20px rgba(0,0,0,.45);cursor:pointer;z-index:5">✎</div>`;
  }
  window.waChatsPaneHTML = function () {
    const list = CHAT.whatsapp.contacts;
    return (list.length ? list.map((c) => listRowHTML("whatsapp", c)).join("") : `<div style="text-align:center;color:#8e8e93;font-size:13px;margin-top:40px">Aucune discussion.<br>Appuie sur ✎ pour écrire à un contact.</div>`);
  };
  window.waStatusPaneHTML = function () {
    return `<div style="text-align:center;color:#8e8e93;font-size:13px;margin-top:40px">Les statuts arrivent bientôt.</div>`;
  };
  function renderChatLists() {
    const wl = document.getElementById("whatsapp-list");
    if (wl) {
      if (typeof waTabCur !== "undefined" && document.getElementById("waPane")) waTab(waTabCur);
      if (!wl.querySelector(".pnNew")) { const d = document.createElement("div"); d.className = "pnNew"; d.innerHTML = newChatBtn("whatsapp"); wl.style.position = "relative"; wl.appendChild(d); }
    }
    const ml = document.getElementById("messages-list");
    if (ml) {
      const list = CHAT.messages.contacts;
      ml.style.position = "relative";
      ml.innerHTML = (list.length ? list.map((c) => listRowHTML("messages", c)).join("") : `<div style="text-align:center;color:#8e8e93;font-size:13px;margin-top:40px">Aucun message.<br>Appuie sur ✎ pour écrire.</div>`) + newChatBtn("messages");
    }
    window.renderHome();
    updateLock();
  }
  const _openApp = openApp;
  window.openApp = function (id) {
    if (id === "wave") { openWave(); return; }
    _openApp(id);
    if (id === "whatsapp" || id === "messages") setTimeout(renderChatLists, 30);
    if (id === "call") phoneRender();
  };
  const _openChat = openChat;
  window.openChat = function (appId, id) {
    const list = CHAT[appId].contacts;
    if (!list.some((c) => c.id === id)) list.unshift(convEntry(id, "", false));
    _openChat(appId, id);
    const box = document.getElementById(appId + "-msgs");
    if (box) box.innerHTML = `<div style="margin:auto;color:#8e8e93;font-size:12px">Chargement…</div>`;
    const sub = document.querySelector("#whatsapp-chat [style*='#25D366']");
    if (appId === "whatsapp" && sub) sub.textContent = fmt(id);
    ST.conv = { num: id, msgs: [] };
    send("conv", { num: id });
  };
  window.sendChat = function (appId) {
    const inp = document.getElementById(appId + "-in");
    const txt = (inp.value || "").trim();
    if (!txt || !curChat) return;
    if (!ST.has) { toast("Pas de carte SIM"); return; }
    inp.value = "";
    addMsg(appId, false, txt, hhmm(Date.now() / 1000));
    send("sms", { num: curChat.id, text: txt });
  };
  window.waSendLocation = function () { toast("Partage de position : bientôt"); };
  function renderConv() {
    if (!curChat || curChat.id !== ST.conv.num) return;
    const box = document.getElementById(curChat.appId + "-msgs");
    if (!box) return;
    box.innerHTML = "";
    if (!ST.conv.msgs.length) box.innerHTML = `<div style="margin:auto;color:#8e8e93;font-size:12px;text-align:center">Aucun message.<br>Dis bonjour 👋</div>`;
    ST.conv.msgs.forEach((m) => addMsg(curChat.appId, !m[0], m[1], hhmm(m[2])));
  }
  window.PhoneNet = window.PhoneNet || {};
  window.PhoneNet.compose = function (app) {
    let el = document.getElementById("pnCompose");
    if (el) el.remove();
    el = document.createElement("div");
    el.id = "pnCompose";
    el.style.cssText = "position:absolute;inset:0;z-index:205;display:flex;align-items:flex-end;background:rgba(0,0,0,.55)";
    el.innerHTML = `<div style="width:100%;max-height:75%;overflow:auto;background:#1c1c1e;border-radius:22px 22px 0 0;padding:16px 14px 24px;color:#fff">
      <div style="font-size:18px;font-weight:800;margin-bottom:10px">Nouveau message</div>
      <div style="display:flex;gap:8px"><input id="pnCompNum" type="tel" placeholder="Numéro (9 chiffres)" style="flex:1;background:#2c2c2e;border:none;color:#fff;padding:12px;border-radius:12px;font-size:14px;outline:none;min-width:0">
      <button class="btn" style="width:auto;padding:0 16px;background:#0a84ff" id="pnCompGo">OK</button></div>
      <div style="color:#8e8e93;font-size:12px;margin:12px 0 6px">CONTACTS</div>
      ${ST.contacts.length ? ST.contacts.map((c) => `<div data-n="${esc(c[2])}" style="display:flex;align-items:center;gap:10px;padding:8px 4px;cursor:pointer">${avatar(esc((c[1][0] || "#").toUpperCase()), colorOf(c[2]))}<div><div style="font-weight:700">${esc(c[1])}</div><div style="color:#8e8e93;font-size:12px">${fmt(c[2])}</div></div></div>`).join("") : `<div style="color:#8e8e93;font-size:13px">Aucun contact enregistré.</div>`}
      <button class="btn" style="margin-top:12px;background:#3a3a3c" id="pnCompNo">Annuler</button></div>`;
    (document.getElementById("screen") || document.body).appendChild(el);
    const go = (n) => { n = digits(n); if (n.length < 2) { toast("Numéro invalide"); return; } el.remove(); window.openChat(app, n); };
    el.querySelector("#pnCompGo").onclick = () => go(el.querySelector("#pnCompNum").value);
    el.querySelector("#pnCompNo").onclick = () => el.remove();
    el.querySelectorAll("[data-n]").forEach((r) => (r.onclick = () => go(r.dataset.n)));
  };

  /* ---------------- Écran verrouillé : vrais messages non lus ---------------- */
  function updateLock() {
    LOCK_NOTIFS.length = 0;
    ST.convs.filter((c) => Number(c[4]) > 0 && !c[3]).slice(0, 3).forEach((c) => {
      LOCK_NOTIFS.push({ app: "WhatsApp", img: "WhatsIcon.png", color: "#25D366", title: label(c[0]), msg: c[1], time: when(c[2]) });
    });
    renderLockNotifs();
  }

  /* ---------------- Uber = taxis en service ---------------- */
  window.uberRequest = function () {
    const d = document.getElementById("uberDest");
    uberDest = ((d && d.value) || uberDest || "").trim() || "Ma position";
    const ride = UBER_RIDES.find((r) => r.id === uberSel) || UBER_RIDES[0];
    uberTrip = { ride, dest: uberDest, driver: "…", car: "Taxi Crestwood", plate: "", min: "—" };
    uberStop(); uberState = "searching"; uberRender();
    send("taxi", { text: ride.name + " vers " + uberDest });
  };
  const _uberCancel = uberCancel;
  window.uberCancel = function () { send("soscancel"); _uberCancel(); };
  window.uberStart = function () { uberState = "trip"; uberRender(); };

  /* ---------------- Wave ---------------- */
  function openWave() {
    document.querySelectorAll(".view").forEach((v) => v.classList.remove("active"));
    document.getElementById("lock").classList.remove("active");
    document.getElementById("home").classList.remove("active");
    let el = document.getElementById("scr-wave");
    if (!el) {
      el = document.createElement("div");
      el.id = "scr-wave";
      el.className = "view dark";
      screens.appendChild(el);
    }
    renderWave();
    setTimeout(() => el.classList.add("active"), 10);
  }
  function renderWave() {
    const el = document.getElementById("scr-wave");
    if (!el) return;
    const keep = el.querySelector("#wvNum") ? { n: el.querySelector("#wvNum").value, a: el.querySelector("#wvAmt").value } : { n: "", a: "" };
    el.innerHTML = `<div class="atop" style="color:#fff;justify-content:center"><span style="font-weight:600;font-size:16px">Wave</span></div>
      <div class="content" style="padding:12px 16px;color:#fff">
        <div style="border-radius:20px;padding:18px;background:linear-gradient(135deg,#1dc4ff,#0a6cff);box-shadow:0 12px 26px rgba(10,108,255,.35)">
          <div style="font-size:12px;opacity:.85">Solde (compte bancaire)</div>
          <div style="font-size:28px;font-weight:800;margin:4px 0">${money(ST.bank)}</div>
          <div style="font-size:12px;opacity:.85">${ST.num ? "Mon numéro : " + fmt(ST.num) : ""}</div>
        </div>
        <div style="font-size:16px;font-weight:800;margin:18px 0 8px">Envoyer de l'argent</div>
        <input id="wvNum" type="tel" placeholder="Numéro du destinataire" value="${esc(keep.n)}" style="width:100%;background:#1c1c1e;border:none;color:#fff;padding:12px;border-radius:12px;font-size:14px;outline:none;box-sizing:border-box;margin-bottom:8px">
        <div style="display:flex;gap:6px;flex-wrap:wrap;margin-bottom:8px">${ST.contacts.slice(0, 8).map((c) => `<span data-n="${esc(c[2])}" style="background:#1c1c1e;border-radius:999px;padding:6px 12px;font-size:12px;cursor:pointer">${esc(c[1])}</span>`).join("")}</div>
        <input id="wvAmt" type="number" inputmode="numeric" placeholder="Montant ($)" value="${esc(keep.a)}" style="width:100%;background:#1c1c1e;border:none;color:#fff;padding:12px;border-radius:12px;font-size:14px;outline:none;box-sizing:border-box">
        <button class="btn" id="wvGo" style="margin-top:12px;background:#1dc4ff;color:#002a3a;font-weight:800">Envoyer</button>
        <div style="color:#8e8e93;font-size:12px;margin-top:8px">Sans frais · max 500 000 $ par envoi</div>
        <div style="font-size:16px;font-weight:800;margin:18px 0 8px">Activité</div>
        ${ST.waveLog.length ? ST.waveLog.map((w) => `<div style="display:flex;align-items:center;gap:10px;padding:10px 0;border-bottom:1px solid #222"><div style="width:36px;height:36px;border-radius:50%;background:${w.in ? "#0f3d22" : "#3a1c1c"};display:flex;align-items:center;justify-content:center">${w.in ? "↓" : "↑"}</div><div style="flex:1"><div style="font-weight:700">${esc(w.who)}</div><div style="color:#8e8e93;font-size:12px">${hhmm(w.ts)}</div></div><div style="font-weight:800;color:${w.in ? "#34c759" : "#ff453a"}">${w.in ? "+" : "−"}${money(w.amount)}</div></div>`).join("") : `<div style="color:#8e8e93;font-size:13px">Aucune opération depuis l'ouverture du téléphone.</div>`}
      </div>`;
    el.querySelectorAll("[data-n]").forEach((s) => (s.onclick = () => { el.querySelector("#wvNum").value = fmt(s.dataset.n); }));
    el.querySelector("#wvGo").onclick = () => {
      const n = digits(el.querySelector("#wvNum").value), a = Math.floor(Number(el.querySelector("#wvAmt").value) || 0);
      if (!ST.has) { toast("Pas de carte SIM"); return; }
      if (n.length < 2) { toast("Numéro invalide"); return; }
      if (a < 1) { toast("Montant invalide"); return; }
      if (a > ST.bank) { toast("Solde insuffisant"); return; }
      send("wave", { num: n, amount: a });
    };
  }

  /* ---------------- Événements du serveur ---------------- */
  function onNative(raw) {
    let d;
    try { d = typeof raw === "string" ? JSON.parse(raw) : raw; } catch (_) { return; }
    if (!d || !d.ev) return;
    switch (d.ev) {
      case "sync":
        ST.own = (d.own === undefined || Number(d.own)) ? 1 : 0;
        if (!ST.own) { closePhone(); return; }
        revealPhone();
        ST.num = d.num || ""; ST.me = d.me || ""; ST.has = Number(d.has) ? 1 : 0; ST.unread = Number(d.unread) || 0;
        ST.cash = Number(d.cash) || 0; ST.bank = Number(d.bank) || 0;
        ST.contacts = Array.isArray(d.contacts) ? d.contacts : [];
        syncPhoneData();
        renderNoPhone();
        if (document.getElementById("scr-call")) phoneRender();
        renderWave();
        if (d.call && Number(d.call.st)) restoreCall(d.call);
        break;
      case "simnew":
        ST.has = 1; ST.num = d.num || ST.num;
        showSimActivated(ST.num);
        send("sync");
        break;
      case "convs":
        ST.convs = Array.isArray(d.list) ? d.list : [];
        ST.unread = ST.convs.reduce((s, c) => s + (c[3] ? 0 : Number(c[4]) || 0), 0);
        syncChats();
        break;
      case "recents":
        ST.recents = Array.isArray(d.list) ? d.list : [];
        syncPhoneData();
        if (document.getElementById("scr-call") && !phoneCallActive) phoneRender();
        break;
      case "conv":
        ST.conv = { num: d.num || "", msgs: Array.isArray(d.msgs) ? d.msgs : [] };
        renderConv();
        send("convs");
        break;
      case "sent":
        if (!(curChat && curChat.id === d.to)) toast("Message envoyé");
        setTimeout(() => send("convs"), 500);
        break;
      case "sms":
        if (curChat && curChat.id === d.from && document.getElementById(curChat.appId + "-chat") && document.getElementById(curChat.appId + "-chat").style.display !== "none") {
          send("conv", { num: d.from });
        } else {
          toast("💬 " + (d.name || fmt(d.from)) + " : " + d.text);
          send("convs");
        }
        break;
      case "call": onCall(d); break;
      case "sos": onSos(d); break;
      case "wave":
        ST.bank = Number(d.bank) || ST.bank;
        ST.waveLog.unshift({ in: d.st === "in", who: d.name || fmt(d.num), amount: Number(d.amount) || 0, ts: Date.now() / 1000 });
        toast(d.st === "in" ? "Wave reçu : " + money(d.amount) : "Envoyé : " + money(d.amount));
        renderWave();
        if (d.st === "ok") { const el = document.getElementById("scr-wave"); if (el) { const a = el.querySelector("#wvAmt"); if (a) a.value = ""; } }
        break;
      case "err":
        toast(d.msg || "Erreur");
        if (phoneCallActive && phoneCallActive.status !== "connected") { phoneCallActive = null; phoneRender(); }
        if (typeof uberState !== "undefined" && uberState === "searching") { uberState = "idle"; uberRender(); }
        break;
    }
  }

  function restoreCall(c) {
    const st = Number(c.st), num = c.num || "", name = c.name || label(num);
    if (st === 2) { phoneShowIncoming(name, fmt(num)); return; }
    phoneCallActive = { name, number: fmt(num), raw: num, seconds: st === 3 ? Math.max(0, Math.floor(Date.now() / 1000 - Number(c.since || 0))) : 0, status: st === 3 ? "connected" : "ringing" };
    openApp("call");
    phoneRender();
    if (st === 3) startTimer();
  }

  function onCall(d) {
    const num = d.num || "", name = d.name || label(num);
    if (d.st === "incoming") { phoneShowIncoming(name, fmt(num)); return; }
    if (d.st === "ringing") {
      if (!phoneCallActive) phoneCallActive = { name, number: fmt(num), raw: num, seconds: 0, status: "ringing" };
      phoneCallActive.status = "ringing";
      phoneRender();
      return;
    }
    if (d.st === "incall") {
      document.getElementById("incomingCallOverlay").style.display = "none";
      phonePendingIncoming = null;
      phoneCallActive = { name, number: fmt(num), raw: num, seconds: 0, status: "connected" };
      if (!document.getElementById("scr-call") || !document.getElementById("scr-call").classList.contains("active")) openApp("call");
      phoneRender();
      startTimer();
      return;
    }
    if (d.st === "ended") {
      document.getElementById("incomingCallOverlay").style.display = "none";
      phonePendingIncoming = null;
      phoneCallActive = null;
      clearInterval(phoneCallTimerInt);
      phoneRender();
      toast(d.why || "Appel terminé");
      send("recents");
    }
  }

  function onSos(d) {
    const names = { police: "la police", medic: "le SAMU", mech: "un dépanneur", taxi: "un taxi" };
    if (d.svc === "taxi" && typeof uberState !== "undefined" && uberState !== "idle") {
      if (d.st === "sent" && !Number(d.n)) toast("Aucun chauffeur en service : ta demande reste en attente");
      if (d.st === "taken") {
        uberTrip = Object.assign(uberTrip || {}, { driver: d.name || "Chauffeur", car: "Taxi Crestwood", plate: "", min: "—" });
        uberState = "enroute"; uberRender(); toast(d.name + " arrive !");
      }
      if (d.st === "done") { uberState = "done"; uberRender(); }
      return;
    }
    if (d.st === "sent") toast(Number(d.n) ? "Appel transmis à " + (names[d.svc] || "la centrale") + " (" + d.n + " en service)" : "Appel enregistré : personne en service pour le moment");
    if (d.st === "taken") toast((d.name || "Un agent") + " a pris ton appel et arrive");
    if (d.st === "done") toast("Intervention clôturée");
  }

  window.PhoneNet.onNative = onNative;

  /* ---------------- Démarrage ---------------- */
  if (B && typeof B.ready === "function") {
    try { B.ready(); } catch (_) {}
  } else {
    // Aperçu navigateur : fausses données
    const now = Math.floor(Date.now() / 1000);
    onNative({ ev: "sync", num: "771234567", me: "Awa Diallo", has: 1, unread: 2, cash: 12500, bank: 84000, call: { st: 0 }, contacts: [[1, "Moussa", "781112233"], [2, "Fatou", "765554433"], [3, "Garage Kone", "709998877"]] });
    onNative({ ev: "convs", list: [["781112233", "On se retrouve au marché ?", now - 120, 0, 2], ["765554433", "Merci pour hier", now - 7200, 1, 0]] });
    onNative({ ev: "recents", list: [["781112233", 2, now - 600], ["709998877", 0, now - 90000]] });
  }
})();
