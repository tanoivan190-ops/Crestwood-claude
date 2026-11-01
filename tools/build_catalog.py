#!/usr/bin/env python3
"""Genere data-clothes/ (textures renommees + clothes.dat) et gui/ClothesCatalog.h
a partir du pack Clothes.zip dezippe (variable CLOTHES_SRC). Source de verite : CATALOG.
Pour ajouter un vetement : ajoute une ligne (ou une variante) ici, relance le script."""
import os, shutil, colorsys, re
from PIL import Image

SRC = os.environ.get("CLOTHES_SRC", "/home/claude/work/clothes")
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "data-clothes")
HDR = os.path.join(ROOT, "app/src/main/cpp/samp/gui/ClothesCatalog.h")
SKIN_ID = 289
T = "Texturas/Male/"
TV = os.environ.get("TV_SRC", "/home/claude/work/skinzip/skin/tv/male")  # texturevetements.zip dezippe

def num_sorted(folder):
    return sorted((f for f in os.listdir(folder) if f.endswith(".png")), key=lambda f: int(os.path.splitext(f)[0]))

def color_name(path):
    """Nom francais automatique : couleurs dominantes hors peau (a renommer a la main si besoin)."""
    im = Image.open(path).convert("RGBA").resize((48, 48))
    cnt = {}
    for r, g, b, a in im.getdata():
        if a < 200: continue
        h, sa, v = colorsys.rgb_to_hsv(r/255, g/255, b/255)
        hd = h * 360
        if 8 <= hd <= 40 and 0.15 <= sa <= 0.75 and 0.35 <= v <= 0.98: continue  # peau
        if v < 0.2: c = "noir"
        elif sa < 0.13: c = "blanc" if v > 0.82 else "gris"
        elif hd < 12 or hd >= 345: c = "rouge" if sa > 0.4 else "rose"
        elif hd < 40: c = "orange" if v > 0.5 else "marron"
        elif hd < 70: c = "jaune"
        elif hd < 165: c = "vert"
        elif hd < 200: c = "turquoise"
        elif hd < 255: c = "bleu"
        elif hd < 300: c = "violet"
        else: c = "rose"
        cnt[c] = cnt.get(c, 0) + 1
    if not cnt: return "Neutre"
    top = sorted(cnt.items(), key=lambda x: -x[1])
    name = top[0][0].capitalize()
    if len(top) > 1 and top[1][1] >= 0.35 * top[0][1]: name += " et " + top[1][0]
    return name

def variants_from_tv(folder):
    d = os.path.join(TV, folder)
    return [("TV:" + folder, int(os.path.splitext(f)[0]), "%s %d" % (color_name(os.path.join(d, f)), int(os.path.splitext(f)[0])))
            for f in num_sorted(d)]

def src_png(folder, n):
    if folder.startswith("TV:"): return os.path.join(TV, folder[3:], "%d.png" % n)
    return os.path.join(SRC, T, folder, "%d.png" % n)

