"""Boutons natifs Crestwood v35 : cercle de verre + icone blanche (style de la capture d'Anzo)."""
from PIL import Image, ImageDraw, ImageFilter, ImageChops
import math, os, sys

OUT = sys.argv[1]
S = 1024          # dessin en 4x puis reduction (bords lisses)
F = 256


def glass():
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    c, r = S / 2, S * 0.47
    # ombre douce sous le bouton
    sh = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    ImageDraw.Draw(sh).ellipse((c - r, c - r + 14, c + r, c + r + 14), fill=(0, 0, 0, 70))
    im.alpha_composite(sh.filter(ImageFilter.GaussianBlur(18)))
    # corps de verre : degrade radial (plus clair en haut)
    body = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    px = body.load()
    for y in range(int(c - r), int(c + r) + 1):
        for x in range(int(c - r), int(c + r) + 1):
            d = math.hypot(x - c, y - c) / r
            if d > 1: continue
            t = (y - (c - r)) / (2 * r)                 # 0 haut -> 1 bas
            a = 58 + 40 * (1 - t) + 30 * d ** 6         # bord legerement plus lumineux
            g = int(205 + 40 * (1 - t))
            px[x, y] = (g, g, min(255, g + 6), int(a))
    im.alpha_composite(body)
    d = ImageDraw.Draw(im)
    # liseret clair + fin trait sombre exterieur (verre)
    d.ellipse((c - r, c - r, c + r, c + r), outline=(0, 0, 0, 60), width=10)
    d.ellipse((c - r + 8, c - r + 8, c + r - 8, c + r - 8), outline=(255, 255, 255, 150), width=12)
    # reflet en haut
    hl = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    ImageDraw.Draw(hl).ellipse((c - r * 0.72, c - r * 0.92, c + r * 0.72, c - r * 0.05), fill=(255, 255, 255, 46))
    im.alpha_composite(hl.filter(ImageFilter.GaussianBlur(26)))
    return im


def icon_layer():
    return Image.new("L", (S, S), 0)


def finish(name, mask, color=(255, 255, 255), glow=None):
    base = glass()
    # ombre de l'icone (relief)
    sh = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    sh.putalpha(mask.point(lambda v: v * 0.45))
    sh = ImageChops.offset(sh, 0, 10).filter(ImageFilter.GaussianBlur(10))
    base.alpha_composite(sh)
    if glow:
        g = Image.new("RGBA", (S, S), glow + (0,))
        g.putalpha(mask.filter(ImageFilter.GaussianBlur(28)))
        base.alpha_composite(g)
    ic = Image.new("RGBA", (S, S), color + (0,))
    ic.putalpha(mask)
    base.alpha_composite(ic)
    base.resize((F, F), Image.LANCZOS).save(os.path.join(OUT, name + ".png"))


def P(*pts):
    return [(x * S, y * S) for x, y in pts]


def rrect(d, box, rad, fill=255):
    d.rounded_rectangle([v * S for v in box], radius=rad * S, fill=fill)


def steer(left):
    m = icon_layer(); d = ImageDraw.Draw(m)
    d.polygon(P((0.20, 0.50), (0.46, 0.27), (0.46, 0.73)), fill=255)
    rrect(d, (0.44, 0.40, 0.56, 0.60), 0.02)
    for x0 in (0.595, 0.665, 0.735):
        rrect(d, (x0, 0.37, x0 + 0.045, 0.63), 0.015)
    if not left: m = m.transpose(Image.FLIP_LEFT_RIGHT)
    return m


def pedal(wide):
    m = icon_layer(); d = ImageDraw.Draw(m)
    if wide:
        box = (0.24, 0.33, 0.76, 0.67); cols, rows = 5, 3
    else:
        box = (0.36, 0.22, 0.64, 0.78); cols, rows = 2, 5
    rrect(d, box, 0.06)
    x0, y0, x1, y1 = box
    rr = 0.026
    for j in range(rows):
        for i in range(cols):
            cx = x0 + (x1 - x0) * (i + 0.5) / cols
            cy = y0 + (y1 - y0) * (j + 0.5) / rows
            d.ellipse(((cx - rr) * S, (cy - rr) * S, (cx + rr) * S, (cy + rr) * S), fill=0)
    return m


