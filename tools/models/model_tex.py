import sys,struct,os,json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))  # txdpc.py est dans le meme dossier
from txdpc import textures,to_img
SRC=sys.argv[1]
OUT=sys.argv[2]
os.makedirs(OUT+'tex',exist_ok=True)
# 1) textures -> png/jpg (max 512)
names={}
for name,w,h,d,fc,data in textures(SRC+sys.argv[3]+'.txd'):
    img=to_img(w,h,d,fc,data)
    if img is None: print('skip',name,hex(fc),d); continue
    if max(img.size)>512:
        s=512/max(img.size); img=img.resize((max(1,round(img.size[0]*s)),max(1,round(img.size[1]*s))))
    alpha=img.getchannel('A').getextrema()[0]<250
    fn=name.lower()+('.png' if alpha else '.jpg')
    if alpha: img.save(OUT+'tex/'+fn,optimize=True)
    else: img.convert('RGB').save(OUT+'tex/'+fn,quality=88)
    names[name.lower()]=fn
# 2) DFF : texture name of each material (in order) of the (single) geometry
b=open(SRC+sys.argv[3]+'.dff','rb').read()
mats=[]
def walk(o,end,path):
    while o+12<=end:
        t,s,v=struct.unpack_from('<III',b,o)
        body=o+12
        if t==0x07:   # material
            # material: struct, then texture (0x06) whose first string child (0x02) is the name
            q=body; tex=''
            while q+12<=body+s:
                tt,ss,vv=struct.unpack_from('<III',b,q)
                if tt==0x06:
                    r=q+12
                    while r+12<=q+12+ss:
                        t2,s2,v2=struct.unpack_from('<III',b,r)
                        if t2==0x02: tex=b[r+12:r+12+s2].split(b'\0')[0].decode('latin-1'); break
                        r+=12+s2
                q+=12+ss
            mats.append(tex)
        elif t in (0x10,0x1A,0x0F,0x08):
            walk(body,body+s,path+[t])
        o=body+s
walk(0,len(b),[])
print('materials',len(mats),'textures',len(names))
missing=[m for m in mats if m and m.lower() not in names]
print('missing',missing[:10])
open(OUT+'materials.txt','w').write('\n'.join((names.get(m.lower(),'') if m else '') for m in mats)+'\n')
