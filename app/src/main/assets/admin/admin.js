/* Panneau d'administration Crestwood.
   Serveur -> page : AdminApp.openFromNative(json) / AdminApp.onUpdateFromNative(json)
   Page -> serveur : SampAdmin.action(type, {id, arg}) puis /adminact (voir SAMP.java)
   Le serveur (basicgm.pwn, module ADMINISTRATION) re-verifie le niveau a chaque action :
   masquer un bouton ici n'est qu'un confort, pas une protection. */
(function () {
  "use strict";
  const bridge = window.SampAdmin || null;
  // 10 niveaux de staff (meme ordre que Admin_RoleName / Adm_Need dans le gamemode)
  const ROLES = ["Joueur", "Assistant", "Modérateur", "Modérateur+", "Administrateur", "Administrateur+",
    "Chef administrateur", "Gestionnaire", "Responsable", "Développeur", "Fondateur"];
  const ROLE_DESC = ["", "Aller vers, observer, infos, respawn, réparer", "Soigner, amener, geler, muet, avertir, annonces",
    "Expulser, prison, désarmer, tuer, monde / intérieur", "Factions, gangs, vestiaires, skin, invincible, TP coordonnées",
    "Bannir (durée), débannir, véhicules, météo, heure, armes, jetpack", "Ban hors ligne, créer / supprimer des factions, points de gang, commandes",
    "Rôles du staff (en dessous de son niveau)", "Argent liquide et banque", "Redémarrage, fin de saison des gangs", "Tous les droits"];
  const GANG_RANKS = ["Recrue", "Soldat", "Lieutenant", "Chef"];
  const state = { lvl: 0, role: "Joueur", me: -1, count: 0, players: [], lockers: [], cshops: [], chat: [], sel: null, tab: "players", filter: "",
    reports: [], repView: -1, thread: null, repFilter: "all", myName: "",
    fac: [], gangs: [], gangOnline: [], season: 1, bans: [], banNow: 0, log: [], logNow: 0,
    world: { invis: 0, rt: 1, gz: [], pumps: [], auc: [] } };
  const REP_CATS = ["Question", "Bug", "Signalement", "Remboursement", "Autre"];
  const REP_QUICK = ["Salut ! Je regarde ça tout de suite.", "J'arrive vers toi.", "Peux-tu me donner plus de détails ?", "C'est réglé de mon côté, tout est bon pour toi ?"];
  // Lieux du serveur (meme ordre que /lieux : mappings puis interieurs)
  const PLACES = [
    { n: "Mécanicien de Los Santos" }, { n: "Centre de rééducation (Palomino Creek)" }, { n: "Base de Verdant Meadows" },
    { n: "Mairie de Los Santos" }, { n: "Armurerie", entry: 1 }, { n: "Studio Crestwood News", entry: 2 }, { n: "Bunker de la base", entry: 3 }
  ];
  const FAC_TYPES = ["police", "medic", "news", "government", "hitman", "federal", "mechanic", "terrorist", "army", "cardealer", "burger", "taxi"];
  const FAC_TYPE_FR = ["Police", "Médecins", "Journalistes", "Gouvernement", "Tueurs à gages", "Fédéraux", "Mécaniciens", "Terroristes", "Armée", "Concessionnaire", "Burger", "Taxi"];
  function facOf(id) { return state.fac.find((f) => f.id === Number(id)) || null; }
  function facName(id) { const f = facOf(id); return f ? f.n : (Number(id) ? "Faction " + id : "Civil"); }
  function facLabel(p) {
    if (!p || !p.f) return "";
    const f = facOf(p.f);
    const rk = f && Array.isArray(f.rk) ? f.rk[(Number(p.r) || 1) - 1] : "";
    return facName(p.f) + (rk ? " · " + rk : "");
  }
  function gangOf(id) { return state.gangs.find((g) => g.id === Number(id)) || null; }
  function gangOfPlayer(pid) { const row = state.gangOnline.find((r) => Number(r[0]) === pid); return row ? { g: Number(row[1]), r: Number(row[2]) } : null; }
  function gangLabel(pid) {
    const m = gangOfPlayer(pid);
    if (!m) return "";
    const g = gangOf(m.g);
    return (g ? g.tag : "Gang " + m.g) + " · " + (GANG_RANKS[m.r - 1] || "");
  }
  function tier(l) { l = Number(l) || 0; return l <= 0 ? 0 : l <= 3 ? 1 : l <= 6 ? 2 : 3; }
  function roleName(l) { return ROLES[Math.max(0, Math.min(10, Number(l) || 0))]; }

  const $ = (id) => document.getElementById(id);
  function el(tag, cls, text) {
    const n = document.createElement(tag);
    if (cls) n.className = cls;
    if (text !== undefined) n.textContent = text;
    return n;
  }
  function parse(raw) {
    try { return JSON.parse(raw || "{}") || {}; } catch (_) { return {}; }
  }

  /* ------------------------------------------------------------ envoi */
  function send(type, id, arg) {
    const payload = JSON.stringify({ id: id == null ? 0 : id, arg: arg == null ? "" : String(arg) });
    if (bridge && typeof bridge.action === "function") {
      try { bridge.action(type, payload); } catch (_) {}
    } else {
      console.log("[admin] action", type, payload);
    }
  }

  /* ------------------------------------------------------------ actions */
  const ACTIONS = [
    { k: "goto",      label: "Aller vers",        sub: "Te téléporter",       lvl: 1, other: true },
    { k: "spec",      label: "Observer",          sub: "Mode spectateur",     lvl: 1, other: true },
    { k: "specoff",   label: "Arrêter l'observation", sub: "Revenir",         lvl: 1 },
    { k: "getinfo",   label: "Infos",             sub: "Ping, argent, IP",    lvl: 1 },
    { k: "respawn",   label: "Respawn",           sub: "Le faire réapparaître", lvl: 1 },
    { k: "fixveh",    label: "Réparer véhicule",  sub: "Son véhicule",        lvl: 1 },
    { k: "unfreeze",  label: "Dégeler",           sub: "Débloquer",           lvl: 1 },
    { k: "unmute",    label: "Rendre la parole",  sub: "Débloque le chat",    lvl: 1 },
    { k: "heal",      label: "Soigner",           sub: "Vie + gilet",         lvl: 2 },
    { k: "bring",     label: "Amener",            sub: "Le téléporter à toi", lvl: 2, other: true },
    { k: "freeze",    label: "Geler",             sub: "Bloquer",             lvl: 2, other: true },
    { k: "slap",      label: "Claque",            sub: "Le soulever",         lvl: 2, other: true },
    { k: "mute",      label: "Rendre muet",       sub: "Bloque le chat",      lvl: 2, other: true, danger: true },
    { k: "warn",      label: "Avertir",           sub: "3 = expulsion",       lvl: 2, other: true, danger: true,
      ask: { title: "Avertissement", text: "Raison de l'avertissement :", value: "Règlement" } },
    { k: "sethp",     label: "Vie",               sub: "0 à 100",             lvl: 2,
      ask: { title: "Points de vie", text: "Valeur de 0 à 100 :", value: "100", number: true } },
    { k: "armour",    label: "Gilet",             sub: "0 à 100",             lvl: 2,
      ask: { title: "Gilet pare-balles", text: "Valeur de 0 à 100 :", value: "100", number: true } },
    { k: "kick",      label: "Expulser",          sub: "Avec raison",         lvl: 3, other: true, danger: true,
      ask: { title: "Expulser le joueur", text: "Raison de l'expulsion :", value: "Comportement" } },
    { k: "jail",      label: "Prison",            sub: "En minutes",          lvl: 3, other: true, danger: true,
      ask: { title: "Envoyer en prison", text: "Durée en minutes (1 à 60) :", value: "10", number: true } },
    { k: "unjail",    label: "Libérer",           sub: "Sortir de prison",    lvl: 3 },
    { k: "disarm",    label: "Désarmer",          sub: "Retire les armes",    lvl: 3, other: true, danger: true },
    { k: "kill",      label: "Tuer",              sub: "Mort immédiate",      lvl: 3, other: true, danger: true },
    { k: "resetwarns", label: "Effacer avertis.", sub: "Remet à 0",           lvl: 3 },
    { k: "setvw",     label: "Monde virtuel",     sub: "0 à 9999",            lvl: 3,
      ask: { title: "Monde virtuel", text: "Numéro du monde (0 = monde normal) :", value: "0", number: true } },
    { k: "setint",    label: "Intérieur",         sub: "0 à 255",             lvl: 3,
      ask: { title: "Intérieur", text: "Numéro de l'intérieur (0 = extérieur) :", value: "0", number: true } },
    { k: "setfaction", label: "Définir la faction", sub: "Toutes les factions", lvl: 4, custom: "faction" },
    { k: "setrank",   label: "Grade de faction",  sub: "Selon sa faction",    lvl: 4, custom: "rank" },
    { k: "setgang",   label: "Définir le gang",   sub: "CB, JB, SK, RV",      lvl: 4, custom: "gang" },
    { k: "setgangrank", label: "Grade de gang",   sub: "Recrue à Chef",       lvl: 4,
      ask: { title: "Grade de gang", text: "Choisis le grade :", choices: GANG_RANKS, base: 1 } },
    { k: "setskin",   label: "Changer le skin",   sub: "0 à 311",             lvl: 4,
      ask: { title: "Skin", text: "ID du skin (0 à 311) :", value: "0", number: true } },
    { k: "god",       label: "Invincible",        sub: "Activer / couper",    lvl: 4 },
    { k: "ban",       label: "Bannir",            sub: "Avec durée",          lvl: 5, other: true, danger: true,
      ask: { title: "Bannir le joueur", text: "Durée en jours (0 = définitif) puis la raison. Ex : 7 Triche", value: "0 Règlement" } },
    { k: "givegun",   label: "Donner une arme",   sub: "ID + balles",         lvl: 5,
      ask: { title: "Donner une arme", text: "ID de l'arme et munitions (ex : 24 200) :", value: "24 200" } },
    { k: "jetpack",   label: "Jetpack",           sub: "À pied seulement",    lvl: 5 },
    { k: "delveh",    label: "Renvoyer véhicule", sub: "À son point",         lvl: 5 },
    { k: "makeadmin", label: "Définir le rôle",   sub: "Niveau 0 à 10",       lvl: 7, other: true, custom: "role" },
    { k: "givemoney", label: "Donner de l'argent", sub: "Liquide",            lvl: 8,
      ask: { title: "Donner de l'argent", text: "Montant en liquide (négatif pour retirer) :", value: "1000", number: true } },
    { k: "givebank",  label: "Argent en banque",  sub: "Compte bancaire",     lvl: 8,
      ask: { title: "Compte bancaire", text: "Montant (négatif pour retirer) :", value: "5000", number: true } }
  ];


  /* ------------------------------------------------------------ fenetre de saisie */
  let modalDone = null;
  function openModal(cfg, done) {
    $("admModalTitle").textContent = cfg.title || "";
    $("admModalText").textContent = cfg.text || "";
    const input = $("admModalInput");
    const choices = $("admModalChoices");
    modalDone = done;
    choices.replaceChildren();
    if (cfg.choices) {
      input.classList.add("hidden");
      choices.classList.remove("hidden");
      let picked = 0;
      cfg.choices.forEach((name, i) => {
        const b = el("button", "adm-btn" + (i === 0 ? " on" : ""), name);
        b.type = "button";
        b.addEventListener("click", () => {
          picked = i;
          Array.prototype.forEach.call(choices.children, (c, j) => c.classList.toggle("on", j === i));
          modalDone.pick = picked;
        });
        choices.appendChild(b);
      });
      modalDone.pick = 0;
      modalDone.mode = "choice";
    } else {
      choices.classList.add("hidden");
      input.classList.remove("hidden");
      input.value = cfg.value || "";
      input.inputMode = cfg.number ? "numeric" : "text";
      input.maxLength = 90;
      modalDone.mode = "text";
      setTimeout(() => { try { input.focus(); input.select(); } catch (_) {} }, 60);
    }
    $("admModal").classList.remove("hidden");
  }
  function closeModal() { $("admModal").classList.add("hidden"); modalDone = null; }
  $("admModalCancel").addEventListener("click", closeModal);
  $("admModalOk").addEventListener("click", () => {
    if (!modalDone) return;
    const fn = modalDone.fn;
    const value = modalDone.mode === "choice" ? String(modalDone.pick || 0) : $("admModalInput").value.trim();
    closeModal();
    fn(value);
  });

  /* ------------------------------------------------------------ toast */
  let toastTimer = null;
  function toast(msg, ok) {
    if (!msg) return;
    const t = $("admToast");
    t.textContent = msg;
    t.classList.toggle("err", !ok);
    t.classList.remove("hidden");
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => t.classList.add("hidden"), 3600);
  }

  /* ------------------------------------------------------------ rendu */
  function roleClass(l) { return tier(l) ? " r" + tier(l) : ""; }
  function initials(n) {
    const parts = String(n || "?").split(/[_\s.]+/).filter(Boolean);
    return ((parts[0] || "?").charAt(0) + (parts[1] ? parts[1].charAt(0) : "")).toUpperCase();
  }
  function selected() { return state.players.find((p) => p.id === state.sel) || null; }

  function renderHead() {
    $("admCount").textContent = state.count + (state.count > 1 ? " joueurs en ligne" : " joueur en ligne");
    const r = $("admRole");
    r.textContent = (state.lvl ? roleName(state.lvl) + " · niv. " + state.lvl : state.role) + (state.world && state.world.invis ? " · invisible" : "");
    r.className = "adm-role" + roleClass(state.lvl);
  }

  function renderList() {
    const list = $("admList");
    list.replaceChildren();
    const q = state.filter.toLowerCase();
    const rows = state.players.filter((p) => !q || p.n.toLowerCase().includes(q) || String(p.id) === q);
    if (!rows.length) { list.appendChild(el("div", "adm-empty", "Aucun joueur trouvé")); return; }
    rows.forEach((p) => {
      const b = el("button", "adm-row" + (p.id === state.sel ? " sel" : ""));
      b.type = "button";
      b.appendChild(el("div", "adm-av", initials(p.n)));
      const main = el("div", "adm-row-main");
      main.appendChild(el("div", "adm-row-name", p.n + (p.id === state.me ? " (toi)" : "")));
      const gl = gangLabel(p.id);
      main.appendChild(el("div", "adm-row-sub", "ID " + p.id + " · " + p.p + " ms" + (p.f ? " · " + facLabel(p) : "") + (gl ? " · " + gl : "")));
      b.appendChild(main);
      if (p.l > 0) b.appendChild(el("span", "adm-tag r" + tier(p.l), "Niv. " + p.l));
      b.addEventListener("click", () => { state.sel = p.id; renderList(); renderDetail(); });
      list.appendChild(b);
    });
  }

  function renderDetail() {
    const box = $("admDetail");
    box.replaceChildren();
    const p = selected();
    if (!p) { box.appendChild(el("div", "adm-empty", "Choisis un joueur dans la liste pour afficher les actions.")); return; }
    const isMe = p.id === state.me;

    const who = el("div", "adm-card");
    const row = el("div", "adm-who");
    row.appendChild(el("div", "adm-av", initials(p.n)));
    const info = el("div");
    info.appendChild(el("div", "adm-who-name", p.n));
    const gl = gangLabel(p.id);
    info.appendChild(el("div", "adm-who-sub", "ID " + p.id + " · " + p.p + " ms · " + roleName(p.l) + (p.f ? " · " + facLabel(p) : "") + (gl ? " · " + gl : "")));
    row.appendChild(info);
    who.appendChild(row);
    const bars = el("div", "adm-bars");
    [["Vie", p.h, ""], ["Gilet", p.a, " a"]].forEach(([label, val, cls]) => {
      const b = el("div", "adm-bar" + cls, label + " · " + Math.max(0, Math.min(100, val)) + "%");
      const track = el("i");
      const fill = el("b");
      fill.style.width = Math.max(0, Math.min(100, val)) + "%";
      track.appendChild(fill);
      b.appendChild(track);
      bars.appendChild(b);
    });
    who.appendChild(bars);
    box.appendChild(who);

    const acts = el("div", "adm-card");
    acts.appendChild(el("h3", "", "Actions"));
    const grid = el("div", "adm-grid");
    ACTIONS.forEach((a) => {
      const locked = state.lvl < a.lvl || (a.other && isMe);
      const b = el("button", "adm-btn" + (a.danger ? " danger" : "") + (locked ? " locked" : ""));
      b.type = "button";
      b.appendChild(document.createTextNode(a.label));
      b.appendChild(el("small", "", state.lvl < a.lvl ? "Niveau " + a.lvl + " · " + ROLES[a.lvl] : a.sub));
      b.addEventListener("click", () => runAction(a, p));
      grid.appendChild(b);
    });
    acts.appendChild(grid);
    box.appendChild(acts);
  }

  function pickFaction(p) {
    const list = [{ id: 0, n: "Civil (retirer)", rk: [] }].concat(state.fac);
    if (list.length === 1) { toast("Aucune faction chargée : appuie sur Actualiser.", false); return; }
    openModal({ title: "Faction de " + p.n, text: "Choisis la faction :", choices: list.map((f) => f.id ? f.n + " (#" + f.id + ")" : f.n) }, { fn: (pick) => {
      const f = list[Number(pick) || 0];
      if (!f.id) { send("setfaction", p.id, "0"); return; }
      const ranks = Array.isArray(f.rk) && f.rk.length ? f.rk : ["Grade 1"];
      openModal({ title: f.n, text: "Choisis le grade :", choices: ranks }, { fn: (r) => send("setfaction", p.id, f.id + " " + (Number(r) + 1)) });
    } });
  }
  function pickRank(p) {
    const f = facOf(p.f);
    if (!p.f || !f) { toast("Ce joueur n'est dans aucune faction.", false); return; }
    const ranks = Array.isArray(f.rk) && f.rk.length ? f.rk : ["Grade 1"];
    openModal({ title: "Grade · " + f.n, text: "Choisis le grade :", choices: ranks }, { fn: (r) => send("setrank", p.id, String(Number(r) + 1)) });
  }
  function pickGang(p) {
    const gangs = state.gangs.length ? state.gangs : [{ id: 1, n: "Cold Blooded" }, { id: 2, n: "Jungle Boys" }, { id: 3, n: "Silent Killers" }, { id: 4, n: "Royal Vipers" }];
    const list = [{ id: 0, n: "Aucun gang (retirer)" }].concat(gangs);
    openModal({ title: "Gang de " + p.n, text: "Choisis le gang :", choices: list.map((g) => g.n) }, { fn: (pick) => {
      const g = list[Number(pick) || 0];
      if (!g.id) { send("setgang", p.id, "0 1"); return; }
      openModal({ title: g.n, text: "Choisis le grade :", choices: GANG_RANKS }, { fn: (r) => send("setgang", p.id, g.id + " " + (Number(r) + 1)) });
    } });
  }
  function pickRole(p) {
    const max = state.lvl >= 10 ? 10 : state.lvl - 1;
    const list = ROLES.slice(0, Math.max(1, max + 1)).map((r, i) => i + " · " + r);
    openModal({ title: "Rôle staff de " + p.n, text: "Tu peux donner un niveau inférieur au tien :", choices: list }, { fn: (pick) => send("makeadmin", p.id, String(Number(pick) || 0)) });
  }

  function runAction(a, p) {
    if (a.custom === "faction") { pickFaction(p); return; }
    if (a.custom === "rank") { pickRank(p); return; }
    if (a.custom === "gang") { pickGang(p); return; }
    if (a.custom === "role") { pickRole(p); return; }
    if (!a.ask) { send(a.k, p.id, ""); return; }
    openModal(a.ask, { fn: (value) => {
      if (a.ask.number && !/^-?\d+$/.test(value)) { toast("Montant invalide.", false); return; }
      if (a.ask.choices && a.ask.base) value = String(Number(value) + a.ask.base);
      send(a.k, p.id, value);
    } });
  }

  /* ---- Onglet Serveur ---- */
  function serverCard(title, minLvl, buildBody) {
    const c = el("div", "adm-card" + (state.lvl < minLvl ? " locked" : ""));
    if (state.lvl < minLvl) c.appendChild(el("span", "lock", "Niveau " + minLvl + " · " + ROLES[minLvl]));
    c.appendChild(el("h3", "", title));
    buildBody(c);
    return c;
  }
  function inlineForm(card, placeholder, btnLabel, onSend, opts) {
    const row = el("div", "adm-inline");
    const input = el("input", "adm-input");
    input.type = "text";
    input.placeholder = placeholder;
    input.maxLength = 90;
    if (opts && opts.number) input.inputMode = "numeric";
    const btn = el("button", "adm-btn primary", btnLabel);
    btn.type = "button";
    btn.addEventListener("click", () => {
      const v = input.value.trim();
      if (!v) return;
      onSend(v);
      if (!(opts && opts.keep)) input.value = "";
    });
    row.appendChild(input);
    row.appendChild(btn);
    card.appendChild(row);
    return input;
  }
  function chips(card, items, onPick) {
    const wrap = el("div", "adm-chips");
    items.forEach(([label, value]) => {
      const c = el("button", "adm-chip", label);
      c.type = "button";
      c.addEventListener("click", () => onPick(value));
      wrap.appendChild(c);
    });
    card.appendChild(wrap);
  }

function renderServer() {
    const box = $("admServer");
    box.replaceChildren();
    box.appendChild(serverCard("Annonce globale", 2, (c) => {
      inlineForm(c, "Message dans le chat de tous les joueurs", "Envoyer", (v) => send("announce", 0, v));
      inlineForm(c, "Gros texte au milieu de l'écran", "Afficher", (v) => send("screen", 0, v));
    }));
    box.appendChild(serverCard("Joueurs", 2, (c) => {
      chips(c, [["Effacer le chat", "clearchat"]], (v) => send(v, 0, ""));
      if (state.lvl >= 4) chips(c, [["Soigner tout le monde", "healall"]], (v) => send(v, 0, ""));
      if (state.lvl >= 5) chips(c, [["Geler tout le monde", "freezeall"], ["Dégeler tout le monde", "unfreezeall"]], (v) => send(v, 0, ""));
    }));
    box.appendChild(serverCard("Sanctions en cours", 5, (c) => {
      c.appendChild(el("p", "adm-hint", "Bannissements actifs (nom + IP). Un ban temporaire se lève tout seul à la date de fin."));
      const list = el("div", "adm-lockers");
      if (!state.bans.length) list.appendChild(el("div", "adm-empty", "Aucun joueur banni."));
      state.bans.forEach((b) => {
        const [id, name, reason, by, until] = b;
        const row = el("div", "adm-locker");
        const info = el("div", "adm-locker-info");
        info.appendChild(el("b", "", name + " · " + (reason || "—")));
        const left = Number(until) ? Math.max(1, Math.ceil((Number(until) - state.banNow) / 86400)) : 0;
        info.appendChild(el("small", "", "Par " + by + " · " + (left ? "encore " + left + " jour" + (left > 1 ? "s" : "") : "définitif")));
        row.appendChild(info);
        const un = el("button", "adm-btn", "Débannir");
        un.type = "button";
        un.addEventListener("click", () => openModal({ title: "Débannir " + name + " ?", text: "Il pourra se reconnecter tout de suite." }, { fn: () => send("unban", id, "") }));
        row.appendChild(un);
        list.appendChild(row);
      });
      c.appendChild(list);
      if (state.lvl >= 6) {
        c.appendChild(el("p", "adm-hint", "Bannir un joueur hors ligne : Prénom_Nom, jours (0 = définitif), raison."));
        inlineForm(c, "Ex : Carl_Johnson 7 Triche", "Bannir", (v) => {
          openModal({ title: "Bannir hors ligne ?", text: v }, { fn: () => send("banoff", 0, v) });
        });
      }
    }));
    box.appendChild(serverCard("Téléportation", 4, (c) => {
      c.appendChild(el("p", "adm-hint", "Coordonnées X Y Z (tape /position en jeu pour lire les tiennes)."));
      inlineForm(c, "Ex : 1481 -1771 18", "Y aller", (v) => send("tpxyz", 0, v));
      chips(c, [["Mairie LS", "1481 -1771 18"], ["Aéroport LS", "1685 -2240 13"], ["Grove Street", "2495 -1687 13"], ["Vinewood", "1022 -1124 24"]], (v) => send("tpxyz", 0, v));
    }));
    box.appendChild(serverCard("Créer un véhicule", 5, (c) => {
      inlineForm(c, "Nom ou numéro (400 à 611)", "Créer", (v) => send("vehicle", 0, v));
      chips(c, [["Infernus", "411"], ["NRG-500", "522"], ["Sultan", "560"], ["Bullet", "541"], ["Banshee", "429"]], (v) => send("vehicle", 0, v));
    }));
    box.appendChild(serverCard("Météo", 5, (c) => {
      inlineForm(c, "Numéro de 0 à 45", "Appliquer", (v) => send("weather", 0, v), { number: true });
      chips(c, [["Soleil", "1"], ["Nuageux", "7"], ["Pluie", "8"], ["Brouillard", "9"], ["Tempête", "16"]], (v) => send("weather", 0, v));
    }));
    box.appendChild(serverCard("Heure du serveur", 5, (c) => {
      inlineForm(c, "Heure de 0 à 23", "Régler", (v) => send("time", 0, v), { number: true });
      chips(c, [["Aube 6h", "6"], ["Midi 12h", "12"], ["Soir 18h", "18"], ["Nuit 0h", "0"]], (v) => send("time", 0, v));
    }));
    box.appendChild(serverCard("Redémarrer le serveur", 9, (c) => {
      const b = el("button", "adm-btn danger", "Redémarrer maintenant");
      b.type = "button";
      b.style.marginTop = "6px";
      b.addEventListener("click", () => {
        openModal({ title: "Redémarrer le serveur ?", text: "Tous les joueurs seront déconnectés quelques secondes." }, { fn: () => send("gmx", 0, "") });
      });
      c.appendChild(b);
    }));
  }

  /* ---- [v15] Onglet Monde : invisibilite, zones vertes, stations-service, encheres, heure ---- */
  function fmtN(n) { return Number(n || 0).toLocaleString("fr-FR"); }
  function leftTxt(s) { s = Math.max(0, Number(s) || 0); const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60); return h ? h + " h " + String(m).padStart(2, "0") : m + " min"; }
  function rowItem(list, dotCls, title, sub, buttons) {
    const row = el("div", "adm-locker");
    if (dotCls) row.appendChild(el("span", dotCls));
    const info = el("div", "adm-locker-info");
    info.appendChild(el("b", "", title));
    info.appendChild(el("small", "", sub));
    row.appendChild(info);
    buttons.forEach(([label, cls, fn]) => {
      const b = el("button", "adm-btn" + (cls ? " " + cls : ""), label);
      b.type = "button";
      b.addEventListener("click", fn);
      row.appendChild(b);
    });
    list.appendChild(row);
  }
  function renderWorld() {
    const box = $("admWorld");
    if (!box) return;
    box.replaceChildren();
    const w = state.world;
    box.appendChild(serverCard("Mode invisible", 1, (c) => {
      c.appendChild(el("p", "adm-hint", w.invis
        ? "Tu es INVISIBLE : les joueurs ne voient ni ton personnage, ni ton pseudo, ni ton point sur la carte. Le staff te voit toujours."
        : "Deviens invisible pour observer sans être vu (personnage, pseudo et point de la carte cachés aux joueurs). Ton véhicule reste visible."));
      const b = el("button", "adm-btn " + (w.invis ? "danger" : "primary"), w.invis ? "Redevenir visible" : "Passer invisible");
      b.type = "button";
      b.style.marginTop = "6px";
      b.addEventListener("click", () => send("invis", 0, ""));
      c.appendChild(b);
    }));
    box.appendChild(serverCard("Zones vertes", 4, (c) => {
      c.appendChild(el("p", "adm-hint", "Zones publiques protégées : armes rangées, dégâts annulés, bandeau « Zone verte » sur le HUD. Police, FBI et armée gardent leurs armes."));
      const list = el("div", "adm-lockers");
      if (!w.gz.length) list.appendChild(el("div", "adm-empty", "Aucune zone verte."));
      w.gz.forEach((z) => {
        const [id, name, x, y, r] = z;
        const btns = [["Y aller", "", () => send("gztp", id, "")]];
        if (state.lvl >= 6) btns.push(["Supprimer", "danger", () => openModal({ title: "Supprimer « " + name + " » ?", text: "La zone redevient normale." }, { fn: () => send("gzdel", id, "") })]);
        rowItem(list, "adm-gz-dot", name + " · #" + id, "Rayon " + r + " m · " + x + ", " + y, btns);
      });
      c.appendChild(list);
      if (state.lvl >= 6) {
        c.appendChild(el("p", "adm-hint", "Nouvelle zone centrée sur ta position : rayon (10 à 250 m) puis nom."));
        inlineForm(c, "Ex : 50 Marché de Ganton", "Créer ici", (v) => send("gzadd", 0, v));
      }
    }));
    box.appendChild(serverCard("Stations-service", 4, (c) => {
      c.appendChild(el("p", "adm-hint", "Place-toi à côté de la pompe (dehors) puis « Poser une pompe ici ». Les joueurs y font le plein (main + icône pompe)."));
      const add = el("button", "adm-btn primary", "Poser une pompe ici");
      add.type = "button";
      add.style.margin = "4px 0 8px";
      add.addEventListener("click", () => send("pumpadd", 0, ""));
      c.appendChild(add);
      const list = el("div", "adm-lockers");
      if (!w.pumps.length) list.appendChild(el("div", "adm-empty", "Aucune pompe : les véhicules ne peuvent pas faire le plein."));
      w.pumps.forEach((p) => {
        const [db, x, y, z] = p;
        rowItem(list, "adm-pump-dot", "Pompe #" + db, x + ", " + y + ", " + z, [
          ["Y aller", "", () => send("pumptp", db, "")],
          ["Supprimer", "danger", () => openModal({ title: "Supprimer la pompe #" + db + " ?", text: "Elle disparaît tout de suite." }, { fn: () => send("pumpdel", db, "") })]
        ]);
      });
      c.appendChild(list);
    }));
    box.appendChild(serverCard("Enchères immobilières", 4, (c) => {
      c.appendChild(el("p", "adm-hint", "24 h après la 1re offre · de 500 000 à 50 000 000 FCFA · l'argent est bloqué, l'ancien enchérisseur est remboursé."));
      const list = el("div", "adm-lockers");
      if (!w.auc.length) list.appendChild(el("div", "adm-empty", "Aucune enchère en cours."));
      w.auc.forEach((a) => {
        const [hid, bid, who, left] = a;
        const btns = state.lvl >= 6 ? [["Annuler", "danger", () => openModal({ title: "Annuler l'enchère de la maison #" + hid + " ?", text: String(who).replace(/_/g, " ") + " sera remboursé de " + fmtN(bid) + " FCFA." }, { fn: () => send("aucstop", hid, "") })]] : [];
        rowItem(list, "", "Maison #" + hid + " · " + fmtN(bid) + " FCFA", String(who).replace(/_/g, " ") + " · fin dans " + leftTxt(left), btns);
      });
      c.appendChild(list);
    }));
    box.appendChild(serverCard("Heure réelle", 5, (c) => {
      c.appendChild(el("p", "adm-hint", w.rt ? "L'heure du jeu suit l'heure réelle (Abidjan), mise à jour chaque minute." : "Heure réelle coupée : l'heure du jeu ne bouge que si tu la règles (onglet Serveur)."));
      const b = el("button", "adm-btn " + (w.rt ? "danger" : "primary"), w.rt ? "Couper l'heure réelle" : "Activer l'heure réelle");
      b.type = "button";
      b.style.marginTop = "6px";
      b.addEventListener("click", () => send("rtime", 0, ""));
      c.appendChild(b);
    }));
  }

  /* ---- Onglet Factions : policiers en ligne + vestiaires ---- */
