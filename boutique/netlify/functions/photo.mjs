// Photo reelle d'un modele (image principale de son article Wikipedia), chargee cote serveur.
const UA = { "user-agent": "CrestwoodBoutique/1.0 (https://crestwood.example; contact via Discord)" };
export default async (req) => {
  const t = new URL(req.url).searchParams.get("t") || "";
  if (!/^[\w.()'-]{2,80}$/.test(t)) return new Response("bad", { status: 400 });
  try {
    const api = "https://en.wikipedia.org/w/api.php?action=query&redirects=1&prop=pageimages&piprop=thumbnail&pithumbsize=640&format=json&titles=" + encodeURIComponent(t);
    const j = await (await fetch(api, { headers: UA })).json();
    let src = "";
    for (const k in (j.query && j.query.pages) || {}) { const th = j.query.pages[k].thumbnail; if (th && th.source) { src = th.source; break; } }
    if (!src) return new Response("none", { status: 404 });
    const img = await fetch(src, { headers: UA });
    if (!img.ok) return new Response("none", { status: 404 });
    return new Response(await img.arrayBuffer(), { headers: { "content-type": img.headers.get("content-type") || "image/jpeg", "cache-control": "public, max-age=604800" } });
  } catch { return new Response("err", { status: 502 }); }
};
export const config = { path: "/api/photo" };
