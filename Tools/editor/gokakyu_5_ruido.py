"""Gōkakyū (07/11): ruídos próprios de ALTA RESOLUÇÃO (1024 px, repetíveis) para o fogo do jato.

Os ruídos do pack usados até aqui (T_VFX_Noise_01/08) têm 256x256 (G8, grupo World): ampliados nas chamas grandes
(e ainda esticados 2,2x) davam cara de "PNG de baixa qualidade". Aqui um material procedural (fBm de value noise
periódico, 6 oitavas) é desenhado num render target de 1024 e salvo como textura estática:
  T_Gokakyu_Ruido_A  cinza: nuvens/línguas (Grayscale, sem gama)
  T_Gokakyu_Ruido_B  RGB: R e G independentes = distorção de UV, B = detalhe fino (BC7, sem gama)
Grupo de textura Effects, sem limite de tamanho nem LOD bias. Pode rodar de novo (refaz). Editor aberto.
"""
import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = "/Game/HypeWorld/Jutsus/Katon/VFX/Gokakyu/Texturas"
N = 1024

# ---------------------------------------------------------------- material gerador
gpath = f"{FOLDER}/M_Gokakyu_GeraRuido"
gm = unreal.load_asset(gpath)
if gm:
    MEL.delete_all_material_expressions(gm)
else:
    gm = tools.create_asset("M_Gokakyu_GeraRuido", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
gm.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
uv = MEL.create_material_expression(gm, unreal.MaterialExpressionTextureCoordinate, -800, 0)
modo = MEL.create_material_expression(gm, unreal.MaterialExpressionScalarParameter, -800, 100)
modo.set_editor_property("parameter_name", "Modo")
CODE = r"""
float3 o = 0;
for (int ch = 0; ch < 3; ch++) {
    // A (Modo 0): mesmo fBm nos 3 canais. B: R/G baixa frequência (distorção), B = detalhe fino
    float base = (Modo < 0.5) ? 6.0 : ((ch < 2) ? 4.0 : 16.0);
    int oct = (Modo < 0.5) ? 6 : ((ch < 2) ? 4 : 4);
    float sh = (Modo < 0.5) ? 0.0 : (ch * 37.0 + 11.0);
    float sum = 0.0, amp = 0.5, norm = 0.0, per = base;
    for (int k = 0; k < 8; k++) {
        if (k >= oct) break;
        float2 x = UV * per;
        float2 i = floor(x);
        float2 f = x - i;
        float2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
        float2 i0 = fmod(i, per), i1 = fmod(i + 1.0, per);
        float a = frac(sin(dot(float2(i0.x, i0.y) + sh + k * 7.13, float2(127.1, 311.7))) * 43758.5453);
        float b = frac(sin(dot(float2(i1.x, i0.y) + sh + k * 7.13, float2(127.1, 311.7))) * 43758.5453);
        float c = frac(sin(dot(float2(i0.x, i1.y) + sh + k * 7.13, float2(127.1, 311.7))) * 43758.5453);
        float d = frac(sin(dot(float2(i1.x, i1.y) + sh + k * 7.13, float2(127.1, 311.7))) * 43758.5453);
        sum += amp * lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
        norm += amp; amp *= 0.5; per *= 2.0;
    }
    float v = sum / norm;
    v = saturate((v - 0.5) * 1.9 + 0.5);   // contraste: usa a faixa toda
    if (Modo < 0.5) { o = v; break; }
    o[ch] = v;
}
return o;
"""
c = MEL.create_material_expression(gm, unreal.MaterialExpressionCustom, -400, 0)
c.set_editor_property("code", CODE)
c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
pins = []
for n in ("UV", "Modo"):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    pins.append(ci)
c.set_editor_property("inputs", pins)
MEL.connect_material_expressions(uv, "", c, "UV")
MEL.connect_material_expressions(modo, "", c, "Modo")
MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(gm)
EAL.save_loaded_asset(gm, False)

# ---------------------------------------------------------------- desenha e salva
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def gerar(nome, modo_valor, compressao):
    rtp = f"{FOLDER}/RT_{nome}"
    rt = unreal.load_asset(rtp)
    if not rt:
        f = unreal.TextureRenderTargetFactoryNew()
        rt = tools.create_asset(f"RT_{nome}", FOLDER, unreal.TextureRenderTarget2D, f)
    rt.set_editor_property("size_x", N)
    rt.set_editor_property("size_y", N)
    rt.set_editor_property("render_target_format", unreal.TextureRenderTargetFormat.RTF_RGBA8)
    rt.set_editor_property("srgb", False)
    mip = f"{FOLDER}/MI_GeraRuido_{nome[-1]}"
    mid = unreal.load_asset(mip) or tools.create_asset(f"MI_GeraRuido_{nome[-1]}", FOLDER,
                                                       unreal.MaterialInstanceConstant,
                                                       unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mid, gm)
    MEL.set_material_instance_scalar_parameter_value(mid, "Modo", modo_valor)
    MEL.update_material_instance(mid)
    EAL.save_loaded_asset(mid, False)
    unreal.RenderingLibrary.clear_render_target2d(world, rt, unreal.LinearColor(0, 0, 0, 1))
    unreal.RenderingLibrary.draw_material_to_render_target(world, rt, mid)
    tp = f"{FOLDER}/{nome}"
    if EAL.does_asset_exist(tp):
        EAL.delete_asset(tp)   # textura gerada por este script: refaz
    tex = unreal.RenderingLibrary.render_target_create_static_texture2d_editor_only(
        rt, nome, compressao, unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
    assert tex, nome
    tex.set_editor_property("srgb", False)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_EFFECTS)
    tex.set_editor_property("compression_settings", compressao)
    tex.set_editor_property("max_texture_size", 0)
    tex.set_editor_property("lod_bias", 0)
    tex.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP)
    tex.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)
    EAL.save_loaded_asset(tex, False)
    print("OK", tex.get_path_name(), tex.blueprint_get_size_x(), "x", tex.blueprint_get_size_y(),
          tex.get_editor_property("compression_settings"))


gerar("T_Gokakyu_Ruido_A", 0.0, unreal.TextureCompressionSettings.TC_GRAYSCALE)
gerar("T_Gokakyu_Ruido_B", 1.0, unreal.TextureCompressionSettings.TC_BC7)
print("GOKAKYU_RUIDO_OK")
