/* Immobilier Crestwood
   Serveur -> page : HouseApp.openFromNative(json) / onUpdateFromNative(json)
   Page -> serveur : SampHouse.action(type, '{"arg":"..."}') -> /houseact <type> <arg> */
(function () {
  "use strict";
  const $ = (id) => document.getElementById(id);
  const fmt = (n) => Number(n || 0).toLocaleString("fr-FR");
  const esc = (s) => String(s == null ? "" : s).replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
  const bridge = () => window.SampHouse || null;
  const TYPES = ["—", "Studio", "Appartement", "Maison", "Grande maison", "Villa"];
  const ROOMS = ["", "1 pièce", "2 pièces", "3 pièces", "5 pièces", "6 pièces + jardin"];
  const COLORS = ["#4da8ff", "#4da8ff", "#3db5ff", "#3ddc97", "#f5c451", "#b874ff"];
  const ICON = {
    buy: '<svg viewBox="0 0 24 24"><path d="M3 11 12 4l9 7"/><path d="M5 10v10h14V10"/><path d="M12 13v5M9.5 15.5h5"/></svg>',
    enter: '<svg viewBox="0 0 24 24"><path d="M14 4h5v16h-5"/><path d="M10 8l4 4-4 4M3 12h11"/></svg>',
    lock: '<svg viewBox="0 0 24 24"><rect x="5" y="11" width="14" height="10" rx="2.5"/><path d="M8 11V8a4 4 0 0 1 8 0v3"/></svg>',
    unlock: '<svg viewBox="0 0 24 24"><rect x="5" y="11" width="14" height="10" rx="2.5"/><path d="M8 11V8a4 4 0 0 1 7.7-1.5"/></svg>',
    sell: '<svg viewBox="0 0 24 24"><path d="M20 12 12 20l-8-8V4h8l8 8Z"/><circle cx="8" cy="8" r="1.5"/></svg>',
    gps: '<svg viewBox="0 0 24 24"><path d="M12 21s-6.5-5.6-6.5-11a6.5 6.5 0 0 1 13 0c0 5.4-6.5 11-6.5 11Z"/><circle cx="12" cy="10" r="2.4"/></svg>',
    plus: '<svg viewBox="0 0 24 24"><path d="M12 5v14M5 12h14"/></svg>',
    gavel: '<svg viewBox="0 0 24 24"><path d="m14 5 5 5M11 8l5 5M12.5 6.5l-6 6 3 3 6-6M8 14l-5 5"/><path d="M13 21h8"/></svg>',
    bolt: '<svg viewBox="0 0 24 24"><path d="M13 2 4 14h7l-1 8 9-12h-7l1-8Z"/></svg>',
    minus: '<svg viewBox="0 0 24 24"><path d="M5 12h14"/></svg>',
    home: '<svg viewBox="0 0 24 24"><path fill="currentColor" d="M12 3 2 11.5h3V21h5.5v-6h3v6H19v-9.5h3L12 3Z"/></svg>'
  };
  const SAMPLE = { v: "door", cash: 12500, bank: 4800000, mine: 1, max: 2, sellPct: 60, ok: 1, msg: "", now: Math.floor(Date.now() / 1000), atop: 0,
    h: { id: 7, t: 3, price: 250000, owner: "", me: 0, lock: 1, safe: 0, in: 0, ab: 1250000, abn: "Carl_Johnson", aend: Math.floor(Date.now() / 1000) + 50000, amin: 1312500, atop: 0, amax: 50000000, aopen: 500000 },
    list: [[3, 2, 120000, "Anzo_Diman", 1, 1, 640, 0, 0, 500000], [7, 3, 250000, "", 1, 0, 2, 1250000, Math.floor(Date.now() / 1000) + 50000, 500000], [9, 5, 1500000, "", 1, 0, 1830, 0, 0, 1500000], [12, 1, 60000, "", 1, 0, 920, 0, 0, 500000]] };

  let S = null, tab = "door", confirmFn = null, toastTimer = 0, clockSkew = 0;
  // [v15] encheres : temps restant (horloge du serveur)
  function serverNow() { return Math.floor(Date.now() / 1000) + clockSkew; }
  function leftText(end) {
    let s = Math.max(0, Number(end) - serverNow());
    const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), sec = s % 60;
    if (h > 0) return h + " h " + String(m).padStart(2, "0");
    return m + " min " + String(sec).padStart(2, "0");
  }
  setInterval(() => {
    document.querySelectorAll("[data-left]").forEach((e) => { e.textContent = leftText(e.dataset.left); });
  }, 1000);

  function send(type, arg) {
    const b = bridge();
    if (!b) { console.log("[house]", type, arg); return; }
    try { b.action(type, JSON.stringify({ arg: arg == null ? "" : String(arg) })); } catch (e) { console.log(e); }
  }
  function toast(msg, ok) {
    if (!msg) return;
    const t = $("toast");
    t.textContent = msg; t.classList.toggle("err", !ok); t.hidden = false;
    clearTimeout(toastTimer); toastTimer = setTimeout(() => { t.hidden = true; }, 3800);
  }
  function confirmBox(title, text, fn) { $("mTitle").textContent = title; $("mText").textContent = text; confirmFn = fn; $("modal").hidden = false; }
  $("mNo").addEventListener("click", () => { $("modal").hidden = true; confirmFn = null; });
  $("mYes").addEventListener("click", () => { $("modal").hidden = true; const f = confirmFn; confirmFn = null; if (f) f(); });

  function houseArt(type) {
    const c = COLORS[type] || "#4da8ff";
    return `<svg viewBox="0 0 120 120" aria-hidden="true">
      <defs><linearGradient id="g" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="${c}" stop-opacity=".95"/><stop offset="1" stop-color="${c}" stop-opacity=".45"/></linearGradient></defs>
      <ellipse cx="60" cy="104" rx="44" ry="7" fill="#000" opacity=".35"/>
      <path d="M18 56 60 22l42 34v46H18Z" fill="url(#g)" stroke="#fff" stroke-opacity=".5" stroke-width="2"/>
      <path d="M10 58 60 18l50 40" fill="none" stroke="#fff" stroke-width="6" stroke-linecap="round" stroke-linejoin="round"/>
      <rect x="50" y="70" width="20" height="32" rx="3" fill="#0b1220" stroke="#fff" stroke-opacity=".5"/>
      <circle cx="66" cy="87" r="1.8" fill="${c}"/>
      <rect x="27" y="64" width="15" height="13" rx="2" fill="#fff" opacity=".85"/>
      <rect x="78" y="64" width="15" height="13" rx="2" fill="#fff" opacity=".85"/>
      ${type >= 4 ? '<rect x="78" y="30" width="9" height="16" fill="#fff" opacity=".7"/>' : ""}
    </svg>`;
  }

  function renderDoor() {
    const h = S && S.h;
    const hero = $("hero"), side = $("side");
    if (!h) {
      hero.style.setProperty("--c", "#4da8ff");
      hero.innerHTML = `<div class="house-art">${houseArt(3)}</div><h2>Aucune maison ici</h2><div class="sub">Va devant la porte d'une maison (icône de maison) pour l'acheter ou y entrer.</div>`;
      side.innerHTML = `<div class="ph"><span>Immobilier</span></div><p class="hint">Les maisons à vendre et les tiennes sont dans l'onglet <b>Biens</b>, avec le GPS pour t'y rendre. Tu peux posséder jusqu'à ${S ? S.max : 2} maisons.</p>
        <div class="actions"><button class="chip-btn" type="button" data-go="list"><span class="ico">${ICON.gps}</span>Voir les biens</button></div>`;
      return;
    }
    const type = Number(h.t) || 1, owned = !!h.owner, me = Number(h.me) === 1, inside = Number(h.in) === 1;
    hero.style.setProperty("--c", COLORS[type]);
    const ownerName = owned ? String(h.owner).replace(/_/g, " ") : "";
    hero.innerHTML = `<div class="house-art">${houseArt(type)}</div>
      <h2>${esc(TYPES[type])} #${h.id}</h2>
      <div class="sub">${ROOMS[type]}${owned ? " · à " + esc(ownerName) : ""}</div>
      ${owned ? "" : (Number(h.ab) > 0 ? `<div class="price num">${fmt(h.ab)} FCFA</div><div class="sub">Offre actuelle · fin dans <b data-left="${h.aend}">${leftText(h.aend)}</b></div>` : `<div class="price num">${fmt(h.aopen || h.price)} FCFA</div><div class="sub">Mise de départ</div>`)}
      <div class="chips">
        ${owned ? (me ? '<span class="st ok">Ta maison</span>' : '<span class="st info">Habitée</span>') : '<span class="st gold">Aux enchères</span>'}
        ${!owned && Number(h.atop) ? '<span class="st ok">Tu mènes l\'enchère</span>' : ""}
        ${owned ? (Number(h.lock) ? '<span class="st bad">Fermée à clé</span>' : '<span class="st ok">Ouverte</span>') : ""}
        ${inside ? '<span class="st info">Tu es à l\'intérieur</span>' : ""}
      </div>`;

    let html = `<div class="facts">
      <div class="fact"><small>Type</small><b>${esc(TYPES[type])}</b></div>
      <div class="fact"><small>${owned ? "Valeur" : "Prix"}</small><b class="num">${fmt(h.price)}</b></div>
      <div class="fact"><small>Mes biens</small><b class="num">${S.mine} / ${S.max}</b></div>
    </div>`;
    const acts = [];
    if (!owned) {
      // [v15] enchères 24 h (500 000 à 50 000 000 FCFA)
      const min = Number(h.amin) || Number(h.aopen) || 500000, max = Number(h.amax) || 50000000;
      const top = Number(h.atop) === 1, full = S.mine + (Number(S.atop) || 0) >= S.max;
      const who = h.abn ? String(h.abn).replace(/_/g, " ") : "—";
      html = `<div class="facts">
        <div class="fact"><small>Offre actuelle</small><b class="num">${Number(h.ab) > 0 ? fmt(h.ab) : "Aucune"}</b></div>
        <div class="fact"><small>Fin dans</small><b class="num">${Number(h.ab) > 0 ? `<span data-left="${h.aend}">${leftText(h.aend)}</span>` : "24 h"}</b></div>
        <div class="fact"><small>Meilleur</small><b>${top ? "Toi" : esc(who)}</b></div>
      </div>
      <div class="safe auction"><h3><span>Faire une offre</span><span class="num">min ${fmt(min)}</span></h3>
        <div class="row"><input class="field num" id="bidAmount" type="text" inputmode="numeric" maxlength="8" value="${min}">
          <button class="mini" type="button" data-bump="${Math.max(10000, Math.round(min * 0.05))}" aria-label="Plus">${ICON.plus}</button></div>
        <div class="row quick">
          <button class="chip-btn ghost" type="button" data-set="${min}">Min.</button>
          <button class="chip-btn ghost" type="button" data-bump="100000">+100 000</button>
          <button class="chip-btn ghost" type="button" data-bump="1000000">+1 M</button>
        </div>
        <p class="hint">L'argent est bloqué sur ton compte bancaire (${fmt(S.bank)} FCFA). Si quelqu'un surenchérit, tu es remboursé tout de suite. L'enchère dure 24 h après la 1re offre ; une offre dans les 5 dernières minutes la prolonge de 5 min.</p>
      </div>`;
      acts.push(`<button class="chip-btn gold" type="button" data-act="bid" ${top || full ? "disabled" : ""}><span class="ico">${ICON.gavel}</span>Enchérir</button>`);
      acts.push(`<button class="chip-btn ghost" type="button" data-act="bidmax" ${top || full || S.bank < max ? "disabled" : ""}><span class="ico">${ICON.bolt}</span>Achat immédiat · ${fmt(max)}</button>`);
      if (top) html += '<p class="hint">Tu as la meilleure offre : attends la fin ou qu\'un autre joueur surenchérisse.</p>';
      else if (full) html += `<p class="hint">Tu as déjà ${S.max} maisons (ou offres gagnantes) : impossible d'enchérir.</p>`;
    }
    if (!inside && (me || !Number(h.lock))) acts.push(`<button class="chip-btn" type="button" data-act="enter"><span class="ico">${ICON.enter}</span>${owned ? "Entrer" : "Visiter"}</button>`);
    if (me) acts.push(`<button class="chip-btn ghost" type="button" data-act="lock"><span class="ico">${Number(h.lock) ? ICON.unlock : ICON.lock}</span>${Number(h.lock) ? "Ouvrir la porte" : "Fermer à clé"}</button>`);
    if (owned && !me && Number(h.lock) && !inside) html += `<p class="hint">La porte est fermée à clé : seul le propriétaire peut entrer.</p>`;
    html += `<div class="actions">${acts.join("")}</div>`;
    if (me) {
      html += `<div class="safe"><h3><span>Coffre-fort</span><span class="num">${fmt(h.safe)} FCFA</span></h3>
        ${inside ? `<div class="row"><input class="field num" id="safeAmount" type="text" inputmode="numeric" maxlength="9" placeholder="Montant">
          <button class="mini" type="button" data-act="deposit" aria-label="Déposer">${ICON.plus}</button>
          <button class="mini" type="button" data-act="withdraw" aria-label="Retirer">${ICON.minus}</button></div>
          <p class="hint">Liquide sur toi : <b class="num">${fmt(S.cash)} FCFA</b>. + range l'argent, − le reprend.</p>`
        : '<p class="hint">Le coffre se trouve à l\'intérieur de la maison.</p>'}</div>`;
      const back = Math.floor(h.price / 100) * S.sellPct + Number(h.safe || 0);
      html += `<div class="actions"><button class="chip-btn danger" type="button" data-act="sell"><span class="ico">${ICON.sell}</span>Vendre à l'État · ${fmt(back)} FCFA</button></div>`;
    }
    side.innerHTML = `<div class="ph"><span>${owned ? (me ? "Ma maison" : "Maison habitée") : "Maison à vendre"}</span><span class="num">#${h.id}</span></div>` + html;
  }

  function renderList() {
    const rows = (S && S.list) || [];
    const list = $("list");
    $("listCount").textContent = rows.length ? rows.length + " bien" + (rows.length > 1 ? "s" : "") : "";
    if (!rows.length) { list.innerHTML = '<div class="empty">Aucune maison à vendre pour le moment.</div>'; return; }
    list.innerHTML = rows.map((r) => {
      const [id, t, price, owner, lock, mine, dist, bid, end, open] = r;
      const auc = Number(bid) > 0 ? "Enchère " + fmt(bid) + ` FCFA · <span data-left="${end}">${leftText(end)}</span>` : "Départ " + fmt(open || price) + " FCFA";
      const sub = Number(mine) ? (Number(lock) ? "Fermée à clé" : "Ouverte") + " · à " + fmt(dist) + " m" : auc + " · à " + fmt(dist) + " m";
      return `<div class="card ${Number(mine) ? "mine" : ""}"><span class="ic">${ICON.home}</span>
        <span class="t"><b>${esc(TYPES[t] || "Maison")} #${id}${Number(mine) ? " · à toi" : ""}</b><small>${Number(mine) ? esc(sub) : sub}</small></span>
        <button class="mini" type="button" data-gps="${id}" aria-label="GPS">${ICON.gps}</button></div>`;
    }).join("");
  }

  function setTab(t) {
    tab = t;
    document.querySelectorAll("[data-tab]").forEach((b) => b.setAttribute("aria-selected", String(b.dataset.tab === t)));
    document.querySelectorAll("[data-view]").forEach((v) => { v.hidden = v.dataset.view !== t; });
    render();
  }
  function render() {
    if (!S) return;
    $("bank").textContent = fmt(S.bank) + " FCFA";
    if (tab === "door") renderDoor(); else renderList();
  }
  function apply(d) {
    if (typeof d === "string") { try { d = JSON.parse(d); } catch (_) { d = null; } }
    if (!d || typeof d !== "object") return;
    S = d;
    if (d.now) clockSkew = Number(d.now) - Math.floor(Date.now() / 1000);
    $("app").hidden = false;
    if (d.v === "list" && !d.h) setTab("list"); else if (d.h && tab !== "door" && !d.msg) setTab("door"); else render();
    if (d.msg) toast(d.msg, Number(d.ok) === 1);
  }

  document.addEventListener("click", (e) => {
    const go = e.target.closest("[data-go]");
    if (go) { setTab(go.dataset.go); return; }
    const bump = e.target.closest("[data-bump],[data-set]");
    if (bump) {
      const inp = $("bidAmount");
      if (inp) {
        const cur = Number(String(inp.value).replace(/[^0-9]/g, "")) || 0;
        const v = bump.dataset.set ? Number(bump.dataset.set) : cur + Number(bump.dataset.bump);
        inp.value = String(Math.min(50000000, v));
      }
      return;
    }
    const gps = e.target.closest("[data-gps]");
    if (gps) { send("gps", gps.dataset.gps); return; }
    const b = e.target.closest("[data-act]");
    if (!b || b.disabled) return;
    const a = b.dataset.act, h = S && S.h;
    if (a === "bid" && h) {
      const v = Number(String(($("bidAmount") || {}).value || "").replace(/[^0-9]/g, ""));
      if (!v) { toast("Entre un montant.", false); return; }
      if (v < (Number(h.amin) || 0)) { toast("Offre minimum : " + fmt(h.amin) + " FCFA.", false); return; }
      if (v > S.bank) { toast("Solde bancaire insuffisant.", false); return; }
      confirmBox("Enchérir " + fmt(v) + " FCFA ?", "La somme est bloquée sur ton compte jusqu'à la fin de l'enchère (remboursée si quelqu'un fait mieux).", () => send("bid", v));
    } else if (a === "bidmax" && h) confirmBox("Achat immédiat ?", fmt(h.amax || 50000000) + " FCFA : la maison est à toi tout de suite.", () => send("bid", h.amax || 50000000));
    else if (a === "buy" && h) confirmBox("Acheter cette maison ?", fmt(h.price) + " FCFA seront prélevés sur ton compte bancaire.", () => send("buy"));
    else if (a === "sell" && h) confirmBox("Vendre ta maison à l'État ?", "Tu récupères " + (S.sellPct || 60) + " % du prix + le contenu du coffre. Cette action est définitive.", () => send("sell"));
    else if (a === "deposit" || a === "withdraw") {
      const v = String(($("safeAmount") || {}).value || "").replace(/[^0-9]/g, "");
      if (!v) { toast("Entre un montant.", false); return; }
      send(a, v);
    } else send(a);
  });
  document.querySelectorAll("[data-tab]").forEach((b) => b.addEventListener("click", () => {
    setTab(b.dataset.tab);
    send("view", b.dataset.tab === "list" ? "list" : "door");
  }));
  $("close").addEventListener("click", () => { const b = bridge(); if (b && b.close) b.close(); });

  window.HouseApp = {
    openFromNative(json) { apply(json); },
    onUpdateFromNative(json) { apply(json); },
    closeFromNative() { $("app").hidden = true; }
  };
  const b = bridge();
  if (b && b.ready) { try { b.ready(); } catch (_) { /* ignore */ } } else apply(SAMPLE);
})();
