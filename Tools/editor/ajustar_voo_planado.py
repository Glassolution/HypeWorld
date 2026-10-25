"""Voo dos saltos especiais (pedido do Felipe: "como catapulta no espaço" / mortal "mais alto, levitando").
- Super pulo: sai empurrado num arco baixo e longo, gravidade fraca o voo todo (SuperJumpGravity 0,35 subindo e descendo).
- Mortal para trás: sobe mais alto e fica flutuando no topo (BackflipGravity 0,55 subindo, 0,15 no topo, queda máx. 550).
Gravidade = × a do personagem (GravityScale 2). Rodar com o C++ novo compilado."""
import unreal

bp = "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"
cdo = unreal.get_default_object(unreal.load_object(None, bp + ".BP_ThirdPersonCharacter_C"))
cdo.set_editor_property("super_jump_up_speed", unreal.Vector2D(650.0, 1000.0))
cdo.set_editor_property("super_jump_forward_speed", unreal.Vector2D(700.0, 1300.0))
cdo.set_editor_property("super_jump_gravity", unreal.Vector(0.35, 0.35, 900.0))
cdo.set_editor_property("backflip_up_speed", 1300.0)
cdo.set_editor_property("backflip_back_speed", 240.0)
cdo.set_editor_property("backflip_gravity", unreal.Vector(0.55, 0.15, 550.0))
unreal.EditorAssetLibrary.save_asset(bp, only_if_is_dirty=False)
for k in ["super_jump_up_speed", "super_jump_forward_speed", "super_jump_gravity", "backflip_up_speed", "backflip_back_speed", "backflip_gravity"]:
    print(k, cdo.get_editor_property(k))
