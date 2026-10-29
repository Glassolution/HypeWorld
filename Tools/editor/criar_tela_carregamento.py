"""Tela de carregamento do Hype World (25/10/2026). Cria:
- /Game/HypeWorld/UI/Textures/T_Logo_HypeWorld (SÓ se ainda não existir: importa a cópia recortada com transparência
  Referencias/UI/Logo/T_Logo_HypeWorld.png, feita por Tools/ui/recortar_logo.py a partir da imagem do Felipe)
- /Game/HypeWorld/UI/Loading/M_UI_LoadingLogo (logo + revelar com borda de pincel + brilho; aceita logo transparente
  OU letras brancas em fundo escuro: a transparência sai também da claridade)
- /Game/HypeWorld/UI/Loading/M_UI_LoadingBackground (papel escuro quase invisível + vinheta + pontinhos de chakra subindo)
- /Game/HypeWorld/UI/Loading/M_UI_LoadingGlow (brilho redondo da ponta da barra)
- /Game/HypeWorld/UI/Loading/WBP_LoadingScreen (filho de UHypeLoadingScreenWidget, montado em C++)
- /Game/Maps/Boot/L_Boot (mapa vazio, GameMode = AHypeBootGameMode)
Rodar: python3 -I Tools/ue_exec.py -f Tools/editor/criar_tela_carregamento.py
"""
import unreal

MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
LOADING = "/Game/HypeWorld/UI/Loading"
LOGO = "/Game/HypeWorld/UI/Textures/T_Logo_HypeWorld"
LOGO_PNG = "/Users/luisfelipexavier/Desktop/Hype World/Referencias/UI/Logo/T_Logo_HypeWorld.png"
BOOT = "/Game/Maps/Boot/L_Boot"
saved = []

# --- Logo ---------------------------------------------------------------------------------------------------------------
if not EAL.does_asset_exist(LOGO):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", LOGO_PNG)
    task.set_editor_property("destination_path", "/Game/HypeWorld/UI/Textures")
    task.set_editor_property("destination_name", "T_Logo_HypeWorld")
    task.set_editor_property("automated", True)
    task.set_editor_property("save", False)
    tools.import_asset_tasks([task])
    print("Logo importada da cópia:", LOGO_PNG)
else:
    print("Logo já existe (não mexi):", LOGO)
logo = unreal.load_asset(LOGO)
if logo and not logo.get_editor_property("asset_import_data").get_first_filename().endswith("Referencias/UI/Logo/T_Logo_HypeWorld.png"):
    print("  (logo do Felipe: configurações dela não foram alteradas)")
elif logo:
    logo.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    logo.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)   # encolher sem serrilhar
    logo.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    logo.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
    logo.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    logo.set_editor_property("srgb", True)
    saved.append(LOGO)
print("Logo:", logo.blueprint_get_size_x() if logo else None, "x", logo.blueprint_get_size_y() if logo else None)


# --- Materiais (um nó Custom com o código HLSL em cada) -----------------------------------------------------------------
def material(name):
    path = f"{LOADING}/{name}"
    mat = unreal.load_asset(path)
    if mat:
        MEL.delete_all_material_expressions(mat)
    else:
        mat = tools.create_asset(name, LOADING, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    return mat, path


def custom(mat, code, inputs):
    """inputs: lista de (nome, nó). Saída float4: RGB -> emissivo, A -> opacidade."""
    c = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -400, 0)
    c.set_editor_property("code", code)
    c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    pins = []
    for name, _ in inputs:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", name)
        pins.append(ci)
    c.set_editor_property("inputs", pins)
    for name, node in inputs:
        if not MEL.connect_material_expressions(node, "", c, name):
            print("  FALHA ao ligar", name)
    rgb = MEL.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -150, -60)
    rgb.set_editor_property("r", True); rgb.set_editor_property("g", True); rgb.set_editor_property("b", True)
    alpha = MEL.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -150, 60)
    # o nó já nasce com R ligado: sem desligar, a "opacidade" vira (R, A) e a Unreal usa o R (= 1, quadrado branco)
    alpha.set_editor_property("r", False); alpha.set_editor_property("g", False); alpha.set_editor_property("b", False)
    alpha.set_editor_property("a", True)
    MEL.connect_material_expressions(c, "", rgb, "")
    MEL.connect_material_expressions(c, "", alpha, "")
    MEL.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)


