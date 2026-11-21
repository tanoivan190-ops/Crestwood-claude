/* Crestwood RP v22 - introduction animee. Construit les lettres, joue ~5 s puis laisse la place a l'ecran de chargement. */
(function () {
  "use strict";
  var KEY = "cw.splashAt";
  var HOLD_MS = 5400;      // duree avant le fondu (les animations CSS durent ~4,6 s)
  var REPLAY_MS = 120000;  // ne rejoue pas l'intro si elle a deja ete vue il y a moins de 2 minutes
  var root = document.getElementById("splash");
  if (!root) return;

  function remove() { if (root && root.parentNode) root.parentNode.removeChild(root); root = null; }

  var recent = false;
  try { recent = Date.now() - Number(window.localStorage.getItem(KEY) || 0) < REPLAY_MS; } catch (e) { recent = false; }
  if (recent) { remove(); return; }
  try { window.localStorage.setItem(KEY, String(Date.now())); } catch (e) { /* stockage indisponible */ }

  var word = root.querySelector(".sp-word");
  var text = "CRESTWOOD";
  for (var i = 0; i < text.length; i++) {
    var s = document.createElement("span");
    s.textContent = text.charAt(i);
    s.style.setProperty("--i", String(i));
    if (i >= 5) s.className = "b";   // WOOD en bleu clair
    word.appendChild(s);
  }

  var done = false;
  function finish() {
    if (done || !root) return;
    done = true;
    root.classList.add("out");
    window.setTimeout(remove, 900);
  }
  window.setTimeout(finish, HOLD_MS);
  root.addEventListener("click", finish);
  root.addEventListener("touchstart", finish, { passive: true });
})();