def handbrake():
    m = icon_layer(); d = ImageDraw.Draw(m)
    c = 0.5 * S
    R, r2 = 0.27 * S, 0.11 * S
    d.ellipse((c - R, c - R, c + R, c + R), fill=255)
    d.ellipse((c - r2, c - r2, c + r2, c + r2), fill=0)
    d.ellipse((c - 0.05 * S, c - 0.05 * S, c + 0.05 * S, c + 0.05 * S), fill=255)
    for k in range(8):   # trous de ventilation du disque
        a = k * math.pi / 4
        x, y = c + math.cos(a) * 0.19 * S, c + math.sin(a) * 0.19 * S
        d.ellipse((x - 0.022 * S, y - 0.022 * S, x + 0.022 * S, y + 0.022 * S), fill=0)
    # etrier
    d.pieslice((c - 0.33 * S, c - 0.33 * S, c + 0.33 * S, c + 0.33 * S), -60, 10, fill=255)
    d.pieslice((c - 0.285 * S, c - 0.285 * S, c + 0.285 * S, c + 0.285 * S), -62, 12, fill=0)
    d.pieslice((c - 0.33 * S, c - 0.33 * S, c + 0.33 * S, c + 0.33 * S), -55, 5, fill=255)
    d.pieslice((c - 0.29 * S, c - 0.29 * S, c + 0.29 * S, c + 0.29 * S), -57, 7, fill=0)
    d.ellipse((c - R, c - R, c + R, c + R), outline=255, width=int(0.035 * S))
    return m


def horn():
    m = icon_layer(); d = ImageDraw.Draw(m)
    d.polygon(P((0.30, 0.50), (0.66, 0.30), (0.70, 0.31), (0.70, 0.75), (0.66, 0.76), (0.30, 0.58)), fill=255)
    d.ellipse(P((0.62, 0.28), (0.76, 0.78)), fill=255)
    d.ellipse(P((0.22, 0.47), (0.33, 0.61)), fill=255)
    rrect(d, (0.36, 0.56, 0.44, 0.70), 0.02)
    # ondes sonores
    for k, rad in enumerate((0.16, 0.24)):
        d.arc(((0.70 - rad) * S, (0.53 - rad) * S, (0.70 + rad) * S, (0.53 + rad) * S), -40, 40, fill=255, width=int(0.03 * S))
    return m


def nitro():
    m = icon_layer(); d = ImageDraw.Draw(m)
    rrect(d, (0.38, 0.32, 0.62, 0.80), 0.08)
    rrect(d, (0.46, 0.22, 0.54, 0.34), 0.015)
    rrect(d, (0.42, 0.18, 0.58, 0.23), 0.015)
    rrect(d, (0.38, 0.50, 0.62, 0.56), 0.0, fill=0)
    return m


def door():
    m = icon_layer(); d = ImageDraw.Draw(m)
    d.polygon(P((0.22, 0.52), (0.40, 0.27), (0.78, 0.27), (0.78, 0.74), (0.28, 0.74), (0.22, 0.66)), fill=255)
    d.polygon(P((0.33, 0.49), (0.45, 0.33), (0.72, 0.33), (0.72, 0.49)), fill=0)
    rrect(d, (0.58, 0.55, 0.70, 0.59), 0.015, fill=0)
    return m


def car_in_glass(src):
    im = Image.open(src).convert("RGBA").resize((int(S * 0.5), int(S * 0.5)), Image.LANCZOS)
    m = icon_layer()
    m.paste(im.getchannel("A"), (int(S * 0.25), int(S * 0.25)))
    return m


def camera():
    m = icon_layer(); d = ImageDraw.Draw(m)
    rrect(d, (0.27, 0.36, 0.73, 0.68), 0.06)
    rrect(d, (0.40, 0.30, 0.56, 0.38), 0.02)
    d.ellipse(P((0.40, 0.40), (0.60, 0.64)), fill=0)
    d.ellipse(P((0.445, 0.455), (0.555, 0.585)), fill=255)
    # fleches de rotation
    w = int(0.03 * S)
    d.arc((0.18 * S, 0.18 * S, 0.82 * S, 0.82 * S), 200, 250, fill=255, width=w)
    d.arc((0.18 * S, 0.18 * S, 0.82 * S, 0.82 * S), 20, 70, fill=255, width=w)
    d.polygon(P((0.68, 0.79), (0.75, 0.68), (0.80, 0.78)), fill=255)
    d.polygon(P((0.32, 0.21), (0.25, 0.32), (0.20, 0.22)), fill=255)
    return m


def engine():
    m = icon_layer(); d = ImageDraw.Draw(m)
    w = int(0.055 * S)
    d.arc((0.29 * S, 0.29 * S, 0.71 * S, 0.71 * S), -60, 240, fill=255, width=w)
    rrect(d, (0.4725, 0.22, 0.5275, 0.50), 0.027)
    # mention START sous le symbole
    return m


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    finish("v_left", steer(True))
    finish("v_right", steer(False))
    finish("v_accel", pedal(False))
    finish("v_brake", pedal(True))
    finish("v_handbrake", handbrake())
    finish("v_horn", horn())
    finish("v_nitro", nitro())
    finish("v_door", door())
    finish("v_car", car_in_glass(sys.argv[2]))
    finish("v_camera", camera())
    finish("engine_off", engine())
    finish("engine_on", engine(), color=(120, 255, 160), glow=(40, 220, 110))
