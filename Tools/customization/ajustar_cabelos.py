# CORPO BASE DE REFERÊNCIA: Content/HypeWorld/Characters/Base/SK_Base_Body.fbx = etapa 10 de 30/10/2026 (ombros 2,40
# larguras de cabeça, cabeça -5%; ferramentas_ia/corpo_base/etapa10_ombros.py). O rig e a bind pose vêm desse FBX
# (braço e cabeça fora da pose do Indra_Referencia). Versões antigas: SourceAssets/_Old_Versions.
# Reajusta os cabelos na cabeça do SK_Base_Body (substitui o encaixe de cabelo do fit_indra_cosmetics.py).
# blender -b --factory-startup --python ajustar_cabelos.py -- [--ids Hair_itachi,...] [--modo novo|antigo] [--fotos PASTA] [--sem-fbx]
# Os GLBs originais não são alterados. Saída: SourceAssets/Customization/Fitted/<id>/<id>.fbx (o FBX anterior fica em <id>/antes/).
import bpy,sys,re,json,math,shutil,argparse,numpy as np
from pathlib import Path
from mathutils import Vector,Matrix
from mathutils.bvhtree import BVHTree
from mathutils.kdtree import KDTree
ROOT=Path(__file__).resolve().parents[2]
SRC=ROOT/'SourceAssets/Customization/Hair';OUT=ROOT/'SourceAssets/Customization/Fitted'
ap=argparse.ArgumentParser();ap.add_argument('--ids',default='');ap.add_argument('--modo',default='novo');ap.add_argument('--fotos',default='');ap.add_argument('--sem-fbx',action='store_true')
args=ap.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
HAIR_WORDS=['hair','allback','straight','spiky']
# Arquivos que não seguem o molde comum (personagem inteiro, outra unidade ou duplicata).
SKIP={'naruto','naruto__sasuke_low_poly__rig__texture','naruto_sage','naruto_shippuden-_naruto','zabusa (1)'}
GAP=.0025      # folga entre a raiz do cabelo e o couro cabeludo (2,5 mm)
CENTER=Vector((0,.005,1.64))
# Direções (elevação, azimute; azimute 0 = frente = -Y) usadas para medir as duas cabeças.
DIRS=[]
for el in range(0,91,15):
 for az in range(0,360,30):
  if el==90 and az:continue
  e,a=math.radians(el),math.radians(az);DIRS.append((el,az,Vector((math.cos(e)*math.sin(a),-math.cos(e)*math.cos(a),math.sin(e)))))

def clear():bpy.ops.wm.read_factory_settings(use_empty=True)
def hair_objects(path):
 before=set(bpy.data.objects);bpy.ops.import_scene.gltf(filepath=str(path));new=set(bpy.data.objects)-before
 hair=[o for o in new if o.type=='MESH' and any(any(s in m.name.lower() for s in HAIR_WORDS) for m in o.data.materials if m)]
 for o in hair:
  w=o.matrix_world.copy();o.parent=None;o.matrix_world=w
  bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 return new,hair

def source_envelope():
 # Todos os cabelos vêm do mesmo molde de cabeça (a bandana fica no mesmo lugar em todos).
 # A cabeça de origem = superfície interna dos cabelos: 1º toque de um raio saindo do centro, quartil de baixo entre todos.
 radii=[]
 for p in sorted(SRC.glob('*.glb')):
  if p.stem in SKIP:continue
  clear();_,hair=hair_objects(p)
  verts=[];polys=[]
  for o in hair:
   b=len(verts);verts+=[tuple(v.co) for v in o.data.vertices];polys+=[tuple(b+i for i in f.vertices) for f in o.data.polygons]
  if not verts or max(abs(v[2]) for v in verts)>10:continue  # em cm (akatsuki_hat, sakura_young, wasabi): fora da medida do molde
  bvh=BVHTree.FromPolygons(verts,polys)
  radii.append([(lambda h:np.nan if h[0] is None else (h[0]-CENTER).length)(bvh.ray_cast(CENTER,d,.5)) for _,_,d in DIRS])
 R=np.array(radii);return [float(np.nanpercentile(R[:,i],25)) for i in range(len(DIRS))],len(radii)

