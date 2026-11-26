import struct, io
from PIL import Image
def chunks(b,o,end):
    while o+12<=end:
        t,s,v=struct.unpack_from('<III',b,o); yield t,o+12,s,v; o+=12+s
def read_txd(path):
    b=open(path,'rb').read()
    t,s,v=struct.unpack_from('<III',b,0)
    if t!=0x16: raise ValueError('pas un TXD RenderWare (chiffre ?)')
    texs=[]; ver=v
    for t2,o2,s2,v2 in chunks(b,12,12+s):
        if t2!=0x15: continue
        # struct
        st,so,ss,sv=next(chunks(b,o2,o2+s2))
        d=so
        plat=struct.unpack_from('<I',b,d)[0]
        if plat not in (8,9): texs.append({'err':'plat%d'%plat}); continue
        flt=struct.unpack_from('<I',b,d+4)[0]
        name=b[d+8:d+40].split(b'\0')[0].decode('latin1'); mask=b[d+40:d+72].split(b'\0')[0].decode('latin1')
        rfmt=struct.unpack_from('<I',b,d+72)[0]
        fmt4=b[d+76:d+80]
        w,h=struct.unpack_from('<HH',b,d+80); depth,lv,rt,fl=struct.unpack_from('<BBBB',b,d+84)
        p=d+88
        pal=None
        if rfmt & 0x2000: pal=b[p:p+1024]; p+=1024
        elif rfmt & 0x4000: pal=b[p:p+64]; p+=64
        sz=struct.unpack_from('<I',b,p)[0]; data=b[p+4:p+4+sz]
        texs.append(dict(name=name,mask=mask,rfmt=rfmt,fmt4=fmt4,w=w,h=h,depth=depth,levels=lv,flags=fl,flt=flt,data=data,pal=pal,plat=plat,ver=v2))
    return texs,ver
