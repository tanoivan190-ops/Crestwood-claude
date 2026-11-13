/* ============================================================
   Panneau des gangs Crestwood (assets/gang)
   Serveur -> page : GangApp.openFromNative(json) / onUpdateFromNative(json) / closeFromNative()
   Page -> serveur : SampGang.action(type, '{"arg":"..."}')  -> /gangact <type> <arg>
   ============================================================ */
(function () {
  "use strict";

  const $ = (id) => document.getElementById(id);
  const fmt = (n) => Number(n || 0).toLocaleString("fr-FR");
  const esc = (s) => String(s == null ? "" : s).replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
  const NEUTRAL = "#5b6f8f";
  const RANKS = ["", "Recrue", "Soldat", "Lieutenant", "Chef"];
  const RANK_MANAGE = 3, RANK_MAX = 4;
  const bridge = () => window.SampGang || null;

  const ICON = {
    gps: '<svg viewBox="0 0 24 24"><path d="M12 21s-6.5-5.6-6.5-11a6.5 6.5 0 0 1 13 0c0 5.4-6.5 11-6.5 11Z"/><circle cx="12" cy="10" r="2.4"/></svg>',
    attack: '<svg viewBox="0 0 24 24"><path d="M14.5 4.5 19.5 4.5 19.5 9.5 9 20l-5-5L14.5 4.5Z"/><path d="m5 13 6 6M3.5 20.5l2-2"/></svg>',
    hq: '<svg viewBox="0 0 24 24"><path d="M4 20V10l8-6 8 6v10"/><path d="M9 20v-6h6v6"/></svg>',
    quit: '<svg viewBox="0 0 24 24"><path d="M15 4h4v16h-4M10 8l-4 4 4 4M6 12h10"/></svg>',
    up: '<svg viewBox="0 0 24 24"><path d="m6 14 6-6 6 6"/></svg>',
    down: '<svg viewBox="0 0 24 24"><path d="m6 10 6 6 6-6"/></svg>',
    kick: '<svg viewBox="0 0 24 24"><path d="M6 6l12 12M18 6 6 18"/></svg>'
  };

  // Donnees d'exemple : affichees seulement quand la page est ouverte hors du jeu (navigateur)
  const SAMPLE = {
    season: 1, left: 4 * 86400 + 12 * 3600, my: 2, rank: 4, here: 6, ok: 1, msg: "",
    gangs: [
      { id: 1, name: "Cold Blooded", tag: "CB", color: "#4da8ff", crest: "coldBlooded", points: 1840, members: 14, online: 3, trend: 1, home: 10, hq: [-1973, 117] },
      { id: 2, name: "Jungle Boys", tag: "JB", color: "#3ddc97", crest: "jungleBoys", points: 1798, members: 12, online: 4, trend: -1, home: 9, hq: [-2242, 128] },
      { id: 3, name: "Silent Killers", tag: "SK", color: "#ff4d4d", crest: "silentKillers", points: 1265, members: 11, online: 2, trend: 0, home: 0, hq: [-2625, 1411] },
      { id: 4, name: "Royal Vipers", tag: "RV", color: "#a855f7", crest: "royalVipers", points: 934, members: 9, online: 1, trend: 0, home: 5, hq: [-1638, 1202] }
    ],
    zones: [
      { name: "Battery Point", owner: 3, income: 120, home: 3, war: 0, left: 0, r: [-2741, 1268, -2533, 1490] },
      { name: "Esplanade Nord", owner: 4, income: 150, home: 0, war: 2, left: 140, r: [-2533, 1358, -1996, 1500] },
      { name: "Juniper Hollow", owner: 3, income: 110, home: 0, war: 0, left: 0, r: [-2533, 968, -2274, 1358] },
      { name: "Calton Heights", owner: 4, income: 140, home: 0, war: 0, left: 0, r: [-2274, 968, -1982, 1358] },
      { name: "Paradiso", owner: 0, income: 100, home: 0, war: 0, left: 900, r: [-2741, 793, -2533, 1268] },
      { name: "Downtown", owner: 4, income: 260, home: 4, war: 0, left: 0, r: [-1982, 744, -1580, 1274] },
      { name: "Juniper Hill", owner: 2, income: 115, home: 0, war: 0, left: 0, r: [-2533, 578, -2274, 968] },
      { name: "Chinatown", owner: 1, income: 180, home: 0, war: 0, left: 0, r: [-2274, 578, -1982, 968] },
      { name: "Ocean Flats", owner: 2, income: 120, home: 0, war: 0, left: 0, r: [-2994, -222, -2593, 277] },
      { name: "Garcia", owner: 2, income: 130, home: 2, war: 0, left: 0, r: [-2411, -222, -2173, 265] },
      { name: "Doherty", owner: 1, income: 160, home: 1, war: 0, left: 0, r: [-2173, -324, -1794, 265] },
      { name: "Easter Basin", owner: 1, income: 200, home: 0, war: 0, left: 0, r: [-1794, 249, -1242, 578] }
    ],
    events: [{ ago: 180, k: 1, g: 2, o: 4, z: 1 }, { ago: 660, k: 2, g: 1, o: 0, z: 7 }, { ago: 1560, k: 3, g: 3, o: 4, z: 2 }],
    members: [["Anzo_Diman", 4, 1], ["Kofi_Mensah", 3, 1], ["Yao_Kouassi", 2, 0], ["Ibra_Traore", 1, 1]]
  };

  let S = null;              // dernier etat recu
  let opened = false;
  let tab = "war";
  let selGang = null, selZone = null, ev = 0;
  let recvAt = Date.now();   // reception de l'etat : sert aux comptes a rebours
  let confirmKick = null, confirmQuit = false, toastTimer = 0;

  /* Carte reelle : decoupe de la carte du jeu (img/sf-map.jpg) couvrant X -3000..-1200, Y -400..1560.
     Monde -> SVG : sx = x + 3000, sy = 1560 - y (le nord en haut). */
  const MAP = { x0: -3000, y1: 1560, w: 1800, h: 1960 };
  const wx = (x) => x - MAP.x0;
  const wy = (y) => MAP.y1 - y;
  const vb = { x: 0, y: 0, w: MAP.w, h: MAP.h };
  const MIN_W = 320;
  const svgEl = () => document.getElementById("map");
  function clampVb() {
    vb.w = Math.max(MIN_W, Math.min(MAP.w, vb.w));
    vb.h = vb.w * MAP.h / MAP.w;
    vb.x = Math.max(0, Math.min(MAP.w - vb.w, vb.x));
    vb.y = Math.max(0, Math.min(MAP.h - vb.h, vb.y));
  }
  function applyVb() { clampVb(); svgEl().setAttribute("viewBox", vb.x.toFixed(1) + " " + vb.y.toFixed(1) + " " + vb.w.toFixed(1) + " " + vb.h.toFixed(1)); }
  function pxPerUnit() {
    const r = svgEl().getBoundingClientRect();
    if (!r.width || !r.height) return 0.15;
    return Math.min(r.width / vb.w, r.height / vb.h);
  }
  function zoomAt(cx, cy, factor) {
    const nw = Math.max(MIN_W, Math.min(MAP.w, vb.w / factor));
    const nh = nw * MAP.h / MAP.w;
    vb.x = cx - nw / 2; vb.y = cy - nh / 2; vb.w = nw; vb.h = nh;
    applyVb();
    renderMap();
  }
  function zoneRect(z) {
    const r = z.r || [0, 0, 0, 0];
    return { x: wx(r[0]), y: wy(r[3]), w: r[2] - r[0], h: r[3] - r[1] };
  }

  const gangs = () => (S && S.gangs) || [];
  const zones = () => (S && S.zones) || [];
  const gOf = (id) => gangs().find((g) => g.id === id);
  const colOf = (id) => (gOf(id) || { color: NEUTRAL }).color;
  const crest = (g, kind) => "img/" + g.crest + "-" + kind + ".webp";
  const count = (id) => zones().filter((z) => z.owner === id).length;
  const rate = (id) => zones().filter((z) => z.owner === id).reduce((a, z) => a + z.income, 0);
  const sorted = () => gangs().slice().sort((a, b) => b.points - a.points || a.id - b.id);
  const elapsed = () => Math.floor((Date.now() - recvAt) / 1000);
  const zLeft = (z) => Math.max(0, (z.left || 0) - elapsed());
  const mmss = (s) => Math.floor(s / 60) + ":" + String(s % 60).padStart(2, "0");
  const myGang = () => (S && S.my) || 0;
  const myRank = () => (S && S.rank) || 0;

  function send(type, arg) {
    const b = bridge();
    if (!b) { console.log("[gang] action", type, arg); return; }
    try { b.action(type, JSON.stringify({ arg: arg == null ? "" : String(arg) })); } catch (e) { console.log(e); }
  }

  function toast(msg, ok) {
    const el = $("toast");
    if (!msg) return;
    el.textContent = msg;
    el.classList.toggle("err", !ok);
    el.hidden = false;
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => { el.hidden = true; }, 3800);
  }

  /* ---------------- classement ---------------- */
  function renderBoard() {
    const list = sorted(), max = list[0] && list[0].points > 0 ? list[0].points : 1;
    $("rows").innerHTML = list.map((g, i) => {
      const t = g.trend || 0, n = count(g.id);
      return `<button class="g r${i + 1}" type="button" data-g="${g.id}" aria-selected="${g.id === selGang}" style="--c:${esc(g.color)}">
        <span class="rk">${i + 1}</span>
        <span class="emb"><img src="${crest(g, "av")}" alt=""></span>
        <span style="min-width:0"><span class="gn"><span class="n">${esc(g.name)}</span>${g.id === myGang() ? '<span class="you">VOUS</span>' : ""}</span>
          <span class="gs" style="display:block">${n} territoire${n > 1 ? "s" : ""} · ${g.members} membre${g.members > 1 ? "s" : ""} · <span class="on">${g.online} en ligne</span></span>
          <span class="bar" style="display:block"><i style="width:${Math.max(3, (g.points / max) * 100)}%"></i></span></span>
        <span class="pts">${fmt(g.points)}<small class="${t > 0 ? "up" : t < 0 ? "dn" : "eq"}">${t > 0 ? "▲ " + t : t < 0 ? "▼ " + -t : "—"}</small></span>
      </button>`;
    }).join("");
    $("cnt").textContent = list.length + " gangs";
    $("season").textContent = "Saison " + ((S && S.season) || 1) + " · classement en direct";
  }

  /* ---------------- carte ---------------- */
  function renderMap() {
    const here = S ? S.here : -1;
    const s = pxPerUnit();
    const fs = 10.5 / s, fsi = 8.5 / s, sw = 1 / s;
    let out = `<image href="img/sf-map.jpg" x="0" y="0" width="${MAP.w}" height="${MAP.h}" preserveAspectRatio="none"/>`;
    out += `<defs>${gangs().map((g) => `<clipPath id="hqc${g.id}"><circle cx="${wx(g.hq ? g.hq[0] : 0)}" cy="${wy(g.hq ? g.hq[1] : 0)}" r="${(15 / s).toFixed(1)}"/></clipPath>`).join("")}</defs>`;
    zones().forEach((z, k) => {
      if (!z.r) return;
      const R = zoneRect(z), col = colOf(z.owner), dim = selGang !== null && z.owner !== selGang;
      const stroke = z.war ? "#f5c451" : col;
      out += `<rect class="zr ${z.war ? "war" : ""} ${dim ? "dim" : ""} ${selZone === k ? "sel" : ""}" data-z="${k}" x="${R.x}" y="${R.y}" width="${R.w}" height="${R.h}"
        fill="${esc(col)}" fill-opacity="${z.owner ? ".42" : ".16"}" stroke="${esc(stroke)}" style="stroke-width:${((selZone === k ? 3 : 1.5) * sw).toFixed(2)}px"/>`;
      if (k === here) out += `<rect class="mez" x="${R.x + 4 * sw}" y="${R.y + 4 * sw}" width="${Math.max(1, R.w - 8 * sw)}" height="${Math.max(1, R.h - 8 * sw)}" style="stroke-width:${(2.4 * sw).toFixed(2)}px"/>`;
    });
    // QG : embleme du gang a sa vraie position
    gangs().forEach((g) => {
      if (!g.hq) return;
      const x = wx(g.hq[0]), y = wy(g.hq[1]), r = 15 / s, dim = selGang !== null && g.id !== selGang;
      out += `<g class="hqm ${dim ? "dim" : ""}"><circle cx="${x}" cy="${y}" r="${(r + 2.5 * sw).toFixed(1)}" fill="${esc(g.color)}"/>
        <image href="${crest(g, "av")}" x="${(x - r).toFixed(1)}" y="${(y - r).toFixed(1)}" width="${(2 * r).toFixed(1)}" height="${(2 * r).toFixed(1)}" clip-path="url(#hqc${g.id})" preserveAspectRatio="xMidYMid slice"/></g>`;
    });
    // noms : en taille ecran constante ; abreges quand la zone est trop petite a l'ecran
    zones().forEach((z, k) => {
      if (!z.r) return;
      const R = zoneRect(z), dim = selGang !== null && z.owner !== selGang;
      const cx = R.x + R.w / 2, cy = R.y + R.h / 2;
      const wPx = R.w * s, hPx = R.h * s;
      let label = z.name;
      if (wPx < z.name.length * 6.2) label = z.name.split(/\s+/).map((w) => w[0]).join("").toUpperCase();
      const showIncome = hPx > 34 && wPx > 44;
      out += `<text class="zl ${dim ? "dim" : ""}" x="${cx.toFixed(1)}" y="${(cy + (showIncome ? -2 / s : fs * 0.35)).toFixed(1)}" style="font-size:${fs.toFixed(1)}px;stroke-width:${(3 * sw).toFixed(2)}px">${esc(label)}</text>`;
      if (showIncome) out += `<text class="zi ${dim ? "dim" : ""}" x="${cx.toFixed(1)}" y="${(cy + 11 / s).toFixed(1)}" style="font-size:${fsi.toFixed(1)}px;stroke-width:${(3 * sw).toFixed(2)}px">+${z.income} pts/h</text>`;
    });
    svgEl().innerHTML = out;
    const hz = zones()[here];
    $("here").textContent = hz ? "Tu es à " + hz.name : "";
  }

  function zoneStatus(z) {
    if (z.war) return { cls: "war", txt: "En guerre · " + mmss(zLeft(z)) };
    if (z.home) return { cls: "hq", txt: "QG protégé" };
    if (zLeft(z) > 0) return { cls: "rest", txt: "Repos · " + Math.ceil(zLeft(z) / 60) + " min" };
    return z.owner ? { cls: "held", txt: "Contrôlé" } : { cls: "free", txt: "Libre" };
  }

  /* ---------------- bandeau d'information ---------------- */
  function renderInfo() {
    const el = $("info");
    if (selZone !== null && zones()[selZone]) {
      const z = zones()[selZone], g = gOf(z.owner), col = colOf(z.owner), st = zoneStatus(z);
      const att = z.war ? gOf(z.war) : null;
      const canAttack = myGang() > 0 && !z.home && z.owner !== myGang() && !z.war && zLeft(z) === 0;
      const isHere = S && S.here === selZone;
      let sub;
      if (att) sub = `<b style="color:${esc(att.color)}">${esc(att.name)}</b> attaque${g ? " <b style=\"color:" + esc(col) + "\">" + esc(g.name) + "</b>" : ""}`;
      else if (g) sub = `Tenu par <b style="color:${esc(col)}">${esc(g.name)}</b>`;
      else sub = "Aucun gang ne contrôle ce territoire";
      sub = `<b style="color:var(--gold)">+${z.income} pts/h</b> · ` + sub;
      if (canAttack && !isHere) sub += " · va sur place pour attaquer";
      el.classList.add("zone");
      el.style.setProperty("--c", att ? "#f5c451" : col);
      el.innerHTML = `${g ? `<img class="bn" src="${crest(g, "banner")}" alt="">` : ""}
        <div class="grow"><h2>${esc(z.name)}<span class="st ${st.cls}">${st.txt}</span></h2><div class="sub">${sub}</div></div>
        <div class="kv"><div><b style="color:var(--gold)">+${z.income}</b><small>Pts / h</small></div></div>
        <div class="acts">
          ${canAttack && isHere ? `<button class="chip-btn danger pulse" type="button" data-act="attack"><span class="ico">${ICON.attack}</span>Attaquer</button>` : ""}
          <button class="chip-btn" type="button" data-act="gps"><span class="ico">${ICON.gps}</span>Localiser</button>
        </div>`;
    } else if (selGang !== null && gOf(selGang)) {
      el.classList.remove("zone");
      const g = gOf(selGang), i = sorted().findIndex((x) => x.id === g.id) + 1;
      el.style.setProperty("--c", g.color);
      el.innerHTML = `<img class="bn" src="${crest(g, "banner")}" alt="">
        <div class="grow"><h2>${esc(g.name)}</h2><div class="sub">Rang #${i} · ${g.members} membres · ${g.online} en ligne</div></div>
        <div class="kv"><div><b>${count(g.id)}</b><small>Territoires</small></div><div><b>${fmt(g.points)}</b><small>Points</small></div><div><b style="color:var(--gold)">+${fmt(rate(g.id))}</b><small>Pts / h</small></div></div>
        <div class="acts"><button class="chip-btn ghost" type="button" data-act="hq" data-g="${g.id}"><span class="ico">${ICON.hq}</span>QG</button></div>`;
    } else {
      el.classList.remove("zone");
      const owned = zones().filter((z) => z.owner).length, wars = zones().filter((z) => z.war).length;
      el.style.setProperty("--c", "#3b82ff");
      el.innerHTML = `<div class="grow"><h2>Vue d'ensemble</h2><div class="sub">Touche un territoire ou un gang pour voir les détails</div></div>
        <div class="kv"><div><b>${owned}/${zones().length}</b><small>Conquis</small></div><div><b style="color:var(--gold)">${wars}</b><small>En guerre</small></div></div>`;
    }
  }

  /* ---------------- fil des evenements ---------------- */
  function evText(e) {
    const g = gOf(e.g), o = gOf(e.o), z = zones()[e.z];
    const G = g ? `<b style="color:${esc(g.color)}">${esc(g.name)}</b>` : "<b>?</b>";
    const O = o ? `<b style="color:${esc(o.color)}">${esc(o.name)}</b>` : "";
    const Z = z ? `<b>${esc(z.name)}</b>` : "";
    switch (e.k) {
      case 1: return `${G} attaque ${Z}${O ? " (" + O + ")" : ""}`;
      case 2: return `${G} a capturé ${Z}${O ? " à " + O : ""}`;
      case 3: return `${G} a défendu ${Z}${O ? " contre " + O : ""}`;
      case 4: return `L'attaque de ${G} sur ${Z} a échoué`;
      case 5: return `${G} remporte la saison !`;
      default: return "";
    }
  }
  function agoText(s) {
    s = Math.max(0, (s || 0) + elapsed());
    if (s < 60) return "à l'instant";
    if (s < 3600) return "il y a " + Math.floor(s / 60) + " min";
    if (s < 86400) return "il y a " + Math.floor(s / 3600) + " h";
    return "il y a " + Math.floor(s / 86400) + " j";
  }
  function renderFeed() {
    const list = (S && S.events) || [];
    const e = list[ev % Math.max(1, list.length)];
    $("feed").innerHTML = e ? `<span class="live"></span><span class="t">${evText(e)}</span><time>${agoText(e.ago)}</time>` : '<span class="t">Aucune guerre récente.</span>';
  }

  /* ---------------- onglet Mon gang ---------------- */
  function renderCrew() {
    const g = gOf(myGang());
    const card = $("crewCard"), list = $("mlist"), form = $("invite");
    $("tabCrew").hidden = false;
    if (!g) {
      card.style.setProperty("--c", "#3b82ff");
      card.innerHTML = `<h2>Sans gang</h2>
        <p class="help">Tu ne fais partie d'aucun gang. Rends-toi au QG d'un gang et demande à un <code>Lieutenant</code> ou au <code>Chef</code> de t'inviter, puis tape <code>/gangaccepter</code>.</p>
        <div class="row">${gangs().map((x) => `<button class="chip-btn ghost" type="button" data-act="hq" data-g="${x.id}" style="--c:${esc(x.color)}"><span class="ico">${ICON.hq}</span>QG ${esc(x.tag)}</button>`).join("")}</div>`;
      list.innerHTML = '<div class="empty">Rejoins un gang pour voir ses membres.</div>';
      $("mcount").textContent = "";
      form.hidden = true;
      return;
    }
    const rank = sorted().findIndex((x) => x.id === g.id) + 1;
    card.style.setProperty("--c", g.color);
    card.innerHTML = `<img class="banner" src="${crest(g, "banner")}" alt="${esc(g.name)}">
      <h2>${esc(g.name)}</h2>
      <div class="me">Ton grade : <b>${RANKS[myRank()] || "-"}</b></div>
      <div class="kv"><div><b>#${rank}</b><small>Rang</small></div><div><b>${count(g.id)}</b><small>Territoires</small></div><div><b>${fmt(g.points)}</b><small>Points</small></div></div>
      <div class="row">
        <button class="chip-btn" type="button" data-act="hq" data-g="${g.id}"><span class="ico">${ICON.hq}</span>GPS du QG</button>
        ${myRank() < RANK_MAX ? `<button class="chip-btn danger" type="button" data-act="quit"><span class="ico">${ICON.quit}</span>${confirmQuit ? "Confirmer ?" : "Quitter"}</button>` : ""}
      </div>
      <p class="help"><code>/g</code> radio du gang · <code>/capturer</code> attaque le territoire où tu te trouves.</p>`;

    const mem = (S && S.members) || [];
    $("mcount").textContent = g.members + " membre" + (g.members > 1 ? "s" : "") + " · " + g.online + " en ligne";
    const me = myRank();
    list.innerHTML = mem.length ? mem.map((m) => {
      const name = String(m[0]), r = Number(m[1]) || 1, on = Number(m[2]) === 1;
      const canKick = me >= RANK_MANAGE && r < me;
      const canRank = me >= RANK_MAX && r < RANK_MAX;
      const k = confirmKick === name;
      return `<div class="m" style="--c:${esc(g.color)}"><span class="dot ${on ? "on" : ""}"></span>
        <span class="nm">${esc(name.replace(/_/g, " "))}</span>
        <span class="rank r${r}">${RANKS[r] || r}</span>
        <span class="mact">
          ${canRank && r < RANK_MAX - 1 ? `<button class="mini" type="button" data-act="promote" data-n="${esc(name)}" aria-label="Promouvoir">${ICON.up}</button>` : ""}
          ${canRank && r > 1 ? `<button class="mini" type="button" data-act="demote" data-n="${esc(name)}" aria-label="Rétrograder">${ICON.down}</button>` : ""}
          ${canKick ? `<button class="mini kick ${k ? "confirm" : ""}" type="button" data-act="kick" data-n="${esc(name)}" aria-label="${k ? "Confirmer le renvoi" : "Renvoyer"}">${ICON.kick}</button>` : ""}
        </span></div>`;
    }).join("") : '<div class="empty">Aucun membre.</div>';
    form.hidden = me < RANK_MANAGE;
  }

  function tick() {
    if (!S) return;
    const ms = Math.max(0, ((S.left || 0) - elapsed()) * 1000);
    const d = Math.floor(ms / 864e5), h = Math.floor((ms % 864e5) / 36e5), m = Math.floor((ms % 36e5) / 6e4);
    $("timer").textContent = (d ? d + " j " : "") + h + " h " + String(m).padStart(2, "0");
    if (tab === "war" && selZone !== null && zones()[selZone] && (zones()[selZone].war || zones()[selZone].left)) renderInfo();
  }

  function setTab(t) {
    tab = t;
    document.querySelectorAll(".tabs [data-tab]").forEach((b) => b.setAttribute("aria-selected", String(b.dataset.tab === t)));
    document.querySelectorAll(".view").forEach((v) => { v.hidden = v.dataset.view !== t; });
    render();
  }

  function render() {
    if (!S) return;
    if (tab === "war") { renderBoard(); renderMap(); renderInfo(); renderFeed(); }
    else renderCrew();
    tick();
  }

  function apply(data, first) {
    let d = data;
    if (typeof d === "string") { try { d = JSON.parse(d); } catch (_) { d = null; } }
    if (!d || !Array.isArray(d.gangs)) return;
    S = d;
    recvAt = Date.now();
    confirmKick = null;
    confirmQuit = false;
    if (first) { selGang = null; selZone = null; }
    if (selZone !== null && !zones()[selZone]) selZone = null;
    $("app").hidden = false;
    render();
    if (d.msg) toast(d.msg, Number(d.ok) === 1);
  }

  /* ---------------- evenements ---------------- */
  $("rows").addEventListener("click", (e) => {
    const b = e.target.closest(".g");
    if (!b) return;
    const id = +b.dataset.g;
    selGang = selGang === id ? null : id;
    selZone = null;
    render();
  });
  let drag = null, dragged = false;
  $("mapw").addEventListener("pointerdown", (e) => {
    if (e.target.closest(".zoom")) return;
    drag = { id: e.pointerId, x: e.clientX, y: e.clientY, vx: vb.x, vy: vb.y };
    dragged = false;
  });
  $("mapw").addEventListener("pointermove", (e) => {
    if (!drag || e.pointerId !== drag.id) return;
    const dx = e.clientX - drag.x, dy = e.clientY - drag.y;
    if (!dragged && Math.hypot(dx, dy) < 7) return;
    if (!dragged) { dragged = true; $("mapw").classList.add("dragging"); try { $("mapw").setPointerCapture(e.pointerId); } catch (_) { /* ignore */ } }
    const s = pxPerUnit();
    vb.x = drag.vx - dx / s;
    vb.y = drag.vy - dy / s;
    applyVb();
  });
  const endDrag = () => { if (drag) { drag = null; $("mapw").classList.remove("dragging"); } };
  $("mapw").addEventListener("pointerup", endDrag);
  $("mapw").addEventListener("pointercancel", endDrag);
  $("map").addEventListener("click", (e) => {
    if (dragged) { dragged = false; return; }
    const p = e.target.closest(".zr");
    if (!p) { selZone = null; selGang = null; render(); return; }
    const k = +p.dataset.z;
    if (selZone === k) { selZone = null; selGang = null; render(); return; }
    selZone = k;
    selGang = zones()[k].owner || null;
    // vue d'ensemble : on zoome sur le territoire touche
    if (vb.w > MAP.w * 0.7) {
      const R = zoneRect(zones()[k]);
      zoomAt(R.x + R.w / 2, R.y + R.h / 2, MAP.w / Math.max(MIN_W, Math.max(R.w, R.h * MAP.w / MAP.h) * 2.6));
    }
    render();
  });
  document.querySelectorAll("[data-zoom]").forEach((b) => b.addEventListener("click", (e) => {
    e.stopPropagation();
    const m = b.dataset.zoom;
    if (m === "reset") { vb.x = 0; vb.y = 0; vb.w = MAP.w; applyVb(); renderMap(); return; }
    zoomAt(vb.x + vb.w / 2, vb.y + vb.h / 2, m === "in" ? 1.6 : 1 / 1.6);
  }));
  window.addEventListener("resize", () => { if (S && tab === "war") renderMap(); });

  document.addEventListener("click", (e) => {
    const b = e.target.closest("[data-act]");
    if (!b) return;
    const act = b.dataset.act;
    if (navigator.vibrate) { try { navigator.vibrate(15); } catch (_) { /* ignore */ } }
    if (act === "gps" && selZone !== null) send("gps", selZone);
    else if (act === "attack" && selZone !== null) send("attack", selZone);
    else if (act === "hq") send("hq", b.dataset.g || "");
    else if (act === "promote" || act === "demote") send(act, b.dataset.n);
    else if (act === "kick") {
      if (confirmKick === b.dataset.n) { confirmKick = null; send("kick", b.dataset.n); }
      else { confirmKick = b.dataset.n; renderCrew(); }
    } else if (act === "quit") {
      if (confirmQuit) { confirmQuit = false; send("quit"); }
      else { confirmQuit = true; renderCrew(); }
    }
  });
  document.querySelectorAll(".tabs [data-tab]").forEach((b) => b.addEventListener("click", () => setTab(b.dataset.tab)));
  $("invite").addEventListener("submit", (e) => {
    e.preventDefault();
    const v = $("inviteId").value.replace(/[^0-9]/g, "");
    if (!v) { toast("Entre l'ID du joueur à inviter.", false); return; }
    send("invite", v);
    $("inviteId").value = "";
    $("inviteId").blur();
  });
  $("close").addEventListener("click", () => {
    const b = bridge();
    if (b && b.close) b.close();
  });

  setInterval(tick, 1000);
  setInterval(() => { ev++; if (tab === "war") renderFeed(); }, 4500);

  /* ---------------- API appelee par SAMP.java ---------------- */
  window.GangApp = {
    openFromNative(json) { apply(json, !opened); opened = true; },
    onUpdateFromNative(json) { apply(json, false); },
    closeFromNative() { $("app").hidden = true; opened = false; }
  };

  const b = bridge();
  if (b && b.ready) { try { b.ready(); } catch (_) { /* ignore */ } }
  else { apply(SAMPLE, true); opened = true; }   // apercu dans un navigateur
})();
