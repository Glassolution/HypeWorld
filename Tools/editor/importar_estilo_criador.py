import sys, math, os, unreal
from pathlib import Path
r=Path(unreal.Paths.project_dir()).resolve()
stage=r/'SourceAssets/Customization/CreatorStyle'
sys.path.insert(0,str(r/'Content/Python'))
import indra_character_standard as standard
u='/Game/HypeWorld/Personagens/Indra'
m=u+'/Modular'
f='/Game/Ninja/Characters/Customization'
paths={
 'SK_Indra_Head':(stage/'IndraModular/SK_Indra_Head.fbx',m),
 'SK_Hair_Indra':(stage/'IndraModular/SK_Hair_Indra.fbx',m),
 'SK_Outfit_Indra':(stage/'IndraModular/SK_Outfit_Indra.fbx',m),
}
for ident in ('ItachiAnbu','KakashiAnbu','Sasuke'):
 name='SK_Outfit_'+ident
 paths[name]=(stage/'IndraModular'/ident/(name+'.fbx'),m+'/Trajes/'+ident)
for p in sorted((stage/'Fitted').glob('Hair_*/Hair_*.fbx')):
 paths['SK_'+p.stem]=(p,f+'/'+p.stem)
skel=unreal.load_asset(u+'/SKEL_Indra')
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
A=unreal.AssetToolsHelpers.get_asset_tools()
E=unreal.EditorAssetLibrary
for name,(source,folder) in paths.items():
 if os.environ.get('HYPE_CREATOR_QUICK') and name not in ('SK_Outfit_ItachiAnbu','SK_Hair_sasuke_boruto'):
  continue
 old=unreal.load_asset(folder+'/'+name)
 if not old:
  print('ESTUDO_PULADO',name)
  continue
 old_ref=standard._reference_bones(old)
 old_physics=old.get_editor_property('physics_asset')
 old_slots=old.get_editor_property('materials')
 materials={str(x.get_editor_property('imported_material_slot_name')): x.get_editor_property('material_interface') for x in old_slots}
 opts=unreal.FbxImportUI()
 for prop,val in {'import_as_skeletal':True,'import_mesh':True,'import_animations':False,'import_materials':False,'import_textures':False,'create_physics_asset':False,'skeleton':skel}.items():
  opts.set_editor_property(prop,val)
 data=opts.get_editor_property('skeletal_mesh_import_data')
 data.set_editor_property('update_skeleton_reference_pose',False)
 data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
 data.set_editor_property('vertex_color_import_option',unreal.VertexColorImportOption.REPLACE)
 task=unreal.AssetImportTask()
 task.filename=str(source);task.destination_path=folder;task.destination_name=name
 task.automated=True;task.replace_existing=True;task.save=False;task.options=opts
 A.import_asset_tasks([task])
 assert task.get_editor_property('imported_object_paths'),name
 mesh=unreal.load_asset(folder+'/'+name)
 new_ref=standard._reference_bones(mesh)
 assert set(new_ref)==set(old_ref),(name,'ossos')
 err=max(math.dist(new_ref[k]['position_cm'],v['position_cm']) for k,v in old_ref.items())
 assert err<.1,(name,'bind pose',err)
 slots=mesh.get_editor_property('materials')
 for index,slot in enumerate(slots):
  key=str(slot.get_editor_property('imported_material_slot_name'))
  if key in materials:
   slot.set_editor_property('material_interface',materials[key]);slots[index]=slot
  else:
   assert slot.get_editor_property('material_interface'),(name,'material',key)
 mesh.call_method('SetMaterials',(slots,))
 mesh.set_editor_property('physics_asset',old_physics)
 assert E.save_loaded_asset(mesh,False),name
 print('ESTUDO_IMPORTADO',name,err)