def fit_ellipsoid(points):
 # Elipsoide alinhado aos eixos, centro em x=0: (x/a)^2+((y-cy)/b)^2+((z-cz)/c)^2=1, resolvido em mínimos quadrados.
 P=np.array(points);A=np.c_[P[:,0]**2,P[:,1]**2,P[:,1],P[:,2]**2,P[:,2],np.ones(len(P))]
 # u*x²+v*y²+w*y+s*z²+t*z+k=0, fixando k=-1 por escala
 sol=np.linalg.lstsq(A[:,:5],np.ones(len(P)),rcond=None)[0];u,v,w,s,t=sol
 cy=-w/(2*v);cz=-t/(2*s);g=1+v*cy*cy+s*cz*cz
 return Vector((0,cy,cz)),Vector((math.sqrt(g/u),math.sqrt(g/v),math.sqrt(g/s)))

def load_base():
 # rig e corpo do mesmo FBX (corpo base atual). Sem o nulo SK_Indra o rig antigo ficava com os ossos em centímetros no
 # mundo (escala x100) e a exportação multiplica a malha por 100: o rig do corpo novo recebe a mesma escala.
 clear();bpy.ops.import_scene.fbx(filepath=str(ROOT/'Content/HypeWorld/Characters/Base/SK_Base_Body.fbx'))
 rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
 body=next(o for o in bpy.context.scene.objects if o.type=='MESH')
 w=body.matrix_world.copy();body.parent=None;body.matrix_world=w
 for o in list(bpy.context.scene.objects):
  if o not in (rig,body):bpy.data.objects.remove(o,do_unlink=True)
 rig.matrix_world=Matrix.Scale(100,4)@rig.matrix_world
 for m in list(body.modifiers):body.modifiers.remove(m)
 bpy.ops.object.select_all(action='DESELECT');body.select_set(True);bpy.context.view_layer.objects.active=body;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 body.name='SK_Base_Body'
 return rig,body

# --- medições ---
src_r,n_src=source_envelope()
rig,body=load_base()
names={g.index:g.name for g in body.vertex_groups}
head_bone=next(b.name for b in rig.data.bones if 'head1_' in b.name);neck_bone=next(b.name for b in rig.data.bones if 'neck_' in b.name)
bbvh=BVHTree.FromObject(body,bpy.context.evaluated_depsgraph_get())
dst_r=[(lambda h:(h[0]-CENTER).length)(bbvh.ray_cast(CENTER+d*.5,-d,1)) for _,_,d in DIRS]
# Crânio = de 15° para cima (sem rosto nem orelhas) + nuca/lados na altura das sobrancelhas.
use=[i for i,(el,az,_) in enumerate(DIRS) if el>=15 or (el==0 and 60<=az<=300)]
src_c,src_ax=fit_ellipsoid([CENTER+DIRS[i][2]*src_r[i] for i in use if not math.isnan(src_r[i])])
dst_c,dst_ax=fit_ellipsoid([CENTER+DIRS[i][2]*dst_r[i] for i in use])
S=Vector((dst_ax.x/src_ax.x,dst_ax.y/src_ax.y,0))
# A altura do crânio de origem não sai confiável da parte de dentro do cabelo (as mechas laterais se afastam da cabeça);
# vertical = média de largura e profundidade, e os topos ficam alinhados: a linha da bandana cai na sobrancelha do corpo base (z 1,64).
S.z=(S.x+S.y)/2
src_top=src_c.z+src_ax.z;dst_top=dst_c.z+dst_ax.z
medidas={'cabelos_medidos':n_src,'origem':{'centro':list(src_c),'semi_eixos_xyz':list(src_ax),'topo_z':src_c.z+src_ax.z},
 'corpo_base':{'centro':list(dst_c),'semi_eixos_xyz':list(dst_ax),'topo_z':dst_c.z+dst_ax.z},'escala_xyz':list(S),'bandana_origem_z':1.646,'bandana_no_corpo_base_z':dst_top-(src_top-1.646)*S.z}
