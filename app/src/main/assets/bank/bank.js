/* ============================================================
   Banque de Crestwood — logique (pont avec le jeu)
   JS  --SampBank.action(type, json)-->  natif  -->  serveur (/bankact)
   JS  <--BankApp.openFromNative / onUpdateFromNative / notifyFromNative / closeFromNative--
   payload : { player, cash, balance, hasPin, cardLast, ok, msg, history:[{desc,date,amount}] }
   ============================================================ */
const bridge = window.SampBank || null;

let hasPinRegistered = false;
let authenticated = false;
let currentRegPin = "";
let currentAuthPin = "";
let activeAction = "";
let pendingAmount = 0;
let pendingTarget = 0;
let authMode = "access";          // "access" (connexion) | "transaction"
let lastCash = 0, lastBalance = 0;

const ICON_IN  = '<svg class="history-item-icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><path d="M12 7v10M8 13l4 4 4-4"/></svg>';
const ICON_OUT = '<svg class="history-item-icon neg" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><path d="M12 17V7M8 11l4-4 4 4"/></svg>';

function showToast(message, type) {
    const t = document.getElementById("toast");
    t.innerText = message;
    t.className = "toast show " + (type || "");
    clearTimeout(t._timer);
    t._timer = setTimeout(() => t.classList.remove("show"), 2600);
}
function formatMoney(v) {
    return Math.floor(Number(v) || 0).toLocaleString("fr-FR").replace(/[  ]/g, " ") + " $";
}
function escapeHtml(s) {
    return String(s).replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[c]);
}
function parse(p) { if (typeof p === "string") { try { return JSON.parse(p); } catch (_) { return {}; } } return p || {}; }
function sendToServer(action, dataJson) {
    if (bridge && typeof bridge.action === "function") bridge.action(action, dataJson || "{}");
    else showToast("Aperçu : " + action, "");
}

// clavier numerique
document.querySelectorAll(".pad").forEach((pad) => {
    const type = pad.dataset.type;
    ["1","2","3","4","5","6","7","8","9","C","0","⌫"].forEach((k) => {
        const b = document.createElement("button");
        b.type = "button"; b.textContent = k;
        b.addEventListener("click", () => {
            if (k === "⌫") clearPin(type);
            else if (k === "C") { if (type === "reg") currentRegPin = ""; else currentAuthPin = ""; updatePinDisplays(); }
            else addPin(k, type);
        });
        pad.appendChild(b);
    });
});

window.BankApp = {
    openFromNative(payload) {
        payload = parse(payload);
        applyState(payload);
        const c = document.getElementById("bankContainer");
        c.style.display = "grid";
        c.classList.remove("animating", "closing");
        void c.offsetWidth;
        c.classList.add("animating");
        authenticated = false;
        if (!payload.hasPin) { currentRegPin = ""; updatePinDisplays(); showOverlay("registerOverlay"); }
        else openLogin();
        try { if (bridge && typeof bridge.ready === "function") bridge.ready(); } catch (_) {}
        if (payload.msg) showToast(payload.msg, payload.ok ? "success" : "error");
    },
    closeFromNative() { closeBank(true); },
    onUpdateFromNative(payload) {
        payload = parse(payload);
        applyState(payload);
        if (payload.msg) showToast(payload.msg, payload.ok ? "success" : "error");
        if (payload.hasPin === true && payload.ok) {
            closeOverlay("registerOverlay");
            if (authMode === "access" && !authenticated) {
                authenticated = true;
                currentAuthPin = ""; updatePinDisplays();
                closeOverlay("authOverlay");
                return;
            }
            currentAuthPin = ""; updatePinDisplays();
            closeOverlay("authOverlay");
            closeOverlay("actionModal");
        } else if (!payload.ok) {
            currentAuthPin = ""; updatePinDisplays();
        }
    },
    notifyFromNative(payload) {
        payload = parse(payload);
        if (payload.msg) showToast(payload.msg, payload.ok ? "success" : "error");
    }
};

