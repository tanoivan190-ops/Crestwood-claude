import struct,sys,io
from PIL import Image
def textures(p):
    b=open(p,'rb').read(); o=12; t,s,v=struct.unpack_from('<III',b,o); o+=12+s
    while o+12<=len(b):
        t,s,v=struct.unpack_from('<III',b,o)
        if t!=0x15: break
        st=o+12; d=st+12
        plat,filt=struct.unpack_from('<II',b,d)
        name=b[d+8:d+40].split(b'\0')[0].decode('latin-1')
        q=d+8+64
        rfmt,fourcc=struct.unpack_from('<II',b,q); q+=8
        w,h,depth,levels,rtype,flags=struct.unpack_from('<HHBBBB',b,q); q+=8
        size=struct.unpack_from('<I',b,q)[0]; q+=4
        yield name,w,h,depth,fourcc,b[q:q+size]
        o=o+12+s
def to_img(w,h,depth,fourcc,data):
    fc=struct.pack('<I',fourcc)
    if fc in (b'DXT1',b'DXT3',b'DXT5'):
        hdr=b'DDS '+struct.pack('<7I',124,0x1|0x2|0x4|0x1000,h,w,len(data),0,1)+b'\0'*44+struct.pack('<2I',32,4)+fc+b'\0'*20+struct.pack('<I',0x1000)+b'\0'*16
        return Image.open(io.BytesIO(hdr+data)).convert('RGBA')
    if depth==32: return Image.frombytes('RGBA',(w,h),data[:w*h*4],'raw','BGRA')
    return None
