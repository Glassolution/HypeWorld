# CORPO BASE DE REFERÊNCIA: /Game/HypeWorld/Characters/Base/SK_Base_Body (etapa 10 de 30/10/2026: ombros 2,40 larguras
# de cabeça, cabeça -5%). A bind pose das peças é copiada dele, não do SK_Indra: braço e cabeça saíram da pose do Indra.
import unreal,json
# Reimporta por cima dos SK_Hair_* os FBX gerados por Tools/customization/ajustar_cabelos (mesmo Skeleton, mesmo material).
# Pode rodar de novo sem duplicar nada. IDS vazio = todos os cabelos do manifesto.
IDS=globals().get("IDS", [])
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve()
FOLDER='/Game/Ninja/Characters/Customization'
DATA=Path(globals().get('FITTED_SOURCE', ROOT/'SourceAssets/Customization/Fitted'));rows=json.loads((DATA/'manifest.json').read_text())['parts']
E=unreal.EditorAssetLibrary;A=unreal.AssetToolsHelpers.get_asset_tools()
M=unreal.MaterialEditingLibrary

def accessory_material(folder, slot_name, texture_file):
 tex_task=unreal.AssetImportTask()
 tex_task.filename=str(texture_file)
 tex_task.destination_path=folder
 tex_task.destination_name='T_'+slot_name
 tex_task.automated=True
 tex_task.replace_existing=True
 tex_task.save=True
 A.import_asset_tasks([tex_task])
 tex=unreal.load_asset(folder+'/'+tex_task.destination_name)
 assert tex,texture_file
 master_folder=FOLDER+'/Materials'
 master_name='M_CreatorHairAccessory'
 master=unreal.load_asset(master_folder+'/'+master_name)
 if not master:
  master=A.create_asset(master_name,master_folder,unreal.Material,unreal.MaterialFactoryNew())
  master.set_editor_property('two_sided',True)
  master.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
  sample=M.create_material_expression(master,unreal.MaterialExpressionTextureSampleParameter2D)
  sample.set_editor_property('parameter_name','Texture')
  sample.set_editor_property('texture',tex)
  M.connect_material_property(sample,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
  M.connect_material_property(sample,'A',unreal.MaterialProperty.MP_OPACITY_MASK)
  M.set_base_material_usage(master,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH,True)
  M.recompile_material(master)
  E.save_loaded_asset(master,False)
 name='MI_'+slot_name
 mi=unreal.load_asset(folder+'/'+name)
 if not mi:
  mi=A.create_asset(name,folder,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
 M.set_material_instance_parent(mi,master)
 M.set_material_instance_texture_parameter_value(mi,'Texture',tex)
 E.save_loaded_asset(mi,False)
 return mi

unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
skel=unreal.load_asset('/Game/HypeWorld/Personagens/Indra/SKEL_Indra')
BODY=unreal.load_asset('/Game/HypeWorld/Characters/Base/SK_Base_Body')
ref=unreal.SkeletonModifier();ref.set_skeletal_mesh(BODY);names=ref.get_all_bone_names()
for row in rows:
 id=row['id']
 if row['kind']!='hair' or (IDS and id not in IDS):continue
 folder=FOLDER+'/'+id;mi=unreal.load_asset(folder+'/MI_'+id)
 opts=unreal.FbxImportUI();opts.set_editor_property('import_as_skeletal',True);opts.set_editor_property('import_mesh',True);opts.set_editor_property('import_animations',False);opts.set_editor_property('import_materials',False);opts.set_editor_property('import_textures',False);opts.set_editor_property('create_physics_asset',False);opts.set_editor_property('skeleton',skel);opts.get_editor_property('skeletal_mesh_import_data').set_editor_property('update_skeleton_reference_pose',False)
 task=unreal.AssetImportTask();task.filename=str(DATA/id/(id+'.fbx'));task.destination_path=folder;task.destination_name='SK_'+id;task.automated=True;task.replace_existing=True;task.save=False;task.options=opts;A.import_asset_tasks([task])
 assert task.get_editor_property('imported_object_paths'),('Falha ao importar',id)
 mesh=unreal.load_asset(folder+'/SK_'+id);assert mesh,id
 mod=unreal.SkeletonModifier();assert mod.set_skeletal_mesh(mesh)
 assert set(map(str,mod.get_all_bone_names()))==set(map(str,names)),id
 assert all(mod.get_parent_name(n)==ref.get_parent_name(n) for n in names),(id,'hierarquia diferente')
 # bind pose idêntica à do corpo base (igual ao importar_cosmeticos_indra.py); antes confere que o FBX já está nela
 pior=max((mod.get_bone_transform(n,True).translation-ref.get_bone_transform(n,True).translation).length() for n in names)
 assert pior<.1,(id,'bind pose longe do SK_Base_Body (cm)',pior)
 mod.set_bones_transforms(names,[ref.get_bone_transform(n,False) for n in names],True);mod.commit_skeleton_to_skeletal_mesh()
 accessory_file=DATA/id/'acessorios.json'
 accessories=json.loads(accessory_file.read_text()) if accessory_file.exists() else {}
 assigned=set()
 slots=mesh.get_editor_property('materials')
 for k,slot in enumerate(slots):
  original=str(slot.get_editor_property('imported_material_slot_name'))
  mat=mi
  if original in accessories:
   mat=accessory_material(folder,original,DATA/id/accessories[original])
   assigned.add(original)
  assert mat,(id,original)
  slot.set_editor_property('material_interface',mat)
  slots[k]=slot
 assert assigned==set(accessories),(id,sorted(set(accessories)-assigned))
 mesh.call_method('SetMaterials',(slots,))
 assert E.save_loaded_asset(mesh,False),('Falha ao salvar',id)
 b=mesh.get_bounds();unreal.log('CABELO_REIMPORTADO %s topo_cm=%.1f'%(id,b.origin.z+b.box_extent.z))
