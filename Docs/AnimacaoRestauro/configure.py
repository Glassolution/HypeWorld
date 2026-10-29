import unreal,json
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/AnimacaoRestauro')
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
al=unreal.EditorAssetLibrary
abp=al.load_asset('/Game/Ninja/Characters/Animations/ABP_Ninja')
cdo=unreal.get_default_object(abp.generated_class());cdo.modify()
cdo.set_editor_property('combat_ready_pose',al.load_asset('/Game/HypeWorld/Animacoes/A_Pose_Prontidao'))
unreal.BlueprintEditorLibrary.compile_blueprint(abp);assert al.save_loaded_asset(abp)
bp=al.load_asset('/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter')
components=[x for x in unreal.ObjectIterator(unreal.SkeletalMeshComponent) if x.get_path_name().startswith(bp.get_path_name()) and x.get_editor_property('skeletal_mesh_asset')]
result=[]
for c in components:
    c.modify();base=c.get_editor_property('skeletal_mesh_asset').get_name()=='SKM_Quinn_Simple'
    c.set_visibility(base,False);c.set_hidden_in_game(not base,False)
    result.append(dict(name=c.get_name(),visible=base,mesh=c.get_editor_property('skeletal_mesh_asset').get_path_name()))
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
main=unreal.get_default_object(bp.generated_class()).get_editor_property('mesh');main.modify()
main.set_visibility(True,False);main.set_hidden_in_game(False,False)
assert al.save_loaded_asset(bp)
result.append(dict(name=main.get_name(),visible=True,mesh=main.get_editor_property('skeletal_mesh_asset').get_path_name()))
move=unreal.get_default_object(bp.generated_class()).get_editor_property('character_movement')
report={'ready':unreal.get_default_object(abp.generated_class()).get_editor_property('combat_ready_pose').get_path_name(),'components':result,'movement':{p:move.get_editor_property(p) for p in ['run_speed','run_back_speed','run_strafe_speed','sprint_speed','strafe_max_angle']}}
(OUT/'configured.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
