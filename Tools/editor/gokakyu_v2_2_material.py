"""Gōkakyū v2 (08/11), passo 2: material mestre M_FireMaster + instâncias (skill anime-fireball-vfx, seção 4).

Unlit + Aditivo, dois lados, para sprites/malhas/fitas do Niagara e para as malhas do projétil (C++).
- Distorção: ruído rolando desloca a UV da textura principal.
- Erosão (Rayane): textura principal MENOS ruído rolando; o limiar sobe quando o Niagara baixa o alfa da cor da
  partícula (ScaleColor) -> nascer/viver/sumir controlado pelo Niagara. Nas malhas do C++ o alfa é 1 (sem erosão).
- Cor: rampa por calor; o canal R da cor da partícula mistura a rampa fria com a quente (Kirill).
- Fresnel: contorno laranja-escuro (#E0401A), nunca marrom; forte no núcleo e na esfera da carga, sutil nas cascas.
- Máscaras: cor de vértice (borda preta some), radial (sprites), borda das fitas, Depth Fade, perto da câmera.
- Flipbook: T_Katon_Alongada_Dados 8x8 (A = densidade = forma, R = temperatura = calor), com mistura entre quadros.
- WPO: crescer a partir do centro da partícula pela idade (com overshoot) e respirar (ruído leve na normal).
Paleta da skill (sRGB -> linear). Brilho do miolo 8 (VoidFX); corpo fica ~2 para o laranja não virar creme.
Pode rodar de novo (recria os nós do material; as instâncias são atualizadas).
"""
import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = "/Game/HypeWorld/VFX/Katon/Materiais"
RUIDO_A = "/Game/HypeWorld/Jutsus/Katon/VFX/Gokakyu/Texturas/T_Gokakyu_Ruido_A"
RUIDO_B = "/Game/HypeWorld/Jutsus/Katon/VFX/Gokakyu/Texturas/T_Gokakyu_Ruido_B"
FLIPBOOK = "/Game/HypeWorld/Jutsus/Katon/VFX/Flipbooks/T_Katon_Alongada_Dados"

path = f"{FOLDER}/M_FireMaster"
mat = unreal.load_asset(path)
if mat:
    MEL.delete_all_material_expressions(mat)
else:
    mat = tools.create_asset("M_FireMaster", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)
for flag in ("used_with_niagara_sprites", "used_with_niagara_ribbons", "used_with_niagara_mesh_particles",
             "used_with_static_lighting"):
    try:
        mat.set_editor_property(flag, flag != "used_with_static_lighting")
    except Exception as e:
        print("flag", flag, e)
Y = [-600]


def node(cls, x=-1100):
    Y[0] += 64
    return MEL.create_material_expression(mat, cls, x, Y[0])


def scalar(name, value):
    n = node(unreal.MaterialExpressionScalarParameter)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", value)
    return n


def vector(name, value):
    n = node(unreal.MaterialExpressionVectorParameter)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", unreal.LinearColor(*value))
    return n


def texobj(name, asset):
    n = node(unreal.MaterialExpressionTextureObjectParameter)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("texture", unreal.load_asset(asset))
    return n


def custom(code, inputs, out_type, x=-350, y=0):
    c = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, x, y)
    c.set_editor_property("code", code)
    c.set_editor_property("output_type", out_type)
    pins = []
    for name, _ in inputs:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", name)
        pins.append(ci)
    c.set_editor_property("inputs", pins)
    for name, n in inputs:
        assert MEL.connect_material_expressions(n, "", c, name), name
    return c


# flipbook: amostra do quadro atual + próximo misturados (SubUV do Niagara, SubImageSize 8x8 no renderer)
fb = node(unreal.MaterialExpressionTextureSampleParameterSubUV)
fb.set_editor_property("parameter_name", "Flipbook")
fb.set_editor_property("texture", unreal.load_asset(FLIPBOOK))
fb.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
fb.set_editor_property("blend", True)
fb_rgba = node(unreal.MaterialExpressionAppendVector)
MEL.connect_material_expressions(fb, "RGB", fb_rgba, "A")
MEL.connect_material_expressions(fb, "A", fb_rgba, "B")

df = node(unreal.MaterialExpressionDepthFade)
df.set_editor_property("fade_distance_default", 60.0)