print('MEDIDAS',json.dumps(medidas))
(ROOT/'Tools/customization/cabecas_medidas.json').write_text(json.dumps(medidas,indent=1))

def map_old(co):
 # Encaixe antigo (fit_indra_cosmetics.py), só para as fotos de "antes".
 head=[v.co for v in body.data.vertices if sum(g.weight for g in v.groups if 'head1_' in names[g.group])>.7]
 hlo=Vector([min(v[i] for v in head) for i in range(3)]);hhi=Vector([max(v[i] for v in head) for i in range(3)])
 sc=(hhi.x-hlo.x)/.198;tc=(hlo+hhi)/2;tc.z=hhi.z-(1.697-1.618)*sc
 return [(c-Vector((0,0,1.618)))*sc+tc for c in co]

def nearest(p):
 q,n,_,d=bbvh.find_nearest(p);return q,n,(d if (p-q).dot(n)>=0 else -d)

def fit_hair(obj):
 co=[v.co.copy() for v in obj.data.vertices]
 if args.modo=='antigo':return map_old(co),{}
 # 1) crânio de origem sobre o crânio do corpo base, escala eixo por eixo
 co=[Vector(((c-src_c).x*S.x,(c-src_c).y*S.y+dst_c.y,(c.z-src_top)*S.z+dst_top)) for c in co]
 P=np.array([tuple(c) for c in co]);N=len(co);C=np.array(tuple(dst_c))
 # 2) encaixe radial suave: para cada direção a partir do centro do crânio mede a folga entre a parte de dentro
 #    do cabelo e o couro cabeludo, suaviza (~12°) e move o cabelo nessa direção. Todas as camadas de uma mecha
 #    andam a mesma distância: as raízes encostam (folga GAP) e o volume de fora não achata.
 polys=[tuple(f.vertices) for f in obj.data.polygons];hbvh=BVHTree.FromPolygons([tuple(c) for c in co],polys)
 g=np.arange(3000)+.5;phi=np.arccos(1-2*g/3000);th=np.pi*(1+5**.5)*g
 D=np.c_[np.cos(th)*np.sin(phi),np.sin(th)*np.sin(phi),np.cos(phi)];gap=np.full(len(D),np.nan)
 for k,d in enumerate(D):
  d=Vector(d);s=bbvh.ray_cast(dst_c+d*.5,-d,1)[0];h=hbvh.ray_cast(dst_c,d,.5)[0]
  if s is None or h is None or s.z<1.45:continue
  # sobre o rosto (abaixo da sobrancelha, na frente) a franja fica solta de propósito
  if s.z<1.645 and s.y<-.03:continue
  r=(h-dst_c).length-(s-dst_c).length
  # mais longe que 2,5 cm = volume de propósito do penteado; mais de 1,5 cm para dentro = parte escondida no modelo
  if -.015<r<.025:gap[k]=r-GAP
 ok=~np.isnan(gap);Dv=D[ok];G=gap[ok]
 def field(V,values,dirs,sigma):
  U=V-C;U/=np.linalg.norm(U,axis=1)[:,None];A=np.arccos(np.clip(U@dirs.T,-1,1));W=np.exp(-(A/sigma)**2)
  return (W@values)/(W.sum(1)+.3),U
 shift,U=field(P,G,Dv,math.radians(12))
 rad=np.linalg.norm(P-C,axis=1)
 f=np.clip((P[:,2]-1.45)/.08,0,1)  # mechas que descem nas costas não são puxadas
 out=[]
 for p,u,s_,fi in zip(P,U,shift,f):
  v=Vector(p-u*s_*fi)
  # o puxão para no couro cabeludo/orelha (não entra para depois ser empurrado de volta)
  if s_>0 and nearest(v)[2]<GAP and nearest(Vector(p))[2]>=GAP:
   lo_,hi_=0.,1.
   for _ in range(6):
    m=(lo_+hi_)/2
    if nearest(Vector(p-u*s_*fi*m))[2]>=GAP:lo_=m
    else:hi_=m
   v=Vector(p-u*s_*fi*lo_)
  out.append(v)
 # 3) nada dentro da cabeça: vértices e pontos dentro dos triângulos; o empurrão também é suave por direção
 tri=[(f_[0],f_[k],f_[k+1]) for f_ in polys for k in range(1,len(f_)-1)]
 bary=[(a/4,b/4,1-a/4-b/4) for a in range(5) for b in range(5-a)]
 pushed=0;total=[0.]*len(out)
 for it in range(10):
  need=[];where=[];normal=[]
  for t in tri:
   for w in bary:
    c=out[t[0]]*w[0]+out[t[1]]*w[1]+out[t[2]]*w[2]
    if c.z<1.45:continue
    d=nearest(c)[2]
    # fundo dentro da cabeça (> 1,5 cm) = parte escondida de propósito no modelo: não empurra (senão o cabelo incha)
    if -.015<d<GAP*.6:q,n,_d=nearest(c);need.append(min(GAP-d+.0005,.012));where.append(tuple(c));normal.append(tuple(n))
  if not need:break
  # empurra na direção da pele (normal), espalhado por 2,5 cm: a mecha sobe inteira, sem dobra
  pushed+=len(need);Q=np.array([tuple(v) for v in out]);Wp=np.array(where);Nn=np.array(normal)*np.array(need)[:,None]
  move=[]
  for qv in Q:
   d=np.linalg.norm(Wp-qv,axis=1);k=np.where(d<.025)[0]
   if not len(k):move.append(None);continue
   m=Nn[k]*((1-d[k]/.025)**2)[:,None];move.append(m[np.argmax(np.linalg.norm(m,axis=1))])
  # empurrão total por vértice limitado a 1,5 cm
  for i,m in enumerate(move):
   if m is None:continue
   m=Vector(m);room=.015-total[i]
   if room<=0:continue
   if m.length>room:m=m*(room/m.length)
   out[i]=out[i]+m;total[i]+=m.length
 Q=np.array([tuple(v) for v in out]);inner=[nearest(v)[2] for v in out if v.z>1.6]
 return out,{'direcoes_medidas':int(ok.sum()),'deslocamento_mm_min_max':[round(float(shift.min())*1000,1),round(float(shift.max())*1000,1)],'pontos_empurrados':pushed,'folga_minima_mm':round(min(inner)*1000,1)}

