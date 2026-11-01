import re,os,sys,colorsys
ROOT='/home/claude/work/app/src/main/assets/'
DIRS='admin anim bank blazer common config dialog faction fish gang garage gov house inventory job locker login map npc police report rp service shop ui'.split()
def clamp(v): return max(0,min(255,int(round(v))))
def conv(r,g,b,a):
    # bleuatre ?
    mx=max(r,g,b); mn=min(r,g,b)
    if not (b>=r+18 and b>=g+4 and (mx-mn)>=18): return None
    lum=0.299*r+0.587*g+0.114*b
    if lum<26:                       # fond de panel -> noir translucide
        return (0,0,0, round(min(a,1.0)*0.78,3))
    if lum<75:                       # surfaces / cartes -> voile blanc tres leger sur le noir
        k=0.05+(lum-26)/49*0.09
        return (255,255,255, round(min(a,1.0)*k*(1.0 if a>=0.5 else 2.0) if a<1 else k,3))
    # accents / textes : gris neutre de meme luminosite (un peu eclairci pour rester lisible)
    gr=clamp(lum*1.08 if lum<200 else lum)
    return (gr,gr,gr,a)
def fmt(c, orig_hex=None):
    r,g,b,a=c
    if a>=0.999 and orig_hex: return '#%02x%02x%02x'%(r,g,b)
    return 'rgba(%d,%d,%d,%s)'%(r,g,b,('%g'%a))
hexre=re.compile(r'#([0-9a-fA-F]{8}|[0-9a-fA-F]{6}|[0-9a-fA-F]{3})\b')
rgbre=re.compile(r'rgba?\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*([\d.]+%?)\s*)?\)')
stats={}
def sub_hex(m):
    h=m.group(1)
    if len(h)==3: r,g,b=[int(x*2,16) for x in h]; a=1.0
    elif len(h)==6: r,g,b=int(h[0:2],16),int(h[2:4],16),int(h[4:6],16); a=1.0
    else: r,g,b,a=int(h[0:2],16),int(h[2:4],16),int(h[4:6],16),int(h[6:8],16)/255
    c=conv(r,g,b,a)
    if not c: return m.group(0)
    stats['n']=stats.get('n',0)+1
    return fmt(c, True)
def sub_rgb(m):
    r,g,b=int(m.group(1)),int(m.group(2)),int(m.group(3)); a=m.group(4)
    a=1.0 if a is None else (float(a[:-1])/100 if a.endswith('%') else float(a))
    c=conv(r,g,b,a)
    if not c: return m.group(0)
    stats['n']=stats.get('n',0)+1
    return fmt(c)
def process(text, is_html):
    if is_html:
        # seulement dans <style> et attributs style= / fill= / stroke=
        def in_style(m): return m.group(1)+hexre.sub(sub_hex, rgbre.sub(sub_rgb, m.group(2)))+m.group(3)
        text=re.sub(r'(<style[^>]*>)(.*?)(</style>)', in_style, text, flags=re.S)
        text=re.sub(r'((?:style|fill|stroke|stop-color)=")([^"]*)(")', in_style, text)
        return text
    return hexre.sub(sub_hex, rgbre.sub(sub_rgb, text))
changed=0
for d in DIRS:
    for dp,_,fs in os.walk(ROOT+d):
        for f in fs:
            if not (f.endswith('.css') or f.endswith('.html')): continue
            p=os.path.join(dp,f); t=open(p,encoding='utf-8',errors='surrogateescape').read()
            n0=stats.get('n',0); t2=process(t, f.endswith('.html'))
            if t2!=t: open(p,'w',encoding='utf-8',errors='surrogateescape').write(t2); changed+=1; print(p.replace(ROOT,''), stats['n']-n0)
print('files',changed,'colors',stats.get('n',0))
