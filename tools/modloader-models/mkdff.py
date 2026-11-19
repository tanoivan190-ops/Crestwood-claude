import struct,sys
V=0x1803FFFF
def ch(t,data): return struct.pack('<III',t,len(data),V)+data
def box_dff(name,sx,sy,sz):
    hx,hy,hz=sx/2,sy/2,sz/2
    faces=[((1,0,0),[(hx,-hy,-hz),(hx,hy,-hz),(hx,hy,hz),(hx,-hy,hz)]),
           ((-1,0,0),[(-hx,hy,-hz),(-hx,-hy,-hz),(-hx,-hy,hz),(-hx,hy,hz)]),
           ((0,1,0),[(hx,hy,-hz),(-hx,hy,-hz),(-hx,hy,hz),(hx,hy,hz)]),
           ((0,-1,0),[(-hx,-hy,-hz),(hx,-hy,-hz),(hx,-hy,hz),(-hx,-hy,hz)]),
           ((0,0,1),[(-hx,-hy,hz),(hx,-hy,hz),(hx,hy,hz),(-hx,hy,hz)]),
           ((0,0,-1),[(-hx,hy,-hz),(hx,hy,-hz),(hx,-hy,-hz),(-hx,-hy,-hz)])]
    verts=[];norms=[];uvs=[];tris=[]
    uv=[(0,1),(1,1),(1,0),(0,0)]
    for n,q in faces:
        b=len(verts)
        for i,p in enumerate(q): verts.append(p);norms.append(n);uvs.append(uv[i])
        tris.append((b,b+1,b+2));tris.append((b,b+2,b+3))
    nv=len(verts);nt=len(tris)
    # Geometry struct
    g=struct.pack('<IIII',0x00010076,nt,nv,1)
    for u in uvs: g+=struct.pack('<ff',*u)
    for a,b_,c in tris: g+=struct.pack('<HHHH',b_,a,0,c)   # v2,v1,mat,v3
    r=max(hx,hy,hz); r=(hx*hx+hy*hy+hz*hz)**0.5
    g+=struct.pack('<ffffII',0,0,0,r,1,1)
    for v in verts: g+=struct.pack('<fff',*v)
    for n in norms: g+=struct.pack('<fff',*n)
    gstruct=ch(0x01,g)
    mat=ch(0x01,struct.pack('<IIIIfff',0,0xFFFFFFFF,1,0,1.0,1.0,1.0))+ch(0x03,b'')
    matlist=ch(0x08,ch(0x01,struct.pack('<Ii',1,-1))+ch(0x07,mat))
    binmesh=struct.pack('<III',0,1,nt*3)+struct.pack('<II',nt*3,0)
    for a,b_,c in tris: binmesh+=struct.pack('<III',a,b_,c)
    gext=ch(0x03,ch(0x050E,binmesh))
    geom=ch(0x0F,gstruct+matlist+gext)
    gl=ch(0x1A,ch(0x01,struct.pack('<I',1))+geom)
    fr=struct.pack('<I',1)+struct.pack('<9f',1,0,0,0,1,0,0,0,1)+struct.pack('<3f',0,0,0)+struct.pack('<iI',-1,0x00020003)
    nm=name.encode()+b'\0'; nm+=b'\0'*((-len(nm))%4)
    frext=ch(0x03,ch(0x0253F2FE,nm))
    fl=ch(0x0E,ch(0x01,fr)+frext)
    at=ch(0x14,ch(0x01,struct.pack('<IIII',0,0,5,0))+ch(0x03,b''))
    cl=ch(0x10,ch(0x01,struct.pack('<III',1,0,0))+fl+gl+at+ch(0x03,b''))
    return cl
if __name__=='__main__':
    specs={'Concrete1mx1mx5m':(1,1,5),'Concrete10mx1mx5m':(10,1,5),'Concrete1mx1mx25m':(1,1,25),
           'Concrete3mx3mx5m':(3,3,5),'Concrete5mx5mx5m':(5,5,5),'Concrete10mx10mx5m':(10,10,5)}
    for n,d in specs.items():
        open(n+'.dff','wb').write(box_dff(n,*d))
        print(n,len(box_dff(n,*d)))
