import bpy,json,math,re,zipfile,bmesh,numpy as np,tempfile
from pathlib import Path
from mathutils import Vector,Matrix
from mathutils.kdtree import KDTree
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceAssets/Customization/Fitted';OUT.mkdir(parents=True,exist_ok=True)
WORK=tempfile.TemporaryDirectory(prefix='hype-fitting-')
for archive in sorted((ROOT/'SourceAssets/Customization/Clothes').glob('*.zip')):
 with zipfile.ZipFile(archive) as z:
  for entry in z.infolist():
   if not entry.is_dir() and not Path(entry.filename).is_absolute() and '..' not in Path(entry.filename).parts:
    z.extract(entry,Path(WORK.name)/archive.stem)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(ROOT/'Docs/PadraoPersonagem/Indra_Referencia.fbx'))
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
for o in list(bpy.context.scene.objects):
 if o!=rig:bpy.data.objects.remove(o,do_unlink=True)
before=set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=str(ROOT/'Content/HypeWorld/Characters/Base/SK_Base_Body.fbx'))
donor=next(o for o in set(bpy.data.objects)-before if o.type=='MESH')
world=donor.matrix_world.copy();donor.parent=None;donor.matrix_world=world
for o in set(bpy.data.objects)-before:
 if o!=donor:bpy.data.objects.remove(o,do_unlink=True)
# Weight donor is a copy of the actual base body, never the Indra coat.
for mod in list(donor.modifiers):donor.modifiers.remove(mod)
bpy.context.view_layer.objects.active=donor;donor.select_set(True)
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
kd=KDTree(len(donor.data.vertices))
for v in donor.data.vertices:kd.insert(v.co,v.index)
kd.balance(); groupnames={g.index:g.name for g in donor.vertex_groups}
weights={v.index:[(groupnames[g.group],g.weight) for g in v.groups] for v in donor.data.vertices}
head=[v.co for v in donor.data.vertices if sum(g.weight for g in v.groups if 'head1_' in groupnames[g.group])>.7]
hlo=Vector([min(v[i] for v in head) for i in range(3)]);hhi=Vector([max(v[i] for v in head) for i in range(3)])
head_center=(hlo+hhi)/2;head_bone=next(b.name for b in rig.data.bones if 'head1_' in b.name)
report={'head_min':list(hlo),'head_max':list(hhi),'parts':[],'excluded':[]}
def texture(obj,dest):
 mat=obj.data.materials[0] if obj.data.materials else None
 image=None
 if mat and mat.use_nodes:
  bs=next((n for n in mat.node_tree.nodes if n.type=='BSDF_PRINCIPLED'),None)
  if bs and bs.inputs['Base Color'].is_linked:
   n=bs.inputs['Base Color'].links[0].from_node
   if n.type=='TEX_IMAGE':image=n.image
 if image is None:
  image=bpy.data.images.new('BaseColor',width=4,height=4);c=tuple(mat.diffuse_color) if mat else (1,1,1,1);image.pixels=list(c)*16
 image.filepath_raw=str(dest);image.file_format='PNG';image.save()
def export(obj,id,label,kind,clan='',mask=None):
 dest=OUT/id;dest.mkdir(exist_ok=True);texture(obj,dest/'BaseColor.png')
 bm=bmesh.new();bm.from_mesh(obj.data);bmesh.ops.dissolve_degenerate(bm,dist=0.0000001,edges=list(bm.edges));bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(obj.data);bm.free()
 # The reference rig stores centimeter bone coordinates. Convert fitted geometry only.
 for v in obj.data.vertices:v.co*=100
 obj.name='SK_'+id
 for m in list(obj.modifiers):obj.modifiers.remove(m)
 obj.parent=rig;obj.matrix_parent_inverse=rig.matrix_world.inverted()
 bpy.context.view_layer.update()
 mod=obj.modifiers.new('Indra','ARMATURE');mod.object=rig
 bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);obj.select_set(True);bpy.context.view_layer.objects.active=obj
 bpy.ops.export_scene.fbx(filepath=str(dest/(id+'.fbx')),global_scale=0.01,use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,bake_anim=False,use_armature_deform_only=False,apply_unit_scale=True,axis_forward='-Z',axis_up='Y',path_mode='AUTO',mesh_smooth_type='FACE')
 row={'id':id,'label':label,'kind':kind,'clan':clan,'vertices':len(obj.data.vertices),'triangles':sum(len(p.vertices)-2 for p in obj.data.polygons),'file':id+'/'+id+'.fbx','texture':id+'/BaseColor.png','mask':mask}
 report['parts'].append(row);bpy.data.objects.remove(obj,do_unlink=True)
