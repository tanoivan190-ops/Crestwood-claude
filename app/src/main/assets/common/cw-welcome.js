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
  var LANGS = [["fr", "FR", "Français", "Langue de la ville"], ["en", "EN", "English", "City language: French"], ["pt", "PT", "Português", "Idioma da cidade: francês"]];

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

  function el(tag, cls, html) { var e = document.createElement(tag); if (cls) e.className = cls; if (html !== undefined) e.innerHTML = html; return e; }
  function esc(s) { return String(s).replace(/[&<>"]/g, function (c) { return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]; }); }
  var T = function (s) { return I ? I.t(s) : s; };

  function show() {
    var st = document.createElement("style");
    st.textContent = CSS;
    document.head.appendChild(st);
    var root = el("div", "cww");
    root.setAttribute("role", "dialog");
    root.setAttribute("aria-modal", "true");
    var card = el("div", "cww-card");
    root.appendChild(card);
    document.body.appendChild(root);
    var picked = (I && I.chosen) ? I.lang : "";

    function langStep() {
      card.innerHTML = "";
      var head = el("div", "cww-head", "<span class='cww-plate'>CW</span><div><h2>Choisis ta langue · Choose your language</h2><p>Tu pourras la changer plus tard dans les réglages.</p></div>");
      var body = el("div", "cww-body");
      LANGS.forEach(function (l) {
        var b = el("button", "cww-lang" + (picked === l[0] ? " on" : ""), "<i>" + l[1] + "</i><span><b>" + l[2] + "</b><small>" + esc(l[3]) + "</small></span>");
        b.type = "button";
        b.addEventListener("click", function () {
          picked = l[0];
          Array.prototype.forEach.call(body.querySelectorAll(".cww-lang"), function (x) { x.classList.toggle("on", x === b); });
          next.disabled = false;
        });
        body.appendChild(b);
      });
      var foot = el("div", "cww-foot");
      foot.appendChild(el("span", "cww-check"));
      var next = el("button", "cww-btn", "Continuer");
      next.type = "button";
      next.disabled = !picked;
      next.addEventListener("click", function () {
        if (!picked) return;
        savePref("cw.lang", picked);
        if (I && I.live) I.live(picked);
        rulesStep();
      });
      foot.appendChild(next);
      card.appendChild(head); card.appendChild(body); card.appendChild(foot);
    }

    function rulesStep() {
      var code = (I && I.lang) || "fr";
      var list = RULES[code] || RULES.fr;
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