def weights(obj):
 for g in list(obj.vertex_groups):obj.vertex_groups.remove(g)
 gh=obj.vertex_groups.new(name=head_bone);gn=obj.vertex_groups.new(name=neck_bone)
 for v in obj.data.vertices:
  # mechas da nuca (atrás, abaixo do crânio) passam aos poucos para o pescoço
  t=min(1,max(0,(1.60-v.co.z)/.10))*(1 if v.co.y>.02 else 0)*.6;t=t*t*(3-2*t) if t<1 else t
  gh.add([v.index],1-t,'REPLACE')
  if t>0:gn.add([v.index],t,'REPLACE')

def silhouette(objs):
 pts=np.array([tuple(v.co) for o in objs for v in o.data.vertices if v.co.z>1.60]);hl=np.array(dst_ax)
 if not len(pts):return {}
 w=(pts[:,0].max()-pts[:,0].min())/2;top=pts[:,2].max()-dst_c.z
 return {'largura_vs_cabeca_%':round((w/hl[0]-1)*100,1),'topo_vs_cabeca_%':round((top/hl[2]-1)*100,1)}

def export(obj,id):
 dest=OUT/id;old=dest/(id+'.fbx')
 if old.exists() and not (dest/'antes'/(id+'.fbx')).exists():(dest/'antes').mkdir(exist_ok=True);shutil.copy2(old,dest/'antes'/(id+'.fbx'))
 for v in obj.data.vertices:v.co*=100  # o rig de referência guarda centímetros (igual ao fit_indra_cosmetics.py)
 obj.name='SK_'+id
 for m in list(obj.modifiers):obj.modifiers.remove(m)
 obj.parent=rig;obj.matrix_parent_inverse=rig.matrix_world.inverted();bpy.context.view_layer.update()
 mod=obj.modifiers.new('Indra','ARMATURE');mod.object=rig
 bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);obj.select_set(True);bpy.context.view_layer.objects.active=obj
 bpy.ops.export_scene.fbx(filepath=str(old),global_scale=0.01,use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,bake_anim=False,use_armature_deform_only=False,apply_unit_scale=True,axis_forward='-Z',axis_up='Y',path_mode='AUTO',mesh_smooth_type='FACE')
 for v in obj.data.vertices:v.co/=100
 obj.parent=None;obj.modifiers.clear()