for p in sorted((ROOT/'SourceAssets/Customization/Hair').glob('*.glb')):
 if p.stem=='zabusa (1)':report['excluded'].append({'file':p.name,'reason':'identical duplicate of zabusa'});continue
 before=set(bpy.data.objects);bpy.ops.import_scene.gltf(filepath=str(p));new=set(bpy.data.objects)-before
 hair=[o for o in new if o.type=='MESH' and any(any(s in m.name.lower() for s in ['hair','allback','straight','spiky']) for m in o.data.materials)]
 if not hair:
  report['excluded'].append({'file':p.name,'reason':'complete character, hair not isolated by material'})
 else:
  for j,obj in enumerate(hair):
   # Apply source transforms, then align the common source scalp envelope to the actual base head.
   world=obj.matrix_world.copy();obj.parent=None;obj.matrix_world=world
   bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
   # Source GLBs use meters; some accessory exports bake centimeters into vertices.
   if max(abs(v.co.z) for v in obj.data.vertices)>10:
    for v in obj.data.vertices:v.co*=.01
   for g in list(obj.vertex_groups):obj.vertex_groups.remove(g)
   source_center=Vector((0,0,1.618)); scale=(hhi.x-hlo.x)/.198
   target_center=head_center.copy();target_center.z=hhi.z-(1.697-1.618)*scale
   for v in obj.data.vertices:v.co=(v.co-source_center)*scale+target_center
   # These two source files have a translated modeling origin and larger scalp width.
   if p.stem in ['sakura_young','wasabi']:
    # Undo the shared-template fit, then use individually calibrated scalp landmarks.
    source_scale=.90 if p.stem=='sakura_young' else .94
    source_top=.635917 if p.stem=='sakura_young' else .491311
    target_top=1.82 if p.stem=='sakura_young' else 1.84
    for v in obj.data.vertices:
     original=(v.co-target_center)/scale+source_center
     v.co=Vector((original.x*source_scale,(original.y-.018)*source_scale+head_center.y,(original.z-source_top)*source_scale+target_top))
   group=obj.vertex_groups.new(name=head_bone);group.add(list(range(len(obj.data.vertices))),1,'REPLACE')
   id='Hair_'+re.sub('[^A-Za-z0-9_]','_',p.stem)+(f'_{j}' if len(hair)>1 else '')
   export(obj,id,'Penteado para trás' if p.stem=='b536425a98f64881bd78a48eb127072c' else p.stem.replace('_',' ').title(),'hair')
 for o in new:
  try:
   if o.name in bpy.data.objects:bpy.data.objects.remove(o,do_unlink=True)
  except ReferenceError:pass
# Fit headless T-pose outfits to the exact A-pose joint positions; bind by the base-body surface weights.
for p in sorted(Path(WORK.name).rglob('*.fbx')):
 before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(p));new=set(bpy.data.objects)-before
 obj=next(o for o in new if o.type=='MESH')
 world=obj.matrix_world.copy();obj.parent=None;obj.matrix_world=world
 bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 lo=Vector([min(v.co[i] for v in obj.data.vertices) for i in range(3)]);hi=Vector([max(v.co[i] for v in obj.data.vertices) for i in range(3)]);scale=1.56/(hi.z-lo.z)
 # Estimate the source sleeve center line, then map it to Indra's shoulder-wrist axis.
 points=np.array([tuple((v.co-Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z)))*scale) for v in obj.data.vertices])
 ax=np.abs(points[:,0]); bins=[]
 for x in np.arange(.3,float(ax.max()),.035):
  section=points[(ax>=x)&(ax<x+.035)&(points[:,2]>1.15)]
  if len(section)>8:bins.append((x+.0175,(section[:,1].min()+section[:,1].max())/2,(section[:,2].min()+section[:,2].max())/2))
 bins=np.array(bins); sy,iy=np.polyfit(bins[:,0],bins[:,1],1);sz,iz=np.polyfit(bins[:,0],bins[:,2],1)
 arm_length=.505/max(.1,float(ax.max())-.17) if 'Armor' not in p.stem else 1.
 for v,point in zip(obj.data.vertices,points):
  q=Vector(point);side=1 if q.x>=0 else -1;x=abs(q.x)
  if x>.14 and q.z>1.12:
   blend=min(1,max(0,(x-.14)/.08));blend=blend*blend*(3-2*blend)
   t=(x-.17)*arm_length;crossz=(q.z-(sz*x+iz))*1.12;crossy=(q.y-(sy*x+iy))*1.12
   rotated=Vector((side*(.157+(t+crossz)*math.sqrt(.5)),.013+crossy,1.393+(-t+crossz)*math.sqrt(.5)))
   q=q.lerp(rotated,blend)
  elif q.z>1.0:
   q.x*=1.045;q.y*=1.045
  if q.z<.85:q.x*=1+.25*max(0,1-q.z/.85)
  v.co=q
 # Reduce generated triangle density before skinning; original high resolution files are preserved.
 ratio=min(1,45000/max(1,len(obj.data.polygons)))
 if ratio<.99:
  mod=obj.modifiers.new('Game density','DECIMATE');mod.ratio=ratio;bpy.ops.object.modifier_apply(modifier=mod.name)
 for g in list(obj.vertex_groups):obj.vertex_groups.remove(g)
 groups={name:obj.vertex_groups.new(name=name) for name in groupnames.values()}
 for v in obj.data.vertices:
  combined={}
  for co,index,distance in kd.find_n(v.co,4):
   factor=1/max(distance,.002)**2
   for name,w in weights[index]:combined[name]=combined.get(name,0)+w*factor
  top=sorted(combined.items(),key=lambda x:-x[1])[:4];total=sum(w for _,w in top)
  for name,w in top:
   if total:groups[name].add([v.index],w/total,'REPLACE')
 label=p.stem.replace('Meshy_AI_','').split('_20261029')[0]
 short='Uchiha_'+label
 mask={'torso_min_z':80.,'torso_max_z':147.,'arm_max_t':29. if 'Armor' in label else 49.,'leg_min_z':0.,'foot_max_z':18.}
 export(obj,short,label.replace('_',' '),'outfit','Uchiha',mask)
 for o in new:
  try:
   if o.name in bpy.data.objects:bpy.data.objects.remove(o,do_unlink=True)
  except ReferenceError:pass
(OUT/'manifest.json').write_text(json.dumps(report,indent=2))
WORK.cleanup()
