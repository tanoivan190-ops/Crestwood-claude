/* ============================================================
   [v38] Photos : vraie galerie (les photos de l'appareil photo du jeu)
   - relit les photos a CHAQUE ouverture (avant : liste figee au chargement + fausses photos internet)
   - toucher une photo = plein ecran ; glisser / fleches = suivante ; Supprimer ; Publier sur Instagram
   ============================================================ */
(function () {
  "use strict";
  var KEY = "crestwoodPhonePhotos";
  function list() {
    try { var a = JSON.parse(localStorage.getItem(KEY) || "[]"); return Array.isArray(a) ? a : []; } catch (e) { return []; }
  }
  function save(a) {
    for (;;) {
      try { localStorage.setItem(KEY, JSON.stringify(a)); return; } catch (e) { if (!a.length) return; a.pop(); }
    }
  }
  function say(m) { if (typeof toast === "function") toast(m); }

  var css = document.createElement("style");
  css.textContent =
    "#scr-photos .content{padding:4px 2px 16px}" +
    ".ph-head{display:flex;justify-content:space-between;align-items:flex-end;padding:4px 12px 10px}" +
    ".ph-head b{font-size:30px;font-weight:800;color:#fff;letter-spacing:-.5px}.ph-head span{font-size:13px;color:#8e8e93}" +
    ".ph-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:2px}" +
    ".ph-grid div{aspect-ratio:1;overflow:hidden;background:#1c1c1e;cursor:pointer}" +
    ".ph-grid img{width:100%;height:100%;object-fit:cover;display:block}" +
    ".ph-empty{text-align:center;color:#8e8e93;padding:70px 26px;font-size:14px;line-height:1.5}" +
    ".ph-empty b{display:block;color:#fff;font-size:18px;margin:12px 0 6px}" +
    ".ph-empty button{margin-top:16px;border:0;border-radius:12px;padding:11px 18px;background:#0a84ff;color:#fff;font-weight:700;font-size:15px}" +
    ".ph-view{position:absolute;inset:0;z-index:80;background:#000;display:flex;flex-direction:column}" +
    ".ph-view .bar{display:flex;justify-content:space-between;align-items:center;padding:54px 14px 10px;color:#fff;font-size:15px}" +
    ".ph-view .bar button{border:0;background:none;color:#0a84ff;font-size:16px;font-weight:600}" +
    ".ph-view .pic{flex:1;display:flex;align-items:center;justify-content:center;min-height:0;touch-action:pan-y}" +
    ".ph-view .pic img{max-width:100%;max-height:100%;object-fit:contain}" +
    ".ph-view .acts{display:flex;justify-content:space-around;padding:14px 10px 30px}" +
    ".ph-view .acts button{border:0;background:none;color:#fff;font-size:12px;display:flex;flex-direction:column;align-items:center;gap:6px}" +
    ".ph-view .acts svg{width:26px;height:26px;fill:none;stroke:#0a84ff;stroke-width:2;stroke-linecap:round;stroke-linejoin:round}" +
    ".ph-view .acts .del svg{stroke:#ff453a}";
  document.head.appendChild(css);

  function render() {
    var box = document.querySelector("#scr-photos .content");
    if (!box) return;
    var photos = list();
    var h = '<div class="ph-head"><b>Photos</b><span>' + photos.length + ' photo' + (photos.length > 1 ? "s" : "") + '</span></div>';
    if (!photos.length) {
      h += '<div class="ph-empty"><svg width="54" height="54" viewBox="0 0 24 24" fill="none" stroke="#8e8e93" stroke-width="1.5"><rect x="3" y="5" width="18" height="14" rx="2"/><circle cx="9" cy="10" r="2"/><path d="M21 16l-5-5-8 8"/></svg>' +
        '<b>Aucune photo</b>Ouvre l’appli Caméra et appuie sur le déclencheur : tes photos apparaîtront ici.' +
        '<br><button onclick="openApp(\'camera\')">Ouvrir la caméra</button></div>';
    } else {
      h += '<div class="ph-grid">' + photos.map(function (u, i) { return '<div onclick="CwPhotos.view(' + i + ')"><img src="' + u + '" loading="lazy" alt=""></div>'; }).join("") + "</div>";
    }
    box.innerHTML = h;
  }

  var cur = -1;
  function view(i) {
    var photos = list();
    if (!photos[i]) return;
    cur = i;
    var scr = document.getElementById("scr-photos");
    if (!scr) return;
    var v = scr.querySelector(".ph-view");
    if (!v) { v = document.createElement("div"); v.className = "ph-view"; scr.appendChild(v); }
    v.innerHTML = '<div class="bar"><button onclick="CwPhotos.close()">‹ Photos</button><span>' + (i + 1) + " / " + photos.length + '</span><span style="width:60px"></span></div>' +
      '<div class="pic"><img src="' + photos[i] + '" alt=""></div>' +
      '<div class="acts">' +
      '<button onclick="CwPhotos.step(-1)"><svg viewBox="0 0 24 24"><path d="M15 5l-7 7 7 7"/></svg>Précédente</button>' +
      '<button onclick="CwPhotos.post()"><svg viewBox="0 0 24 24"><rect x="3" y="3" width="18" height="18" rx="5"/><circle cx="12" cy="12" r="4"/><circle cx="17.5" cy="6.5" r="1"/></svg>Instagram</button>' +
      '<button class="del" onclick="CwPhotos.del()"><svg viewBox="0 0 24 24"><path d="M4 7h16M9 7V4h6v3M6 7l1 13h10l1-13"/></svg>Supprimer</button>' +
      '<button onclick="CwPhotos.step(1)"><svg viewBox="0 0 24 24"><path d="M9 5l7 7-7 7"/></svg>Suivante</button>' +
      "</div>";
    // glisser gauche / droite
    var pic = v.querySelector(".pic"), x0 = null;
    pic.addEventListener("touchstart", function (e) { x0 = e.touches[0].clientX; }, { passive: true });
    pic.addEventListener("touchend", function (e) {
      if (x0 === null) return;
      var dx = e.changedTouches[0].clientX - x0; x0 = null;
      if (Math.abs(dx) > 40) step(dx < 0 ? 1 : -1);
    });
  }
  function close() { var v = document.querySelector("#scr-photos .ph-view"); if (v) v.remove(); cur = -1; render(); }
  function step(d) { var n = list().length; if (!n) return close(); view((cur + d + n) % n); }
  function del() {
    var a = list(); if (cur < 0 || !a[cur]) return;
    a.splice(cur, 1); save(a); say("Photo supprimée");
    if (!a.length) return close();
    view(Math.min(cur, a.length - 1));
  }
  function post() {
    var a = list(); if (cur < 0 || !a[cur]) return;
    try { localStorage.setItem("cwIgPick", String(cur)); } catch (e) { /* plein */ }
    try { if (typeof USER_PHOTOS !== "undefined") { USER_PHOTOS.length = 0; Array.prototype.push.apply(USER_PHOTOS, a); } } catch (e) { /* idem */ }
    try { if (typeof igPicked !== "undefined") igPicked = cur; } catch (e) { /* idem */ }
    openApp("insta");
  }
  window.CwPhotos = { view: view, close: close, step: step, del: del, post: post, render: render };

  var orig = window.openApp;
  if (typeof orig === "function") {
    window.openApp = function (id) {
      var r = orig.apply(this, arguments);
      if (id === "photos") {
        // les photos de la camera arrivent pendant que le telephone est ouvert : on relit tout
        try { if (typeof USER_PHOTOS !== "undefined") { USER_PHOTOS.length = 0; Array.prototype.push.apply(USER_PHOTOS, list()); } } catch (e) { /* idem */ }
        render();
      }
      return r;
    };
  }
})();
