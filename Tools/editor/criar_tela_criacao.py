"""Tela de criação de personagem no estilo do Zenkai (30/10/2026). Cria as peças reutilizáveis:
- /Game/HypeWorld/UI/CharacterSelect/Components/WBP_OptionCard   (filho de UHypeOptionCardWidget)
- /Game/HypeWorld/UI/CharacterSelect/Components/WBP_ColorRow      (filho de UHypeColorRowWidget)
- /Game/HypeWorld/UI/CharacterSelect/Components/WBP_GradientSlider (filho de UHypeGradientSliderWidget)
Tudo é montado em C++; os WBP só guardam os ajustes em Class Defaults (seção Criacao). Também recompila o
WBP_CharacterSelect (a tela em si). Pode rodar de novo: só cria o que falta.
Rodar: python3 -I Tools/ue_exec.py -f Tools/editor/criar_tela_criacao.py
"""
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
PASTA = "/Game/HypeWorld/UI/CharacterSelect/Components"

for nome, classe in (("WBP_OptionCard", unreal.HypeOptionCardWidget),
                     ("WBP_ColorRow", unreal.HypeColorRowWidget),
                     ("WBP_GradientSlider", unreal.HypeGradientSliderWidget)):
    caminho = f"{PASTA}/{nome}"
    if not EAL.does_asset_exist(caminho):
        factory = unreal.WidgetBlueprintFactory()
        factory.set_editor_property("parent_class", classe)
        tools.create_asset(nome, PASTA, unreal.WidgetBlueprint, factory)
    wbp = unreal.load_asset(caminho)
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    print("salvo", caminho, EAL.save_asset(caminho, only_if_is_dirty=False))

TELA = "/Game/HypeWorld/UI/CharacterSelect/WBP_CharacterSelect"
if EAL.does_asset_exist(TELA):
    wbp = unreal.load_asset(TELA)
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    cdo = unreal.get_default_object(wbp.generated_class())
    print("tela:", TELA, "| mapa do jogo:", cdo.get_editor_property("mapa_do_jogo"),
          "| distância da câmera:", cdo.get_editor_property("camera_distance"))
    print("salvo", TELA, EAL.save_asset(TELA, only_if_is_dirty=False))
