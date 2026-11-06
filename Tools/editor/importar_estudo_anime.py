from pathlib import Path
import json,math,sys,unreal
root=Path(unreal.Paths.project_dir()).resolve()
source=root/'SourceAssets/Customization/AnimeStudy'
sys.path.insert(0,str(root/'Content/Python'))
import indra_character_standard as standard
E=unreal.EditorAssetLibrary;A=unreal.AssetToolsHelpers.get_asset_tools();M=unreal.MaterialEditingLibrary
indra='/Game/HypeWorld/Personagens/Indra'
mod=indra+'/Modular'
dest=mod+'/AnimeStudy'
skel=unreal.load_asset(indra+'/SKEL_Indra')
original=unreal.load_asset(mod+'/SK_Indra_Head')
ref=standard._reference_bones(original)
palette=json.loads((source/'materiais.json').read_text())
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')

def material(name,rgb):
 path=dest+'/M_'+name
 mat=unreal.load_asset(path) if E.does_asset_exist(path) else A.create_asset('M_'+name,dest,unreal.Material,unreal.MaterialFactoryNew())
 assert mat
 M.delete_all_material_expressions(mat)
 mat.set_editor_property('two_sided',True)
 node=M.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-350,0)
 node.set_editor_property('constant',unreal.LinearColor(*rgb,1))
 color=node
 if name=='Rosto_Pele':
  tint=M.create_material_expression(mat,unreal.MaterialExpressionVectorParameter,-350,170)
  tint.set_editor_property('parameter_name','SkinTone')
  tint.set_editor_property('default_value',unreal.LinearColor(.85,.62,.51,1))
  gain=M.create_material_expression(mat,unreal.MaterialExpressionMultiply,-100,80)
  node.set_editor_property('constant',unreal.LinearColor(rgb[0]/.85,rgb[1]/.62,rgb[2]/.51,1))
  M.connect_material_expressions(node,'',gain,'A')
  M.connect_material_expressions(tint,'',gain,'B')
  vertex=M.create_material_expression(mat,unreal.MaterialExpressionVertexColor,-350,340)
  shadow=M.create_material_expression(mat,unreal.MaterialExpressionMultiply,100,80)
  M.connect_material_expressions(gain,'',shadow,'A')
  M.connect_material_expressions(vertex,'RGB',shadow,'B')
  color=shadow
 M.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
 rough=M.create_material_expression(mat,unreal.MaterialExpressionConstant,-100,250)
 rough.set_editor_property('r',.62 if name.startswith('Cabelo_') else .70)
 if name.startswith('Cabelo_'):
  spec=M.create_material_expression(mat,unreal.MaterialExpressionConstant,-100,360)
  spec.set_editor_property('r',.20)
  M.connect_material_property(spec,'',unreal.MaterialProperty.MP_SPECULAR)
 M.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
 M.set_base_material_usage(mat,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH,True)
 M.recompile_material(mat)
 assert E.save_loaded_asset(mat,False)
 return mat

mats={key:material(key,value) for key,value in palette.items()}
for name in ('SK_Indra_Head_Anime','SK_Hair_Anime_Camadas','SK_Outfit_ItachiAnbu'):
 outfit=name.startswith('SK_Outfit_')
 folder=mod+'/Trajes/ItachiAnbu' if outfit else dest
 if outfit:
  existing=unreal.load_asset(folder+'/'+name)
  old_slots={str(slot.get_editor_property('imported_material_slot_name')):slot.get_editor_property('material_interface')
             for slot in existing.get_editor_property('materials')}
 opts=unreal.FbxImportUI()
 for key,value in {'import_as_skeletal':True,'import_mesh':True,'import_animations':False,'import_materials':False,
                   'import_textures':False,'create_physics_asset':False,'skeleton':skel}.items():
  opts.set_editor_property(key,value)
 data=opts.get_editor_property('skeletal_mesh_import_data')
 data.set_editor_property('update_skeleton_reference_pose',False)
 if not outfit:
  data.set_editor_property('vertex_color_import_option',unreal.VertexColorImportOption.REPLACE)
 data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
 task=unreal.AssetImportTask()
 task.filename=str(source/(name+'.fbx'));task.destination_path=folder;task.destination_name=name
 task.automated=True;task.replace_existing=True;task.save=False;task.options=opts
 A.import_asset_tasks([task])
 assert task.get_editor_property('imported_object_paths'),name
 mesh=unreal.load_asset(folder+'/'+name)
 assert mesh and mesh.get_editor_property('skeleton')==skel,name
 actual=standard._reference_bones(mesh)
 assert set(actual)==set(ref),name
 err=max(math.dist(actual[k]['position_cm'],v['position_cm']) for k,v in ref.items())
 assert err<.1,(name,err)
 slots=mesh.get_editor_property('materials')
 for i,slot in enumerate(slots):
  key=str(slot.get_editor_property('imported_material_slot_name'))
  selected=(old_slots if outfit else mats).get(key)
  assert selected,(name,key)
  slot.set_editor_property('material_interface',selected);slots[i]=slot
 mesh.call_method('SetMaterials',(slots,))
 mesh.set_editor_property('physics_asset',unreal.load_asset(indra+'/PHYS_Indra'))
 assert E.save_loaded_asset(mesh,False),name
 print('ANIME_IMPORTADO',name,err)