CORES = [("CMiolo", "Cor_Miolo", (1.0, 0.905, 0.539, 1)),          # #FFF4C2
         ("CMioloQ", "Cor_MioloQuente", (1.0, 0.658, 0.063, 1)),   # #FFD447
         ("CCorpo", "Cor_Corpo", (1.0, 0.434, 0.040, 1)),          # #FFB038
         ("CCorpoQ", "Cor_CorpoQuente", (1.0, 0.254, 0.014, 1)),   # #FF8A1F
         ("CCorpoB", "Cor_CorpoBorda", (1.0, 0.195, 0.013, 1)),    # #FF7A1E
         ("CBorda", "Cor_Borda", (1.0, 0.069, 0.010, 1)),          # #FF4A1A
         ("CFresnel", "Cor_Fresnel", (0.745, 0.052, 0.010, 1))]    # #E0401A
ESCALARES = [("Brilho", 8.0), ("Calor", 1.0), ("Escala", 1.0), ("EscalaV", 1.0), ("PanU", 0.0), ("PanV", 0.35),
             ("Distorcao", 0.12), ("ErosaoRuido", 0.5), ("ErosaoSuave", 0.12), ("Limiar", 0.05),
             ("FresnelForca", 0.0), ("FresnelExp", 2.5), ("FresnelSome", 0.0), ("UsarVC", 1.0), ("UsarFB", 0.0),
             ("MascaraRadial", 0.0), ("FormaRadial", 0.0), ("FadeBordaV", 0.0), ("Esfriar", 0.0),
             ("SomePerto", 0.0), ("Seed", 0.0)]
# cor da partícula: a saída padrão é só RGB; o alfa (erosão) entra junto
pcol = node(unreal.MaterialExpressionParticleColor)
pc_rgba = node(unreal.MaterialExpressionAppendVector)
MEL.connect_material_expressions(pcol, "", pc_rgba, "A")
MEL.connect_material_expressions(pcol, "A", pc_rgba, "B")
inputs = [("UV", node(unreal.MaterialExpressionTextureCoordinate)),
          ("VC", node(unreal.MaterialExpressionVertexColor)),
          ("PC", pc_rgba),
          ("AGE", node(unreal.MaterialExpressionParticleRelativeTime)),
          ("RND", node(unreal.MaterialExpressionParticleRandom)),
          ("N", node(unreal.MaterialExpressionVertexNormalWS)),
          ("V", node(unreal.MaterialExpressionCameraVectorWS)),
          ("T", node(unreal.MaterialExpressionTime)),
          ("PD", node(unreal.MaterialExpressionPixelDepth)),
          ("DF", df), ("FB", fb_rgba),
          ("Tex", texobj("Textura", RUIDO_A)), ("Rui", texobj("Ruido", RUIDO_B)),
          ("Tint", vector("Tint", (1, 1, 1, 1)))]
inputs += [(k, vector(n, v)) for k, n, v in CORES]
inputs += [(n, scalar(n, v)) for n, v in ESCALARES]

