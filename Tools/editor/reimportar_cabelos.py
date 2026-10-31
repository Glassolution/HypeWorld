# CORPO BASE DE REFERÊNCIA: /Game/HypeWorld/Characters/Base/SK_Base_Body (etapa 10 de 30/10/2026: ombros 2,40 larguras
# de cabeça, cabeça -5%). A bind pose das peças é copiada dele, não do SK_Indra: braço e cabeça saíram da pose do Indra.
import unreal,json
# Reimporta por cima dos SK_Hair_* os FBX gerados por Tools/customization/ajustar_cabelos (mesmo Skeleton, mesmo material).
# Pode rodar de novo sem duplicar nada. IDS vazio = todos os cabelos do manifesto.
IDS=[]
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve()
FOLDER='/Game/Ninja/Characters/Customization'
DATA=ROOT/'SourceAssets/Customization/Fitted';rows=json.loads((DATA/'manifest.json').read_text())['parts']
E=unreal.EditorAssetLibrary;A=unreal.AssetToolsHelpers.get_asset_tools()
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
skel=unreal.load_asset('/Game/HypeWorld/Personagens/Indra/SKEL_Indra')
BODY=unreal.load_asset('/Game/HypeWorld/Characters/Base/SK_Base_Body')
ref=unreal.SkeletonModifier();ref.set_skeletal_mesh(BODY);names=ref.get_all_bone_names()
for row in rows:
 id=row['id']
 if row['kind']!='hair' or (IDS and id not in IDS):continue
 folder=FOLDER+'/'+id;mi=unreal.load_asset(folder+'/MI_'+id)
 opts=unreal.FbxImportUI();opts.set_editor_property('import_as_skeletal',True);opts.set_editor_property('import_mesh',True);opts.set_editor_property('import_animations',False);opts.set_editor_property('import_materials',False);opts.set_editor_property('import_textures',False);opts.set_editor_property('create_physics_asset',False);opts.set_editor_property('skeleton',skel);opts.get_editor_property('skeletal_mesh_import_data').set_editor_property('update_skeleton_reference_pose',False)
 task=unreal.AssetImportTask();task.filename=str(DATA/id/(id+'.fbx'));task.destination_path=folder;task.destination_name='SK_'+id;task.automated=True;task.replace_existing=True;task.save=True;task.options=opts;A.import_asset_tasks([task])
 mesh=unreal.load_asset(folder+'/SK_'+id);assert mesh,id
 mod=unreal.SkeletonModifier();assert mod.set_skeletal_mesh(mesh)
 assert set(map(str,mod.get_all_bone_names()))==set(map(str,names)),id
 # bind pose idêntica à do corpo base (igual ao importar_cosmeticos_indra.py); antes confere que o FBX já está nela
 pior=max((mod.get_bone_transform(n,True).translation-ref.get_bone_transform(n,True).translation).length() for n in names)
 assert pior<.1,(id,'bind pose longe do SK_Base_Body (cm)',pior)
 mod.set_bones_transforms(names,[ref.get_bone_transform(n,False) for n in names],True);mod.commit_skeleton_to_skeletal_mesh()
 if mi:
  # o FBX novo traz o material com outro nome e a reimportação cria um 2º espaço vazio (= material padrão cinza): todos recebem o MI
  slots=mesh.get_editor_property('materials')
  for k,slot in enumerate(slots):slot.set_editor_property('material_interface',mi);slots[k]=slot
  mesh.call_method('SetMaterials',(slots,))
 E.save_loaded_asset(mesh,False)
 b=mesh.get_bounds();unreal.log('CABELO_REIMPORTADO %s topo_cm=%.1f'%(id,b.origin.z+b.box_extent.z))
