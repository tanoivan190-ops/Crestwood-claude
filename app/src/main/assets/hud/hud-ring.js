/* Jauges en fondu autour de la minimap + rond d'arme dans la minimap.
   - Les arcs portent data-status (health, armour, thirst, hunger, energy) : hud.js (setStatus) y ecrit
     data-value / data-level comme sur les anciennes jauges ; ici on ne fait que dessiner.
   - La ligne d'arme (sous l'argent) ecoute "crestwood:ammo" (hud.js) : icone de l'arme, chargeur / reserve.
     Toucher = arme suivante, appui long = arme precedente (SampHud.weaponAction -> touches natives). */
(() => {
  "use strict";
  const root = document.querySelector(".hud-root");
  const map = document.querySelector(".minimap-frame");
  const ring = document.getElementById("mmRing");
  const icons = document.getElementById("mmRingIcons");
  const bubble = document.getElementById("mmWeapon");
  if (!root || !map || !ring) return;

  // Ordre et angles (degres, sens horaire depuis midi). La minimap touche les bords gauche/haut de
  // l'ecran : les jauges occupent le cote visible (droite puis bas).
  const ARCS = ["health", "armour", "thirst", "hunger", "energy"];
  const START = 32;
  const END = 214;
  const GAP = 5;
  const STROKE = 3.5;

  function polar(cx, cy, r, deg) {
    const a = (deg - 90) * Math.PI / 180;
    return [cx + r * Math.cos(a), cy + r * Math.sin(a)];
  }
  function arcPath(cx, cy, r, a0, a1) {
    const [x0, y0] = polar(cx, cy, r, a0);
    const [x1, y1] = polar(cx, cy, r, a1);
    const large = a1 - a0 > 180 ? 1 : 0;
    return `M${x0.toFixed(2)} ${y0.toFixed(2)} A${r.toFixed(2)} ${r.toFixed(2)} 0 ${large} 1 ${x1.toFixed(2)} ${y1.toFixed(2)}`;
  }

  function layout() {
    const m = map.getBoundingClientRect();
    if (m.width < 10) return;
    const R = m.width / 2;
    const r = R + 3 + STROKE / 2;               // juste a l'exterieur du liseré blanc
    const pad = Math.ceil(STROKE + 14);
    const size = m.width + pad * 2;
    ring.style.left = `${m.left - pad}px`;
    ring.style.top = `${m.top - pad}px`;
    ring.style.width = `${size}px`;
    ring.style.height = `${size}px`;
    ring.setAttribute("viewBox", `0 0 ${size} ${size}`);
    const c = size / 2;
    const span = (END - START - GAP * (ARCS.length - 1)) / ARCS.length;
    ARCS.forEach((name, i) => {
      const a0 = START + i * (span + GAP);
      const a1 = a0 + span;
      const g = ring.querySelector(`.mm-arc.${name}`);
      if (!g) return;
      const d = arcPath(c, c, r, a0, a1);
      g.querySelector(".trk").setAttribute("d", d);
      g.querySelector(".val").setAttribute("d", d);
      // petite icone au debut de l'arc, posee sur la jauge
      const img = icons?.querySelector(`[data-ring-icon="${name}"]`);
      if (img) {
        const [x, y] = polar(c, c, r, a0 + 2);
        img.style.left = `${m.left - pad + x}px`;
        img.style.top = `${m.top - pad + y}px`;
      }
    });
    try { window.crestwoodSyncTouchBounds?.(); } catch (_) { /* ignore */ }
  }

  // Valeur d'une jauge : hud.js ecrit data-value (0..100) sur le <g>
  function paint(g) {
    const v = Math.max(0, Math.min(100, Number(g.dataset.value ?? 100)));
    const val = g.querySelector(".val");
    if (val) val.style.strokeDasharray = `${v} 100`;
    const name = g.dataset.status;
    const img = icons?.querySelector(`[data-ring-icon="${name}"]`);
    if (img) img.dataset.level = g.dataset.level || "ok";
  }
  const arcs = Array.from(ring.querySelectorAll(".mm-arc"));
  arcs.forEach(paint);
  const mo = new MutationObserver((list) => {
    const seen = new Set();
    list.forEach((rec) => { if (!seen.has(rec.target)) { seen.add(rec.target); paint(rec.target); } });
  });
  arcs.forEach((g) => mo.observe(g, { attributes: true, attributeFilter: ["data-value", "data-level"] }));

  // ------------------------------------------------------------ rond d'arme
  if (bubble) {
    const img = bubble.querySelector(".mm-weapon-icon");
    const clipEl = bubble.querySelector("[data-mm-clip]");
    const resEl = bubble.querySelector("[data-mm-reserve]");
    let curIcon = -1;
    img?.addEventListener("load", () => { img.hidden = false; bubble.classList.add("has-weapon"); });
    img?.addEventListener("error", () => { img.hidden = true; bubble.classList.remove("has-weapon"); });

    document.addEventListener("crestwood:ammo", (e) => {
      const d = e.detail || {};
      const id = Number(d.weaponid) || 0;
      const icon = Number(d.icon) || 0;
      if (icon !== curIcon) {
        curIcon = icon;
        if (icon > 0 && img) img.src = `assets/weapons-white/GTASA_Weapon${icon}.png`;
        else if (img) { img.hidden = true; img.removeAttribute("src"); bubble.classList.remove("has-weapon"); }
      }
      // armes sans munitions (poings, armes blanches) : pas de compteur
      const melee = id <= 15 || id === 40 || id === 44 || id === 45 || id === 46;
      bubble.classList.toggle("no-ammo", melee || !(icon > 0));
      try { window.crestwoodSyncTouchBounds?.(); } catch (_) { /* ignore */ }
      if (clipEl) clipEl.textContent = String(Math.max(0, Number(d.clip) || 0));
      if (resEl) resEl.textContent = String(Math.max(0, Number(d.reserve) || 0));
    });

    const hud = () => window.SampHud;
    let timer = 0;
    let long = false;
    bubble.addEventListener("pointerdown", (e) => {
      if (root.classList.contains("hud-editing")) return;
      e.preventDefault();
      e.stopPropagation();
      bubble.classList.add("down");
      long = false;
      clearTimeout(timer);
      timer = setTimeout(() => {
        long = true;
        try { hud()?.weaponAction?.("prev"); } catch (_) { /* ignore */ }
        if (navigator.vibrate) { try { navigator.vibrate([10, 40, 10]); } catch (_) { /* ignore */ } }
      }, 450);
    });
    const end = (fire) => (e) => {
      clearTimeout(timer);
      bubble.classList.remove("down");
      e?.stopPropagation?.();
      if (fire && !long && !root.classList.contains("hud-editing")) {
        try { hud()?.weaponAction?.("next"); } catch (_) { /* ignore */ }
        if (navigator.vibrate) { try { navigator.vibrate(8); } catch (_) { /* ignore */ } }
        bubble.classList.remove("swap"); void bubble.offsetWidth; bubble.classList.add("swap");
      }
    };
    bubble.addEventListener("pointerup", end(true));
    bubble.addEventListener("pointercancel", end(false));
    bubble.addEventListener("click", (e) => e.stopPropagation());
    bubble.addEventListener("contextmenu", (e) => e.preventDefault());
  }

  layout();
  window.addEventListener("resize", () => requestAnimationFrame(layout));
  // la minimap peut changer de taille quand la page finit de se mettre en place
  if (window.ResizeObserver) new ResizeObserver(() => layout()).observe(map);
  setTimeout(layout, 300);
})();
