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

  /* ---------- gauche ---------- */
  function renderLeft() {
    $("ivAv").textContent = (nm(S.name).trim()[0] || "?").toUpperCase();
    $("ivName").textContent = nm(S.name) || "—";
    $("ivId").textContent = "#" + String(S.id).padStart(5, "0");
    $("ivCash").textContent = money(S.cash);
    $("ivWtTotal").textContent = kg(S.w) + " kg";
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
        if (S.sel && S.tab === "bag") { send("slot", { slot: k + 1, item: S.slots[k] === S.sel ? "" : S.sel }); return; }
        if (id && qtyOf(id) > 0) { if (info(id).use) send("use", { item: id, qty: 1 }); else { S.sel = id; S.tab = "bag"; render(); } }
      });
      q.appendChild(b);
    });
  }

  /* ---------- centre ---------- */
  function renderCenter() {
    document.querySelectorAll(".iv-tab").forEach((t) => t.classList.toggle("on", t.dataset.tab === S.tab));
    $("ivGroundN").textContent = S.ground.length;
    $("ivW").textContent = kg(S.w);
    $("ivWmax").textContent = "/" + Math.round(S.wmax / 1000) + " kg";
    const pct = Math.min(100, (S.w / Math.max(1, S.wmax)) * 100);
    $("ivBar").style.width = pct + "%";
    $("ivBar").parentElement.classList.toggle("heavy", pct > 90);
    const g = $("ivGrid"); g.textContent = "";
    const list = S.tab === "bag" ? S.items.map((x) => ({ id: x.id, qty: x.qty, key: x.id })) : S.ground.map((x) => ({ id: x[1], qty: x[2], key: "g" + x[0], gid: x[0] }));
    for (let i = 0; i < Math.max(CELLS, list.length); i++) {
      const it = list[i];
      const c = document.createElement("button"); c.type = "button"; c.className = "cell";
      if (it) {
        c.classList.add("filled");
        if (S.sel === it.key || (S.tab === "bag" && S.sel === it.id)) c.classList.add("sel");
        c.appendChild(img(it.id));
        const q = document.createElement("span"); q.className = "q"; q.textContent = it.qty + "x"; c.appendChild(q);
        const w = document.createElement("span"); w.className = "w"; w.textContent = kg(info(it.id).w * it.qty); c.appendChild(w);
        c.addEventListener("click", () => { S.sel = S.tab === "bag" ? it.id : it.key; S.amount = 1; S.giving = false; render(); });
      } else c.disabled = true;
      g.appendChild(c);
    }
    $("ivNote").textContent = S.tab === "bag"
      ? (S.sel ? "Touche un raccourci (1 à 5) à gauche pour y placer l'objet." : "")
      : (S.ground.length ? "Objets posés au sol autour de toi (ils disparaissent après 10 min)." : "Rien au sol autour de toi.");
  }

  /* ---------- droite ---------- */
  function renderRight() {
    const d = $("ivDetail"); d.textContent = "";
    if (S.tab === "ground") {
      const it = S.ground.find((x) => "g" + x[0] === S.sel);
      if (!it) { d.innerHTML = '<p class="d-empty">Touche un objet au sol<br>pour le ramasser.</p>'; return; }
      const inf = info(it[1]);
      const box = document.createElement("div"); box.className = "d-img"; box.appendChild(img(it[1])); d.appendChild(box);
      d.insertAdjacentHTML("beforeend", '<div class="d-name"></div><div class="d-meta"></div>');
      d.querySelector(".d-name").textContent = inf.name;
      d.querySelector(".d-meta").textContent = it[2] + "x · " + kg(inf.w * it[2]) + " kg · au sol";
      const b = document.createElement("button"); b.type = "button"; b.className = "btn main"; b.textContent = "Ramasser";
      b.addEventListener("click", () => { b.disabled = true; send("pick", { gid: it[0] }); });
      d.appendChild(b);
      return;
    }
    const have = S.sel ? qtyOf(S.sel) : 0;
    if (!S.sel || have <= 0) { S.sel = null; d.innerHTML = '<p class="d-empty">Touche un objet<br>pour voir ce que tu peux en faire.</p>'; return; }
    const inf = info(S.sel);
    if (S.amount > have) S.amount = have;
    const box = document.createElement("div"); box.className = "d-img"; box.appendChild(img(S.sel)); d.appendChild(box);
    const n = document.createElement("div"); n.className = "d-name"; n.textContent = inf.name; d.appendChild(n);
    const m = document.createElement("div"); m.className = "d-meta"; m.textContent = inf.tag + " · " + have + "x · " + kg(inf.w * have) + " kg"; d.appendChild(m);
    if (inf.desc) { const p = document.createElement("div"); p.className = "d-desc"; p.textContent = inf.desc; d.appendChild(p); }
    const q = document.createElement("div"); q.className = "d-qty";
    q.innerHTML = '<button type="button">−</button><b></b><button type="button">+</button><small>sur ' + have + "</small>";
    q.querySelector("b").textContent = S.amount;
    const [mi, pl] = q.querySelectorAll("button");
    mi.addEventListener("click", () => { S.amount = Math.max(1, S.amount - 1); renderRight(); });
    pl.addEventListener("click", () => { S.amount = Math.min(have, S.amount + 1); renderRight(); });
    d.appendChild(q);
    const use = document.createElement("button"); use.type = "button"; use.className = "btn main"; use.textContent = inf.use || "Utiliser"; use.disabled = !inf.use;
    use.addEventListener("click", () => { use.disabled = true; send("use", { item: S.sel, qty: 1 }); });
    d.appendChild(use);
    const give = document.createElement("button"); give.type = "button"; give.className = "btn"; give.textContent = S.giving ? "Annuler" : "Donner";
    give.addEventListener("click", () => { S.giving = !S.giving; if (S.giving) send("refresh"); renderRight(); });
    d.appendChild(give);
    if (S.giving) {
      const pp = document.createElement("div"); pp.className = "d-people";
      if (!S.near.length) pp.innerHTML = '<p class="d-empty" style="margin:4px 0">Personne à côté de toi.</p>';
      S.near.forEach((p) => {
        const b = document.createElement("button"); b.type = "button";
        b.innerHTML = "<span></span><small>#" + p[0] + "</small>"; b.firstChild.textContent = nm(p[1]);
        b.addEventListener("click", () => { S.giving = false; send("give", { item: S.sel, qty: S.amount, to: p[0] }); });
        pp.appendChild(b);
      });
      d.appendChild(pp);
    }
    const drop = document.createElement("button"); drop.type = "button"; drop.className = "btn red"; drop.textContent = "Jeter au sol";
    drop.addEventListener("click", () => { drop.disabled = true; send("drop", { item: S.sel, qty: S.amount }); });
    d.appendChild(drop);
    const l = document.createElement("div"); l.className = "d-lbl"; l.textContent = "Raccourci"; d.appendChild(l);
    const sl = document.createElement("div"); sl.className = "d-slots";
    for (let k = 0; k < 5; k++) {
      const b = document.createElement("button"); b.type = "button"; b.textContent = k + 1;
      if (S.slots[k] === S.sel) b.classList.add("on");
      b.addEventListener("click", () => send("slot", { slot: k + 1, item: S.slots[k] === S.sel ? "" : S.sel }));
      sl.appendChild(b);
    }
    d.appendChild(sl);
  }

  function render() { renderLeft(); renderCenter(); renderRight(); }

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
    if (S.tab === "ground" && S.sel && !S.ground.some((x) => "g" + x[0] === S.sel)) S.sel = null;
    render();
    if (d.msg) toast(d.msg, Number(d.ok) === 1);
  }

  document.querySelectorAll(".iv-tab").forEach((t) => t.addEventListener("click", () => {
    S.tab = t.dataset.tab; S.sel = null; S.giving = false; render(); send("refresh");
  }));
  $("ivClose").addEventListener("click", () => { if (bridge && typeof bridge.close === "function") { try { bridge.close(); } catch (_) {} } });

  window.InvApp = { openFromNative(raw) { S.tab = "bag"; S.sel = null; apply(parse(raw)); }, onUpdateFromNative(raw) { apply(parse(raw)); } };

  if (!bridge) {   // apercu navigateur
    window.InvApp.openFromNative({ items: [{ id: "hamburger", qty: 1 }, { id: "iphone_noir", qty: 1 }, { id: "bandage", qty: 3 }, { id: "eau_kirene", qty: 2 }, { id: "carte_sim", qty: 1 }, { id: "gilet_tactique", qty: 1 }],
      slots: ["hamburger", "iphone_noir", "bandage", "", ""], ground: [[3, "eau_kirene", 1]], near: [[4, "Kofi_Mensah"]], w: 6180, wmax: 50000, cash: 5000, id: 7, name: "Anzo_Diman", ok: 1, msg: "Tu as jete 1x hamburger au sol." });
  }
  try { if (bridge && typeof bridge.ready === "function") bridge.ready(); } catch (_) {}
})();
