import unreal,json,shutil
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/AnimacaoRestauro/CorrecaoShisui')
OUT.mkdir(exist_ok=True)
ROOT=OUT.parents[2]
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
al=unreal.EditorAssetLibrary
for relative in ['Content/Ninja/Characters/Animations/ABP_Ninja.uasset','Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset']:
    dest=OUT/'BackupAntes'/relative
    if not dest.exists():
        dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/relative,dest)
abp=al.load_asset('/Game/Ninja/Characters/Animations/ABP_Ninja')
defaults=unreal.get_default_object(abp.generated_class());defaults.modify()
# Empty override selects ActiveBattleIdle, the animated battle pose of the selected style.
defaults.set_editor_property('combat_ready_pose',None)
defaults.set_editor_property('combat_run_center_pose',None)
unreal.BlueprintEditorLibrary.compile_blueprint(abp)
assert al.save_loaded_asset(abp)
bp=al.load_asset('/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter')
body_templates=[c for c in unreal.ObjectIterator(unreal.SkeletalMeshComponent) if c.get_path_name().startswith(bp.get_path_name()) and c.get_editor_property('skeletal_mesh_asset') and c.get_editor_property('skeletal_mesh_asset').get_name()=='SK_Shisui']
assert body_templates,'Shisui component not found'
for body in body_templates:
    body.modify();body.set_visibility(True,False);body.set_hidden_in_game(False,False)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
main=unreal.get_default_object(bp.generated_class()).get_editor_property('mesh');main.modify()
main.set_visibility(False,False);main.set_hidden_in_game(True,False)
main.set_editor_property('visibility_based_anim_tick_option',unreal.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
assert al.save_loaded_asset(bp)
assert unreal.get_default_object(abp.generated_class()).get_editor_property('combat_ready_pose') is None
report={'upper_body_source':'ActiveBattleIdle / animated pose from the selected fighting style, including combat trot','static_ready_override':None,'visible_body':'SK_Shisui','hidden_driver':'SKM_Quinn_Simple','driver_tick':'ALWAYS_TICK_POSE_AND_REFRESH_BONES','body_templates':[c.get_path_name() for c in body_templates]}
(OUT/'configuration.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
