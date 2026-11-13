/* =========================================================
   CRESTWOOD v22 - MINI PAGE DES ANIMATIONS (dans le HUD, a droite, sans WebView en plus)
   Le petit bouton d'animations l'ouvre / la ferme. Toucher une tuile = /animq <cle> (Cw22.inc),
   "Arreter" = /animq stop. Appui long sur une tuile = favori (garde sur le telephone).
   Les cles sont celles de g_RpAnim (include/Rp.inc).
   ========================================================= */
(() => {
  "use strict";
  const ANIMS = [
    ["salut", "Saluer", "g", "👋"], ["pointer", "Pointer", "g", "👉"], ["crier", "Crier", "g", "📢"], ["non", "Dire non", "g", "🙅"],
    ["rire", "Rire", "g", "😂"], ["bras", "Bras croisés", "g", "🙎"], ["reflechir", "Réfléchir", "g", "🤔"], ["regarder", "Regarder autour", "g", "👀"],
    ["gang1", "Signe de gang 1", "g", "✌️"], ["gang2", "Signe de gang 2", "g", "🤟"], ["mainsair", "Mains en l'air", "g", "🙌"],
    ["assis", "Assis (homme)", "r", "🪑"], ["assisf", "Assise (femme)", "r", "💺"], ["allonge", "Allongé", "r", "🛌"], ["bronzer", "Bronzer", "r", "😎"],
    ["dormir", "Dormir", "r", "😴"], ["adosse", "Adossé", "r", "🧍"], ["fumer", "Fumer", "r", "🚬"], ["boire", "Boire", "r", "🥤"],
    ["telephone", "Téléphoner", "r", "📱"], ["taichi", "Tai-chi", "r", "🧘"],
    ["danse1", "Danse 1", "d", "💃"], ["danse2", "Danse 2", "d", "🕺"], ["danse3", "Danse 3", "d", "💃"], ["danse4", "Danse 4", "d", "🕺"],
    ["danse5", "Danse 5", "d", "💃"], ["rap", "Rap", "d", "🎤"],
    ["reparer", "Réparer", "w", "🔧"], ["fouiller", "Fouiller", "w", "🔍"], ["ordi", "Ordinateur", "w", "💻"], ["cpr", "Massage cardiaque", "w", "❤️"],
    ["garde", "Monter la garde", "w", "🛡️"], ["blesse", "Blessé au sol", "w", "🤕"]
  ];
  const CATS = [["fav", "★"], ["all", "Tout"], ["g", "Gestes"], ["r", "Repos"], ["d", "Danse"], ["w", "Travail"]];
  const FKEY = "cwAnimFav";
  let fav = [];
  try { fav = JSON.parse(localStorage.getItem(FKEY) || "[]"); if (!Array.isArray(fav)) fav = []; } catch (_) { fav = []; }
  let cat = fav.length ? "fav" : "all", cur = "", lock = 0;

  const root = document.createElement("aside");
  root.className = "cw22-anim";
  root.id = "cwAnimMini";
  root.hidden = true;
  root.setAttribute("aria-label", "Animations");
  root.innerHTML = `
    <header><b>Animations</b><small id="cwAnimCur">Touche une animation</small>
      <button type="button" class="cw22-anim-x" aria-label="Fermer"><svg viewBox="0 0 24 24"><path d="M6 6l12 12M18 6 6 18"/></svg></button></header>
    <nav class="cw22-anim-cats"></nav>
    <div class="cw22-anim-grid"></div>
    <footer><button type="button" class="cw22-anim-stop">Arrêter l'animation</button><small>Appui long = favori ★</small></footer>`;
  document.body.appendChild(root);
  const cats = root.querySelector(".cw22-anim-cats"), grid = root.querySelector(".cw22-anim-grid"), curLbl = root.querySelector("#cwAnimCur");
  const sync = () => { try { window.crestwoodSyncTouchBounds && window.crestwoodSyncTouchBounds(); } catch (_) {} };
  const bridge = () => window.SampHud || window.AndroidHUD || null;
  function send(cmd) { try { bridge()?.sendChatMessage?.(cmd); } catch (_) {} }

  function renderCats() {
    cats.replaceChildren();
    CATS.forEach(([k, label]) => {
      const b = document.createElement("button");
      b.type = "button";
      b.className = (k === cat ? "on" : "") + (k === "fav" ? " star" : "");
      b.textContent = label;
      b.addEventListener("click", () => { cat = k; renderCats(); renderGrid(); });
      cats.appendChild(b);
    });
  }
  function renderGrid() {
    grid.replaceChildren();
    const list = ANIMS.filter((a) => cat === "all" || (cat === "fav" ? fav.includes(a[0]) : a[2] === cat));
    if (!list.length) { const p = document.createElement("p"); p.className = "cw22-anim-empty"; p.textContent = "Aucun favori : appui long sur une animation pour l'ajouter."; grid.appendChild(p); return; }
    list.forEach((a) => {
      const b = document.createElement("button");
      b.type = "button";
      b.className = "cw22-anim-tile" + (a[0] === cur ? " on" : "") + (fav.includes(a[0]) ? " fav" : "");
      b.dataset.k = a[0];
      b.textContent = a[1];   // [v22b] par noms, sans emojis
      grid.appendChild(b);
    });
  }
  // toucher = jouer ; appui long = favori
  let pressT = 0, longDone = false;
  grid.addEventListener("pointerdown", (e) => {
    const b = e.target.closest(".cw22-anim-tile"); if (!b) return;
    e.stopPropagation();
    longDone = false;
    clearTimeout(pressT);
    pressT = setTimeout(() => {
      longDone = true;
      const k = b.dataset.k, i = fav.indexOf(k);
      if (i >= 0) fav.splice(i, 1); else fav.unshift(k);
      try { localStorage.setItem(FKEY, JSON.stringify(fav.slice(0, 18))); } catch (_) {}
      try { navigator.vibrate && navigator.vibrate(18); } catch (_) {}
      renderGrid();
    }, 480);
  });
  grid.addEventListener("pointerup", (e) => {
    clearTimeout(pressT);
    const b = e.target.closest(".cw22-anim-tile"); if (!b || longDone) return;
    const now = Date.now(); if (now < lock) return; lock = now + 500;
    cur = b.dataset.k;
    const a = ANIMS.find((x) => x[0] === cur);
    curLbl.textContent = "En cours : " + (a ? a[1] : cur);
    send("/animq " + cur);
    renderGrid();
  });
  grid.addEventListener("pointercancel", () => clearTimeout(pressT));
  root.querySelector(".cw22-anim-stop").addEventListener("click", () => { cur = ""; curLbl.textContent = "Touche une animation"; send("/animq stop"); renderGrid(); });
  root.querySelector(".cw22-anim-x").addEventListener("click", () => toggle(false));
  root.addEventListener("pointerdown", (e) => e.stopPropagation());

  function toggle(force) {
    const on = typeof force === "boolean" ? force : root.hidden;
    if (on) { renderCats(); renderGrid(); root.hidden = false; requestAnimationFrame(() => root.classList.add("show")); }
    else { root.classList.remove("show"); setTimeout(() => { root.hidden = true; sync(); }, 200); }
    document.querySelector(".animations-toggle-button")?.classList.toggle("on", on);
    setTimeout(sync, 30);
    return on;
  }
  window.CwAnim = { toggle, isOpen: () => !root.hidden };
})();
