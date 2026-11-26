/* =========================================================
   CRESTWOOD v23 - PAGES INTEGREES AU HUD (MDT, armee, assembleur, arene, craft)
   Meilleure methode d'affichage : tout vit dans la WebView du HUD deja chargee ->
   pas de chargement de page, pas de WebView en plus, ouverture instantanee.
   Serveur -> HUD : dialogue 9860 "@hud" -> CwBus(json)   (SAMP.java)
   HUD -> serveur : /c23 <module> <action> [args]           (Cw23.inc)
   Les champs texte utilisent le clavier rapide Crestwood (CwKb.openFor).
   ========================================================= */
(() => {
  "use strict";
  const esc = (s) => String(s == null ? "" : s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
  const money = (n) => (Number(n) || 0).toLocaleString("fr-FR") + " $";
  const send = (cmd) => { try { (window.SampHud || window.AndroidHUD)?.sendChatMessage?.("/c23 " + cmd); } catch (_) {} };
  const sync = () => { try { window.crestwoodSyncTouchBounds && window.crestwoodSyncTouchBounds(); } catch (_) {} };

  const root = document.createElement("aside");
  root.id = "cw23";
  root.className = "cw23";
  root.hidden = true;
  // [v24] le MDT s'ouvre en "ordinateur" facon macOS (classe .mac) : barre de titre avec les 3 feux,
  // barre laterale a gauche, contenu a droite, presque tout l'ecran en paysage.
  root.innerHTML = '<header><span class="cw23-lights"><button type="button" class="l-r" aria-label="Fermer"></button><button type="button" class="l-y" aria-label="Réduire"></button><button type="button" class="l-g" aria-label=""></button></span><div><b id="cw23T"></b><small id="cw23S"></small></div><button type="button" class="cw23-x" aria-label="Fermer"><svg viewBox="0 0 24 24"><path d="M6 6l12 12M18 6 6 18"/></svg></button></header><div class="cw23-main"><nav class="cw23-tabs" id="cw23Tabs"></nav><div class="cw23-body" id="cw23B"></div></div>';
  document.body.appendChild(root);
  const $T = root.querySelector("#cw23T"), $S = root.querySelector("#cw23S"), $tabs = root.querySelector("#cw23Tabs"), $b = root.querySelector("#cw23B");

  let mod = "", tab = "find", mdt = null, person = null, plate = null, camOn = 0;

  function show() { if (root.hidden) { root.hidden = false; requestAnimationFrame(() => root.classList.add("show")); } setTimeout(sync, 30); }
  function hide() {
    if (mod === "mdt" && camOn) send("mdt camoff");
    camOn = 0;
    root.classList.remove("show");
    setTimeout(() => { root.hidden = true; sync(); }, 180);
    try { window.CwKb?.close?.(); } catch (_) {}
  }
  root.querySelector(".cw23-x").addEventListener("click", hide);
  root.querySelector(".l-r").addEventListener("click", hide);
  root.querySelector(".l-y").addEventListener("click", hide);
  const ICON = {
    find: '<path d="M10.5 4a6.5 6.5 0 1 0 4 11.6l4.2 4.2 1.4-1.4-4.2-4.2A6.5 6.5 0 0 0 10.5 4zm0 2a4.5 4.5 0 1 1 0 9 4.5 4.5 0 0 1 0-9z"/>',
    wanted: '<path d="M12 3.5l2.6 5.3 5.9.9-4.3 4.1 1 5.8L12 16.9l-5.2 2.7 1-5.8-4.3-4.1 5.9-.9z"/>',
    radio: '<path d="M4 5h16a1 1 0 0 1 1 1v10a1 1 0 0 1-1 1H9l-4 3v-3H4a1 1 0 0 1-1-1V6a1 1 0 0 1 1-1z"/>',
    cams: '<path d="M3 7h12a1 1 0 0 1 1 1v2.5L21 7v10l-5-3.5V16a1 1 0 0 1-1 1H3a1 1 0 0 1-1-1V8a1 1 0 0 1 1-1z"/>',
    radar: '<path d="M12 3a9 9 0 1 0 9 9h-2a7 7 0 1 1-7-7V3zm0 4a5 5 0 1 0 5 5h-2a3 3 0 1 1-3-3V7zm0 4a1 1 0 1 0 0 2l8-8-1.4-1.4z"/>'
  };
  root.addEventListener("pointerdown", (e) => e.stopPropagation());

  // ---------- petits constructeurs ----------
  function h(tag, cls, html) { const e = document.createElement(tag); if (cls) e.className = cls; if (html != null) e.innerHTML = html; return e; }
  function btn(label, fn, cls) { const b = h("button", "cw23-btn" + (cls ? " " + cls : ""), esc(label)); b.type = "button"; b.addEventListener("click", fn); return b; }
  function field(ph, max) {
    const i = h("input", "cw23-in"); i.type = "text"; i.placeholder = ph; i.maxLength = max || 40; i.readOnly = true;
    i.setAttribute("inputmode", "none"); i.autocomplete = "off";
    i.addEventListener("pointerdown", (e) => {
      e.preventDefault();
      // [v31] clavier du telephone par defaut : CwKb.openFor renvoie false -> champ normal + clavier Android
      if (window.CwKb?.openFor && window.CwKb.openFor(i, () => i.dispatchEvent(new Event("change")))) return;
      i.readOnly = false; i.removeAttribute("inputmode"); i.focus(); window.SampHud?.showAndroidKeyboard?.();
    });
    return i;
  }
  function row(...kids) { const r = h("div", "cw23-row"); kids.forEach((k) => r.appendChild(k)); return r; }
  function tabs(list) {
    $tabs.replaceChildren();
    $tabs.hidden = !list.length;
    list.forEach(([k, label]) => { const b = h("button", k === tab ? "on" : "", (ICON[k] ? '<svg viewBox="0 0 24 24">' + ICON[k] + "</svg>" : "") + "<span>" + esc(label) + "</span>"); b.type = "button"; b.addEventListener("click", () => { tab = k; render(); }); $tabs.appendChild(b); });
  }

  // ---------- MDT ----------
  function renderMdt() {
    const d = mdt || {};
    $T.textContent = "MDT"; $S.textContent = (d.tn || "Forces de l'ordre") + " · " + String(d.me || "").replace(/_/g, " ");
    const TABS = [["find", "Recherche"], ["wanted", "Recherchés"], ["radio", "Radio"], ["cams", "Caméras"], ["radar", "Radar"]];
    tabs(TABS);
    $b.replaceChildren();
    $b.appendChild(h("h2", "cw23-h2", esc((TABS.find((t) => t[0] === tab) || TABS[0])[1])));
    if (tab === "find") {
      const n = field("Nom ou ID du citoyen", 24), p = field("Plaque (ex. CW-0012)", 10);
      $b.appendChild(row(n, btn("Chercher", () => n.value.trim() && send("mdt find " + n.value.trim()), "main")));
      $b.appendChild(row(p, btn("Plaque", () => p.value.trim() && send("mdt plate " + p.value.trim()))));
      n.addEventListener("change", () => n.value.trim() && send("mdt find " + n.value.trim()));
      p.addEventListener("change", () => p.value.trim() && send("mdt plate " + p.value.trim()));
      if (plate) {
        const c = h("div", "cw23-card");
        c.innerHTML = `<b>${esc(plate.plate)}</b><small>${esc(plate.veh)} · propriétaire ${esc(plate.owner)}${plate.id >= 0 ? " (en ligne, ID " + plate.id + ")" : " (hors ligne)"}</small>`;
        if (plate.owner) c.appendChild(btn("Fiche du propriétaire", () => send("mdt find " + plate.owner.replace(/ /g, "_"))));
        $b.appendChild(c);
      }
      if (person) $b.appendChild(personCard(person));
      if (!plate && !person) $b.appendChild(h("p", "cw23-hint", "Cherche un citoyen pour voir ses avis de recherche, ses amendes et ses véhicules."));
    } else if (tab === "wanted") {
      const list = d.wanted || [];
      if (!list.length) $b.appendChild(h("p", "cw23-hint", "Aucun avis de recherche en cours."));
      const grid = h("div", "cw23-list"); $b.appendChild(grid);
      list.forEach(([id, name, lvl, reason]) => {
        const c = h("div", "cw23-card");
        c.innerHTML = `<b>${esc(String(name).replace(/_/g, " "))} <em>${"★".repeat(lvl)}</em></b><small>ID ${id} · ${esc(reason || "—")}</small>`;
        c.appendChild(row(btn("Localiser", () => send("mdt gps " + id), "main"), btn("Fiche", () => { tab = "find"; send("mdt find " + id); })));
        grid.appendChild(c);
      });
    } else if (tab === "radio") {
      const box = h("div", "cw23-chat");
      (d.chat || []).forEach(([f, n, t]) => box.appendChild(h("p", "", `<b>${esc(f)} · ${esc(String(n).replace(/_/g, " "))}</b>${esc(t)}`)));
      if (!(d.chat || []).length) box.appendChild(h("p", "cw23-hint", "Radio commune police, FBI, armée, EMS et gouvernement. Aussi : /fr message"));
      $b.appendChild(box);
      const i = field("Message radio", 90);
      const go = () => { if (i.value.trim()) { send("mdt say " + i.value.trim()); i.value = ""; } };
      i.addEventListener("change", go);
      $b.appendChild(row(i, btn("Envoyer", go, "main")));
    } else if (tab === "cams") {
      const g = h("div", "cw23-grid");
      (d.cams || []).forEach((name, k) => g.appendChild(btn(name, () => { camOn = k + 1; send("mdt cam " + k); render(); }, camOn === k + 1 ? "on" : "")));
      $b.appendChild(g);
      $b.appendChild(btn("Quitter la caméra", () => { camOn = 0; send("mdt camoff"); render(); }, "danger"));
    } else if (tab === "radar") {
      $b.appendChild(h("p", "cw23-hint", d.radar ? `Ton radar flashe au-dessus de ${d.radar} km/h (30 min). Les excès donnent une amende automatique.` : "Pose un radar à pied, au bord de la route : il flashe les véhicules trop rapides et envoie l'amende."));
      if (d.radar) $b.appendChild(btn("Retirer mon radar", () => send("mdt radar 0"), "danger"));
      else { const g = h("div", "cw23-grid"); [50, 70, 90, 110, 130].forEach((v) => g.appendChild(btn(v + " km/h", () => send("mdt radar " + v)))); $b.appendChild(g); }
    }
  }
  function personCard(p) {
    const c = h("div", "cw23-card");
    const st = p.wl > 0 ? `<em class="bad">${"★".repeat(p.wl)} recherché</em>` : `<em class="ok">pas recherché</em>`;
    c.innerHTML = `<b>${esc(p.name)} ${st}</b><small>${p.id >= 0 ? "En ligne · ID " + p.id + (p.lvl >= 0 ? " · niveau " + p.lvl : "") : "Hors ligne"}${p.wr ? " · " + esc(p.wr) : ""}</small>
      <small>Amendes impayées : <b>${p.fines}</b>${p.fines ? " (" + money(p.fsum) + ")" : ""}</small>`;
    const veh = (p.veh || []).map(([pl, m]) => `<span class="cw23-plate">${esc(pl)}</span> ${esc(m)}`).join("<br>");
    c.appendChild(h("div", "cw23-sub", veh ? "<i>Véhicules</i>" + veh : "<i>Véhicules</i>Aucun véhicule enregistré."));
    if ((p.list || []).length) c.appendChild(h("div", "cw23-sub", "<i>Amendes</i>" + p.list.map(([id, am, r]) => `N.${id} · ${money(am)} · ${esc(r)}`).join("<br>")));
    if (p.id >= 0) {
      c.appendChild(btn("Localiser (GPS)", () => send("mdt gps " + p.id), "main"));
      const am = field("Montant $", 6), rs = field("Raison de l'amende", 60);
      c.appendChild(row(am, rs));
      c.appendChild(btn("Donner l'amende", () => { const v = parseInt(am.value, 10); if (v > 0) send("mdt fine " + p.id + " " + v + " " + (rs.value.trim() || "Infraction")); }, "main"));
      const wr = field("Avis : étoiles puis raison (ex. 3 vol a main armee)", 60);
      c.appendChild(row(wr, btn("Recherché", () => { const m = wr.value.trim().match(/^([0-6])\s+(.{3,})$/); if (m) send("mdt want " + p.id + " " + m[1] + " " + m[2]); })));
    }
    return c;
  }

  // ---------- ARMEE ----------
  let army = {};
  function renderArmy() {
    $T.textContent = "Armée · munitions"; $S.textContent = "Stocks des factions";
    tabs([]);
    $b.replaceChildren();
    const g = h("div", "cw23-stats");
    [["Armée", army.army], ["Police", army.police], ["FBI", army.fbi], ["Gouv.", army.gov]].forEach(([n, v]) => g.appendChild(h("div", "", `<b>${(Number(v) || 0).toLocaleString("fr-FR")}</b><small>${n}</small>`)));
    $b.appendChild(g);
    if (army.cargo > 0) $b.appendChild(h("p", "cw23-hint", `Cargaison à bord : <b>${army.cargo}</b> munitions.`));
    if (army.me === 9) {
      $b.appendChild(row(btn("Charger au dépôt", () => send("army load"), "main"), btn("Livrer à la base", () => send("army deliver"))));
      const q = field("Quantité", 5);
      $b.appendChild(row(q, btn("→ Police", () => send("army give police " + (parseInt(q.value, 10) || 100))), btn("→ FBI", () => send("army give fbi " + (parseInt(q.value, 10) || 100))), btn("→ Gouv", () => send("army give gov " + (parseInt(q.value, 10) || 100)))));
    }
    $b.appendChild(btn("Armurerie : remplir mon arme (+60)", () => send("army armory")));
  }

  // ---------- ASSEMBLEUR ----------
  let asm = {};
  function renderAsm() {
    $T.textContent = "Usine d'assemblage"; $S.textContent = (asm.team || []).filter(Boolean).map((n) => n.replace(/_/g, " ")).join(" + ");
    tabs([]);
    $b.replaceChildren();
    const done = asm.parts >= asm.total;
    $b.appendChild(h("div", "cw23-prog", `<i style="width:${Math.round(((done ? asm.paint : asm.parts / asm.total * 100) || 0))}%"></i>`));
    $b.appendChild(h("p", "cw23-hint", done ? `Peinture <b>${asm.color}</b> : ${asm.paint} % — vise la voiture avec la bombe (10 % / 5 s par bombe).` : `Pièces : <b>${asm.parts} / ${asm.total}</b> — prochaine : <b>${esc(asm.next)}</b>`));
    if (!done) $b.appendChild(row(btn("Prendre la pièce", () => send("asm piece"), "main"), btn("Poser sur la voiture", () => send("asm put"))));
    else $b.appendChild(btn("Prendre une bombe de peinture", () => send("asm spray"), "main"));
    $b.appendChild(btn("Quitter le poste", () => { send("asm quit"); hide(); }, "danger"));
  }

  // ---------- ARENE ----------
  let arena = {};
  function renderArena() {
    $T.textContent = "Arène PvP"; $S.textContent = `Toi : ${arena.k || 0} éliminations · ${arena.d || 0} morts`;
    tabs([]);
    $b.replaceChildren();
    const t = h("div", "cw23-chat");
    (arena.top || []).forEach(([n, k, d], i) => t.appendChild(h("p", "", `<b>${i + 1}. ${esc(String(n).replace(/_/g, " "))}</b>${k} élim. · ${d} morts`)));
    if (!(arena.top || []).length) t.appendChild(h("p", "cw23-hint", "Personne dans l'arène."));
    $b.appendChild(t);
    $b.appendChild(btn("Quitter l'arène", () => { send("arena leave"); hide(); }, "danger"));
  }

  // ---------- CRAFT ----------
  let craft = {};
  const RECIPES = [["ammo", "Munitions (arme en main)", 20], ["colt", "Pistolet 9 mm", 150], ["deagle", "Desert Eagle", 300], ["uzi", "Uzi", 350], ["pompe", "Fusil à pompe", 400], ["ak", "AK-47", 700], ["drogue", "Drogue", 30]];
  function renderCraft() {
    $T.textContent = "Établi"; $S.textContent = `${craft.mats || 0} matériaux · ${craft.drug || 0} drogue`;
    tabs([]);
    $b.replaceChildren();
    if (!craft.near) $b.appendChild(h("p", "cw23-hint", "Va à l'établi de Grove Street pour fabriquer. Matériaux : recyclage (/ramasser) et cargaisons militaires volées."));
    RECIPES.forEach(([k, n, c]) => {
      const r = h("div", "cw23-recipe", `<span><b>${n}</b><small>${c} matériaux</small></span>`);
      const b = btn("Fabriquer", () => send("craft " + k), (craft.mats || 0) >= c && craft.near ? "main" : "");
      b.disabled = !((craft.mats || 0) >= c && craft.near);
      r.appendChild(b);
      $b.appendChild(r);
    });
  }

  function render() {
    root.classList.toggle("mac", mod === "mdt");
    if (mod === "mdt") renderMdt();
    else if (mod === "army") renderArmy();
    else if (mod === "asm") renderAsm();
    else if (mod === "arena") renderArena();
    else if (mod === "craft") renderCraft();
    setTimeout(sync, 30);
  }

  window.CwBus = function (raw) {
    let d = raw;
    if (typeof d === "string") { try { d = JSON.parse(d); } catch (_) { return; } }
    if (!d || !d.m) return;
    // [v25] le MDT a sa propre fenetre (mdt.js) ; autres modules ajoutes par CwBusExt[m] (ex. bike.js)
    if (d.m === "mdt" && window.CwMdt) { if (!root.hidden) hide(); return window.CwMdt.onData(d); }
    if (window.CwBusExt && typeof window.CwBusExt[d.m] === "function") return window.CwBusExt[d.m](d);
    if (d.m === "mdt") {
      if (mod !== "mdt") { tab = "find"; person = null; plate = null; }
      if (d.v === "home") mdt = d;
      else if (d.v === "person") { person = d.p; plate = null; tab = "find"; }
      else if (d.v === "plate") { plate = d.r; person = null; tab = "find"; }
    } else if (d.m === "army") army = d;
    else if (d.m === "asm") { asm = d; if (!Number(d.on)) { if (mod === "asm") hide(); return; } }
    else if (d.m === "arena") { arena = d; if (!Number(d.on)) { if (mod === "arena") hide(); return; } }
    else if (d.m === "craft") craft = d;
    else return;
    mod = d.m;
    render();
    show();
  };
  window.Cw23 = { hide, isOpen: () => !root.hidden };
})();
