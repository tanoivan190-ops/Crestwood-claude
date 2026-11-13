import json, re, os, sys, io, struct, collections, hashlib
import xml.etree.ElementTree as ET
sys.path.insert(0,'/tmp/claude-0/rw')
from txdpc import textures, to_img
PK="/home/claude/up3/SISTEMA + ARQUIVOS PARA DATA/UTGAWA_CUSTOM_SAMP/"
WORK="/home/claude/work/app/src/main/"
SOUTH=WORK+"assets/clothes/south/"
OUTREL="utg/"                          # relative to south/
# ---- textures of the pack
TX={}
def tex_img(ref):
    txd,name=ref.split(':')
    if txd not in TX:
        TX[txd]={n:(w,h,d,fc,data) for n,w,h,d,fc,data in textures(PK+'modelos/roupas/'+txd+'.txd')}
    v=TX[txd].get(name)
    if v is None: return None
    return to_img(*v)
written={}
def asset_for(ref, gen):
    """write the pack texture as png/jpg under south/utg/, return path relative to south/"""
    if ref in written: return written[ref]
    img=tex_img(ref)
    if img is None: print('MISSING',ref); written[ref]=None; return None
    if max(img.size)>512:
        s=512/max(img.size); img=img.resize((max(1,round(img.size[0]*s)),max(1,round(img.size[1]*s))))
    txd,name=ref.split(':')
    alpha=img.getchannel('A').getextrema()[0]<250
    rel=f"{OUTREL}{txd}/{name}.{'png' if alpha else 'jpg'}"
    os.makedirs(os.path.dirname(SOUTH+rel),exist_ok=True)
    if alpha: img.save(SOUTH+rel,optimize=True)
    else: img.convert('RGB').save(SOUTH+rel,quality=90)
    written[ref]=rel; return rel
# ---- Lasouth catalogue
cat=json.load(open(SOUTH+'catalogue.json'))
lz={}
for g in cat:
    for it in g['items']: lz[(g['model'],it['name'],it['material'])]=it['texture']
def nat(s): return [int(x) if x.isdigit() else x for x in re.split(r'(\d+)',s)]
def rnum(it):
    m=re.search(r'(\d+)\s*$',it.get('rotulo') or ''); return int(m.group(1)) if m else 0
BRL={}      # brl name -> path relative to south/
def put(name,ref,model,item=None):
    p=None
    if item is not None and item.get('malha'):
        p=lz.get((model,item.get('nome'),item.get('malha')))
    if p is None: p=asset_for(ref,model)
    if p: BRL[name]=p
