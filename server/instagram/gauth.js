// ============================================================
//  [v50] Crestwood RP - Connexion Google (Firebase Authentication)
//  Charge par ig-server.js (meme port, meme processus). Aucun module a installer.
// ------------------------------------------------------------
//  1. L'appli connecte le joueur a Google puis a Firebase, et recoit un "ID token" Firebase.
//  2. L'appli l'envoie ici :  POST /gauth/verify  {"token":"<ID token>","name":"Prenom_Nom"}
//     -> la signature RS256 est verifiee avec les certificats publics de Google
//        (iss / aud = projet Firebase, exp, iat, sub, fournisseur google.com)
//     -> on rend un TICKET court a usage unique (2 minutes) : {"ok":1,"ticket":"..."}
//  3. L'appli envoie /authact google <ticket> au serveur SA-MP (une commande fait 128 caracteres
//     max, l'ID token en fait ~1000 : d'ou le ticket).
//  4. Le gamemode echange le ticket EN LOCAL :  POST /gauth/redeem  secret=..&ticket=..
//     -> "OK|<ticket>|<uid>|<email>|<pseudo>"   ou   "ERR|<raison>"
//  Reglage : GAUTH_PROJECT = project_id Firebase accepte(s), separes par des virgules.
//  [v52] defaut "crestwoodrp-3e53d,gta-2-11" : le nouveau projet ET l'ancien (le temps de changer
//        app/google-services.json). Quand tout le monde a la nouvelle APK : GAUTH_PROJECT=crestwoodrp-3e53d
// ============================================================
"use strict";
const https = require("https");
const crypto = require("crypto");

const PROJECTS = String(process.env.GAUTH_PROJECT || "crestwoodrp-3e53d,gta-2-11").split(",").map((x) => x.trim()).filter(Boolean);
const PROJECT = PROJECTS[0];
const CERT_URL = "https://www.googleapis.com/robot/v1/metadata/x509/securetoken@system.gserviceaccount.com";
const TICKET_TTL = 120 * 1000;

let certs = {}, certsExp = 0, certsLoading = null;
const tickets = new Map();     // ticket -> { uid, email, name, exp }
const usedTokens = new Map();  // sha256(token) -> exp   (un ID token ne sert qu'une fois)
const perIp = new Map();       // ip -> { n, t }

function loadCerts() {
  if (Date.now() < certsExp && Object.keys(certs).length) return Promise.resolve(certs);
  if (certsLoading) return certsLoading;
  certsLoading = new Promise((ok, ko) => {
    https.get(CERT_URL, (r) => {
      let raw = "";
      r.on("data", (c) => { raw += c; });
      r.on("end", () => {
        certsLoading = null;
        try {
          const j = JSON.parse(raw);
          const m = /max-age=(\d+)/.exec(String(r.headers["cache-control"] || ""));
          certs = j;
          certsExp = Date.now() + Math.max(300, m ? Number(m[1]) : 3600) * 1000;
          ok(certs);
        } catch (e) { ko(new Error("certificats Google illisibles")); }
      });
    }).on("error", (e) => { certsLoading = null; ko(e); });
  });
  return certsLoading;
}

const b64u = (s) => Buffer.from(String(s).replace(/-/g, "+").replace(/_/g, "/"), "base64");

function publicKeyOf(pem) {
  try { if (crypto.X509Certificate) return new crypto.X509Certificate(pem).publicKey; } catch (_) {}
  return pem;
}

// Verifie un ID token Firebase. certsOverride : tests uniquement.
async function verifyIdToken(token, certsOverride, project) {
  const allowed = project ? [project] : PROJECTS;
  const parts = String(token || "").split(".");
  if (parts.length !== 3 || token.length > 4096) throw new Error("jeton mal forme");
  let head, pay;
  try { head = JSON.parse(b64u(parts[0]).toString("utf8")); pay = JSON.parse(b64u(parts[1]).toString("utf8")); }
  catch (_) { throw new Error("jeton illisible"); }
  if (head.alg !== "RS256" || !head.kid) throw new Error("algorithme refuse");
  const list = certsOverride || await loadCerts();
  const pem = list[head.kid];
  if (!pem) throw new Error("cle inconnue");
  const ok = crypto.verify("RSA-SHA256", Buffer.from(parts[0] + "." + parts[1]), publicKeyOf(pem), b64u(parts[2]));
  if (!ok) throw new Error("signature invalide");
  const now = Math.floor(Date.now() / 1000);
  if (allowed.indexOf(pay.aud) < 0) throw new Error("mauvais projet (" + pay.aud + ")");
  if (pay.iss !== "https://securetoken.google.com/" + pay.aud) throw new Error("mauvais emetteur");
  if (!(pay.exp > now)) throw new Error("jeton expire");
  if (!(pay.iat <= now + 300)) throw new Error("jeton du futur");
  if (pay.auth_time && pay.auth_time > now + 300) throw new Error("auth_time invalide");
  if (typeof pay.sub !== "string" || !pay.sub || pay.sub.length > 128) throw new Error("sub invalide");
  const prov = pay.firebase && pay.firebase.sign_in_provider;
  if (prov && prov !== "google.com") throw new Error("fournisseur " + prov + " refuse");
  return pay;
}

