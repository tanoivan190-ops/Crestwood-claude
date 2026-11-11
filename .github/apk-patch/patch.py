import sys, pathlib
root = pathlib.Path(sys.argv[1])
f = root / 'smali_classes9/com/gta/launcher/ui/IntroOverlay.smali'
s = f.read_text(encoding='utf-8')
anchor = ("    .line 221\n    invoke-virtual {v9, v1, v2}, Landroid/widget/LinearLayout;->addView"
          "(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V\n")
assert s.count(anchor) == 1, 'ancre introuvable'
add = anchor + ("\n    # patch : cache le bandeau CRESTWOOD / ROLEPLAY . SAISON 1\n"
                "    const/16 v4, 0x8\n\n"
                "    invoke-virtual {v9, v4}, Landroid/view/View;->setVisibility(I)V\n")
f.write_text(s.replace(anchor, add), encoding='utf-8')
print('bandeau masque')

# ---- file de rendu 64 bits : 1 Mo -> 4 Mo (RenderQueue::RenderQueue, std::vector::resize(0x100000)) ----
# Plantage observe : "TryLoadModel 211 : charge" puis SIGSEGV dans memcpy depuis RenderQueue::QueueDataAligned
# (copie de ~1,1 Mo de sommets dans une file de 1 Mo).
def patch_bytes(path, off, expect, repl):
    b = bytearray(path.read_bytes())
    cur = bytes(b[off:off+len(expect)])
    if cur == repl:
        print('deja applique', path.name, hex(off)); return
    assert cur == expect, f'{path.name} {hex(off)} octets inattendus : {cur.hex()}'
    b[off:off+len(repl)] = repl
    path.write_bytes(bytes(b))
    print('patch', path.name, hex(off))

g64 = root / 'lib/arm64-v8a/libGame.so'
patch_bytes(g64, 0x78c608, bytes.fromhex('0102a052'), bytes.fromhex('0108a052'))  # movz w1,#0x10,lsl16 -> #0x40,lsl16

# ---- intro : 8 passages (photos + legendes demandees), bandeau deja masque plus haut ----
import re, shutil
s = f.read_text(encoding='utf-8')

CAPTIONS = [
    ("LOS SANTOS", "Bienvenue \\u00e0 Crestwood"),
    ("LIBERT\\u00c9", "Roule. Construis. R\\u00e8gne."),
    ("ICI, CHOISIS TON CAMP", "Deviens la loi, ou cr\\u00e9e ta loi."),
    ("DEVIENS LA LOI", "Policier, militaire et bien d'autres m\\u00e9tiers t'attendent."),
    ("LA RUE", "Ton histoire commence ici"),
    ("CHEF DE GANG", "Cr\\u00e9e ta loi, montre ta d\\u00e9termination, domine tous les territoires."),
    ("CRESTWOOD RP", "Le roleplay comme jamais"),
    ("CRESTWOOD RP", "Bien plus que le RP"),
]
# 0 = intro_slide_1, 1 = intro_slide_2 (ressources), 11..14 = photos prises dans assets/intro/slideN.jpg
SLIDES = ['s1', 's2', 11, 12, 's1', 13, 's2', 14]

lines = ['.method static constructor <clinit>()V', '    .locals 8', '',
         '    sget v0, Lcom/gta/game/R$drawable;->intro_slide_1:I',
         '    sget v1, Lcom/gta/game/R$drawable;->intro_slide_2:I']
regs = []
for i, sl in enumerate(SLIDES):
    r = f'v{2 + i % 6}'
    if sl == 's1':
        lines.append(f'    move v{2}, v0') if False else None
    regs.append(sl)
# construction simple : tableau d'entiers rempli element par element
lines += ['    const/16 v2, 0x8', '    new-array v2, v2, [I']
for i, sl in enumerate(SLIDES):
    lines.append(f'    const/16 v3, 0x{i:x}')
    if sl == 's1': lines.append('    aput v0, v2, v3')
    elif sl == 's2': lines.append('    aput v1, v2, v3')
    else:
        lines.append(f'    const/16 v4, 0x{sl:x}')
        lines.append('    aput v4, v2, v3')