CODE = r"""
float t = T;
float2 uv = UV * float2(Escala, Escala * EscalaV) + Seed * float2(0.371, 0.913) + RND * 3.17;
// distorção: ruído rolando desloca a UV da textura principal
float2 d = (Texture2DSample(Rui, RuiSampler, uv * 0.6 + float2(t * 0.05, t * PanV * 0.5)).rg - 0.5) * Distorcao;
float base = Texture2DSample(Tex, TexSampler, uv + d + float2(t * PanU, t * PanV)).r;
float2 q = (UV - 0.5) * 2.0;
float r = length(q);
base = lerp(base, saturate(1.0 - r) * (0.7 + 0.6 * base), FormaRadial);
float calorFB = 0.0;
if (UsarFB > 0.5) { base = saturate(FB.a * 1.25) * (0.75 + 0.5 * base); calorFB = saturate(FB.r * 0.3); }
// erosão (Rayane): textura menos ruído; o limiar sobe quando o alfa da partícula cai
float dn = Texture2DSample(Rui, RuiSampler, uv * 1.37 + 0.37 + float2(-t * 0.07, t * PanV * 1.3)).r;
float s = saturate(base * (1.0 + ErosaoRuido) - dn * ErosaoRuido);
float er = Limiar + (1.0 - saturate(PC.a)) * (1.0 - Limiar);
float a = smoothstep(er, er + ErosaoSuave, s);
// fresnel: contorno laranja-escuro
float ndv = saturate(abs(dot(normalize(N), normalize(V))));
float rim = pow(1.0 - ndv, FresnelExp) * FresnelForca;
// calor -> rampa (quente e fria), misturadas pelo canal R da cor da partícula
float h = saturate((s * 0.8 + 0.25) * Calor + calorFB - rim * 0.7 - AGE * Esfriar);
float3 quente = lerp(CBorda.rgb, CCorpoQ.rgb, smoothstep(0.05, 0.35, h));
quente = lerp(quente, CCorpo.rgb, smoothstep(0.3, 0.6, h));
quente = lerp(quente, CMioloQ.rgb, smoothstep(0.58, 0.8, h));
quente = lerp(quente, CMiolo.rgb, smoothstep(0.78, 0.97, h));
float3 frio = lerp(CBorda.rgb * 0.7, CCorpoB.rgb, smoothstep(0.1, 0.8, h));
float3 col = lerp(frio, quente, saturate(PC.r));
col = lerp(col, CFresnel.rgb, saturate(rim));
float ganho = lerp(1.5, Brilho, pow(h, 3.0));
// máscaras
a *= lerp(1.0, VC.r, UsarVC);
a *= lerp(1.0, 1.0 - smoothstep(0.55, 0.95, r), MascaraRadial);
a *= lerp(1.0, sin(saturate(UV.y) * 3.14159), FadeBordaV);
a *= 1.0 - FresnelSome * pow(1.0 - ndv, 5.0);
a *= DF;
if (SomePerto > 0.0) { a *= saturate((PD - 30.0) / SomePerto); }
return col * ganho * Tint.rgb * saturate(a);
"""
c = custom(CODE, inputs, unreal.CustomMaterialOutputType.CMOT_FLOAT3)
MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# WPO: crescer a partir do centro da partícula (idade normalizada, com overshoot) + respirar
wins = [("WP", node(unreal.MaterialExpressionWorldPosition, -800)),
        ("UV", node(unreal.MaterialExpressionTextureCoordinate, -800)),
        ("Rui", texobj("RuidoWPO", RUIDO_B)), ("Seed", scalar("SeedWPO", 0.0)),
        ("PP", node(unreal.MaterialExpressionParticlePositionWS, -800)),
        ("AGE", node(unreal.MaterialExpressionParticleRelativeTime, -800)),
        ("N", node(unreal.MaterialExpressionVertexNormalWS, -800)),
        ("T", node(unreal.MaterialExpressionTime, -800))]
wins += [(n, scalar(n, v)) for n, v in (("CrescerLiga", 0.0), ("Cresce0", 1.0), ("Cresce1", 1.0),
                                        ("CresceTempo", 1.0), ("CresceOvershoot", 0.0), ("Respirar", 0.0),
                                        ("Deformar", 0.0), ("Ondular", 0.0))]