# groupe, cle, nom du style, [materiaux du dff], [(dossier source, numero png, nom de la variante), ...]
CATALOG = [
 ("face","face","Visage",["cara3"],[("Cara/1",1,"Visage 1"),("Cara/1",2,"Visage tatoué (cou)"),("Cara/1",3,"Visage barbu")]),
 ("hair","hair1","Cheveux courts",["hair_diff_002_d_uni"],[("Cabello/1",1,"Châtain clair"),("Cabello/1",2,"Gris"),("Cabello/1",3,"Rouge"),("Cabello/1",4,"Vert")]),
 ("hair","hair2","Cheveux mi-longs",["hair_diff_003_a_uni"],[("Cabello/2",1,"Brun"),("Cabello/2",2,"Noir"),("Cabello/2",3,"Rouge"),("Cabello/2",4,"Bleu"),("Cabello/8",1,"Châtain")]),
 ("hair","hair3","Casquette",["LACOSTE15"],[("Cabello/3",1,"Noire"),("Cabello/3",2,"Noire 2")]),
 ("hair","hair4","Cheveux en pointes",["hair_diff_000_e_uni"],[("Cabello/4",1,"Noir"),("Cabello/5",1,"Châtain")]),
 ("beard","beard1","Barbe",["accs_diff_003_a_uni"],[("Barba/1",1,"Barbe noire")]),
 ("top","top1","T-shirt imprimé",["jbib_diff_007_x_uni"],[("Torso/1",1,"Fleurs rouges"),("Torso/1",2,"Noir logo rouge"),("Torso/1",3,"Tropical"),("Torso/1",4,"Noir groupe rock")]),
 ("top","top2","Haut",["jbib_diff_004_a_uni"],[("Torso/2",1,"Gilet sombre"),("Torso/2",2,"Polo blanc"),("Torso/2",3,"Tie-dye"),("Torso/2",4,"Gris")]),
 ("bottom","bottom1","Short",["perna"],[("Piernas/1",1,"Rose")]),
 ("bottom","bottom2","Pantalon",["lowr_diff_004_a_whi"],[("Piernas/2",1,"Noir"),("Piernas/2",2,"Jean bleu"),("Piernas/2",3,"Cargo gris")]),
 ("shoes","shoes1","Claquettes",["pesandalia","san"],[("Calzado/1",1,"Bleues")]),
 ("shoes","shoes2","Baskets",["NIKE14"],[("Calzado/2",1,"Roses"),("Calzado/2",2,"Vert et gris"),("Calzado/2",3,"Rouges"),("Calzado/2",4,"Rouge et noir"),("Calzado/2",5,"Blanc et rouge"),("Calzado/2",6,"Bleues"),("Calzado/2",7,"Noir et rouge"),("Calzado/2",8,"Gris et orange"),("Calzado/2",9,"Noir et blanc")]),
 ("bottom","bottom3","Short d'été",["perna"],variants_from_tv("bermuda.padrao")),
 ("bottom","bottom4","Short camouflage",["perna"],variants_from_tv("bermuda.camo")),
 ("bottom","bottom5","Caleçon",["perna"],variants_from_tv("body.cueca")),
 ("bottom","bottom6","Jean",["lowr_diff_004_a_whi"],variants_from_tv("calca.jeans")),
 ("top","top3","Sweat",["jbib_diff_004_a_uni"],variants_from_tv("casaco.moletom")),
 ("watch","watch1","Montre",["p_lwrist_diff_000_a"],[("Reloj/1",1,"Montre noire")]),
]
LIBRARY = [  # (cle, categorie, nom, dossier texturevetements/male) : non branche tant qu'on ne connait pas le materiau du modele
 ("hair_afro","Cheveux","Afro / boucles","Cabelo1"),("hair_longs","Cheveux","Cheveux longs","Cabelo2"),
 ("hair_rase","Cheveux","Cheveux rasés","Cabelo3"),("hair_pointes","Cheveux","Cheveux en pointes","Cabelo4"),
 ("pants_large","Bas","Pantalon large","calca.larga"),("terno_pants","Bas","Pantalon de costume","terno.calca"),
 ("top_std","Hauts","T-shirt","camisa.padrao"),("top_media","Hauts","Chemise mi-longue","camisa.media"),
 ("top_polo","Hauts","Polo","camisa.polo"),("top_beach","Hauts","Chemise de plage","camisa.praia"),
 ("top_tank","Hauts","Débardeur","camisa.regata"),("terno_top","Hauts","Veste de costume","terno.blusa"),
 ("terno_mid","Hauts","Chemise de costume","terno.meio"),
 ("shoe_flip","Chaussures","Tongs","pe.chinelo"),("shoe_sandal","Chaussures","Sandales","pe.sandalia"),
 ("shoe_dress","Chaussures","Chaussures de ville","pe.sapatosocial"),("shoe_sneak2","Chaussures","Baskets 2","pe.tenis2"),
 ("shoe_sneak3","Chaussures","Baskets 3","pe.tenis3"),
 ("hat_cap","Chapeaux","Casquette","bone.padrao"),("hat_bucket","Chapeaux","Bob","bucket.hat"),("hat_cowboy","Chapeaux","Chapeau de cowboy","chapeu.vaqueiro"),
 ("glasses_juliet","Lunettes","Lunettes Juliet","oculos.juliet"),("glasses_std","Lunettes","Lunettes classiques","oculos.padrao"),("glasses_juliette2","Lunettes","Lunettes Juliette","glasse.juliette"),
 ("mask_covid","Masques","Masque sanitaire","corona.mask"),("mask_skull","Masques","Masque tête de mort","skull.mask"),
 ("parachute","Accessoires","Parachute","fivem.parachute"),
 ("face_beard","Visage","Barbe (tracé)","facetextures/beard"),("face_eyebrow","Visage","Sourcils","facetextures/eyebrow"),
 ("face_eyes","Visage","Yeux","facetextures/eyes"),("face_blush","Visage","Blush","facetextures/blush"),
]
SPARE = ["Cabello/6","Cabello/7"]  # coiffures afro/boucles : pas de modele correspondant
BODY = ["uppr_diff_015_a_whi", "lowr_diff_000_a_whi"]

def tex(key, n): return "brl_cw_clothes_%s_%02d" % (key, n)
def q(s): return '"%s"' % s.replace('"', '\\"')

