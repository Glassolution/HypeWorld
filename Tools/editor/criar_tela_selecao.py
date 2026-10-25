"""Tela de seleção do personagem (vila e clã) do Hype World (25/10/2026). Cria:
- /Game/HypeWorld/UI/CharacterSelect/WBP_CharacterSelect (filho de UHypeCharacterSelectWidget, montado em C++;
  os ajustes — vilas, clãs, ícones, posições, câmera — ficam em Class Defaults, seção Selecao)
A tela abre sozinha depois da tela de carregamento (WBP_LoadingScreen → TelaAoEntrar). Sem este WBP o jogo usa a
classe C++ direto (mesmo visual). Pode rodar de novo: só cria se não existir, e recompila.
Rodar: python3 -I Tools/ue_exec.py -f Tools/editor/criar_tela_selecao.py
"""
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
PASTA = "/Game/HypeWorld/UI/CharacterSelect"
WBP = f"{PASTA}/WBP_CharacterSelect"

if not EAL.does_asset_exist(WBP):
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.HypeCharacterSelectWidget)
    tools.create_asset("WBP_CharacterSelect", PASTA, unreal.WidgetBlueprint, factory)
wbp = unreal.load_asset(WBP)
unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
cdo = unreal.get_default_object(wbp.generated_class())
print("WBP:", WBP, "| vilas:", len(cdo.get_editor_property("villages")))
print("salvo", WBP, EAL.save_asset(WBP, only_if_is_dirty=False))
