import unreal
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/ItachiAnbu');D='/Game/HypeWorld/Personagens/ItachiAnbu';al=unreal.EditorAssetLibrary
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
mesh=al.load_asset(D+'/SK_ItachiAnbu');skeleton=mesh.get_editor_property('skeleton');materials=list(mesh.get_editor_property('materials'));physics=mesh.get_editor_property('physics_asset')
params=unreal.ImportAssetParameters();params.is_automated=True;params.replace_existing=True;params.follow_redirectors=True;params.reimport_asset=mesh
pipeline=unreal.InterchangeGenericAssetsPipeline();pipeline.get_editor_property('mesh_pipeline').set_editor_property('combine_skeletal_meshes_behavior',unreal.InterchangeCombineSkeletalMeshesBehavior.BY_SKELETON);pipeline.get_editor_property('mesh_pipeline').set_editor_property('create_physics_asset',False);pipeline.get_editor_property('animation_pipeline').set_editor_property('import_animations',False);pipeline.get_editor_property('material_pipeline').set_editor_property('import_materials',False)
params.override_pipelines.append(unreal.SoftObjectPath(pipeline.get_path_name()))
assert unreal.InterchangeManager.get_interchange_manager_scripted().import_asset(D,unreal.InterchangeManager.create_source_data(str(OUT/'SK_ItachiAnbu.glb')),params)
assert mesh.get_editor_property('skeleton')==skeleton
mesh.set_editor_property('materials',materials);mesh.set_editor_property('physics_asset',physics);assert al.save_loaded_asset(mesh)
print('ITACHI_NECK_SEAM_UPDATED_SKELETON_PRESERVED')
