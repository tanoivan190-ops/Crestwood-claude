// ============================================================
//  Crestwood RP - "Instagram" de la ville (photos prises en jeu par les joueurs)
//  Node.js 14+ SANS module a installer :   node ig-server.js
//  A lancer sur la MEME machine que le serveur SA-MP (le gamemode lui parle en local).
// ------------------------------------------------------------
//  Reglages (variables d'environnement, ou valeurs par defaut ci-dessous) :
//    IG_PORT    port HTTP ouvert aux telephones (defaut 7790)   -> ouvre-le dans le pare-feu
//    IG_SECRET  mot de passe partage avec le gamemode (IG_SECRET dans include/Insta.inc)
//  Donnees : ./data/posts.json + ./data/img/<id>.jpg
//  Routes :
//    POST /auth      (gamemode seulement) secret, name, token        -> le joueur peut publier
//    POST /delete    (gamemode seulement) secret, id                 -> moderation du staff
//    GET  /feed?before=<id>                                          -> 30 dernieres publications
//    GET  /user?name=<nom>                                           -> publications d'un joueur
//    GET  /img/<id>.jpg
//    POST /post      {token, caption, image:"data:image/jpeg;base64,..."}
//    POST /like      {token, id}          POST /comment {token, id, text}     GET /comments?id=<id>
//    GET  /notifs?token=<token>          (j'aime et commentaires sur mes photos)
//    [v45] statuts WhatsApp (24 h) :
//    POST /status   {token, caption, image:"data:image/jpeg;base64,..." (facultatif), color}
//    GET  /statuses                      -> statuts de moins de 24 h, du plus recent au plus ancien
//    GET  /simg/<id>.jpg                 POST /status/delete {token, id}
//    [v50] connexion Google : POST /gauth/verify {token,name} (appli)  POST /gauth/redeem (gamemode)
// ============================================================
"use strict";
const http = require("http");
const fs = require("fs");
const path = require("path");
const qs = require("querystring");
// [v50] connexion Google (Firebase) : routes /gauth/* (voir gauth.js)
let gauth = null;
try { gauth = require("./gauth.js"); } catch (e) { console.log("[gauth] module absent : " + e.message); }

const PORT = Number(process.env.IG_PORT || 7790);
const SECRET = String(process.env.IG_SECRET || "change-moi-crestwood");
const DIR = path.join(__dirname, "data");
const IMG = path.join(DIR, "img");
const DB = path.join(DIR, "posts.json");
fs.mkdirSync(IMG, { recursive: true });
const SIMG = path.join(DIR, "status");
fs.mkdirSync(SIMG, { recursive: true });

let db = { next: 1, posts: [] };
try { db = JSON.parse(fs.readFileSync(DB, "utf8")); } catch (_) {}
if (!Array.isArray(db.statuses)) db.statuses = [];
if (!db.snext) db.snext = 1;
const DAY = 24 * 3600;
function purgeStatuses() {
  const lim = Math.floor(Date.now() / 1000) - DAY;
  db.statuses = db.statuses.filter((x) => { if (x.ts >= lim) return true; if (x.img) fs.unlink(path.join(SIMG, x.id + ".jpg"), () => {}); return false; });
}
const lastStatus = new Map();
let saveT = null;
function save() { clearTimeout(saveT); saveT = setTimeout(() => fs.writeFile(DB, JSON.stringify(db), () => {}), 400); }

const tokens = new Map();        // token -> { name, exp }
const lastPost = new Map();      // name -> ms
const clean = (s, n) => String(s == null ? "" : s).replace(/[\u0000-\u001f<>]/g, " ").trim().slice(0, n);
const nameOf = (t) => { const e = tokens.get(String(t || "")); if (!e || e.exp < Date.now()) return null; return e.name; };
const pub = (p, me) => ({ id: p.id, name: p.name, caption: p.caption, ts: p.ts, likes: p.likes.length, liked: me ? p.likes.includes(me) : false, comments: p.comments.length, img: "img/" + p.id + ".jpg" });

function send(res, code, obj) {
  res.writeHead(code, { "Content-Type": "application/json; charset=utf-8", "Access-Control-Allow-Origin": "*", "Access-Control-Allow-Headers": "Content-Type", "Cache-Control": "no-store" });
  res.end(JSON.stringify(obj));
}
function body(req, max) {
  return new Promise((ok, ko) => {
    let n = 0; const parts = [];
    req.on("data", (c) => { n += c.length; if (n > max) { ko(new Error("trop gros")); req.destroy(); } else parts.push(c); });
    req.on("end", () => {
      const raw = Buffer.concat(parts).toString("utf8");
      if ((req.headers["content-type"] || "").includes("json")) { try { ok(JSON.parse(raw || "{}")); } catch (e) { ok({}); } }
      else ok(qs.parse(raw));
    });
    req.on("error", ko);
  });
}

