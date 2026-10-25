"""Cria o WBP_NinjaHUD (filho do C++ UNinjaHUDWidget), põe retrato e ícones e liga no controle do jogador.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/criar_hud.py
"""
import unreal

WBP = "/Game/Ninja/UI/WBP_NinjaHUD"
TEX = "/Game/Ninja/UI/Textures/"
PC = "/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController"

if not unreal.EditorAssetLibrary.does_asset_exist(WBP):
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.NinjaHUDWidget)
    unreal.AssetToolsHelpers.get_asset_tools().create_asset("WBP_NinjaHUD", "/Game/Ninja/UI", unreal.WidgetBlueprint, factory)

wbp = unreal.load_asset(WBP)
cdo = unreal.get_default_object(wbp.generated_class())
cdo.modify()
cdo.set_editor_property("portrait_texture", unreal.load_asset(TEX + "T_HUD_Retrato"))
cdo.set_editor_property("ability_icons", [unreal.load_asset(TEX + n) for n in ("T_HUD_Poder_Q", "T_HUD_Poder_E", "T_HUD_Poder_R", "T_HUD_Poder_T")])
unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
print("WBP salvo:", unreal.EditorAssetLibrary.save_asset(WBP, only_if_is_dirty=False))

pc = unreal.load_asset(PC)
pc_cdo = unreal.get_default_object(pc.generated_class())
pc_cdo.modify()
pc_cdo.set_editor_property("hud_widget_class", wbp.generated_class())
unreal.BlueprintEditorLibrary.compile_blueprint(pc)
print("Controle salvo:", unreal.EditorAssetLibrary.save_asset(PC, only_if_is_dirty=False))
print("HUD do controle:", unreal.get_default_object(pc.generated_class()).get_editor_property("hud_widget_class").get_name())