def scalar(mat, name, value, y):
    n = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -800, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", value)
    return n


def uv(mat):
    return MEL.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -800, -200)


def finish(mat, path):
    MEL.delete_unused_expressions(mat)   # sobras de uma rodada anterior quebravam a compilação
    MEL.recompile_material(mat)
    saved.append(path)
    print("OK", path, "nós:", MEL.get_num_material_expressions(mat))


KEY = "smoothstep(0.30, 0.60, dot({s}.rgb, float3(0.299, 0.587, 0.114))) * {s}.a"

LOGO_CODE = """
float4 c = Texture2DSample(Tex, TexSampler, UV);
float letters = KEY_C;
// revelar da esquerda para a direita; a borda tem "fios" de pincel (cada linha um pouco adiantada ou atrasada)
float row = floor(UV.y * 90.0);
float bristle = frac(sin(row * 12.9898) * 43758.5453);
float wave = sin(UV.y * 9.0) * 0.5 + 0.5;
float edge = lerp(-0.25, 1.35, Reveal);
float x = UV.x + bristle * 0.06 + wave * 0.05;
float shown = 1.0 - smoothstep(edge - 0.10, edge, x);
// brilho: média de amostras borradas (mips) em volta das letras
float g = 0.0;
for (int i = 0; i < 12; i++)
{
    float a = i * 0.5235988;
    float2 d = float2(cos(a), sin(a) * TexAspect) * Radius;
    float4 s1 = Texture2DSampleLevel(Tex, TexSampler, UV + d, 3.0);
    float4 s2 = Texture2DSampleLevel(Tex, TexSampler, UV + d * 2.0, 4.0);
    g += (KEY_S1) * 0.6 + (KEY_S2) * 0.4;
}
g = saturate(g / 12.0 * 1.8);
float halo = g * Glow * (1.0 - letters) * shown;
return float4(1.0, 1.0, 1.0, saturate(letters * shown + halo));
""".replace("KEY_C", KEY.format(s="c")).replace("KEY_S1", KEY.format(s="s1")).replace("KEY_S2", KEY.format(s="s2"))

BACKGROUND_CODE = """
struct FNoise
{
    float h(float2 p) { return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453); }
    float vn(float2 p)
    {
        float2 i = floor(p);
        float2 f = frac(p);
        f = f * f * (3.0 - 2.0 * f);
        return lerp(lerp(h(i), h(i + float2(1, 0)), f.x), lerp(h(i + float2(0, 1)), h(i + float2(1, 1)), f.x), f.y);
    }
};
FNoise n;
float2 p = float2(UV.x * Aspect, UV.y);
// papel/concreto escuro: manchas grandes + grão fino, quase preto
float paper = n.vn(p * 3.0) * 0.5 + n.vn(p * 9.0) * 0.3 + n.vn(p * 28.0) * 0.2;
float grain = n.h(floor(p * 720.0));
float base = 0.0012 + paper * 0.0032 + grain * 0.0009;
// vinheta
float d = length((UV - 0.5) * float2(Aspect, 1.0));
float vig = 1.0 - smoothstep(0.35, 1.05, d) * 0.8;
// pontinhos de chakra subindo devagar (3 camadas: longe = menores e mais lentos)
float3 parts = float3(0, 0, 0);
for (int L = 0; L < 3; L++)
{
    float s = 16.0 - L * 5.0;
    float spd = 0.02 + L * 0.012;
    float2 q = float2(p.x, UV.y + Time * spd) * s + L * 17.0;
    float2 cell = floor(q);
    float2 f = frac(q);
    float r = n.h(cell + 3.1 * L);
    if (r > 0.6)
    {
        float2 c = float2(0.2 + 0.6 * n.h(cell + 5.7), 0.2 + 0.6 * n.h(cell + 9.3));
        c.x += 0.15 * sin(Time * 0.6 + r * 40.0);
        float size = 0.03 + 0.035 * n.h(cell + 1.3);
        float dotv = smoothstep(size, 0.0, length(f - c));
        float tw = 0.55 + 0.45 * sin(Time * (0.8 + r) + r * 60.0);
        float depth = 0.45 + 0.3 * L;
        parts += dotv * tw * depth * lerp(float3(1.0, 1.0, 1.0), float3(0.45, 0.7, 1.0), n.h(cell + 7.7));
    }
}
parts *= smoothstep(0.0, 0.45, UV.y) * Particles;
return float4((base * vig + parts) * Fade, 1.0);
"""

