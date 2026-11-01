/* Peche Crestwood
   Serveur -> page : FishApp.openFromNative(json) / onUpdateFromNative(json)
   Page -> serveur : SampFish.action(type, '{"arg":"..."}') -> /fishact <type> <arg>
   Deroule : cast -> attente (wait ms donne par le serveur) -> touche -> jauge -> hook <precision 0-100> */
(function () {
  "use strict";
  const $ = (id) => document.getElementById(id);
  const fmt = (n) => Number(n || 0).toLocaleString("fr-FR");
  const bridge = () => window.SampFish || null;
  // meme ordre que g_FishKinds dans le gamemode : [nom, prix FCFA / kg, rarete]
  const KINDS = [["Sardine", 60, 0], ["Maquereau", 90, 0], ["Tilapia", 110, 0], ["Poisson-chat", 130, 1], ["Bar", 180, 1],
    ["Dorade", 220, 1], ["Capitaine", 300, 2], ["Thon", 350, 2], ["Espadon", 480, 3]];
  const RARE = ["Commun", "Peu commun", "Rare", "Légendaire"];
  const FISH_SVG = '<svg viewBox="0 0 24 24"><path d="M2 12c2.5-4 6-6 10-6 3.6 0 6.4 1.7 8 4l2-3v10l-2-3c-1.6 2.3-4.4 4-8 4-4 0-7.5-2-10-6Zm13-1.5a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3Z"/></svg>';
  const ICON = {
    cast: '<svg viewBox="0 0 24 24"><path d="M4 20 18 4"/><path d="M18 4v9a3 3 0 1 1-3-3"/></svg>',
    hook: '<svg viewBox="0 0 24 24"><path d="M12 3v10a4 4 0 1 1-4-4"/><path d="M12 3l3 3"/></svg>',
    stop: '<svg viewBox="0 0 24 24"><path d="M6 6l12 12M18 6 6 18"/></svg>',
    rod: '<svg viewBox="0 0 24 24"><path d="M4 20 18 4"/><circle cx="7" cy="17" r="2"/></svg>'
  };
  const SAMPLE = { rod: 1, rodPrice: 500, cash: 4200, state: "idle", wait: 0, max: 20, kg: 18450, best: [7, 26400], spot: 1, ok: 1, msg: "", value: 2862,
    cooler: [[4, 3200], [0, 260], [6, 8400], [1, 900]], caught: 0 };

  let S = null, phase = "idle", waitTimer = 0, biteTimer = 0, raf = 0, gaugeStart = 0, zone = { a: 40, b: 60 }, toastTimer = 0, catchTimer = 0;

  function send(type, arg) {
    const b = bridge();
    if (!b) { demo(type, arg); return; }
    try { b.action(type, JSON.stringify({ arg: arg == null ? "" : String(arg) })); } catch (e) { console.log(e); }
  }
  function toast(msg, ok) {
    if (!msg) return;
    const t = $("toast");
    t.textContent = msg; t.classList.toggle("err", !ok); t.hidden = false;
    clearTimeout(toastTimer); toastTimer = setTimeout(() => { t.hidden = true; }, 3600);
  }
  const kg = (g) => (Number(g || 0) / 1000).toLocaleString("fr-FR", { maximumFractionDigits: 1 }) + " kg";
  const priceOf = (k, g) => Math.floor((Number(g) * (KINDS[k] ? KINDS[k][1] : 0)) / 1000);

  function setPhase(p, title, text) {
    phase = p;
    const lake = $("lake");
    lake.className = "glass lake " + (p === "bite" ? "bite" : p === "cast" ? "cast" : "idle");
    if (title != null) $("stTitle").textContent = title;
    if (text != null) $("stText").textContent = text;
    $("gauge").hidden = p !== "bite";
    renderControls();
  }
  function clearTimers() { clearTimeout(waitTimer); clearTimeout(biteTimer); cancelAnimationFrame(raf); }

  function renderControls() {
    const c = $("controls");
    if (!S) return;
    if (!Number(S.rod)) {
      c.innerHTML = `<button class="chip-btn gold big" type="button" data-act="buyrod"><span class="ico">${ICON.rod}</span>Acheter une canne · ${fmt(S.rodPrice)} FCFA</button>`;
      return;
    }
    if (phase === "bite") c.innerHTML = `<button class="chip-btn big" type="button" data-act="hookNow"><span class="ico">${ICON.hook}</span>Ferrer !</button>`;
    else if (phase === "cast") c.innerHTML = `<button class="chip-btn ghost big" type="button" data-act="cancel"><span class="ico">${ICON.stop}</span>Remonter la ligne</button>`;
    else c.innerHTML = `<button class="chip-btn big" type="button" data-act="cast" ${Number(S.spot) ? "" : "disabled"}><span class="ico">${ICON.cast}</span>Lancer la ligne</button>`;
  }

  function renderSide() {
    const list = Array.isArray(S.cooler) ? S.cooler : [];
    $("cash").textContent = fmt(S.cash) + " FCFA";
    $("coolerCount").textContent = list.length + " / " + (S.max || 20);
    $("statKg").textContent = kg(S.kg);
    const best = Array.isArray(S.best) && S.best[0] >= 0 && KINDS[S.best[0]] ? KINDS[S.best[0]][0] + " · " + kg(S.best[1]) : "—";
    $("statBest").textContent = best;
    $("value").textContent = fmt(S.value) + " FCFA";
    $("sellBtn").disabled = !list.length;
    $("cooler").innerHTML = list.length ? list.map(([k, g]) => {
      const kd = KINDS[k] || ["?", 0, 0];
      return `<div class="fish r${kd[2]}"><span class="fi">${FISH_SVG}</span><span><b>${kd[0]}</b><small>${kg(g)} · ${RARE[kd[2]]}</small></span><span class="v num">${fmt(priceOf(k, g))}</span></div>`;
    }).join("") : '<div class="empty">Ta glacière est vide.<br>Lance ta ligne pour commencer.</div>';
  }

  function showCatch(c) {
    if (!Array.isArray(c) || !KINDS[c[0]]) return;
    const kd = KINDS[c[0]];
    const box = $("catchBox");
    box.innerHTML = `<div class="catch r${kd[2]}">${FISH_SVG}<b>${kd[0]}</b><small>${kg(c[1])} · ${RARE[kd[2]]} · ${fmt(priceOf(c[0], c[1]))} FCFA</small></div>`;
    clearTimeout(catchTimer);
    catchTimer = setTimeout(() => { box.innerHTML = ""; }, 3200);
  }

  /* ---------- jauge de ferrage ---------- */
  function startBite() {
    const width = 16 + Math.random() * 10;                // zone verte de 16 a 26 %
    const start = 10 + Math.random() * (80 - width);
    zone = { a: start, b: start + width };
    $("zone").style.left = zone.a + "%";
    $("zone").style.width = width + "%";
    setPhase("bite", "Ça mord !", "Appuie sur « Ferrer » quand le curseur est dans la zone verte.");
    if (navigator.vibrate) { try { navigator.vibrate([60, 40, 60]); } catch (_) { /* ignore */ } }
    gaugeStart = performance.now();
    const speed = 0.9 + Math.random() * 0.6;              // allers-retours par seconde
    const loop = (t) => {
      const x = ((t - gaugeStart) / 1000) * speed;
      const pos = (1 - Math.cos(x * Math.PI)) * 50;        // 0..100, va-et-vient
      $("cursor").style.left = pos + "%";
      $("cursor").dataset.pos = pos;
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);
    biteTimer = setTimeout(() => hook(true), 3200);      // trop lent : le poisson repart
  }
  function hook(timeout) {
    if (phase !== "bite") return;
    clearTimers();
    let q = 0;
    if (!timeout) {
      const pos = Number($("cursor").dataset.pos || 0);
      const mid = (zone.a + zone.b) / 2, half = (zone.b - zone.a) / 2;
      const d = Math.abs(pos - mid);
      q = d <= half ? Math.round(100 - (d / half) * 45) : Math.max(0, Math.round(29 - (d - half) * 2));
    }
    setPhase("idle", "…", "");
    send("hook", q);
  }

  function apply(d, fromOpen) {
    if (typeof d === "string") { try { d = JSON.parse(d); } catch (_) { d = null; } }
    if (!d || typeof d !== "object") return;
    S = d;
    $("app").hidden = false;
    renderSide();
    clearTimers();
    if (d.state === "cast") {
      setPhase("cast", "Ligne à l'eau…", "Attends que le bouchon s'agite.");
      waitTimer = setTimeout(startBite, Math.max(400, Number(d.wait) || 3000) + 150);
    } else {
      setPhase("idle", Number(d.rod) ? "Prêt à pêcher" : "Il te faut une canne", Number(d.rod) ? "Lance ta ligne, attends que ça morde, puis ferre au bon moment." : "Achète une canne à pêche (une seule fois) pour commencer.");
      if (!Number(d.spot)) $("stText").textContent = "Rapproche-toi du ponton pour pêcher.";
    }
    if (Array.isArray(d.caught)) showCatch(d.caught);
    if (d.msg) toast(d.msg, Number(d.ok) === 1);
    else if (fromOpen) { /* rien */ }
  }

  document.addEventListener("click", (e) => {
    const b = e.target.closest("[data-act]");
    if (!b || b.disabled) return;
    const a = b.dataset.act;
    if (a === "hookNow") { hook(false); return; }
    if (a === "cancel") clearTimers();
    send(a);
  });
  $("close").addEventListener("click", () => { clearTimers(); const b = bridge(); if (b && b.close) b.close(); });

  window.FishApp = {
    openFromNative(json) { apply(json, true); },
    onUpdateFromNative(json) { apply(json, false); },
    closeFromNative() { clearTimers(); $("app").hidden = true; }
  };

  /* Apercu dans un navigateur : simulation locale du serveur */
  function demo(type, arg) {
    const d = Object.assign({}, S, { msg: "", ok: 1, caught: 0, state: "idle" });
    if (type === "cast") { d.state = "cast"; d.wait = 1800; }
    if (type === "hook") {
      const q = Number(arg);
      if (q < 30) { d.ok = 0; d.msg = "Raté ! Le poisson s'est décroché."; }
      else { const k = q > 85 ? 7 : q > 60 ? 4 : 1, g = 900 + q * 120; d.cooler = [[k, g]].concat(d.cooler || []); d.caught = [k, g]; d.value = (d.value || 0) + priceOf(k, g); d.msg = "Belle prise !"; }
    }
    if (type === "sell") { d.cash += d.value; d.msg = "Poisson vendu : +" + fmt(d.value) + " FCFA."; d.cooler = []; d.value = 0; }
    if (type === "buyrod") { d.rod = 1; d.msg = "Canne achetée."; }
    setTimeout(() => apply(d, false), 120);
  }

  const b = bridge();
  if (b && b.ready) { try { b.ready(); } catch (_) { /* ignore */ } } else apply(SAMPLE, true);
})();
