"""Importa o retrato e os ícones de poder (recortados do mockup) para /Game/Ninja/UI/Textures.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/importar_texturas_hud.py
"""
import unreal

SRC = "/Users/luisfelipexavier/Desktop/Hype World/Referencias/HUD"
DEST = "/Game/Ninja/UI/Textures"
NAMES = ["T_HUD_Retrato", "T_HUD_Poder_Q", "T_HUD_Poder_E", "T_HUD_Poder_R", "T_HUD_Poder_T"]

tasks = []
for name in NAMES:
    t = unreal.AssetImportTask()
    t.filename = f"{SRC}/{name}.png"
    t.destination_path = DEST
    t.destination_name = name
    t.automated = True
    t.replace_existing = True
    t.save = False
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for name in NAMES:
    path = f"{DEST}/{name}"
    tex = unreal.load_asset(path)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)  # "UserInterface2D"
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    print("OK", path, tex.blueprint_get_size_x(), "x", tex.blueprint_get_size_y())
