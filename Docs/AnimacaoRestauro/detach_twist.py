import unreal,json
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/AnimacaoRestauro')
assert (OUT/'baseline_check.json').exists()
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
bp=unreal.load_asset('/Game/HypeWorld/Personagens/Shisui/ABP_Shisui')
unreal.BlueprintEditorLibrary.reparent_blueprint(bp,unreal.AnimInstance)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert not isinstance(unreal.get_default_object(bp.generated_class()),unreal.NinjaBodyAnimInstance)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
(OUT/'twist_detached.json').write_text(json.dumps({'blueprint':bp.get_path_name(),'parent':'AnimInstance'}))
unreal.SystemLibrary.quit_editor()
