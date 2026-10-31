# CORPO BASE DE REFERÊNCIA: /Game/HypeWorld/Characters/Base/SK_Base_Body (etapa 10 de 30/10/2026: ombros 2,40 larguras
# de cabeça, cabeça -5%). A bind pose das peças é copiada dele, não do SK_Indra: braço e cabeça saíram da pose do Indra.
import unreal,sys,json,math
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(ROOT/'Content/Python'))
import indra_character_standard as standard
FOLDER='/Game/Ninja/Characters/Customization'
DATA=ROOT/'SourceAssets/Customization/Fitted';report=json.loads((DATA/'manifest.json').read_text())
E=unreal.EditorAssetLibrary;A=unreal.AssetToolsHelpers.get_asset_tools();M=unreal.MaterialEditingLibrary
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
skel=unreal.load_asset('/Game/HypeWorld/Personagens/Indra/SKEL_Indra');BODY=unreal.load_asset('/Game/HypeWorld/Characters/Base/SK_Base_Body');ref=unreal.SkeletonModifier();ref.set_skeletal_mesh(BODY);names=ref.get_all_bone_names();expected=standard._reference_bones(BODY)
validated=[]
def texture(file,folder,name):
 task=unreal.AssetImportTask();task.filename=str(file);task.destination_path=folder;task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True;A.import_asset_tasks([task]);return unreal.load_asset(folder+'/'+name)
def parent(hair,tex):
 name='M_CreatorHair' if hair else 'M_CreatorOutfit';path=FOLDER+'/Materials/'+name
 mat=unreal.load_asset(path)
 if mat:
  M.set_base_material_usage(mat,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH,True);E.save_loaded_asset(mat,False);return mat
 mat=A.create_asset(name,FOLDER+'/Materials',unreal.Material,unreal.MaterialFactoryNew());mat.set_editor_property('two_sided',True)
 if hair:mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
 def node(cls,**props):
  n=M.create_material_expression(mat,cls)
  for k,v in props.items():n.set_editor_property(k,v)
  return n
 sample=node(unreal.MaterialExpressionTextureSampleParameter2D,parameter_name='Texture',texture=tex)
 tint=node(unreal.MaterialExpressionVectorParameter,parameter_name='BaseColor Tint',default_value=unreal.LinearColor(1,1,1,1))
 base=sample
 if hair:
  gray=node(unreal.MaterialExpressionDesaturation);M.connect_material_expressions(sample,'RGB',gray,'')
  boost=node(unreal.MaterialExpressionMultiply,const_b=3.0);M.connect_material_expressions(gray,'',boost,'A')
  add=node(unreal.MaterialExpressionAdd,const_b=.25);M.connect_material_expressions(boost,'',add,'A')
  clamp=node(unreal.MaterialExpressionClamp);M.connect_material_expressions(add,'',clamp,'');base=clamp
  M.connect_material_property(sample,'A',unreal.MaterialProperty.MP_OPACITY_MASK)
 mul=node(unreal.MaterialExpressionMultiply);M.connect_material_expressions(base,'RGB' if base==sample else '',mul,'A');M.connect_material_expressions(tint,'',mul,'B');M.connect_material_property(mul,'',unreal.MaterialProperty.MP_BASE_COLOR)
 rough=node(unreal.MaterialExpressionConstant,r=.85);M.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
 M.set_base_material_usage(mat,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH,True);M.recompile_material(mat);E.save_loaded_asset(mat,False);return mat
for i,row in enumerate(report['parts']):
 id=row['id'];folder=FOLDER+'/'+id
 opts=unreal.FbxImportUI();opts.set_editor_property('import_as_skeletal',True);opts.set_editor_property('import_mesh',True);opts.set_editor_property('import_animations',False);opts.set_editor_property('import_materials',False);opts.set_editor_property('import_textures',False);opts.set_editor_property('create_physics_asset',False);opts.set_editor_property('skeleton',skel);opts.get_editor_property('skeletal_mesh_import_data').set_editor_property('update_skeleton_reference_pose',False)
 task=unreal.AssetImportTask();task.filename=str(DATA/id/(id+'.fbx'));task.destination_path=folder;task.destination_name='SK_'+id;task.automated=True;task.replace_existing=True;task.save=True;task.options=opts;A.import_asset_tasks([task])
 mesh=unreal.load_asset(folder+'/SK_'+id);assert mesh,id
 mod=unreal.SkeletonModifier();assert mod.set_skeletal_mesh(mesh)
 assert set(map(str,mod.get_all_bone_names()))==set(map(str,names))
 actual=standard._reference_bones(mesh)
 for name,e in expected.items():
  assert ref.get_parent_name(name)==mod.get_parent_name(name)
  a=actual[name]
  assert math.dist(a['position_cm'],e['position_cm'])<.1,(id,name,'position')
  assert max(abs(x-y) for x,y in zip(a['scale'],e['scale']))<.001,(id,name,'scale')
  assert abs(sum(x*y for x,y in zip(a['rotation_xyzw'],e['rotation_xyzw'])))>.999,(id,name,'orientation')
 mod.set_bones_transforms(names,[ref.get_bone_transform(n,False) for n in names],True);mod.commit_skeleton_to_skeletal_mesh();standard.validate_mesh(mesh,BODY)
 tex=texture(DATA/id/'BaseColor.png',folder,'T_'+id)
 master=parent(row['kind']=='hair',tex);mi=unreal.load_asset(folder+'/MI_'+id)
 if not mi:mi=A.create_asset('MI_'+id,folder,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
 M.set_material_instance_parent(mi,master);M.set_material_instance_texture_parameter_value(mi,'Texture',tex);M.set_material_usage_override(mi,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH,True,True);E.save_loaded_asset(mi,False)
 slots=mesh.get_editor_property('materials');slot=slots[0];slot.set_editor_property('material_interface',mi);slots[0]=slot;mesh.call_method('SetMaterials',(slots,));E.save_loaded_asset(mesh,False)
 bounds=mesh.get_bounds();z=bounds.origin.z;size=bounds.box_extent.z*2
 assert (50<z<200 and 5<size<200 and 150<z+size/2<220) if row['kind']=='hair' else (60<z<100 and 140<size<180),(id,z,size)
 validated.append({'id':id,'bones':len(names),'bounds':str(mesh.get_bounds())});unreal.log('FIT_VALIDATED '+id)
 (ROOT/'Docs/Personalizacao/Trajes/import_validation.json').write_text(json.dumps(validated,indent=2))
 if i%5==0:unreal.SystemLibrary.collect_garbage()
unreal.log('ALL_FITTED_PARTS_VALIDATED '+str(len(validated)))

folder=FOLDER+'/Materials';body=unreal.load_asset(folder+'/MI_CreatorBody')
if not body:body=A.create_asset('MI_CreatorBody',folder,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
M.set_material_instance_parent(body,unreal.load_asset('/Game/HypeWorld/Characters/Base/MI_Base_Body_Custom'))
override=body.get_editor_property('base_property_overrides');override.set_editor_property('override_blend_mode',True);override.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED);body.set_editor_property('base_property_overrides',override);E.save_loaded_asset(body)