function applyState(p) {
    if (!p) return;
    if (p.player !== undefined) document.getElementById("playerName").innerText = String(p.player).replace(/_/g, " ");
    if (p.cash !== undefined) { lastCash = Number(p.cash) || 0; document.getElementById("playerCash").innerText = formatMoney(p.cash); }
    if (p.balance !== undefined) { lastBalance = Number(p.balance) || 0; document.getElementById("bankBalance").innerText = formatMoney(p.balance); }
    if (p.cardLast !== undefined) document.getElementById("cardNumber").innerHTML = "&bull;&bull;&bull;&bull; &bull;&bull;&bull;&bull; &bull;&bull;&bull;&bull; " + escapeHtml(p.cardLast);
    if (p.hasPin !== undefined) hasPinRegistered = !!p.hasPin;
    if (Array.isArray(p.history)) renderHistory(p.history);
}

function renderHistory(items) {
    const list = document.getElementById("historyList");
    list.innerHTML = "";
    if (!items.length) { list.innerHTML = '<div class="empty-history">Aucune opération récente</div>'; return; }
    items.forEach((it) => {
        const pos = Number(it.amount) >= 0;
        const d = document.createElement("div");
        d.className = "history-item";
        d.innerHTML = '<div class="history-item-info">' + (pos ? ICON_IN : ICON_OUT) +
            '<div><span>' + escapeHtml(it.desc || "Opération") + '</span><div class="history-item-date">' + escapeHtml(it.date || "") + '</div></div></div>' +
            '<div class="history-item-amount' + (pos ? "" : " neg") + '">' + (pos ? "+ " : "- ") + formatMoney(Math.abs(Number(it.amount))) + '</div>';
        list.appendChild(d);
    });
}

// ---- connexion ----
function openLogin() {
    authMode = "access";
    document.getElementById("authSubtitle").innerText = "Entre ton code pour accéder au compte";
    document.getElementById("btnAuthConfirm").innerText = "Accéder au compte";
    document.getElementById("btnAuthCancel").style.display = "none";
    currentAuthPin = ""; updatePinDisplays();
    showOverlay("authOverlay");
}
function confirmAuth() {
    if (currentAuthPin.length !== 4) { showToast("Entre les 4 chiffres de ton code !", "error"); return; }
    const pin = parseInt(currentAuthPin, 10);
    if (authMode === "access") { sendToServer("verify_pin", JSON.stringify({ pin: pin })); showToast("Vérification…"); }
    else validateTransaction(pin);
}
function cancelAuth() {
    if (authMode === "access") { closeBank(false); return; }
    closeOverlay("authOverlay");
}

// ---- code ----
function updatePinDisplays() {
    document.getElementById("regPinDisplay").innerText = "•".repeat(currentRegPin.length).padEnd(4, "○");
    document.getElementById("authPinDisplay").innerText = "•".repeat(currentAuthPin.length).padEnd(4, "○");
}
function addPin(num, type) {
    if (type === "reg" && currentRegPin.length < 4) currentRegPin += num;
    else if (type === "auth" && currentAuthPin.length < 4) currentAuthPin += num;
    updatePinDisplays();
    if (type === "auth" && currentAuthPin.length === 4 && authMode === "access") confirmAuth();
}
function clearPin(type) {
    if (type === "reg") currentRegPin = currentRegPin.slice(0, -1);
    else currentAuthPin = currentAuthPin.slice(0, -1);
    updatePinDisplays();
}
function saveInitialPin() {
    if (currentRegPin.length !== 4) { showToast("Ton code doit faire 4 chiffres !", "error"); return; }
    sendToServer("register_pin", JSON.stringify({ pin: parseInt(currentRegPin, 10) }));
    showToast("Enregistrement du code…");
    currentRegPin = ""; updatePinDisplays();
}