WCODE = r"""
float x = saturate(AGE / max(CresceTempo, 0.01)) - 1.0;
float k = CresceOvershoot;
float e = 1.0 + (k + 1.0) * x * x * x + k * x * x;      // ease out back (k = 0: ease out cúbico)
float g = lerp(Cresce0, Cresce1, e);
float3 off = (WP - PP) * (g - 1.0) * CrescerLiga;
off += N * Respirar * sin(T * 2.2 + dot(WP, float3(0.031, 0.027, 0.023)));
// deformação forte por ruído (cm): o núcleo nunca fica redondo; o padrão rola com o tempo
if (Deformar > 0.0) {
    float2 nuv = float2(dot(N, float3(0.71, 0.0, 0.70)), dot(N, float3(0.0, 0.8, 0.6))) * 0.45 + Seed * 0.13;
    float n1 = Texture2DSampleLevel(Rui, RuiSampler, nuv + float2(T * 0.07, T * 0.11), 0).r;
    float n2 = Texture2DSampleLevel(Rui, RuiSampler, nuv * 2.3 - float2(T * 0.13, T * 0.05), 0).g;
    off += N * Deformar * ((n1 - 0.5) * 1.6 + (n2 - 0.5) * 0.8);
}
// ondular (línguas em malha): onda senoidal + ruído, amplitude crescendo da base (V=0) até a ponta (V=1)
if (Ondular > 0.0) {
    float wv = sin(T * 5.0 + UV.y * 6.0 + Seed * 3.0) + 0.5 * sin(T * 8.3 - UV.y * 9.0 + Seed);
    float nz = Texture2DSampleLevel(Rui, RuiSampler, float2(UV.y * 0.7 + Seed * 0.31, T * 0.21), 0).r - 0.5;
    off += N * Ondular * UV.y * (wv + nz * 1.5);
}
return off;
"""
w = custom(WCODE, wins, unreal.CustomMaterialOutputType.CMOT_FLOAT3, -350, 500)
MEL.connect_material_property(w, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
MEL.recompile_material(mat)
EAL.save_loaded_asset(mat, False)
print("OK", path)


def mi(name, scalars, parent=None):
    p = f"{FOLDER}/{name}"
    inst = unreal.load_asset(p) or tools.create_asset(name, FOLDER, unreal.MaterialInstanceConstant,
                                                      unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(inst, parent or mat)
    for k, v in scalars.items():
        MEL.set_material_instance_scalar_parameter_value(inst, k, v)
    MEL.update_material_instance(inst)
    EAL.save_loaded_asset(inst, False)
    print("OK", p)


# núcleo (malha do C++): fresnel FORTE, sem erosão, manchas quentes rolando devagar, respira
mi("MI_Katon_Nucleo", {"FresnelForca": 1.0, "FresnelExp": 2.0, "ErosaoRuido": 0.0, "Limiar": -1.0, "Calor": 0.6,
                       "Escala": 1.6, "PanU": 0.05, "PanV": 0.18, "Distorcao": 0.18, "Brilho": 3.0, "Respirar": 1.5,
                       "Deformar": 22.0})
# cascas (malhas do C++): fresnel sutil, erosão forte (vazada), rolando mais rápido
mi("MI_Katon_Casca", {"FresnelForca": 0.3, "FresnelSome": 0.8, "ErosaoRuido": 0.7, "Limiar": 0.35,
                      "ErosaoSuave": 0.18, "Calor": 0.45, "Escala": 2.0, "PanV": 0.7, "Brilho": 1.2})
# fitas (malhas do Niagara, U ao longo da fita): textura correndo ao longo, borda pela cor de vértice
mi("MI_Katon_Fita", {"FresnelForca": 0.15, "ErosaoRuido": 0.7, "Limiar": 0.22, "Calor": 0.5, "Escala": 2.0,
                     "EscalaV": 0.5, "PanU": -1.0, "PanV": 0.0, "Brilho": 3.5})
mi("MI_Katon_FitaImpacto", {"FresnelForca": 0.15, "ErosaoRuido": 0.6, "Limiar": 0.12, "Calor": 0.85, "Escala": 2.0,
                            "EscalaV": 0.5, "PanU": -1.6, "PanV": 0.0, "Brilho": 3.5, "CrescerLiga": 1.0,
                            "Cresce0": 1.0, "Cresce1": 1.35, "CresceTempo": 0.8})
mi("MI_Katon_FitaLiberacao", {"FresnelForca": 0.15, "ErosaoRuido": 0.5, "Limiar": 0.1, "Calor": 1.0, "Escala": 2.0,
                              "EscalaV": 0.5, "PanU": -2.0, "PanV": 0.0, "Brilho": 4.0, "CrescerLiga": 1.0,
                              "Cresce0": 1.35, "Cresce1": 0.55, "CresceTempo": 1.0})
# línguas de fogo em MALHA (SM_Katon_Linguas, cascas do C++): chama rolando da base para a ponta, borda pela cor de
# vértice, ondulando (WPO) e deformada
mi("MI_Katon_LinguaMalha", {"UsarVC": 1.0, "PanU": 0.0, "PanV": -1.2, "Escala": 1.5, "EscalaV": 0.6, "ErosaoRuido": 0.65,
                            "Limiar": 0.12, "Calor": 0.5, "Brilho": 2.0, "FresnelForca": 0.2, "FresnelSome": 0.6,
                            "Ondular": 35.0, "Deformar": 8.0})
# línguas (sprites com o flipbook nosso 8x8)
# labaredas da superfície da bola: muitas sobrepostas -> um pouco menos intensas cada uma (somadas ficavam creme);
# mesma paleta, mais erosão para a borda quebrar
mi("MI_Katon_Labareda", {"UsarFB": 1.0, "MascaraRadial": 1.0, "UsarVC": 0.0, "ErosaoRuido": 0.45, "Limiar": 0.1,
                         "Calor": 0.45, "Brilho": 1.8, "SomePerto": 200.0, "PanV": 0.2})
mi("MI_Katon_Lingua", {"UsarFB": 1.0, "MascaraRadial": 1.0, "UsarVC": 0.0, "ErosaoRuido": 0.35, "Limiar": 0.08,
                       "Calor": 0.7, "Brilho": 3.5, "SomePerto": 200.0, "PanV": 0.2})
# cauda (fita do Niagara): borda da largura some, esfria com a idade
# 08/11: cauda com forma procedural (os últimos quadros do flipbook já são quase transparentes e a cauda sumia a ~3 m)
mi("MI_Katon_Cauda", {"UsarVC": 0.0, "UsarFB": 0.0, "FormaRadial": 1.0, "MascaraRadial": 1.0, "FadeBordaV": 0.0,
                      "ErosaoRuido": 0.5, "Limiar": 0.0, "Calor": 0.65, "Esfriar": 0.25, "PanU": 0.0, "PanV": 0.4,
                      "Escala": 1.2, "Brilho": 5.0})
# brasas e ignição: pontinhos redondos muito brilhantes
mi("MI_Katon_Brasa", {"FormaRadial": 1.0, "MascaraRadial": 1.0, "ErosaoRuido": 0.0, "Limiar": -1.0, "Calor": 1.3,
                      "UsarVC": 0.0, "Brilho": 14.0, "Distorcao": 0.0})
mi("MI_Katon_Ignicao", {"FormaRadial": 1.0, "MascaraRadial": 1.0, "ErosaoRuido": 0.0, "Limiar": -1.0, "Calor": 1.6,
                        "UsarVC": 0.0, "Brilho": 20.0, "Distorcao": 0.0})
# esfera da carga: fresnel forte, cresce na frente da boca com overshoot (~15%)
mi("MI_Katon_CargaEsfera", {"FresnelForca": 1.0, "FresnelExp": 2.0, "ErosaoRuido": 0.3, "Limiar": 0.0, "Calor": 0.8,
                            "Brilho": 3.5, "CrescerLiga": 1.0, "Cresce0": 0.15, "Cresce1": 1.0, "CresceTempo": 0.7,
                            "CresceOvershoot": 2.3, "PanV": 0.5, "Escala": 1.5})
# anel da carga: encolhe para o centro
mi("MI_Katon_Anel", {"CrescerLiga": 1.0, "Cresce0": 2.2, "Cresce1": 0.25, "CresceTempo": 1.0, "Calor": 0.85,
                     "ErosaoRuido": 0.4, "Limiar": 0.05, "PanU": 0.6, "PanV": 0.0, "Brilho": 3.5})
# impacto: onda de choque abrindo e esfera estourando
mi("MI_Katon_Onda", {"CrescerLiga": 1.0, "Cresce0": 0.3, "Cresce1": 3.2, "CresceTempo": 0.7, "ErosaoRuido": 0.6,
                     "Limiar": 0.1, "Calor": 0.85, "PanU": 0.4, "PanV": 0.0, "Brilho": 3.5})
mi("MI_Katon_Estouro", {"FresnelForca": 1.0, "FresnelExp": 2.0, "CrescerLiga": 1.0, "Cresce0": 1.0, "Cresce1": 2.2,
                        "CresceTempo": 0.8, "ErosaoRuido": 0.5, "Limiar": 0.05, "Calor": 0.8, "Brilho": 4.0,
                        "PanV": 0.8, "Escala": 1.5})
# halo e clarões: sprite aditivo grande e suave do próprio mestre (forma radial que chega a 0 antes da borda do
# quadrado; o M_FireHalo antigo mostrava a borda reta do sprite)
HALO = {"FormaRadial": 1.0, "MascaraRadial": 1.0, "ErosaoRuido": 0.0, "Limiar": -1.0, "UsarVC": 0.0,
        "Distorcao": 0.0, "SomePerto": 300.0}
mi("MI_Katon_Halo", dict(HALO, Calor=0.45, Brilho=0.3))
mi("MI_Katon_HaloFlash", dict(HALO, Calor=0.95, Brilho=3.0))
print("GOKAKYU_V2_MATERIAL_OK")
