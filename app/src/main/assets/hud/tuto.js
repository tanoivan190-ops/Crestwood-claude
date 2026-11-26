/* =========================================================
   CRESTWOOD v46 - TUTORIEL PAR APPEL TELEPHONIQUE (dans le HUD)
   Le telephone sonne (sonnerie assets/ringtone.mp3) : Maya, de la mairie, guide le joueur
   etape par etape (kit, banque, papiers, metier, vetements, garage, telephone, tablette, regles).
   Serveur -> HUD : dialogue 9860 "@hud" {"m":"tuto","st":"ring|call|missed|end|off","step":N,"has":0|1,"dist":m,"ok":1,"rew":1500}
   HUD -> serveur : /tuto accept | decline | next | skip | gps | stop          (Cw46.inc)
   ========================================================= */
(() => {
  "use strict";
  const send = (a) => { try { (window.SampHud || window.AndroidHUD)?.sendChatMessage?.("/tuto " + a); } catch (_) {} };
  const sync = () => { try { window.crestwoodSyncTouchBounds && window.crestwoodSyncTouchBounds(); } catch (_) {} };
  const esc = (s) => String(s == null ? "" : s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));

  const STEPS = [
    { t: "Bienvenue", lines: [
      "Allô ? Salut ! Moi c'est Maya, de l'accueil de la mairie de Crestwood.",
      "Bienvenue en ville ! Je vais te faire visiter l'essentiel, ça prend quelques minutes.",
      "Reste en ligne, je te guide étape par étape. Tu peux raccrocher quand tu veux." ] },
    { t: "Ton écran", lines: [
      "D'abord ton écran : le joystick à gauche pour marcher, les boutons à droite pour courir, sauter ou monter dans un véhicule.",
      "Quand une main bleue apparaît, touche-la : c'est comme ça qu'on utilise tout (portes, magasins, distributeurs, PNJ).",
      "Pour agir avec quelqu'un, approche-toi et touche-le deux fois : son menu d'interaction s'ouvre.",
      "Et si tu bloques, /aide liste tout ce que tu peux faire." ] },
    { t: "Kit de départ", obj: "Récupère ton kit de départ", ico: "gift", lines: [
      "La mairie t'offre un kit de départ : une Blista, un iPhone avec sa puce, de l'eau et 5 000 $.",
      "Accepte la fenêtre « Kit de départ » qui vient de s'ouvrir. Si tu l'as fermée, touche « Ouvrir »." ] },
    { t: "La banque", obj: "Va à la banque", ico: "bank", lines: [
      "Parfait. Direction la banque, je t'ai mis le GPS.",
      "Dépose ton liquide là-bas : si tu tombes inconscient, tu perds ce que tu as sur toi, pas ce qui est en banque." ] },
    { t: "Tes papiers", obj: "Fais ta carte d'identité", ico: "id", lines: [
      "Prochaine étape : tes papiers.",
      "Va voir l'agent de l'état civil, la première carte d'identité est gratuite. Sans papiers, la police peut t'embarquer." ] },
    { t: "Un travail", obj: "Va au point métier", ico: "job", lines: [
      "Il te faut un travail pour gagner ta vie.",
      "Je t'envoie au métier le plus proche : éboueur, routier, convoyeur de fonds, facteur… à toi de choisir.",
      "Plus tard, tu pourras entrer dans une faction (police, EMS, pompiers, justice…) ou dans un gang." ] },
    { t: "Ton style", obj: "Va au magasin de vêtements", ico: "shirt", lines: [
      "Un peu de style maintenant !",
      "Au magasin de vêtements, tu choisis chaque pièce et tu la vois en direct sur toi." ] },
    { t: "Ta voiture", obj: "Va au garage", ico: "car", lines: [
      "Ta Blista t'attend au garage.",
      "Approche-toi et sors ton véhicule : tu peux le ranger dans n'importe quel garage de la ville." ] },
    { t: "Ton téléphone", lines: [
      "Ton téléphone : WhatsApp, Instagram, TikTok, Wave pour envoyer de l'argent, Uber pour un taxi.",
      "En cas de pépin : 17 la police, 15 le SAMU, 18 les pompiers. Quelqu'un arrive s'il est en service.",
      "Ton numéro est dans les réglages du téléphone. Donne-le à tes amis !" ] },
    { t: "La tablette RP", lines: [
      "La tablette RP, c'est ton couteau suisse : inventaire, papiers, permis, véhicule, tenues, maison…",
      "« Mon groupe » te montre tes stats et, si tu entres dans un gang ou une faction, ses stats et ses gains.",
      "Et pour te détendre : casino, bourse, événements et petites annonces." ] },
    { t: "Les règles", lines: [
      "Dernière chose, la plus importante : le roleplay.",
      "Pas de tir sans raison, pas d'infos venues de l'extérieur du jeu, et on respecte la vie de son personnage.",
      "Au-dessus des têtes, tu vois un numéro du jour, pas un nom : présente-toi en jeu pour te faire connaître !",
      "Un souci ? Tablette RP > Support, le staff te répond." ] },
    { t: "C'est parti !", lines: [
      "Voilà, tu sais tout !",
      "Bon séjour à Crestwood. À très vite !" ] }
  ];
  const ICON = {
    gift: '<path d="M4 11h16v9H4zM3 7h18v4H3zM12 7v13M12 7c-2-4-6-3-5 0M12 7c2-4 6-3 5 0"/>',
    bank: '<path d="M3 10h18L12 4zM5 10v8M9 10v8M15 10v8M19 10v8M3 20h18"/>',
    id: '<rect x="3" y="5" width="18" height="14" rx="2"/><circle cx="9" cy="11" r="2.2"/><path d="M5.5 16c.8-1.8 5.2-1.8 6 0M14 10h4M14 13h4"/>',
    job: '<rect x="3" y="7" width="18" height="12" rx="2"/><path d="M9 7V5h6v2M3 12h18"/>',
    shirt: '<path d="M8 3 3 6l2 4 2-1v11h10V9l2 1 2-4-5-3a4 4 0 0 1-8 0z"/>',
    car: '<path d="M4 16v-4l2-5h12l2 5v4zM4 16v2h3v-2M17 16v2h3v-2"/><circle cx="7.5" cy="13" r="1"/><circle cx="16.5" cy="13" r="1"/>'
  };
  const PIN = '<path d="M12 21s-6.5-5.6-6.5-11a6.5 6.5 0 0 1 13 0c0 5.4-6.5 11-6.5 11Z"/><circle cx="12" cy="10" r="2.4"/>';

  const root = document.createElement("section");
  root.id = "cwTuto";
  root.className = "tt";
  root.hidden = true;
  root.innerHTML = `
    <div class="tt-head">
      <div class="tt-av"><span>M</span><i></i></div>
      <div class="tt-who"><b>Maya</b><small id="ttSub">Mairie de Crestwood</small></div>
      <div class="tt-time" id="ttTime"></div>
    </div>
    <div class="tt-ring" id="ttRing">
      <button type="button" class="tt-no" data-a="decline" aria-label="Refuser"><svg viewBox="0 0 24 24"><path d="M3 15c5-5 13-5 18 0l-2.5 2.5-3-1.5v-2.5a10 10 0 0 0-5 0V16l-3 1.5z"/></svg></button>
      <span class="tt-ringtxt">Appel entrant…</span>
      <button type="button" class="tt-yes" data-a="accept" aria-label="Répondre"><svg viewBox="0 0 24 24"><path d="M6.6 10.8a15 15 0 0 0 6.6 6.6l2.2-2.2c.3-.3.7-.4 1-.2 1.1.4 2.3.6 3.6.6.6 0 1 .4 1 1V20c0 .6-.4 1-1 1A17 17 0 0 1 3 4c0-.6.4-1 1-1h3.5c.6 0 1 .4 1 1 0 1.3.2 2.5.6 3.6.1.3 0 .7-.2 1z"/></svg></button>
    </div>
    <div class="tt-call" id="ttCall">
      <div class="tt-steps" id="ttSteps"></div>
      <p class="tt-say" id="ttSay"></p>
      <div class="tt-obj" id="ttObj" hidden><span class="tt-oico" id="ttOico"></span><div><b id="ttObjT"></b><small id="ttObjD"></small></div></div>
      <div class="tt-btns">
        <button type="button" class="tt-hang" data-a="stop" aria-label="Raccrocher"><svg viewBox="0 0 24 24"><path d="M3 15c5-5 13-5 18 0l-2.5 2.5-3-1.5v-2.5a10 10 0 0 0-5 0V16l-3 1.5z"/></svg></button>
        <button type="button" class="tt-b ghost" data-a="skip" id="ttSkip">Passer</button>
        <button type="button" class="tt-b ghost" data-a="gps" id="ttGps">GPS</button>
        <button type="button" class="tt-b" data-a="next" id="ttNext">Continuer</button>
      </div>
    </div>
    <div class="tt-end" id="ttEnd" hidden></div>`;
  document.body.appendChild(root);
  const $ = (id) => root.querySelector("#" + id);

  const ring = new Audio("assets/ringtone.mp3");
  ring.loop = true; ring.volume = 0.85;
  const chime = new Audio("assets/tuto-ok.mp3");
  chime.volume = 0.7;
  const playRing = () => { try { ring.currentTime = 0; const p = ring.play(); p && p.catch && p.catch(() => {}); } catch (_) {} };
  const stopRing = () => { try { ring.pause(); } catch (_) {} };
  window.CwRingtone = { play: playRing, stop: stopRing };   // aussi utilise par les appels du telephone (HUD)

  let st = "", step = -1, has = 0, dist = 0, lineIdx = 0, typing = 0, lineTimer = 0, callStart = 0, clock = 0, hideTimer = 0;

  function show() {
    clearTimeout(hideTimer);
    if (root.hidden) { root.hidden = false; requestAnimationFrame(() => root.classList.add("show")); }
    setTimeout(sync, 40);
  }
  function hide(ms) {
    clearTimeout(hideTimer);
    hideTimer = setTimeout(() => {
      root.classList.remove("show");
      setTimeout(() => { root.hidden = true; sync(); }, 260);
    }, ms || 0);
    stopRing();
    clearInterval(clock);
  }
  function fmtDist(m) { m = Number(m) || 0; return m >= 1000 ? (m / 1000).toFixed(1).replace(".", ",") + " km" : m + " m"; }
  function tick() {
    if (!callStart) return;
    const s = Math.floor((Date.now() - callStart) / 1000);
    $("ttTime").textContent = String(Math.floor(s / 60)).padStart(2, "0") + ":" + String(s % 60).padStart(2, "0");
  }
  function typeLine(text) {
    clearInterval(typing);
    clearTimeout(lineTimer);
    const el = $("ttSay");
    let i = 0;
    el.textContent = "";
    el.classList.add("typing");
    typing = setInterval(() => {
      i += 2;
      el.textContent = text.slice(0, i);
      if (i >= text.length) { clearInterval(typing); typing = 0; el.classList.remove("typing"); afterLine(); }
    }, 24);
  }
  function afterLine() {
    const s = STEPS[step];
    if (!s) return;
    if (lineIdx < s.lines.length - 1) {
      lineTimer = setTimeout(() => { lineIdx++; typeLine(s.lines[lineIdx]); }, 1500 + s.lines[lineIdx].length * 22);
    } else renderButtons(true);
  }
  function renderSteps() {
    const box = $("ttSteps");
    box.innerHTML = STEPS.map((_, i) => `<i class="${i < step ? "done" : i === step ? "on" : ""}"></i>`).join("");
  }
  function renderButtons(linesDone) {
    const s = STEPS[step] || {};
    const task = !!s.obj;
    $("ttNext").hidden = task || !linesDone;
    $("ttNext").textContent = step >= STEPS.length - 1 ? "Terminer" : "Continuer";
    $("ttSkip").hidden = !task;
    $("ttGps").hidden = !task;
    $("ttGps").textContent = step === 2 ? "Ouvrir" : "GPS";
    setTimeout(sync, 30);
  }
  function renderObj() {
    const s = STEPS[step] || {};
    const o = $("ttObj");
    if (!s.obj) { o.hidden = true; return; }
    o.hidden = false;
    $("ttOico").innerHTML = '<svg viewBox="0 0 24 24">' + (ICON[s.ico] || PIN) + "</svg>";
    $("ttObjT").textContent = s.obj;
    $("ttObjD").innerHTML = has ? '<svg viewBox="0 0 24 24">' + PIN + "</svg>" + esc(fmtDist(dist)) + " · suis le GPS" : "En attente…";
  }

  function onData(d) {
    const nst = String(d.st || "");
    const nstep = Number(d.step);
    has = Number(d.has) || 0;
    dist = Number(d.dist) || 0;
    if (nst === "ring") {
      if (st !== "ring") {
        root.className = "tt ringing" + (root.classList.contains("show") ? " show" : "");
        $("ttSub").textContent = "Mairie de Crestwood · appel entrant";
        $("ttTime").textContent = "";
        $("ttEnd").hidden = true;
        playRing();
      }
      st = "ring";
      show();
      return;
    }
    if (nst === "missed") {
      stopRing();
      st = "missed";
      root.className = "tt missed show";
      $("ttSub").textContent = "Appel manqué · elle rappellera";
      show();
      hide(4500);
      return;
    }
    if (nst === "off") { st = ""; step = -1; hide(0); return; }
    if (nst === "end") {
      stopRing();
      st = "end";
      root.className = "tt ending show";
      step = STEPS.length - 1;
      renderSteps();
      const rew = Number(d.rew) || 0;
      $("ttEnd").hidden = false;
      $("ttEnd").innerHTML = "<b>Tutoriel terminé</b><span>" + (rew ? "+" + rew.toLocaleString("fr-FR") + " $ offerts par la mairie" : "Tu peux le relancer depuis /aide") + "</span>";
      lineIdx = 0;
      typeLine(STEPS[step].lines.join(" "));
      $("ttNext").hidden = true; $("ttSkip").hidden = true; $("ttGps").hidden = true;
      try { chime.currentTime = 0; chime.play().catch(() => {}); } catch (_) {}
      show();
      hide(9000);
      return;
    }
    if (nst === "call") {
      stopRing();
      if (st !== "call") {
        callStart = Date.now();
        clearInterval(clock);
        clock = setInterval(tick, 1000);
        tick();
        $("ttSub").textContent = "Mairie de Crestwood · en ligne";
        $("ttEnd").hidden = true;
      }
      st = "call";
      root.className = "tt calling" + (root.classList.contains("show") ? " show" : "");
      if (Number(d.ok)) {
        root.classList.add("flash");
        setTimeout(() => root.classList.remove("flash"), 900);
        try { chime.currentTime = 0; chime.play().catch(() => {}); } catch (_) {}
      }
      if (nstep !== step) {
        step = Math.max(0, Math.min(STEPS.length - 1, nstep));
        lineIdx = 0;
        renderSteps();
        renderButtons(false);
        typeLine(STEPS[step].lines[0]);
      }
      renderObj();
      show();
    }
  }

  root.addEventListener("click", (e) => {
    const b = e.target.closest && e.target.closest("[data-a]");
    if (b) {
      const a = b.getAttribute("data-a");
      if (a === "accept" || a === "decline") stopRing();
      if (a === "stop") { hide(0); st = ""; }
      send(a);
      return;
    }
    // toucher la bulle : texte suivant tout de suite
    if (e.target.closest && e.target.closest("#ttSay") && st === "call") {
      const s = STEPS[step];
      if (!s) return;
      if (typing) { clearInterval(typing); typing = 0; $("ttSay").textContent = s.lines[lineIdx]; $("ttSay").classList.remove("typing"); afterLine(); return; }
      if (lineIdx < s.lines.length - 1) { clearTimeout(lineTimer); lineIdx++; typeLine(s.lines[lineIdx]); }
    }
  });

  window.CwBusExt = window.CwBusExt || {};
  window.CwBusExt.tuto = onData;
  window.CwTuto = { onData, isOpen: () => !root.hidden };
})();
