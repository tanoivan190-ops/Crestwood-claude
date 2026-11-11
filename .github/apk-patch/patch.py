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
