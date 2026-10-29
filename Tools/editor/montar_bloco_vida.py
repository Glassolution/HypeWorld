"""Bloco de vida com as imagens do Felipe: importa a fonte Montserrat (ExtraBold/Bold/SemiBold) e as peças recortadas
(Tools/hud/recortar_bloco_vida.py) e liga tudo no WBP_NinjaHUD. Rodar com ue_exec -f, editor fora do Play.
"""
import unreal

DESK = "/Users/luisfelipexavier/Desktop/Hype World"
FONTES, PECAS = DESK + "/Fontes/Montserrat", DESK + "/Referencias/HUD/Bloco_Vida_PNG/pecas"
DST_FONTE, DST_TEX = "/Game/Ninja/UI/Fontes", "/Game/Ninja/UI/HUD/BlocoVida"
tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def importar(arquivo, destino, nome):
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", arquivo)
    t.set_editor_property("destination_path", destino)
    t.set_editor_property("destination_name", nome)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    tools.import_asset_tasks([t])
    caminhos = list(t.get_editor_property("imported_object_paths"))
    return [unreal.load_asset(p) for p in caminhos]


# ---------------- Fonte ----------------
faces = []
for face in ("ExtraBold", "Bold", "SemiBold"):
    objs = importar("%s/Montserrat-%s.ttf" % (FONTES, face), DST_FONTE, "FF_Montserrat_" + face)
    ff = next((o for o in objs if isinstance(o, unreal.FontFace)), None)
    print("face", face, [o.get_class().get_name() + ":" + o.get_name() for o in objs])
    ff.set_editor_property("loading_policy", unreal.FontLoadingPolicy.LAZY_LOAD)
    faces.append(ff)
    eal.save_loaded_asset(ff, False)
fonte = unreal.load_asset(DST_FONTE + "/F_Montserrat")
if not fonte:
    fonte = tools.create_asset("F_Montserrat", DST_FONTE, unreal.Font, unreal.FontFactory())
print("fonte composta:", unreal.NinjaEditorTools.build_composite_font(fonte, ["ExtraBold", "Bold", "SemiBold"], faces))
eal.save_loaded_asset(fonte, False)

# ---------------- Peças (texturas de interface, sem mipmap, sRGB) ----------------
tex = {}
for nome in ("Base", "Selo", "Vida_Cheia", "Vida_Vazia", "Chakra_Normal_Cheia", "Chakra_Normal_Vazia", "Chakra_Batalha_Cheia",
             "Chakra_Batalha_Vazia", "Moeda", "Icone1", "Icone2"):
    t = next(o for o in importar("%s/%s.png" % (PECAS, nome), DST_TEX, "T_BV_" + nome) if isinstance(o, unreal.Texture2D))
    t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    t.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    t.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    t.set_editor_property("srgb", True)
    eal.save_loaded_asset(t, False)
    tex[nome] = t

# ---------------- WBP_NinjaHUD ----------------
wbp = unreal.load_asset("/Game/Ninja/UI/WBP_NinjaHUD")
cdo = unreal.get_default_object(wbp.generated_class())
cdo.modify()
cdo.set_editor_property("hud_font", fonte)
for prop, nome in (("life_base_texture", "Base"), ("level_badge_texture", "Selo"), ("health_full_texture", "Vida_Cheia"),
                   ("health_empty_texture", "Vida_Vazia"), ("chakra_full_texture", "Chakra_Normal_Cheia"),
                   ("chakra_empty_texture", "Chakra_Normal_Vazia"), ("chakra_battle_full_texture", "Chakra_Batalha_Cheia"),
                   ("chakra_battle_empty_texture", "Chakra_Batalha_Vazia"), ("coin_icon_texture", "Moeda")):
    cdo.set_editor_property(prop, tex[nome])
cdo.set_editor_property("battle_icon_textures", [tex["Icone1"], tex["Icone2"]])
unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
print("WBP salvo:", eal.save_loaded_asset(wbp, False))
cdo = unreal.get_default_object(unreal.load_asset("/Game/Ninja/UI/WBP_NinjaHUD").generated_class())
print("fonte no HUD:", cdo.get_editor_property("hud_font").get_name(), "| base:", cdo.get_editor_property("life_base_texture").get_name(),
      "| escala:", cdo.get_editor_property("life_block_scale"), "| posição:", cdo.get_editor_property("life_block_position"))
