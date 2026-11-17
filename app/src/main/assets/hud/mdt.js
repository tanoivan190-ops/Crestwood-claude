/* =========================================================
   CRESTWOOD v25 - MDT "ordinateur" de la police (Crestwood Police Department)
   Fenetre facon macOS dans la WebView du HUD (aucune page en plus).
   Serveur -> HUD : dialogue 9860 "@hud" -> CwBus({m:"mdt", v:"home"|"person"|"plate", ...}) -> CwMdt.onData
   HUD -> serveur : /c23 mdt <home|find|plate|say|cam|camoff|radar|fine|want|gps> ...   (Cw23.inc)
   ========================================================= */
(() => {
  "use strict";
  const esc = (s) => String(s == null ? "" : s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
  const nm = (s) => String(s || "").replace(/_/g, " ");
  const money = (n) => (Number(n) || 0).toLocaleString("fr-FR") + " $";
  const send = (cmd) => { try { (window.SampHud || window.AndroidHUD)?.sendChatMessage?.("/c23 mdt " + cmd); } catch (_) {} };
  const sync = () => { try { window.crestwoodSyncTouchBounds && window.crestwoodSyncTouchBounds(); } catch (_) {} };
  const initials = (s) => nm(s).split(" ").filter(Boolean).slice(0, 2).map((w) => w[0].toUpperCase()).join("") || "?";
  const LOGO = "img/cw_police.webp";

  const I = {
    dash: '<path d="M4 13h7V4H4zM13 20h7v-9h-7zM13 4v5h7V4zM4 20h7v-5H4z"/>',
    find: '<circle cx="10.5" cy="10.5" r="6"/><path d="m15 15 5 5"/>',
    plate: '<rect x="2.5" y="7" width="19" height="10" rx="2"/><path d="M6.5 12h3M12 12h5.5"/>',
    wanted: '<path d="M12 3.5l2.6 5.3 5.9.9-4.3 4.1 1 5.8L12 16.9l-5.2 2.7 1-5.8-4.3-4.1 5.9-.9z"/>',
    radio: '<path d="M4 5h16v11H9l-5 4z"/><path d="M8 9.5h8M8 12.5h5"/>',
    cams: '<rect x="2.5" y="7" width="13" height="10" rx="2"/><path d="m15.5 11 6-3.5v9l-6-3.5"/>',
    radar: '<path d="M12 12 19 5"/><path d="M19.5 12A7.5 7.5 0 1 1 12 4.5"/><path d="M15.5 12a3.5 3.5 0 1 1-3.5-3.5"/>',
    gps: '<path d="M12 21s7-6.6 7-11.5a7 7 0 1 0-14 0C5 14.4 12 21 12 21Z"/><circle cx="12" cy="9.5" r="2.5"/>',
    fine: '<path d="M6 3h12v18l-3-2-3 2-3-2-3 2z"/><path d="M9 8h6M9 12h6"/>',
    star: '<path d="M12 3.5l2.6 5.3 5.9.9-4.3 4.1 1 5.8L12 16.9l-5.2 2.7 1-5.8-4.3-4.1 5.9-.9z"/>',
    car: '<path d="M4 16v-4l2-5h12l2 5v4M3 16h18v3H3zM6.5 19v1.5M17.5 19v1.5"/><circle cx="7.5" cy="13.5" r=".8"/><circle cx="16.5" cy="13.5" r=".8"/>',
    user: '<circle cx="12" cy="8" r="4"/><path d="M4 21c1.2-4.2 4.3-6 8-6s6.8 1.8 8 6"/>',
    refresh: '<path d="M20 11a8 8 0 1 0-2.3 5.7M20 4v7h-7"/>',
    send: '<path d="m4 12 16-8-6 16-2.5-6.5z"/>'
  };
  const svg = (k, c) => `<svg viewBox="0 0 24 24" class="${c || ""}" aria-hidden="true">${I[k] || ""}</svg>`;

  // ---------- fenetre ----------
  const root = document.createElement("section");
  root.id = "cwMdt";
  root.className = "mdt";
  root.hidden = true;
  root.innerHTML = `
    <header class="mdt-bar">
      <span class="mdt-lights"><button type="button" class="l-r" aria-label="Fermer"></button><button type="button" class="l-y" aria-label="Réduire"></button><button type="button" class="l-g" aria-label="Actualiser"></button></span>
      <div class="mdt-title"><img src="${LOGO}" alt=""><b>Crestwood Police Department</b><span>MDT</span></div>
      <time class="mdt-clock" id="mdtClock"></time>
    </header>
    <div class="mdt-main">
      <nav class="mdt-side">
        <div class="mdt-brand"><img src="${LOGO}" alt="Crestwood Police Department"><div><b>CPD</b><small>Mobile Data Terminal</small></div></div>
        <div class="mdt-nav" id="mdtNav"></div>
        <div class="mdt-me" id="mdtMe"></div>
      </nav>
      <div class="mdt-view">
        <div class="mdt-head" id="mdtHead"></div>
        <div class="mdt-body" id="mdtBody"></div>
      </div>
    </div>
    <div class="mdt-login" id="mdtLogin">
      <img class="mdt-login-logo" src="${LOGO}" alt="Crestwood Police Department">
      <h1>Crestwood Police Department</h1>
      <p>Mobile Data Terminal · accès réservé aux forces de l'ordre</p>
      <div class="mdt-login-card">
        <span class="mdt-avatar" id="mdtLoginAv">?</span>
        <div><b id="mdtLoginName">Agent</b><small id="mdtLoginRank">—</small></div>
      </div>
      <button type="button" class="mdt-login-go" id="mdtLoginGo">Se connecter</button>
      <div class="mdt-login-bar"><i></i></div>
    </div>`;
  document.body.appendChild(root);
  const $ = (id) => root.querySelector("#" + id);
  const $nav = $("mdtNav"), $head = $("mdtHead"), $body = $("mdtBody"), $me = $("mdtMe"), $login = $("mdtLogin");

  let d = {}, view = "dash", person = null, plate = null, camOn = 0, logged = false, clockTimer = 0;

  const VIEWS = [
    ["dash", "Tableau de bord"], ["find", "Citoyens"], ["plate", "Véhicules"], ["wanted", "Recherchés"],
    ["radio", "Radio"], ["cams", "Caméras"], ["radar", "Radar"]
  ];

  function tick() {
    const t = new Date();
    $("mdtClock").textContent = t.toLocaleDateString("fr-FR", { weekday: "short", day: "numeric", month: "short" }) + "  " + t.toLocaleTimeString("fr-FR", { hour: "2-digit", minute: "2-digit" });
  }
  function show() {
    if (root.hidden) { root.hidden = false; requestAnimationFrame(() => root.classList.add("show")); }
    tick(); clearInterval(clockTimer); clockTimer = setInterval(tick, 20000);
    setTimeout(sync, 40);
  }
  function hide() {
    if (camOn) send("camoff");
    camOn = 0;
    clearInterval(clockTimer);
    root.classList.remove("show");
    setTimeout(() => { root.hidden = true; sync(); }, 200);
    try { window.CwKb?.close?.(); } catch (_) {}
  }
  root.querySelector(".l-r").addEventListener("click", hide);
  root.querySelector(".l-y").addEventListener("click", hide);
  root.querySelector(".l-g").addEventListener("click", () => send("home"));
  root.addEventListener("pointerdown", (e) => e.stopPropagation());

  $("mdtLoginGo").addEventListener("click", () => {
    if (root.classList.contains("logging")) return;
    root.classList.add("logging");
    setTimeout(() => { logged = true; root.classList.remove("logging"); $login.classList.add("off"); render(); }, 750);
  });

  // ---------- constructeurs ----------
  function h(tag, cls, html) { const e = document.createElement(tag); if (cls) e.className = cls; if (html != null) e.innerHTML = html; return e; }
  function btn(label, fn, cls, icon) {
    const b = h("button", "mdt-btn" + (cls ? " " + cls : ""), (icon ? svg(icon) : "") + "<span>" + esc(label) + "</span>");
    b.type = "button"; b.addEventListener("click", fn); return b;
  }
  function field(ph, max, onDone, cls) {
    const i = h("input", "mdt-in" + (cls ? " " + cls : "")); i.type = "text"; i.placeholder = ph; i.maxLength = max || 40; i.readOnly = true;
    i.setAttribute("inputmode", "none"); i.autocomplete = "off";
    i.addEventListener("pointerdown", (e) => {
      e.preventDefault();
      if (window.CwKb?.openFor) window.CwKb.openFor(i, () => onDone && onDone(i.value.trim()));
      else { i.readOnly = false; i.focus(); }
    });
    i.addEventListener("change", () => onDone && onDone(i.value.trim()));
    return i;
  }
  function searchBar(ph, max, go, cls) {
    const wrap = h("div", "mdt-search" + (cls ? " " + cls : ""));
    wrap.innerHTML = svg("find");
    const i = field(ph, max, (v) => v && go(v));
    wrap.appendChild(i);
    wrap.appendChild(btn("Rechercher", () => i.value.trim() && go(i.value.trim()), "main"));
    return wrap;
  }
  function panel(title, extra) {
    const p = h("section", "mdt-panel");
    p.appendChild(h("header", "", `<b>${esc(title)}</b>${extra ? `<small>${esc(extra)}</small>` : ""}`));
    return p;
  }
  function empty(text, icon) { return h("div", "mdt-empty", (icon ? svg(icon) : "") + `<p>${esc(text)}</p>`); }
  function stars(n) { let s = ""; for (let i = 1; i <= 6; i++) s += `<i class="${i <= n ? "on" : ""}">${svg("star")}</i>`; return `<span class="mdt-stars">${s}</span>`; }

  // ---------- rendu ----------
  function renderNav() {
    $nav.replaceChildren();
    VIEWS.forEach(([k, label]) => {
      const n = k === "wanted" ? (d.wanted || []).length : k === "radio" ? 0 : 0;
      const b = h("button", "mdt-nav-i" + (k === view ? " on" : ""), svg(k === "dash" ? "dash" : k) + `<span>${esc(label)}</span>` + (n ? `<em>${n}</em>` : ""));
      b.type = "button";
      b.addEventListener("click", () => { view = k; render(); });
      $nav.appendChild(b);
    });
    $me.innerHTML = `<span class="mdt-avatar">${esc(initials(d.me))}</span><div><b>${esc(nm(d.me) || "Agent")}</b><small>${esc(d.tn || "Police")} · ${esc(d.rank || "—")}</small></div><i class="mdt-dot" title="En service"></i>`;
  }

  function setHead(title, sub, tools) {
    $head.replaceChildren();
    const t = h("div", "mdt-head-t", `<h2>${esc(title)}</h2>${sub ? `<p>${esc(sub)}</p>` : ""}`);
    $head.appendChild(t);
    if (tools) $head.appendChild(tools);
  }

  function viewDash() {
    const hour = new Date().getHours();
    setHead((hour < 18 ? "Bonjour, " : "Bonsoir, ") + (nm(d.me).split(" ")[0] || "agent"), "Situation de la ville en direct", btn("Actualiser", () => send("home"), "ghost", "refresh"));
    const stats = h("div", "mdt-stats");
    [
      ["Unités en service", (d.units || []).length, "user", ""],
      ["Avis de recherche", (d.wanted || []).length, "wanted", (d.wanted || []).length ? "warn" : ""],
      ["Amendes impayées", d.fines || 0, "fine", ""],
      ["Radars actifs", d.radars || 0, "radar", ""]
    ].forEach(([label, v, ic, cls]) => stats.appendChild(h("div", "mdt-stat " + cls, `${svg(ic)}<b>${Number(v) || 0}</b><small>${esc(label)}</small>`)));
    $body.appendChild(stats);

    const grid = h("div", "mdt-cols");
    const pw = panel("Avis de recherche", (d.wanted || []).length ? "les plus récents" : "");
    if (!(d.wanted || []).length) pw.appendChild(empty("Aucun suspect recherché pour le moment.", "wanted"));
    (d.wanted || []).slice(0, 4).forEach(([id, name, lvl, reason]) => {
      const r = h("button", "mdt-line", `<span class="mdt-avatar sm bad">${esc(initials(name))}</span><div><b>${esc(nm(name))}</b><small>${esc(reason || "Sans motif")}</small></div>${stars(lvl)}`);
      r.type = "button"; r.addEventListener("click", () => { view = "find"; send("find " + id); });
      pw.appendChild(r);
    });
    const pu = panel("Unités en service", (d.online ? d.online + " citoyens en ville" : ""));
    if (!(d.units || []).length) pu.appendChild(empty("Aucune unité en ligne.", "user"));
    (d.units || []).forEach(([id, name, fac, inVeh]) => {
      pu.appendChild(h("div", "mdt-line", `<span class="mdt-avatar sm">${esc(initials(name))}</span><div><b>${esc(nm(name))}</b><small>${esc(fac)} · ID ${id}</small></div><span class="mdt-tag ${inVeh ? "blue" : ""}">${inVeh ? "En patrouille" : "À pied"}</span>`));
    });
    grid.appendChild(pw); grid.appendChild(pu);
    $body.appendChild(grid);

    const pr = panel("Dernières transmissions radio");
    const chat = (d.chat || []).slice(-3);
    if (!chat.length) pr.appendChild(empty("Canal radio silencieux.", "radio"));
    chat.forEach(([f, n, t]) => pr.appendChild(h("div", "mdt-msg", `<b>${esc(f)} · ${esc(nm(n))}</b><p>${esc(t)}</p>`)));
    const more = btn("Ouvrir la radio", () => { view = "radio"; render(); }, "ghost", "radio");
    pr.appendChild(more);
    $body.appendChild(pr);
  }

  function personCard(p) {
    const wrap = h("article", "mdt-record");
    const status = p.wl > 0 ? `<span class="mdt-tag red">Recherché · ${p.wl} étoile${p.wl > 1 ? "s" : ""}</span>` : `<span class="mdt-tag green">Pas recherché</span>`;
    wrap.innerHTML = `
      <div class="mdt-rec-top">
        <div class="mdt-mug">${svg("user")}<small>${p.id >= 0 ? "ID " + p.id : "Hors ligne"}</small></div>
        <div class="mdt-rec-id">
          <h3>${esc(p.name)}</h3>
          <div class="mdt-tags">${status}<span class="mdt-tag ${p.id >= 0 ? "blue" : ""}">${p.id >= 0 ? "En ligne" : "Hors ligne"}</span>${p.fines ? `<span class="mdt-tag amber">${p.fines} amende${p.fines > 1 ? "s" : ""} · ${money(p.fsum)}</span>` : ""}</div>
          <dl>
            <div><dt>Niveau</dt><dd>${p.lvl >= 0 ? p.lvl : "—"}</dd></div>
            <div><dt>Motif de recherche</dt><dd>${esc(p.wr || "—")}</dd></div>
            <div><dt>Véhicules</dt><dd>${(p.veh || []).length}</dd></div>
          </dl>
        </div>
      </div>`;
    const cols = h("div", "mdt-cols");
    const pv = panel("Véhicules enregistrés");
    if (!(p.veh || []).length) pv.appendChild(empty("Aucun véhicule à son nom.", "car"));
    (p.veh || []).forEach(([pl, m]) => pv.appendChild(h("div", "mdt-line", `<span class="mdt-plate">${esc(pl)}</span><div><b>${esc(m)}</b></div>`)));
    const pf = panel("Amendes impayées");
    if (!(p.list || []).length) pf.appendChild(empty("Casier propre : aucune amende.", "fine"));
    (p.list || []).forEach(([id, am, r]) => pf.appendChild(h("div", "mdt-line", `<span class="mdt-num">N°${id}</span><div><b>${money(am)}</b><small>${esc(r)}</small></div>`)));
    cols.appendChild(pv); cols.appendChild(pf);
    wrap.appendChild(cols);

    if (p.id >= 0) {
      const act = panel("Actions", "agent : " + nm(d.me));
      const r1 = h("div", "mdt-act");
      r1.appendChild(btn("Localiser au GPS", () => send("gps " + p.id), "main", "gps"));
      act.appendChild(r1);
      const am = field("Montant ($)", 6), rs = field("Motif de l'amende", 60);
      const r2 = h("div", "mdt-act");
      r2.appendChild(am); r2.appendChild(rs);
      r2.appendChild(btn("Donner l'amende", () => { const v = parseInt(am.value, 10); if (v > 0) send("fine " + p.id + " " + v + " " + (rs.value.trim() || "Infraction")); }, "", "fine"));
      act.appendChild(r2);
      let lvl = Math.max(1, p.wl || 1);
      const pick = h("div", "mdt-starpick");
      const paint = () => Array.prototype.forEach.call(pick.children, (s, i) => s.classList.toggle("on", i < lvl));
      for (let i = 1; i <= 6; i++) { const s = h("button", "", svg("star")); s.type = "button"; s.setAttribute("aria-label", i + " étoiles"); s.addEventListener("click", () => { lvl = i; paint(); }); pick.appendChild(s); }
      paint();
      const wr = field("Motif de l'avis de recherche", 60);
      const r3 = h("div", "mdt-act");
      r3.appendChild(pick); r3.appendChild(wr);
      r3.appendChild(btn("Émettre l'avis", () => { const t = wr.value.trim(); if (t.length >= 3) send("want " + p.id + " " + lvl + " " + t); }, "danger", "wanted"));
      act.appendChild(r3);
      wrap.appendChild(act);
    }
    return wrap;
  }

  function viewFind() {
    setHead("Citoyens", "Fiche, véhicules, amendes et avis de recherche");
    $body.appendChild(searchBar("Nom (Prenom_Nom) ou ID du citoyen", 24, (v) => send("find " + v)));
    if (person) $body.appendChild(personCard(person));
    else $body.appendChild(empty("Cherche un citoyen pour ouvrir son dossier.", "user"));
  }

  function viewPlate() {
    setHead("Véhicules", "Recherche par plaque d'immatriculation");
    $body.appendChild(searchBar("Plaque (ex. CW-0012)", 10, (v) => send("plate " + v)));
    if (plate) {
      const c = h("article", "mdt-record");
      c.innerHTML = `<div class="mdt-rec-top"><div class="mdt-bigplate"><small>SAN ANDREAS</small><b>${esc(plate.plate)}</b></div>
        <div class="mdt-rec-id"><h3>${esc(plate.veh)}</h3><div class="mdt-tags"><span class="mdt-tag ${plate.id >= 0 ? "blue" : ""}">${plate.id >= 0 ? "Propriétaire en ligne · ID " + plate.id : "Propriétaire hors ligne"}</span></div>
        <dl><div><dt>Propriétaire</dt><dd>${esc(plate.owner || "—")}</dd></div></dl></div></div>`;
      if (plate.owner) { const a = h("div", "mdt-act"); a.appendChild(btn("Ouvrir le dossier du propriétaire", () => { view = "find"; send("find " + plate.owner.replace(/ /g, "_")); }, "main", "user")); c.appendChild(a); }
      $body.appendChild(c);
    } else $body.appendChild(empty("Entre une plaque pour retrouver le véhicule et son propriétaire.", "plate"));
  }

  function viewWanted() {
    const list = d.wanted || [];
    setHead("Avis de recherche", list.length ? list.length + " suspect" + (list.length > 1 ? "s" : "") + " en fuite" : "Rien à signaler", btn("Actualiser", () => send("home"), "ghost", "refresh"));
    if (!list.length) return $body.appendChild(empty("Aucun avis de recherche en cours.", "wanted"));
    const g = h("div", "mdt-grid");
    list.forEach(([id, name, lvl, reason]) => {
      const c = h("article", "mdt-wcard", `<div class="mdt-wtop"><span class="mdt-avatar bad">${esc(initials(name))}</span><div><b>${esc(nm(name))}</b><small>ID ${id}</small></div></div>${stars(lvl)}<p>${esc(reason || "Sans motif")}</p>`);
      const a = h("div", "mdt-act");
      a.appendChild(btn("Localiser", () => send("gps " + id), "main", "gps"));
      a.appendChild(btn("Dossier", () => { view = "find"; send("find " + id); }, "", "user"));
      c.appendChild(a);
      g.appendChild(c);
    });
    $body.appendChild(g);
  }

  function viewRadio() {
    setHead("Radio inter-services", "Police · FBI · Armée · EMS · Gouvernement  —  aussi /fr message");
    const box = h("div", "mdt-chat");
    const chat = d.chat || [];
    if (!chat.length) box.appendChild(empty("Canal silencieux. Le premier message ouvre la fréquence.", "radio"));
    chat.forEach(([f, n, t]) => {
      const mine = nm(n) === nm(d.me);
      box.appendChild(h("div", "mdt-bub" + (mine ? " me" : ""), `<b>${esc(f)} · ${esc(nm(n))}</b><p>${esc(t)}</p>`));
    });
    $body.appendChild(box);
    const go = (v) => { if (v) { send("say " + v); i.value = ""; } };
    const bar = h("div", "mdt-compose");
    const i = field("Message radio…", 90, go);
    bar.appendChild(i);
    bar.appendChild(btn("Envoyer", () => go(i.value.trim()), "main", "send"));
    $body.appendChild(bar);
    requestAnimationFrame(() => { box.scrollTop = box.scrollHeight; });
  }

  function viewCams() {
    setHead("Caméras de la ville", camOn ? "Caméra en cours : " + ((d.cams || [])[camOn - 1] || "") : "Touchez une caméra pour la voir");
    const g = h("div", "mdt-grid cams");
    (d.cams || []).forEach((name, k) => {
      const on = camOn === k + 1;
      const c = h("button", "mdt-cam" + (on ? " on" : ""), `<span class="mdt-rec"><i></i>${on ? "EN DIRECT" : "CAM " + String(k + 1).padStart(2, "0")}</span>${svg("cams")}<b>${esc(name)}</b>`);
      c.type = "button";
      c.addEventListener("click", () => { camOn = k + 1; send("cam " + k); render(); });
      g.appendChild(c);
    });
    $body.appendChild(g);
    if (camOn) { const a = h("div", "mdt-act"); a.appendChild(btn("Quitter la caméra", () => { camOn = 0; send("camoff"); render(); }, "danger", "cams")); $body.appendChild(a); }
  }

  function viewRadar() {
    setHead("Radar de vitesse", d.radar ? "Ton radar flashe au-dessus de " + d.radar + " km/h" : "Pose un radar à pied, au bord de la route");
    const p = panel(d.radar ? "Radar actif" : "Choisir la limite");
    p.appendChild(h("p", "mdt-note", "Les véhicules trop rapides sont flashés et reçoivent une amende automatique. Le radar reste 30 minutes."));
    if (d.radar) {
      p.appendChild(h("div", "mdt-gauge", `<b>${d.radar}</b><small>km/h</small>`));
      const a = h("div", "mdt-act"); a.appendChild(btn("Retirer mon radar", () => send("radar 0"), "danger", "radar")); p.appendChild(a);
    } else {
      const g = h("div", "mdt-chips");
      [30, 50, 70, 90, 110, 130].forEach((v) => g.appendChild(btn(v + " km/h", () => send("radar " + v), "", "")));
      p.appendChild(g);
    }
    $body.appendChild(p);
  }

  function render() {
    renderNav();
    $body.replaceChildren();
    $body.scrollTop = 0;
    if (view === "dash") viewDash();
    else if (view === "find") viewFind();
    else if (view === "plate") viewPlate();
    else if (view === "wanted") viewWanted();
    else if (view === "radio") viewRadio();
    else if (view === "cams") viewCams();
    else if (view === "radar") viewRadar();
    setTimeout(sync, 40);
  }

  window.CwMdt = {
    onData(x) {
      if (!x) return;
      const wasHidden = root.hidden;
      if (x.v === "home") { d = x; if (wasHidden) { view = "dash"; person = null; plate = null; } }
      else if (x.v === "person") { person = x.p; view = "find"; }
      else if (x.v === "plate") { plate = x.r; view = "plate"; }
      $("mdtLoginAv").textContent = initials(d.me);
      $("mdtLoginName").textContent = nm(d.me) || "Agent";
      $("mdtLoginRank").textContent = (d.tn || "Police") + " · " + (d.rank || "—") + (d.id != null ? " · matricule " + String(d.id).padStart(4, "0") : "");
      $login.classList.toggle("off", logged);
      render();
      show();
      try { window.Cw23 && window.Cw23.isOpen() && window.Cw23.hide(); } catch (_) {}
    },
    hide,
    isOpen: () => !root.hidden
  };
})();
