/* Crestwood - Inventaire v2
   Serveur -> page : InvApp.openFromNative(json) / InvApp.onUpdateFromNative(json)
     {"items":[{"id":"eau_kirene","qty":2}],"slots":["hamburger","",...5],"ground":[[gid,"item",qty]],
      "near":[[id,"Nom_Prenom"]],"w":1900,"wmax":50000,"cash":5000,"id":7,"name":"Nom","ok":1,"msg":"..."}
   Page -> serveur : SampInv.action(type, {...})  type = use | drop | give | slot | pick | refresh  (SAMP.java -> /invact)
   Ajouter un objet : image dans items/ (png ou svg) + une ligne dans CATALOG + le meme id dans g_InvItems (basicgm1.pwn). */
(() => {
  "use strict";
  const bridge = window.SampInv || null;
  const CELLS = 30;
  const CATALOG = {
    iphone_orange:  { name: "iPhone Orange",      tag: "Téléphone",  w: 220,  use: "",          desc: "Smartphone haut de gamme, finition orange cuivré. Ouvre ton téléphone depuis le HUD." },
    iphone_noir:    { name: "iPhone Titane Noir", tag: "Téléphone",  w: 220,  use: "",          desc: "Smartphone haut de gamme, finition titane noir. Ouvre ton téléphone depuis le HUD." },
    enceinte_jbl:   { name: "Enceinte JBL",       tag: "Accessoire", w: 900,  use: "",          desc: "Enceinte Bluetooth portable et étanche." },
    apple_watch:    { name: "Apple Watch",        tag: "Accessoire", w: 80,   use: "",          desc: "Montre connectée avec bracelet noir." },
    carte_sim:      { name: "Carte SIM",          tag: "Téléphone",  w: 10,   use: "Activer",   desc: "La puce de ton téléphone : active-la pour recevoir ton numéro (appels, SMS, WhatsApp, Wave)." },
    liasse_billets: { name: "Liasse de billets",  tag: "Argent",     w: 120,  use: "Encaisser", desc: "Une liasse de billets : +1 000 $ en liquide." },
    gilet_tactique: { name: "Gilet tactique",     tag: "Équipement", w: 3500, use: "Équiper",   desc: "Gilet pare-balles : armure à 100." },
    vody:           { name: "Vody Vodka Energy",  tag: "Boisson",    w: 400,  use: "Boire",     desc: "Canette de vodka énergisante (18 %). À consommer avec modération." },
    eau_kirene:     { name: "Eau Kirène",         tag: "Boisson",    w: 600,  use: "Boire",     desc: "Eau minérale naturelle : +10 de santé." },
    hamburger:      { name: "Hamburger",          tag: "Nourriture", w: 350,  use: "Manger",    desc: "Un bon burger bien chaud : +25 de santé." },
    bandage:        { name: "Bandage",            tag: "Soins",      w: 60,   use: "Soigner",   desc: "Pansement de premiers secours : +35 de santé." },
    tablette_job:   { name: "Tablette Job",       tag: "Travail",    w: 650,  use: "Ouvrir",    desc: "Ta tablette d'emploi : métiers, emplois Crestwood, prise de service, GPS et quêtes. S'ouvre aussi depuis la roue." }
  };
  const info = (id) => CATALOG[id] || { name: id, tag: "Objet", w: 0, use: "", desc: "" };
  const $ = (id) => document.getElementById(id);
  const S = { items: [], slots: ["", "", "", "", ""], ground: [], near: [], w: 0, wmax: 50000, cash: 0, id: 0, name: "", tab: "bag", sel: null, amount: 1, giving: false };

  function parse(raw) { if (raw && typeof raw === "object") return raw; try { return JSON.parse(raw || "{}") || {}; } catch (_) { return {}; } }
  function send(type, o) {
    if (!bridge || typeof bridge.action !== "function") { console.log("[inv]", type, o); return; }
    try { bridge.action(type, JSON.stringify(o || {})); } catch (_) {}
  }
  const kg = (g) => (Math.max(0, Number(g) || 0) / 1000).toFixed(1).replace(".", ",");
  const money = (v) => (Math.max(0, Math.floor(Number(v) || 0))).toLocaleString("fr-FR") + " $";
  const nm = (s) => String(s || "").replace(/_/g, " ");
  function img(id) {
    const i = document.createElement("img");
    i.alt = ""; i.src = "items/" + id + ".png";
    i.onerror = () => { if (!i.dataset.svg) { i.dataset.svg = 1; i.src = "items/" + id + ".svg"; } else i.style.visibility = "hidden"; };
    return i;
  }
  let toastT = 0;
  function toast(msg, ok) {
    if (!msg) return;
    const t = $("ivToast"); t.textContent = msg; t.className = "iv-toast show" + (ok ? "" : " err");
    clearTimeout(toastT); toastT = setTimeout(() => { t.className = "iv-toast"; }, 2600);
  }
  const qtyOf = (id) => { const it = S.items.find((x) => x.id === id); return it ? it.qty : 0; };

  /* ---------- v3 : 3 panneaux ---------- */
  const SLOT_ICONS = [
    '<path d="M4 14c0-5 3.5-8 8-8s8 3 8 8z"/><path d="M3 14h18v2H3z"/>',
    '<path d="M8 4 3 7l2 4 2-1v10h10V10l2 1 2-4-5-3a4 4 0 0 1-8 0Z"/>',
    '<path d="M8 3h8l1 5H7zM7 8v13h4v-7h2v7h4V8"/>',
    '<path d="M6 12V5a1.5 1.5 0 0 1 3 0v5V4a1.5 1.5 0 0 1 3 0v6V5a1.5 1.5 0 0 1 3 0v8c0 4-2 7-5 7s-5-3-5-6z"/>',
    '<path d="M4 20l3-3h10l3 3M7 17V9l5-4 5 4v8"/>',
    '<path d="M12 3a8 8 0 0 0-8 8v4a3 3 0 0 0 3 3h1v-7H5M12 3a8 8 0 0 1 8 8v4a3 3 0 0 1-3 3h-1v-7h3"/>',
    '<circle cx="8" cy="12" r="3.5"/><circle cx="16" cy="12" r="3.5"/><path d="M11.5 12h1"/>',
    '<path d="M12 3a6 6 0 0 0-3 11l-1 7h8l-1-7a6 6 0 0 0-3-11z"/>',
    '<circle cx="12" cy="12" r="8"/><path d="M12 7v5l3 2"/>',
    '<path d="M12 3l3 6 6 1-4.5 4.5L18 21l-6-3-6 3 1.5-6.5L3 10l6-1z"/>'
  ];
  function renderSlots() {
    ["ivSlotsL", "ivSlotsR"].forEach((id, side) => {
      const c = $(id); if (c.childElementCount) return;
      for (let k = 0; k < 5; k++) {
        const d = document.createElement("div"); d.className = "sl";
        d.innerHTML = '<svg viewBox="0 0 24 24">' + SLOT_ICONS[side * 5 + k] + "</svg>";
        c.appendChild(d);
      }
    });
  }
  function closePop() { S.sel = null; S.giving = false; $("ivPop").hidden = true; render(); }
  function renderTop() {
    $("ivName").textContent = nm(S.name) || "—";
    $("ivId").textContent = "#" + String(S.id).padStart(5, "0");
    $("ivCash").textContent = money(S.cash);
    $("ivGroundN").textContent = S.ground.length;
    $("ivW").textContent = kg(S.w);
    $("ivWmax").textContent = "/" + Math.round(S.wmax / 1000) + " kg";
    const pct = Math.min(100, (S.w / Math.max(1, S.wmax)) * 100);
    $("ivBar").style.width = pct + "%";
    $("ivBar").parentElement.classList.toggle("heavy", pct > 90);
  }
  function renderPockets() {
    const q = $("ivQuick"); q.textContent = "";
    S.slots.forEach((id, k) => {
      const b = document.createElement("button"); b.type = "button"; b.className = "qs";
      if (S.sel && S.tab === "bag" && S.slots[k] === S.sel) b.classList.add("target");
      if (id && qtyOf(id) > 0) {
        b.appendChild(img(id));
        const c = document.createElement("span"); c.className = "c"; c.textContent = qtyOf(id) + "x"; b.appendChild(c);
      }
      const n = document.createElement("span"); n.className = "n"; n.textContent = k + 1; b.appendChild(n);
      b.addEventListener("click", () => {
        if (id && qtyOf(id) > 0) { S.sel = id; S.tab = "bag"; S.amount = 1; S.giving = false; render(); }
      });
      q.appendChild(b);
    });
  }
  function fillGrid(el, list, bag) {
    el.textContent = "";
    for (let i = 0; i < Math.max(bag ? 24 : 20, list.length); i++) {
      const it = list[i];
      const c = document.createElement("button"); c.type = "button"; c.className = "cell";
      if (it) {
        c.classList.add("filled");
        if (S.sel === it.key) c.classList.add("sel");
        c.appendChild(img(it.id));
        const q = document.createElement("span"); q.className = "q"; q.textContent = it.qty + "x"; c.appendChild(q);
        const w = document.createElement("span"); w.className = "w"; w.textContent = kg(info(it.id).w * it.qty); c.appendChild(w);
        c.addEventListener("click", () => { S.tab = bag ? "bag" : "ground"; S.sel = it.key; S.amount = 1; S.giving = false; render(); });
      } else c.disabled = true;
      el.appendChild(c);
    }
  }
  function renderPop() {
    const p = $("ivPop"); p.textContent = "";
    const ground = S.sel && S.sel[0] === "g" && S.ground.some((x) => "g" + x[0] === S.sel);
    const bagIt = !ground && S.sel && qtyOf(S.sel) > 0;
    if (!ground && !bagIt) { p.hidden = true; if (S.sel) S.sel = null; return; }
    p.hidden = false;
    const x = document.createElement("button"); x.type = "button"; x.className = "x"; x.textContent = "✕"; x.addEventListener("click", closePop); p.appendChild(x);
    if (ground) {
      const it = S.ground.find((y) => "g" + y[0] === S.sel), inf = info(it[1]);
      const t = document.createElement("div"); t.className = "pt"; t.appendChild(img(it[1]));
      const tt = document.createElement("div"); tt.innerHTML = "<b></b><small></small>"; tt.querySelector("b").textContent = inf.name; tt.querySelector("small").textContent = it[2] + "x · au sol"; t.appendChild(tt); p.appendChild(t);
      const b = document.createElement("button"); b.type = "button"; b.className = "act main"; b.textContent = "Ramasser";
      b.addEventListener("click", () => { b.disabled = true; send("pick", { gid: it[0] }); }); p.appendChild(b);
      return;
    }
    const id = S.sel, inf = info(id), have = qtyOf(id);
    if (S.amount > have) S.amount = have;
    const t = document.createElement("div"); t.className = "pt"; t.appendChild(img(id));
    const tt = document.createElement("div"); tt.innerHTML = "<b></b><small></small>"; tt.querySelector("b").textContent = inf.name; tt.querySelector("small").textContent = inf.tag + " · " + have + "x · " + kg(inf.w * have) + " kg"; t.appendChild(tt); p.appendChild(t);
    if (inf.desc) { const d = document.createElement("div"); d.className = "pd"; d.textContent = inf.desc; p.appendChild(d); }
    if (have > 1) {
      const q = document.createElement("div"); q.className = "qt"; q.innerHTML = '<button type="button">−</button><b></b><button type="button">+</button>';
      q.querySelector("b").textContent = S.amount;
      const [mi, pl] = q.querySelectorAll("button");
      mi.addEventListener("click", () => { S.amount = Math.max(1, S.amount - 1); renderPop(); });
      pl.addEventListener("click", () => { S.amount = Math.min(have, S.amount + 1); renderPop(); });
      p.appendChild(q);
    }
    const use = document.createElement("button"); use.type = "button"; use.className = "act main"; use.textContent = inf.use || "Utiliser"; use.disabled = !inf.use;
    use.addEventListener("click", () => { use.disabled = true; send("use", { item: id, qty: 1 }); }); p.appendChild(use);
    const give = document.createElement("button"); give.type = "button"; give.className = "act"; give.textContent = S.giving ? "Annuler" : "Donner";
    give.addEventListener("click", () => { S.giving = !S.giving; if (S.giving) send("refresh"); renderPop(); }); p.appendChild(give);
    if (S.giving) {
      const pp = document.createElement("div"); pp.className = "ppl";
      if (!S.near.length) pp.innerHTML = '<small style="color:var(--muted);font-size:10px">Personne à côté de toi.</small>';
      S.near.forEach((n) => {
        const b = document.createElement("button"); b.type = "button"; b.innerHTML = "<span></span><small>#" + n[0] + "</small>"; b.firstChild.textContent = nm(n[1]);
        b.addEventListener("click", () => { S.giving = false; send("give", { item: id, qty: S.amount, to: n[0] }); }); pp.appendChild(b);
      });
      p.appendChild(pp);
    }
    const drop = document.createElement("button"); drop.type = "button"; drop.className = "act red"; drop.textContent = "Jeter au sol";
    drop.addEventListener("click", () => { drop.disabled = true; send("drop", { item: id, qty: S.amount }); }); p.appendChild(drop);
    const r5 = document.createElement("div"); r5.className = "row5";
    for (let k = 0; k < 5; k++) {
      const b = document.createElement("button"); b.type = "button"; b.textContent = k + 1; if (S.slots[k] === id) b.classList.add("on");
      b.addEventListener("click", () => send("slot", { slot: k + 1, item: S.slots[k] === id ? "" : id })); r5.appendChild(b);
    }
    p.appendChild(r5);
  }
  function render() {
    renderSlots(); renderTop(); renderPockets();
    fillGrid($("ivGrid"), S.items.map((x) => ({ id: x.id, qty: x.qty, key: x.id })), true);
    fillGrid($("ivGround"), S.ground.map((x) => ({ id: x[1], qty: x[2], key: "g" + x[0] })), false);
    $("ivNote").textContent = S.ground.length ? "Objets posés autour de toi (10 min)." : "Rien au sol autour de toi.";
    renderPop();
  }

  function apply(d) {
    if (Array.isArray(d.items)) S.items = d.items.filter((x) => x && x.id && Number(x.qty) > 0).map((x) => ({ id: String(x.id), qty: Number(x.qty) }));
    if (Array.isArray(d.slots)) S.slots = [0, 1, 2, 3, 4].map((k) => String(d.slots[k] || ""));
    if (Array.isArray(d.ground)) S.ground = d.ground;
    if (Array.isArray(d.near)) S.near = d.near;
    if (d.w !== undefined) S.w = Number(d.w) || 0;
    if (d.wmax) S.wmax = Number(d.wmax) || 50000;
    if (d.cash !== undefined) S.cash = Number(d.cash) || 0;
    if (d.id !== undefined) S.id = Number(d.id) || 0;
    if (d.name) S.name = String(d.name);
    if (S.sel && S.sel[0] === "g" && !S.ground.some((x) => "g" + x[0] === S.sel)) S.sel = null;
    render();
    if (d.msg) toast(d.msg, Number(d.ok) === 1);
  }

  $("ivClose").addEventListener("click", () => { if (bridge && typeof bridge.close === "function") { try { bridge.close(); } catch (_) {} } });

  window.InvApp = { openFromNative(raw) { S.tab = "bag"; S.sel = null; apply(parse(raw)); }, onUpdateFromNative(raw) { apply(parse(raw)); } };

  if (!bridge) {   // apercu navigateur
    window.InvApp.openFromNative({ items: [{ id: "hamburger", qty: 1 }, { id: "iphone_noir", qty: 1 }, { id: "bandage", qty: 3 }, { id: "eau_kirene", qty: 2 }, { id: "carte_sim", qty: 1 }, { id: "gilet_tactique", qty: 1 }],
      slots: ["hamburger", "iphone_noir", "bandage", "", ""], ground: [[3, "eau_kirene", 1]], near: [[4, "Kofi_Mensah"]], w: 6180, wmax: 50000, cash: 5000, id: 7, name: "Anzo_Diman", ok: 1, msg: "Tu as jete 1x hamburger au sol." });
  }
  try { if (bridge && typeof bridge.ready === "function") bridge.ready(); } catch (_) {}
})();
