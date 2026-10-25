"""Cria as montagens de movimento a partir de animações que já vêm com o manequim.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/criar_montagens_movimento.py
"""
import unreal

FOLDER = "/Game/Ninja/Characters/Animations"
ANIMS = "/Game/Characters/Mannequins/Anims"
SKELETON = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SK_Mannequin")

MONTAGES = {
    "AM_Ninja_Dash": f"{ANIMS}/Unarmed/Jump/MM_Dash",
    "AM_Ninja_WallJump": f"{ANIMS}/Unarmed/Jump/MM_WallJump",
    "AM_Ninja_WallRun": f"{ANIMS}/Unarmed/Jog/MF_Unarmed_Jog_Fwd",  # corrida comum; o C++ inclina o boneco e repete em loop
}

tools = unreal.AssetToolsHelpers.get_asset_tools()
for name, source in MONTAGES.items():
    path = f"{FOLDER}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        print("já existe:", path)
        continue
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("source_animation", unreal.load_asset(source))
    factory.set_editor_property("target_skeleton", SKELETON)
    montage = tools.create_asset(name, FOLDER, unreal.AnimMontage, factory)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    slots = [str(t.get_editor_property("slot_name")) for t in montage.get_editor_property("slot_anim_tracks")]
    print("OK", path, "duração %.2fs" % montage.get_play_length(), "slot", slots)
