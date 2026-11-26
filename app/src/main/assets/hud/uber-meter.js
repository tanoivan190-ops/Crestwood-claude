/* [v32] Taximetre Uber dans le HUD (chauffeur + client) : serveur {"m":"meter","on":1,"km":1.25,"fare":775,"type":"UberX"} */
(() => {
  "use strict";
  let box = null;
  const fmt = (n) => String(Math.round(Number(n) || 0)).replace(/\B(?=(\d{3})+(?!\d))/g, " ") + " $";
  function ensure() {
    if (box) return box;
    const st = document.createElement("style");
    st.textContent = `
      .cw-meter{position:fixed;top:max(8px,env(safe-area-inset-top));left:50%;transform:translateX(-50%);z-index:30;display:flex;align-items:center;gap:10px;
        padding:6px 14px 6px 8px;border-radius:99px;background:rgba(10,12,16,.82);border:1px solid rgba(255,255,255,.12);color:#fff;
        box-shadow:0 6px 18px rgba(0,0,0,.4);pointer-events:none;animation:cwMeterIn .3s ease both}
      .cw-meter[hidden]{display:none}
      .cw-meter .lg{background:#fff;color:#000;font-weight:900;font-size:11px;letter-spacing:.5px;padding:4px 8px;border-radius:99px}
      .cw-meter .ty{font-size:10px;color:#9aa0aa;font-weight:700}
      .cw-meter .km{font-size:12px;font-weight:700;color:#cfd3da}
      .cw-meter .fr{font-size:16px;font-weight:900;color:#30d158;min-width:64px;text-align:right}
      .cw-meter .dot{width:7px;height:7px;border-radius:50%;background:#30d158;box-shadow:0 0 8px #30d158;animation:cwMeterBl 1s infinite}
      @keyframes cwMeterIn{from{opacity:0;transform:translate(-50%,-8px)}to{opacity:1;transform:translate(-50%,0)}}
      @keyframes cwMeterBl{50%{opacity:.3}}`;
    document.head.appendChild(st);
    box = document.createElement("div");
    box.className = "cw-meter"; box.hidden = true;
    box.innerHTML = '<span class="lg">Uber</span><span class="dot"></span><span class="ty"></span><span class="km"></span><span class="fr"></span>';
    document.body.appendChild(box);
    return box;
  }
  (window.CwBusExt = window.CwBusExt || {}).meter = function (d) {
    const b = ensure();
    if (!Number(d.on)) { b.hidden = true; return; }
    b.querySelector(".ty").textContent = d.type || "";
    b.querySelector(".km").textContent = Number(d.km || 0).toFixed(2).replace(".", ",") + " km";
    b.querySelector(".fr").textContent = fmt(d.fare);
    b.hidden = false;
  };
})();
