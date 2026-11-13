/* Service de faction Crestwood
   Serveur -> page : SvcApp.openFromNative(json) / onUpdateFromNative(json)
   Page -> serveur : SampSvc.action(type, '{"arg":"..."}') -> /svcact <type> <arg> */
(function () {
  "use strict";
  const $ = (id) => document.getElementById(id);
  const bridge = () => window.SampSvc || null;
  const fmt = (n) => Math.max(0, Math.floor(Number(n) || 0)).toLocaleString("fr-FR");
  const esc = (s) => String(s == null ? "" : s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
  const nm = (s) => String(s || "").replace(/_/g, " ");
  const ini = (s) => { const p = nm(s).trim().split(/\s+/); return ((p[0] || "?")[0] + (p[1] ? p[1][0] : "")).toUpperCase(); };

  // [nom, sous-titre, couleur, icone]
  const TYPES = {
    0: ["Service", "Contrats et services", "#9fb0c3", "t0"],
    1: ["Police", "Ordre public", "#4d82ff", "t1"],
    2: ["Médecins", "Urgences et soins", "#ff5a6a", "t2"],
    3: ["Crestwood News", "Journalistes", "#f5c451", "t3"],
    4: ["Gouvernement", "Mairie de Crestwood", "#ffcc33", "t4"],
    5: ["Tueurs à gages", "Contrats", "#b48cff", "t5"],
    6: ["FBI", "Agence fédérale", "#3db5ff", "t1"],
    7: ["Mécaniciens", "Garage et dépannage", "#ffb84d", "t7"],
    8: ["Organisation", "Réseau clandestin", "#ff7a59", "t8"],
    9: ["Armée", "Forces armées", "#7fbf6a", "t9"],
    10: ["Concession", "Vente de véhicules", "#33ccff", "t10"],
    11: ["Burger Shot", "Restauration rapide", "#ff9f43", "t11"],
    12: ["Taxis", "Transport de passagers", "#f5d142", "t12"]
  };
  // meme ordre que g_SvcFood / g_SvcCars / g_SvcBlack dans le gamemode
  const FOOD = [["Burger simple", 150, 25], ["Menu Maxi", 300, 50], ["Menu Familial", 550, 100], ["Boisson fraîche", 80, 10]];
  const CARS = [["Faggio", 2500, "Scooter"], ["Sanchez", 6000, "Moto tout-terrain"], ["Premier", 12000, "Berline"], ["Elegant", 15000, "Berline luxe"],
    ["Sultan", 25000, "Sportive 4 portes"], ["Huntley", 30000, "SUV"], ["Infernus", 80000, "Supercar"]];
  const BLACK = [[0, "Couteau", 300, 8], [1, "Uzi + 300 balles", 3500, 8], [2, "AK-47 + 250 balles", 6000, 8], [3, "Gilet pare-balles", 2500, 8],
    [4, "Couteau", 300, 5], [5, "Pistolet silencieux + 100", 2500, 5], [6, "Sniper + 30 balles", 9000, 5], [7, "Gilet pare-balles", 2500, 5]];
  const COLORS = [[0, "#000"], [1, "#f5f5f5"], [3, "#a51d2d"], [6, "#f2c200"], [79, "#1f4fa3"], [86, "#2e7d32"], [126, "#ff6fb5"], [128, "#2a2a2a"], [146, "#6a2c91"], [252, "#c0c0c0"]];

  let S = null, toastTimer = 0, sel = { buyer: -1, item: -1, car: -1, paint: null };

  function send(type, arg) {
    const b = bridge();
    if (!b) { demo(type, arg); return; }
    try { b.action(type, JSON.stringify({ arg: arg == null ? "" : String(arg) })); } catch (e) { console.log(e); }
  }
  function toast(msg, ok) {
    if (!msg) return;
    const t = $("toast");
    t.textContent = msg; t.classList.toggle("err", !ok); t.hidden = false;
    clearTimeout(toastTimer); toastTimer = setTimeout(() => { t.hidden = true; }, 3400);
  }
  const el = (tag, cls, html) => { const e = document.createElement(tag); if (cls) e.className = cls; if (html != null) e.innerHTML = html; return e; };
  const btn = (label, cls, fn, dis) => { const b = el("button", "mini " + (cls || ""), esc(label)); b.type = "button"; if (dis) b.disabled = true; b.addEventListener("click", (e) => { e.stopPropagation(); fn(); }); return b; };
  const sec = (parent, label, right) => { const s = el("div", "sec", "<span>" + esc(label) + "</span>" + (right ? "<span>" + right + "</span>" : "")); parent.appendChild(s); return s; };
  const empty = (parent, text) => parent.appendChild(el("div", "empty", esc(text)));
  const mmss = (s) => { s = Math.max(0, Number(s) || 0); const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60); return h ? h + " h " + String(m).padStart(2, "0") : m + " min"; };
  const hpCls = (v) => (v < 30 ? "low" : v < 65 ? "mid" : "");

  function row(parent, name, sub, actions, extra) {
    const r = el("div", "row");
    r.innerHTML = '<span class="av">' + esc(ini(name)) + '</span><div class="t"><b>' + esc(nm(name)) + "</b><small>" + sub + "</small>" + (extra || "") + "</div>";
    const a = el("div", "acts"); (actions || []).forEach((b) => a.appendChild(b)); r.appendChild(a);
    parent.appendChild(r);
    return r;
  }

  function inputSend(parent, ph, action, label, max, area) {
    const w = el("div", area ? "" : "inline");
    const f = el(area ? "textarea" : "input", "field"); f.placeholder = ph; f.maxLength = max || 100;
    if (area) { f.rows = 2; f.style.width = "100%"; }
    const go = () => { const v = f.value.trim(); if (v.length < 2) { toast("Message trop court.", false); return; } send(action, v); f.value = ""; f.blur(); };
    f.addEventListener("keydown", (e) => { if (e.key === "Enter" && !area) go(); });
    w.appendChild(f);
    const b = el("button", "chip-btn", '<span class="ico"><svg viewBox="0 0 24 24"><path d="M4 12h14M13 6l6 6-6 6"/></svg></span>' + esc(label)); b.type = "button";
    b.addEventListener("click", go);
    if (area) { b.style.marginTop = "8px"; }
    w.appendChild(b);
    parent.appendChild(w);
    return f;
  }

  function buyersChips(parent, list) {
    const c = el("div", "chips");
    if (!list.length) { empty(parent, "Aucun client à côté de toi."); return; }
    if (!list.some((b) => b[0] === sel.buyer)) sel.buyer = list[0][0];
    list.forEach(([id, n]) => {
      const b = el("button", "chip" + (id === sel.buyer ? " on" : ""), esc(nm(n)) + " · " + id); b.type = "button";
      b.addEventListener("click", () => { sel.buyer = id; render(); });
      c.appendChild(b);
    });
    parent.appendChild(c);
  }

  /* ---------- sections metier ---------- */
  function secMedic(m) {
    sec(m, "Patients à proximité", (S.near || []).length + "");
    const l = el("div", "list"); m.appendChild(l);
    if (!(S.near || []).length) empty(l, "Personne à moins de 8 m. Rapproche-toi d'un patient.");
    (S.near || []).forEach(([id, n, hp]) => row(l, n, "Vie " + hp + " % · ID " + id, [btn("Examiner", "ghost", () => send("exam", id)), btn("Soigner", "", () => send("heal", id), hp >= 99)],
      '<div class="bar"><i class="' + hpCls(hp) + '" style="width:' + Math.min(100, hp) + '%"></i></div>'));
    sec(m, "Raccourcis");
    const c = el("div", "chips"); m.appendChild(c);
    c.appendChild(btn("Appels d'urgence", "red", () => send("go", "sos")));
    c.appendChild(btn("Facturer un soin", "gold", () => send("go", "bill")));
  }

  function secMech(m) {
    sec(m, "Véhicules à proximité", (S.veh || []).length + "");
    const l = el("div", "list"); m.appendChild(l);
    if (!(S.veh || []).length) empty(l, "Aucun véhicule à moins de 12 m.");
    (S.veh || []).forEach(([vid, n, hp, d]) => {
      const r = row(l, n, "État " + hp + " % · " + d + " m · n°" + vid, [btn("Retourner", "ghost", () => send("flip", vid)), btn("Peinture", "gold", () => { sel.paint = sel.paint === vid ? null : vid; render(); }), btn("Réparer", "", () => send("repair", vid))],
        '<div class="bar"><i class="' + hpCls(hp) + '" style="width:' + Math.min(100, hp) + '%"></i></div>');
      if (sel.paint === vid) {
        const p = el("div", "chips"); p.style.marginTop = "6px";
        COLORS.forEach(([c, hex]) => {
          const b = el("button", "chip", '<span style="display:inline-block;width:12px;height:12px;border-radius:50%;background:' + hex + ';vertical-align:-2px;margin-right:6px;border:1px solid rgba(255,255,255,.3)"></span>' + c); b.type = "button";
          b.addEventListener("click", () => { send("paint", vid + " " + c + " " + c); sel.paint = null; });
          p.appendChild(b);
        });
        r.querySelector(".t").appendChild(p);
      }
    });
    sec(m, "Raccourcis");
    const c = el("div", "chips"); m.appendChild(c);
    c.appendChild(btn("Appels dépanneuse", "red", () => send("go", "sos")));
    c.appendChild(btn("Facturer une réparation", "gold", () => send("go", "bill")));
  }

  function secTaxi(m) {
    sec(m, "Compteur");
    const box = el("div", "meter");
    box.innerHTML = '<div><div class="note">Tarif</div><div class="big">' + fmt(S.fare) + ' <small style="font-size:12px;color:var(--muted)">$/km</small></div></div>';
    const a = el("div", "inline");
    a.appendChild(btn("−", "ghost", () => send("fare", Math.max(20, (S.fare || 100) - 10))));
    a.appendChild(btn("+", "ghost", () => send("fare", Math.min(1000, (S.fare || 100) + 10))));
    a.appendChild(btn(S.meter ? "Arrêter le compteur" : "Lancer le compteur", S.meter ? "red" : "", () => send("meter"), !S.meter && !S.inVeh));
    box.appendChild(a);
    m.appendChild(box);
    m.appendChild(el("div", "note", S.meter ? "Compteur en marche : chaque passager reçoit sa facture à la fin de la course." : (S.inVeh ? "Lance le compteur quand tes clients sont montés." : "Mets-toi au volant de ton taxi pour lancer le compteur.")));
    sec(m, "Passagers", (S.pass || []).length + "");
    const l = el("div", "list"); m.appendChild(l);
    if (!(S.pass || []).length) empty(l, "Aucun passager au compteur.");
    (S.pass || []).forEach(([id, n, meters, cost]) => row(l, n, (meters / 1000).toLocaleString("fr-FR", { maximumFractionDigits: 1 }) + " km · " + fmt(cost) + " $", []));
    const c = el("div", "chips"); m.appendChild(c);
    c.appendChild(btn("Appels taxi", "red", () => send("go", "sos")));
  }

  function secNews(m) {
    sec(m, "Flash info", S.flashCd ? "prochain dans " + S.flashCd + " s" : "");
    inputSend(m, "Ex. Grand concert ce soir sur la plage de Santa Maria…", "flash", "Diffuser", 100, true);
    sec(m, "Direct", S.live ? '<span class="live"><i></i>EN DIRECT</span>' : "");
    m.appendChild(el("div", "note", S.live ? "Tout ce que tu écris dans le chat est diffusé à toute la ville" + (S.guest ? ", ainsi que ton invité " + esc(nm(S.guest)) + "." : ".") : "Lance un direct : ton chat est diffusé à toute la ville. Tu peux inviter une personne à côté de toi."));
    const c = el("div", "chips"); m.appendChild(c);
    c.appendChild(btn(S.live ? "Terminer le direct" : "Lancer le direct", S.live ? "red" : "", () => send("live")));
    if (S.live) {
      sec(m, "Inviter en direct");
      const l = el("div", "list"); m.appendChild(l);
      if (!(S.buyers || []).length) empty(l, "Personne à côté de toi.");
      (S.buyers || []).forEach(([id, n]) => row(l, n, "ID " + id, [btn("Inviter", "gold", () => send("guest", id))]));
    }
  }

  function secSell(m, catalog, isCar) {
    sec(m, isCar ? "Catalogue" : "Carte du restaurant");
    const g = el("div", "grid"); m.appendChild(g);
    const cur = isCar ? "car" : "item";
    if (sel[cur] < 0) sel[cur] = 0;
    catalog.forEach((it, i) => {
      const b = el("button", "item" + (sel[cur] === i ? " on" : ""), "<b>" + esc(it[0]) + "</b><small>" + esc(isCar ? it[2] : "+" + it[2] + " % de vie") + '</small><span class="p">' + fmt(it[1]) + " $</span>"); b.type = "button";
      b.addEventListener("click", () => { sel[cur] = i; render(); });
      g.appendChild(b);
    });
    sec(m, "Client");
    buyersChips(m, S.buyers || []);
    const c = el("div", "chips"); c.style.marginTop = "6px"; m.appendChild(c);
    const it = catalog[sel[cur]];
    c.appendChild(btn("Proposer " + it[0] + " · " + fmt(it[1]) + " $", "gold", () => send("offer", sel.buyer + " " + sel[cur]), !(S.buyers || []).length));
    m.appendChild(el("div", "note", isCar ? "Le client accepte et paie : le véhicule lui est livré (jusqu'à sa déconnexion). 90 % vont au coffre, 10 % pour toi." : "Le client accepte et paie : il mange et regagne de la vie. 90 % vont au coffre, 10 % pour toi."));
  }

  function secArmy(m) {
    sec(m, "Armurerie");
    const box = el("div", "meter");
    box.innerHTML = '<div><b style="font-size:14px">Paquetage de service</b><div class="note">Desert Eagle, M4, MP5 et gilet pare-balles</div></div>';
    box.appendChild(btn(S.kitCd ? "Disponible dans " + mmss(S.kitCd) : "Prendre", "", () => send("kit"), !!S.kitCd || !S.duty));
    m.appendChild(box);
    secAlert(m);
  }

  function secBlack(m, t) {
    sec(m, "Marché noir");
    const g = el("div", "grid"); m.appendChild(g);
    BLACK.filter((b) => b[3] === t).forEach(([idx, n, p]) => {
      const b = el("button", "item", "<b>" + esc(n) + '</b><span class="p">' + fmt(p) + " $</span>"); b.type = "button";
      b.addEventListener("click", () => send("buy", idx));
      g.appendChild(b);
    });
    secAlert(m);
  }

  function secAlert(m) {
    sec(m, "Alerte à toute la faction", S.alertCd ? S.alertCd + " s" : "");
    inputSend(m, "Ex. Renforts demandés à la base !", "alert", "Alerter", 100);
  }

  function secOfficial(m, t) {
    sec(m, "Raccourcis");
    const c = el("div", "chips"); m.appendChild(c);
    c.appendChild(btn("Panneau de faction", "", () => send("go", "fac")));
    if (t === 1 || t === 6) c.appendChild(btn("Appels police", "red", () => send("go", "sos")));
    c.appendChild(btn("Factures", "gold", () => send("go", "bill")));
    m.appendChild(el("div", "note", t === 4 ? "Les actions du gouvernement (licences, subventions, taxes) sont dans le panneau du gouvernement et le menu d'interaction." :
      "Fouille, menottes, amendes, arrestation : utilise le menu d'interaction sur un joueur (bouton à côté de lui)."));
  }

  function secContracts(m, t) {
    const list = S.con || [];
    if (t === 5) {
      sec(m, "Contrats ouverts", list.length + "");
      const l = el("div", "list"); m.appendChild(l);
      if (!list.length) empty(l, "Aucun contrat pour le moment.");
      list.forEach(([id, target, giver, price, taker, mineTaken]) => row(l, target, fmt(price) + " $ · " + (taker ? "pris par " + esc(taker) : "libre"),
        [btn(mineTaken ? "Abandonner" : "Accepter", mineTaken ? "ghost" : "gold", () => send("ctake", id), !!taker && !mineTaken)]));
      return;
    }
    sec(m, "Placer un contrat");
    m.appendChild(el("div", "note", "La prime est bloquée sur ta banque. Un tueur à gages la touche s'il élimine la cible ; tu peux annuler tant que personne ne l'a pris."));
    const w = el("div", "inline");
    const fid = el("input", "field"); fid.placeholder = "ID de la cible"; fid.inputMode = "numeric"; fid.maxLength = 3; fid.style.maxWidth = "130px";
    const fp = el("input", "field"); fp.placeholder = "Prime ($)"; fp.inputMode = "numeric"; fp.maxLength = 7;
    w.appendChild(fid); w.appendChild(fp);
    w.appendChild(btn("Placer", "red", () => {
      const id = parseInt(fid.value, 10), p = parseInt(fp.value, 10);
      if (!(id >= 0)) { toast("Indique l'ID de la cible.", false); return; }
      if (!(p >= 1000 && p <= 1000000)) { toast("Prime entre 1 000 et 1 000 000 $.", false); return; }
      send("contract", id + " " + p);
    }));
    m.appendChild(w);
    if (list.length) {
      sec(m, "Mes contrats", list.length + "");
      const l = el("div", "list"); m.appendChild(l);
      list.forEach(([id, target, giver, price, taker]) => row(l, target, fmt(price) + " $ · " + (taker ? "pris par un tueur" : "en attente"), [btn("Annuler", "ghost", () => send("ccancel", id), !!taker)]));
    }
  }

  /* ---------- colonne equipe ---------- */
  function side(col) {
    const team = S.team || [];
    sec(col, "Équipe en ligne", team.length + "");
    const l = el("div", "list scroll"); l.style.flex = "1"; l.style.minHeight = "0"; col.appendChild(l);
    if (!team.length) empty(l, "Aucun collègue en ligne.");
    team.forEach(([id, n, d, duty]) => row(l, n, (duty ? '<span class="st ok">En service</span> ' : '<span class="st info">Hors service</span> ') + d + " m", [btn("GPS", "ghost", () => send("gps", id))]));
    sec(col, "Radio de faction");
    inputSend(col, "Message radio…", "radio", "Envoyer", 100);
    if (Number(S.fund) >= 0) col.appendChild(el("div", "note", "Coffre de la faction : <b style='color:var(--gold)'>" + fmt(S.fund) + " $</b>"));
  }

  /* ---------- rendu ---------- */
  function renderOffer() {
    const kind = Number(S.kind);
    const t = TYPES[kind === 1 ? 11 : 10];
    document.documentElement.style.setProperty("--c", t[2]);
    $("tTitle").textContent = t[0];
    $("tSub").textContent = "Proposition";
    $("duty").hidden = true;
    const v = $("view");
    v.style.display = "flex";
    v.innerHTML = "";
    const card = el("div", "glass offer");
    card.innerHTML = '<div class="badge"><svg><use href="#' + t[3] + '"/></svg></div><h2>' + esc(nm(S.from)) + " te propose</h2><p>" + esc(S.what) +
      (kind === 2 ? " · véhicule livré devant toi" : " · repas") + '</p><div class="price">' + fmt(S.price) + ' $</div><div class="ttl" id="ttl"></div>';
    const r = el("div", "row2");
    const no = el("button", "chip-btn danger", '<span class="ico"><svg viewBox="0 0 24 24"><path d="M6 6l12 12M18 6 6 18"/></svg></span>Refuser'); no.type = "button";
    const yes = el("button", "chip-btn gold", '<span class="ico"><svg viewBox="0 0 24 24"><path d="m5 12 5 5 9-10"/></svg></span>Accepter et payer'); yes.type = "button";
    no.addEventListener("click", () => send("refuse"));
    yes.addEventListener("click", () => { yes.disabled = true; send("accept"); setTimeout(() => { yes.disabled = false; }, 900); });
    r.appendChild(no); r.appendChild(yes); card.appendChild(r);
    card.appendChild(el("p", "foot", "Liquide " + fmt(S.cash) + " $ · banque " + fmt(S.bank) + " $ (le liquide est utilisé en premier)"));
    v.appendChild(card);
    let ttl = Number(S.ttl) || 0;
    const tick = () => { const e = $("ttl"); if (e) e.textContent = ttl > 0 ? "Offre valable " + ttl + " s" : "Offre expirée"; };
    clearInterval(renderOffer.timer); tick();
    renderOffer.timer = setInterval(() => { ttl--; tick(); if (ttl <= 0) clearInterval(renderOffer.timer); }, 1000);
  }

  function render() {
    if (!S) return;
    $("app").hidden = false;
    $("cash").textContent = fmt(S.cash) + " $";
    if (S.v === "offer") { renderOffer(); return; }
    clearInterval(renderOffer.timer);
    const t = Number(S.t) || 0, T = TYPES[t] || TYPES[0];
    document.documentElement.style.setProperty("--c", T[2]);
    $("tTitle").textContent = t ? (S.fac || T[0]) : "Service";
    $("tSub").textContent = t ? (T[0] + (S.rk ? " · " + S.rk : "")) : "Aucune faction";
    const d = $("duty");
    d.hidden = !Number(S.hasDuty);
    d.classList.toggle("on", !!Number(S.duty));
    $("dutyTxt").textContent = Number(S.duty) ? "En service · " + mmss(S.dts) : "Prendre mon service";

    const v = $("view");
    v.style.display = "";
    const keep = v.querySelector(".col.main");
    const scroll = keep ? keep.scrollTop : 0;
    v.innerHTML = "";
    const main = el("div", "glass col main scroll");
    const hero = el("div", "hero");
    hero.innerHTML = '<span class="badge"><svg><use href="#' + T[3] + '"/></svg></span><div><b>' + esc(t ? T[0] : "Contrats") + "</b><small>" + esc(t ? T[1] : "Tu ne fais partie d'aucune faction.") + "</small></div>" +
      (t && Number(S.hasDuty) ? '<div class="stat"><b>' + (Number(S.duty) ? "EN SERVICE" : "HORS SERVICE") + "</b><small>" + (Number(S.duty) ? "depuis " + mmss(S.dts) : "appuie en haut à droite") + "</small></div>" : "");
    main.appendChild(hero);
    if (t === 2) secMedic(main);
    else if (t === 7) secMech(main);
    else if (t === 12) secTaxi(main);
    else if (t === 3) secNews(main);
    else if (t === 11) secSell(main, FOOD, false);
    else if (t === 10) secSell(main, CARS, true);
    else if (t === 9) secArmy(main);
    else if (t === 8 || t === 5) secBlack(main, t);
    else if (t === 1 || t === 4 || t === 6) secOfficial(main, t);
    if (t === 5 || t === 0 || (S.con || []).length) secContracts(main, t);
    v.appendChild(main);
    if (t) { const col = el("div", "glass col side"); side(col); v.appendChild(col); }
    else v.style.gridTemplateColumns = "1fr";
    main.scrollTop = scroll;
  }

  function apply(d) {
    if (typeof d === "string") { try { d = JSON.parse(d); } catch (_) { d = null; } }
    if (!d || typeof d !== "object") return;
    S = d;
    render();
    if (d.msg) toast(d.msg, Number(d.ok) === 1);
  }

  $("close").addEventListener("click", () => { const b = bridge(); if (b && b.close) b.close(); else $("app").hidden = true; });
  $("duty").addEventListener("click", () => send("duty"));

  window.SvcApp = {
    openFromNative(json) { sel = { buyer: -1, item: -1, car: -1, paint: null }; apply(json); },
    onUpdateFromNative(json) { apply(json); },
    closeFromNative() { $("app").hidden = true; }
  };

  /* ---------- apercu navigateur ---------- */
  const DEMO = {
    2: { v: "main", t: 2, fac: "Hôpital de Crestwood", rk: "Interne", me: "Awa Diallo", duty: 1, dts: 1520, hasDuty: 1, cash: 12500, bank: 84000, fund: 41000,
      team: [[3, "Moussa_Sow", 120, 1], [8, "Fatou_Ba", 860, 0]], near: [[4, "Ibra_Ndiaye", 34], [9, "Marc_Kone", 88]], con: [] },
    7: { v: "main", t: 7, fac: "Garage Mécano", rk: "Chef d'atelier", me: "Marc Kone", duty: 1, dts: 300, hasDuty: 1, cash: 7400, bank: 15000, fund: -1,
      team: [[2, "Ali_Toure", 40, 1]], veh: [[12, "Sultan", 41, 3], [27, "Premier", 96, 9]], con: [] },
    12: { v: "main", t: 12, fac: "Taxis Crestwood", rk: "Chauffeur", me: "Ibra Ndiaye", duty: 1, dts: 900, hasDuty: 1, cash: 3100, bank: 9000, fund: -1,
      team: [], fare: 120, meter: 1, inVeh: 1, pass: [[5, "Fatou_Ba", 2300, 276]], con: [] },
    3: { v: "main", t: 3, fac: "Crestwood News", rk: "Reporter", me: "Sara Diop", duty: 1, dts: 60, hasDuty: 1, cash: 2000, bank: 5000, fund: -1,
      team: [[6, "Yao_Kouame", 300, 1]], live: 1, guest: "Ali_Toure", flashCd: 34, buyers: [[2, "Ali_Toure"]], con: [] },
    11: { v: "main", t: 11, fac: "Burger Shot", rk: "Serveur", me: "Lina Kane", duty: 1, dts: 200, hasDuty: 1, cash: 900, bank: 3000, fund: 12000,
      team: [], buyers: [[4, "Ibra_Ndiaye"], [7, "Awa_Diallo"]], con: [] },
    10: { v: "main", t: 10, fac: "Concession Grotti", rk: "Vendeur", me: "Paul Mendy", duty: 0, dts: 0, hasDuty: 1, cash: 900, bank: 3000, fund: -1,
      team: [], buyers: [[4, "Ibra_Ndiaye"]], con: [] },
    9: { v: "main", t: 9, fac: "Armée de Crestwood", rk: "Sergent", me: "Yao Kouame", duty: 1, dts: 4000, hasDuty: 1, cash: 900, bank: 3000, fund: -1,
      team: [[1, "Ali_Toure", 20, 1]], kitCd: 0, alertCd: 0, con: [] },
    5: { v: "main", t: 5, fac: "Les Ombres", rk: "Exécutant", me: "X", duty: 0, dts: 0, hasDuty: 0, cash: 900, bank: 3000, fund: -1,
      team: [], kitCd: 0, alertCd: 0, con: [[3, "Marc_Kone", "Anonyme", 25000, "", 0, 0], [4, "Ali_Toure", "Anonyme", 8000, "X", 1, 0]] },
    0: { v: "main", t: 0, fac: "", rk: "", me: "Awa Diallo", duty: 0, dts: 0, hasDuty: 0, cash: 12500, bank: 84000, fund: -1, con: [[2, "Marc_Kone", "Awa_Diallo", 5000, "", 0, 1]] },
    offer: { v: "offer", kind: 1, from: "Lina_Kane", what: "Menu Maxi", price: 300, ttl: 52, cash: 12500, bank: 84000 }
  };
  function demo(type, arg) {
    const msg = { duty: "Tu es en service.", heal: "Patient soigné.", repair: "Véhicule réparé.", offer: "Offre envoyée.", accept: "Bon appétit !" }[type] || (type + " " + (arg || ""));
    setTimeout(() => apply(Object.assign({}, S, { ok: 1, msg })), 120);
  }
  const b = bridge();
  if (b && b.ready) { try { b.ready(); } catch (_) { /* ignore */ } }
  else { const k = (location.hash || "#2").slice(1); apply(DEMO[k] || DEMO[2]); }
})();
