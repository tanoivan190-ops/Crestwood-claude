/* ==========================================================================
   Crestwood RP v21 - le telephone se souvient de tout.
   Sauvegarde : fond d'ecran, reglages (avion, wifi...), inscription WhatsApp / Instagram (+ pseudo,
   publications, likes), musique en cours, likes TikTok / X, appels recents, notes, dernier app ouverte.
   Double sauvegarde : localStorage de la page + SharedPreferences Android (SampPhone.saveStore / loadStore),
   pour survivre a un vidage du cache WebView ou a une mise a jour de l'appli.
   Les contacts, SMS, appels et Wave restent sur le serveur (net.js).
   ========================================================================== */
(function () {
  "use strict";
  var KEY = "cwPhoneState.v1";
  var MIRROR = ["crestwoodPhoneApps", "crestwoodPhonePhotos"];   // cles deja utilisees par la page
  var saveTimer = 0;
  var lastJson = "";

  function native() { return window.SampPhone || null; }
  function g(name) { try { return (0, eval)("typeof " + name + "!=='undefined'?" + name + ":undefined"); } catch (e) { return undefined; } }
  function s(name, value) { try { (0, eval)(name + "=" + JSON.stringify(value)); } catch (e) { /* variable absente */ } }

  function snapshot() {
    var st = {
      v: 1,
      wp: g("wpIdx"),
      dev: g("devSettings"),
      wa: g("waRegistered"),
      ig: g("igLoggedIn"),
      igName: g("igUserName"),
      igPosts: g("igMyPosts"),
      igLiked: g("igLiked"),
      mus: g("musI"),
      tt: g("ttLiked"),
      xf: g("xfLiked"),
      recents: (g("phoneRecents") || []).slice(0, 40),
      notes: notes,
      last: lastApp,
      keys: {}
    };
    MIRROR.forEach(function (k) {
      try {
        var v = window.localStorage.getItem(k);
        if (v && v.length < 200000) st.keys[k] = v;   // les photos en base64 trop lourdes restent dans la page
      } catch (e) { /* stockage indisponible */ }
    });
    return st;
  }

  function apply(st) {
    if (!st || typeof st !== "object") return;
    if (typeof st.wp === "number" && g("WP") && st.wp >= 0 && st.wp < g("WP").length) {
      s("wpIdx", st.wp);
      try { applyWallpaper(document.getElementById("homeWp")); applyWallpaper(document.getElementById("lockWp")); } catch (e) { /* pas encore pret */ }
    }
    if (st.dev && typeof st.dev === "object") { s("devSettings", st.dev); try { updateStatusIcons(); } catch (e) { /* idem */ } }
    if (typeof st.wa === "boolean") s("waRegistered", st.wa);
    if (typeof st.ig === "boolean") s("igLoggedIn", st.ig);
    if (typeof st.igName === "string") s("igUserName", st.igName);
    if (Array.isArray(st.igPosts)) s("igMyPosts", st.igPosts);
    if (st.igLiked && typeof st.igLiked === "object") s("igLiked", st.igLiked);
    if (typeof st.mus === "number") s("musI", st.mus);
    if (st.tt && typeof st.tt === "object") s("ttLiked", st.tt);
    if (st.xf && typeof st.xf === "object") s("xfLiked", st.xf);
    if (Array.isArray(st.recents)) s("phoneRecents", st.recents);
    if (Array.isArray(st.notes)) notes = st.notes.filter(function (n) { return n && typeof n.t === "string"; }).slice(0, 60);
    if (typeof st.last === "string") lastApp = st.last;
    if (st.keys && typeof st.keys === "object") {
      MIRROR.forEach(function (k) {
        try { if (st.keys[k] && !window.localStorage.getItem(k)) window.localStorage.setItem(k, st.keys[k]); } catch (e) { /* idem */ }
      });
    }
  }

  function load() {
    var raw = "";
    try { raw = window.localStorage.getItem(KEY) || ""; } catch (e) { raw = ""; }
    if (!raw) { try { var n = native(); raw = n && n.loadStore ? (n.loadStore() || "") : ""; } catch (e) { raw = ""; } }
    if (!raw) return;
    try { apply(JSON.parse(raw)); lastJson = raw; } catch (e) { /* sauvegarde illisible : on repart a zero */ }
  }

  function saveNow() {
    window.clearTimeout(saveTimer);
    saveTimer = 0;
    var json;
    try { json = JSON.stringify(snapshot()); } catch (e) { return; }
    if (json === lastJson) return;
    lastJson = json;
    try { window.localStorage.setItem(KEY, json); } catch (e) { /* plein */ }
    try { var n = native(); if (n && n.saveStore) n.saveStore(json); } catch (e) { /* hors jeu */ }
  }
  function saveSoon() { window.clearTimeout(saveTimer); saveTimer = window.setTimeout(saveNow, 700); }

  // ---------------- Notes : vraie application (ecrire, garder, supprimer) ----------------
  var notes = [];
  var lastApp = "";
  function esc(t) { return String(t).replace(/[&<>"]/g, function (c) { return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]; }); }
  function notesView(box, openIdx) {
    if (!box) return;
    if (typeof openIdx === "number") {
      var n = notes[openIdx] || { t: "", d: Date.now() };
      box.innerHTML = "<div style='display:flex;flex-direction:column;height:100%;gap:10px'>"
        + "<div style='display:flex;justify-content:space-between;align-items:center'><button type='button' data-nb='back' style='border:0;background:none;color:#e6a100;font-size:16px;font-weight:600'>‹ Notes</button>"
        + "<button type='button' data-nb='del' style='border:0;background:none;color:#ff3b30;font-size:15px'>Supprimer</button></div>"
        + "<textarea data-nb='text' placeholder='Écris ta note…' style='flex:1;min-height:260px;width:100%;border:0;outline:none;resize:none;background:transparent;font:16px/1.5 -apple-system,Roboto,sans-serif;color:#111'>" + esc(n.t) + "</textarea></div>";
      var ta = box.querySelector("textarea");
      ta.addEventListener("input", function () {
        if (!notes[openIdx]) notes.unshift({ t: "", d: Date.now() }), openIdx = 0;
        notes[openIdx].t = ta.value; notes[openIdx].d = Date.now(); saveSoon();
      });
      box.querySelector("[data-nb=back]").onclick = function () { notes = notes.filter(function (x) { return x.t.trim(); }); saveSoon(); notesView(box); };
      box.querySelector("[data-nb=del]").onclick = function () { notes.splice(openIdx, 1); saveSoon(); notesView(box); };
      window.setTimeout(function () { try { ta.focus(); } catch (e) { /* idem */ } }, 60);
      return;
    }
    var list = notes.map(function (n, i) {
      var first = (n.t.split("\n")[0] || "Nouvelle note").slice(0, 40);
      var rest = n.t.split("\n").slice(1).join(" ").slice(0, 60);
      var d = new Date(n.d || Date.now());
      return "<div data-ni='" + i + "' style='padding:12px 14px;border-bottom:1px solid #eee'><b style='display:block;font-size:15px'>" + esc(first) + "</b>"
        + "<span style='color:#8e8e93;font-size:13px'>" + d.toLocaleDateString() + "  " + esc(rest) + "</span></div>";
    }).join("");
    box.innerHTML = "<div style='font-size:30px;font-weight:800;margin:4px 0 12px'>Notes</div>"
      + "<div style='background:#fff;border-radius:12px;overflow:hidden'>" + (list || "<div style='padding:18px;color:#8e8e93;text-align:center'>Aucune note. Touche + pour écrire.</div>") + "</div>"
      + "<button type='button' data-nb='new' style='position:absolute;right:18px;bottom:24px;width:52px;height:52px;border-radius:50%;border:0;background:#e6a100;color:#fff;font-size:28px;box-shadow:0 6px 16px rgba(0,0,0,.25)'>+</button>";
    box.style.position = "relative";
    box.querySelector("[data-nb=new]").onclick = function () { notes.unshift({ t: "", d: Date.now() }); notesView(box, 0); };
    Array.prototype.forEach.call(box.querySelectorAll("[data-ni]"), function (row) {
      row.onclick = function () { notesView(box, Number(row.getAttribute("data-ni"))); };
    });
  }

  // openApp est une fonction globale de la page : on l'enveloppe (notes + derniere app)
  if (typeof window.openApp === "function") {
    var orig = window.openApp;
    window.openApp = function (id) {
      orig.apply(this, arguments);
      if (id !== "camera") { lastApp = id; saveSoon(); }
      if (id === "notes") {
        var box = document.querySelector("#scr-notes .content");
        if (box) notesView(box);
      }
    };
  }

  load();
  // toute action dans le telephone peut changer un etat : sauvegarde groupee
  ["click", "change", "input"].forEach(function (ev) { document.addEventListener(ev, saveSoon, true); });
  window.addEventListener("cw:pagehide", saveNow);
  window.addEventListener("pagehide", saveNow);
  document.addEventListener("visibilitychange", function () { if (document.hidden) saveNow(); });
  window.CwPhoneStore = { save: saveNow, load: load };
})();
