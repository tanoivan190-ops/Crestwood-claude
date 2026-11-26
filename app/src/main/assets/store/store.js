/* ==========================================================================
   Crestwood Store [v30]
   Serveur -> page : StoreApp.openFromNative(json) / StoreApp.onUpdateFromNative(json)
       {"name":"Prenom Nom","cc":3500,"money":120000,"orders":0,"waves":0,"ok":1,"msg":""}
   Page -> serveur (pont Java "SampStore") :
       SampStore.action("buy", "<article> [extra]")   SampStore.action("wave", "<pack> <reference>")
       SampStore.action("refresh", "")  SampStore.close()  SampStore.ready()  SampStore.openUrl(url)
   Les prix sont re-verifies par le serveur (include/Store.inc) : garder les memes ids et prix.
   ========================================================================== */
(function () {
  "use strict";
  const $ = (s) => document.querySelector(s);
  const bridge = window.SampStore || null;
  const WAVE = { link: "https://pay.wave.com/m/M_ci_6dhSkjElX8cm/c/ci/", phone: "+225 07 02 95 78 51" };

  let P = { name: "John Crest", cc: 3500, money: 275000, orders: 0, waves: 0 };

  // ---------- icones (traits, 24x24) ----------
  const I = {
    chevL: '<path d="M20 12a8 8 0 1 1-2.3-5.6"/><path d="M20 4v4h-4"/>', chevR: '<path d="M4 12a8 8 0 1 0 2.3-5.6"/><path d="M4 4v4h4"/>',
    car: '<path d="M5 16v2.5M19 16v2.5M3.5 16h17v-4l-2-5.2a1.5 1.5 0 0 0-1.4-.8H6.9a1.5 1.5 0 0 0-1.4.8L3.5 12z"/><path d="M3.5 12h17M7 14h.01M17 14h.01"/>',
    heli: '<path d="M3 5h18M12 5v3"/><path d="M6 13a5 5 0 0 1 5-5h3a4 4 0 0 1 4 4v1H6z"/><path d="M18 12h3M8 13v3h8v-3M7 19h11"/>',
    boat: '<path d="M3 15h18l-2.5 4h-13z"/><path d="M6 15V9h8l3 6M10 9V5"/>',
    bike: '<circle cx="6" cy="16" r="3.2"/><circle cx="18" cy="16" r="3.2"/><path d="M6 16l4-6h5l3 6M10 10 8.5 7H6M14 7h2.5"/>',
    truck: '<path d="M2.5 6h11v10h-11zM13.5 9.5h4.2L21 13v3h-7.5"/><circle cx="6.5" cy="17.5" r="1.8"/><circle cx="17" cy="17.5" r="1.8"/>',
    anchor: '<circle cx="12" cy="5" r="2"/><path d="M12 7v13M8 10h8M4.5 13a7.5 7.5 0 0 0 15 0"/>',
    shield: '<path d="M12 3 20 6v5.5c0 5-3.4 8.6-8 9.9-4.6-1.3-8-4.9-8-9.9V6z"/><path d="m9 12 2 2 4-4"/>',
    cash: '<rect x="2.5" y="6" width="19" height="12" rx="2.5"/><circle cx="12" cy="12" r="2.6"/><path d="M6 9.5v5M18 9.5v5"/>',
    bag: '<path d="M5 8h14l-1.2 11a2 2 0 0 1-2 1.8H8.2a2 2 0 0 1-2-1.8z"/><path d="M9 8V6a3 3 0 0 1 6 0v2"/>',
    med: '<rect x="3" y="7" width="18" height="13" rx="2.5"/><path d="M9 7V5h6v2M12 10.5v6M9 13.5h6"/>',
    drink: '<path d="M7 3h10l-1.3 17a1.5 1.5 0 0 1-1.5 1.4H9.8a1.5 1.5 0 0 1-1.5-1.4z"/><path d="M7.4 8h9.2"/>',
    id: '<rect x="3" y="5" width="18" height="14" rx="3"/><circle cx="9" cy="11" r="2.2"/><path d="M5.8 16c.6-1.6 1.8-2.4 3.2-2.4s2.6.8 3.2 2.4M14.5 10h4M14.5 13.5h3"/>',
    plate: '<rect x="2.5" y="7" width="19" height="10" rx="2.5"/><path d="M6.5 12h3M11.5 12h1M14.5 12h3"/>',
    phone: '<rect x="6.5" y="2.5" width="11" height="19" rx="3"/><path d="M10.5 18.5h3"/>',
    warn: '<path d="M12 3 2.5 20h19z"/><path d="M12 10v4.5M12 17h.01"/>',
    star: '<path d="m12 3 2.7 5.6 6.1.9-4.4 4.3 1 6.1L12 17l-5.4 2.9 1-6.1-4.4-4.3 6.1-.9z"/>',
    check: '<path d="M4 12.5 9.5 18 20 6.5"/>',
    alert: '<circle cx="12" cy="12" r="9"/><path d="M12 7.5v5.5M12 16.5h.01"/>',
    info: '<circle cx="12" cy="12" r="9"/><path d="M12 11v5.5M12 7.6h.01"/>',
    wave: '<path d="M2 12c2-3 4-3 6 0s4 3 6 0 4-3 6 0"/>',
    lock: '<rect x="5" y="10.5" width="14" height="10" rx="2.5"/><path d="M8 10.5V8a4 4 0 0 1 8 0v2.5"/>',
    x: '<path d="M6 6l12 12M18 6 6 18"/>',
  };
  const svg = (k, cls = "") => `<svg viewBox="0 0 24 24" class="${cls}" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">${I[k] || I.star}</svg>`;
  const coin = (c) => `<svg class="coin"><use href="#${c === "usd" ? "usd" : "cc"}"/></svg>`;
  const fmt = (n) => Math.round(n).toLocaleString("fr-FR").replace(/ | /g, " ");
  const ptxt = (n, c) => c === "usd" ? fmt(n) + " $" : c === "fcfa" ? fmt(n) + " FCFA" : fmt(n) + " CC";
  const tag = (it) => it.cur === "fcfa" ? `<span class="tag fcfa"><b>${fmt(it.price)} F</b></span>`
    : `<span class="tag ${it.cur === "usd" ? "usd" : ""}">${coin(it.cur)}<b>${fmt(it.price)}</b>${it.old ? `<s>${fmt(it.old)}</s>` : ""}</span>`;
  const RNAME = ["", "Commun", "Rare", "Epique", "Legendaire"];

  // ---------- catalogue (identique au serveur) ----------
  const VEH = { 411: "Infernus", 451: "Turismo", 541: "Bullet", 487: "Maverick", 469: "Sparrow", 593: "Dodo", 493: "Jetmax", 446: "Squalo", 452: "Speeder",
    429: "Banshee", 415: "Cheetah", 477: "ZR-350", 409: "Stretch", 580: "Stafford", 507: "Elegant", 562: "Elegy", 560: "Sultan", 559: "Jester", 565: "Flash",
    495: "Sandking", 579: "Huntley", 500: "Mesa", 489: "Rancher", 522: "NRG-500", 521: "FCR-900", 461: "PCJ-600", 468: "Sanchez",
    536: "Blade", 567: "Savanna", 412: "Voodoo", 575: "Broadway" };
  const CAT = {
    coins: [
      { id: "cc_500", cc: 500, bonus: 0, price: 1000, cur: "fcfa", rar: 1, st: [3] },
      { id: "cc_1100", cc: 1100, bonus: 100, price: 2000, cur: "fcfa", rar: 1, st: [4, 3] },
      { id: "cc_3000", cc: 3000, bonus: 500, price: 5000, cur: "fcfa", rar: 2, st: [5, 7, 4] },
      { id: "cc_6500", cc: 6500, bonus: 1500, price: 10000, cur: "fcfa", rar: 3, st: [5, 8, 6, 4] },
      { id: "cc_14000", cc: 14000, bonus: 4000, price: 20000, cur: "fcfa", rar: 4, st: [6, 9, 10, 8, 5], best: 1 },
    ],
    money: [
      { id: "mo_100k", name: "100 000 $", amt: 100000, price: 100, cur: "cc", rar: 1, st: [2] },
      { id: "mo_500k", name: "500 000 $", amt: 500000, price: 450, cur: "cc", rar: 1, st: [3, 2] },
      { id: "mo_1m", name: "1 000 000 $", amt: 1000000, price: 850, cur: "cc", rar: 2, st: [4, 3, 2] },
      { id: "mo_5m", name: "5 000 000 $", amt: 5000000, price: 4000, cur: "cc", rar: 3, st: [5, 4, 4, 3], best: 1 },
    ],
    vehicles: [
      { id: "pv_super", name: "Supercar", sub: "Le top de la vitesse.", v: [411, 451, 541], icon: "car", price: 4500, cur: "cc", rar: 4 },
      { id: "pv_air", name: "Aerien", sub: "Helicoptere ou avion leger.", v: [487, 469, 593], icon: "heli", price: 6000, cur: "cc", rar: 4 },
      { id: "pv_sea", name: "Nautique", sub: "Bateaux rapides.", v: [493, 446, 452], icon: "boat", price: 3500, cur: "cc", rar: 3 },
      { id: "pv_sport", name: "Sport", sub: "Nerveuses en ville.", v: [429, 415, 477], icon: "car", price: 3200, cur: "cc", rar: 3 },
      { id: "pv_prest", name: "Prestige", sub: "Arrive avec classe.", v: [409, 580, 507], icon: "car", price: 3000, cur: "cc", rar: 3 },
      { id: "pv_tuning", name: "Tuning", sub: "Bases Wheel Arch Angels.", v: [562, 560, 559, 565], icon: "car", price: 2800, cur: "cc", rar: 2 },
      { id: "pv_4x4", name: "Tout-terrain", sub: "Pour la campagne.", v: [495, 579, 500, 489], icon: "truck", price: 2200, cur: "cc", rar: 2 },
      { id: "pv_moto", name: "Moto", sub: "Se faufiler partout.", v: [522, 521, 461, 468], icon: "bike", price: 1900, cur: "cc", rar: 2 },
      { id: "pv_low", name: "Lowrider", sub: "Hydrauliques old-school.", v: [536, 567, 412, 575], icon: "car", price: 1600, cur: "cc", rar: 1 },
    ],
    licences: [
      { id: "li_truck", name: "Poids lourds", sub: "Camions et bus.", icon: "truck", price: 60000, cur: "usd", rar: 2 },
      { id: "li_boat", name: "Permis navire", sub: "Tous les bateaux.", icon: "anchor", price: 80000, cur: "usd", rar: 2 },
      { id: "li_gun", name: "Port d'arme", sub: "Licence officielle.", icon: "shield", price: 150000, cur: "usd", rar: 3 },
    ],
    kits: [
      { id: "kt_surv", name: "Kit de survie", sub: "5 hamburgers, 5 eaux, 2 bandages.", icon: "bag", price: 120, cur: "cc", rar: 1 },
      { id: "kt_heal", name: "Kit de soins", sub: "6 bandages.", icon: "med", price: 90, cur: "cc", rar: 1 },
      { id: "kt_drink", name: "Pack boissons", sub: "6 eaux Kirene, 4 Vody.", icon: "drink", price: 80, cur: "cc", rar: 1 },
    ],
    services: [
      { id: "sv_name", name: "Changement de nom", sub: "Nouveau nom Prenom_Nom.", icon: "id", price: 500, cur: "cc", rar: 2, input: "name" },
      { id: "sv_plate", name: "Plaque perso", sub: "Sur ton vehicule.", icon: "plate", price: 350, cur: "cc", rar: 1, input: "plate" },
      { id: "sv_phone", name: "Numero perso", sub: "De 4 a 7 chiffres.", icon: "phone", price: 400, cur: "cc", rar: 1, input: "phone" },
      { id: "sv_warn", name: "Retrait d'avertissement", sub: "Retire 1 avertissement.", icon: "warn", price: 800, cur: "cc", rar: 3 },
    ],
    squid: [
      { key: "frontman", name: "Front Man", sub: "Le maitre du jeu." },
      { key: "poupee", name: "La Poupee", sub: "Robe orange, regard qui juge." },
      { key: "carre", name: "Garde Carre", sub: "Chef des gardes." },
      { key: "triangle", name: "Garde Triangle", sub: "Garde arme." },
      { key: "rond", name: "Garde Rond", sub: "Garde de base." },
      { key: "456", name: "Joueur 456", sub: "Survetement vert." },
      { key: "218", name: "Joueur 218", sub: "Survetement vert." },
      { key: "067", name: "Joueuse 067", sub: "Survetement vert." },
    ],
  };
  CAT.coins.forEach((c) => { c.name = fmt(c.cc) + " CC"; c.sub = c.bonus ? `Dont ${fmt(c.bonus)} CC offerts` : "Recharge simple"; });

  const TABS = [
    ["home", "Accueil"], ["coins", "CrestCoins"], ["money", "Argent"], ["vehicles", "Vehicules"],
    ["licences", "Permis"], ["kits", "Kits"], ["services", "Services"], ["squid", "Squid Game", "sq"],
  ];
  let tab = "home";

  // ---------- rendu ----------
  function stackArt(st, c) {
    // pieces en eventail : plus le pack est gros, plus il y a de pieces
    const n = st.length, mid = (n - 1) / 2;
    return `<div class="fan">${st.map((_, i) => {
      const d = Math.abs(i - mid), s = 1 - d * 0.14;
      return `<svg style="left:${50 + (i - mid) * 17}%;transform:translateX(-50%) scale(${s.toFixed(2)});z-index:${10 - Math.round(d * 2)}"><use href="#${c}"/></svg>`;
    }).join("")}</div>`;
  }
  function art(it) {
    if (it.v || it.icon) return `<div class="art">${svg(it.icon || "car", "ic")}${it.n ? `<span class="n">${it.n}</span>` : ""}</div>`;
    return `<div class="art">${svg("star", "ic")}</div>`;
  }
  function card(it, extra = "") {
    const corner = it.input || it.id === "sv_warn" ? `<span class="corner alt">Staff</span>` : it.rar === 4 ? `<span class="corner">Top</span>` : "";
    return `<button class="it r${it.rar}" onclick="Store.buy('${it.id}')">${corner}${art(it)}
      <div class="meta"><div class="rar">${RNAME[it.rar]}</div><h3 class="d">${it.name}</h3><div class="sub">${it.sub || ""}</div>${extra}${tag(it)}</div></button>`;
  }
  function pack(c, money) {
    return `<button class="pack ${money ? "money" : ""} ${c.best ? "best" : ""}" onclick="Store.buy('${c.id}')">
      ${c.best ? `<span class="best-t">${money ? "Meilleur prix" : "Meilleure offre"}</span>` : ""}
      <div class="ill">${stackArt(c.st, money ? "usd" : "cc")}</div>
      <div class="body">
        <div class="amt">${money ? fmt(c.amt) : fmt(c.cc)}<small>${money ? "$" : "CC"}</small></div>
        <div class="bonus ${money || !c.bonus ? "no" : ""}">${money ? "Argent liquide" : c.sub}</div>
        <div class="pr">${money ? `<span>Prix</span><b>${fmt(c.price)} CC</b>` : `<span>${svg("wave")} Wave</span><b>${fmt(c.price)} F</b>`}</div>
      </div></button>`;
  }
  function head(title, sub, cls = "") { return `<div class="head"><h2 class="d ${cls}">${title}</h2><p>${sub}</p></div>`; }

  function render() {
    $("#nav").innerHTML = TABS.map(([k, l, c]) => `<button class="${c || ""} ${k === tab ? "on" : ""}" onclick="Store.go('${k}')">${l}</button>`).join("");
    let h = "";
    if (tab === "home") {
      h = `<div class="hero">
          <div><span class="kick">Crestwood Store</span><h1 class="d">Recharge<br>tes CrestCoins</h1>
          <p>Paie avec Wave, envoie la reference : le staff verifie et tes CC arrivent. Ensuite vehicules, argent, permis et services en un clic.</p>
          <div class="cta"><button class="btn main" onclick="Store.go('coins')">Voir les packs</button><button class="btn ghost" onclick="Store.go('vehicles')">${svg("car")} Vehicules</button></div></div>
          <svg class="big"><use href="#cc"/></svg></div>
        ${head("En vedette", "Les plus demandes")}
        <div class="grid">${card(CAT.vehicles[0], chips(CAT.vehicles[0]))}${card(CAT.vehicles[1], chips(CAT.vehicles[1]))}${card(CAT.kits[0])}${card(CAT.licences[2])}</div>
        <button class="sqstrip" onclick="Store.go('squid')"><img src="img/squid_banner.jpg" alt="">
          <div><h3 class="d">Squid Game<em>Collection de skins - bientot</em></h3><p>8 skins exclusifs arrivent dans la boutique.</p></div></button>`;
    } else if (tab === "coins") {
      h = `${head("CrestCoins", "Paiement Wave - credites apres verification du staff")}
        <div class="packs">${CAT.coins.map((c) => pack(c, false)).join("")}</div>
        <div class="note">${svg("info")}<p><b>Comment ca marche :</b> choisis un pack, paie avec Wave, puis colle la reference de la transaction. Le staff verifie et credite tes CC. Un souci : ${WAVE.phone}.${P.waves ? ` <b>${P.waves} paiement(s) en verification.</b>` : ""}</p></div>`;
    } else if (tab === "money") {
      h = `${head("Argent", "Echange tes CrestCoins contre de l'argent liquide")}<div class="packs m">${CAT.money.map((c) => pack(c, true)).join("")}</div>`;
    } else if (tab === "vehicles") {
      h = `${head("Vehicules", "Choisis ton modele : il est livre dans ton garage")}<div class="grid w">${CAT.vehicles.map((v) => card(v, chips(v))).join("")}</div>`;
    } else if (tab === "licences") {
      h = `${head("Permis", "Payes en argent ($), ajoutes a tes papiers")}<div class="grid">${CAT.licences.map((x) => card(x)).join("")}</div>`;
    } else if (tab === "kits") {
      h = `${head("Kits", "Ranges directement dans ton sac")}<div class="grid">${CAT.kits.map((x) => card(x)).join("")}</div>`;
    } else if (tab === "services") {
      h = `${head("Services", "Faits par le staff" + (P.orders ? ` - ${P.orders} commande(s) en cours` : ""))}<div class="grid">${CAT.services.map((x) => card(x)).join("")}</div>`;
    } else if (tab === "squid") {
      h = `${head("Squid Game", "Collection de skins - bientot disponible", "")}
        <div class="grid sq">${CAT.squid.map((s) => `<div class="it r4"><img class="photo" src="img/sq_${s.key}.jpg" alt=""><span class="corner">Bientot</span>
          <div class="meta"><div class="rar">Skin exclusif</div><h3 class="d">${s.name}</h3><div class="sub">${s.sub}</div><span class="tag soon"><b>Bientot</b></span></div></div>`).join("")}</div>`;
    }
    $("#content").innerHTML = `<div class="view">${h}</div>`;
  }
  function chips(v) { return `<div class="chips">${v.v.map((m) => `<span>${VEH[m]}</span>`).join("")}</div>`; }
  function wallet(bump) {
    $("#balCC").textContent = fmt(P.cc);
    $("#balUSD").textContent = fmt(P.money) + " $";
    if (bump) ["#wCC", "#wUSD"].forEach((s) => { const e = $(s); e.classList.remove("bump"); void e.offsetWidth; e.classList.add("bump"); });
  }
  function go(k) { tab = k; render(); $("#content").scrollTop = 0; }

  // ---------- achat ----------
  let cur = null, pick = 0;
  const all = () => [].concat(CAT.coins, CAT.money, CAT.vehicles, CAT.licences, CAT.kits, CAT.services);
  const bal = (c) => c === "cc" ? P.cc : P.money;

  function openBuy(id) {
    cur = all().find((x) => x.id === id); if (!cur) return;
    const it = cur; pick = 0;
    pvRot = 0; pvCol = 0;
    const side = it.v && bridge ? `<div class="side r${it.rar} pvslot" id="pvSlot"><div class="pvbar"><button onclick="Store.pvTurn(-1)" aria-label="Tourner">${svg("chevR")}</button><span id="pvName">${VEH[it.v[0]]}</span><button onclick="Store.pvColor()" aria-label="Couleur" class="pvcol"></button><button onclick="Store.pvTurn(1)" aria-label="Tourner">${svg("chevL")}</button></div></div>` : `<div class="side r${it.rar}">${it.cur === "fcfa" ? `<svg style="width:52%;height:auto"><use href="#cc"/></svg>` : it.amt ? `<svg style="width:52%;height:auto"><use href="#usd"/></svg>` : svg(it.icon || "star", "ic")}</div>`;
    const top = `<div class="rar">${RNAME[it.rar]}</div><h3 class="d">${it.name}</h3><div class="desc">${it.sub || ""}</div>`;
    let body;
    if (it.cur === "fcfa") {
      body = `<div class="recap"><div class="tot"><span>A payer</span><b>${fmt(it.price)} FCFA</b></div><div><span>Tu recois</span><b>${fmt(it.cc)} CC</b></div></div>
        <ol class="steps"><li>Paie ${fmt(it.price)} FCFA avec Wave.</li><li>Copie la reference de la transaction (dans ton recu Wave).</li><li>Colle-la ici et envoie.</li></ol>
        <button class="btn wave full" style="margin-top:10px" onclick="Store.wave()">${svg("wave")} Payer avec Wave</button>
        <div class="field"><label for="xIn">Reference Wave</label><input class="input" id="xIn" maxlength="32" placeholder="T_ABC123XYZ" autocomplete="off"><span class="hint" id="xHint">Lettres et chiffres, 5 caracteres minimum.</span></div>
        <div class="acts"><button class="btn ghost" onclick="Store.closeModal()">Annuler</button><button class="btn main" id="okBtn" disabled onclick="Store.confirm()">Envoyer</button></div>`;
    } else {
      body = `${field(it)}<div class="recap" id="recap"></div>
        <div class="acts"><button class="btn ghost" onclick="Store.closeModal()">Annuler</button><button class="btn main" id="okBtn" onclick="Store.confirm()">Acheter</button></div>`;
    }
    $("#modal").innerHTML = `${side}<div class="main">${top}${body}</div><button class="x" onclick="Store.closeModal()" aria-label="Fermer">${svg("x")}</button>`;
    const inp = $("#xIn"); if (inp) inp.addEventListener("input", onInput);
    $("#mbg").classList.add("on");
    if (it.cur !== "fcfa") recap(); else validate();
    pvSync();
  }

  // ---------- [v34] apercu 3D du vehicule : rendu par le JEU (donnees du jeu, mods compris) ----------
  // La case de gauche est un trou transparent ; le serveur place dessous une textdraw "model preview".
  let pvRot = 0, pvCol = 0, pvTimer = 0, pvOn = false;
  const PV_COLS = [1, 0, 3, 6, 79, 86, 126, 16];
  function pvSync() {
    clearTimeout(pvTimer);
    const it = cur;
    if (!it || !it.v || !bridge || !$("#mbg").classList.contains("on")) { pvOff(); return; }
    document.body.classList.add("pv-on");
    const nm = $("#pvName"); if (nm) nm.textContent = VEH[it.v[pick]];
    pvTimer = setTimeout(() => {
      const el = $("#pvSlot"); if (!el) return;
      const r = el.getBoundingClientRect(), W = window.innerWidth || 1, H = window.innerHeight || 1;
      const k = (v, d) => Math.max(0, Math.min(1000, Math.round(v / d * 1000)));
      send("pv", [it.v[pick], k(r.left, W), k(r.top, H), k(r.width, W), k(r.height - 34, H), pvRot, PV_COLS[pvCol]].join(" "));
      pvOn = true;
    }, 260);
  }
  function pvOff() {
    clearTimeout(pvTimer);
    document.body.classList.remove("pv-on");
    if (pvOn) { pvOn = false; send("pvoff", ""); }
  }
  window.addEventListener("resize", () => { if (pvOn) pvSync(); });
  function field(it) {
    if (it.v) return `<div class="picker">${it.v.map((m, i) => `<button class="${i === pick ? "on" : ""}" onclick="Store.pick(${i})">${VEH[m]}</button>`).join("")}</div>`;
    if (it.input === "name") return `<div class="field"><label for="xIn">Nouveau nom</label><input class="input" id="xIn" maxlength="22" placeholder="Kevin_Kouassi" autocomplete="off"><span class="hint" id="xHint">Format : Prenom_Nom</span></div>`;
    if (it.input === "plate") return `<div class="field"><label for="xIn">Texte de la plaque</label><input class="input mono" id="xIn" maxlength="8" placeholder="CREST01" autocomplete="off"><span class="hint" id="xHint">2 a 8 lettres ou chiffres.</span></div>`;
    if (it.input === "phone") return `<div class="field"><label for="xIn">Numero voulu</label><input class="input mono" id="xIn" maxlength="7" inputmode="numeric" placeholder="225225" autocomplete="off"><span class="hint" id="xHint">4 a 7 chiffres, s'il est libre.</span></div>`;
    return "";
  }
  function onInput(e) {
    const it = cur, el = e.target;
    if (it.input === "plate") el.value = el.value.toUpperCase().replace(/[^A-Z0-9]/g, "");
    else if (it.input === "phone") el.value = el.value.replace(/\D/g, "");
    else if (it.input === "name") el.value = el.value.replace(/[^A-Za-z_]/g, "");
    else el.value = el.value.replace(/[^A-Za-z0-9_-]/g, "");
    validate();
  }
  function recap() {
    const it = cur, t = it.price, b = bal(it.cur), after = b - t;
    $("#recap").innerHTML = `${it.v ? `<div><span>Modele</span><b>${VEH[it.v[pick]]}</b></div>` : ""}
      <div><span>Ton solde</span><b>${ptxt(b, it.cur)}</b></div>
      <div class="tot"><span>Total</span><b>${ptxt(t, it.cur)}</b></div>
      <div><span>Apres l'achat</span><b class="${after < 0 ? "neg" : ""}">${after < 0 ? "Il te manque " + ptxt(-after, it.cur) : ptxt(after, it.cur)}</b></div>`;
    validate();
  }
  function validate() {
    const it = cur, btn = $("#okBtn"), inp = $("#xIn"), h = $("#xHint");
    let ok = it.cur === "fcfa" || it.price <= bal(it.cur);
    if (inp) {
      const v = inp.value.trim(); let g = false, m = "";
      if (it.cur === "fcfa") { g = v.length >= 5; m = g ? "Reference prete" : "5 caracteres minimum"; }
      else if (it.input === "name") { g = /^[A-Z][a-z]{1,11}_[A-Z][a-z]{1,11}$/.test(v); m = g ? "Nom valide" : "Format attendu : Kevin_Kouassi"; }
      else if (it.input === "plate") { g = /^[A-Z0-9]{2,8}$/.test(v); m = g ? "Plaque valide" : "2 a 8 caracteres"; }
      else if (it.input === "phone") { g = /^\d{4,7}$/.test(v); m = g ? "Numero valide" : "4 a 7 chiffres"; }
      if (v.length && h) { h.textContent = m; h.className = "hint " + (g ? "ok" : "err"); }
      if (!g) ok = false;
    }
    if (btn) btn.disabled = !ok;
  }
  function confirm() {
    const it = cur; if (!it) return;
    const inp = $("#xIn"), v = inp ? inp.value.trim() : "";
    const btn = $("#okBtn"); if (btn) btn.disabled = true;
    if (it.cur === "fcfa") send("wave", it.id + " " + v);
    else send("buy", it.id + (it.v ? " " + it.v[pick] : v ? " " + v : ""));
    closeModal();
    if (!bridge) demo(it, v);
  }
  function send(type, arg) {
    if (!bridge) return false;
    try { bridge.action(type, arg); return true; } catch (e) { return false; }
  }
  function openWave() {
    try { if (bridge && bridge.openUrl) { bridge.openUrl(WAVE.link); return; } } catch (e) {}
    try { window.open(WAVE.link, "_blank"); } catch (e) {}
  }
  function closeModal() { $("#mbg").classList.remove("on"); pvOff(); }
  function toast(m, ok) {
    if (!m) return;
    const t = document.createElement("div"); t.className = "toast " + (ok ? "ok" : "err");
    t.innerHTML = svg(ok ? "check" : "alert") + `<span>${m}</span>`;
    $("#toasts").appendChild(t); setTimeout(() => t.remove(), 3600);
  }
  // sans le jeu (apercu dans un navigateur)
  function demo(it, v) {
    if (it.cur === "fcfa") { P.waves++; toast("Reference envoyee. Tes CC arrivent apres verification.", 1); render(); return; }
    if (it.price > bal(it.cur)) { toast("Solde insuffisant", 0); return; }
    if (it.cur === "cc") P.cc -= it.price; else P.money -= it.price;
    if (it.amt) P.money += it.amt;
    if (it.input || it.id === "sv_warn") P.orders++;
    wallet(true); render(); toast(it.name + " achete", 1);
  }

  function apply(json, isOpen) {
    let d = {};
    try { d = typeof json === "string" ? JSON.parse(json || "{}") : (json || {}); } catch (e) { d = {}; }
    const before = P.cc + "|" + P.money;
    ["name", "cc", "money", "orders", "waves"].forEach((k) => { if (d[k] !== undefined) P[k] = d[k]; });
    wallet(!isOpen && before !== P.cc + "|" + P.money);
    if (isOpen) { tab = "home"; $("#overlay").classList.remove("hidden"); }
    render();
    if (d.msg) toast(d.msg, +d.ok === 1);
  }

  window.Store = {
    go, buy: openBuy, pick(i) { pick = i; document.querySelectorAll("#modal .picker button").forEach((b, k) => b.classList.toggle("on", k === i)); recap(); pvRot = 0; pvSync(); },
    pvTurn(d) { pvRot = (pvRot + d * 45 + 360) % 360; pvSync(); },
    pvColor() { pvCol = (pvCol + 1) % PV_COLS.length; pvSync(); },
    confirm, wave: openWave, closeModal,
    close() { closeModal(); pvOff(); try { if (bridge) { bridge.close(); return; } } catch (e) {} $("#overlay").classList.add("hidden"); setTimeout(() => $("#overlay").classList.remove("hidden"), 900); },
  };
  window.StoreApp = {
    openFromNative(j) { apply(j, true); },
    onUpdateFromNative(j) { apply(j, false); },
  };
  window.closeModal = closeModal;
  document.addEventListener("keydown", (e) => { if (e.key === "Escape") { if ($("#mbg").classList.contains("on")) closeModal(); else Store.close(); } });

  wallet(); render();
  try { if (bridge && bridge.ready) bridge.ready(); } catch (e) {}
})();
