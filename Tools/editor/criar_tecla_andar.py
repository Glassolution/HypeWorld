# Alt (IA_Walk): liga/desliga o andar. O padrão do personagem é trotar; Shift 1x = corrida ninja.
import unreal
tools = unreal.AssetToolsHelpers.get_asset_tools()
ia = unreal.load_asset("/Game/Ninja/Core/Input/IA_Walk") or tools.create_asset("IA_Walk", "/Game/Ninja/Core/Input", unreal.InputAction, unreal.InputAction_Factory())
ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
IMC = unreal.load_asset("/Game/Input/IMC_Default")
IMC.modify()
used = {str(m.key.get_editor_property("key_name")): (m.action.get_name() if m.action else None)
        for m in IMC.get_editor_property("default_key_mappings").get_editor_property("mappings")}
for nome in ("LeftAlt", "RightAlt"):
    if used.get(nome) is None:
        k = unreal.Key(); k.set_editor_property("key_name", nome); IMC.map_key(ia, k)
    elif used.get(nome) != "IA_Walk":
        print("AVISO:", nome, "já usada por", used[nome])
bp = unreal.load_asset("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter")
unreal.get_default_object(bp.generated_class()).set_editor_property("walk_toggle_action", ia)
for a in ("/Game/Ninja/Core/Input/IA_Walk", "/Game/Input/IMC_Default", "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"):
    unreal.EditorAssetLibrary.save_asset(a, only_if_is_dirty=False)
print("tecla andar ok", used.get("LeftAlt"), used.get("RightAlt"))
