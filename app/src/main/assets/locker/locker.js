/* [v56] Vestiaire de faction : menu compact a gauche (facon GTA V).
   Serveur -> page : LockerApp.openFromNative(json) / LockerApp.onUpdateFromNative(json)
     {"fac","facName","type","rank","rankName","uniform","female","belt","vest","hat","police",
      "outfits":[[index,skin],...],"worn":index,"stock","gear":[[cle,libelle,info],...],"ok","msg"}
   Page -> serveur : SampLocker.action(type, {"arg":".."}) -> /lockeract <type> <arg> (SAMP.java)
     police : wear "<femme> <ceinture> <gilet> <casquette>" | civil | gear <cle>
     autres : wear <index de tenue> | civil | gear <cle>
   Le serveur re-verifie faction + distance a chaque action. */
(function () {
  "use strict";
  const bridge = window.SampLocker || null;
  const $ = (id) => document.getElementById(id);
  const OUTFIT_NAMES = { 250: "Tenue SAMU 1", 275: "Tenue SAMU 2" };
  const OUTFIT_DESC = { 250: "Tenue de service du SAMU.", 275: "Tenue de service du SAMU, variante." };
  const S = { police: 0, facName: "", rankName: "", uniform: 0, female: 0, belt: 1, vest: 1, hat: 0, outfits: [], worn: -1, gear: [], stock: -1 };
  let sel = "";

  function parse(raw) { if (raw && typeof raw === "object") return raw; try { return JSON.parse(raw || "{}") || {}; } catch (_) { return {}; } }
  function send(type, arg) {
    const payload = JSON.stringify({ arg: arg == null ? "" : String(arg) });
    if (bridge && typeof bridge.action === "function") { try { bridge.action(type, payload); } catch (_) {} }
    else console.log("[locker]", type, payload);
  }
  let toastT = 0;
  function toast(msg, ok) {
    if (!msg) return;
    const t = $("lkToast");
    t.textContent = msg;
    t.classList.toggle("err", !ok);
    t.hidden = false;
    clearTimeout(toastT);
    toastT = setTimeout(() => { t.hidden = true; }, 3000);
  }

  // une ligne : {id, label, value, valueClass, desc, off, run}
  function rows() {
    const out = [];
    if (S.police) {
      out.push({ sec: "Tenue de service" });
      out.push({ id: "model", label: "Modèle", value: S.female ? "Femme" : "Homme", cls: "arrows", desc: "Corps de la tenue de police.", run: () => { S.female = S.female ? 0 : 1; } });
      [["belt", "Ceinture", "Ceinture d'équipement."], ["vest", "Gilet", "Gilet pare-balles (visuel)."], ["hat", "Casquette", "Casquette d'officier."]].forEach(([k, l, d]) =>
        out.push({ id: k, label: l, value: S[k] ? "Oui" : "Non", cls: S[k] ? "on" : "", desc: d, run: () => { S[k] = S[k] ? 0 : 1; } }));
      out.push({ id: "wear", label: S.uniform ? "Mettre à jour la tenue" : "Enfiler la tenue", value: "", desc: "Tu te changes tout de suite.", run: () => send("wear", [S.female, S.belt, S.vest, S.hat].join(" ")) });
    } else if (S.outfits.length) {
      out.push({ sec: "Tenues" });
      S.outfits.forEach(([i, skin]) => {
        const worn = S.uniform && S.worn === i;
        out.push({ id: "o" + i, label: OUTFIT_NAMES[skin] || "Tenue " + (i + 1), value: worn ? "Portée" : "", cls: worn ? "on" : "", desc: OUTFIT_DESC[skin] || "Tenue de service de la faction.", run: () => { if (!worn) send("wear", i); } });
      });
    }
    out.push({ id: "civil", label: "Tenue civile", value: S.uniform ? "" : "Portée", cls: S.uniform ? "" : "on", off: !S.uniform, desc: "Remets tes vêtements de tous les jours.", run: () => { if (S.uniform) send("civil", ""); } });
    if (S.gear.length) {
      out.push({ sec: "Armurerie" });
      S.gear.forEach(([key, label, info]) => out.push({ id: "g" + key, label, value: info || "", desc: S.stock >= 0 ? "Munitions de la faction : " + Number(S.stock).toLocaleString("fr-FR") + " (livrées par l'armée)." : "Prendre : " + label + ".", run: () => send("gear", key) }));
    }
    return out;
  }

  function render() {
    $("lkFac").textContent = S.facName || "Faction";
    $("lkRank").textContent = S.rankName || "";
    const st = $("lkState");
    st.textContent = S.uniform ? "En service" : "Tenue civile";
    st.classList.toggle("on", !!S.uniform);
    const list = $("lkList");
    const top = list.scrollTop;
    list.replaceChildren();
    let selDesc = "";
    rows().forEach((r) => {
      if (r.sec) { const h = document.createElement("div"); h.className = "lk-sec"; h.textContent = r.sec; list.appendChild(h); return; }
      const b = document.createElement("button");
      b.type = "button";
      b.className = "lk-row" + (r.id === sel ? " sel" : "") + (r.off ? " off" : "");
      const a = document.createElement("span"); a.textContent = r.label; b.appendChild(a);
      if (r.value) { const e = document.createElement("em"); e.textContent = r.value; if (r.cls) e.className = r.cls; b.appendChild(e); }
      b.addEventListener("click", () => { sel = r.id; r.run(); render(); });
      if (r.id === sel) selDesc = r.desc || "";
      list.appendChild(b);
    });
    list.scrollTop = top;
    $("lkDesc").textContent = selDesc || "Touche une ligne pour te changer ou prendre ton équipement.";
  }

  function apply(d) {
    S.facName = d.facName || "";
    S.rankName = d.rankName || "";
    S.uniform = Number(d.uniform) ? 1 : 0;
    S.female = Number(d.female) ? 1 : 0;
    S.belt = Number(d.belt) ? 1 : 0;
    S.vest = Number(d.vest) ? 1 : 0;
    S.hat = Number(d.hat) ? 1 : 0;
    S.police = d.police === undefined ? 1 : (Number(d.police) ? 1 : 0);
    S.outfits = Array.isArray(d.outfits) ? d.outfits : [];
    S.worn = d.worn == null ? -1 : Number(d.worn);
    S.gear = Array.isArray(d.gear) ? d.gear : [];
    S.stock = d.stock == null ? -1 : Number(d.stock);
    render();
  }

  $("lkClose").addEventListener("click", () => { if (bridge && typeof bridge.close === "function") { try { bridge.close(); } catch (_) {} } });

  window.LockerApp = {
    openFromNative(raw) { sel = ""; apply(parse(raw)); },
    onUpdateFromNative(raw) { const d = parse(raw); apply(d); if (d.msg) toast(d.msg, d.ok === 1 || d.ok === true); }
  };

  // apercu navigateur : ?ems / sinon police
  if (!bridge) {
    document.body.style.background = "#222 url('../loading/scenes/cw19-d.jpg') center/cover";
    if (/ems/.test(location.search)) window.LockerApp.openFromNative({ facName: "EMS Los Santos", rankName: "Ambulancier", uniform: 1, police: 0, outfits: [[0, 250], [1, 275]], worn: 0, gear: [["soin", "Trousse de soin", "santé 100"], ["bandages", "Bandages", "x3 inventaire"], ["extincteur", "Extincteur", ""]], stock: -1 });
    else window.LockerApp.openFromNative({ facName: "LSPD", rankName: "Officier", uniform: 0, police: 1, female: 0, belt: 1, vest: 1, hat: 0, stock: 1240, gear: [["matraque", "Matraque", ""], ["taser", "Taser", "50 balles"], ["deagle", "Desert Eagle", "70 balles"], ["gilet", "Gilet pare-balles", "armure 100"]] });
  }
  try { if (bridge && typeof bridge.ready === "function") bridge.ready(); } catch (_) {}
})();
