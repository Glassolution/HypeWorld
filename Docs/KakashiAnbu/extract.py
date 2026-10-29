from pathlib import Path
import json,struct,numpy as np
from PIL import Image,ImageDraw
import io
OUT=Path(__file__).parent
SOURCE=Path(r'C:/Users/lucas/Downloads/personagem para customizacao/Personagens para pegar as roupas/anbu_kakashi_hatake.glb')
data=SOURCE.read_bytes();jl=struct.unpack_from('<I',data,12)[0];g=json.loads(data[20:20+jl]);blob=data[28+jl:]
def read(i):
    a=g['accessors'][i];v=g['bufferViews'][a['bufferView']];n={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']];dtype={5126:'<f4',5125:'<u4',5123:'<u2',5121:'u1'}[a['componentType']];dt=np.dtype(dtype);offset=v.get('byteOffset',0)+a.get('byteOffset',0);stride=v.get('byteStride',dt.itemsize*n)
    return np.ndarray((a['count'],n),dtype=dt,buffer=blob,offset=offset,strides=(stride,dt.itemsize)).copy()
textures=[]
for i,image in enumerate(g['images']):
    view=g['bufferViews'][image['bufferView']];raw=blob[view.get('byteOffset',0):view.get('byteOffset',0)+view['byteLength']]
    (OUT/f'texture_{i}.png').write_bytes(raw);textures.append(Image.open(io.BytesIO(raw)).convert('RGB'))
P=[];N=[];UV=[];T=[];MAT=[];parts=[];offset=0
for mi,mesh in enumerate(g['meshes']):
    for primitive in mesh['primitives']:
        a=primitive['attributes'];p=read(a['POSITION']).astype(float)*100;n=read(a['NORMAL']);uv=read(a['TEXCOORD_0']);tri=read(primitive['indices']).reshape(-1,3)
        P.append(p);N.append(n);UV.append(uv);T.append(tri+offset);MAT.extend([primitive.get('material',0)]*len(tri));parts.extend([mi]*len(p));offset+=len(p)
P=np.vstack(P);N=np.vstack(N);UV=np.vstack(UV);T=np.vstack(T);MAT=np.array(MAT);parts=np.array(parts)
np.savez(OUT/'geometry.npz',P=P,N=N,UV=UV,T=T,MAT=MAT,PART=parts)
(OUT/'source.json').write_text(json.dumps({'source':str(SOURCE),'asset':g['asset'],'materials':g['materials'],'textures':g['textures'],'bounds_cm':[P.min(0).tolist(),P.max(0).tolist()]},indent=2))
canvas=Image.new('RGB',(1200,900),'#e9edf1');draw=ImageDraw.Draw(canvas)
for view in range(2):
    ax=0 if view==0 else 1;depth=1 if view==0 else 0;center=300 if view==0 else 900;scale=3.8
    for index in np.argsort(-P[T,depth].mean(1)):
        tri=T[index];material=g['materials'][MAT[index]];ti=material.get('pbrMetallicRoughness',{}).get('baseColorTexture',{}).get('index',0);tex=textures[g['textures'][ti]['source']];uv=UV[tri].mean(0);color=tex.getpixel((int(uv[0]*tex.width)%tex.width,int((1-uv[1])*tex.height)%tex.height))
        points=[(center+P[v,ax]*scale,820-P[v,2]*scale) for v in tri];draw.polygon(points,fill=color)
draw.text((30,25),'Kakashi ANBU - geometria original, sem rig',fill='black')
canvas.save(OUT/'reference.png')
print('GEOMETRY',len(P),'TRIANGLES',len(T),'BOUNDS',P.min(0),P.max(0))
