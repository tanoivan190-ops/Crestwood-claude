/* ============================================================
   [v31] Telephone : vraie carte dans Uber + Instagram de la ville
   ------------------------------------------------------------
   UBER : carte de San Andreas (../map/map.jpg) centree sur le joueur, voiture du chauffeur
          qui a pris la course, toucher la carte = destination (GPS du jeu).
          Serveur : /phoneact pos -> {"ev":"pos","x","y","dx","dy"} ; /phoneact gps <x> <y>
   INSTAGRAM : photos de l'appli Camera publiees pour toute la ville.
          /phoneact ig -> {"ev":"ig","ok":1,"token","name","port"} puis HTTP direct vers
          http://<ip du serveur>:<port> (server/instagram/ig-server.js).
   ============================================================ */
(function () {
  "use strict";
  var send = function (t, o) { if (window.PhoneNet && window.PhoneNet.send) window.PhoneNet.send(t, o); };
  var esc = function (s) { return String(s == null ? "" : s).replace(/[&<>"']/g, function (c) { return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]; }); };
  var pretty = function (n) { return String(n || "").replace(/_/g, " "); };
  var ago = function (ts) {
    var s = Math.max(0, Math.floor(Date.now() / 1000) - Number(ts || 0));
    if (s < 60) return "à l'instant"; if (s < 3600) return Math.floor(s / 60) + " min";
    if (s < 86400) return Math.floor(s / 3600) + " h"; return Math.floor(s / 86400) + " j";
  };
  var say = function (m) { if (typeof toast === "function") toast(m); };

  /* ======================= UBER : CARTE ======================= */
  var MAP = 2560, WORLD = 6000, ZOOM = 0.62;          // 1 px de carte -> 0,62 px a l'ecran (~1 200 m visibles)
  var U = { x: 0, y: 0, dx: null, dy: null, who: "car", tx: null, ty: null, has: false, pin: null, timer: 0 };
  function toPx(x, y) { return { x: (x + 3000) / WORLD * MAP * ZOOM, y: (3000 - y) / WORLD * MAP * ZOOM }; }

  window.uberMapHTML = function () {
    return '<div id="uberMap" style="position:relative;height:150px;flex:none;overflow:hidden;background:#0b1220;touch-action:none">' +
      '<div id="uberMapIn" style="position:absolute;left:0;top:0;width:' + Math.round(MAP * ZOOM) + 'px;height:' + Math.round(MAP * ZOOM) + 'px;will-change:transform">' +
      '<img src="../map/map.jpg" alt="" style="width:100%;height:100%;display:block;filter:saturate(1.1) brightness(.92)">' +
      '<div id="uberPin" style="position:absolute;display:none;width:22px;height:22px;margin:-22px 0 0 -11px;font-size:22px;line-height:22px;text-align:center;filter:drop-shadow(0 2px 3px rgba(0,0,0,.6))">📍</div>' +
      '<img id="uberDrv" src="uber/uberx_top.png" alt="" style="position:absolute;display:none;height:26px;width:auto;margin:-13px 0 0 -9px;filter:drop-shadow(0 2px 4px rgba(0,0,0,.6))">' +
      '<div id="uberCli" style="position:absolute;display:none;width:26px;height:26px;margin:-13px 0 0 -13px;border-radius:50%;background:#30d158;border:3px solid #fff;color:#000;font-size:13px;font-weight:900;line-height:20px;text-align:center;box-shadow:0 0 0 6px rgba(48,209,88,.25)">C</div>' +
      '<div id="uberMe" style="position:absolute;width:14px;height:14px;margin:-7px 0 0 -7px;border-radius:50%;background:#2f8cff;border:3px solid #fff;box-shadow:0 0 0 6px rgba(47,140,255,.25)"></div>' +
      '</div>' +
      '<div id="uberHint" style="position:absolute;left:8px;bottom:6px;font-size:10.5px;font-weight:700;color:#fff;background:rgba(0,0,0,.55);padding:3px 8px;border-radius:8px">Touche la carte pour choisir ta destination</div>' +
      '</div>';
  };

  function uberLayout() {
    var box = document.getElementById("uberMap"), inner = document.getElementById("uberMapIn");
    if (!box || !inner) return;
    var w = box.clientWidth || 330, h = box.clientHeight || 150;
    var me = toPx(U.x, U.y);
    var cx = me.x, cy = me.y;
    if (U.dx !== null) { var d = toPx(U.dx, U.dy); cx = (me.x + d.x) / 2; cy = (me.y + d.y) / 2; }
    inner.style.transform = "translate(" + Math.round(w / 2 - cx) + "px," + Math.round(h / 2 - cy) + "px)";
    var dot = document.getElementById("uberMe");
    if (dot) { dot.style.left = me.x + "px"; dot.style.top = me.y + "px"; }
    // [v32] l'autre personne de la course : la voiture du chauffeur (passager) ou le client (chauffeur)
    var car = document.getElementById("uberDrv"), cli = document.getElementById("uberCli");
    [car, cli].forEach(function (el) { if (el) el.style.display = "none"; });
    var other = U.who === "client" ? cli : car;
    if (other && U.dx !== null) { var p = toPx(U.dx, U.dy); other.style.display = "block"; other.style.left = p.x + "px"; other.style.top = p.y + "px"; }
    var pin = document.getElementById("uberPin"), dest = U.pin || (U.tx !== null ? [U.tx, U.ty] : null);
    if (pin) {
      if (dest) { var q = toPx(dest[0], dest[1]); pin.style.display = "block"; pin.style.left = q.x + "px"; pin.style.top = q.y + "px"; }
      else pin.style.display = "none";
    }
    if (!box.dataset.bound) {
      box.dataset.bound = "1";
      box.addEventListener("click", function (e) {
        var r = box.getBoundingClientRect(), m = inner.style.transform.match(/translate\((-?\d+)px,\s*(-?\d+)px\)/);
        if (!m) return;
        var px = e.clientX - r.left - Number(m[1]), py = e.clientY - r.top - Number(m[2]);
        var wx = px / (MAP * ZOOM) * WORLD - 3000, wy = 3000 - py / (MAP * ZOOM) * WORLD;
        U.pin = [Math.round(wx), Math.round(wy)];
        try { uberDest = "Point sur la carte"; var inp = document.getElementById("uberDest"); if (inp) inp.value = uberDest; } catch (_) {}
        send("gps", { x: U.pin[0], y: U.pin[1] });
        uberLayout();
      });
    }
  }
  window.CwUber = {
    state: U,   // [v34] lu par apps-v34.js (estimation du prix, recentrage)
    layout: function () { uberLayout(); },
    onPos: function (d) {
      U.x = Number(d.x) || 0; U.y = Number(d.y) || 0; U.has = true;
      if (d.dx !== undefined) { U.dx = Number(d.dx); U.dy = Number(d.dy); } else { U.dx = null; U.dy = null; }
      U.who = d.who === "client" ? "client" : "car";
      if (d.tx !== undefined) { U.tx = Number(d.tx); U.ty = Number(d.ty); } else { U.tx = null; U.ty = null; }
      uberLayout();
    }
  };
  // la carte est recreee a chaque rendu de l'appli : on la replace et on demande la position
  setInterval(function () {
    var box = document.getElementById("uberMap");
    if (!box || !box.offsetParent) return;
    uberLayout();
    var now = Date.now();
    if (now - U.timer > 2500) { U.timer = now; send("pos"); }
  }, 500);

  /* ======================= INSTAGRAM ======================= */
  var IG = { token: "", name: "", base: "", feed: null, mine: null, notifs: null, err: "", busy: false };
  try { igLoggedIn = true; } catch (_) {}
  function host() {
    try { if (window.SampPhone && SampPhone.serverHost) return SampPhone.serverHost(); } catch (_) {}
    return location.hostname || "127.0.0.1";
  }
  function api(path, opt) {
    if (!IG.base) return Promise.reject(new Error("hors ligne"));
    return fetch(IG.base + path, opt).then(function (r) { return r.json(); });
  }
  function post(path, obj) {
    obj.token = IG.token;
    return api(path, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(obj) });
  }
  function rerender() {
    try {
      var bd = document.getElementById("igBody");
      if (bd) bd.innerHTML = igBodyHTML();
    } catch (_) {}
  }
  function loadFeed() {
    api("/feed?token=" + encodeURIComponent(IG.token)).then(function (d) { IG.feed = d.posts || []; IG.err = ""; rerender(); })
      .catch(function () { IG.err = "Instagram est hors ligne pour le moment."; IG.feed = []; rerender(); });
  }
  window.CwIg = {
    info: function () { return { base: IG.base, token: IG.token, name: IG.name }; },   // [v45] statuts WhatsApp
    onToken: function (d) {
      IG.base = "http://" + host() + ":" + (Number(d.port) || 7790);
      if (!Number(d.ok)) { IG.err = "Instagram est hors ligne pour le moment."; IG.feed = []; rerender(); return; }
      IG.token = d.token || ""; IG.name = d.name || "";
      try { window.dispatchEvent(new Event("cw:igtoken")); } catch (_) {}
      try { igUserName = pretty(IG.name); } catch (_) {}
      loadFeed();
    },
    like: function (id) {
      post("/like", { id: id }).then(function (d) {
        (IG.feed || []).concat(IG.mine || []).forEach(function (p) { if (p.id === id) { p.likes = d.likes; p.liked = d.liked; } });
        rerender();
      }).catch(function () { say("Connexion impossible"); });
    },
    comments: function (id) { openComments(id); },
    refresh: function () { IG.feed = null; rerender(); loadFeed(); }
  };

  function postHTML(p) {
    var n = pretty(p.name), ini = (n[0] || "?").toUpperCase();
    return '<div style="margin:14px 0">' +
      '<div style="display:flex;align-items:center;gap:10px;margin-bottom:8px">' +
      '<div style="width:38px;height:38px;border-radius:50%;padding:2px;background:linear-gradient(45deg,#feda75,#d62976,#4f5bd5);flex:none"><div style="width:100%;height:100%;border-radius:50%;background:#5a6efe;color:#fff;display:flex;align-items:center;justify-content:center;font-weight:800;border:2px solid #fff;box-sizing:border-box">' + esc(ini) + '</div></div>' +
      '<div style="flex:1;min-width:0"><div style="font-weight:700">' + esc(n) + '</div><div style="color:#8e8e93;font-size:11px">Crestwood · ' + ago(p.ts) + '</div></div>' +
      '<div style="color:#c7c7cc;font-size:11px">#' + p.id + '</div></div>' +
      '<div style="aspect-ratio:1;border-radius:10px;overflow:hidden;background:#eee"><img class="thumb" src="' + esc(IG.base + "/" + p.img) + '" loading="lazy" style="width:100%;height:100%;object-fit:cover"></div>' +
      '<div style="display:flex;align-items:center;gap:16px;padding:9px 2px">' +
      '<span onclick="CwIg.like(' + p.id + ')" style="cursor:pointer;display:flex">' + igHeart(!!p.liked) + '</span>' +
      '<span onclick="CwIg.comments(' + p.id + ')" style="cursor:pointer;display:flex">' + IG_ICON("instagram_comment.svg", 22) + '</span></div>' +
      '<div style="font-weight:600;font-size:13px">' + p.likes + " j'aime</div>" +
      (p.caption ? '<div style="font-size:13px;margin-top:3px"><b>' + esc(n) + '</b> ' + esc(p.caption) + '</div>' : '') +
      (p.comments ? '<div onclick="CwIg.comments(' + p.id + ')" style="color:#8e8e93;font-size:13px;margin-top:3px;cursor:pointer">Voir les ' + p.comments + ' commentaire' + (p.comments > 1 ? "s" : "") + '</div>' : '') +
      '</div>';
  }
  function emptyHTML(t, s) {
    return '<div style="text-align:center;padding:46px 20px"><img src="instagram/empty_images.png" alt="" style="width:86px;height:86px;opacity:.85"><div style="font-weight:700;font-size:16px;margin-top:12px">' + t + '</div><div style="color:#8e8e93;font-size:13px;margin-top:6px;line-height:1.5">' + s + '</div></div>';
  }

  window.instaFeedHTML = function () {
    var head = '<div style="display:flex;align-items:center;justify-content:space-between"><img src="instagram/instagram_text.svg" alt="Instagram" style="height:26px;width:auto;display:block;margin:4px 0 2px"><span onclick="CwIg.refresh()" style="cursor:pointer;font-size:18px;color:#262626">⟳</span></div>';
    if (IG.feed === null) {
      if (!IG.base) send("ig");
      return head + '<div style="text-align:center;color:#8e8e93;padding:40px 0">Chargement…</div>';
    }
    if (IG.err) return head + emptyHTML("Hors ligne", esc(IG.err));
    if (!IG.feed.length) return head + emptyHTML("Aucune publication", "Sois le premier : prends une photo avec l'appli Caméra puis touche ＋.");
    return head + IG.feed.map(postHTML).join("");
  };

  window.igPublish = function () {
    var photos = (typeof USER_PHOTOS !== "undefined" && USER_PHOTOS) || [];
    try { var fresh = JSON.parse(localStorage.getItem("crestwoodPhonePhotos") || "[]"); if (Array.isArray(fresh) && fresh.length) photos = fresh; } catch (_) {}
    if (igPicked < 0 || !photos[igPicked]) { say("Choisis une photo"); return; }
    var img = photos[igPicked];
    if (String(img).indexOf("data:image/jpeg;base64,") !== 0) { say("Seules les photos de l'appli Caméra peuvent être publiées"); return; }
    if (IG.busy) return;
    IG.busy = true;
    var cap = ((document.getElementById("igCaption") || {}).value || "").trim();
    say("Publication…");
    post("/post", { caption: cap, image: img }).then(function (d) {
      IG.busy = false;
      if (!d.ok) { say(d.err || "Publication refusée"); return; }
      igPicked = -1; IG.feed = null; IG.mine = null; say("Publication partagée avec la ville");
      igTab("home"); loadFeed();
    }).catch(function () { IG.busy = false; say("Instagram est hors ligne"); });
  };

  window.igProfileHTML = function () {
    if (IG.mine === null) {
      api("/user?token=" + encodeURIComponent(IG.token) + "&name=" + encodeURIComponent(IG.name)).then(function (d) { IG.mine = d.posts || []; rerender(); })
        .catch(function () { IG.mine = []; rerender(); });
    }
    var mine = IG.mine || [], likes = mine.reduce(function (a, p) { return a + p.likes; }, 0);
    return '<div style="display:flex;align-items:center;gap:16px;margin:10px 0 14px">' +
      '<img src="instagram/avatar.png" alt="" style="width:74px;height:74px;border-radius:50%;object-fit:cover;flex:none">' +
      '<div style="flex:1;display:flex;justify-content:space-around;text-align:center">' +
      '<div><b>' + mine.length + '</b><div style="font-size:12px">publications</div></div><div><b>' + likes + "</b><div style=\"font-size:12px\">j'aime</div></div></div></div>" +
      '<div style="font-weight:700;margin-bottom:10px">' + esc(pretty(IG.name) || "Moi") + '</div>' +
      (mine.length ? '<div style="display:grid;grid-template-columns:repeat(3,1fr);gap:2px">' + mine.map(function (p) {
        return '<div onclick="CwIg.comments(' + p.id + ')" style="aspect-ratio:1;background:#eee;overflow:hidden;cursor:pointer"><img class="thumb" src="' + esc(IG.base + "/" + p.img) + '" loading="lazy" style="width:100%;height:100%;object-fit:cover"></div>';
      }).join("") + "</div>" : emptyHTML("Aucune publication", "Tes photos publiées apparaîtront ici."));
  };

  window.igNotifsHTML = function () {
    if (IG.notifs === null && IG.token) {
      api("/notifs?token=" + encodeURIComponent(IG.token)).then(function (d) { IG.notifs = d.notifs || []; rerender(); setTimeout(function () { IG.notifs = null; }, 15000); })
        .catch(function () { IG.notifs = []; rerender(); });
    }
    var list = IG.notifs || [];
    return '<div style="font-weight:800;font-size:17px;margin:8px 0 6px">Notifications</div>' +
      (list.length ? list.map(function (n) {
        return '<div onclick="CwIg.comments(' + n.id + ')" style="display:flex;align-items:center;gap:12px;padding:9px 0;cursor:pointer"><div style="width:40px;height:40px;border-radius:50%;background:#5a6efe;color:#fff;display:flex;align-items:center;justify-content:center;font-weight:800;flex:none">' + esc(pretty(n.n)[0] || "?") + '</div>' +
          '<div style="flex:1;font-size:13px"><b>' + esc(pretty(n.n)) + '</b> ' + (n.t === "like" ? "a aimé ta photo." : "a commenté : « " + esc(n.txt) + " »") + ' <span style="color:#8e8e93">' + ago(n.ts) + '</span></div></div>';
      }).join("") : emptyHTML("Rien pour le moment", "Les j'aime et commentaires sur tes photos apparaîtront ici."));
  };

  window.igSearchHTML = function () {
    var list = IG.feed || [];
    return '<div style="font-weight:800;font-size:17px;margin:8px 0 8px">Explorer</div>' +
      (list.length ? '<div style="display:grid;grid-template-columns:repeat(3,1fr);gap:2px">' + list.map(function (p) {
        return '<div onclick="CwIg.comments(' + p.id + ')" style="aspect-ratio:1;background:#eee;overflow:hidden;cursor:pointer"><img class="thumb" src="' + esc(IG.base + "/" + p.img) + '" loading="lazy" style="width:100%;height:100%;object-fit:cover"></div>';
      }).join("") + "</div>" : emptyHTML("Rien à explorer", "Les photos de la ville apparaîtront ici."));
  };

  function openComments(id) {
    var scr = document.getElementById("screen") || document.body;
    var el = document.createElement("div");
    el.style.cssText = "position:absolute;inset:0;z-index:60;background:rgba(0,0,0,.45);display:flex;align-items:flex-end";
    el.innerHTML = '<div style="width:100%;max-height:75%;background:#fff;color:#262626;border-radius:16px 16px 0 0;display:flex;flex-direction:column">' +
      '<div style="text-align:center;font-weight:800;padding:12px;border-bottom:1px solid #eee">Commentaires · #' + id + '</div>' +
      '<div id="igCm" style="flex:1;overflow:auto;padding:8px 14px;min-height:80px;color:#8e8e93">Chargement…</div>' +
      '<div style="display:flex;gap:8px;padding:10px 12px;border-top:1px solid #eee"><input id="igCmIn" maxlength="120" placeholder="Ajouter un commentaire…" style="flex:1;border:1px solid #ddd;border-radius:18px;padding:9px 12px;font-size:14px;outline:none">' +
      '<button id="igCmGo" style="border:0;background:none;color:#0095f6;font-weight:800;font-size:14px">Publier</button></div></div>';
    scr.appendChild(el);
    el.addEventListener("click", function (e) { if (e.target === el) el.remove(); });
    var load = function () {
      api("/comments?id=" + id).then(function (d) {
        var box = el.querySelector("#igCm"); if (!box) return;
        var c = d.comments || [];
        box.style.color = "#262626";
        box.innerHTML = c.length ? c.map(function (x) { return '<div style="padding:6px 0;font-size:13px"><b>' + esc(pretty(x.n)) + '</b> ' + esc(x.txt) + ' <span style="color:#8e8e93;font-size:11px">' + ago(x.ts) + '</span></div>'; }).join("") : '<div style="color:#8e8e93;padding:12px 0">Aucun commentaire. Écris le premier !</div>';
        box.scrollTop = box.scrollHeight;
      }).catch(function () { var box = el.querySelector("#igCm"); if (box) box.textContent = "Hors ligne."; });
    };
    load();
    el.querySelector("#igCmGo").onclick = function () {
      var inp = el.querySelector("#igCmIn"), t = (inp.value || "").trim();
      if (!t) return;
      inp.value = "";
      post("/comment", { id: id, text: t }).then(function (d) {
        if (!d.ok) { say(d.err || "Commentaire refusé"); return; }
        (IG.feed || []).forEach(function (p) { if (p.id === id) p.comments = d.comments; });
        load(); rerender();
      }).catch(function () { say("Instagram est hors ligne"); });
    };
  }

  // a chaque ouverture de l'appli : fil a jour + jeton du serveur
  var origOpen = window.openApp;
  if (typeof origOpen === "function") {
    window.openApp = function (id) {
      var r = origOpen.apply(this, arguments);
      if (id === "insta") { IG.feed = null; IG.mine = null; IG.notifs = null; send("ig"); }
      if (id === "uber") { U.timer = 0; }
      return r;
    };
  }
})();
