/* ============================================================
   [v32] Uber : passager + CHAUFFEUR (voiture personnelle, taximetre, solde)
   ------------------------------------------------------------
   Telephone -> serveur : PhoneNet.send("uber", {sub, arg})  -> /phoneact uber <sub> [arg]
     passager : req <x|comfort|black|moto> | cancel        chauffeur : on | off | accept | decline | end | withdraw
     sync (a l'ouverture de l'appli)
   Serveur -> telephone : {"ev":"uber","role":"rider"|"driver", ...}  (include/Uber.inc)
   Carte : {"ev":"pos","dx","dy","who":"car"|"client","tx","ty"} (apps-v31.js)
   ============================================================ */
(function () {
  "use strict";
  var send = function (t, o) { if (window.PhoneNet && window.PhoneNet.send) window.PhoneNet.send(t, o); };
  var esc = function (s) { return String(s == null ? "" : s).replace(/[&<>"']/g, function (c) { return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]; }); };
  var money = function (n) { return String(Math.round(Number(n) || 0)).replace(/\B(?=(\d{3})+(?!\d))/g, " ") + " $"; };
  var say = function (m) { if (m && typeof toast === "function") toast(m); };
  var UB = { mode: "rider", drv: null, tick: 0 };

  function rerender() { try { uberRender(); if (window.CwUber && window.CwUber.layout) window.CwUber.layout(); } catch (_) {} }

  /* ---------- barre Passager / Chauffeur ---------- */
  function switchHTML() {
    var b = function (id, label) {
      var on = UB.mode === id;
      return '<div onclick="CwUberApp.mode(\'' + id + '\')" style="flex:1;text-align:center;padding:7px 0;border-radius:10px;font-weight:800;font-size:13px;cursor:pointer;' +
        (on ? "background:#fff;color:#000" : "color:#8e8e93") + '">' + label + '</div>';
    };
    return '<div style="flex:none;display:flex;gap:4px;margin:6px 12px 8px;padding:3px;border-radius:12px;background:#1c1c1e">' + b("rider", "Passager") + b("driver", "Chauffeur") + '</div>';
  }

  /* ---------- vue chauffeur ---------- */
  function driverHTML() {
    var d = UB.drv || {}, on = Number(d.on) === 1;
    var card = function (inner, extra) { return '<div style="background:#1c1c1e;border-radius:14px;padding:12px 14px;margin-bottom:10px;' + (extra || "") + '">' + inner + '</div>'; };
    var btn = function (label, fn, style) { return '<button class="btn" style="margin-top:10px;font-weight:800;' + (style || "background:#fff;color:#000") + '" onclick="' + fn + '">' + label + '</button>'; };
    var h = "";
    // etat en ligne
    h += card('<div style="display:flex;align-items:center;gap:12px">' +
      '<div style="width:12px;height:12px;border-radius:50%;flex:none;background:' + (on ? "#30d158;box-shadow:0 0 10px #30d158" : "#636366") + '"></div>' +
      '<div style="flex:1;min-width:0"><div style="font-weight:800;font-size:16px">' + (on ? "En ligne" : "Hors ligne") + '</div>' +
      '<div style="color:#8e8e93;font-size:12px">' + (d.veh && d.veh !== "-" ? "Véhicule : " + esc(d.veh) : "Monte au volant de ta voiture") + '</div></div>' +
      '<div onclick="CwUberApp.act(\'' + (on ? "off" : "on") + '\')" style="width:50px;height:30px;border-radius:99px;position:relative;cursor:pointer;background:' + (on ? "#30d158" : "#3a3a3c") + '">' +
      '<div style="position:absolute;top:3px;left:' + (on ? 23 : 3) + 'px;width:24px;height:24px;border-radius:50%;background:#fff;transition:left .2s"></div></div></div>');
    // course proposee
    if (d.req) {
      h += card('<div style="color:#30d158;font-weight:800;font-size:12px;letter-spacing:1px">NOUVELLE COURSE · ' + Math.max(0, Number(d.req.left) || 0) + ' s</div>' +
        '<div style="font-weight:800;font-size:18px;margin-top:4px">' + esc(d.req.name) + '</div>' +
        '<div style="color:#8e8e93;font-size:13px">' + esc(d.req.type) + ' · à ' + (Number(d.req.dist) >= 1000 ? (d.req.dist / 1000).toFixed(1) + " km" : d.req.dist + " m") + '</div>' +
        '<div style="display:flex;gap:8px">' + btn("Accepter", "CwUberApp.act('accept')", "background:#30d158;color:#000;flex:1") + btn("Refuser", "CwUberApp.act('decline')", "background:#3a3a3c;color:#fff;flex:1") + '</div>',
        "border:1px solid #30d158;animation:ubPulse 1.2s ease-in-out infinite");
    }
    // course en cours
    if (d.ride) {
      var trip = d.ride.st === "trip";
      h += card('<div style="color:#0a84ff;font-weight:800;font-size:12px;letter-spacing:1px">' + (trip ? "COURSE EN COURS" : "VA CHERCHER LE CLIENT") + '</div>' +
        '<div style="font-weight:800;font-size:18px;margin-top:4px">' + esc(d.ride.name) + '</div>' +
        '<div style="color:#8e8e93;font-size:13px">' + esc(d.ride.type) + (trip ? (d.ride.dest ? " · destination sur ton GPS" : " · le client choisit sa destination") : " · suis le GPS jusqu'au client") + '</div>' +
        (trip ? '<div style="display:flex;justify-content:space-between;margin-top:10px"><div><div style="color:#8e8e93;font-size:11px">TAXIMÈTRE</div><div style="font-weight:900;font-size:24px">' + money(d.ride.fare) + '</div></div>' +
          '<div style="text-align:right"><div style="color:#8e8e93;font-size:11px">DISTANCE</div><div style="font-weight:800;font-size:18px">' + Number(d.ride.km || 0).toFixed(2) + ' km</div></div></div>' + btn("Terminer la course", "CwUberApp.act('end')") : ""));
    }
    // solde
    h += card('<div style="display:flex;justify-content:space-between;align-items:flex-end"><div><div style="color:#8e8e93;font-size:11px">SOLDE UBER</div><div style="font-weight:900;font-size:26px">' + money(d.bal) + '</div></div>' +
      '<div style="text-align:right;color:#8e8e93;font-size:12px">' + (Number(d.rides) || 0) + ' course' + (Number(d.rides) > 1 ? "s" : "") + '<br>80 % du prix pour toi</div></div>' +
      (Number(d.bal) > 0 ? btn("Retirer en liquide", "CwUberApp.act('withdraw')", "background:#3a3a3c;color:#fff") : ""));
    if (!on && !d.ride) h += '<div style="color:#8e8e93;font-size:12px;line-height:1.5;padding:2px 4px 8px">Avec ta voiture personnelle (garage), passe en ligne : la course du client le plus proche t’est proposée, tu as 20 s pour accepter. Le taximètre démarre quand le client monte.</div>';
    return uberMapHTML() + '<div style="flex:1;overflow:auto;padding:12px 14px 6px;min-height:0">' + h + '</div>';
  }

  /* ---------- rendu de l'appli ---------- */
  var origBody = window.uberBodyHTML;
  window.uberBodyHTML = function () {
    return switchHTML() + (UB.mode === "driver" ? driverHTML() : origBody());
  };
  if (!document.getElementById("ubPulseCss")) {
    var st = document.createElement("style"); st.id = "ubPulseCss";
    st.textContent = "@keyframes ubPulse{50%{box-shadow:0 0 0 4px rgba(48,209,88,.25)}}";
    document.head.appendChild(st);
  }

  /* ---------- passager : vraies courses ---------- */
  window.uberRequest = function () {
    var dEl = document.getElementById("uberDest");
    uberDest = ((dEl && dEl.value) || uberDest || "").trim() || "Ma position";
    var ride = UBER_RIDES.find(function (r) { return r.id === uberSel; }) || UBER_RIDES[0];
    uberTrip = { ride: Object.assign({}, ride), dest: uberDest, driver: "…", car: "", plate: "", min: "—" };
    uberStop(); uberState = "searching"; rerender();
    send("uber", { sub: "req", arg: ride.id });
  };
  window.uberCancel = function () { send("uber", { sub: "cancel" }); uberStop(); uberState = "idle"; uberTrip = null; rerender(); };
  window.uberStart = function () { say("Monte dans la voiture : le taximètre démarre tout seul."); };

  function onRider(d) {
    if (!uberTrip) {
      var ride = UBER_RIDES.find(function (r) { return r.name === d.type; }) || UBER_RIDES[0];
      uberTrip = { ride: Object.assign({}, ride), dest: uberDest || "Ma position", driver: "…", car: "", plate: "", min: "—" };
    }
    if (d.drv) { uberTrip.driver = d.drv; uberTrip.car = d.car || "Véhicule"; uberTrip.plate = d.plate || ""; uberTrip.min = d.eta || "—"; }
    if (d.fare !== undefined) uberTrip.ride.price = Number(d.fare) || 0;
    if (d.st === "trip") uberTrip.min = Number(d.km || 0).toFixed(1) + " km ·";
    uberStop();
    if (d.st === "search") uberState = "searching";
    else if (d.st === "enroute") uberState = "enroute";
    else if (d.st === "arrived") uberState = "arrived";
    else if (d.st === "trip") uberState = "trip";
    else if (d.st === "done") uberState = "done";
    else { uberState = "idle"; uberTrip = null; }
    if (d.msg) say(d.msg);
    if (UB.mode === "rider") rerender();
  }

  function onDriver(d) {
    var hadReq = UB.drv && UB.drv.req;
    UB.drv = d;
    if (d.req && !hadReq && UB.mode !== "driver") UB.mode = "driver";   // course proposee : on bascule tout seul
    if (d.msg) say(d.msg);
    if (UB.mode === "driver") rerender();
  }

  window.CwUberApp = {
    state: UB,   // [v34] lu par apps-v34.js
    onEvent: function (d) { if (d.role === "driver") onDriver(d); else onRider(d); },
    mode: function (m) { UB.mode = m === "driver" ? "driver" : "rider"; rerender(); if (UB.mode === "driver") send("uber", { sub: "sync" }); },
    act: function (a) { send("uber", { sub: a }); }
  };

  // compte a rebours de la course proposee + rafraichissement du taximetre
  setInterval(function () {
    var box = document.getElementById("uberRoot");
    if (!box || !box.offsetParent || UB.mode !== "driver" || !UB.drv) return;
    if (UB.drv.req) { UB.drv.req.left = Math.max(0, (Number(UB.drv.req.left) || 0) - 1); rerender(); }
    var now = Date.now();
    if (UB.drv.ride && now - UB.tick > 3000) { UB.tick = now; send("uber", { sub: "sync" }); }
  }, 1000);

  var origOpen = window.openApp;
  if (typeof origOpen === "function") {
    window.openApp = function (id) {
      var r = origOpen.apply(this, arguments);
      if (id === "uber") send("uber", { sub: "sync" });
      return r;
    };
  }
})();