http.createServer(async (req, res) => {
  try {
    const url = new URL(req.url, "http://x");
    const p = url.pathname;
    if (req.method === "OPTIONS") return send(res, 204, {});
    if (req.method === "GET" && p.startsWith("/img/")) {
      const id = parseInt(p.slice(5), 10);
      const f = path.join(IMG, id + ".jpg");
      if (!id || !fs.existsSync(f)) { res.writeHead(404); return res.end(); }
      res.writeHead(200, { "Content-Type": "image/jpeg", "Access-Control-Allow-Origin": "*", "Cache-Control": "public, max-age=86400" });
      return fs.createReadStream(f).pipe(res);
    }
    if (req.method === "GET" && p.startsWith("/simg/")) {
      const id = parseInt(p.slice(6), 10);
      const f = path.join(SIMG, id + ".jpg");
      if (!id || !fs.existsSync(f)) { res.writeHead(404); return res.end(); }
      res.writeHead(200, { "Content-Type": "image/jpeg", "Access-Control-Allow-Origin": "*", "Cache-Control": "public, max-age=86400" });
      return fs.createReadStream(f).pipe(res);
    }
    if (req.method === "GET" && p === "/statuses") {
      purgeStatuses();
      return send(res, 200, { ok: 1, statuses: db.statuses.slice(-120).reverse().map((x) => ({ id: x.id, name: x.name, caption: x.caption, color: x.color, ts: x.ts, img: x.img ? "simg/" + x.id + ".jpg" : "" })) });
    }
    if (req.method === "GET" && p === "/feed") {
      const me = nameOf(url.searchParams.get("token"));
      const before = parseInt(url.searchParams.get("before") || "0", 10);
      const list = db.posts.filter((x) => !before || x.id < before).slice(-30).reverse();
      return send(res, 200, { ok: 1, me, posts: list.map((x) => pub(x, me)) });
    }
    if (req.method === "GET" && p === "/user") {
      const me = nameOf(url.searchParams.get("token"));
      const who = clean(url.searchParams.get("name"), 24);
      return send(res, 200, { ok: 1, posts: db.posts.filter((x) => x.name === who).slice(-60).reverse().map((x) => pub(x, me)) });
    }
    if (req.method === "GET" && p === "/comments") {
      const post = db.posts.find((x) => x.id === parseInt(url.searchParams.get("id"), 10));
      return send(res, 200, { ok: 1, comments: post ? post.comments.slice(-50) : [] });
    }
    if (req.method === "GET" && p === "/notifs") {
      const me = nameOf(url.searchParams.get("token"));
      if (!me) return send(res, 401, { ok: 0, err: "session" });
      const out = [];
      db.posts.filter((x) => x.name === me).forEach((x) => {
        x.likeLog.forEach((l) => { if (l.n !== me) out.push({ t: "like", n: l.n, ts: l.ts, id: x.id }); });
        x.comments.forEach((c) => { if (c.n !== me) out.push({ t: "comment", n: c.n, txt: c.txt, ts: c.ts, id: x.id }); });
      });
      out.sort((a, b) => b.ts - a.ts);
      return send(res, 200, { ok: 1, notifs: out.slice(0, 40) });
    }
    if (gauth && p.startsWith("/gauth/") && await gauth.handle(req, res, p, body, send, SECRET)) return;
    if (req.method !== "POST") return send(res, 404, { ok: 0 });
    const b = await body(req, (p === "/post" || p === "/status") ? 900000 : 8000);
    if (p === "/auth") {
      if (b.secret !== SECRET) return send(res, 403, { ok: 0 });
      const name = clean(b.name, 24), token = clean(b.token, 40);
      if (!name || token.length < 12) return send(res, 400, { ok: 0 });
      tokens.set(token, { name, exp: Date.now() + 12 * 3600 * 1000 });
      return send(res, 200, { ok: 1 });
    }
    if (p === "/delete") {
      if (b.secret !== SECRET) return send(res, 403, { ok: 0 });
      const id = parseInt(b.id, 10), i = db.posts.findIndex((x) => x.id === id);
      if (i < 0) return send(res, 404, { ok: 0 });
      db.posts.splice(i, 1); fs.unlink(path.join(IMG, id + ".jpg"), () => {}); save();
      return send(res, 200, { ok: 1 });
    }
    const me = nameOf(b.token);
    if (!me) return send(res, 401, { ok: 0, err: "session" });
    if (p === "/post") {
      const now = Date.now();
      if (now - (lastPost.get(me) || 0) < 60000) return send(res, 429, { ok: 0, err: "Attends une minute entre deux publications." });
      const m = /^data:image\/jpeg;base64,([A-Za-z0-9+/=]+)$/.exec(String(b.image || ""));
      if (!m) return send(res, 400, { ok: 0, err: "Image invalide." });
      const buf = Buffer.from(m[1], "base64");
      if (buf.length < 500 || buf.length > 650000 || buf[0] !== 0xff || buf[1] !== 0xd8) return send(res, 400, { ok: 0, err: "Image invalide." });
      const id = db.next++;
      fs.writeFileSync(path.join(IMG, id + ".jpg"), buf);
      db.posts.push({ id, name: me, caption: clean(b.caption, 140), ts: Math.floor(now / 1000), likes: [], likeLog: [], comments: [] });
      if (db.posts.length > 2000) { const old = db.posts.shift(); fs.unlink(path.join(IMG, old.id + ".jpg"), () => {}); }
      lastPost.set(me, now); save();
      return send(res, 200, { ok: 1, id });
    }
    if (p === "/status") {
      const now = Date.now();
      if (now - (lastStatus.get(me) || 0) < 20000) return send(res, 429, { ok: 0, err: "Attends un peu avant un autre statut." });
      const id = db.snext++;
      let hasImg = false;
      if (b.image) {
        const m = /^data:image\/jpeg;base64,([A-Za-z0-9+/=]+)$/.exec(String(b.image));
        if (!m) return send(res, 400, { ok: 0, err: "Image invalide." });
        const buf = Buffer.from(m[1], "base64");
        if (buf.length < 500 || buf.length > 650000 || buf[0] !== 0xff || buf[1] !== 0xd8) return send(res, 400, { ok: 0, err: "Image invalide." });
        fs.writeFileSync(path.join(SIMG, id + ".jpg"), buf);
        hasImg = true;
      }
      const caption = clean(b.caption, 140);
      if (!hasImg && !caption) return send(res, 400, { ok: 0, err: "Statut vide." });
      const color = /^#[0-9a-fA-F]{6}$/.test(String(b.color || "")) ? String(b.color) : "#128c7e";
      db.statuses.push({ id, name: me, caption, color, ts: Math.floor(now / 1000), img: hasImg });
      purgeStatuses();
      lastStatus.set(me, now); save();
      return send(res, 200, { ok: 1, id });
    }
    if (p === "/status/delete") {
      const id = parseInt(b.id, 10), i = db.statuses.findIndex((x) => x.id === id && x.name === me);
      if (i < 0) return send(res, 404, { ok: 0 });
      const st = db.statuses.splice(i, 1)[0];
      if (st.img) fs.unlink(path.join(SIMG, id + ".jpg"), () => {});
      save();
      return send(res, 200, { ok: 1 });
    }
    const post = db.posts.find((x) => x.id === parseInt(b.id, 10));
    if (!post) return send(res, 404, { ok: 0, err: "Publication introuvable." });
    if (p === "/like") {
      const i = post.likes.indexOf(me);
      if (i >= 0) { post.likes.splice(i, 1); post.likeLog = post.likeLog.filter((l) => l.n !== me); }
      else { post.likes.push(me); post.likeLog.push({ n: me, ts: Math.floor(Date.now() / 1000) }); }
      save();
      return send(res, 200, { ok: 1, likes: post.likes.length, liked: i < 0 });
    }
    if (p === "/comment") {
      const txt = clean(b.text, 120);
      if (!txt) return send(res, 400, { ok: 0, err: "Commentaire vide." });
      post.comments.push({ n: me, txt, ts: Math.floor(Date.now() / 1000) });
      if (post.comments.length > 200) post.comments.shift();
      save();
      return send(res, 200, { ok: 1, comments: post.comments.length });
    }
    return send(res, 404, { ok: 0 });
  } catch (e) {
    return send(res, 400, { ok: 0, err: String(e.message || e) });
  }
}).listen(PORT, () => console.log("[instagram] port " + PORT + " - donnees dans " + DIR));