function renderFactions() {
    const box = $("admFactions");
    box.replaceChildren();
    const FAC_TYPE_OF = (t) => FAC_TYPE_FR[(Number(t) || 1) - 1] || "Faction";

    box.appendChild(serverCard("Factions du serveur", 4, (c) => {
      c.appendChild(el("p", "adm-hint", "Pour mettre un joueur dans une faction : onglet Joueurs → choisis le joueur → « Définir la faction » puis le grade."));
      const list = el("div", "adm-lockers");
      if (!state.fac.length) list.appendChild(el("div", "adm-empty", "Aucune faction. Crée-en une dans l'onglet Staff."));
      state.fac.forEach((f) => {
        const row = el("div", "adm-locker");
        const dot = el("span", "adm-fac-dot");
        dot.style.background = f.c || "#7db4ff";
        row.appendChild(dot);
        const info = el("div", "adm-locker-info");
        info.appendChild(el("b", "", f.n + " · #" + f.id));
        info.appendChild(el("small", "", FAC_TYPE_OF(f.t) + " · " + (f.rk ? f.rk.length : 0) + " grades · " + (f.on || 0) + " en ligne"));
        row.appendChild(info);
        if (state.lvl >= 6) {
          const del = el("button", "adm-btn danger", "Supprimer");
          del.type = "button";
          del.addEventListener("click", () => openModal({ title: "Supprimer « " + f.n + " » ?", text: "Ses membres redeviennent civils." }, { fn: () => send("removefac", f.id, "") }));
          row.appendChild(del);
        }
        list.appendChild(row);
      });
      c.appendChild(list);
    }));

    box.appendChild(serverCard("Membres de faction en ligne", 4, (c) => {
      const members = state.players.filter((p) => Number(p.f) > 0);
      const list = el("div", "adm-lockers");
      if (!members.length) list.appendChild(el("div", "adm-empty", "Aucun membre de faction connecté."));
      members.forEach((p) => {
        const row = el("div", "adm-locker");
        const info = el("div", "adm-locker-info");
        info.appendChild(el("b", "", p.n));
        info.appendChild(el("small", "", "ID " + p.id + " · " + facLabel(p)));
        row.appendChild(info);
        const rank = el("button", "adm-btn", "Grade");
        rank.type = "button";
        rank.addEventListener("click", () => pickRank(p));
        const retire = el("button", "adm-btn danger", "Retirer");
        retire.type = "button";
        retire.addEventListener("click", () => send("setfaction", p.id, "0"));
        row.appendChild(rank);
        row.appendChild(retire);
        list.appendChild(row);
      });
      c.appendChild(list);
    }));

    box.appendChild(serverCard("Gangs · saison " + (state.season || 1), 4, (c) => {
      c.appendChild(el("p", "adm-hint", "Mettre un joueur dans un gang : onglet Joueurs → « Définir le gang ». Les points se modifient à partir du niveau 6."));
      const list = el("div", "adm-lockers");
      if (!state.gangs.length) list.appendChild(el("div", "adm-empty", "Chargement des gangs…"));
      state.gangs.forEach((g) => {
        const row = el("div", "adm-locker");
        const dot = el("span", "adm-fac-dot");
        dot.style.background = g.c || "#7db4ff";
        row.appendChild(dot);
        const info = el("div", "adm-locker-info");
        info.appendChild(el("b", "", g.n + " (" + g.tag + ") · " + (Number(g.pts) || 0).toLocaleString("fr-FR") + " pts"));
        info.appendChild(el("small", "", g.m + " membres · " + g.on + " en ligne · " + g.z + " territoire" + (g.z > 1 ? "s" : "")));
        row.appendChild(info);
        if (state.lvl >= 6) {
          const pts = el("button", "adm-btn", "± Points");
          pts.type = "button";
          pts.addEventListener("click", () => openModal({ title: "Points · " + g.n, text: "Points à ajouter (négatif pour retirer) :", value: "100", number: true }, { fn: (v) => {
            if (!/^-?\d+$/.test(v)) { toast("Nombre invalide.", false); return; }
            send("gangpts", g.id, v);
          } }));
          row.appendChild(pts);
        }
        list.appendChild(row);
      });
      c.appendChild(list);
      const online = state.gangOnline.length;
      c.appendChild(el("p", "adm-hint", online + " membre" + (online > 1 ? "s" : "") + " de gang connecté" + (online > 1 ? "s" : "") + "."));
      if (state.lvl >= 9) {
        const end = el("button", "adm-btn danger", "Terminer la saison maintenant");
        end.type = "button";
        end.style.marginTop = "6px";
        end.addEventListener("click", () => openModal({ title: "Terminer la saison ?", text: "Le gang en tête gagne, tous les points repartent à 0." }, { fn: () => send("gangseason", 0, "") }));
        c.appendChild(end);
      }
    }));

    box.appendChild(serverCard("Vestiaires de faction", 4, (c) => {
      c.appendChild(el("p", "adm-hint", "Place-toi à l'endroit voulu (à pied), choisis la faction : le vestiaire apparaît avec une icône et un texte 3D."));
      const add = el("button", "adm-btn primary", "Créer un vestiaire ici");
      add.type = "button";
      add.style.marginTop = "6px";
      add.addEventListener("click", () => {
        if (!state.fac.length) { send("lockeradd", 0, "1"); return; }
        openModal({ title: "Vestiaire de quelle faction ?", text: "Seuls ses membres pourront l'utiliser.", choices: state.fac.map((f) => f.n) }, { fn: (pick) => send("lockeradd", 0, String(state.fac[Number(pick) || 0].id)) });
      });
      c.appendChild(add);
      const list = el("div", "adm-lockers");
      if (!state.lockers.length) list.appendChild(el("div", "adm-empty", "Aucun vestiaire pour le moment."));
      state.lockers.forEach((l) => {
        const row = el("div", "adm-locker");
        const info = el("div", "adm-locker-info");
        info.appendChild(el("b", "", "Vestiaire " + facName(l.f) + " #" + l.i));
        info.appendChild(el("small", "", "X " + l.x + " · Y " + l.y + " · Z " + l.z));
        row.appendChild(info);
        const go = el("button", "adm-btn", "Y aller");
        go.type = "button";
        go.addEventListener("click", () => send("lockergoto", l.i, ""));
        const del = el("button", "adm-btn danger", "Supprimer");
        del.type = "button";
        del.addEventListener("click", () => {
          openModal({ title: "Supprimer le vestiaire #" + l.i + " ?", text: "Il disparaît pour tous les joueurs." }, { fn: () => send("lockerdel", l.i, "") });
        });
        row.appendChild(go);
        row.appendChild(del);
        list.appendChild(row);
      });
      c.appendChild(list);
    }));
  }

  function setTab(name) {
    state.tab = name;
    Array.prototype.forEach.call(document.querySelectorAll(".adm-tab"), (b) => b.classList.toggle("active", b.dataset.tab === name));
    $("tabPlayers").classList.toggle("hidden", name !== "players");
    $("tabFactions").classList.toggle("hidden", name !== "factions");
    $("tabServer").classList.toggle("hidden", name !== "server");
    $("tabWorld").classList.toggle("hidden", name !== "world");
    if (name === "world") send("worldinfo", 0, "");
    $("tabStaff").classList.toggle("hidden", name !== "staff");
    $("tabCmds").classList.toggle("hidden", name !== "cmds");
    $("tabReports").classList.toggle("hidden", name !== "reports");
    if (name === "staff") scrollChat();
    if (name === "reports") { renderReports(); scrollRep(); }
  }

  function apply(data, fromOpen) {
    state.lvl = Number(data.lvl) || 0;
    state.role = data.role || "Joueur";
    state.me = data.me == null ? -1 : Number(data.me);
    state.count = Number(data.count) || 0;
    state.players = Array.isArray(data.players) ? data.players : [];
    const meRow = state.players.find((p) => p.id === state.me);
    if (meRow) state.myName = meRow.n;
    state.lockers = Array.isArray(data.lockers) ? data.lockers : [];
    if (Array.isArray(data.chat)) state.chat = data.chat;
    if (state.sel != null && !state.players.some((p) => p.id === state.sel)) state.sel = null;
    if (fromOpen && state.sel == null && state.me >= 0) state.sel = state.me;
    renderHead();
    renderList();
    renderDetail();
    renderFactions();
    renderServer();
    renderWorld();
    renderStaff();
    renderCmds();
  }

  function applyExtra(d) {
    if (d.x === "fac") state.fac = Array.isArray(d.fac) ? d.fac : [];
    else if (d.x === "gang") { state.gangs = Array.isArray(d.gangs) ? d.gangs : []; state.gangOnline = Array.isArray(d.online) ? d.online : []; state.season = Number(d.season) || 1; }
    else if (d.x === "ban") { state.bans = Array.isArray(d.bans) ? d.bans : []; state.banNow = Number(d.now) || Math.floor(Date.now() / 1000); }
    else if (d.x === "log") { state.log = Array.isArray(d.log) ? d.log : []; state.logNow = Number(d.now) || 0; }
    else if (d.x === "cshop") state.cshops = Array.isArray(d.shops) ? d.shops : [];
    else if (d.x === "world") { state.world = { invis: Number(d.invis) || 0, rt: Number(d.rt) || 0, gz: d.gz || [], pumps: d.pumps || [], auc: d.auc || [] }; renderWorld(); renderHead(); return; }
    else return;
    if (d.x === "fac" || d.x === "gang") { renderList(); renderDetail(); renderFactions(); }
    if (d.x === "cshop") renderFactions();
    if (d.x === "ban") renderServer();
    if (d.x === "log") renderStaff();
  }

  /* ---- Onglet Commandes : toutes les commandes staff du serveur, avec leurs champs ---- */
  const LVL_SEL = ROLES.map((r, i) => [String(i), i + " · " + r]);
  const GANG_SEL = [["0", "0 · Retirer du gang"], ["1", "1 · Cold Blooded"], ["2", "2 · Jungle Boys"], ["3", "3 · Silent Killers"], ["4", "4 · Royal Vipers"]];
  const GRANK_SEL = [["1", "1 · Recrue"], ["2", "2 · Soldat"], ["3", "3 · Lieutenant"], ["4", "4 · Chef"]];
  const HOUSE_SEL = [["1", "1 · Studio"], ["2", "2 · Appartement"], ["3", "3 · Maison"], ["4", "4 · Grande maison"], ["5", "5 · Villa"]];
  const ITEM_SEL = [["all", "Tous les objets"], ["iphone_orange", "iPhone orange"], ["iphone_noir", "iPhone noir"], ["enceinte_jbl", "Enceinte JBL"], ["apple_watch", "Apple Watch"],
    ["liasse_billets", "Liasse de billets"], ["gilet_tactique", "Gilet tactique"], ["vody", "Vody"], ["eau_kirene", "Eau Kirène"]];
  const FTYPE_SEL = FAC_TYPES.map((t, i) => [t, FAC_TYPE_FR[i]]);
  const FEDIT_SEL = [["name", "Nom"], ["shortname", "Sigle"], ["type", "Type"], ["color", "Couleur (RRGGBB)"], ["rankcount", "Nombre de grades"],
    ["rankname", "Nom d'un grade (grade nom)"], ["skin", "Skin (emplacement skin)"], ["paycheck", "Salaire (grade montant)"], ["leader", "Chef (pseudo)"]];
  const P = {
    id: { ph: "ID joueur", num: true, w: 90 },
    who: { ph: "ID ou pseudo", w: 130 },
    msg: { ph: "Message", w: 260 },
    amount: { ph: "Montant (± FCFA)", num: true, signed: true, w: 140 },
    n: { ph: "Numéro", num: true, w: 90 }
  };
  const CMDS = [
    { cat: "Staff", c: "/a", lvl: 1, d: "Écrire dans le chat du staff.", p: [P.msg] },
    { cat: "Staff", c: "/admins", lvl: 1, d: "Liste du staff connecté avec les niveaux." },
    { cat: "Staff", c: "/reports", lvl: 1, d: "Ouvre la liste des reports des joueurs." },
    { cat: "Staff", c: "/makeadmin", lvl: 7, d: "Donner un niveau staff (en dessous du tien).", p: [P.id, { sel: LVL_SEL }] },
    { cat: "Lieux", c: "/lieux", lvl: 1, d: "Liste des lieux et portes d'intérieurs." },
    { cat: "Lieux", c: "/tplieu", lvl: 1, d: "Se téléporter à un lieu (numéro de /lieux).", p: [P.n] },
    { cat: "Lieux", c: "/position", lvl: 1, d: "Affiche tes coordonnées X Y Z." },
    { cat: "Lieux", c: "/setentree", lvl: 6, d: "Placer la porte extérieure d'un intérieur à ta position.", p: [P.n] },
    { cat: "Police & prison", c: "/wanted", lvl: 1, d: "Liste des avis de recherche." },
    { cat: "Police & prison", c: "/leveravis", lvl: 1, d: "Lever l'avis de recherche d'un joueur.", p: [P.id] },
    { cat: "Police & prison", c: "/unjail", lvl: 3, d: "Libérer un joueur de prison.", p: [P.id] },
    { cat: "Police & prison", c: "/setprison", lvl: 6, d: "Placer la cellule de prison à ta position." },
    { cat: "Police & prison", c: "/setprisonexit", lvl: 6, d: "Placer la sortie de prison à ta position." },
    { cat: "Factions", c: "/factions", lvl: 1, d: "Liste des factions et de leurs membres en ligne." },
    { cat: "Factions", c: "/setfaction", lvl: 4, d: "Mettre un joueur dans une faction (0 = civil) avec un grade.", p: [P.who, { ph: "ID faction", num: true, w: 100 }, { ph: "Grade", num: true, w: 80, opt: true }] },
    { cat: "Factions", c: "/editfaction", lvl: 4, d: "Modifier une faction : nom, sigle, type, couleur, grades, skins, salaires, chef.", p: [{ ph: "ID faction", num: true, w: 100 }, { sel: FEDIT_SEL }, { ph: "Valeur", w: 200 }] },
    { cat: "Factions", c: "/createfaction", lvl: 6, d: "Créer une faction.", p: [{ sel: FTYPE_SEL }, { ph: "Nom de la faction", w: 220 }] },
    { cat: "Factions", c: "/removefaction", lvl: 6, d: "Supprimer une faction (ses membres redeviennent civils).", p: [{ ph: "ID faction", num: true, w: 100 }], danger: true },
    { cat: "Gangs", c: "/setgang", lvl: 4, d: "Mettre un joueur dans un gang avec un grade.", p: [P.who, { sel: GANG_SEL }, { sel: GRANK_SEL }] },
    { cat: "Immobilier", c: "/creermaison", lvl: 6, d: "Créer une maison à vendre, porte à ta position.", p: [{ sel: HOUSE_SEL }, { ph: "Prix (FCFA)", num: true, w: 130 }] },
    { cat: "Immobilier", c: "/allermaison", lvl: 4, d: "Se téléporter devant une maison.", p: [{ ph: "ID maison", num: true, w: 100 }] },
    { cat: "Immobilier", c: "/supprmaison", lvl: 6, d: "Supprimer une maison.", p: [{ ph: "ID maison", num: true, w: 100 }], danger: true },
    { cat: "Pêche", c: "/creerpeche", lvl: 6, d: "Créer un ponton de pêche à ta position." },
    { cat: "Pêche", c: "/supprpeche", lvl: 6, d: "Supprimer un ponton de pêche.", p: [{ ph: "ID ponton", num: true, w: 100 }], danger: true },
    { cat: "Économie & objets", c: "/givemoney", lvl: 8, d: "Donner (ou retirer avec -) de l'argent liquide.", p: [P.id, P.amount] },
    { cat: "Économie & objets", c: "/givebank", lvl: 8, d: "Créditer (ou débiter avec -) un compte bancaire.", p: [P.id, P.amount] },
    { cat: "Économie & objets", c: "/giveitem", lvl: 4, d: "Te donner un objet d'inventaire.", p: [{ sel: ITEM_SEL }, { ph: "Quantité", num: true, w: 90, opt: true }] },
    { cat: "Véhicules", c: "/v", lvl: 5, d: "Créer un véhicule (ID 400-611 ou nom) avec ses couleurs.", p: [{ ph: "Modèle (ex : infernus)", w: 170 }, { ph: "Couleur 1", num: true, w: 90, opt: true }, { ph: "Couleur 2", num: true, w: 90, opt: true }] },
    { cat: "Véhicules", c: "/vdel", lvl: 5, d: "Supprimer le véhicule que tu as créé." },
    { cat: "Véhicules", c: "/vreparer", lvl: 5, d: "Réparer ton véhicule." },
    { cat: "Staff", c: "/invisible", lvl: 1, d: "Mode invisible : personnage, pseudo et point de carte cachés aux joueurs." },
    { cat: "Monde", c: "/zonesvertes", lvl: 1, d: "Liste des zones vertes (armes rangées, pas de violence)." },
    { cat: "Monde", c: "/creerzoneverte", lvl: 6, d: "Créer une zone verte centrée sur ta position.", p: [{ ph: "Rayon (m)", num: true, w: 90 }, { ph: "Nom", w: 180 }] },
    { cat: "Monde", c: "/supprzoneverte", lvl: 6, d: "Supprimer une zone verte.", p: [{ ph: "ID zone", num: true, w: 90 }], danger: true },
    { cat: "Monde", c: "/heurereelle", lvl: 5, d: "Activer / couper l'heure réelle (Abidjan) dans le jeu." },
    { cat: "Immobilier", c: "/encheres", lvl: 0, d: "Encheres en cours (24 h, 500 000 à 50 000 000 FCFA)." },
    { cat: "Médical", c: "/blesses", lvl: 1, d: "Liste des joueurs inconscients (GPS vers le premier)." },
    { cat: "Médical", c: "/reanimer", lvl: 2, d: "Réanimer un joueur inconscient.", p: [P.id] },
    { cat: "Divers", c: "/vocal", lvl: 1, d: "État du vocal de proximité (micro)." },
    { cat: "Divers", c: "/debloquer", lvl: 1, d: "Se débloquer si on est figé." }
  ];
  function renderCmds() {
    const box = $("admCmds");
    if (!box) return;
    const q = (($("admCmdSearch") || {}).value || "").trim().toLowerCase();
    box.replaceChildren();
    const cats = [];
    CMDS.forEach((c) => { if (!cats.includes(c.cat)) cats.push(c.cat); });
    let shown = 0;
    cats.forEach((cat) => {
      const rows = CMDS.filter((c) => c.cat === cat && (!q || (c.c + " " + c.d + " " + c.cat).toLowerCase().includes(q)));
      if (!rows.length) return;
      const card = el("div", "adm-card adm-cmd-card");
      card.appendChild(el("h3", "", cat));
      rows.forEach((c) => {
        shown++;
        const locked = state.lvl < c.lvl;
        const row = el("div", "adm-cmd" + (locked ? " locked" : ""));
        const head = el("div", "adm-cmd-head");
        head.appendChild(el("code", "", c.c));
        head.appendChild(el("span", "adm-tag r" + tier(c.lvl), "Niv. " + c.lvl));
        row.appendChild(head);
        row.appendChild(el("p", "adm-hint", c.d + (locked ? " — réservé au niveau " + c.lvl + " (" + ROLES[c.lvl] + ")." : "")));
        const form = el("div", "adm-cmd-form");
        const inputs = (c.p || []).map((p) => {
          let inp;
          if (p.sel) {
            inp = el("select", "adm-input adm-select");
            p.sel.forEach(([v, label]) => { const o = el("option", "", label); o.value = v; inp.appendChild(o); });
          } else {
            inp = el("input", "adm-input");
            inp.type = "text";
            inp.placeholder = p.ph + (p.opt ? " (option)" : "");
            if (p.num) inp.inputMode = p.signed ? "text" : "numeric";
            inp.maxLength = 60;
          }
          if (p.w) inp.style.maxWidth = p.w + "px";
          inp.dataset.opt = p.opt ? "1" : "";
          form.appendChild(inp);
          return inp;
        });
        const run = el("button", "adm-btn " + (c.danger ? "danger" : "primary"), "Exécuter");
        run.type = "button";
        run.addEventListener("click", () => {
          const parts = [c.c];
          for (let i = 0; i < inputs.length; i++) {
            const v = String(inputs[i].value || "").trim().replace(/[\s]+/g, " ");
            if (!v) { if (inputs[i].dataset.opt) continue; toast("Remplis : " + (c.p[i].ph || "le champ"), false); return; }
            parts.push(v);
          }
          const line = parts.join(" ");
          const go = () => { send("cmd", 0, line); inputs.forEach((x) => { if (x.tagName === "INPUT") x.value = ""; }); };
          if (c.danger) openModal({ title: "Exécuter " + c.c + " ?", text: line }, { fn: go }); else go();
        });
        form.appendChild(run);
        row.appendChild(form);
        card.appendChild(row);
      });
      box.appendChild(card);
    });
    if (!shown) box.appendChild(el("div", "adm-empty", "Aucune commande ne correspond à ta recherche."));
  }

  /* ---- Onglet Staff : staff en ligne, chat staff, commandes ---- */
  function scrollChat() {
    const log = document.getElementById("admChatLog");
    if (log) log.scrollTop = log.scrollHeight;
  }
  function renderChatLog(log) {
    log.replaceChildren();
    if (!state.chat.length) log.appendChild(el("div", "adm-empty", "Aucun message pour le moment."));
    state.chat.forEach((line) => {
      const m = String(line).match(/^(\d\d:\d\d) ([^:]+) : (.*)$/);
      const row = el("div", "adm-chat-line");
      if (m) { row.appendChild(el("small", "", m[1])); row.appendChild(el("b", "", m[2])); row.appendChild(document.createTextNode(m[3])); }
      else row.textContent = line;
      log.appendChild(row);
    });
    scrollChat();
  }
  function renderStaff() {
    const box = $("admStaff");
    if (!box) return;
    const keepDraft = document.getElementById("admChatInput");
    const draft = keepDraft ? keepDraft.value : "";
    box.replaceChildren();

    box.appendChild(serverCard("Chat du staff", 1, (c) => {
      const log = el("div", "adm-chat");
      log.id = "admChatLog";
      c.appendChild(log);
      renderChatLog(log);
      const input = inlineForm(c, "Message au staff (aussi /a en jeu)", "Envoyer", (v) => send("staffchat", 0, v));
      input.id = "admChatInput";
      input.value = draft;
      input.addEventListener("keydown", (e) => { if (e.key === "Enter" && input.value.trim()) { send("staffchat", 0, input.value.trim()); input.value = ""; } });
    }));

    box.appendChild(serverCard("Staff en ligne", 1, (c) => {
      const list = el("div", "adm-lockers");
      const staff = state.players.filter((p) => Number(p.l) > 0);
      if (!staff.length) list.appendChild(el("div", "adm-empty", "Personne."));
      staff.forEach((p) => {
        const row = el("div", "adm-locker");
        const info = el("div", "adm-locker-info");
        info.appendChild(el("b", "", p.n + (p.id === state.me ? " (toi)" : "")));
        info.appendChild(el("small", "", "ID " + p.id + " · niv. " + p.l + " · " + roleName(p.l)));
        row.appendChild(info);
        if (p.id !== state.me) {
          const go = el("button", "adm-btn", "Y aller");
          go.type = "button";
          go.addEventListener("click", () => send("goto", p.id, ""));
          row.appendChild(go);
        }
        list.appendChild(row);
      });
      c.appendChild(list);
      c.appendChild(el("p", "adm-hint", "Rôles : onglet Joueurs → « Définir le rôle » (niveau 7+), ou /makeadmin <id> <0-10>."));
    }));

    box.appendChild(serverCard("Journal du staff", 1, (c) => {
      const list = el("div", "adm-chat adm-log");
      if (!state.log.length) list.appendChild(el("div", "adm-empty", "Aucune action enregistrée."));
      state.log.forEach((row) => {
        const [ts, who, text, ok, lv] = row;
        const d = new Date((Number(ts) || 0) * 1000);
        const hh = String(d.getHours()).padStart(2, "0") + ":" + String(d.getMinutes()).padStart(2, "0");
        const line = el("div", "adm-chat-line" + (Number(ok) ? "" : " ko"));
        line.appendChild(el("small", "", hh));
        line.appendChild(el("b", "", who));
        line.appendChild(el("span", "adm-log-lv", "niv. " + lv));
        line.appendChild(document.createTextNode(text + (Number(ok) ? "" : " (refusé)")));
        list.appendChild(line);
      });
      c.appendChild(list);
    }));

    box.appendChild(serverCard("Les 10 niveaux du staff", 1, (c) => {
      const list = el("div", "adm-levels");
      for (let i = 1; i <= 10; i++) {
        const row = el("div", "adm-level" + (i === state.lvl ? " me" : ""));
        row.appendChild(el("b", "adm-tag r" + tier(i), String(i)));
        const t = el("div", "adm-locker-info");
        t.appendChild(el("b", "", ROLES[i] + (i === state.lvl ? " (toi)" : "")));
        t.appendChild(el("small", "", ROLE_DESC[i]));
        row.appendChild(t);
        list.appendChild(row);
      }
      c.appendChild(list);
      c.appendChild(el("p", "adm-hint", "Chaque niveau a aussi tous les droits des niveaux en dessous."));
    }));

    box.appendChild(serverCard("Exécuter une commande", 1, (c) => {
      c.appendChild(el("p", "adm-hint", "N'importe quelle commande du serveur, comme dans le chat. Le résultat s'affiche dans le chat du jeu."));
      inlineForm(c, "/commande arguments", "Exécuter", (v) => send("cmd", 0, v.charAt(0) === "/" ? v : "/" + v));
      chips(c, [["/admins", "/admins"], ["/lieux", "/lieux"], ["/factions", "/factions"], ["/tresor", "/tresor"], ["/position", "/position"]], (v) => send("cmd", 0, v));
    }));

    box.appendChild(serverCard("Lieux & portes d'intérieurs", 1, (c) => {
      const list = el("div", "adm-lockers");
      PLACES.forEach((pl, i) => {
        const row = el("div", "adm-locker");
        const info = el("div", "adm-locker-info");
        info.appendChild(el("b", "", pl.n));
        info.appendChild(el("small", "", pl.entry ? "Intérieur · entrée " + pl.entry : "Mapping"));
        row.appendChild(info);
        const go = el("button", "adm-btn", "Y aller");
        go.type = "button";
        go.addEventListener("click", () => send("tplace", i + 1, ""));
        row.appendChild(go);
        if (pl.entry) {
          const door = el("button", "adm-btn primary", "Porte ici");
          door.type = "button";
          door.addEventListener("click", () => openModal({ title: "Placer la porte de « " + pl.n + " » ?", text: "La porte extérieure sera mise à ta position actuelle." }, { fn: () => send("setentree", pl.entry, "") }));
          row.appendChild(door);
        }
        list.appendChild(row);
      });
      c.appendChild(list);
    }));

    box.appendChild(serverCard("Prison", 6, (c) => {
      chips(c, [["Cellule ici", "setprison"], ["Sortie de prison ici", "prisonexit"]], (v) => send(v, 0, ""));
    }));

    box.appendChild(serverCard("Créer / supprimer une faction", 6, (c) => {
      const b = el("button", "adm-btn primary", "Créer une faction");
      b.type = "button";
      b.style.marginTop = "6px";
      b.addEventListener("click", () => {
        openModal({ title: "Type de faction", text: "Choisis le type :", choices: FAC_TYPE_FR }, { fn: (pick) => {
          const type = FAC_TYPES[Number(pick) || 0];
          openModal({ title: "Nom de la faction", text: "Nom affiché (ex : Gouvernement de San Andreas) :", value: "" }, { fn: (name) => {
            if (name) send("createfac", 0, type + " " + name);
          } });
        } });
      });
      c.appendChild(b);
      inlineForm(c, "ID de la faction à supprimer", "Supprimer", (v) => {
        openModal({ title: "Supprimer la faction " + v + " ?", text: "Ses membres redeviennent civils." }, { fn: () => send("removefac", Number(v) || 0, "") });
      }, { number: true });
    }));
    if (state.tab === "staff") scrollChat();
  }

  /* ---- Onglet Reports : support des joueurs ---- */
  function scrollRep() {
    const box = document.getElementById("admRepMsgs");
    if (box) box.scrollTop = box.scrollHeight;
  }
  function repBadge() {
    const waiting = state.reports.filter((r) => Number(r.st) !== 2).length;
    const unread = state.reports.reduce((n, r) => n + (Number(r.u) || 0), 0);
    const b = $("admRepBadge");
    const n = Math.max(waiting, unread);
    b.textContent = String(n);
    b.classList.toggle("hidden", n === 0);
  }
  function renderRepList() {
    const list = $("admRepList");
    list.replaceChildren();
    const rows = state.reports.filter((r) => {
      if (state.repFilter === "wait") return Number(r.st) !== 2;
      if (state.repFilter === "mine") return Number(r.st) === 2 && r.by === state.myName;
      return true;
    });
    if (!rows.length) { list.appendChild(el("div", "adm-empty", state.reports.length ? "Aucun report dans ce filtre." : "Aucun report ouvert. Tout est calme !")); return; }
    rows.forEach((r) => {
      const claimed = Number(r.st) === 2;
      const b = el("button", "adm-row adm-rep-row" + (claimed ? "" : " wait") + (r.id === state.repView ? " sel" : ""));
      b.type = "button";
      b.appendChild(el("div", "adm-av", "#" + r.num));
      const main = el("div", "adm-row-main");
      main.appendChild(el("div", "adm-row-name", r.n + " (" + r.id + ")"));
      const mins = Number(r.m) || 0;
      main.appendChild(el("div", "adm-row-sub", (REP_CATS[Number(r.c)] || "Autre") + (r.tg ? " · " + r.tg : "") + " · il y a " + (mins < 1 ? "moins d'1 min" : mins + " min")));
      b.appendChild(main);
      if (Number(r.u) > 0) b.appendChild(el("span", "adm-rep-unread", String(r.u)));
      b.appendChild(el("span", "adm-rep-state" + (claimed ? " on" : ""), claimed ? (r.by || "Pris") : "Attente"));
      b.addEventListener("click", () => { state.repView = r.id; renderReports(); send("repview", r.id, ""); });
      list.appendChild(b);
    });
  }
  function renderRepDetail() {
    const box = $("admRepDetail");
    const keep = document.getElementById("admRepInput");
    const draft = keep ? keep.value : "";
    box.replaceChildren();
    const t = state.thread && state.thread.id === state.repView ? state.thread : null;
    if (!t) { box.appendChild(el("div", "adm-empty", state.reports.length ? "Choisis un report dans la liste pour lire et répondre." : "Les reports des joueurs apparaîtront ici en direct.")); return; }
    const claimed = Number(t.st) === 2;
    const mine = claimed && t.by === state.myName;

    const head = el("div", "adm-card adm-rep-head");
    const who = el("div", "adm-who");
    who.appendChild(el("div", "adm-av", "#" + t.num));
    const info = el("div");
    info.appendChild(el("div", "adm-who-name", t.n + " (ID " + t.id + ")"));
    info.appendChild(el("div", "adm-who-sub", (REP_CATS[Number(t.c)] || "Autre") + (t.tg ? " · signale " + t.tg : "") + " · " + (claimed ? "pris en charge par " + (t.by || "?") : "en attente")));
    who.appendChild(info);
    head.appendChild(who);
    const acts = el("div", "adm-rep-actions");
    const mk = (label, cls, fn) => { const b = el("button", "adm-btn " + cls, label); b.type = "button"; b.addEventListener("click", fn); acts.appendChild(b); return b; };
    if (!mine) mk(claimed ? "Reprendre" : "Prendre en charge", "primary", () => send("repclaim", t.id, ""));
    mk("Aller vers", "", () => send("goto", t.id, ""));
    mk("Amener", "", () => send("bring", t.id, ""));
    mk("Fermer", "danger", () => openModal({ title: "Fermer le report #" + t.num + " ?", text: "Le joueur sera prévenu que son report est fermé." }, { fn: () => send("repclose", t.id, "") }));
    head.appendChild(acts);
    box.appendChild(head);

    const card = el("div", "adm-card adm-rep-thread");
    const msgs = el("div", "adm-rep-msgs");
    msgs.id = "admRepMsgs";
    (Array.isArray(t.msgs) ? t.msgs : []).forEach((m) => {
      const f = Number(m.f);
      const row = el("div", "adm-rep-m" + (f === 1 ? " staff" : f === 2 ? " sys" : ""));
      if (f !== 2) {
        const w = el("span", "who", f === 1 ? (m.n === state.myName ? "Toi" : m.n) : m.n);
        w.appendChild(el("time", "", m.h || ""));
        row.appendChild(w);
      }
      row.appendChild(el("p", "", m.t || ""));
      msgs.appendChild(row);
    });
    card.appendChild(msgs);
    const quick = el("div", "adm-rep-quick");
    REP_QUICK.forEach((q) => {
      const c = el("button", "adm-chip", q.length > 26 ? q.slice(0, 24) + "…" : q);
      c.type = "button";
      c.addEventListener("click", () => send("repreply", t.id, q));
      quick.appendChild(c);
    });
    card.appendChild(quick);
    const input = inlineForm(card, claimed && !mine ? "Répondre (tu reprends le report)" : "Répondre au joueur…", "Envoyer", (v) => send("repreply", t.id, v));
    input.id = "admRepInput";
    input.value = draft;
    input.addEventListener("keydown", (e) => { if (e.key === "Enter" && input.value.trim()) { send("repreply", t.id, input.value.trim()); input.value = ""; } });
    box.appendChild(card);
    scrollRep();
  }
  function renderReports() {
    repBadge();
    if (state.tab !== "reports") return;
    renderRepList();
    renderRepDetail();
  }
  Array.prototype.forEach.call(document.querySelectorAll("[data-rf]"), (b) => b.addEventListener("click", () => {
    state.repFilter = b.dataset.rf;
    Array.prototype.forEach.call(document.querySelectorAll("[data-rf]"), (x) => x.classList.toggle("on", x === b));
    renderRepList();
  }));

  /* ------------------------------------------------------------ evenements */
  Array.prototype.forEach.call(document.querySelectorAll(".adm-tab"), (b) => b.addEventListener("click", () => setTab(b.dataset.tab)));
  $("admSearch").addEventListener("input", (e) => { state.filter = e.target.value.trim(); renderList(); });
  $("admCmdSearch").addEventListener("input", () => renderCmds());
  $("admRefresh").addEventListener("click", () => send("refresh", 0, ""));
  $("admClose").addEventListener("click", () => {
    if (bridge && typeof bridge.close === "function") { try { bridge.close(); } catch (_) {} }
  });

  window.AdminApp = {
    openFromNative(raw) {
      setTab(state.tab === "reports" ? "reports" : "players");
      apply(parse(raw), true);
      send("replist", 0, "");
    },
    onReports(raw) {
      const d = parse(raw);
      const before = state.reports.length;
      state.reports = Array.isArray(d.reports) ? d.reports : [];
      state.repView = d.view == null ? -1 : Number(d.view);
      state.thread = d.thread && typeof d.thread === "object" ? d.thread : null;
      renderReports();
      if (d.msg) toast(d.msg, d.ok === 1 || d.ok === true);
      else if (state.tab !== "reports" && state.reports.length > before) toast("Nouveau report d'un joueur : onglet Reports.", true);
    },
    onUpdateFromNative(raw) {
      const d = parse(raw);
      // donnees supplementaires (factions, gangs, bannissements, journal) : envoyees a part par le serveur
      if (d.x) { applyExtra(d); return; }
      // message du chat staff en direct : seule la liste des messages est envoyee
      if (Array.isArray(d.chat) && d.lvl === undefined) {
        state.chat = d.chat;
        const log = document.getElementById("admChatLog");
        if (log) renderChatLog(log);
        return;
      }
      apply(d, false);
      if (d.msg) toast(d.msg, d.ok === 1 || d.ok === true);
    }
  };

  /* Apercu navigateur (hors jeu) : donnees fictives */
  if (!bridge) {
    window.AdminApp.openFromNative(JSON.stringify({
      lvl: 10, role: "Fondateur", me: 0, count: 4,
      chat: ["13:52 Carl_Johnson : quelqu un peut checker le spawn ?", "13:53 Anzo_Smith : oui j y vais"],
      players: [
        { id: 0, n: "Anzo_Smith", h: 100, a: 100, p: 34, l: 10, f: 0, r: 1 },
        { id: 1, n: "Sir_Sugo", h: 76, a: 0, p: 58, l: 0, f: 1, r: 2 },
        { id: 2, n: "Carl_Johnson", h: 100, a: 40, p: 41, l: 4, f: 0, r: 1 },
        { id: 3, n: "Marie_Dupont", h: 52, a: 0, p: 90, l: 7, f: 2, r: 4 }
      ],
      lockers: [{ i: 1, f: 1, x: 1555, y: -1675, z: 16 }, { i: 2, f: 1, x: 1481, y: -1771, z: 18 }]
    }));
    window.AdminApp.onUpdateFromNative(JSON.stringify({ x: "fac", fac: [
      { id: 1, n: "Police de Los Santos", t: 1, c: "#3366ff", on: 1, rk: ["Cadet", "Officier", "Sergent", "Lieutenant", "Capitaine", "Commandant"] },
      { id: 2, n: "Hôpital All Saints", t: 2, c: "#ff5577", on: 1, rk: ["Stagiaire", "Infirmier", "Médecin", "Chef de service"] },
      { id: 3, n: "Gouvernement", t: 4, c: "#c9a227", on: 0, rk: ["Agent", "Conseiller", "Maire"] } ] }));
    window.AdminApp.onUpdateFromNative(JSON.stringify({ x: "gang", season: 1, gangs: [
      { id: 1, n: "Cold Blooded", tag: "CB", c: "#4da8ff", pts: 1840, m: 14, on: 3, z: 3 }, { id: 2, n: "Jungle Boys", tag: "JB", c: "#3ddc97", pts: 1798, m: 12, on: 1, z: 3 },
      { id: 3, n: "Silent Killers", tag: "SK", c: "#ff4d4d", pts: 1265, m: 11, on: 0, z: 2 }, { id: 4, n: "Royal Vipers", tag: "RV", c: "#a855f7", pts: 934, m: 9, on: 0, z: 2 } ],
      online: [[1, 2, 3]] }));
    window.AdminApp.onUpdateFromNative(JSON.stringify({ x: "ban", now: 1800000000, bans: [[4, "Tricheur_Pro", "Aimbot", "Anzo_Smith", 0, 1799990000], [3, "Kevin_Spam", "Spam", "Marie_Dupont", 1800259200, 1799900000]] }));
    window.AdminApp.onUpdateFromNative(JSON.stringify({ x: "log", now: 1800000000, log: [[1800000000, "Marie_Dupont", "setgang Sir_Sugo 2 3", 1, 7], [1799999000, "Carl_Johnson", "ban Kevin_Spam 3 Spam", 0, 4]] }));
    window.AdminApp.onReports(JSON.stringify({
      reports: [
        { id: 1, num: 3, n: "Sir_Sugo", c: 2, st: 1, by: "", u: 1, m: 2, tg: "Carl_Johnson" },
        { id: 3, num: 4, n: "Marie_Dupont", c: 1, st: 2, by: "Anzo_Smith", u: 0, m: 7, tg: "" }
      ],
      view: 3,
      thread: { id: 3, num: 4, n: "Marie_Dupont", c: 1, st: 2, by: "Anzo_Smith", tg: "", msgs: [
        { f: 0, n: "Marie_Dupont", t: "Ma voiture a disparu après le reboot", h: "17:58" },
        { f: 2, n: "Support", t: "Anzo_Smith prend en charge ton report.", h: "17:59" },
        { f: 1, n: "Anzo_Smith", t: "Salut ! Je regarde ça, c'était quel modèle ?", h: "17:59" },
        { f: 0, n: "Marie_Dupont", t: "Une Sultan bleue", h: "18:00" }
      ] }
    }));
  }
  try { if (bridge && typeof bridge.ready === "function") bridge.ready(); } catch (_) {}
})();