def decode(t):
    w,h=t['w'],t['h']; f=t['fmt4']; data=t['data']
    if f in (b'DXT1',b'DXT2',b'DXT3',b'DXT4',b'DXT5'):
        fourcc=f if f in (b'DXT1',b'DXT3',b'DXT5') else (b'DXT3' if f==b'DXT2' else b'DXT5')
        hdr=b'DDS '+struct.pack('<7I',124,0x1007,h,w,len(data),0,1)+b'\0'*44+struct.pack('<2I',32,4)+fourcc+b'\0'*20+struct.pack('<I',0x1000)+b'\0'*16
        im=Image.open(io.BytesIO(hdr+data)); im.load(); return im.convert('RGBA')
    base=t['rfmt']&0xF00
    if t['pal'] is not None:
        idx=data[:w*h] if t['depth']==8 else bytes([ (data[i//2]>>(4*(i&1)))&15 for i in range(w*h)])
        pal=t['pal']; px=bytearray()
        for i in idx: px+=bytes([pal[i*4],pal[i*4+1],pal[i*4+2],pal[i*4+3]])
        im=Image.frombytes('RGBA',(w,h),bytes(px)); 
        if not (t['rfmt']&0x0500==0x0500 or base==0x500): pass
        return im
    if t['depth']==32 or base in (0x500,0x600):
        im=Image.frombytes('RGBA',(w,h),data[:w*h*4],'raw','BGRA')
        if base==0x600: im.putalpha(255)
        return im
    if t['depth']==16:
        import numpy as np
        a=np.frombuffer(data[:w*h*2],dtype='<u2').reshape(h,w)
        if base==0x200: r=((a>>11)&31)*255//31; g=((a>>5)&63)*255//63; b=(a&31)*255//31; al=np.full_like(a,255)
        elif base==0x300: al=((a>>12)&15)*17; r=((a>>8)&15)*17; g=((a>>4)&15)*17; b=(a&15)*17
        else: al=((a>>15)&1)*255; r=((a>>10)&31)*255//31; g=((a>>5)&31)*255//31; b=(a&31)*255//31
        import numpy as np
        return Image.fromarray(np.dstack([r,g,b,al]).astype('uint8'),'RGBA')
    raise ValueError('format %s %x depth %d'%(f,t['rfmt'],t['depth']))
def write_txd(path, texs, ver=0x1803FFFF):
    out=b''
    for t in texs:
        im=t['img']; w,h=im.size
        has_alpha=t['alpha']
        rfmt=0x0500
        px=im.tobytes('raw','BGRA')
        st=struct.pack('<I',9)+struct.pack('<I',t['flt'])+t['name'].encode('latin1')[:31].ljust(32,b'\0')+t['mask'].encode('latin1')[:31].ljust(32,b'\0')
        st+=struct.pack('<I',rfmt)+struct.pack('<I',21)+struct.pack('<HHBBBB',w,h,32,1,4,1 if has_alpha else 0)
        st+=struct.pack('<I',len(px))+px
        body=struct.pack('<III',1,len(st),ver)+st+struct.pack('<III',3,0,ver)
        out+=struct.pack('<III',0x15,len(body),ver)+body
    sthdr=struct.pack('<HH',len(texs),0)
    body=struct.pack('<III',1,4,ver)+sthdr+out+struct.pack('<III',3,0,ver)
    open(path,'wb').write(struct.pack('<III',0x16,len(body),ver)+body)

def dff_textures(path):
    """noms des textures utilisees par les materiaux du DFF (tous les atomics)"""
    b=open(path,'rb').read()
    names=set()
    def walk(o,end,depth):
        while o+12<=end:
            t,s,v=struct.unpack_from('<III',b,o)
            if s>end-o-12 or s<0: return
            if t==0x06:  # texture: struct, string(name), string(mask)
                sub=list(chunks(b,o+12,o+12+s))
                strs=[x for x in sub if x[0]==0x02]
                if strs:
                    tt,so,ss,sv=strs[0]; names.add(b[so:so+ss].split(b'\0')[0].decode('latin1').lower())
            elif t in (0x10,0x1A,0x0F,0x08,0x07,0x0E,0x14):
                walk(o+12,o+12+s,depth+1)
            o+=12+s
    walk(0,len(b),0)
    return names

def dff_material_names(path):
    """nom de texture de chaque materiau de la (premiere) geometrie, dans l'ordre du DFF ('' = sans texture)"""
    b = open(path, 'rb').read()
    def find(o, end, typ):
        for t, so, ss, sv in chunks(b, o, end):
            if t == typ: return so, ss
        return None
    clump = (12, len(b) - 12)
    gl = find(clump[0], len(b), 0x1A)
    if not gl: return []
    geo = find(gl[0], gl[0] + gl[1], 0x0F)
    if not geo: return []
    ml = find(geo[0], geo[0] + geo[1], 0x08)
    if not ml: return []
    o, end = ml[0], ml[0] + ml[1]
    it = list(chunks(b, o, end))
    st = [x for x in it if x[0] == 0x01][0]
    n = struct.unpack_from('<I', b, st[1])[0]
    idx = [struct.unpack_from('<i', b, st[1] + 4 + 4 * i)[0] for i in range(n)]
    mats = [x for x in it if x[0] == 0x07]
    names, k = [], 0
    for i in range(n):
        if idx[i] >= 0:
            names.append(names[idx[i]] if idx[i] < len(names) else ''); continue
        nm = ''
        if k < len(mats):
            t, so, ss, sv = mats[k]; k += 1
            tex = find(so, so + ss, 0x06)
            if tex:
                strs = [x for x in chunks(b, tex[0], tex[0] + tex[1]) if x[0] == 0x02]
                if strs:
                    _, s2, l2, _ = strs[0]
                    nm = b[s2:s2 + l2].split(b'\0')[0].decode('latin1')
        names.append(nm)
    return names