def photos(objs,folder,tag):
 folder=Path(folder);folder.mkdir(parents=True,exist_ok=True);sc=bpy.context.scene
 sc.render.engine='BLENDER_WORKBENCH';sc.display.shading.light='STUDIO';sc.display.shading.color_type='MATERIAL';sc.display.shading.show_cavity=True
 sc.render.resolution_x=sc.render.resolution_y=640;sc.render.film_transparent=False
 sc.world=sc.world or bpy.data.worlds.new('W');sc.display.shading.background_type='VIEWPORT'
 skin=bpy.data.materials.new('pele');skin.diffuse_color=(.85,.72,.62,1);body.data.materials.clear();body.data.materials.append(skin)
 hm=bpy.data.materials.new('cabelo');hm.diffuse_color=(.12,.13,.18,1)
 for o in objs:o.data.materials.clear();o.data.materials.append(hm)
 cam=bpy.data.objects.new('cam',bpy.data.cameras.new('cam'));sc.collection.objects.link(cam);sc.camera=cam
 cam.data.type='ORTHO';cam.data.ortho_scale=.5;tgt=Vector((0,0,1.63))
 for name,d in [('frente',Vector((0,-1,0))),('lado',Vector((1,0,0))),('costas',Vector((0,1,0))),('cima',Vector((0,-.25,1)).normalized())]:
  cam.location=tgt+d*2;cam.rotation_euler=(tgt-cam.location).to_track_quat('-Z','Y' if name!='cima' else 'Y').to_euler()
  sc.render.filepath=str(folder/f'{tag}_{name}.png');bpy.ops.render.render(write_still=True)
 bpy.data.objects.remove(cam)

ids=[s for s in args.ids.split(',') if s]
report={}
for p in sorted(SRC.glob('*.glb')):
 if p.stem in SKIP:continue
 base_id='Hair_'+re.sub('[^A-Za-z0-9_]','_',p.stem)
 if ids and not any(i==base_id or i.startswith(base_id+'_') for i in ids):continue
 new,hair=hair_objects(p)
 for o in new:
  if o not in hair:bpy.data.objects.remove(o,do_unlink=True)
 if hair and max(abs(v.co.z) for o in hair for v in o.data.vertices)>10:
  for o in hair:  # alguns GLBs estão em centímetros (ex.: akatsuki_hat)
   for v in o.data.vertices:v.co*=.01
 if not hair or max(v.co.z for o in hair for v in o.data.vertices)<1.2:
  report[base_id]={'pulado':'outra unidade/origem (sakura_young, wasabi): fica o FBX antigo, precisa de encaixe manual'}
  for o in hair:bpy.data.objects.remove(o,do_unlink=True)
  continue
 done=[]
 for j,obj in enumerate(hair):
  id=base_id+(f'_{j}' if len(hair)>1 else '')
  co,info=fit_hair(obj)
  for v,c in zip(obj.data.vertices,co):v.co=c
  weights(obj);done.append((id,obj,info))
 info={i:inf for i,_,inf in done};info['silhueta']=silhouette([o for _,o,_ in done]);report[base_id]=info
 print('CABELO',base_id,json.dumps(info))
 if args.fotos:photos([o for _,o,_ in done],args.fotos,f'{base_id}_{args.modo}')
 if not args.sem_fbx and args.modo=='novo':
  for id,obj,_ in done:export(obj,id)
 for _,o,_ in done:bpy.data.objects.remove(o,do_unlink=True)
print('RELATORIO',json.dumps(report))
