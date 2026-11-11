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
