/* =========================================================
   CRESTWOOD v14 - BARRE D'OBJETS RAPIDES
   Serveur -> HUD : CwHotbar.set({slots:[["eau_kirene",3],["",0],...], show:1, jbl:0})
   (dialogue "@hotbar" 9840, relaye par SAMP.java)
   Toucher un emplacement -> /hotbar <n> (utilise l'objet sans ouvrir l'inventaire)
   ========================================================= */
(() => {
  "use strict";
  const bar = document.getElementById("cwHotbar");
  if (!bar) return;
  const bridge = window.SampHud || null;
  const NAMES = {
    eau_kirene: "Eau", vody: "Vody", hamburger: "Burger", bandage: "Bandage", gilet_tactique: "Gilet",
    liasse_billets: "Billets", enceinte_jbl: "JBL", carte_sim: "Puce", iphone_orange: "iPhone", iphone_noir: "iPhone",
    apple_watch: "Montre"
  };
  let last = "";
  let lock = 0;
  function icon(id) {
    const img = document.createElement("img");
    img.alt = ""; img.draggable = false;
    img.src = "../inventory/items/" + id + ".png";
    img.onerror = () => { if (!img.dataset.svg) { img.dataset.svg = 1; img.src = "../inventory/items/" + id + ".svg"; } else img.style.visibility = "hidden"; };
    return img;
  }
  function use(n, el) {
    const now = Date.now();
    if (now < lock) return;
    lock = now + 600;   // pas de double utilisation par erreur
    el.classList.add("tap");
    setTimeout(() => el.classList.remove("tap"), 180);
    try { bridge?.sendChatMessage?.("/hotbar " + n); } catch (_) {}
  }
  function set(raw) {
    let d = raw;
    if (typeof d === "string") { try { d = JSON.parse(d); } catch (_) { return; } }
    if (!d || !Array.isArray(d.slots)) return;
    const key = JSON.stringify(d);
    if (key === last) return;
    last = key;
    bar.replaceChildren();
    let any = false;
    d.slots.forEach((s, i) => {
      const id = String((s && s[0]) || ""), qty = Number((s && s[1]) || 0);
      const b = document.createElement("button");
      b.type = "button";
      b.className = "cw-hot-slot" + (id ? "" : " empty") + (id && qty <= 0 ? " none" : "") + (id === "enceinte_jbl" && Number(d.jbl) > 0 ? " active" : "");
      const k = document.createElement("span"); k.className = "cw-hot-key"; k.textContent = String(i + 1); b.appendChild(k);
      if (id) {
        any = true;
        b.appendChild(icon(id));
        const q = document.createElement("span"); q.className = "cw-hot-qty"; q.textContent = id === "enceinte_jbl" ? "" : "x" + qty; b.appendChild(q);
        b.setAttribute("aria-label", (NAMES[id] || id) + " (" + qty + ")");
        b.addEventListener("click", () => use(i + 1, b));
      } else {
        b.disabled = true;
      }
      bar.appendChild(b);
    });
    bar.hidden = !(any && Number(d.show) !== 0);
    try { window.crestwoodSyncTouchBounds && window.crestwoodSyncTouchBounds(); } catch (_) {}
  }
  window.CwHotbar = { set };
})();