// ---- operations ----
const LABELS = {
    retirer:  { title: "Retirer de l'argent",  hint: "L'argent passe de ton compte à ta poche.",  btn: "Continuer" },
    deposer:  { title: "Déposer de l'argent",  hint: "L'argent liquide passe sur ton compte.",    btn: "Continuer" },
    virer:    { title: "Faire un virement",    hint: "Envoie de l'argent sur le compte d'un joueur (son ID).", btn: "Continuer" }
};
function openActionModal(type) {
    activeAction = type;
    const L = LABELS[type];
    const pass = document.getElementById("inputPassport");
    const amount = document.getElementById("inputAmount");
    document.getElementById("modalTag").innerText = L.title;
    document.getElementById("modalHint").innerText = L.hint;
    document.getElementById("btnModalConfirm").innerText = L.btn;
    amount.value = ""; pass.value = "";
    pass.style.display = type === "virer" ? "block" : "none";
    amount.placeholder = "Montant en dollars";
    const max = type === "deposer" ? lastCash : lastBalance;
    const q = document.getElementById("quickAmounts");
    q.innerHTML = "";
    [1000, 5000, 20000, "Tout"].forEach((v) => {
        const b = document.createElement("button");
        b.type = "button";
        b.textContent = v === "Tout" ? "Tout" : (v / 1000) + " k";
        b.addEventListener("click", () => { amount.value = v === "Tout" ? max : v; });
        q.appendChild(b);
    });
    showOverlay("actionModal");
}
function proceedToAuth() {
    const amount = parseInt(document.getElementById("inputAmount").value, 10);
    if (!amount || amount <= 0) { showToast("Entre un montant valide !", "error"); return; }
    pendingAmount = amount;
    pendingTarget = parseInt(document.getElementById("inputPassport").value, 10);
    if (isNaN(pendingTarget)) pendingTarget = -1;
    if (activeAction === "virer" && pendingTarget < 0) { showToast("Entre l'ID du joueur !", "error"); return; }
    closeOverlay("actionModal");
    authMode = "transaction";
    document.getElementById("authSubtitle").innerText = "Confirme avec ton code : " + formatMoney(pendingAmount);
    document.getElementById("btnAuthConfirm").innerText = "Valider l'opération";
    document.getElementById("btnAuthCancel").style.display = "block";
    currentAuthPin = ""; updatePinDisplays();
    showOverlay("authOverlay");
}
function validateTransaction(pin) {
    const map = { retirer: "withdraw", deposer: "deposit", virer: "transfer" };
    sendToServer(map[activeAction], JSON.stringify({ amount: pendingAmount, pin: pin, target: Math.max(0, pendingTarget) }));
    showToast("Opération en cours…");
}

// ---- fenetres ----
function showOverlay(id) { document.getElementById(id).style.display = "flex"; }
function closeOverlay(id) { const e = document.getElementById(id); if (e) e.style.display = "none"; }

let closing = false;
function closeBank(fromNative) {
    ["registerOverlay", "authOverlay", "actionModal"].forEach(closeOverlay);
    const c = document.getElementById("bankContainer");
    c.classList.remove("animating");
    if (fromNative) { closing = false; c.classList.remove("closing"); c.style.display = "none"; return; }
    if (closing) return;
    closing = true;
    c.classList.add("closing");
    setTimeout(() => {
        c.classList.remove("closing");
        c.style.display = "none";
        closing = false;
        if (bridge && typeof bridge.close === "function") bridge.close();
    }, 350);
}

// apercu hors jeu
if (!bridge) {
    setTimeout(() => BankApp.openFromNative({ player: "Anzo_Smith", cash: 12500, balance: 845000, hasPin: true, cardLast: "4821",
        history: [{ desc: "Salaire LSPD", date: "31/10 18:02", amount: 15000 }, { desc: "Retrait", date: "31/10 17:40", amount: -5000 },
                  { desc: "Virement de Jay_Kent", date: "30/10 22:11", amount: 120000 }, { desc: "Station-service", date: "30/10 21:03", amount: -462 }] }), 50);
}
