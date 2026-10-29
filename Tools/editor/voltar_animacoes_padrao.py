"""Volta o personagem às animações padrão da Epic (template) e desliga todos os retoques por código.
Nada é apagado: as animações do Mixamo/DeepMotion e as montagens próprias continuam no projeto, só desligadas.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/voltar_animacoes_padrao.py
"""
import re
import unreal

MM = "/Game/Characters/Mannequins/Anims/Unarmed/"
NINJA = "/Game/Ninja/Characters/Animations/"
ABP = NINJA + "ABP_Ninja"
save = []

# --- ABP_Ninja: nós de animação de volta às do template ---
BASE = ABP + ".ABP_Ninja:AnimGraph."
NODES = {
    "AnimGraphNode_StateMachine_0.Locomotion.AnimStateNode_1.Idle.AnimGraphNode_SequencePlayer_1": ("Sequence", MM + "MM_Idle"),
    "AnimGraphNode_StateMachine_0.Locomotion.AnimStateNode_2.Walk / Run.AnimGraphNode_BlendSpacePlayer_0": ("BlendSpace", MM + "BS_Idle_Walk_Run"),
    "AnimGraphNode_StateMachine_1.Main States.AnimStateNode_1.Jump.AnimGraphNode_SequencePlayer_1": ("Sequence", MM + "Jump/MM_Jump"),
    "AnimGraphNode_StateMachine_1.Main States.AnimStateNode_2.Fall Loop.AnimGraphNode_SequencePlayer_3": ("Sequence", MM + "Jump/MM_Fall_Loop"),
    "AnimGraphNode_StateMachine_1.Main States.AnimStateNode_3.Land.AnimGraphNode_SequencePlayer_4": ("Sequence", MM + "Jump/MM_Land"),
}
for path, (field, asset_path) in NODES.items():
    asset = unreal.load_asset(asset_path)
    assert asset, asset_path
    graph_node = unreal.find_object(None, BASE + path)
    node = graph_node.get_editor_property("node")
    cls = "AnimSequence" if field == "Sequence" else "BlendSpace"
    new_text, n = re.subn(field + r'="[^"]*"', '%s="/Script/Engine.%s\'%s\'"' % (field, cls, asset.get_path_name()), node.export_text(), count=1)
    assert n == 1, path
    node.import_text(new_text)
    graph_node.set_editor_property("node", node)
    print("ABP:", path.split(".")[-2], "->", asset.get_name())

abp = unreal.load_asset(ABP)
abp_cdo = unreal.get_default_object(abp.generated_class())
abp_cdo.modify()
abp_cdo.set_editor_property("procedural_pose", False)   # nenhum retoque por código
unreal.BlueprintEditorLibrary.compile_blueprint(abp)
save.append(abp.get_outermost())
print("ABP: retoques por código =", abp_cdo.get_editor_property("procedural_pose"))


def epic_only(cdo, is_player):
    cdo.modify()
    m = cdo.get_editor_property("character_movement")
    # Dash: animação e montagem padrão (MM_Dash), valores de antes da estrela
    m.set_editor_property("dash_root_motion_anim", unreal.load_asset(MM + "Jump/MM_Dash") if is_player else None)
    m.set_editor_property("dash_anim_start", 0.0)
    m.set_editor_property("dash_anim_rate", 1.2)
    m.set_editor_property("dash_anim_move_end", 0.75)
    m.set_editor_property("dash_distance_scale", 0.7)
    m.set_editor_property("dash_min_speed_fraction", 0.0)
    if is_player:
        cdo.set_editor_property("dash_montage", unreal.load_asset(NINJA + "AM_Ninja_Dash"))
    cdo.set_editor_property("idle_break_montage", None)            # era do Mixamo
    # Reações só da Epic
    cdo.set_editor_property("hit_react_montages", [unreal.load_asset(NINJA + "AM_Ninja_HitReact_%d" % i) for i in (1, 2)])
    cdo.set_editor_property("heavy_hit_react_montage", unreal.load_asset(NINJA + "AM_Ninja_HitReact_3"))
    # Golpes: o combo leve e o soco carregado já são da Epic; chutes do Mixamo saem
    heavy = list(cdo.get_editor_property("heavy_attacks"))[:1]
    assert str(heavy[0].get_editor_property("name")) == "Soco carregado"
    cdo.set_editor_property("heavy_attacks", heavy)
    cdo.set_editor_property("air_attack", unreal.NinjaAttack())     # sem golpe no ar
    # Sem retoques no corpo inteiro: inclinação nas curvas/parede, aceleração da animação, acrobacias
    cdo.set_editor_property("max_turn_lean", 0.0)
    cdo.set_editor_property("wall_run_lean", 0.0)
    cdo.set_editor_property("max_run_anim_rate", 1.0)
    cdo.set_editor_property("dash_flip_degrees", 0.0)
    cdo.set_editor_property("jump_spin_degrees", 0.0)


for path, is_player in (("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter", True), ("/Game/Ninja/Characters/BP_NinjaTrainingDummy", False)):
    bp = unreal.load_asset(path)
    cdo = unreal.get_default_object(bp.generated_class())
    epic_only(cdo, is_player)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    save.append(bp.get_outermost())

print("salvo:", unreal.EditorLoadingAndSavingUtils.save_packages(save, False))
for path in ("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter", "/Game/Ninja/Characters/BP_NinjaTrainingDummy"):
    cdo = unreal.get_default_object(unreal.load_asset(path).generated_class())
    dm = cdo.get_editor_property("dash_montage")
    print(path.split("/")[-1], "| dash:", dm.get_name() if dm else None,
          "| leves:", [str(a.get_editor_property("name")) for a in cdo.get_editor_property("light_combo")],
          "| fortes:", [str(a.get_editor_property("name")) for a in cdo.get_editor_property("heavy_attacks")],
          "| reações:", [x.get_name() for x in cdo.get_editor_property("hit_react_montages")], cdo.get_editor_property("heavy_hit_react_montage").get_name(),
          "| inclinação:", cdo.get_editor_property("max_turn_lean"))
