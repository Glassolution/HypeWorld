"""Cria IA_Sprint e IA_Dash em /Game/Ninja/Core/Input e mapeia as teclas no IMC_Default do template.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/criar_inputs_movimento.py
"""
import unreal

FOLDER = "/Game/Ninja/Core/Input"
IMC = unreal.load_asset("/Game/Input/IMC_Default")
tools = unreal.AssetToolsHelpers.get_asset_tools()

# Teclas: corrida ninja (segurar) e dash (apertar)
KEYS = {
    "IA_Sprint": ["LeftShift", "Gamepad_LeftThumbstick"],
    "IA_Dash": ["Q", "Gamepad_FaceButton_Right"],
}


def make_key(name):
    k = unreal.Key()
    k.set_editor_property("key_name", name)
    return k


existing = {(m.action.get_name() if m.action else None, str(m.key.get_editor_property("key_name")))
            for m in IMC.get_editor_property("default_key_mappings").get_editor_property("mappings")}

IMC.modify()  # marca o IMC como alterado; sem isso o salvamento é ignorado
for ia_name, keys in KEYS.items():
    path = f"{FOLDER}/{ia_name}"
    ia = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
        tools.create_asset(ia_name, FOLDER, unreal.InputAction, unreal.InputAction_Factory())
    ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    unreal.EditorAssetLibrary.save_loaded_asset(ia)
    for key in keys:
        if (ia_name, key) not in existing:
            IMC.map_key(ia, make_key(key))
    print("OK", path, keys)

unreal.EditorAssetLibrary.save_asset(IMC.get_path_name(), only_if_is_dirty=False)
for m in IMC.get_editor_property("default_key_mappings").get_editor_property("mappings"):
    if m.action and m.action.get_name() in KEYS:
        print("  mapeado:", m.action.get_name(), m.key.get_editor_property("key_name"))
