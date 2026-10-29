"""Trote e corrida ninja mais responsivos (25/10/2026, pedido do Felipe): trote = Mixamo Run2 (frente/lados do mesmo conjunto),
5,2 m/s (lado 4,3); corrida ninja vira mais rápido (curva ~280°/s, corpo 1000°/s).
Só mexe no blend space de locomoção e nas velocidades/curvas — o resto do aplicar_pack_anime_ninja fica como está.
Rodar com ue_exec -f, editor fora do Play.
"""
import unreal

MX = "/Game/Ninja/Characters/Animations/Mixamo/A_Ninja_MX_Avulsa_"
WALK_SPEED, RUN_SPEED, RUN_STRAFE_SPEED, SPRINT_SPEED = 150.0, 520.0, 430.0, 800.0
bs = unreal.load_asset("/Game/Ninja/Characters/Animations/ANAP/BS_ANAP_Locomocao")
fwd, left, right = (unreal.load_asset(MX + n) for n in ("Run2_Forward", "Run2_Left", "Run2_Right"))
samples = []
for s in bs.get_editor_property("sample_data"):
    a = s.get_editor_property("animation"); v = s.get_editor_property("sample_value")
    name = a.get_name() if a else ""
    if name.endswith("Run_Type01_Loop") or name.endswith("Run2_Forward"):
        s.set_editor_property("animation", fwd); v.y = RUN_SPEED
    elif name.endswith("Run2_Left") or name.endswith("Run2_Right"):
        v.y = RUN_STRAFE_SPEED
    s.set_editor_property("sample_value", v)
    samples.append(s)
bs.set_editor_property("sample_data", samples)
print("blend space:", unreal.NinjaEditorTools.rebuild_blend_space(bs))
unreal.EditorAssetLibrary.save_loaded_asset(bs, False)

for path, is_player in (("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter", True), ("/Game/Ninja/Characters/BP_NinjaTrainingDummy", False)):
    bp = unreal.load_asset(path)
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.modify()
    m = cdo.get_editor_property("character_movement")
    m.set_editor_property("run_speed", RUN_SPEED)
    m.set_editor_property("run_strafe_speed", RUN_STRAFE_SPEED)
    cdo.set_editor_property("anim_speed_curve", [unreal.Vector2D(WALK_SPEED, 75.0), unreal.Vector2D(RUN_SPEED, 560.0), unreal.Vector2D(SPRINT_SPEED, 491.0)])
    if is_player:
        cdo.set_editor_property("sprint_steer_angle", 30.0)
        cdo.set_editor_property("sprint_rotation_rate", 1000.0)
        cdo.set_editor_property("sprint_camera_follow_speed", 8.0)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
cdo = unreal.get_default_object(unreal.load_asset("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter").generated_class())
m = cdo.get_editor_property("character_movement")
print("trote", m.get_editor_property("run_speed"), "lado", m.get_editor_property("run_strafe_speed"), "curva", cdo.get_editor_property("sprint_steer_angle"),
      "giro", cdo.get_editor_property("sprint_rotation_rate"))
for s in bs.get_editor_property("sample_data"):
    v = s.get_editor_property("sample_value")
    if v.y >= 400: print("  ", s.get_editor_property("animation").get_name(), round(v.x), round(v.y))
