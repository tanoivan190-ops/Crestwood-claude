/* [v30c] Outils communs des pages (amendes, garage, bodycam).
   Natif -> page : PageApp.openFromNative(json) / PageApp.onUpdateFromNative(json)  (SAMP.java)
   Page -> natif : SampPage.action(type, arg) -> /pageact <page> <type> <arg> ; SampPage.close() ; SampPage.ready() */
(function () {
  "use strict";
  var bridge = function () { return window.SampPage || null; };
  var Cw = window.CwPage = {
    send: function (type, arg) {
      var b = bridge();
      if (!b) { console.log("[page]", type, arg || ""); return; }
      try { b.action(type, arg == null ? "" : String(arg)); } catch (e) { }
    },
    close: function () {
      var b = bridge();
      if (b) { try { b.close(); } catch (e) { } } else console.log("[page] close");
    },
    $: function (id) { return document.getElementById(id); },
    el: function (tag, cls, txt) { var e = document.createElement(tag); if (cls) e.className = cls; if (txt != null) e.textContent = txt; return e; },
    money: function (n) { n = Math.round(Number(n) || 0); return n.toString().replace(/\B(?=(\d{3})+(?!\d))/g, " ") + " $"; },
    initials: function (name) { var p = String(name || "?").trim().split(/\s+/); return ((p[0] || "?")[0] + (p[1] ? p[1][0] : "")).toUpperCase(); },
    toast: function (msg, ok) {
      if (!msg) return;
      var t = document.querySelector(".toast");
      if (t) t.parentNode.removeChild(t);
      t = Cw.el("div", "toast " + (ok === 1 ? "ok" : ok === 0 ? "bad" : ""), msg);
      document.body.appendChild(t);
      clearTimeout(Cw._tt);
      Cw._tt = setTimeout(function () { if (t.parentNode) t.parentNode.removeChild(t); }, 2600);
    },
    parse: function (json) {
      if (json && typeof json === "object") return json;
      try { return JSON.parse(json || "{}"); } catch (e) { return {}; }
    },
    boot: function (onData) {
      window.PageApp = {
        openFromNative: function (j) { var d = Cw.parse(j); onData(d, true); if (d.msg) Cw.toast(d.msg, d.ok); },
        onUpdateFromNative: function (j) { var d = Cw.parse(j); onData(d, false); if (d.msg) Cw.toast(d.msg, d.ok); }
      };
      var x = document.querySelector(".x");
      if (x) x.addEventListener("click", Cw.close);
      document.addEventListener("keydown", function (e) { if (e.key === "Escape") Cw.close(); });
      var ready = function () { var b = bridge(); if (b) { try { b.ready(); } catch (e) { } } };
      if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", ready); else ready();
    }
  };
})();
