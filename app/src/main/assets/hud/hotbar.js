/* =========================================================
   CRESTWOOD v14 - BARRE D'OBJETS RAPIDES  ([v22] hotbar Crestwood : onglet +N a droite, Cw22.inc cote serveur)
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
    eau_kirene: "Eau", vody: "Vody", hamburger: "Burger", bandage: "Bandage", gilet_tactique: "Gilet", tablette_job: "Tablette",
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
  // [v22] hotbar Crestwood : onglet "+N" a droite, les 5 emplacements sortent vers la gauche
  let openT = 0;
  bar.classList.add("cw22-hb");
  const tab = document.createElement("button");
  tab.type = "button";
  tab.className = "cw22-hb-tab";
  tab.setAttribute("aria-label", "Objets rapides");
  tab.innerHTML = '<b>+0</b><svg viewBox="0 0 24 24" aria-hidden="true"><path d="m14.5 5-7 7 7 7"/></svg>';
  const slots = document.createElement("div");
  slots.className = "cw22-hb-slots";
  const sync = () => { try { window.crestwoodSyncTouchBounds && window.crestwoodSyncTouchBounds(); } catch (_) {} };
  function setOpen(on) {
    bar.classList.toggle("open", on);
    window.clearTimeout(openT);
    if (on) openT = window.setTimeout(() => setOpen(false), 6500);
    window.setTimeout(sync, 230);
  }
  tab.addEventListener("click", () => setOpen(!bar.classList.contains("open")));
  function set(raw) {
    let d = raw;
    if (typeof d === "string") { try { d = JSON.parse(d); } catch (_) { return; } }
    if (!d || !Array.isArray(d.slots)) return;
    const key = JSON.stringify(d);
    if (key === last) return;
    last = key;
    slots.replaceChildren();
    let any = false, total = 0;
    d.slots.forEach((s, i) => {
      const id = String((s && s[0]) || ""), qty = Number((s && s[1]) || 0);
      const b = document.createElement("button");
      b.type = "button";
      b.className = "cw-hot-slot" + (id ? "" : " empty") + (id && qty <= 0 ? " none" : "") + (id === "enceinte_jbl" && Number(d.jbl) > 0 ? " active" : "");
      const k = document.createElement("span"); k.className = "cw-hot-key"; k.textContent = String(i + 1); b.appendChild(k);
      if (id) {
        any = true;
        if (qty > 0) total += id === "enceinte_jbl" ? 1 : qty;
        b.appendChild(icon(id));
        const q = document.createElement("span"); q.className = "cw-hot-qty"; q.textContent = id === "enceinte_jbl" ? "" : "x" + qty; b.appendChild(q);
        const n = document.createElement("span"); n.className = "cw-hot-name"; n.textContent = NAMES[id] || id; b.appendChild(n);
        b.setAttribute("aria-label", (NAMES[id] || id) + " (" + qty + ")");
        b.addEventListener("click", () => { use(i + 1, b); setOpen(true); });
      } else {
        b.disabled = true;
      }
      slots.appendChild(b);
    });
    tab.querySelector("b").textContent = "+" + (total > 99 ? "99" : total);
    if (!tab.isConnected) { bar.replaceChildren(slots, tab); }
    bar.hidden = !(any && Number(d.show) !== 0);
    sync();
  }
  window.CwHotbar = { set };
})();
