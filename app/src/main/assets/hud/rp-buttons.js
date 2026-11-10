(() => {
  const send = (control, pressed) => {
    try { window.AndroidHUD?.onHudAction?.("vehicle:control", JSON.stringify({ control, pressed })); } catch (_) {}
  };
  document.querySelectorAll(".rp-btn").forEach((btn) => {
    const key = btn.dataset.rp;
    let id = null;
    btn.addEventListener("pointerdown", (e) => {
      if (id !== null) return;
      e.preventDefault();
      id = e.pointerId;
      try { btn.setPointerCapture(id); } catch (_) {}
      btn.classList.add("rp-down");
      send(key, true);
      try { navigator.vibrate?.(8); } catch (_) {}
    });
    const up = (e) => {
      if (e.pointerId !== id) return;
      id = null;
      btn.classList.remove("rp-down");
      send(key, false);
    };
    ["pointerup", "pointercancel", "lostpointercapture"].forEach((n) => btn.addEventListener(n, up));
    btn.addEventListener("contextmenu", (e) => e.preventDefault());
  });
  setTimeout(() => window.crestwoodSyncTouchBounds?.(), 300);
})();
