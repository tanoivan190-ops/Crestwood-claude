/* [v15b] Garage Crestwood (style garage RP, bleu transparent).
   Serveur -> page : GarageApp.openFromNative(json) / onUpdateFromNative(json)
   Page -> serveur : SampGarage.action(type, '{"arg":"<id>"}') -> /garact take|store|sell|gps <id>
   Le serveur re-verifie tout (proprietaire, distance, argent). */
(() => {
  "use strict";
  const $ = (id) => document.getElementById(id);
  const bridge = () => window.SampGarage || null;

  const CAT = {
    bike: [448, 461, 462, 463, 468, 471, 481, 509, 510, 521, 522, 523, 581, 586],
    boat: [430, 446, 452, 453, 454, 472, 473, 484, 493, 595],
    air: [417, 425, 447, 460, 469, 476, 487, 488, 497, 511, 512, 513, 519, 520, 548, 553, 563, 577, 592, 593],
    truck: [403, 406, 407, 408, 413, 414, 416, 423, 427, 428, 431, 433, 437, 440, 443, 455, 456, 459, 482, 483, 498, 499, 508, 514, 515, 524, 525, 528, 532, 544, 552, 573, 578, 582, 588, 601, 609]
  };
  // reservoir (litres) et coffre (kg) par categorie
  const TANK = { car: 60, bike: 15, boat: 80, truck: 150, air: 200 };
  const TRUNK = { car: 250, bike: 20, boat: 300, truck: 1500, air: 500 };
  const CAT_FR = { car: "Voiture", bike: "Moto", boat: "Bateau", truck: "Camion", air: "Avion" };
  const catOf = (m) => {
    for (const [k, list] of Object.entries(CAT)) if (list.includes(Number(m))) return k;
    return "car";
  };
  const num = (n) => Math.round(Number(n) || 0).toLocaleString("fr-FR").replace(/[  ]/g, " ");
  const fcfa = (n) => num(n) + " FCFA";
  const esc = (s) => String(s ?? "").replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));

  const S = { garage: "Mes véhicules", at: 0, deliv: 500, cash: 0, bank: 0, maxOut: 2, veh: [] };
  let cat = "all", selId = null, busy = false;

  function parse(json) {
    if (typeof json === "object" && json) return json;
    try { return JSON.parse(json || "{}"); } catch (_) { return {}; }
  }

  function apply(json, open) {
    const d = parse(json);
    ["garage", "at", "deliv", "cash", "bank", "maxOut"].forEach((k) => { if (d[k] !== undefined) S[k] = d[k]; });
    if (Array.isArray(d.veh)) S.veh = d.veh;
    busy = false;
    $("app").hidden = false;
    if (open || !S.veh.some((v) => v.id === selId)) {
      const list = visible();
      selId = list.length ? list[0].id : (S.veh.length ? S.veh[0].id : null);
    }
    render();
    if (d.msg) toast(d.msg, Number(d.ok) === 0);
  }

  const visible = () => S.veh.filter((v) => cat === "all" || catOf(v.m) === cat);

  function render() {
    $("garKind").textContent = Number(S.at) ? "Garage" : "Garage · à distance";
    $("garName").textContent = S.garage || "Mes véhicules";
    $("cash").textContent = fcfa(S.cash);
    $("bank").textContent = fcfa(S.bank);
    document.querySelectorAll(".tab").forEach((t) => {
      const c = t.dataset.cat;
      const n = c === "all" ? S.veh.length : S.veh.filter((v) => catOf(v.m) === c).length;
      t.classList.toggle("on", c === cat);
      if (!t.dataset.label) t.dataset.label = t.textContent.trim();
      t.innerHTML = esc(t.dataset.label) + (n ? ` <b>${n}</b>` : "");
    });
    const list = visible();
    $("grid").innerHTML = list.length ? list.map((v) => `
      <button class="card${v.id === selId ? " on" : ""}" type="button" data-id="${v.id}">
        <h4>${esc(v.n)}</h4>
        <span class="meta"><span>${esc(v.plate || "")}</span><span class="${Number(v.out) ? "out" : ""}">${Number(v.out) ? "Sorti" : "Au garage"}</span></span>
        <svg viewBox="0 0 64 32" aria-hidden="true"><use href="#sil-${catOf(v.m)}"/></svg>
        <span class="pick">${v.id === selId ? "Sélectionné" : "Sélectionner"}</span>
      </button>`).join("")
      : `<div class="empty">${S.veh.length ? "Aucun véhicule dans cette catégorie." : "Tu n'as encore aucun véhicule. Achète-en un chez un concessionnaire."}</div>`;
    renderSel();
  }

  function renderSel() {
    const v = S.veh.find((x) => x.id === selId) || null;
    const c = v ? catOf(v.m) : "car";
    $("bigUse").setAttribute("href", "#sil-" + c);
    $("selName").textContent = v ? v.n : "Aucun véhicule";
    $("selPlate").textContent = v ? (v.plate || "") : "";
    const st = $("selState");
    st.textContent = v ? (Number(v.out) ? "Sorti" : "Au garage") + " · " + CAT_FR[c] : "—";
    st.classList.toggle("out", !!(v && Number(v.out)));
    const fuel = v ? Math.max(0, Math.min(100, Number(v.fuel) || 0)) : 0;
    const tank = TANK[c];
    $("fuelTxt").textContent = v ? `${Math.round(tank * fuel / 100)} / ${tank} L` : "—";
    $("fuelBar").style.width = fuel + "%";
    $("trunkTxt").textContent = v ? TRUNK[c] + " kg" : "—";
    $("trunkBar").style.width = v ? Math.min(100, TRUNK[c] / 15) + "%" : "0%";
    const hp = v ? Math.max(0, Math.min(100, Number(v.hp) || 0)) : 0;
    $("hpTxt").textContent = v ? hp + " %" : "—";
    $("hpBar").style.width = hp + "%";

    const outCount = S.veh.filter((x) => Number(x.out)).length;
    const at = Number(S.at) === 1;
    $("btnTake").innerHTML = at ? "Sortir le véhicule" : `Se faire livrer <small>${fcfa(S.deliv)}</small>`;
    $("btnTake").disabled = busy || !v || !!Number(v.out) || outCount >= (S.maxOut || 1);
    $("btnStore").disabled = busy || !v || !Number(v.out);
    $("btnGps").disabled = busy || !v || !Number(v.out);
    $("btnSell").innerHTML = v ? `Vendre le véhicule <small>${fcfa(v.sell)}</small>` : "Vendre le véhicule";
    $("btnSell").disabled = busy || !v || !!Number(v.out);
    $("hint").textContent = !v ? "Sélectionne le véhicule souhaité et utilise les options ci-dessous pour réaliser l'action."
      : at ? "Tu es au garage : le véhicule sort à la sortie du garage. Pour le ranger, ramène-le devant."
      : `Loin d'un garage : le véhicule est livré à côté de toi (${fcfa(S.deliv)}). Pour le ranger, il doit être à moins de 25 m de toi.`;
  }

  function send(type, id) {
    const b = bridge();
    busy = true;
    renderSel();
    if (b && b.action) {
      try { b.action(type, JSON.stringify({ arg: String(id ?? "") })); } catch (_) { busy = false; renderSel(); }
      setTimeout(() => { if (busy) { busy = false; renderSel(); } }, 3000);
    } else demo(type, id);
  }

  let toastTimer = 0;
  function toast(msg, err) {
    const t = $("toast");
    t.textContent = msg;
    t.classList.toggle("err", !!err);
    t.hidden = false;
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => { t.hidden = true; }, 3000);
  }

  $("tabs").addEventListener("click", (e) => {
    const t = e.target.closest(".tab");
    if (!t) return;
    cat = t.dataset.cat;
    const list = visible();
    if (list.length && !list.some((v) => v.id === selId)) selId = list[0].id;
    render();
  });
  $("grid").addEventListener("click", (e) => {
    const c = e.target.closest(".card");
    if (!c) return;
    selId = Number(c.dataset.id);
    render();
  });
  document.querySelector(".actions").addEventListener("click", (e) => {
    const b = e.target.closest("[data-act]");
    if (!b || b.disabled) return;
    const v = S.veh.find((x) => x.id === selId);
    if (!v) return;
    const act = b.dataset.act;
    if (act === "sell") {
      $("cfTitle").textContent = `Vendre ${v.n} ?`;
      $("cfText").textContent = `Tu recevras ${fcfa(v.sell)} en liquide. Cette action est définitive.`;
      $("confirm").hidden = false;
      $("cfYes").onclick = () => { $("confirm").hidden = true; send("sell", v.id); };
      return;
    }
    send(act, v.id);
  });
  $("cfNo").addEventListener("click", () => { $("confirm").hidden = true; });
  $("confirm").addEventListener("click", (e) => { if (e.target.id === "confirm") $("confirm").hidden = true; });
  $("close").addEventListener("click", () => { const b = bridge(); if (b && b.close) b.close(); else $("app").hidden = true; });

  window.GarageApp = {
    openFromNative(json) { apply(json, true); },
    onUpdateFromNative(json) { apply(json, false); },
    closeFromNative() { $("app").hidden = true; }
  };

  /* Apercu dans un navigateur : donnees d'exemple */
  const SAMPLE = {
    garage: "Mes véhicules", at: 0, deliv: 500, cash: 48500, bank: 1250000, maxOut: 2,
    veh: [
      { id: 1, m: 579, n: "Huntley", plate: "CW-0001", hp: 100, fuel: 88, out: 0, sell: 33000 },
      { id: 2, m: 522, n: "NRG-500", plate: "CW-0002", hp: 74, fuel: 50, out: 1, sell: 21000 },
      { id: 3, m: 411, n: "Infernus", plate: "CW-0003", hp: 100, fuel: 100, out: 0, sell: 57000 },
      { id: 4, m: 473, n: "Dinghy", plate: "CW-0004", hp: 90, fuel: 70, out: 0, sell: 9000 },
      { id: 5, m: 455, n: "Flatbed", plate: "CW-0005", hp: 62, fuel: 35, out: 0, sell: 15000 }
    ]
  };
  function demo(type, id) {
    const d = JSON.parse(JSON.stringify(S));
    const v = d.veh.find((x) => x.id === id);
    if (type === "take" && v) { v.out = 1; d.msg = `${v.n} livré à côté de toi.`; }
    if (type === "store" && v) { v.out = 0; d.msg = `${v.n} rangé.`; }
    if (type === "gps" && v) d.msg = `Position de ${v.n} marquée sur ton GPS.`;
    if (type === "sell" && v) { d.cash += v.sell; d.veh = d.veh.filter((x) => x.id !== id); d.msg = `${v.n} vendu.`; }
    d.ok = 1;
    setTimeout(() => apply(d, false), 150);
  }

  const b = bridge();
  if (b && b.ready) { try { b.ready(); } catch (_) { /* ignore */ } } else apply(SAMPLE, true);
})();
