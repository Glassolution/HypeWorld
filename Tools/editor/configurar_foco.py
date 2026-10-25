"""Tecla do modo foco (segurar botão direito / gatilho esquerdo) e ligação no personagem.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/configurar_foco.py
"""
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
path = "/Game/Ninja/Core/Input/IA_Focus"
ia = unreal.load_asset(path) or tools.create_asset("IA_Focus", "/Game/Ninja/Core/Input", unreal.InputAction, unreal.InputAction_Factory())
ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
IMC = unreal.load_asset("/Game/Input/IMC_Default")
IMC.modify()
have = {(m.action.get_name() if m.action else None, str(m.key.get_editor_property("key_name"))) for m in IMC.get_editor_property("default_key_mappings").get_editor_property("mappings")}
for k in ("RightMouseButton", "Gamepad_LeftTrigger"):
    if ("IA_Focus", k) not in have:
        key = unreal.Key(); key.set_editor_property("key_name", k); IMC.map_key(ia, key)
pk = [ia.get_outermost(), IMC.get_outermost()]
bp = unreal.load_asset("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter")
cdo = unreal.get_default_object(bp.generated_class()); cdo.modify()
cdo.set_editor_property("focus_action", ia)
unreal.BlueprintEditorLibrary.compile_blueprint(bp); pk.append(bp.get_outermost())
print("salvo:", unreal.EditorLoadingAndSavingUtils.save_packages(pk, False), "| foco:", cdo.get_editor_property("focus_action").get_name())