function makeTicket() {
  const c = "abcdefghijkmnpqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789";
  const r = crypto.randomBytes(20);
  let s = "";
  for (let i = 0; i < 20; i++) s += c[r[i] % c.length];
  return s;
}

function purge() {
  const now = Date.now();
  for (const [k, v] of tickets) if (v.exp < now) tickets.delete(k);
  for (const [k, v] of usedTokens) if (v < now) usedTokens.delete(k);
  for (const [k, v] of perIp) if (now - v.t > 60000) perIp.delete(k);
}
setInterval(purge, 30000).unref();

const cleanUid = (s) => String(s || "").replace(/[^A-Za-z0-9]/g, "").slice(0, 64);
const cleanEmail = (s) => { const e = String(s || "").toLowerCase().trim(); return /^[a-z0-9._+\-]+@[a-z0-9.\-]+\.[a-z]{2,}$/.test(e) && e.length <= 63 ? e : ""; };
const cleanName = (s) => { const n = String(s || "").trim(); return /^[A-Za-z0-9_\[\]\.\$@=()]{3,24}$/.test(n) ? n : ""; };

// Retourne true si la route est geree.
async function handle(req, res, p, body, send, secret) {
  if (p !== "/gauth/verify" && p !== "/gauth/redeem" && p !== "/gauth/ping") return false;
  if (p === "/gauth/ping") { send(res, 200, { ok: 1, project: PROJECT, projects: PROJECTS }); return true; }
  if (req.method !== "POST") { send(res, 405, { ok: 0 }); return true; }
  const b = await body(req, 8000);
  if (p === "/gauth/verify") {
    const ip = String(req.socket.remoteAddress || "");
    const rl = perIp.get(ip) || { n: 0, t: Date.now() };
    if (Date.now() - rl.t > 60000) { rl.n = 0; rl.t = Date.now(); }
    if (++rl.n > 12) { perIp.set(ip, rl); send(res, 429, { ok: 0, err: "Trop d'essais, patiente une minute." }); return true; }
    perIp.set(ip, rl);
    const token = String(b.token || "");
    const name = cleanName(b.name);
    const h = crypto.createHash("sha256").update(token).digest("hex");
    if (usedTokens.has(h)) { send(res, 409, { ok: 0, err: "Jeton deja utilise." }); return true; }
    try {
      const pay = await verifyIdToken(token);
      usedTokens.set(h, pay.exp * 1000);
      const t = makeTicket();
      tickets.set(t, { uid: cleanUid(pay.sub), email: pay.email_verified === false ? "" : cleanEmail(pay.email), name, exp: Date.now() + TICKET_TTL });
      console.log("[gauth] ticket pour " + (pay.email || pay.sub) + " (" + (name || "?") + ")");
      send(res, 200, { ok: 1, ticket: t });
    } catch (e) {
      console.log("[gauth] refus : " + e.message);
      send(res, 401, { ok: 0, err: "Connexion Google refusee (" + e.message + ")." });
    }
    return true;
  }
  // /gauth/redeem : gamemode seulement (texte brut, facile a lire en Pawn)
  res.writeHead(200, { "Content-Type": "text/plain; charset=utf-8", "Cache-Control": "no-store" });
  if (b.secret !== secret) { res.end("ERR|secret"); return true; }
  const t = String(b.ticket || "");
  const e = tickets.get(t);
  if (!e || e.exp < Date.now()) { tickets.delete(t); res.end("ERR|ticket"); return true; }
  tickets.delete(t);
  res.end("OK|" + t + "|" + e.uid + "|" + (e.email || "-") + "|" + (e.name || "-"));
  return true;
}

// tests hors ligne uniquement
function _testCerts(c) { certs = c; certsExp = Date.now() + 3600000; }

module.exports = { handle, verifyIdToken, PROJECT, PROJECTS, _testCerts };
