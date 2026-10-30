# Máscara do couro cabeludo na UV do SK_Base_Body (branco = onde o cabelo cobre; o material pinta com a cor do cabelo).
# blender -b --factory-startup --python gerar_mascara_couro.py
# Saída: SourceAssets/Customization/Masks/T_Base_Body_ScalpMask.png (1024x1024, cinza linear).
import bpy,numpy as np
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceAssets/Customization/Masks';OUT.mkdir(parents=True,exist_ok=True)
SIZE=1024
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(ROOT/'Content/HypeWorld/Characters/Base/SK_Base_Body.fbx'))
body=next(o for o in bpy.data.objects if o.type=='MESH')
w=body.matrix_world.copy();body.parent=None;body.matrix_world=w
for m in list(body.modifiers):body.modifiers.remove(m)
bpy.context.view_layer.objects.active=body;body.select_set(True);bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
names={g.index:g.name for g in body.vertex_groups}
def smooth(a,b,x):t=np.clip((x-a)/(b-a),0,1);return t*t*(3-2*t)
# Linha do cabelo (metros, frente = -Y): testa ~1,70 (sobrancelha 1,64), têmporas 1,66, nuca 1,56.
P=np.array([tuple(v.co) for v in body.data.vertices])
head=np.array([sum(g.weight for g in v.groups if 'head1_' in names[g.group]) for v in body.data.vertices])
H=np.interp(P[:,1],[-.12,-.03,.05,.12],[1.70,1.66,1.58,1.54])
mask=smooth(H-.008,H+.02,P[:,2])*smooth(.4,.8,head)
mask*=1-smooth(.094,.102,np.abs(P[:,0]))*(P[:,2]<1.68)  # orelhas ficam com a pele
uv=body.data.uv_layers.active.data
img=np.zeros((SIZE,SIZE),np.float32);cov=np.zeros((SIZE,SIZE),np.float32)
for poly in body.data.polygons:
 idx=list(poly.loop_indices)
 for k in range(1,len(idx)-1):
  L=[idx[0],idx[k],idx[k+1]];T=np.array([tuple(uv[l].uv) for l in L])*SIZE
  vals=np.array([mask[body.data.loops[l].vertex_index] for l in L])
  if vals.max()<=0:continue
  x0,y0=np.floor(T.min(0)).astype(int);x1,y1=np.ceil(T.max(0)).astype(int)
  xs,ys=np.meshgrid(np.arange(max(x0,0),min(x1+1,SIZE)),np.arange(max(y0,0),min(y1+1,SIZE)))
  px=np.c_[xs.ravel()+.5,ys.ravel()+.5];a,b,c=T
  d=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
  if abs(d)<1e-9:continue
  l1=((b[1]-c[1])*(px[:,0]-c[0])+(c[0]-b[0])*(px[:,1]-c[1]))/d;l2=((c[1]-a[1])*(px[:,0]-c[0])+(a[0]-c[0])*(px[:,1]-c[1]))/d;l3=1-l1-l2
  ins=(l1>=-.02)&(l2>=-.02)&(l3>=-.02)
  v=l1*vals[0]+l2*vals[1]+l3*vals[2]
  yy,xx=ys.ravel()[ins],xs.ravel()[ins];img[yy,xx]=np.maximum(img[yy,xx],v[ins]);cov[yy,xx]=1
# dilata 4 px para não aparecer costura na borda das ilhas de UV
for _ in range(4):
 pad=np.pad(img,1);img=np.maximum(img,np.max([pad[1+dy:SIZE+1+dy,1+dx:SIZE+1+dx] for dy in (-1,0,1) for dx in (-1,0,1)],axis=0)*(cov==0))
out=bpy.data.images.new('T_Base_Body_ScalpMask',SIZE,SIZE,float_buffer=False)
out.colorspace_settings.name='Non-Color'
out.pixels=np.dstack([img,img,img,np.ones_like(img)]).ravel()
out.filepath_raw=str(OUT/'T_Base_Body_ScalpMask.png');out.file_format='PNG';out.save()
print('MASCARA',OUT/'T_Base_Body_ScalpMask.png','vertices_no_couro',int((mask>.5).sum()),'de',len(mask))
