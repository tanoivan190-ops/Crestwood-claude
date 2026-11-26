#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CRESTWOOD v47 - MTA -> APK : convertit les modeles 3D des ressources MTA (engineReplaceModel) pour l'appli.

  python3 tools/mta/mta_vers_apk.py <dossier MTA> [<autre dossier> ...] --out app/src/main/assets/mta

Pour chaque modele remplace par un script MTA NON chiffre (engineLoadTXD / engineLoadDFF / engineLoadCOL
+ engineImportTXD / engineReplaceModel / engineReplaceCOL), on ecrit :
  assets/mta/<id>/model.dff   (copie)
  assets/mta/<id>/model.col   (copie, COL2/COL3/COL4)
  assets/mta/<id>/model.txd   (TXD lisible par le jeu mobile : textures DXT decompressees en 32 bits,
                               SEULEMENT celles utilisees par le DFF, reduites pour la memoire du telephone)
  assets/mta/models.txt       id;distance;nom;x,y;x,y;...   (positions trouvees dans les .map : le jeu ne
                               charge un modele que quand le joueur s'en approche)
Les ressources chiffrees (pcrypt, .luac) sont ignorees (on ne dechiffre rien).
Besoin : Python 3 + Pillow.
"""
import argparse, glob, os, re, shutil, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cwtxd  # noqa: E402

# boucles / tables que l'analyse simple ne comprend pas : (id, dossier relatif, dff, txd, col, distance)
MANUEL = [
    (17563, "gs-wattspark", "1.dff", "1.txd", "1.col", 300),
    (17625, "gs-wattspark", "2.dff", "1.txd", "2.col", 300),
]

def ci_path(p):
    """chemin insensible a la casse (MTA est sous Windows)"""
    if os.path.exists(p): return p
    parts = os.path.normpath(p).split(os.sep)
    cur = os.sep if p.startswith(os.sep) else ""
    for part in parts:
        if not part: continue
        nxt = os.path.join(cur, part) if cur else part
        if not os.path.exists(nxt):
            d = cur or "."
            try: found = [x for x in os.listdir(d) if x.lower() == part.lower()]
            except OSError: return None
            if not found: return None
            nxt = os.path.join(cur, found[0]) if cur else found[0]
        cur = nxt
    return cur

def scan_lua(path):
    raw = open(path, "rb").read()
    if raw[:4] == b"\x1bLua" or b"engineReplaceModel" not in raw: return {}
    t = raw.decode("utf8", "ignore")
    d = os.path.dirname(path)
    consts = {m.group(1): int(m.group(2)) for m in re.finditer(r"(?:local\s+)?(\w+)\s*=\s*(\d{3,5})\b", t)}
    def num(x):
        x = x.strip()
        return int(x) if x.isdigit() else consts.get(x)
    out = {}
    var = {}
    pat = re.compile(r"(\w+)\s*=\s*engineLoad(TXD|DFF|COL)\s*\(\s*[\"']([^\"']+)[\"']"
                     r"|engineImportTXD\s*\(\s*(\w+)\s*,\s*(\w+)"
                     r"|engineReplaceModel\s*\(\s*(\w+)\s*,\s*(\w+)"
                     r"|engineReplaceCOL\s*\(\s*(\w+)\s*,\s*(\w+)"
                     r"|engineSetModelLODDistance\s*\(\s*(\w+)\s*,\s*(\d+)")
    for m in pat.finditer(t):
        if m.group(1): var[m.group(1)] = os.path.join(d, m.group(3)); continue
        for gv, gi, kind in ((4, 5, "txd"), (6, 7, "dff"), (8, 9, "col")):
            if m.group(gv):
                i = num(m.group(gi)); f = var.get(m.group(gv))
                if i and f: out.setdefault(i, {})[kind] = f
        if m.group(10):
            i = num(m.group(10))
            if i: out.setdefault(i, {})["lod"] = int(m.group(11))
    # tables { model = N, txd = "..", dff = "..", col = "..", lodDistance = N }
    for m in re.finditer(r"\{([^{}]*model\s*=\s*\d+[^{}]*)\}", t):
        b = m.group(1)
        g = lambda k: (re.search(k + r"\s*=\s*[\"']([^\"']+)[\"']", b) or [None, None])[1]
        i = int(re.search(r"model\s*=\s*(\d+)", b).group(1))
        e = out.setdefault(i, {})
        for k in ("txd", "dff", "col"):
            v = g(k)
            if v: e[k] = os.path.join(d, v)
        ld = re.search(r"lodDistance\s*=\s*(\d+)", b)
        if ld: e["lod"] = int(ld.group(1))
    return {i: e for i, e in out.items() if "dff" in e}

def scan_maps(roots):
    pos = {}
    for r in roots:
        for p in glob.glob(glob.escape(r) + "/**/*.map", recursive=True):
            t = open(p, "rb").read().decode("utf8", "ignore")
            for m in re.finditer(r"<(?:object|removeWorldObject)\b([^>]*)>", t):
                a = m.group(1)
                mm = re.search(r'\bmodel="(\d+)"', a); x = re.search(r'posX="(-?[\d.]+)"', a); y = re.search(r'posY="(-?[\d.]+)"', a)
                if mm and x and y:
                    pos.setdefault(int(mm.group(1)), []).append((float(x.group(1)), float(y.group(1))))
    return pos

def cluster(pts, r=150.0):
    out = []
    for x, y in pts:
        if all((x - a) ** 2 + (y - b) ** 2 > r * r for a, b in out): out.append((x, y))
    return out[:12]

def convert_txd(src, dff, dst, budget_cap):
    names = cwtxd.dff_textures(dff)
    ts, ver = cwtxd.read_txd(src)
    keep = [x for x in ts if "err" not in x and x["name"].lower() in names]
    cap = 256 if len(keep) <= 8 else (192 if len(keep) <= 24 else budget_cap)
    out, total = [], 0
    for x in keep:
        try: im = cwtxd.decode(x)
        except Exception as e:
            print("   texture ignoree", x["name"], e); continue
        w, h = im.size
        s = min(1.0, cap / float(max(w, h)))
        if s < 1.0:
            from PIL import Image
            im = im.resize((max(1, int(w * s)), max(1, int(h * s))), Image.LANCZOS)
        alpha = im.getchannel("A").getextrema()[0] < 250
        out.append({"img": im, "name": x["name"], "mask": x["mask"], "flt": x["flt"], "alpha": alpha})
        total += im.size[0] * im.size[1] * 4
    cwtxd.write_txd(dst, out)
    return len(out), len(ts), total

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src", nargs="+")
    ap.add_argument("--out", required=True)
    ap.add_argument("--cap", type=int, default=128, help="taille max des textures des gros modeles (defaut 128)")
    ap.add_argument("--skip", default="", help="ids a ignorer, ex. 1492,941")
    a = ap.parse_args()
    skip = {int(x) for x in a.skip.split(",") if x.strip()}
    models, origin = {}, {}
    for r in a.src:
        for lua in sorted(glob.glob(glob.escape(r) + "/**/*.lua", recursive=True)):
            for i, e in scan_lua(lua).items():
                if i in models or i in skip: continue
                models[i] = e; origin[i] = os.path.basename(os.path.dirname(lua))
        for (i, folder, dff, txd, col, lod) in MANUEL:
            hit = [p for p in glob.glob(glob.escape(r) + "/**/" + folder, recursive=True) if os.path.isdir(p)]
            if hit and i not in models and i not in skip:
                models[i] = {"dff": os.path.join(hit[0], dff), "txd": os.path.join(hit[0], txd), "col": os.path.join(hit[0], col), "lod": lod}
                origin[i] = folder
    pos = scan_maps(a.src)
    if os.path.isdir(a.out): shutil.rmtree(a.out)
    os.makedirs(a.out)
    lines, grand = [], 0
    for i in sorted(models):
        e = models[i]
        dff = ci_path(e["dff"]); txd = ci_path(e.get("txd", "")) if e.get("txd") else None
        col = ci_path(e.get("col", "")) if e.get("col") else None
        if not dff: print("X", i, "dff introuvable", e["dff"]); continue
        head = open(dff, "rb").read(4)
        if head != b"\x10\x00\x00\x00": print("X", i, "dff chiffre / illisible :", dff); continue
        d = os.path.join(a.out, str(i)); os.makedirs(d)
        shutil.copyfile(dff, os.path.join(d, "model.dff"))
        try:
            with open(os.path.join(d, "materials.txt"), "w", encoding="latin1") as mf:
                mf.write("\n".join(cwtxd.dff_material_names(dff)) + "\n")
        except Exception as ex:
            print("   materials.txt impossible :", ex)
        info = ""
        if txd:
            try:
                n, tot, b = convert_txd(txd, dff, os.path.join(d, "model.txd"), a.cap)
                grand += b; info = "textures %d/%d (%.1f Mo)" % (n, tot, b / 1e6)
            except Exception as ex:
                info = "TXD ignore (%s)" % ex
        if col and open(col, "rb").read(4) in (b"COL2", b"COL3", b"COL4"):
            shutil.copyfile(col, os.path.join(d, "model.col"))
        dist = int(e.get("lod", 0)) if e.get("lod", 0) and e.get("lod", 0) < 100000 else 0
        pts = cluster(pos.get(i, []))
        name = re.sub(r"[^A-Za-z0-9_-]", "", origin.get(i, ""))[:24]
        lines.append("%d;%d;%s;%s" % (i, dist, name, ";".join("%d,%d" % (x, y) for x, y in pts)))
        print("OK", i, origin.get(i), info, "positions:", len(pts))
    with open(os.path.join(a.out, "models.txt"), "w") as f:
        f.write("# id;distance;nom;x,y;x,y...  (genere par tools/mta/mta_vers_apk.py) - une ligne par modele\n")
        f.write("# sans position : le jeu cherche les batiments / objets qui l utilisent. Supprimer une ligne = modele d origine.\n")
        f.write("\n".join(lines) + "\n")
    print("TOTAL textures en memoire si TOUT est charge : %.1f Mo" % (grand / 1e6))

if __name__ == "__main__":
    main()
