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
    ["garde", "Monter la garde", "w", "🛡️"], ["blesse", "Blessé au sol", "w", "🤕"],
    /* [v32] animations du MTA (EC-Anim) : /animq m1..m144 (include/MtaAnims.inc), toutes avec les animations du jeu */
["m1", "Discussion 1", "c"], ["m2", "Discussion 2", "c"], ["m3", "Discussion 3", "c"], ["m4", "Discussion 4", "c"], ["m5", "Discussion 5", "c"], ["m6", "Discussion 6", "c"], ["m7", "Discussion 7", "c"], ["m8", "Discussion 8", "c"], ["m9", "Garde à la batte", "x"], ["m10", "Garde au sabre", "x"], ["m11", "Prendre un coup 1", "x"], ["m12", "Prendre un coup 2", "x"], ["m13", "Prendre un coup 3", "x"], ["m14", "Prendre un coup 4", "x"], ["m15", "Célébration gym", "x"], ["m16", "Boxe dans le vide", "x"], ["m17", "Gifle", "x"], ["m18", "Coup de pied porte", "x"], ["m19", "Braquer", "x"], ["m20", "Braquer (2)", "x"], ["m21", "Allongé, fumer 1", "s"], ["m22", "Allongé, fumer 2", "s"], ["m23", "Fumer debout 1", "s"], ["m24", "Fumer debout 2", "s"], ["m25", "Fumer debout 3", "s"], ["m26", "Fumer debout 4", "s"], ["m27", "Allumer une cigarette", "s"], ["m28", "Fumer", "s"], ["m29", "Tapoter la cendre", "s"], ["m30", "Casino 1", "k"], ["m31", "Casino 2", "k"], ["m32", "Casino 3", "k"], ["m33", "Casino 4", "k"], ["m34", "Casino 5", "k"], ["m35", "Casino 6", "k"], ["m36", "Casino 7", "k"], ["m37", "Casino 8", "k"], ["m38", "Vérifier sa tenue 1", "v"], ["m39", "Vérifier sa tenue 2", "v"], ["m40", "Vérifier sa tenue 3", "v"], ["m41", "Vérifier sa tenue 4", "v"], ["m42", "Vérifier sa tenue 5", "v"], ["m43", "Mort 1", "m"], ["m44", "Mort 2", "m"], ["m45", "Mort 3", "m"], ["m46", "Mort 4", "m"], ["m47", "Mort 5", "m"], ["m48", "Mort 6", "m"], ["m49", "Mort 7", "m"], ["m50", "Mort 8", "m"], ["m51", "Mort 9", "m"], ["m52", "Danse 1", "d"], ["m53", "Danse 2", "d"], ["m54", "Danse 3", "d"], ["m55", "Danse 4", "d"], ["m56", "Danse 5", "d"], ["m57", "Danse 6", "d"], ["m58", "Danse 7", "d"], ["m59", "Danse 8", "d"], ["m60", "Danse 9", "d"], ["m61", "Danse 10", "d"], ["m62", "Danse 11", "d"], ["m63", "Danse 12", "d"], ["m64", "Danse 13", "d"], ["m65", "Danse 14", "d"], ["m66", "Danse 15", "d"], ["m67", "Danse 16", "d"], ["m68", "Danse 17", "d"], ["m69", "Danse 18", "d"], ["m70", "Danse 19", "d"], ["m71", "Danse 20", "d"], ["m72", "Danse 21", "d"], ["m73", "Signe de gang 1", "g"], ["m74", "Signe de gang 2", "g"], ["m75", "Signe de gang 3", "g"], ["m76", "Signe de gang 4", "g"], ["m77", "Signe de gang 5", "g"], ["m78", "Signe de gang 6", "g"], ["m79", "Signe de gang 7", "g"], ["m80", "Serrer la main", "g"], ["m81", "Dire bonjour", "g"], ["m82", "Faire coucou", "g"], ["m83", "Bisou 1", "b"], ["m84", "Bisou 2", "b"], ["m85", "Bisou 3", "b"], ["m86", "Bisou (femme) 1", "b"], ["m87", "Bisou (femme) 2", "b"], ["m88", "Bisou (femme) 3", "b"], ["m89", "Assis confortable", "a"], ["m90", "Se relever", "a"], ["m91", "Allongé 1", "a"], ["m92", "Allongé 2", "a"], ["m93", "Allongé 3", "a"], ["m94", "Allongé 4", "a"], ["m95", "Allongé 5", "a"], ["m96", "Allongé 6", "a"], ["m97", "Assis confortable (2)", "a"], ["m98", "Assis à l'ordinateur", "a"], ["m99", "Assis sur une chaise", "a"], ["m100", "Assis, parler 1", "a"], ["m101", "Assis, parler 2", "a"], ["m102", "Accoudé au sol", "a"], ["m103", "Assis, jambes ballantes", "a"], ["m104", "Assis jambes croisées 1", "a"], ["m105", "Assis jambes croisées 2", "a"], ["m106", "Allongé sur le ventre", "a"], ["m107", "Assis par terre 1", "a"], ["m108", "Assis par terre 2", "a"], ["m109", "Poser une bombe", "v"], ["m110", "Ramasser un objet", "v"], ["m111", "Poser un objet", "v"], ["m112", "Faire signe de venir", "v"], ["m113", "Accroupi, appeler", "v"], ["m114", "Accroupi", "v"], ["m115", "Célébrer", "v"], ["m116", "Regarder sa montre", "v"], ["m117", "Réfléchir", "v"], ["m118", "Secouer la tête", "v"], ["m119", "Donner de l'argent", "v"], ["m120", "Manger un burger", "v"], ["m121", "Manger du poulet", "v"], ["m122", "Manger une pizza", "v"], ["m123", "Debout 1", "v"], ["m124", "Debout 2", "v"], ["m125", "Adossée (femme)", "v"], ["m126", "Approuver", "v"], ["m127", "Avoir peur", "v"], ["m128", "Se rendre 1", "v"], ["m129", "Se rendre 2", "v"], ["m130", "Se rendre 3", "v"], ["m131", "Se rendre 4", "v"], ["m132", "Massage cardiaque", "v"], ["m133", "Se detendre", "v"], ["m134", "Boire une bière", "v"], ["m135", "Au téléphone", "v"], ["m136", "Applaudir 1", "v"], ["m137", "Applaudir 2", "v"], ["m138", "Appuyé sur le bras 1", "v"], ["m139", "Appuyé sur le bras 2", "v"], ["m140", "Assis", "v"], ["m141", "Pied pose", "v"], ["m142", "Pose femme", "v"], ["m143", "Accroupi (2)", "v"], ["m144", "Salut militaire", "v"]
  ];
  const CATS = [["fav", "★"], ["all", "Tout"], ["g", "Gestes"], ["c", "Discuter"], ["r", "Repos"], ["a", "Assis"], ["d", "Danse"], ["s", "Fumer"],
    ["b", "Bisous"], ["x", "Combat"], ["k", "Casino"], ["m", "Blessé"], ["p", "Sport"], ["w", "Travail"], ["v", "Divers"]];
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
