/* =====================================================================
   Crestwood v18/v20 · CLAVIER DE JEU (v20 : lettre ecrite au toucher, ecouteurs delegues) + PHRASES ENREGISTRÉES (macros)
   - s'ouvre avec le chat (bouton chat près de la carte) à la place du clavier Android ;
   - AZERTY, majuscules, chiffres / accents, appui long sur e a u i o c = accent (é à ù î ô ç),
     appui long sur ⌫ = effacement continu ;
   - barre de phrases : toucher = envoyer tout de suite, maintenir = mettre dans la saisie pour la modifier,
     « + » = enregistrer le texte en cours, ✎ = mode suppression. Gardé sur le téléphone (localStorage).
   - touche ⌨ = clavier Android pour ce message (émojis, autres langues).
   hud.js appelle CwKb.open() / CwKb.close() depuis setChatComposer().
   ===================================================================== */
(() => {
  "use strict";
  const input = document.querySelector(".floating-chat-composer input");
  const form = document.querySelector(".floating-chat-composer");
  if (!input || !form) return;

  const KEY = "cw.kb.macros";
  const MAX_MACROS = 24;
  const MAX_LEN = 128;
  const DEFAULTS = ["Bonjour tout le monde !", "Merci beaucoup !", "/me ", "/do ", "J'arrive, 2 minutes.", "Bien reçu."];
  const ACCENT = { e: "é", a: "à", u: "ù", i: "î", o: "ô", c: "ç", E: "É", A: "À", U: "Ù", I: "Î", O: "Ô", C: "Ç" };
  const L1 = ["azertyuiop", "qsdfghjklm", "wxcvbn'"];
  const L2 = ["1234567890", "éèêàçùôî!?", "-_():;\"@"];

  let macros = [];
  try { macros = JSON.parse(window.localStorage.getItem(KEY) || "null"); } catch (_) { macros = null; }
  if (!Array.isArray(macros)) macros = DEFAULTS.slice();
  macros = macros.filter((m) => typeof m === "string" && m.trim()).slice(0, MAX_MACROS);

  let layer = 0, shift = 0, editMode = false, isOpen = false, nativeOnce = false;
  const T = (k, d) => (window.CwI18n ? window.CwI18n.t(k, d) : d);

  const kb = document.createElement("section");
  kb.className = "cw-kb";
  kb.id = "cwKb";
  kb.hidden = true;
  kb.setAttribute("aria-label", "Clavier Crestwood");
  kb.innerHTML = `
    <div class="cw-kb-macros">
      <span class="cw-kb-tag">Phrases</span>
      <div class="cw-kb-chips" id="cwKbChips"></div>
      <button type="button" class="cw-kb-mbtn" data-kb="save" aria-label="Enregistrer la phrase">+</button>
      <button type="button" class="cw-kb-mbtn" data-kb="edit" aria-label="Gérer les phrases">✎</button>
      <button type="button" class="cw-kb-mbtn" data-kb="hide" aria-label="Fermer le chat">✕</button>
    </div>
    <div class="cw-kb-keys" id="cwKbKeys"></div>`;
  document.querySelector(".hud-root")?.appendChild(kb);
  const keys = kb.querySelector("#cwKbKeys");
  const chips = kb.querySelector("#cwKbChips");

  const save = () => { try { window.localStorage.setItem(KEY, JSON.stringify(macros)); } catch (_) {} };
  const canBuzz = typeof navigator.vibrate === "function";
  const buzz = (ms) => { if (canBuzz) { try { navigator.vibrate(ms); } catch (_) {} } };
  const hold = (e) => { e.stopPropagation(); };

  // [v20] saisie plus rapide : la lettre s'ecrit AU TOUCHER (pointerdown) et pas au relachement,
  // l'evenement "input" est regroupe sur une image, une seule bulle d'apercu partagee.
  let inputQueued = false;
  function notifyInput() {
    if (inputQueued) return;
    inputQueued = true;
    window.requestAnimationFrame(() => {
      inputQueued = false;
      input.dispatchEvent(new Event("input", { bubbles: true }));
      input.scrollLeft = 1e6;
    });
  }
  function insert(txt) {
    const v = input.value;
    if (v.length + txt.length > MAX_LEN) return;
    input.value = v + txt;
    notifyInput();
    if (shift === 1) { shift = 0; relabel(); }
  }
  function replaceLast(txt) {
    const chars = Array.from(input.value);
    if (!chars.length) return insert(txt);
    chars[chars.length - 1] = txt;
    input.value = chars.join("");
    notifyInput();
  }
  function backspace() {
    const v = input.value;
    if (!v) return;
    input.value = Array.from(v).slice(0, -1).join("");
    notifyInput();
  }
  function submit() {
    if (!input.value.trim()) return;
    input.dispatchEvent(new Event("input", { bubbles: true }));
    if (typeof form.requestSubmit === "function") form.requestSubmit();
    else form.dispatchEvent(new Event("submit", { cancelable: true, bubbles: true }));
  }
  function sendText(t) {
    input.value = t;
    submit();
  }

  // bulle d'apercu au-dessus de la touche (comme un vrai clavier), un seul element pour tout le clavier
  const pop = document.createElement("b");
  pop.className = "cw-kb-pop";
  pop.hidden = true;
  kb.appendChild(pop);
  // [v21] position de la bulle memorisee par touche : plus de calcul de mise en page a chaque lettre
  let popPos = new WeakMap();
  function showPop(b) {
    if (!b.classList.contains("letter")) return;
    let t = popPos.get(b);
    if (!t) {
      const r = b.getBoundingClientRect();
      const k = kb.getBoundingClientRect();
      t = "translate(" + Math.round(r.left - k.left + r.width / 2) + "px," + Math.round(r.top - k.top) + "px)";
      popPos.set(b, t);
    }
    pop.textContent = b.textContent;
    pop.style.transform = t;
    pop.hidden = false;
  }
  function hidePop() { pop.hidden = true; }

  // une touche = { label, cls, tap(b), long(b, repeat) } ; un seul jeu d'ecouteurs sur le conteneur (delegation)
  const keyDefs = new WeakMap();
  function keyBtn(label, cls, onTap, onLong, commitOnDown) {
    const b = document.createElement("button");
    b.type = "button";
    b.className = "cw-kb-key" + (cls ? " " + cls : "");
    b.textContent = label;
    keyDefs.set(b, { onTap, onLong, down: commitOnDown !== false });
    return b;
  }
  let active = null;   // { b, def, t, rep, longDone, pid }
  function release() {
    if (!active) return;
    window.clearTimeout(active.t);
    window.clearInterval(active.rep);
    active.b.classList.remove("down");
    active = null;
    hidePop();
  }
  keys.addEventListener("pointerdown", (e) => {
    const b = e.target.closest(".cw-kb-key");
    if (!b) return;
    e.preventDefault();
    e.stopPropagation();
    const def = keyDefs.get(b);
    if (!def) return;
    release();
    active = { b, def, t: 0, rep: 0, longDone: false, pid: e.pointerId };
    b.classList.add("down");
    buzz(6);
    if (def.down) { def.onTap(b); window.requestAnimationFrame(() => { if (active && active.b === b) showPop(b); }); }   // lettre ecrite tout de suite, bulle a l'image suivante
    if (def.onLong) {
      const a = active;
      a.t = window.setTimeout(() => {
        if (active !== a) return;
        a.longDone = true;
        def.onLong(b, (fn) => { a.rep = window.setInterval(fn, 55); });
      }, 380);
    }
  });
  const up = (e) => {
    if (!active || (e.pointerId !== undefined && e.pointerId !== active.pid)) return;
    e.stopPropagation();
    const a = active;
    release();
    if (!a.def.down && !a.longDone) a.def.onTap(a.b);   // touches de fonction : au relachement
  };
  keys.addEventListener("pointerup", up);
  keys.addEventListener("pointercancel", () => release());
  keys.addEventListener("pointerleave", () => release());
  keys.addEventListener("click", (e) => { e.preventDefault(); e.stopPropagation(); });

  function row(cls) { const r = document.createElement("div"); r.className = "cw-kb-row" + (cls ? " " + cls : ""); keys.appendChild(r); return r; }

  // [v19b] le clavier est construit UNE fois par couche (lettres / chiffres) ; Maj ne change que les libelles
  let letterKeys = [];
  let shiftKey = null;
  let builtLayer = -1;
  const cur = (ch) => (layer === 0 && shift ? ch.toUpperCase() : ch);
  function relabel() {
    for (let k = 0; k < letterKeys.length; k++) letterKeys[k].b.textContent = cur(letterKeys[k].ch);
    if (shiftKey) {
      shiftKey.textContent = shift === 2 ? "⇪" : "⇧";
      shiftKey.classList.toggle("on", shift > 0);
    }
  }
  function render() {
    if (builtLayer === layer) { relabel(); return; }
    builtLayer = layer;
    letterKeys = [];
    shiftKey = null;
    keys.replaceChildren();
    const src = layer === 0 ? L1 : L2;
    src.forEach((line, i) => {
      const r = row("r" + i);
      if (i === 2) {
        if (layer === 0) {
          shiftKey = keyBtn("⇧", "fn", () => { shift = shift === 0 ? 1 : (shift === 1 ? 2 : 0); relabel(); }, null, true);
          r.appendChild(shiftKey);
        } else r.appendChild(keyBtn("=+", "fn", () => insert("+")));
      }
      Array.from(line).forEach((ch) => {
        const hasAccent = Boolean(ACCENT[ch] || ACCENT[ch.toUpperCase()]);
        const b = keyBtn(cur(ch), "letter", () => insert(cur(ch)), hasAccent ? () => { const a = ACCENT[cur(ch)]; if (a) { buzz(14); replaceLast(a); } } : null);
        letterKeys.push({ b, ch });
        r.appendChild(b);
      });
      if (i === 2) r.appendChild(keyBtn("⌫", "fn wide1", () => backspace(), (b, repeat) => { backspace(); repeat(backspace); }, true));
    });
    const r = row("r3");
    r.appendChild(keyBtn(layer === 0 ? "?123" : "ABC", "fn wide1", () => { layer = layer ? 0 : 1; render(); }, null, false));
    r.appendChild(keyBtn("/", "letter", () => insert("/")));
    r.appendChild(keyBtn(",", "letter", () => insert(",")));
    r.appendChild(keyBtn(T("kb.space", "espace"), "space", () => insert(" ")));
    r.appendChild(keyBtn(".", "letter", () => insert(".")));
    r.appendChild(keyBtn("⌨", "fn", () => useAndroidKeyboard(), null, false));
    r.appendChild(keyBtn(T("kb.send", "Envoyer"), "send", () => submit(), null, false));
    relabel();
  }

  function renderChips() {
    chips.replaceChildren();
    kb.classList.toggle("editing", editMode);
    if (!macros.length) {
      const e = document.createElement("span");
      e.className = "cw-kb-empty";
      e.textContent = "Écris une phrase puis touche + pour l'enregistrer";
      chips.appendChild(e);
      return;
    }
    macros.forEach((m, i) => {
      const c = document.createElement("button");
      c.type = "button";
      c.className = "cw-kb-chip";
      c.textContent = m.length > 26 ? m.slice(0, 25) + "…" : m;
      c.title = m;
      let t = 0, longDone = false;
      c.addEventListener("pointerdown", (e) => {
        hold(e);
        longDone = false;
        if (editMode) return;
        t = window.setTimeout(() => { longDone = true; buzz(16); input.value = m; input.dispatchEvent(new Event("input", { bubbles: true })); }, 420);
      });
      const end = () => window.clearTimeout(t);
      c.addEventListener("pointerleave", end);
      c.addEventListener("pointercancel", end);
      c.addEventListener("click", (e) => {
        e.preventDefault(); e.stopPropagation(); end();
        if (editMode) { macros.splice(i, 1); save(); renderChips(); return; }
        if (longDone) return;
        buzz(10);
        // une phrase qui finit par un espace (ex. "/me ") se complète avant l'envoi
        if (/\s$/.test(m)) { input.value = m; input.dispatchEvent(new Event("input", { bubbles: true })); }
        else sendText(m);
      });
      chips.appendChild(c);
    });
  }

  kb.querySelectorAll(".cw-kb-mbtn").forEach((b) => {
    b.addEventListener("pointerdown", hold);
    b.addEventListener("click", (e) => {
      e.preventDefault(); e.stopPropagation();
      const a = b.dataset.kb;
      if (a === "save") {
        const v = input.value.trim().slice(0, MAX_LEN);
        if (!v) { flash("Écris d'abord la phrase à garder"); return; }
        if (!macros.includes(v)) { macros.unshift(v); macros = macros.slice(0, MAX_MACROS); save(); }
        editMode = false;
        renderChips();
        flash("Phrase enregistrée");
      } else if (a === "edit") {
        editMode = !editMode;
        renderChips();
        if (editMode) flash("Touche une phrase pour la supprimer");
      } else if (a === "hide") {
        window.CwHudChat?.close?.();
      }
    });
  });

  let flashT = 0;
  function flash(msg) {
    kb.dataset.flash = msg;
    kb.classList.add("flash");
    window.clearTimeout(flashT);
    flashT = window.setTimeout(() => kb.classList.remove("flash"), 1600);
  }

  function useAndroidKeyboard() {
    nativeOnce = true;
    close();
    input.readOnly = false;
    input.removeAttribute("inputmode");
    try { input.focus({ preventScroll: true }); } catch (_) { input.focus(); }
    window.SampHud?.showAndroidKeyboard?.();
  }

  function open() {
    if (nativeOnce) return false;          // ce message : clavier Android demandé
    isOpen = true;
    input.readOnly = true;
    input.setAttribute("inputmode", "none");
    input.blur();
    kb.hidden = false;
    document.body.classList.add("cw-kb-open");
    editMode = false;
    layer = 0; shift = 0;
    popPos = new WeakMap();   // [v21] l'ecran a pu tourner / changer de taille
    render();                 // reutilise les touches deja construites
    renderChips();
    window.setTimeout(() => window.crestwoodSyncTouchBounds?.(), 30);
    return true;
  }
  function close() {
    isOpen = false;
    kb.hidden = true;
    document.body.classList.remove("cw-kb-open");
    window.setTimeout(() => window.crestwoodSyncTouchBounds?.(), 30);
  }
  // le chat se ferme : on revient au clavier Crestwood pour le prochain message
  function reset() { nativeOnce = false; input.readOnly = false; input.removeAttribute("inputmode"); close(); }

  kb.addEventListener("pointerdown", hold);
  kb.addEventListener("touchstart", hold, { passive: true });
  window.CwKb = { open, close: reset, isOpen: () => isOpen, element: kb };
})();
