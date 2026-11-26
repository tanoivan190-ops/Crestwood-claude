/* ============================================================
   [v45] WhatsApp : vrais STATUTS (24 h)
   - "Mon statut" : une photo prise en jeu (appli Camera -> Photos) ou un texte sur fond de couleur,
     avec une legende. Garde sur le telephone (24 h) ET partage avec la ville via le service
     Instagram du serveur (server/instagram/ig-server.js, routes /status et /statuses).
   - "Mises a jour recentes" : les statuts des autres joueurs (si le service est en ligne).
   - Lecteur plein ecran : barres de progression, 5 s par statut, toucher a droite / gauche.
   Les videos ne sont pas prises en charge (l'appareil photo du jeu fait des photos).
   ============================================================ */
(function () {
  "use strict";
  var KEY = "cwWaStatus.v1";
  var DAY = 24 * 3600 * 1000;
  var COLORS = ["#128c7e", "#c0392b", "#8e44ad", "#d35400", "#2c3e50", "#16a085", "#b7950b"];
  var W = { list: null, err: "", loading: false, pick: -1, color: 0 };

  function esc(s) { return String(s == null ? "" : s).replace(/[&<>"']/g, function (c) { return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]; }); }
  function say(m) { if (typeof toast === "function") toast(m); }
  function pretty(n) { return String(n || "").replace(/_/g, " "); }
  function ago(ms) {
    var m = Math.max(0, Math.round((Date.now() - ms) / 60000));
    if (m < 1) return "à l'instant";
    if (m < 60) return "il y a " + m + " min";
    return "il y a " + Math.floor(m / 60) + " h";
  }
  function mine() {
    var a = [];
    try { a = JSON.parse(localStorage.getItem(KEY) || "[]"); } catch (e) { a = []; }
    if (!Array.isArray(a)) a = [];
    var lim = Date.now() - DAY;
    return a.filter(function (x) { return x && x.ts > lim; });
  }
  function saveMine(a) {
    a = a.slice(-6);
    for (;;) {
      try { localStorage.setItem(KEY, JSON.stringify(a)); return; } catch (e) { if (!a.length) return; a.shift(); }
    }
  }
  function photos() {
    try { var a = JSON.parse(localStorage.getItem("crestwoodPhonePhotos") || "[]"); return Array.isArray(a) ? a : []; } catch (e) { return []; }
  }
  function ig() { try { return (window.CwIg && window.CwIg.info) ? window.CwIg.info() : {}; } catch (e) { return {}; } }
  function initials(n) { var p = pretty(n).trim().split(/\s+/); return ((p[0] || "?")[0] + ((p[1] || "")[0] || "")).toUpperCase(); }
  function ring(inner, seen) { return '<div style="padding:2px;border-radius:50%;border:2.5px solid ' + (seen ? "#5c6066" : "#25D366") + ';flex:none">' + inner + "</div>"; }
  function face(name, img) {
    if (img) return '<div style="width:46px;height:46px;border-radius:50%;background:#222 url(\'' + img + '\') center/cover"></div>';
    var c = COLORS[(String(name).length * 7) % COLORS.length];
    return '<div style="width:46px;height:46px;border-radius:50%;background:' + c + ';display:flex;align-items:center;justify-content:center;color:#fff;font-weight:800">' + esc(initials(name)) + "</div>";
  }

  function load() {
    var i = ig();
    if (!i.base) {
      if (W.asked) return;
      W.asked = true;
      try { window.PhoneNet && window.PhoneNet.send && window.PhoneNet.send("ig"); } catch (e) {}
      setTimeout(function () { W.asked = false; if (!ig().base && W.list === null) { W.err = "Statuts de la ville indisponibles (service hors ligne)."; W.list = []; refresh(); } }, 6000);
      return;
    }
    if (W.loading) return;
    W.loading = true;
    fetch(i.base + "/statuses").then(function (r) { return r.json(); }).then(function (d) {
      W.loading = false; W.err = ""; W.list = (d && d.statuses) || []; refresh();
    }).catch(function () { W.loading = false; W.err = "Statuts de la ville indisponibles (service hors ligne)."; W.list = []; refresh(); });
  }
  window.addEventListener("cw:igtoken", function () { if (typeof waTabCur !== "undefined" && waTabCur === "status") load(); });

  function refresh() {
    try { if (typeof waTabCur !== "undefined" && waTabCur === "status") { var p = document.getElementById("waPane"); if (p) p.innerHTML = window.waStatusPaneHTML(); } } catch (e) {}
  }

  window.waStatusPaneHTML = function () {
    if (W.list === null || Date.now() - (W.at || 0) > 60000) { W.at = Date.now(); setTimeout(load, 0); }
    var my = mine(), me = (ig().name || "");
    var last = my[my.length - 1];
    var h = '<div onclick="CwWaSt.mine()" style="display:flex;gap:12px;align-items:center;padding:8px;cursor:pointer">' +
      (last ? ring(face(me || "Moi", last.img), false) : '<div style="position:relative;flex:none">' + face(me || "Moi", "") +
        '<span style="position:absolute;right:-2px;bottom:-2px;width:20px;height:20px;border-radius:50%;background:#25D366;color:#000;font-weight:900;display:flex;align-items:center;justify-content:center;border:2px solid #0b0b0d">+</span></div>') +
      '<div style="flex:1"><div style="font-weight:700;color:#fff">Mon statut</div><div style="color:#8e8e93;font-size:12px">' +
      (last ? my.length + " statut(s) · " + ago(last.ts) : "Appuie pour ajouter un statut") + "</div></div>" +
      '<div onclick="event.stopPropagation();CwWaSt.add()" style="width:36px;height:36px;border-radius:50%;background:#1f2c34;color:#25D366;font-size:20px;display:flex;align-items:center;justify-content:center">✎</div></div>';
    h += '<div style="color:#8e8e93;font-size:12px;margin:12px 8px 4px">Mises à jour récentes</div>';
    var groups = {}, order = [];
    (W.list || []).forEach(function (s) { if (s.name === me) return; if (!groups[s.name]) { groups[s.name] = []; order.push(s.name); } groups[s.name].push(s); });
    if (W.err) h += '<div style="color:#8e8e93;font-size:12px;padding:6px 8px">' + esc(W.err) + "</div>";
    else if (W.list === null) h += '<div style="color:#8e8e93;font-size:12px;padding:6px 8px">Chargement…</div>';
    else if (!order.length) h += '<div style="color:#8e8e93;font-size:12px;padding:6px 8px">Aucun statut en ville pour le moment.</div>';
    order.forEach(function (n) {
      var g = groups[n], top = g[0];
      h += '<div onclick="CwWaSt.view(\'' + esc(n) + '\')" style="display:flex;gap:12px;align-items:center;padding:8px;cursor:pointer">' +
        ring(face(n, ""), false) + '<div><div style="font-weight:700;color:#fff">' + esc(pretty(n)) + '</div><div style="color:#8e8e93;font-size:12px">' +
        g.length + " statut(s) · " + ago(top.ts * 1000) + "</div></div></div>";
    });
    return h;
  };

  // ---------- ajout ----------
  function sheet(html) {
    var old = document.getElementById("waStSheet"); if (old) old.remove();
    var el = document.createElement("div");
    el.id = "waStSheet";
    el.style.cssText = "position:absolute;inset:0;z-index:210;display:flex;align-items:flex-end;background:rgba(0,0,0,.6)";
    el.innerHTML = '<div style="width:100%;max-height:86%;overflow:auto;background:#111b21;border-radius:22px 22px 0 0;padding:16px 14px 22px;color:#fff">' + html + "</div>";
    (document.getElementById("screen") || document.body).appendChild(el);
    el.addEventListener("click", function (e) { if (e.target === el) el.remove(); });
    return el;
  }
  function add() {
    var ph = photos();
    W.pick = ph.length ? 0 : -1;
    var grid = ph.length ? '<div style="display:grid;grid-template-columns:repeat(4,1fr);gap:4px;margin:8px 0 10px">' + ph.slice(0, 12).map(function (u, i) {
      return '<div data-i="' + i + '" style="aspect-ratio:1;border-radius:10px;background:#222 url(\'' + u + '\') center/cover;outline:' + (i === 0 ? "3px solid #25D366" : "0") + '"></div>';
    }).join("") + '<div data-i="-1" style="aspect-ratio:1;border-radius:10px;background:' + COLORS[W.color] + ';display:flex;align-items:center;justify-content:center;font-weight:900;font-size:13px">Aa</div></div>'
      : '<div style="color:#8e8e93;font-size:13px;margin:8px 0 10px">Pas encore de photo : prends-en une avec l\'appli Caméra, ou écris un statut texte.</div>';
    var el = sheet('<div style="font-size:18px;font-weight:800">Nouveau statut</div>' + grid +
      '<input id="waStCap" maxlength="140" placeholder="Ajouter une légende… (ou ton texte)" style="width:100%;background:#202c33;border:0;color:#fff;padding:12px;border-radius:12px;font-size:14px;outline:none">' +
      '<div style="display:flex;gap:8px;margin-top:12px"><button id="waStCam" style="flex:1;border:0;border-radius:12px;padding:12px;background:#202c33;color:#fff;font-weight:700">📷 Caméra</button>' +
      '<button id="waStGo" style="flex:1;border:0;border-radius:12px;padding:12px;background:#00a884;color:#fff;font-weight:800">Publier</button></div>');
    el.querySelectorAll("[data-i]").forEach(function (d) {
      d.onclick = function () {
        var i = Number(d.getAttribute("data-i"));
        if (i === -1 && W.pick === -1) { W.color = (W.color + 1) % COLORS.length; d.style.background = COLORS[W.color]; }
        W.pick = i;
        el.querySelectorAll("[data-i]").forEach(function (o) { o.style.outline = Number(o.getAttribute("data-i")) === i ? "3px solid #25D366" : "0"; });
      };
    });
    el.querySelector("#waStCam").onclick = function () { el.remove(); try { openApp("camera"); } catch (e) {} };
    el.querySelector("#waStGo").onclick = function () { publish(el); };
  }
  function publish(el) {
    var cap = String((el.querySelector("#waStCap") || {}).value || "").trim();
    var ph = photos(), img = W.pick >= 0 ? (ph[W.pick] || "") : "";
    if (!img && !cap) { say("Choisis une photo ou écris un texte"); return; }
    var st = { ts: Date.now(), img: img, cap: cap, color: COLORS[W.color] };
    var a = mine(); a.push(st); saveMine(a);
    el.remove();
    var i = ig();
    if (i.base && i.token) {
      var body = { token: i.token, caption: cap, color: st.color };
      if (img && String(img).indexOf("data:image/jpeg;base64,") === 0) body.image = img;
      fetch(i.base + "/status", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(body) })
        .then(function (r) { return r.json(); }).then(function (d) { say(d && d.ok ? "Statut partagé (24 h)" : ((d && d.err) || "Statut gardé sur ton téléphone")); load(); })
        .catch(function () { say("Statut gardé sur ton téléphone (service hors ligne)"); });
    } else say("Statut ajouté (24 h)");
    refresh();
  }

  // ---------- lecteur ----------
  var V = { items: [], i: 0, t: 0 };
  function viewer(items) {
    if (!items.length) return;
    V.items = items; V.i = 0;
    var old = document.getElementById("waStView"); if (old) old.remove();
    var el = document.createElement("div");
    el.id = "waStView";
    el.style.cssText = "position:absolute;inset:0;z-index:220;background:#000;display:flex;flex-direction:column;color:#fff";
    (document.getElementById("screen") || document.body).appendChild(el);
    el.addEventListener("click", function (e) {
      if (e.target.closest && e.target.closest("[data-x]")) { close(); return; }
      var r = el.getBoundingClientRect();
      step(e.clientX - r.left < r.width / 3 ? -1 : 1);
    });
    show();
  }
  function close() { clearTimeout(V.t); var el = document.getElementById("waStView"); if (el) el.remove(); refresh(); }
  function step(d) { V.i += d; if (V.i < 0) V.i = 0; if (V.i >= V.items.length) { close(); return; } show(); }
  function show() {
    var el = document.getElementById("waStView"); if (!el) return;
    clearTimeout(V.t);
    var s = V.items[V.i];
    var bars = V.items.map(function (_, k) {
      return '<div style="flex:1;height:3px;border-radius:2px;background:rgba(255,255,255,.3);overflow:hidden"><div style="height:100%;background:#fff;width:' + (k < V.i ? 100 : 0) + '%;' + (k === V.i ? "animation:waStBar 5s linear forwards" : "") + '"></div></div>';
    }).join("");
    var body = s.img ? '<img src="' + s.img + '" style="max-width:100%;max-height:100%;object-fit:contain">'
      : '<div style="width:100%;height:100%;display:flex;align-items:center;justify-content:center;padding:26px;text-align:center;font-size:24px;font-weight:800;line-height:1.3;background:' + (s.color || "#128c7e") + '">' + esc(s.cap) + "</div>";
    el.innerHTML = '<style>@keyframes waStBar{from{width:0}to{width:100%}}</style>' +
      '<div style="display:flex;gap:4px;padding:48px 10px 8px">' + bars + "</div>" +
      '<div style="display:flex;align-items:center;gap:10px;padding:2px 12px 8px"><b style="flex:1">' + esc(pretty(s.name || "Mon statut")) + ' <span style="font-weight:400;color:#bbb;font-size:12px">' + ago(s.ms) + '</span></b><span data-x style="font-size:22px;padding:4px 8px">✕</span></div>' +
      '<div style="flex:1;min-height:0;display:flex;align-items:center;justify-content:center">' + body + "</div>" +
      (s.img && s.cap ? '<div style="padding:14px 18px 30px;text-align:center;font-size:15px;background:linear-gradient(0deg,rgba(0,0,0,.8),transparent)">' + esc(s.cap) + "</div>" : '<div style="height:30px"></div>');
    V.t = setTimeout(function () { step(1); }, 5000);
  }

  window.CwWaSt = {
    add: add,
    mine: function () {
      var a = mine();
      if (!a.length) { add(); return; }
      viewer(a.map(function (x) { return { name: "Mon statut", img: x.img, cap: x.cap, color: x.color, ms: x.ts }; }));
    },
    view: function (name) {
      var i = ig();
      var items = (W.list || []).filter(function (s) { return s.name === name; }).reverse().map(function (s) {
        return { name: s.name, img: s.img ? i.base + "/" + s.img : "", cap: s.caption, color: s.color, ms: s.ts * 1000 };
      });
      viewer(items);
    }
  };
})();
