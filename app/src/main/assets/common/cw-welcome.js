/* ==========================================================================
   Crestwood RP v20 - accueil du premier lancement : 1) langue  2) reglement a accepter.
   Affiche une seule fois (localStorage "cw.lang" + "cw.rules"), sur l'ecran de chargement
   ou, a defaut, sur l'ecran de connexion. Ne bloque rien cote natif (le telechargement continue).
   Necessite common/i18n.js charge avant.
   ========================================================================== */
(function () {
  "use strict";
  var RULES_KEY = "cw.rules";
  var RULES_VERSION = "1";
  var accepted = false;
  try { accepted = window.localStorage.getItem(RULES_KEY) === RULES_VERSION; } catch (e) { accepted = false; }
  // [v21] copie native : le reglement n'est demande qu'UNE fois, meme si le stockage web est vide
  if (!accepted) { try { accepted = !!(window.AndroidLoading && window.AndroidLoading.getPref && window.AndroidLoading.getPref(RULES_KEY) === RULES_VERSION); } catch (e) { accepted = false; } }
  function savePref(k, v) {
    try { window.localStorage.setItem(k, v); } catch (e) { /* stockage indisponible */ }
    try { if (window.AndroidLoading && window.AndroidLoading.setPref) window.AndroidLoading.setPref(k, v); } catch (e) { /* pas de pont */ }
  }
  var I = window.CwI18n;
  function ready(fn) { if (document.readyState !== "loading") fn(); else document.addEventListener("DOMContentLoaded", fn); }

  var RULES = {
    fr: [
      ["Respect", "Aucune insulte hors jeu, aucun racisme ni harcèlement. On joue ensemble."],
      ["Reste dans ton personnage", "Parle et agis comme ton personnage. /me et /do décrivent tes actions."],
      ["Pas de meurtre gratuit", "Tuer ou frapper sans raison roleplay (freekill, DM) est interdit."],
      ["Pas de métagaming", "N'utilise pas en jeu une information apprise hors jeu (Discord, stream…)."],
      ["Pas de powergaming", "Pas d'actions impossibles ou forcées : laisse une chance aux autres."],
      ["Valeur de la vie", "Sous la menace d'une arme, ton personnage a peur et obéit."],
      ["Zones vertes", "Mairie, hôpital et points d'arrivée : aucune violence."],
      ["Jeu propre", "Pas de triche, de mod ni d'exploitation de bug. Signale-les au staff."],
      ["Nom réaliste", "Ton pseudo est au format Prénom_Nom."],
      ["Le staff tranche", "Un problème ? /report. Les décisions du staff s'appliquent."]
    ],
    en: [
      ["Respect", "No out-of-character insults, racism or harassment. We play together."],
      ["Stay in character", "Talk and act as your character. /me and /do describe your actions."],
      ["No random killing", "Killing or hitting without a roleplay reason (freekill, DM) is forbidden."],
      ["No metagaming", "Never use in game what you learned out of game (Discord, stream…)."],
      ["No powergaming", "No impossible or forced actions: give others a chance to react."],
      ["Value your life", "At gunpoint, your character is afraid and complies."],
      ["Green zones", "City hall, hospital and spawn points: no violence."],
      ["Fair play", "No cheats, mods or bug abuse. Report them to the staff."],
      ["Realistic name", "Your name follows the First_Last format."],
      ["Staff decides", "A problem? /report. Staff decisions apply."]
    ],
    pt: [
      ["Respeito", "Sem insultos fora do personagem, racismo ou assédio. Jogamos juntos."],
      ["Fique no personagem", "Fale e aja como seu personagem. /me e /do descrevem suas ações."],
      ["Sem matar à toa", "Matar ou bater sem motivo de roleplay (freekill, DM) é proibido."],
      ["Sem metagaming", "Não use no jogo o que soube fora dele (Discord, live…)."],
      ["Sem powergaming", "Nada de ações impossíveis ou forçadas: dê chance aos outros."],
      ["Valor da vida", "Sob a mira de uma arma, seu personagem tem medo e obedece."],
      ["Zonas verdes", "Prefeitura, hospital e pontos de chegada: nenhuma violência."],
      ["Jogo limpo", "Sem trapaça, mods ou abuso de bugs. Avise a staff."],
      ["Nome realista", "Seu nome segue o formato Nome_Sobrenome."],
      ["A staff decide", "Algum problema? /report. As decisões da staff valem."]
    ]
  };
  var FLAGS = {
    fr: "<svg viewBox='0 0 60 40'><rect width='20' height='40' fill='#0055A4'/><rect x='20' width='20' height='40' fill='#fff'/><rect x='40' width='20' height='40' fill='#EF4135'/></svg>",
    en: "<svg viewBox='0 0 60 40'><rect width='60' height='40' fill='#012169'/><path d='M0 0L60 40M60 0L0 40' stroke='#fff' stroke-width='7'/><path d='M0 0L60 40M60 0L0 40' stroke='#C8102E' stroke-width='3'/><path d='M30 0V40M0 20H60' stroke='#fff' stroke-width='11'/><path d='M30 0V40M0 20H60' stroke='#C8102E' stroke-width='6.5'/></svg>",
    pt: "<svg viewBox='0 0 60 40'><rect width='60' height='40' fill='#FF0000'/><rect width='24' height='40' fill='#006600'/><circle cx='24' cy='20' r='7' fill='#FFE000'/><circle cx='24' cy='20' r='4' fill='#fff'/><circle cx='24' cy='20' r='2' fill='#0033A0'/></svg>"
  };
  // [code, sigle, nom, eyebrow, titre, sous-titre, bouton]
  var LANGS = [
    ["fr", "FR", "Français", "Langue · Language", "Bienvenue à Crestwood", "Choisis la langue de ton aventure. Tu pourras la changer plus tard dans les réglages.", "Continuer"],
    ["en", "EN", "English", "Language · Langue", "Welcome to Crestwood", "Choose the language of your adventure. You can change it later in the settings.", "Continue"],
    ["pt", "PT", "Português", "Idioma · Language", "Bem-vindo a Crestwood", "Escolha o idioma da sua aventura. Você poderá mudá-lo depois nas configurações.", "Continuar"]
  ];
  function guessLang() {
    var n = ((navigator.language || navigator.userLanguage || "fr") + "").toLowerCase();
    return n.indexOf("pt") === 0 ? "pt" : (n.indexOf("en") === 0 ? "en" : "fr");
  }

  var CSS = ""
    + "@font-face{font-family:'CwMontW';src:url('../common/fonts/montserrat_bold.ttf') format('truetype');font-weight:700}"
    + "@font-face{font-family:'CwMontW';src:url('../common/fonts/montserrat_medium.ttf') format('truetype');font-weight:500}"
    + ".cww{position:fixed;inset:0;z-index:9999;display:flex;align-items:center;justify-content:center;padding:3vh 4vw;"
    + "background:rgba(6,8,11,.72);font-family:'CwMontW',Roboto,system-ui,sans-serif;color:#eef1f5;-webkit-user-select:none;user-select:none}"
    + ".cww-card{width:min(92vw,620px);max-height:94vh;display:flex;flex-direction:column;background:rgba(17,20,25,.94);border:1px solid rgba(255,255,255,.1);"
    + "border-radius:3px;box-shadow:0 20px 60px rgba(0,0,0,.55);overflow:hidden}"
    + ".cww-head{display:flex;align-items:center;gap:12px;padding:14px 16px 12px;border-bottom:1px solid rgba(255,255,255,.08)}"
    + ".cww-plate{flex:none;display:grid;place-items:center;min-width:34px;height:34px;padding:0 6px;border-radius:2px;background:#3b82ff;color:#fff;"
    + "font-weight:700;font-size:13px;box-shadow:inset 0 0 0 2px #3b82ff,inset 0 0 0 3px rgba(255,255,255,.62)}"
    + ".cww-head h2{margin:0;font-size:clamp(15px,4.2vh,19px);font-weight:700}"
    + ".cww-head p{margin:2px 0 0;font-size:clamp(10.5px,2.9vh,12.5px);font-weight:500;color:#8c94a0}"
    + ".cww-body{flex:1;min-height:0;overflow-y:auto;padding:12px 16px;-webkit-overflow-scrolling:touch}"
    + ".cww-lang{display:flex;align-items:center;gap:12px;width:100%;min-height:clamp(42px,12vh,54px);margin:0 0 8px;padding:0 14px;border-radius:2px;"
    + "border:1px solid rgba(255,255,255,.1);background:rgba(255,255,255,.04);color:#eef1f5;text-align:left;font:inherit;position:relative}"
    + ".cww-lang b{font-size:clamp(13px,3.6vh,16px)}.cww-lang small{display:block;font-weight:500;font-size:clamp(10px,2.6vh,11.5px);color:#8c94a0}"
    + ".cww-lang i{font-style:normal;flex:none;display:grid;place-items:center;width:34px;height:26px;border-radius:2px;border:1px solid rgba(255,255,255,.2);font-weight:700;font-size:12px}"
    + ".cww-lang.on{background:rgba(59,130,255,.16);border-color:#3b82ff}.cww-lang.on::before{content:'';position:absolute;left:0;top:0;bottom:0;width:3px;background:#3b82ff}"
    + ".cww-lang.on i{background:#3b82ff;border-color:#3b82ff;color:#fff}"
    + ".cww-rules{margin:0;padding:0;list-style:none;counter-reset:r}"
    + ".cww-rules li{counter-increment:r;display:grid;grid-template-columns:28px 1fr;gap:10px;padding:9px 0;border-bottom:1px solid rgba(255,255,255,.06)}"
    + ".cww-rules li:last-child{border-bottom:0}"
    + ".cww-rules li::before{content:counter(r);display:grid;place-items:center;width:26px;height:22px;border-radius:2px;background:rgba(59,130,255,.18);color:#9cc1ff;font-weight:700;font-size:12px}"
    + ".cww-rules b{display:block;font-size:clamp(12px,3.2vh,14px)}.cww-rules span{display:block;font-weight:500;font-size:clamp(10.5px,2.8vh,12.5px);line-height:1.45;color:#c4cad3}"
    + ".cww-foot{display:flex;align-items:center;gap:12px;padding:12px 16px 14px;border-top:1px solid rgba(255,255,255,.08)}"
    + ".cww-check{flex:1;display:flex;align-items:center;gap:10px;font-weight:500;font-size:clamp(11px,3vh,13px);color:#d3d8df}"
    + ".cww-check input{position:absolute;opacity:0;pointer-events:none}"
    + ".cww-box{flex:none;width:22px;height:22px;border-radius:2px;border:2px solid rgba(255,255,255,.35);display:grid;place-items:center}"
    + ".cww-check input:checked+.cww-box{background:#3b82ff;border-color:#3b82ff}"
    + ".cww-check input:checked+.cww-box::after{content:'';width:10px;height:5px;border-left:2.5px solid #fff;border-bottom:2.5px solid #fff;transform:translateY(-1px) rotate(-45deg)}"
    + ".cww-btn{flex:none;height:clamp(38px,10.5vh,46px);padding:0 22px;border:0;border-radius:2px;background:#3b82ff;color:#fff;font:700 clamp(12px,3.4vh,14px)/1 'CwMontW',sans-serif;"
    + "box-shadow:inset 0 0 0 2px #3b82ff,inset 0 0 0 3px rgba(255,255,255,.62)}"
    + ".cww-btn:disabled{background:#2a2f37;color:#7d8590;box-shadow:none}"
    + ".cww-btn:not(:disabled):active{background:#2a6be0}"
    + ".cww.out{opacity:0;transition:opacity .25s}"
    + "@media (prefers-reduced-motion:reduce){.cww.out{transition:none}}";

  var CSS2 = ""
    + ".cww{background:radial-gradient(ellipse at 20% 30%,rgba(59,130,255,.16),transparent 55%),rgba(4,6,9,.9)}"
    + ".cww-card{border-radius:16px;background:linear-gradient(180deg,rgba(59,130,255,.07),rgba(255,255,255,0) 30%),rgba(15,18,23,.96);box-shadow:inset 0 1px 0 rgba(255,255,255,.07),0 26px 70px rgba(0,0,0,.6)}"
    + ".cww-plate{border-radius:50%;box-shadow:none}"
    + ".cww-btn{border-radius:12px;background:linear-gradient(180deg,#4b8fff,#2f74f0);box-shadow:0 6px 18px rgba(59,130,255,.3)}"
    + ".cww-btn:disabled{background:#2a2f37;box-shadow:none}"
    + ".cww-rules li::before{border-radius:6px}"
    /* ecran de langue : colonne de drapeaux a gauche, apercu a droite */
    + ".cww-card.cwl{flex-direction:row;width:min(94vw,780px);height:min(86vh,430px)}"
    + ".cwl-rail{flex:none;display:flex;flex-direction:column;gap:clamp(8px,2.4vh,14px);align-items:center;justify-content:center;padding:clamp(10px,3vh,18px) clamp(12px,2.6vw,22px);"
    + "background:rgba(255,255,255,.03);border-right:1px solid rgba(255,255,255,.08)}"
    + ".cwl-flag{position:relative;display:flex;flex-direction:column;align-items:center;gap:6px;width:clamp(64px,13vw,92px);padding:clamp(7px,2vh,11px) 6px;border:1px solid rgba(255,255,255,.1);"
    + "border-radius:12px;background:rgba(255,255,255,.04);color:#c7cbd2;font:700 clamp(9px,2.6vh,11px)/1 'CwMontW',sans-serif;letter-spacing:.14em;transition:transform .15s,border-color .15s,background .15s}"
    + ".cwl-flag svg{display:block;width:100%;height:auto;border-radius:5px;box-shadow:0 2px 10px rgba(0,0,0,.45)}"
    + ".cwl-flag.on{border-color:#3b82ff;background:rgba(59,130,255,.16);color:#fff;transform:scale(1.04);box-shadow:0 0 0 3px rgba(59,130,255,.18)}"
    + ".cwl-main{flex:1;min-width:0;display:flex;flex-direction:column;justify-content:center;padding:clamp(16px,5vh,34px) clamp(16px,4vw,36px)}"
    + ".cwl-eyebrow{display:flex;align-items:center;gap:10px;font:700 clamp(8.5px,2.5vh,11px)/1 'CwMontW',sans-serif;letter-spacing:.3em;text-transform:uppercase;color:#9cc1ff}"
    + ".cwl-eyebrow::before{content:'';width:28px;height:2px;border-radius:2px;background:linear-gradient(90deg,#3b82ff,rgba(59,130,255,0))}"
    + ".cwl-title{margin:clamp(8px,2.4vh,14px) 0 0;font:900 clamp(20px,7.6vh,38px)/1.08 'CwMontW',sans-serif;color:#fff;letter-spacing:-.01em;transition:opacity .18s}"
    + ".cwl-sub{margin:clamp(6px,1.8vh,10px) 0 0;font:500 clamp(10.5px,3vh,14px)/1.5 'CwMontW',sans-serif;color:#aab1bc;max-width:46ch;transition:opacity .18s}"
    + ".cwl-title.sw,.cwl-sub.sw{opacity:0}"
    + ".cwl-go{margin-top:clamp(14px,4.4vh,26px);align-self:flex-start}"
    + "@media (max-width:560px){.cww-card.cwl{flex-direction:column;height:auto;max-height:94vh}.cwl-rail{flex-direction:row;border-right:0;border-bottom:1px solid rgba(255,255,255,.08)}}";

  function el(tag, cls, html) { var e = document.createElement(tag); if (cls) e.className = cls; if (html !== undefined) e.innerHTML = html; return e; }
  function esc(s) { return String(s).replace(/[&<>"]/g, function (c) { return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]; }); }
  var T = function (s) { return I ? I.t(s) : s; };

  function show() {
    var st = document.createElement("style");
    st.textContent = CSS + CSS2;
    document.head.appendChild(st);
    var root = el("div", "cww");
    root.setAttribute("role", "dialog");
    root.setAttribute("aria-modal", "true");
    var card = el("div", "cww-card");
    root.appendChild(card);
    document.body.appendChild(root);
    var picked = (I && I.chosen) ? I.lang : "";

    function langStep() {
      card.className = "cww-card cwl";
      card.innerHTML = "";
      if (!picked) picked = guessLang();
      var rail = el("div", "cwl-rail");
      var main = el("div", "cwl-main");
      var eyebrow = el("div", "cwl-eyebrow");
      var title = el("h2", "cwl-title");
      var sub = el("p", "cwl-sub");
      var go = el("button", "cww-btn cwl-go");
      go.type = "button";
      main.appendChild(eyebrow); main.appendChild(title); main.appendChild(sub); main.appendChild(go);

      function paint(code, animate) {
        var l = LANGS.filter(function (x) { return x[0] === code; })[0] || LANGS[0];
        function set() { eyebrow.textContent = l[3]; title.textContent = l[4]; sub.textContent = l[5]; go.textContent = l[6]; }
        if (!animate) { set(); return; }
        title.classList.add("sw"); sub.classList.add("sw");
        window.setTimeout(function () { set(); title.classList.remove("sw"); sub.classList.remove("sw"); }, 170);
      }
      LANGS.forEach(function (l) {
        var b = el("button", "cwl-flag" + (picked === l[0] ? " on" : ""), FLAGS[l[0]] + "<span>" + l[1] + "</span>");
        b.type = "button";
        b.setAttribute("aria-label", l[2]);
        b.addEventListener("click", function () {
          if (picked === l[0]) return;
          picked = l[0];
          Array.prototype.forEach.call(rail.querySelectorAll(".cwl-flag"), function (x) { x.classList.toggle("on", x === b); });
          paint(picked, true);
        });
        rail.appendChild(b);
      });
      go.addEventListener("click", function () {
        if (!picked) return;
        savePref("cw.lang", picked);
        if (I && I.live) I.live(picked);
        rulesStep();
      });
      card.appendChild(rail); card.appendChild(main);
      paint(picked, false);
    }

    function rulesStep() {
      var code = (I && I.lang) || "fr";
      var list = RULES[code] || RULES.fr;
      card.className = "cww-card";
      card.innerHTML = "";
      var head = el("div", "cww-head", "<span class='cww-plate'>CW</span><div><h2>" + esc(T("Règlement de Crestwood")) + "</h2><p>" + esc(T("Lis les règles de la ville avant d'entrer. Elles s'appliquent à tous les joueurs.")) + "</p></div>");
      var body = el("div", "cww-body");
      var ol = el("ol", "cww-rules");
      list.forEach(function (r) { ol.appendChild(el("li", "", "<div><b>" + esc(r[0]) + "</b><span>" + esc(r[1]) + "</span></div>")); });
      body.appendChild(ol);
      var foot = el("div", "cww-foot");
      var lab = el("label", "cww-check", "<input type='checkbox'><span class='cww-box'></span><span>" + esc(T("J'ai lu et j'accepte le règlement")) + "</span>");
      var go = el("button", "cww-btn", esc(T("Accepter et entrer")));
      go.type = "button";
      go.disabled = true;
      var box = lab.querySelector("input");
      box.addEventListener("change", function () { go.disabled = !box.checked; });
      go.addEventListener("click", function () {
        if (!box.checked) return;
        savePref(RULES_KEY, RULES_VERSION);
        var terms = document.getElementById("acceptTerms");
        if (terms) terms.checked = true;
        root.classList.add("out");
        window.setTimeout(function () { root.remove(); }, 260);
        try { window.dispatchEvent(new Event("cw:rules-accepted")); } catch (e) { /* ancien moteur */ }
      });
      foot.appendChild(lab); foot.appendChild(go);
      card.appendChild(head); card.appendChild(body); card.appendChild(foot);
    }

    if (!I || !I.chosen) langStep(); else rulesStep();
  }

  ready(function () {
    if (I) I.apply(document.body);
    if (!accepted || !I || !I.chosen) show();
    else { var terms = document.getElementById("acceptTerms"); if (terms) terms.checked = true; }
  });
  window.CwWelcome = { show: show };
})();
