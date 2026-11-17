/* =========================================================
   CRESTWOOD v25 - LOCATION DE VELO (BMX)  -  page dans le HUD
   Serveur -> HUD : dialogue 9860 "@hud" {"m":"bike", price, min, fine, renting, left, cash, bank, ok, msg} | {"m":"bike","close":1}
   HUD -> serveur : /c25 bike rent cash|bank  |  /c25 bike return  |  /c25 bike close     (Cw25.inc)
   ========================================================= */
(() => {
  "use strict";
  const esc = (s) => String(s == null ? "" : s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
  const money = (n) => (Number(n) || 0).toLocaleString("fr-FR") + " $";
  const send = (cmd) => { try { (window.SampHud || window.AndroidHUD)?.sendChatMessage?.("/c25 bike " + cmd); } catch (_) {} };
  const sync = () => { try { window.crestwoodSyncTouchBounds && window.crestwoodSyncTouchBounds(); } catch (_) {} };
  const BIKE = '<svg viewBox="0 0 48 32" aria-hidden="true"><circle cx="10" cy="22" r="8"/><circle cx="38" cy="22" r="8"/><path d="M10 22 18 8h14l6 14M18 8l6 14h-14M32 8l-8 14M16 4h6M30 8l-1-4h5"/></svg>';

  const root = document.createElement("section");
  root.id = "cwBike";
  root.className = "bk";
  root.hidden = true;
  root.innerHTML = `
    <header class="bk-top">
      <img class="bk-logo" src="assets/project-logo.png" alt="Crestwood RP">
      <i class="bk-sep"></i>
      <span class="bk-ico">${BIKE}</span>
      <div class="bk-title"><h1>Location <em>de vélo</em></h1><p>Explorez la ville à deux roues !</p></div>
      <button type="button" class="bk-x" aria-label="Fermer"><svg viewBox="0 0 24 24"><path d="M6 6l12 12M18 6 6 18"/></svg></button>
    </header>
    <div class="bk-stage">
      <figure class="bk-photo"><img src="img/bmx.webp" alt="BMX Crestwood orange"></figure>
      <article class="bk-card">
        <h2>BMX</h2>
        <p class="bk-tag">Simple, rapide et stylé.</p>
        <dl class="bk-specs">
          <div><dt><svg viewBox="0 0 24 24"><path d="M4.5 17a8 8 0 1 1 15 0"/><path d="m12 13 4-4"/><circle cx="12" cy="13" r="1.4"/></svg>Vitesse</dt><dd>Élevée</dd></div>
          <div><dt><svg viewBox="0 0 24 24"><path d="M12 3.5l2.6 5.3 5.9.9-4.3 4.1 1 5.8L12 16.9l-5.2 2.7 1-5.8-4.3-4.1 5.9-.9z"/></svg>Style</dt><dd>Moderne</dd></div>
          <div><dt><svg viewBox="0 0 24 24"><path d="M3 12V4h8l10 10-8 8z"/><circle cx="7.5" cy="8.5" r="1.4"/></svg>Catégorie</dt><dd>Vélo urbain</dd></div>
        </dl>
        <div class="bk-tiles">
          <div class="bk-tile"><svg viewBox="0 0 24 24"><path d="M8 7h8l3 5v7a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2v-7zM9 7 8 3h8l-1 4"/><path d="M12 11v7M14.2 12.6c-.5-.7-1.3-1-2.2-1-1.2 0-2 .6-2 1.5 0 2 4.4 1 4.4 3 0 .9-.9 1.5-2.2 1.5-1 0-1.8-.4-2.3-1"/></svg><div><small>Prix de location</small><b id="bkPrice">500 $</b></div></div>
          <div class="bk-tile"><svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="8.5"/><path d="M12 7.5V12l3 2"/></svg><div><small id="bkDurLbl">Durée</small><b id="bkDur">1 heure</b></div></div>
        </div>
      </article>
    </div>
    <footer class="bk-bar">
      <div class="bk-tip"><span>i</span><p id="bkTip">Astuce : rendez le vélo avant la fin du temps pour éviter une amende !</p></div>
      <div class="bk-pay" id="bkPay">
        <button type="button" data-p="cash" class="on"><small>Liquide</small><b id="bkCash">0 $</b></button>
        <button type="button" data-p="bank"><small>Carte</small><b id="bkBank">0 $</b></button>
      </div>
      <button type="button" class="bk-go" id="bkGo">${BIKE}<span>Louer ce vélo</span></button>
    </footer>
    <button type="button" class="bk-back"><svg viewBox="0 0 24 24"><path d="M9 14 4 9l5-5"/><path d="M4 9h10.5a5.5 5.5 0 0 1 0 11H11"/></svg><span>Retour</span></button>
    <div class="bk-toast" id="bkToast" hidden></div>`;
  document.body.appendChild(root);
  const $ = (id) => root.querySelector("#" + id);

  let d = {}, pay = "cash", busy = false, timer = 0;

  function show() { if (root.hidden) { root.hidden = false; requestAnimationFrame(() => root.classList.add("show")); } setTimeout(sync, 40); }
  function hide(tell) {
    clearInterval(timer);
    if (tell) send("close");
    root.classList.remove("show");
    setTimeout(() => { root.hidden = true; sync(); }, 220);
  }
  root.querySelector(".bk-x").addEventListener("click", () => hide(true));
  root.querySelector(".bk-back").addEventListener("click", () => hide(true));
  root.addEventListener("pointerdown", (e) => e.stopPropagation());
  $("bkPay").addEventListener("click", (e) => {
    const b = e.target.closest("button[data-p]");
    if (!b) return;
    pay = b.dataset.p;
    Array.prototype.forEach.call($("bkPay").children, (x) => x.classList.toggle("on", x === b));
    render();
  });
  $("bkGo").addEventListener("click", () => {
    if (busy) return;
    busy = true;
    setTimeout(() => { busy = false; render(); }, 1500);
    if (Number(d.renting)) send("return"); else send("rent " + pay);
    render();
  });

  function toast(t, ok) {
    const el = $("bkToast");
    el.textContent = t;
    el.className = "bk-toast " + (ok ? "ok" : "bad");
    el.hidden = !t;
    clearTimeout(toast.t);
    toast.t = setTimeout(() => { el.hidden = true; }, 3500);
  }
  function fmtLeft(s) { s = Math.max(0, s | 0); const m = Math.floor(s / 60), r = s % 60; return m + " min " + String(r).padStart(2, "0") + " s"; }

  function render() {
    const renting = Number(d.renting) === 1;
    const price = Number(d.price) || 500;
    $("bkPrice").textContent = money(price);
    $("bkCash").textContent = money(d.cash);
    $("bkBank").textContent = money(d.bank);
    if (renting) {
      $("bkDurLbl").textContent = "Temps restant";
      $("bkDur").textContent = fmtLeft(d.left);
    } else {
      $("bkDurLbl").textContent = "Durée";
      const min = Number(d.min) || 60;
      $("bkDur").textContent = min >= 60 && min % 60 === 0 ? (min / 60) + (min === 60 ? " heure" : " heures") : min + " min";
    }
    $("bkTip").textContent = renting
      ? "Roulez jusqu'à un point de location (marqueur vélo) pour rendre le vélo automatiquement."
      : "Astuce : rendez le vélo avant la fin du temps pour éviter une amende de " + money(d.fine || 750) + " !";
    root.classList.toggle("renting", renting);
    $("bkPay").hidden = renting;
    const can = renting || (pay === "cash" ? Number(d.cash) >= price : Number(d.bank) >= price);
    const go = $("bkGo");
    go.querySelector("span").textContent = renting ? "Rendre le vélo" : busy ? "Location…" : can ? "Louer ce vélo" : "Fonds insuffisants";
    go.classList.toggle("off", !can || busy);
  }

  (window.CwBusExt = window.CwBusExt || {}).bike = function (x) {
    if (!x) return;
    if (Number(x.close)) return hide(false);
    d = x;
    busy = false;
    if (d.msg) toast(d.msg, Number(d.ok) === 1);
    if (!Number(d.cash) && Number(d.bank) >= (Number(d.price) || 500) && root.hidden) {
      pay = "bank";
      Array.prototype.forEach.call($("bkPay").children, (b) => b.classList.toggle("on", b.dataset.p === "bank"));
    }
    render();
    clearInterval(timer);
    if (Number(d.renting)) timer = setInterval(() => { d.left = Math.max(0, (Number(d.left) || 0) - 1); render(); }, 1000);
    show();
  };
  window.CwBike = { hide: () => hide(true), isOpen: () => !root.hidden };
})();
