/* [v31] Compteur d'origine : graduations + couleurs qui suivent l'etat (meme messages que hud.js). */
(function () {
  "use strict";
  var NS = "http://www.w3.org/2000/svg";
  function stops(id, a, b, c) {
    var g = document.getElementById(id); if (!g) return;
    while (g.firstChild) g.removeChild(g.firstChild);
    [[0, a], [c ? 50 : 100, c ? b : b]].concat(c ? [[100, c]] : []).forEach(function (s) {
      var st = document.createElementNS(NS, "stop"); st.setAttribute("offset", s[0] + "%"); st.setAttribute("stop-color", s[1]); g.appendChild(st);
    });
  }
  function ticks() {
    var svg = document.querySelector(".speedometer-box"); if (!svg || svg.querySelector(".cw-ticks")) return;
    var g = document.createElementNS(NS, "g"); g.setAttribute("class", "cw-ticks");
    var cx = 135, cy = 125, n = 12;
    for (var i = 0; i <= n; i++) {
      var deg = -171 + 162 * (i / n), r = deg * Math.PI / 180, big = i % 3 === 0;
      var r1 = big ? 92.5 : 94, r2 = big ? 98.5 : 97;
      var l = document.createElementNS(NS, "line");
      l.setAttribute("x1", (cx + r1 * Math.cos(r)).toFixed(1)); l.setAttribute("y1", (cy + r1 * Math.sin(r)).toFixed(1));
      l.setAttribute("x2", (cx + r2 * Math.cos(r)).toFixed(1)); l.setAttribute("y2", (cy + r2 * Math.sin(r)).toFixed(1));
      l.setAttribute("class", (big ? "big" : "") + (i >= 10 ? " hot" : ""));
      g.appendChild(l);
    }
    svg.insertBefore(g, svg.firstChild.nextSibling);
  }
  var lastEng = -1;
  function engine(v) {
    v = Number(v); if (isNaN(v) || v === lastEng) return; lastEng = v;
    var sp = document.getElementById("speedometer"); if (!sp) return;
    var col, glow;
    if (v > 60) { stops("grEngine", "#25d17a", "#9dff6b"); col = "#6ff0a8"; glow = "rgba(60,230,140,.5)"; }
    else if (v > 30) { stops("grEngine", "#ff9a1f", "#ffd84a"); col = "#ffc94a"; glow = "rgba(255,190,60,.55)"; }
    else { stops("grEngine", "#ff2d48", "#ff7a5c"); col = "#ff6b6b"; glow = "rgba(255,70,80,.6)"; }
    sp.style.setProperty("--eng-col", col); sp.style.setProperty("--eng-glow", glow);
  }
  function init() {
    stops("grTachometer", "#ffffff", "#9be9ff", "#4f8dff");
    stops("grFuel", "#ff9f1c", "#ffe066");
    ticks(); engine(100);
  }
  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init); else init();
  window.addEventListener("message", function (e) {
    var d = e.data || {}; var sp = document.getElementById("speedometer"); if (!sp || !d.vehicle) return;
    if (d.engine !== undefined) engine(d.engine);
    var s = parseInt(d.speed, 10) || 0;
    sp.classList.toggle("cw-fast", s >= 160);
    sp.classList.toggle("cw-lowfuel", (parseInt(d.fuel, 10) || 0) <= 15);
    if (d.showbelt) { sp.classList.toggle("cw-belt", !!d.seatbelt); sp.classList.toggle("cw-nobelt", !d.seatbelt); }
    else { sp.classList.remove("cw-belt"); sp.classList.remove("cw-nobelt"); }
  });
})();
