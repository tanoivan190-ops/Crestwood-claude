// Commande boutique -> message Discord (le webhook reste dans la variable DISCORD_WEBHOOK de Netlify)
// IMPORTANT : meme liste de packs que server/basicgm/include/PackShop.inc
const PACKS = [
  { f: 500, m: 25000 }, { f: 1000, m: 55000 }, { f: 2000, m: 120000 }, { f: 2500, m: 155000 },
  { f: 3200, m: 210000, v: ["Yamaha YZF-R1 (NRG-500)"] }, { f: 4000, m: 270000 }, { f: 4500, m: 310000 },
  { f: 6000, m: 430000, v: ["Subaru WRX STI (Sultan)"] }, { f: 9000, m: 700000, v: ["Nissan GT-R (Elegy)"] },
  { f: 14500, m: 1150000, v: ["Dodge Viper (Banshee)"] }, { f: 15000, m: 1200000, v: ["Ford GT (Bullet)"] },
  { f: 30000, m: 2600000, v: ["Lamborghini Aventador (Infernus)"] },
  { f: 45000, m: 4000000, v: ["Lamborghini Aventador (Infernus)", "Lamborghini Urus (Huntley)"] },
];
const seen = new Map();
const json = (o, s = 200) => new Response(JSON.stringify(o), { status: s, headers: { "content-type": "application/json" } });

export default async (req, context) => {
  if (req.method !== "POST") return json({ ok: false, error: "Methode invalide." }, 405);
  const hook = Netlify.env.get("DISCORD_WEBHOOK");
  if (!hook) return json({ ok: false, error: "Webhook non configure." }, 500);
  const ip = context.ip || "?";
  const now = Date.now();
  if (now - (seen.get(ip) || 0) < 15000) return json({ ok: false, error: "Patiente quelques secondes." }, 429);
  seen.set(ip, now);
  let b; try { b = await req.json(); } catch { return json({ ok: false, error: "Requete invalide." }, 400); }
  const pseudo = String(b.pseudo || ""), pack = parseInt(b.pack, 10);
  const wave = String(b.wave || "").replace(/[^\w .+-]/g, "").slice(0, 40);
  if (!/^[A-Z][a-z]{0,10}_[A-Z][a-z]{0,10}$/.test(pseudo) || !(pack >= 1 && pack <= PACKS.length) || wave.length < 4)
    return json({ ok: false, error: "Donnees invalides." }, 400);
  const P = PACKS[pack - 1];
  const A = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; let ref = "";
  for (const x of crypto.getRandomValues(new Uint8Array(6))) ref += A[x % A.length];
  const r = await fetch(hook, {
    method: "POST", headers: { "content-type": "application/json" },
    body: JSON.stringify({
      allowed_mentions: { parse: [] },
      embeds: [{ title: "Nouvelle commande boutique", color: 0x34c38f, fields: [
        { name: "Joueur", value: pseudo, inline: true },
        { name: "Montant a verifier dans Wave", value: P.f + " FCFA", inline: true },
        { name: "Pack", value: pack + " : " + P.m.toLocaleString("fr-FR") + " $ | " + ((P.v || []).join(" + ") || "-") },
        { name: "Wave (numero / ID transaction)", value: "`" + wave + "`" },
        { name: "Reference", value: "`" + ref + "`", inline: true },
        { name: "Apres verification du paiement, en jeu :", value: "`/livrer " + pseudo + " " + pack + " " + ref + "`" },
      ], timestamp: new Date().toISOString() }],
    }),
  });
  return r.ok ? json({ ok: true, ref }) : json({ ok: false, error: "Discord a refuse l'envoi." }, 502);
};
export const config = { path: "/api/commande" };