shutil.rmtree(OUT, ignore_errors=True)
for d in ("textures", "models", "non_assignes"): os.makedirs(os.path.join(OUT, d))
for f in sorted(os.listdir(SRC + "/Modelos")):
    shutil.copy(os.path.join(SRC, "Modelos", f), os.path.join(OUT, "models", f.replace("skin.", "cwskin.") if f.startswith("skin.") else f))

dat = ["# groupe;cle;nom du style;materiaux(+);variantes;prefixe texture;noms des variantes (|)"]
for g, key, name, mats, vs in CATALOG:
    for i, (folder, n, _) in enumerate(vs, 1):
        shutil.copy(src_png(folder, n), os.path.join(OUT, "textures", tex(key, i) + ".png"))
    dat.append("%s;%s;%s;%s;%d;brl_cw_clothes_%s;%s" % (g, key, name, "+".join(mats), len(vs), key, "|".join(v[2] for v in vs)))
os.makedirs(os.path.join(OUT, "bibliotheque"))
lib = ["# cle;categorie;nom;nb variantes;prefixe texture;noms des variantes (|) - NON BRANCHE : il manque le materiau du modele"]
for key, cat, name, folder in LIBRARY:
    d = os.path.join(TV, folder)
    names = []
    for i, f in enumerate(num_sorted(d), 1):
        shutil.copy(os.path.join(d, f), os.path.join(OUT, "bibliotheque", tex(key, i) + ".png"))
        names.append("%s %d" % (color_name(os.path.join(d, f)), i))
    lib.append("%s;%s;%s;%d;brl_cw_clothes_%s;%s" % (key, cat, name, len(names), key, "|".join(names)))
open(OUT + "/bibliotheque.dat", "w", encoding="utf-8").write("\n".join(lib) + "\n")
for folder in SPARE:
    shutil.copy(os.path.join(SRC, T, folder, "1.png"), os.path.join(OUT, "non_assignes", folder.replace("/", "_") + ".png"))
open(OUT + "/clothes.dat", "w", encoding="utf-8").write("\n".join(dat) + "\n")

def grp(g): return [c for c in CATALOG if c[0] == g]
def names_arrays(g):
    return "\n".join("    constexpr const char* k%sNames[] = { %s };" % (c[1].capitalize(), ", ".join(q(v[2]) for v in c[4])) for c in grp(g))
def style_arr(g):
    rows = []
    for _, key, name, mats, vs in grp(g):
        m1 = q(mats[1]) if len(mats) > 1 else "nullptr"
        rows.append("        { %s, %s, { %s, %s }, %d, %d, k%sNames }," % (q(name), q(key), q(mats[0]), m1, len(mats), len(vs), key.capitalize()))
    return "\n".join(rows)
parts = ["beard", "hair", "top", "bottom", "shoes", "watch"]
face = grp("face")[0]
body = "\n".join(names_arrays(g) for g in ["face"] + parts)
tabs = "\n".join("    constexpr Style k%s[] = {\n%s\n    };" % (g.capitalize(), style_arr(g)) for g in parts)
h = f'''#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

// ============================================================
// ClothesCatalog - vetements du pack Clothes.zip (skin personnalisable Crestwood).
// FICHIER GENERE par tools/build_catalog.py : modifie CATALOG dans le script, relance-le.
//
// Le jeu cherche chaque texture PAR SON NOM dans la texdb :
//     brl_cw_clothes_<cle>_<NN>        ex. brl_cw_clothes_top1_02
// Le materiau cible est celui de cwskin.dff. Texture introuvable => piece masquee.
// Chaque style ET chaque variante a un nom affiche en jeu.
// ============================================================

namespace ClothesCatalog
{{
    constexpr uint16_t kCustomMaleSkinId = {SKIN_ID};

    struct Style
    {{
        const char*        name;          // nom du style
        const char*        key;           // brl_cw_clothes_<key>_<NN>
        const char*        materials[2];  // materiaux du dff recevant la texture
        int                materialCount;
        int                variants;      // nombre de textures / couleurs
        const char* const* variantNames;  // nom de chaque variante (variants entrees)
    }};

    constexpr const char* kBodyMaterials[] = {{ "{BODY[0]}", "{BODY[1]}" }};
    constexpr const char* kFaceMaterial = "{face[3][0]}";
    constexpr const char* kFaceKey      = "{face[1]}";
    constexpr int         kFaceCount    = {len(face[4])};

{body}

{tabs}

    template <size_t N> constexpr int Count(const Style (&)[N]) {{ return static_cast<int>(N); }}

    inline void TextureName(char* out, size_t size, const char* key, int value)
    {{
        std::snprintf(out, size, "brl_cw_clothes_%s_%02d", key, value);
    }}
}}
'''
open(HDR, "w", encoding="utf-8").write(h)
print("textures:", len(os.listdir(OUT + "/textures")))
