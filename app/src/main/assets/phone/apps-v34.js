/* ============================================================
   [v34] Uber refait facon vraie appli : carte plein ecran + feuille du bas
   ------------------------------------------------------------
   - plus de faux raccourcis (Maison / Travail...) ni de prix inventes :
     le prix affiche = tarif du serveur (Uber.inc) x distance jusqu'au point pose sur la carte
   - Passager : "Ou allez-vous ?" -> toucher la carte -> choisir UberX / Comfort / Black / Moto -> Commander
   - Chauffeur : gros bouton GO, gains en haut, course proposee avec compte a rebours, taximetre
   Utilise l'etat de apps-v31.js (CwUber.state : positions) et apps-v32.js (CwUberApp.state : chauffeur).
   ============================================================ */
(function () {
  "use strict";
  var send = function (t, o) { if (window.PhoneNet && window.PhoneNet.send) window.PhoneNet.send(t, o); };
  var esc = function (s) { return String(s == null ? "" : s).replace(/[&<>"']/g, function (c) { return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]; }); };
  var money = function (n) { return String(Math.round(Number(n) || 0)).replace(/\B(?=(\d{3})+(?!\d))/g, " ") + " $"; };
  var say = function (m) { if (m && typeof toast === "function") toast(m); };

  // memes tarifs que server/basicgm/include/Uber.inc (g_UbBase / g_UbPerKm)
  var TARIF = { x: [150, 500], comfort: [300, 750], black: [600, 1200], moto: [100, 380] };
  var INFO = {
    x: { name: "UberX", seats: "4", desc: "Course de tous les jours", img: "uber/uber_x.png" },
    comfort: { name: "Comfort", seats: "4", desc: "Voiture récente, plus d'espace", img: "uber/uber_confort.png" },
    black: { name: "Black", seats: "4", desc: "Berline haut de gamme", img: "uber/uber_black.png" },
    moto: { name: "Moto", seats: "1", desc: "Rapide dans les bouchons", img: "uber/uber_motorcycle.png" }
  };
  var ORDER = ["x", "comfort", "black", "moto"];

  function U() { return (window.CwUber && window.CwUber.state) || {}; }
  function APP() { return (window.CwUberApp && window.CwUberApp.state) || { mode: "rider", drv: null }; }
  function dest() { var u = U(); return u.pin || (u.tx !== null && u.tx !== undefined ? [u.tx, u.ty] : null); }
  function km() {
    var u = U(), d = dest();
    if (!d || !u.has) return null;
    return Math.hypot(d[0] - u.x, d[1] - u.y) * 1.25 / 1000;   // x1,25 : les routes ne sont pas en ligne droite
  }
  function estimate(id) { var t = TARIF[id] || TARIF.x, k = km(); return k === null ? null : t[0] + Math.round(k * t[1]); }

  /* ---------------- styles ---------------- */
  var css = [
    "#scr-uber .atop{display:none!important}",
    "#scr-uber{background:#0d0f12!important}",
    "#uberRoot{position:relative!important;display:block!important;background:#0d0f12!important;font-family:-apple-system,'SF Pro Display',Roboto,system-ui,sans-serif}",
    "#uberRoot #uberMap{position:absolute!important;inset:0!important;height:auto!important;background:#141821!important}",
    "#uberRoot #uberMapIn>img:first-child{filter:grayscale(.55) brightness(.62) contrast(1.15) saturate(.8)!important}",
    "#uberRoot #uberHint{display:none!important}",
    "#uberRoot #uberMe{width:18px!important;height:18px!important;margin:-9px 0 0 -9px!important;background:#276ef1!important;box-shadow:0 0 0 8px rgba(39,110,241,.22),0 2px 6px rgba(0,0,0,.5)!important}",
    "#uberRoot #uberPin{font-size:0!important;width:18px!important;height:18px!important;margin:-9px 0 0 -9px!important;background:#000;border:4px solid #fff;border-radius:3px;box-shadow:0 2px 8px rgba(0,0,0,.6)}",
    "#uberRoot #uberDrv{height:30px!important}",
    ".ub-route{position:absolute;left:0;top:0;pointer-events:none;overflow:visible}",
    ".ub-pulse{position:absolute;width:12px;height:12px;margin:-6px 0 0 -6px;border-radius:50%;border:2px solid #276ef1;animation:ubRing 1.6s ease-out infinite;pointer-events:none}",
    "@keyframes ubRing{0%{transform:scale(1);opacity:1}100%{transform:scale(14);opacity:0}}",
    ".ub-top{position:absolute;left:12px;right:12px;top:52px;display:flex;justify-content:space-between;align-items:center;z-index:3;pointer-events:none}",
    ".ub-seg{display:flex;padding:3px;border-radius:99px;background:#fff;box-shadow:0 4px 14px rgba(0,0,0,.45);pointer-events:auto}",
    ".ub-seg b{padding:7px 14px;border-radius:99px;font-size:12.5px;font-weight:700;color:#545454;cursor:pointer}",
    ".ub-seg b.on{background:#000;color:#fff}",
    ".ub-earn{padding:8px 14px;border-radius:99px;background:#000;color:#fff;font-size:14px;font-weight:800;box-shadow:0 4px 14px rgba(0,0,0,.45);pointer-events:auto}",
    ".ub-earn small{display:block;font-size:9px;font-weight:600;color:#afafaf;letter-spacing:.6px;text-align:center}",
    ".ub-wrap{position:absolute;left:0;right:0;bottom:0;z-index:4}",
    ".ub-sheet{position:relative;background:#fff;color:#000;border-radius:20px 20px 0 0;padding:8px 16px 26px;box-shadow:0 -8px 24px rgba(0,0,0,.35);max-height:440px;overflow:auto}",
    ".ub-sheet.dark{background:#141414;color:#fff}",
    ".ub-grab{width:38px;height:5px;border-radius:9px;background:#d5d5d5;margin:0 auto 10px}",
    ".ub-sheet.dark .ub-grab{background:#3a3a3a}",
    ".ub-h{font-size:22px;font-weight:800;letter-spacing:-.4px;margin:2px 0 10px}",
    ".ub-sub{font-size:13px;color:#6b6b6b;line-height:1.35}",
    ".ub-sheet.dark .ub-sub{color:#a6a6a6}",
    ".ub-where{display:flex;align-items:center;gap:12px;background:#eee;border-radius:12px;padding:13px 14px;font-size:17px;font-weight:700;cursor:pointer}",
    ".ub-where i{width:9px;height:9px;background:#000;flex:none}",
    ".ub-where em{margin-left:auto;font-style:normal;font-size:12px;font-weight:700;background:#fff;border-radius:99px;padding:5px 10px}",
    ".ub-dest{display:flex;align-items:center;gap:12px;padding:4px 2px 12px;border-bottom:1px solid #eee;margin-bottom:6px}",
    ".ub-dest .dots{display:flex;flex-direction:column;align-items:center;gap:3px;flex:none}",
    ".ub-dest .dots i{width:8px;height:8px;border-radius:50%;background:#000}",
    ".ub-dest .dots i+i{border-radius:0}",
    ".ub-dest .dots s{width:2px;height:16px;background:#000}",
    ".ub-dest .lines{flex:1;min-width:0;font-size:14px;font-weight:600}",
    ".ub-dest .lines div+div{margin-top:12px}",
    ".ub-x{flex:none;width:30px;height:30px;border-radius:50%;background:#eee;display:grid;place-items:center;font-weight:800;cursor:pointer}",
    ".ub-ride{display:flex;align-items:center;gap:10px;padding:8px 8px;border-radius:12px;border:2px solid transparent;cursor:pointer}",
    ".ub-ride.on{border-color:#000}",
    ".ub-ride img{width:62px;height:40px;object-fit:contain;flex:none}",
    ".ub-ride .n{font-size:16px;font-weight:800}",
    ".ub-ride .n span{font-size:12px;font-weight:600;color:#6b6b6b;margin-left:4px}",
    ".ub-ride .p{margin-left:auto;text-align:right;font-size:15px;font-weight:800;white-space:nowrap}",
    ".ub-ride .p small{display:block;font-size:10.5px;font-weight:600;color:#6b6b6b}",
    ".ub-btn{display:block;width:100%;margin-top:12px;padding:14px;border:0;border-radius:10px;background:#000;color:#fff;font-size:16px;font-weight:700;text-align:center;cursor:pointer}",
    ".ub-btn.grey{background:#eee;color:#000}",
    ".ub-sheet.dark .ub-btn.grey{background:#2a2a2a;color:#fff}",
    ".ub-btn.blue{background:#276ef1}",
    ".ub-btn.green{background:#06c167}",
    ".ub-bar{height:4px;border-radius:4px;background:#eee;overflow:hidden;margin:12px 0 4px}",
    ".ub-sheet.dark .ub-bar{background:#2a2a2a}",
    ".ub-bar i{display:block;height:100%;width:35%;border-radius:4px;background:#276ef1;animation:ubSlide 1.3s ease-in-out infinite}",
    "@keyframes ubSlide{0%{transform:translateX(-100%)}100%{transform:translateX(300%)}}",
    ".ub-drv{display:flex;align-items:center;gap:12px;margin:12px 0 4px}",
    ".ub-av{width:48px;height:48px;border-radius:50%;background:#e8e8e8;display:grid;place-items:center;font-weight:800;font-size:19px;flex:none}",
    ".ub-plate{margin-left:auto;text-align:right}",
    ".ub-plate b{display:inline-block;padding:4px 8px;border-radius:6px;background:#f3f3f3;border:1px solid #d6d6d6;font-size:14px;font-weight:800;letter-spacing:1px}",
    ".ub-plate small{display:block;font-size:12px;color:#6b6b6b;margin-top:3px}",
    ".ub-meter{display:flex;justify-content:space-between;align-items:flex-end;margin-top:10px}",
    ".ub-meter small{display:block;font-size:10.5px;font-weight:700;color:#8a8a8a;letter-spacing:.8px}",
    ".ub-meter b{font-size:30px;font-weight:900;letter-spacing:-.8px}",
    ".ub-go{position:absolute;left:50%;bottom:calc(100% + 16px);transform:translateX(-50%);width:74px;height:74px;border-radius:50%;border:0;background:#276ef1;color:#fff;font-size:22px;font-weight:900;box-shadow:0 0 0 6px rgba(39,110,241,.25),0 6px 18px rgba(0,0,0,.5);cursor:pointer}",
    ".ub-go:active{transform:translateX(-50%) scale(.94)}",
    ".ub-state{display:flex;align-items:center;justify-content:space-between;gap:10px}",
    ".ub-state .t{font-size:18px;font-weight:800}",
    ".ub-dot{width:10px;height:10px;border-radius:50%;background:#06c167;box-shadow:0 0 0 4px rgba(6,193,103,.25);display:inline-block;margin-right:8px}",
    ".ub-link{font-size:13px;font-weight:700;color:#276ef1;cursor:pointer}",
    ".ub-req{text-align:center}",
    ".ub-ring{width:84px;height:84px;margin:4px auto 8px;position:relative}",
    ".ub-ring svg{width:100%;height:100%;transform:rotate(-90deg)}",
    ".ub-ring b{position:absolute;inset:0;display:grid;place-items:center;font-size:26px;font-weight:900}",
    ".ub-tag{display:inline-block;padding:4px 10px;border-radius:99px;background:#276ef1;color:#fff;font-size:11px;font-weight:800;letter-spacing:.6px}"
  ].join("\n");
  var st = document.createElement("style"); st.id = "ub34css"; st.textContent = css; document.head.appendChild(st);

  /* ---------------- rendu ---------------- */
  function top(mode, d) {
    var seg = '<div class="ub-seg"><b class="' + (mode === "rider" ? "on" : "") + '" onclick="CwUberApp.mode(\'rider\')">Courses</b>' +
      '<b class="' + (mode === "driver" ? "on" : "") + '" onclick="CwUberApp.mode(\'driver\')">Conduire</b></div>';
    var right = mode === "driver" ? '<div class="ub-earn"><small>SOLDE</small>' + money(d && d.bal) + '</div>' : "";
    return '<div class="ub-top">' + seg + right + '</div>';
  }

  function sheet(inner, dark, extra) {
    return '<div class="ub-wrap">' + (extra || "") + '<div class="ub-sheet' + (dark ? " dark" : "") + '"><div class="ub-grab"></div>' + inner + '</div></div>';
  }

  function riderIdle() {
    var d = dest(), k = km();
    var h = "";
    if (!d) {
      h += '<div class="ub-h">Où allez-vous ?</div>';
      h += '<div class="ub-where" onclick="CwUber34.hint()"><i></i>Touche la carte<em>Destination</em></div>';
      h += '<div class="ub-sub" style="margin:10px 2px 6px">Pose ta destination sur la carte pour voir le prix exact. Tu peux aussi commander sans destination et la donner au chauffeur.</div>';
    } else {
      h += '<div class="ub-dest"><div class="dots"><i></i><s></s><i></i></div><div class="lines"><div>Ma position</div><div>Point sur la carte · ' + (k !== null ? k.toFixed(1) + " km" : "") + '</div></div>' +
        '<div class="ub-x" onclick="CwUber34.clearDest()">✕</div></div>';
    }
    ORDER.forEach(function (id) {
      var r = INFO[id], on = uberSel === id, e = estimate(id), t = TARIF[id];
      h += '<div class="ub-ride' + (on ? " on" : "") + '" onclick="uberPick(\'' + id + '\')">' +
        '<img src="' + r.img + '" alt=""><div style="min-width:0"><div class="n">' + r.name + '<span>👤' + r.seats + '</span></div><div class="ub-sub">' + r.desc + '</div></div>' +
        '<div class="p">' + (e !== null ? money(e) : "dès " + money(t[0])) + '<small>' + money(t[1]) + ' / km</small></div></div>';
    });
    h += '<button class="ub-btn" onclick="uberRequest()">Commander ' + (INFO[uberSel] || INFO.x).name + '</button>';
    return sheet(h);
  }

  function driverRow(t) {
    var n = String((t && t.driver) || "…"), ini = (n.replace(/[^A-Za-z]/g, "")[0] || "?").toUpperCase();
    return '<div class="ub-drv"><div class="ub-av">' + ini + '</div><div><div style="font-weight:800;font-size:16px">' + esc(n) + '</div><div class="ub-sub">' + esc((t && t.car) || "Véhicule") + '</div></div>' +
      '<div class="ub-plate"><b>' + esc((t && t.plate) || "—") + '</b><small>' + esc(t && t.ride ? t.ride.name : "") + '</small></div></div>';
  }

  function riderBody() {
    var t = uberTrip;
    if (uberState === "searching") {
      return sheet('<div class="ub-h" style="margin-bottom:2px">Recherche d\'un chauffeur</div><div class="ub-sub">Ta demande est envoyée au chauffeur le plus proche.</div>' +
        '<div class="ub-bar"><i></i></div><button class="ub-btn grey" onclick="uberCancel()">Annuler</button>');
    }
    if (uberState === "enroute") {
      return sheet('<div class="ub-state"><div class="t">Ton chauffeur arrive</div><div style="font-weight:800">' + esc(t && t.min) + ' min</div></div>' +
        driverRow(t) + '<div class="ub-sub" style="margin-top:6px">Suis sa voiture sur la carte. Reste au bord de la route.</div>' +
        '<button class="ub-btn grey" onclick="uberCancel()">Annuler la course</button>');
    }
    if (uberState === "arrived") {
      return sheet('<div class="ub-state"><div class="t"><span class="ub-dot"></span>Ton chauffeur est là</div></div>' + driverRow(t) +
        '<div class="ub-sub" style="margin-top:6px">Monte dans la voiture : le taximètre démarre tout seul.</div>' +
        '<button class="ub-btn grey" onclick="uberCancel()">Annuler la course</button>');
    }
    if (uberState === "trip") {
      var kmv = t && t.ride ? t.min : "";
      return sheet('<div class="ub-state"><div class="t">En course</div><span class="ub-tag">' + esc(t && t.ride ? t.ride.name : "Uber") + '</span></div>' + driverRow(t) +
        '<div class="ub-meter"><div><small>TAXIMÈTRE</small><b>' + money(t && t.ride ? t.ride.price : 0) + '</b></div><div style="text-align:right"><small>DISTANCE</small><div style="font-weight:800;font-size:17px">' + esc(String(kmv).replace(" ·", "")) + '</div></div></div>');
    }
    if (uberState === "done") {
      return sheet('<div style="text-align:center;padding:6px 0"><div style="width:54px;height:54px;border-radius:50%;background:#06c167;color:#fff;display:grid;place-items:center;font-size:28px;font-weight:900;margin:0 auto 8px">✓</div>' +
        '<div class="ub-h" style="margin:0">Tu es arrivé</div><div style="font-size:30px;font-weight:900;margin:6px 0 2px">' + money(t && t.ride ? t.ride.price : 0) + '</div>' +
        '<div class="ub-sub">Payé en liquide · ' + esc(t && t.ride ? t.ride.name : "") + '</div></div><button class="ub-btn" onclick="uberDone()">OK</button>');
    }
    return riderIdle();
  }

  function driverBody(d) {
    d = d || {};
    var on = Number(d.on) === 1;
    if (d.req) {
      var left = Math.max(0, Number(d.req.left) || 0), frac = Math.min(1, left / 20), C = 2 * Math.PI * 36;
      var dist = Number(d.req.dist) >= 1000 ? (d.req.dist / 1000).toFixed(1) + " km" : (Number(d.req.dist) || 0) + " m";
      return sheet('<div class="ub-req"><span class="ub-tag">' + esc(d.req.type) + '</span>' +
        '<div class="ub-ring"><svg viewBox="0 0 80 80"><circle cx="40" cy="40" r="36" fill="none" stroke="#2a2a2a" stroke-width="6"/>' +
        '<circle cx="40" cy="40" r="36" fill="none" stroke="#276ef1" stroke-width="6" stroke-linecap="round" stroke-dasharray="' + C.toFixed(1) + '" stroke-dashoffset="' + (C * (1 - frac)).toFixed(1) + '"/></svg><b>' + left + '</b></div>' +
        '<div style="font-size:20px;font-weight:800">' + esc(d.req.name) + '</div><div class="ub-sub">à ' + dist + ' de toi · 80 % du prix pour toi</div>' +
        '<button class="ub-btn blue" onclick="CwUberApp.act(\'accept\')">Accepter</button>' +
        '<button class="ub-btn grey" onclick="CwUberApp.act(\'decline\')">Refuser</button></div>', true);
    }
    if (d.ride) {
      var trip = d.ride.st === "trip";
      return sheet('<div class="ub-state"><div class="t">' + (trip ? "Course en cours" : "Va chercher le client") + '</div><span class="ub-tag">' + esc(d.ride.type) + '</span></div>' +
        '<div class="ub-drv"><div class="ub-av" style="background:#2a2a2a;color:#fff">' + esc((String(d.ride.name || "?")[0] || "?").toUpperCase()) + '</div><div><div style="font-weight:800;font-size:16px">' + esc(d.ride.name) + '</div>' +
        '<div class="ub-sub">' + (trip ? (Number(d.ride.dest) ? "Destination sur ton GPS" : "Le client t'indique la route") : "Suis le GPS jusqu'au client") + '</div></div></div>' +
        (trip ? '<div class="ub-meter"><div><small>TAXIMÈTRE</small><b>' + money(d.ride.fare) + '</b></div><div style="text-align:right"><small>DISTANCE</small><div style="font-weight:800;font-size:17px">' + Number(d.ride.km || 0).toFixed(2) + ' km</div></div></div>' +
          '<button class="ub-btn green" onclick="CwUberApp.act(\'end\')">Terminer la course</button>' : ""), true);
    }
    if (on) {
      return sheet('<div class="ub-state"><div class="t"><span class="ub-dot"></span>Tu es en ligne</div><span class="ub-link" onclick="CwUberApp.act(\'off\')">Passer hors ligne</span></div>' +
        '<div class="ub-sub" style="margin-top:4px">Recherche de courses autour de toi…' + (d.veh && d.veh !== "-" ? " · " + esc(d.veh) : "") + '</div><div class="ub-bar"><i></i></div>' +
        '<div class="ub-sub">' + (Number(d.rides) || 0) + ' course' + (Number(d.rides) > 1 ? "s" : "") + ' faites · ' + money(d.bal) + ' à retirer</div>', true);
    }
    var canGo = d.can === undefined || Number(d.can) === 1;
    return sheet('<div class="ub-state"><div class="t">Tu es hors ligne</div>' + (Number(d.bal) > 0 ? '<span class="ub-link" onclick="CwUberApp.act(\'withdraw\')">Retirer ' + money(d.bal) + '</span>' : "") + '</div>' +
      '<div class="ub-sub" style="margin-top:6px">' + (canGo ? "Appuie sur GO pour recevoir des courses. Le client le plus proche t'est proposé, tu as 20 s pour accepter. Le taximètre démarre quand il monte." :
        "Monte au volant de ta voiture personnelle (garage) ou d'un taxi, puis appuie sur GO.") + '</div>', true,
      '<button class="ub-go" onclick="CwUberApp.act(\'on\')">GO</button>');
  }

  window.uberMapHTML = (function (orig) {
    return function () {
      var m = orig ? orig() : '<div id="uberMap"><div id="uberMapIn"></div></div>';
      // trace de l'itineraire + onde "recherche" ajoutes dans la carte
      return m.replace('<div id="uberMe"', '<svg class="ub-route" id="ubRoute"></svg><i class="ub-pulse" id="ubPulse" style="display:none"></i><div id="uberMe"');
    };
  })(window.uberMapHTML);

  window.uberBodyHTML = function () {
    var a = APP(), mode = a.mode === "driver" ? "driver" : "rider";
    return window.uberMapHTML() + top(mode, a.drv) + (mode === "driver" ? driverBody(a.drv) : riderBody());
  };
  window.uberHTML = function () {
    return '<div id="uberRoot" style="flex:1;min-height:0;overflow:hidden">' + window.uberBodyHTML() + '</div>';
  };

  /* trace : moi -> destination (ou moi -> voiture / client) */
  var MAP = 2560, WORLD = 6000, ZOOM = 0.62;
  function toPx(x, y) { return { x: (x + 3000) / WORLD * MAP * ZOOM, y: (3000 - y) / WORLD * MAP * ZOOM }; }
  setInterval(function () {
    var svg = document.getElementById("ubRoute");
    if (!svg || !svg.closest("#uberRoot")) return;
    var u = U(), size = Math.round(MAP * ZOOM);
    svg.setAttribute("width", size); svg.setAttribute("height", size);
    var lines = "";
    if (u.has) {
      var me = toPx(u.x, u.y), d = dest();
      if (u.dx !== null && u.dx !== undefined) {
        var o = toPx(u.dx, u.dy);
        lines += '<line x1="' + me.x + '" y1="' + me.y + '" x2="' + o.x + '" y2="' + o.y + '" stroke="#fff" stroke-width="3" stroke-dasharray="2 6" stroke-linecap="round"/>';
      }
      if (d && APP().mode !== "driver") {
        var q = toPx(d[0], d[1]);
        lines += '<line x1="' + me.x + '" y1="' + me.y + '" x2="' + q.x + '" y2="' + q.y + '" stroke="#000" stroke-width="7" stroke-linecap="round" opacity=".55"/>' +
          '<line x1="' + me.x + '" y1="' + me.y + '" x2="' + q.x + '" y2="' + q.y + '" stroke="#fff" stroke-width="3.5" stroke-linecap="round"/>';
      }
      var p = document.getElementById("ubPulse");
      if (p) {
        var searching = (APP().mode !== "driver" && uberState === "searching") || (APP().mode === "driver" && APP().drv && Number(APP().drv.on) === 1 && !APP().drv.ride && !APP().drv.req);
        p.style.display = searching ? "block" : "none"; p.style.left = me.x + "px"; p.style.top = me.y + "px";
      }
    }
    if (svg.innerHTML !== lines) svg.innerHTML = lines;
  }, 500);

  // le prix depend de la destination : on rafraichit la feuille quand le point change
  var lastKey = "";
  setInterval(function () {
    var box = document.getElementById("uberRoot");
    if (!box || !box.offsetParent || APP().mode === "driver" || uberState !== "idle") return;
    var d = dest(), k = d ? d[0] + "," + d[1] + "," + (U().has ? 1 : 0) : "";
    if (k !== lastKey) { lastKey = k; try { uberRender(); if (window.CwUber) window.CwUber.layout(); } catch (_) {} }
  }, 400);

  window.CwUber34 = {
    hint: function () { say("Touche la carte pour placer ta destination"); },
    clearDest: function () { var u = U(); u.pin = null; u.tx = null; u.ty = null; lastKey = "x"; try { uberRender(); window.CwUber.layout(); } catch (_) {} }
  };
  // plus de destination ecrite a la main : le nom affiche suit le point pose
  window.uberRequest = (function (orig) {
    return function () { uberDest = dest() ? "Point sur la carte" : "Ma position"; return orig.apply(this, arguments); };
  })(window.uberRequest);
})();