lines.append('    sput-object v2, Lcom/gta/launcher/ui/IntroOverlay;->SLIDES:[I')
lines += ['    const/16 v2, 0x8', '    new-array v2, v2, [[Ljava/lang/String;']
for i, (a, b) in enumerate(CAPTIONS):
    lines += ['    const/4 v3, 0x2', '    new-array v3, v3, [Ljava/lang/String;',
              f'    const-string v4, "{a}"', '    const/4 v5, 0x0', '    aput-object v4, v3, v5',
              f'    const-string v4, "{b}"', '    const/4 v5, 0x1', '    aput-object v4, v3, v5',
              f'    const/16 v5, 0x{i:x}', '    aput-object v3, v2, v5']
lines += ['    sput-object v2, Lcom/gta/launcher/ui/IntroOverlay;->CAPTIONS:[[Ljava/lang/String;', '', '    return-void', '.end method']
new_clinit = '\n'.join(lines) + '\n'
m = re.search(r'\.method static constructor <clinit>\(\)V.*?\.end method\n', s, re.S)
assert m, 'clinit introuvable'
s = s[:m.start()] + new_clinit + s[m.end():]

def sub1(old, new):
    global s
    assert s.count(old) == 1, f'attendu 1 occurrence de {old!r}, trouve {s.count(old)}'
    s = s.replace(old, new)

sub1('    :goto_0\n    const/4 v1, 0x4\n\n    if-ge v0, v1, :cond_1', '    :goto_0\n    const/16 v1, 0x8\n\n    if-ge v0, v1, :cond_1')
sub1('const-wide/16 v2, 0x5dc0', 'const-wide/32 v2, 0xa5a0')       # progression totale
sub1('const-wide/16 v2, 0x47e0', 'const-wide/32 v2, 0x8fc0')       # debut de l'affiche finale (8 x 4600 ms)
sub1('const-wide/16 v2, 0x5848', 'const-wide/32 v2, 0xa028')       # debut du fondu de sortie
sub1('invoke-virtual {v0, p1}, Landroid/widget/ImageView;->setImageResource(I)V',
     'invoke-direct {p0, v0, p1}, Lcom/gta/launcher/ui/IntroOverlay;->setSlideImage(Landroid/widget/ImageView;I)V')

helper = '''
.method private setSlideImage(Landroid/widget/ImageView;I)V
    .locals 4

    const v0, 0x7f000000

    if-ge p2, v0, :cond_res

    :try_start_0
    new-instance v0, Ljava/lang/StringBuilder;

    invoke-direct {v0}, Ljava/lang/StringBuilder;-><init>()V

    const-string v1, "intro/slide"

    invoke-virtual {v0, v1}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    add-int/lit8 v1, p2, -0xa

    invoke-virtual {v0, v1}, Ljava/lang/StringBuilder;->append(I)Ljava/lang/StringBuilder;

    const-string v1, ".jpg"

    invoke-virtual {v0, v1}, Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;

    invoke-virtual {v0}, Ljava/lang/StringBuilder;->toString()Ljava/lang/String;

    move-result-object v0

    invoke-virtual {p1}, Landroid/widget/ImageView;->getContext()Landroid/content/Context;

    move-result-object v1

    invoke-virtual {v1}, Landroid/content/Context;->getAssets()Landroid/content/res/AssetManager;

    move-result-object v1

    invoke-virtual {v1, v0}, Landroid/content/res/AssetManager;->open(Ljava/lang/String;)Ljava/io/InputStream;

    move-result-object v1

    invoke-static {v1}, Landroid/graphics/BitmapFactory;->decodeStream(Ljava/io/InputStream;)Landroid/graphics/Bitmap;

    move-result-object v2

    invoke-virtual {v1}, Ljava/io/InputStream;->close()V

    invoke-virtual {p1, v2}, Landroid/widget/ImageView;->setImageBitmap(Landroid/graphics/Bitmap;)V
    :try_end_0
    .catch Ljava/lang/Exception; {:try_start_0 .. :try_end_0} :catch_0

    return-void

    :catch_0
    move-exception v0

    return-void

    :cond_res
    invoke-virtual {p1, p2}, Landroid/widget/ImageView;->setImageResource(I)V

    return-void
.end method
'''
i = s.index('.method private showSlide(IIZ)V')
s = s[:i] + helper.lstrip('\n') + '\n' + s[i:]
f.write_text(s, encoding='utf-8')

dst = root / 'assets/intro'
dst.mkdir(parents=True, exist_ok=True)
for n in range(1, 5):
    shutil.copyfile(pathlib.Path(__file__).parent / 'intro' / f'slide{n}.jpg', dst / f'slide{n}.jpg')
print('intro : 8 passages, photos et legendes ajoutees')
