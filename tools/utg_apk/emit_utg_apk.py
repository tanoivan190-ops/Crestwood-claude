import json,re
d=json.load(open('/tmp/claude-0/utg/out.json'))
W='/home/claude/work/app/src/main/'
def cs(s): return '"'+s.replace('\\','\\\\').replace('"','\\"')+'"'
# UtgBrlMap.h
names=sorted(d['brl'])
h=['#pragma once','// GENERE par tools/build_utg_apk.py : textures du pack Utgawa embarquees dans l APK.',
   '// nom de texture "brl_*" (catalogue Personnage) -> fichier assets/clothes/<chemin> (PNG/JPEG).',
   '// Les pieces identiques au pack Lasouth reutilisent ses fichiers (pas de doublon dans l APK).',
   '#include <cstring>','#include <algorithm>','namespace UtgBrlMap {',
   'struct Entry { const char* name; const char* path; };',f'inline constexpr int kCount = {len(names)};',
   'inline const Entry kEntries[kCount] = {']
for n in names: h.append(f'    {{{cs(n)},{cs("south/"+d["brl"][n])}}},')
h+=['};','// recherche (insensible a la casse, tableau trie)','inline const char* Find(const char* name) {',
    '    if (!name) return nullptr;','    int lo = 0, hi = kCount - 1;','    while (lo <= hi) {','        const int mid = (lo + hi) / 2;',
    '        const int c = strcasecmp(kEntries[mid].name, name);','        if (c == 0) return kEntries[mid].path;','        if (c < 0) lo = mid + 1; else hi = mid - 1;','    }','    return nullptr;','}','}','']
# check sorting is case-insensitive-consistent
assert names==sorted(names,key=str.lower) or True
names_ci=sorted(names,key=lambda s:s.lower())
h2=[l for l in h]
txt='\n'.join(h).replace('#include <cstring>','#include <cstring>\n#include <strings.h>')
# rebuild entries in case-insensitive order
ent='\n'.join(f'    {{{cs(n)},{cs("south/"+d["brl"][n])}}},' for n in names_ci)
txt=re.sub(r'inline const Entry kEntries\[kCount\] = \{\n.*?\n\};', 'inline const Entry kEntries[kCount] = {\n'+ent+'\n};', txt, flags=re.S)
for a in ['samp','samp32']:
    open(W+f'cpp/{a}/gui/UtgBrlMap.h','w').write(txt)
# SouthClothes.h append groups
for a in ['samp','samp32']:
    p=W+f'cpp/{a}/gui/SouthClothes.h'; s=open(p,encoding='utf-8').read()
    import re as _re
    if '// --- pack Utgawa' in s:
        a0=s.index('// --- pack Utgawa'); a1=s.index('inline const Group groups[] = {')
        s=s[:a0]+s[a1:]
        s='\n'.join(l for l in s.split('\n') if '"utg' not in l or 'Group' in l)
    n0=len(re.findall(r'inline const Item items\d+\[\]',s))
    add=[]; gl=[]
    for i,g in enumerate(d['groups']):
        k=n0+i
        add.append(f'inline const Item items{k}[] = {{')
        for it in g['items']:
            add.append('    {'+','.join(cs(it[x]) for x in ('name','material','texture','apply','remove'))+'},')
        add.append('};')
        gl.append(f'    {{{g["model"]},{cs(g["key"])},{cs(g["label"])},{cs(g["tab"])},items{k},{len(g["items"])}}},')
    i=s.index('inline const Group groups[] = {')
    s=s[:i]+'// --- pack Utgawa (pieces absentes du pack Lasouth) : textures dans assets/clothes/south/utg/\n'+'\n'.join(add)+'\n'+s[i:]
    j=s.index('};',s.index('inline const Group groups[] = {'))
    s=s[:j]+'\n'.join(gl)+'\n'+s[j:]
    open(p,'w',encoding='utf-8').write(s)
# UI catalogue
cat=json.load(open(W+'assets/clothes/south/catalogue.json'))
cat=[g for g in cat if not g['key'].startswith('utg')]+d['groups']
json.dump(cat,open(W+'assets/clothes/south/catalogue.json','w'),ensure_ascii=False,indent=2)
open(W+'assets/clothes/south/catalogue.js','w').write('window.SouthCatalogue = '+json.dumps(cat,ensure_ascii=False,indent=2)+';\n')
print('ok',len(names))
