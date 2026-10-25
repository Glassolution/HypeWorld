"""Slots de jutsu com a imagem do Felipe (25/10/2026): importa os 6 recortes (Tools/hud/recortar_slots_jutsu.py) em
/Game/Ninja/UI/HUD/Jutsu/Slots/T_JS_Slot1..6 (UI, sRGB, com mipmaps: 256x256 encolhido para ~80 px não serrilha),
liga no WBP_NinjaHUD (JutsuSlotData: arte, cor temática, número) e diminui o bloco de vida.
SÓ_TEXTURAS = True antes do código importa só as texturas (para criar o material antes de compilar o C++ novo).
Rodar com ue_exec -f, editor fora do Play.
"""
import unreal

PECAS = "/Users/luisfelipexavier/Desktop/Hype World/Referencias/HUD/Slots_Jutsu_PNG/pecas"
DST = "/Game/Ninja/UI/HUD/Jutsu/Slots"
tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
try:
    SÓ_TEXTURAS
except NameError:
    SÓ_TEXTURAS = False

# Cor temática (brilho de seleção) e número de exemplo da imagem, na ordem da imagem
CORES = [(63, 169, 255), (123, 224, 122), (140, 120, 255), (255, 138, 43), (63, 169, 255), (255, 42, 42)]
NUMEROS = [5, 5, 5, 172, 150, 0]

texturas = []
for i in range(6):
    nome = "T_JS_Slot%d" % (i + 1)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", "%s/%s.png" % (PECAS, nome))
    t.set_editor_property("destination_path", DST)
    t.set_editor_property("destination_name", nome)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    tools.import_asset_tasks([t])
    tex = unreal.load_asset("%s/%s" % (DST, nome))
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    tex.set_editor_property("srgb", True)
    eal.save_loaded_asset(tex, False)
    texturas.append(tex)
print("texturas:", [t.get_name() for t in texturas])

if not SÓ_TEXTURAS:
    bp = unreal.load_asset("/Game/Ninja/UI/WBP_NinjaHUD")
    cdo = unreal.get_default_object(bp.generated_class())
    dados = []
    for tex, cor, num in zip(texturas, CORES, NUMEROS):
        d = unreal.NinjaJutsuSlotData()
        d.set_editor_property("icon", tex)
        d.set_editor_property("theme_color", unreal.LinearColor(*[pow(v / 255.0, 2.2) for v in cor], 1.0))
        d.set_editor_property("number", num)
        dados.append(d)
    cdo.set_editor_property("jutsu_slot_data", dados)
    cdo.set_editor_property("jutsu_slot_size", 50.0)        # diâmetro do círculo da arte (px em 1080p)
    cdo.set_editor_property("jutsu_slot_gap", 24.0)         # centro a centro = 74 px (mesmo espaçamento da imagem)
    cdo.set_editor_property("jutsu_hidden_offset", 110.0)   # fora da batalha só a ponta da moldura aparece
    cdo.set_editor_property("life_block_scale", 0.18)       # bloco de vida menor (era 0,245)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, False)
    print("WBP:", len(cdo.get_editor_property("jutsu_slot_data")), "slots | vida escala", cdo.get_editor_property("life_block_scale"))