counts={}
groups_new=[]
for gen,model,pre in (('male',7,'m7'),('female',9,'f9')):
    r=ET.parse(PK+f'xml/catalogo_{gen}.xml').getroot()
    items=list(r.find('itens'))
    corpo={g.get('nome'):[m.replace('.','_') for m in g.get('malhas').split()] for g in r.find('corpo')}
    # tones
    for t in r.find('tons'):
        n=int(t.get('n')); parts={p.get('grupo'):p.get('tex') for p in t}
        put(f'brl_{pre}_skin_t{n:02d}_torso',parts['torso1'],model)
        put(f'brl_{pre}_skin_t{n:02d}_leg',parts['legleft1'],model)
        put(f'brl_{pre}_skin_t{n:02d}_feet',parts.get('feetleft') or parts['feets'],model)
        for f in range(1,13): put(f'brl_{pre}_skin_t{n:02d}_head{f:02d}',t.get('cabeca'),model)
    # clothes per mesh (and hair)
    bymesh=collections.defaultdict(list)
    for it in items:
        tex=it.get('tex'); slot=it.get('slot')
        if not tex: continue
        txd,tn=tex.split(':')
        if slot=='hairs':
            s,c=tn.split('_'); bymesh[f'fantasy_{gen[0]}_hair{s}'].append((0,int(c),it))
        elif it.get('malha') and not slot.startswith('tattoo'):
            brand = 'versace' in txd and 1 or 'louisvuitton' in txd and 2 or 0
            num=int(tn.split('_')[-1]) if tn.split('_')[-1].isdigit() else 0
            bymesh[it.get('malha')].append((brand,num,it))
    for mesh,lst in bymesh.items():
        base=sorted([x for x in lst if x[0]==0],key=lambda x:x[1]); mx=max([x[1] for x in base],default=0)
        k=mx
        for x in base: put(f'brl_{pre}_clothes_{mesh}_{x[1]:02d}',x[2].get('tex'),model,x[2])
        for x in sorted([x for x in lst if x[0]],key=lambda x:(x[0],nat(x[2].get('tex')))):
            k+=1; put(f'brl_{pre}_clothes_{mesh}_{k:02d}',x[2].get('tex'),model,x[2])
        counts[(gen,mesh)]=k
    # face pieces
    for it in items:
        tex=it.get('tex'); slot=it.get('slot')
        if not tex: continue
        tn=tex.split(':')[1]
        if slot=='eyebrown': put(f'brl_m7_eyebrown_{int(tn):02d}',tex,model) if gen=='male' else None
        if slot=='beards': put(f'brl_m7_beard_{int(tn.split("_")[-1]):02d}',tex,model)
        if slot=='makeup': put(f'brl_f9_makeup_{int(tn):02d}',tex,model)
    # tattoos
    area={'tattooarml':('m7','arm'),'tattoolegl':('m7','leg'),'tattootorsoback':(pre,'back'),'tattootorsofront':(pre,'front')}
    for slot,(p2,a) in area.items():
        if gen=='female' and a in('arm','leg'): continue
        lst=[it for it in items if it.get('slot')==slot and it.get('tex')]
        lst.sort(key=lambda it:('louisvuitton' in it.get('tex'), rnum(it), nat(it.get('tex'))))
        for i,it in enumerate(lst,1): put(f'brl_{p2}_tattoo_{a}_{i:02d}',it.get('tex'),model)
        counts[(gen,'tattoo_'+a)]=len(lst)
    for s in ('eyebrown','beards','makeup'): counts[(gen,s)]=sum(1 for it in items if it.get('slot')==s)
    # Utgawa-only collection groups
    def body_expand(lst):
        out=[]
        for p in lst:
            if p.startswith('body_') and p[5:] in corpo: out+=corpo[p[5:]]
            else: out.append(p)
        return ','.join(out)
    want={'male':[('hat','utgHat','Chapeaux (Utgawa)','tete'),('glasse','utgGlasse','Lunettes (Utgawa)','tete'),('necklace','utgNecklace','Colliers (Utgawa)','bijoux'),
                  ('tattooshoulderl','utgTatShoulderL','Tatouage epaule gauche (Utgawa)','tatouages'),('tattooshoulderr','utgTatShoulderR','Tatouage epaule droite (Utgawa)','tatouages')],
          'female':[('t-shirts','utgTop','Hauts (Utgawa)','haut')]}[gen]
    for slot,key,label,tab in want:
        lst=[it for it in items if it.get('slot')==slot and (gen=='male' or not it.get('tex'))]
        lst.sort(key=lambda it:(rnum(it),nat(it.get('nome'))))
        gi=[]
        for it in lst:
            tex=it.get('tex')
            path=asset_for(tex,model) if tex else ''
            ap=it.find('aplica'); rm=it.find('remove'); bl=it.find('bloqueadoPor')
            applyv=body_expand(ap.text.split()) if ap is not None and ap.text else ''
            if bl is not None and bl.text: applyv='!'+','.join(bl.text.split())   # '!' = masque si une de ces pieces est portee
            gi.append({'name':it.get('nome'),'material':it.get('malha') or it.get('malhas'),'texture':path or '',
                       'apply':applyv,
                       'remove':body_expand(rm.text.split()) if rm is not None and rm.text else ''})
        groups_new.append({'model':model,'key':key,'label':label,'tab':tab,'items':gi})
json.dump({'brl':BRL,'groups':groups_new,'counts':{f'{a}|{b}':v for (a,b),v in counts.items()}},open('/tmp/claude-0/utg/out.json','w'),indent=0)
print('brl',len(BRL),'new files',sum(1 for v in written.values() if v),'groups',[(g['key'],g['model'],len(g['items'])) for g in groups_new])
for k,v in sorted(counts.items()): print(k,v)