GLOW_CODE = """
float r = length(UV - 0.5) * 2.0;
float a = pow(saturate(1.0 - r), 2.4);
return float4(1.0, 1.0, 1.0, a);
"""

# Logo
mat, path = material("M_UI_LoadingLogo")
tex = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureObjectParameter, -800, 0)
tex.set_editor_property("parameter_name", "Logo")
tex.set_editor_property("texture", logo or unreal.load_asset("/Engine/EngineResources/DefaultTexture"))
tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
def logo_size():
    """Tamanho da logo. Logo depois de abrir o editor a textura ainda está sendo preparada e diz 32x32: aí lê do PNG."""
    w, h = logo.blueprint_get_size_x(), logo.blueprint_get_size_y()
    if w > 64:
        return w, h
    import struct
    with open(logo.get_editor_property("asset_import_data").get_first_filename(), "rb") as f:
        head = f.read(24)
    return struct.unpack(">II", head[16:24])


aspect = (lambda s: s[0] / s[1])(logo_size()) if logo else 1.8
print("Proporção da logo:", aspect)
custom(mat, LOGO_CODE, [("UV", uv(mat)), ("Tex", tex), ("Reveal", scalar(mat, "Reveal", 1.0, 120)),
                        ("Glow", scalar(mat, "Glow", 0.0, 240)), ("Radius", scalar(mat, "Radius", 0.012, 360)),
                        ("TexAspect", scalar(mat, "TexAspect", aspect, 480))])
finish(mat, path)

# Fundo
mat, path = material("M_UI_LoadingBackground")
custom(mat, BACKGROUND_CODE, [("UV", uv(mat)), ("Time", scalar(mat, "Time", 0.0, 0)), ("Aspect", scalar(mat, "Aspect", 1.7778, 120)),
                              ("Fade", scalar(mat, "Fade", 1.0, 240)), ("Particles", scalar(mat, "Particles", 0.32, 360))])
finish(mat, path)

# Brilho da ponta da barra
mat, path = material("M_UI_LoadingGlow")
custom(mat, GLOW_CODE, [("UV", uv(mat))])
finish(mat, path)

# --- Widget -------------------------------------------------------------------------------------------------------------
WBP = f"{LOADING}/WBP_LoadingScreen"
if not EAL.does_asset_exist(WBP):
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.HypeLoadingScreenWidget)
    tools.create_asset("WBP_LoadingScreen", LOADING, unreal.WidgetBlueprint, factory)
wbp = unreal.load_asset(WBP)
unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
saved.append(WBP)
cdo = unreal.get_default_object(wbp.generated_class())
print("WBP:", WBP, "| MapaSeguinte:", cdo.get_editor_property("mapa_seguinte"))

for p in saved:
    print("salvo", p, EAL.save_asset(p, only_if_is_dirty=False))

# --- Mapa de boot -------------------------------------------------------------------------------------------------------
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not EAL.does_asset_exist(BOOT):
    print("novo mapa:", les.new_level(BOOT, False))
else:
    les.load_level(BOOT)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
ws = world.get_world_settings()
ws.set_editor_property("default_game_mode", unreal.HypeBootGameMode)
print("L_Boot GameMode:", ws.get_editor_property("default_game_mode"), "| atores:", len(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()))
print("L_Boot salvo:", les.save_current_level())
les.load_level("/Game/Ninja/Maps/Lvl_Treino")
