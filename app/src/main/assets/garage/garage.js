/* [v21] Garage Crestwood, refait de zero.
   Serveur -> page : GarageApp.openFromNative(json) / onUpdateFromNative(json) / closeFromNative()
   Page -> serveur : SampGarage.cmd("take 12") -> /garact take 12 (liste blanche cote Java, le serveur re-verifie tout).
   JSON : {"garage","at","deliv","cash","bank","maxOut","ok","msg","fee":{rep,svb,svw,fl,i7,i30,imin,paint,plate,sellPct},
           "veh":[{id,m,n,k,pl,hp,fu,o,s,p,km,w,in,im,f,c1,c2,sh,ow,lk,en}]} */
(() => {
  "use strict";
  const $ = (id) => document.getElementById(id);
  const bridge = () => window.SampGarage || null;
  const money = (n) => (Number(n) || 0).toLocaleString("fr-FR") + " $";
  const esc = (s) => String(s == null ? "" : s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
  const COLORS = { 0: "#0b0c0e", 1: "#f5f5f5", 2: "#2a77a1", 3: "#840410", 5: "#86446e", 6: "#d78e10", 7: "#4c75b7", 8: "#bdbec6", 9: "#5e7072",
    14: "#e4e5e1", 16: "#335f3f", 17: "#730a27", 24: "#414a52", 26: "#a3a7ac", 36: "#1b2125", 43: "#61091a", 44: "#1d4e32", 46: "#9c8d71",
    51: "#2f5a3f", 53: "#1e2e47", 58: "#6c0f18", 61: "#966d3b", 65: "#a78a3b", 79: "#244e8a", 85: "#5d1b20", 86: "#315c1a", 87: "#306c86",
    93: "#1d70b0", 103: "#2a3a59", 113: "#5f4a2f", 125: "#2a4a67", 126: "#f39ad7" };
  const PAINT = Object.keys(COLORS).map(Number);
  const col = (i) => COLORS[i] || "#59606b";

  let S = { veh: [], fee: {} }, sel = 0, filter = "all", tab = "shop", busy = false, paint = [-1, -1], modalFn = null;

  function apply(json, opening) {
    let d = json;
    if (typeof d === "string") { try { d = JSON.parse(d); } catch (_) { return; } }
    if (!d || typeof d !== "object") return;
    S = Object.assign({ veh: [], fee: {} }, d);
    if (!Array.isArray(S.veh)) S.veh = [];
    busy = false;
    if (opening) { $("app").hidden = false; paint = [-1, -1]; }
    if (!S.veh.some((v) => v.id === sel)) sel = S.veh.length ? S.veh[0].id : 0;
    if (S.msg) toast(S.msg, !Number(S.ok));
    render();
  }

  const cur = () => S.veh.find((v) => v.id === sel) || null;
  const shown = () => S.veh.filter((v) => filter === "all" || (filter === "in" && !Number(v.o)) || (filter === "out" && Number(v.o)) || (filter === "fav" && Number(v.f)));

  function render() {
    $("gName").textContent = S.garage || "Garage";
    $("gSub").textContent = Number(S.at) ? "Atelier sur place" : "Livraison à côté de toi · " + money(S.deliv);
    $("gCash").textContent = money(S.cash);
    $("gBank").textContent = money(S.bank);
    $("gOut").textContent = S.veh.filter((v) => Number(v.o)).length + " / " + (S.maxOut || 2);
    const list = $("list");
    list.replaceChildren();
    const rows = shown();
    if (!rows.length) { const e = document.createElement("p"); e.className = "gx-note"; e.textContent = S.veh.length ? "Aucun véhicule dans ce filtre." : "Ton garage est vide."; list.appendChild(e); }
    rows.forEach((v) => {
      const b = document.createElement("button");
      b.type = "button";
      b.className = "gx-row" + (v.id === sel ? " on" : "");
      const st = Number(v.im) ? "imp" : Number(v.o) ? "out" : Number(v.f) ? "fav" : "";
      b.innerHTML = `<span class="gx-sw"><i style="background:${col(v.c1)}"></i><i style="background:${col(v.c2)}"></i></span>
        <span class="gx-rt"><b>${esc(v.k || v.n)}</b><small>${esc(v.pl)} · ${Number(v.im) ? "Fourrière" : Number(v.o) ? "Sorti" : "Au garage"}${Number(v.ow) ? "" : " · prêté"}</small></span><i class="gx-dot ${st}"></i>`;
      b.addEventListener("click", () => { sel = v.id; paint = [-1, -1]; render(); });
      list.appendChild(b);
    });
    const v = cur();
    $("empty").hidden = !!v;
    $("detail").hidden = !v;
    if (!v) return;
    const out = !!Number(v.o), imp = !!Number(v.im), owner = !!Number(v.ow);
    $("vState").textContent = imp ? "En fourrière" : out ? (Number(v.en) ? "Sorti · moteur allumé" : "Sorti") : "Au garage";
    $("vState").className = "gx-state" + (imp ? " imp" : out ? " out" : "");
    $("vName").textContent = v.k ? `${v.k} · ${v.n}` : v.n;
    $("vPlate").textContent = v.pl || "CW";
    $("car").querySelector(".c1").style.background = col(v.c1);
    $("car").querySelector(".c2").style.background = col(v.c2);
    $("btnFav").classList.toggle("on", !!Number(v.f));
    gauge("gHp", v.hp, v.hp < 35 ? "var(--bad)" : v.hp < 70 ? "var(--warn)" : "var(--ok)");
    gauge("gFu", v.fu, v.fu < 20 ? "var(--bad)" : "var(--blue)");
    gauge("gWe", v.w, v.w > 70 ? "var(--bad)" : v.w > 40 ? "var(--warn)" : "var(--blue)");
    $("gKm").querySelector("b").textContent = (Number(v.km) || 0).toLocaleString("fr-FR");
    const take = $("btnTake");
    take.textContent = imp ? "En fourrière" : out ? "Ranger le véhicule" : (Number(S.at) ? "Sortir le véhicule" : "Me le livrer · " + money(S.deliv));
    take.disabled = busy || imp;
    $("btnGps").disabled = busy || !out;
    $("btnLock").textContent = Number(v.lk) ? "Déverrouiller" : "Verrouiller";
    $("btnLock").disabled = busy || !out;
    Array.prototype.forEach.call($("tabs").children, (t) => t.classList.toggle("on", t.dataset.t === tab));
    renderPane(v, out, imp, owner);
  }

  function gauge(id, p, c) {
    const g = $(id);
    const n = Math.max(0, Math.min(100, Number(p) || 0));
    g.querySelector("i").style.setProperty("--p", n);
    g.querySelector("i").style.setProperty("--c", c);
    g.querySelector("b").textContent = n + "%";
  }

  function svc(title, sub, price, label, enabled, fn, cls) {
    const d = document.createElement("div");
    d.className = "gx-svc";
    d.innerHTML = `<div><b>${esc(title)}</b><small>${esc(sub)}${price ? " · <em>" + esc(price) + "</em>" : ""}</small></div>`;
    const b = document.createElement("button");
    b.type = "button";
    b.className = "gx-btn " + (cls || "main");
    b.textContent = label;
    b.disabled = busy || !enabled;
    b.addEventListener("click", fn);
    d.appendChild(b);
    return d;
  }
  function note(text) { const p = document.createElement("p"); p.className = "gx-note"; p.textContent = text; return p; }

  function renderPane(v, out, imp, owner) {
    const f = S.fee || {}, pane = $("pane"), shop = !out || !!Number(S.at);
    pane.replaceChildren();
    if (tab === "shop") {
      const dmg = 100 - (Number(v.hp) || 0), liters = 100 - (Number(v.fu) || 0);
      if (imp) pane.appendChild(svc("Sortie de fourrière", "Véhicule détruit sans assurance", money(v.im), "Payer", true, () => send("unimpound " + v.id)));
      pane.appendChild(svc("Réparation", dmg > 0 ? dmg + " % de dégâts" : "Carrosserie en parfait état", dmg > 0 ? money(dmg * (f.rep || 0)) : "", "Réparer", shop && dmg > 0 && !imp, () => send("repair " + v.id)));
      pane.appendChild(svc("Entretien", v.w > 0 ? "Usure " + v.w + " %" : "Moteur révisé", v.w > 0 ? money((f.svb || 0) + v.w * (f.svw || 0)) : "", "Réviser", shop && v.w > 0 && !imp, () => send("service " + v.id)));
      pane.appendChild(svc("Plein d'essence", liters > 0 ? liters + " L à remettre" : "Réservoir plein", liters > 0 ? money(liters * (f.fl || 0)) : "", "Faire le plein", shop && liters > 0 && !imp, () => send("refuel " + v.id)));
      if (!shop) pane.appendChild(note("Véhicule sorti : gare-le devant un garage et ouvre le garage sur place pour l'atelier."));
    } else if (tab === "ins") {
      const p = Number(v.p) || 0, i7 = Math.max(f.imin || 0, Math.floor(p / 100) * (f.i7 || 0)), i30 = Math.max(f.imin || 0, Math.floor(p / 100) * (f.i30 || 0));
      pane.appendChild(note(Number(v.in) ? `Assuré encore ${v.in} jour${v.in > 1 ? "s" : ""} : en cas de destruction, le véhicule revient au garage sans frais.` : "Pas d'assurance : un véhicule détruit part en fourrière (payante)."));
      pane.appendChild(svc("Formule 7 jours", Number(v.in) ? "Prolonge la couverture" : "Couverture immédiate", money(i7), "Souscrire", owner, () => send("insure " + v.id + " 7")));
      pane.appendChild(svc("Formule 30 jours", "La plus avantageuse", money(i30), "Souscrire", owner, () => send("insure " + v.id + " 30")));
    } else if (tab === "look") {
      pane.appendChild(svc("Surnom", v.k ? "« " + v.k + " »" : "Donne un nom à ton véhicule", "", "Renommer", owner, () => ask("Surnom du véhicule", "Lettres et chiffres, 16 maximum (vide = retirer).", v.k || "", (s) => send("nick " + v.id + (s ? " " + s.replace(/[^A-Za-z0-9_-]/g, "") : "")))));
      pane.appendChild(svc("Plaque personnalisée", "Actuelle : " + (v.pl || "—"), money(f.plate), "Changer", owner, () => ask("Nouvelle plaque", "4 à 8 lettres ou chiffres, unique sur le serveur.", "", (s) => { const p = s.toUpperCase().replace(/[^A-Z0-9]/g, "").slice(0, 8); if (p.length >= 4) send("plate " + v.id + " " + p); else toast("Plaque : 4 à 8 caractères.", true); })));
      ["Couleur principale", "Couleur secondaire"].forEach((label, k) => {
        const row = document.createElement("div");
        row.className = "gx-colors";
        row.innerHTML = `<span>${label}</span>`;
        const now = paint[k] >= 0 ? paint[k] : (k ? v.c2 : v.c1);
        PAINT.forEach((id) => {
          const b = document.createElement("button");
          b.type = "button";
          b.style.background = col(id);
          b.className = id === Number(now) ? "on" : "";
          b.setAttribute("aria-label", "Couleur " + id);
          b.addEventListener("click", () => { paint[k] = id; if (paint[1 - k] < 0) paint[1 - k] = k ? v.c1 : v.c2; render(); });
          row.appendChild(b);
        });
        pane.appendChild(row);
      });
      const changed = paint[0] >= 0 && paint[1] >= 0 && (paint[0] !== Number(v.c1) || paint[1] !== Number(v.c2));
      pane.appendChild(svc("Peinture", changed ? "Nouvelles couleurs choisies" : "Choisis les couleurs ci-dessus", money(f.paint), "Peindre", changed && owner && shop, () => send("paint " + v.id + " " + paint[0] + " " + paint[1])));
    } else {
      pane.appendChild(svc("Clés partagées", v.sh ? "Prêté à " + v.sh.replace(/_/g, " ") : "Personne n'a tes clés", "", v.sh ? "Reprendre" : "Prêter", owner, () => {
        if (v.sh) send("keys " + v.id);
        else ask("Prêter les clés", "Prénom_Nom du joueur.", "", (s) => { const n = s.replace(/[^A-Za-z0-9_]/g, ""); if (n.length >= 3) send("keys " + v.id + " " + n); });
      }, v.sh ? "" : "main"));
      pane.appendChild(svc("Vendre à un joueur", "Proposition à un joueur proche", "", "Proposer", owner && !out, () => ask("Acheteur et prix", "ID du joueur puis prix, ex. 12 45000", "", (s) => { const m = s.match(/^\s*(\d{1,4})\s+(\d{1,8})\s*$/); if (m) send("offer " + v.id + " " + m[1] + " " + m[2]); else toast("Format : ID prix", true); })));
      pane.appendChild(svc("Revendre au garage", (f.sellPct || 60) + " % du prix d'achat", money(v.s), "Vendre", owner && !out, () => confirmBox("Vendre " + (v.k || v.n) + " ?", "Tu reçois " + money(v.s) + ". C'est définitif.", () => send("sell " + v.id)), "danger"));
      if (!owner) pane.appendChild(note("Ce véhicule t'est prêté : seul son propriétaire peut le vendre ou le modifier."));
    }
  }

  function send(command) {
    const b = bridge();
    if (busy) return;
    busy = true;
    render();
    if (b && b.cmd) { try { b.cmd(command); } catch (_) { busy = false; render(); } }
    else if (b && b.action) { const [t, id] = command.split(" "); try { b.action(t, JSON.stringify({ arg: id || "" })); } catch (_) { /* ignore */ } }
    else demo(command);
    setTimeout(() => { if (busy) { busy = false; render(); } }, 2500);
  }

  let toastTimer = 0;
  function toast(msg, err) {
    const t = $("toast");
    t.textContent = msg;
    t.classList.toggle("err", !!err);
    t.hidden = false;
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => { t.hidden = true; }, 3200);
  }
  function ask(title, text, value, fn) {
    $("mTitle").textContent = title; $("mText").textContent = text;
    $("mInput").hidden = false; $("mInput").value = value || "";
    modalFn = () => fn($("mInput").value.trim());
    $("modal").hidden = false;
    setTimeout(() => { try { $("mInput").focus(); } catch (_) { /* ignore */ } }, 60);
  }
  function confirmBox(title, text, fn) {
    $("mTitle").textContent = title; $("mText").textContent = text;
    $("mInput").hidden = true; modalFn = fn; $("modal").hidden = false;
  }
  $("mNo").addEventListener("click", () => { $("modal").hidden = true; });
  $("mYes").addEventListener("click", () => { $("modal").hidden = true; if (modalFn) modalFn(); modalFn = null; });
  $("modal").addEventListener("click", (e) => { if (e.target.id === "modal") $("modal").hidden = true; });

  $("filter").addEventListener("click", (e) => { const b = e.target.closest("button"); if (!b) return; filter = b.dataset.f; Array.prototype.forEach.call($("filter").children, (x) => x.classList.toggle("on", x === b)); render(); });
  $("tabs").addEventListener("click", (e) => { const b = e.target.closest("button"); if (!b) return; tab = b.dataset.t; render(); });
  $("btnTake").addEventListener("click", () => { const v = cur(); if (v) send((Number(v.o) ? "store " : "take ") + v.id); });
  $("btnGps").addEventListener("click", () => { const v = cur(); if (v) send("gps " + v.id); });
  $("btnLock").addEventListener("click", () => { const v = cur(); if (v) send("lock " + v.id); });
  $("btnFav").addEventListener("click", () => { const v = cur(); if (v) send("fav " + v.id); });
  $("close").addEventListener("click", () => { const b = bridge(); if (b && b.close) b.close(); else $("app").hidden = true; });
  window.addEventListener("cw:pageshow", () => { tab = "shop"; });

  window.GarageApp = {
    openFromNative(json) { apply(json, true); },
    onUpdateFromNative(json) { apply(json, false); },
    closeFromNative() { $("app").hidden = true; $("modal").hidden = true; }
  };

  /* Apercu dans un navigateur */
  const SAMPLE = { garage: "Garage Central", at: 1, deliv: 500, cash: 48500, bank: 1250000, maxOut: 2, ok: 1, msg: "",
    fee: { rep: 40, svb: 300, svw: 25, fl: 6, i7: 3, i30: 9, imin: 800, paint: 1500, plate: 25000, sellPct: 60 },
    veh: [
      { id: 1, m: 579, n: "Huntley", k: "Bleue", pl: "CW-0001", hp: 92, fu: 64, o: 0, s: 33000, p: 55000, km: 1240, w: 18, in: 5, im: 0, f: 1, c1: 79, c2: 1, sh: "", ow: 1, lk: 0, en: 0 },
      { id: 2, m: 522, n: "NRG-500", k: "", pl: "CW-0002", hp: 41, fu: 22, o: 1, s: 21000, p: 35000, km: 860, w: 52, in: 0, im: 0, f: 0, c1: 3, c2: 0, sh: "Sara_Lopez", ow: 1, lk: 1, en: 1 },
      { id: 3, m: 411, n: "Infernus", k: "", pl: "ANZO", hp: 100, fu: 100, o: 0, s: 57000, p: 95000, km: 12, w: 0, in: 0, im: 0, f: 0, c1: 1, c2: 1, sh: "", ow: 1, lk: 0, en: 0 },
      { id: 4, m: 455, n: "Flatbed", k: "", pl: "CW-0004", hp: 0, fu: 0, o: 0, s: 15000, p: 25000, km: 4300, w: 80, in: 0, im: 2500, f: 0, c1: 6, c2: 36, sh: "", ow: 1, lk: 0, en: 0 }
    ] };
  function demo(command) {
    const [t, id] = command.split(" ");
    const d = JSON.parse(JSON.stringify(S));
    const v = d.veh.find((x) => x.id === Number(id));
    if (v && t === "take") { v.o = 1; d.msg = v.n + " sorti."; }
    if (v && t === "store") { v.o = 0; d.msg = v.n + " rangé."; }
    if (v && t === "fav") v.f = Number(v.f) ? 0 : 1;
    if (v && t === "repair") { v.hp = 100; d.msg = "Réparé."; }
    d.ok = 1;
    setTimeout(() => apply(d, false), 160);
  }

  const b = bridge();
  if (b && b.ready) { try { b.ready(); } catch (_) { /* ignore */ } } else apply(SAMPLE, true);
})();
