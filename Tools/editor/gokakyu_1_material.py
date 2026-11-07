"""Gōkakyū novo (07/11, especificação do Felipe): material de fogo anime M_FireToon + instâncias.

Um material só para todas as camadas (núcleo, cascas, chamas soltas, rastro), escolhida pelo parâmetro Modo:
  0 = núcleo (esfera): centro branco -> amarelo -> laranja na borda da silhueta (fresnel)
  1 = casca (esfera maior): só a borda da silhueta pega fogo (laranja/vermelho), o meio fica aberto mostrando o núcleo
  2 = sprite (chama solta): radial a partir do centro do quadrado
  3 = fita/rastro (ribbon): forte no começo (U=0, junto da bola) e afinando para o fim e para as laterais
Tudo Masked + Unlit: contorno NÍTIDO (corte por limiar, nada de borda borrada) e brilho que estoura o bloom (HDR).
Faixas de cor por intensidade: branco > amarelo > laranja > vermelho, com uma faixa fina vermelho-escura de contorno.
Ruído do pack (T_VFX_Noise_01/08, só a textura) rolando com distorção de UV = as chamas se mexem.
Erosão: parâmetro Erosao + (1 - Dynamic Parameter X) (o Niagara faz nascer/viver/dissolver; fora do Niagara vale 1)
       + (1 - alfa da cor da partícula). Cascas: deslocamento dos vértices pelo ruído (WPO) = línguas de fogo.
Instâncias: MI_FireToon_Nucleo, MI_FireToon_Casca, MI_FireToon_Chama, MI_FireToon_Rastro.
M_FireSoft (estilo Zenkai, escolhido em 07/11): mesma forma e ruído, Translúcido com brilho forte, rampa de 6 cores
do Felipe (contorno #7A1010, borda #E0281C, corpo #FF5A14 -> #FF8A1F, miolo #FFC93C -> #FFF4C2 só no topo).
Instâncias MI_FireSoft_Nucleo/Casca/Chama/Rastro.
Pode rodar de novo (refaz os nós). Rodar com o editor aberto.
"""
import unreal

MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
FOLDER = "/Game/HypeWorld/Jutsus/Katon/VFX/Gokakyu"
NOISE_A = "/Game/Vefects/Free_Fire/Shared/Textures/T_VFX_Noise_01"
NOISE_B = "/Game/Vefects/Free_Fire/Shared/Textures/T_VFX_Noise_08"

def build(name, soft):
    """soft=False: M_FireToon (contorno nítido, Masked). soft=True: M_FireSoft (estilo Zenkai, aditivo, sem contorno)."""
    global mat
    path = f"{FOLDER}/{name}"
    mat = unreal.load_asset(path)
    if mat:
        MEL.delete_all_material_expressions(mat)
    else:
        mat = tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT if soft else unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("opacity_mask_clip_value", 0.5)
    for flag in ("used_with_niagara_sprites", "used_with_niagara_ribbons", "used_with_niagara_mesh_particles",
                 "used_with_static_lighting"):
        try:
            mat.set_editor_property(flag, flag != "used_with_static_lighting")
        except Exception as e:  # noqa: BLE001
            print("sem flag", flag, e)

    Y = [0]


    def node(cls, x=-900):
        Y[0] += 70
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


    def custom(code, out_type, inputs, x=-350):
        c = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, x, 0)
        c.set_editor_property("code", code)
        c.set_editor_property("output_type", out_type)
        pins = []
        for name, *_ in inputs:
            ci = unreal.CustomInput()
            ci.set_editor_property("input_name", name)
            pins.append(ci)
        c.set_editor_property("inputs", pins)
        for name, n, *out in inputs:
            if not MEL.connect_material_expressions(n, out[0] if out else "", c, name):
                print("  FALHA ao ligar", name)
        return c


    def mask(src, r, g, b, a, x=-150, y=0):
        m = MEL.create_material_expression(mat, unreal.MaterialExpressionComponentMask, x, y)
        for k, v in (("r", r), ("g", g), ("b", b), ("a", a)):
            m.set_editor_property(k, v)
        MEL.connect_material_expressions(src, "", m, "")
        return m


    uv = node(unreal.MaterialExpressionTextureCoordinate)
    time = node(unreal.MaterialExpressionTime)
    cam = node(unreal.MaterialExpressionCameraVectorWS)
    nrm = node(unreal.MaterialExpressionVertexNormalWS)
    dyn = node(unreal.MaterialExpressionDynamicParameter)
    pcol = node(unreal.MaterialExpressionParticleColor)
    vcol = node(unreal.MaterialExpressionVertexColor)
    objpos = node(unreal.MaterialExpressionObjectPositionWS)
    wpos = node(unreal.MaterialExpressionWorldPosition)
    noise_a = texobj("RuidoA", NOISE_A)
    noise_b = texobj("RuidoB", NOISE_B)
    modo = scalar("Modo", 0.0)
    erosao = scalar("Erosao", 0.0)
    contorno = scalar("Contorno", 0.035)
    brilho = scalar("Brilho", 40.0)
    quente = scalar("Calor", 0.0)          # empurra as faixas para o branco (>0) ou para o vermelho (<0)
    escala = scalar("RuidoEscala", 1.0)
    vel = scalar("RuidoVelocidade", 1.0)
    distor = scalar("Distorcao", 0.18)
    semente = scalar("Seed", 0.0)
    fresnel_pot = scalar("FresnelPotencia", 1.6)
    tint = vector("Tint", (1, 1, 1, 1))
    c_branco = vector("CorBranco", (1.0, 0.92, 0.62, 1))
    c_amarelo = vector("CorAmarelo", (1.0, 0.6, 0.07, 1))
    c_laranja = vector("CorLaranja", (1.0, 0.28, 0.02, 1))
    c_vermelho = vector("CorVermelho", (0.9, 0.05, 0.01, 1))
    c_contorno = vector("CorContorno", (0.6, 0.03, 0.01, 1))
    c_laranja_esc = vector("CorLaranjaEscuro", (1.0, 0.102, 0.007, 1))
    faixa_max = scalar("FaixaMax", 1.0)
    suavidade = scalar("Suavidade", 0.35)

    CODE = r"""
    // ---- ruído rolando com distorção (coordenada depende do modo) ----
    float m = Modo;
    float2 nuv = (m > 2.5) ? float2(UV.x * 2.0, UV.y) : UV;
    float t = Time * Vel + Seed * 7.13;
    float2 p = nuv * Esc + Seed * 0.37;
    float2 d2 = Texture2DSample(RB, RBSampler, p * 0.7 + float2(t * 0.11, t * 0.37)).rg - 0.5;
    float n1 = Texture2DSample(RA, RASampler, p + d2 * Dist + float2(-t * 0.07, t * 0.45)).r;
    float n2 = Texture2DSample(RB, RBSampler, p * 1.9 - d2 * Dist * 0.5 + float2(t * 0.21, t * 0.6)).r;
    float n = n1 * 0.65 + n2 * 0.35;
    // ---- forma por modo (v ~ 0 na borda, ~1+ no centro) ----
    float v;
    if (m < 0.5) {            // núcleo: centro forte; a borda da silhueta é recortada pelo ruído
        float f = saturate(dot(normalize(N), normalize(C)));
        v = pow(f, 1.3) * 1.05 + (n - 0.5) * 0.8;
    } else if (m < 1.5) {     // casca: só a borda da silhueta, em línguas
        float f = saturate(dot(normalize(N), normalize(C)));
        float rim = 1.0 - f;
        v = smoothstep(0.14, 0.38, rim) * pow(saturate(1.0 - rim), FresPot) * 0.95 * (0.3 + 1.4 * n);
    } else if (m < 2.5) {     // sprite: radial
        float d = length(UV - 0.5) * 2.0;
        v = pow(saturate(1.0 - d), 0.7) * (0.35 + 1.3 * n);
    } else {                  // fita: forte junto da bola (U=0), afina no fim e nas laterais
        float along = saturate(1.0 - UV.x);
        float across = saturate(1.0 - abs(UV.y - 0.5) * 2.0);
        v = pow(along, 0.8) * pow(across, 0.6) * 1.15 * (0.35 + 1.3 * n);
    }
    v += Calor * 0.25;
    // ---- erosão (nasce / dissolve) ----
    float thr = 0.22 + Ero + (1.0 - Dyn.x) + (1.0 - PA) * 0.9;
    float vis = step(thr, v);
    // ---- faixas de cor (anime): vermelho fora, laranja, amarelo, branco no centro; brilho só estoura no branco ----
    float k = min(saturate((v - thr) / max(1.0 - thr, 0.05)), FMax);   // FaixaMax < 0.76 = sem branco
    float3 col = CRed.rgb; float gain = 0.045;
    float s1 = smoothstep(0.20, 0.24, k), s2 = smoothstep(0.48, 0.52, k), s3 = smoothstep(0.76, 0.80, k);
    col = lerp(col, COra.rgb, s1); gain = lerp(gain, 0.055, s1);
    col = lerp(col, CYel.rgb, s2); gain = lerp(gain, 0.062, s2);
    col = lerp(col, CWhi.rgb, s3); gain = lerp(gain, 1.0, s3);
    float edge = 1.0 - step(Cont, v - thr);
    col = lerp(col, CEdge.rgb, edge); gain = lerp(gain, 0.04, edge);
    float3 e = col * gain * Bri * Tint.rgb * PC.rgb;
    return float4(e, vis);
    """
    # SUAVE (estilo Zenkai, rampa do Felipe 07/11): translúcido com brilho forte; rampa por intensidade
    # miolo #FFF4C2 -> #FFC93C superexposto (só o topo da rampa, ~25% da área), corpo #FF8A1F -> #FF5A14 brilhante
    # mas laranja de verdade (emissivo moderado: alto demais o tonemapper vira branco), borda #E0281C, contorno #7A1010
    CODE_SUAVE = CODE[:CODE.index("// ---- erosão")] + r"""
    float thr = 0.22 + Ero + (1.0 - Dyn.x) + (1.0 - PA) * 0.9;
    float a = smoothstep(thr - 0.01, thr + 0.03 + Suav * 0.15, v);
    float k = min(saturate((v - thr) / max(1.2 - thr, 0.05)), FMax);
    float3 col = CRed.rgb; float g = 0.05;
    float s1 = smoothstep(0.08, 0.16, k), s2 = smoothstep(0.28, 0.36, k), s3 = smoothstep(0.48, 0.56, k), s4 = smoothstep(0.68, 0.76, k);
    col = lerp(col, CDOra.rgb, s1); g = lerp(g, 0.07, s1);
    col = lerp(col, COra.rgb, s2);  g = lerp(g, 0.09, s2);
    col = lerp(col, CYel.rgb, s3);  g = lerp(g, 0.25, s3);
    col = lerp(col, CWhi.rgb, s4);  g = lerp(g, 1.0, s4);
    float edge = 1.0 - smoothstep(Cont * 0.5, Cont, v - thr);
    col = lerp(col, CEdge.rgb, edge); g = lerp(g, 0.03, edge);
    float3 e = col * g * Bri * Tint.rgb * PC.rgb;
    return float4(e, a);
    """
    c = custom(CODE_SUAVE if soft else CODE, unreal.CustomMaterialOutputType.CMOT_FLOAT4, [
        ("UV", uv), ("N", nrm), ("C", cam), ("Time", time), ("Dyn", dyn), ("PC", pcol), ("PA", pcol, "A"),
        ("RA", noise_a), ("RB", noise_b), ("Modo", modo), ("Ero", erosao), ("Cont", contorno), ("Bri", brilho),
        ("Calor", quente), ("Esc", escala), ("Vel", vel), ("Dist", distor), ("Seed", semente), ("FresPot", fresnel_pot),
        ("Tint", tint), ("CWhi", c_branco), ("CYel", c_amarelo), ("COra", c_laranja), ("CRed", c_vermelho),
        ("CEdge", c_contorno), ("FMax", faixa_max), ("Suav", suavidade), ("CDOra", c_laranja_esc)])
    rgb = mask(c, True, True, True, False, y=-60)
    alpha = mask(c, False, False, False, True, y=60)
    MEL.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY if soft else unreal.MaterialProperty.MP_OPACITY_MASK)

    # deslocamento dos vértices (só nas cascas, Deslocar > 0): línguas que se mexem
    deslocar = scalar("Deslocar", 0.0)
    WPO = r"""
    float t = Time * Vel + Seed * 7.13;
    float3 lp = (WP - OP) / max(Raio, 1.0);
    float2 p = float2(lp.x * 0.8 + lp.z * 0.5, lp.y * 0.8 - lp.z * 0.3) * Esc * 0.6 + Seed * 0.37;
    float n = Texture2DSampleLevel(RA, RASampler, p + float2(-t * 0.07, t * 0.45), 0).r;
    float back = saturate(-dot(normalize(lp), Fwd.rgb));   // atrás da bola as línguas ficam maiores
    return N * (n - 0.35) * Desl * (0.6 + 0.8 * back);
    """
    raio = scalar("Raio", 100.0)
    fwd = vector("Frente", (1, 0, 0, 0))
    w = custom(WPO, unreal.CustomMaterialOutputType.CMOT_FLOAT3, [
        ("WP", wpos), ("OP", objpos), ("N", nrm), ("Time", time), ("RA", noise_a), ("Vel", vel), ("Seed", semente),
        ("Esc", escala), ("Desl", deslocar), ("Raio", raio), ("Fwd", fwd)], x=-350)
    MEL.connect_material_property(w, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

    MEL.delete_unused_expressions(mat)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat, False)
    print("OK", path, "nós:", MEL.get_num_material_expressions(mat))
    return mat


toon = build("M_FireToon", False)
soft = build("M_FireSoft", True)


# ---------------------------------------------------------------- instâncias (valores iniciais, ajustáveis)
def mi(name, scalars, vectors=None, parent=None):
    p = f"{FOLDER}/{name}"
    inst = unreal.load_asset(p) or tools.create_asset(name, FOLDER, unreal.MaterialInstanceConstant,
                                                      unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(inst, parent or toon)
    for k, v in scalars.items():
        MEL.set_material_instance_scalar_parameter_value(inst, k, v)
    for k, v in (vectors or {}).items():
        MEL.set_material_instance_vector_parameter_value(inst, k, unreal.LinearColor(*v))
    MEL.update_material_instance(inst)
    EAL.save_loaded_asset(inst, False)
    print("OK", p)


mi("MI_FireToon_Nucleo", {"Modo": 0, "Brilho": 50, "Calor": 0.0, "Erosao": -0.05, "RuidoEscala": 1.2,
                          "RuidoVelocidade": 1.3, "Contorno": 0.03})
mi("MI_FireToon_Casca", {"Modo": 1, "Brilho": 45, "FaixaMax": 0.7, "Calor": 0.0, "Erosao": 0.02, "RuidoEscala": 1.6,
                         "RuidoVelocidade": 1.6, "Deslocar": 40, "FresnelPotencia": 0.7, "Contorno": 0.035})
mi("MI_FireToon_Chama", {"Modo": 2, "Brilho": 45, "FaixaMax": 0.7, "Calor": 0.05, "RuidoEscala": 0.9, "RuidoVelocidade": 1.8,
                         "Contorno": 0.035})
mi("MI_FireToon_Rastro", {"Modo": 3, "Brilho": 40, "FaixaMax": 0.7, "Calor": -0.05, "RuidoEscala": 1.0, "RuidoVelocidade": 2.2,
                          "Contorno": 0.035})
# estilo SUAVE (Zenkai, escolhido pelo Felipe em 07/11): M_FireSoft aditivo, mesmos parâmetros
# Zenkai: miolo branco-amarelo, fora laranja forte puxando para o vermelho (nada de amarelo-bege)
S = dict(parent=soft, vectors={  # cores do Felipe (hex sRGB convertido para linear)
    "CorBranco": (1, 0.906, 0.539, 1),        # #FFF4C2
    "CorAmarelo": (1, 0.584, 0.045, 1),       # #FFC93C
    "CorLaranja": (1, 0.254, 0.0137, 1),      # #FF8A1F
    "CorLaranjaEscuro": (1, 0.102, 0.007, 1), # #FF5A14
    "CorVermelho": (0.745, 0.021, 0.0116, 1), # #E0281C
    "CorContorno": (0.195, 0.0052, 0.0052, 1)})  # #7A1010
mi("MI_FireSoft_Nucleo", {"Modo": 0, "Brilho": 60, "Calor": 0.0, "Erosao": -0.12, "RuidoEscala": 1.2,
                          "RuidoVelocidade": 1.3, "Suavidade": 0.3, "FaixaMax": 1.0, "Contorno": 0.03}, **S)
mi("MI_FireSoft_Casca", {"Modo": 1, "Brilho": 60, "FaixaMax": 0.5, "Calor": 0.0, "Erosao": -0.02, "RuidoEscala": 1.6,
                         "RuidoVelocidade": 1.6, "Deslocar": 40, "FresnelPotencia": 0.7, "Suavidade": 0.3,
                         "Contorno": 0.035}, **S)
mi("MI_FireSoft_Chama", {"Modo": 2, "Brilho": 60, "FaixaMax": 0.5, "Calor": 0.05, "RuidoEscala": 0.9,
                         "RuidoVelocidade": 1.8, "Suavidade": 0.3, "Contorno": 0.035}, **S)
mi("MI_FireSoft_Rastro", {"Modo": 3, "Brilho": 60, "FaixaMax": 0.45, "Calor": 0.0, "RuidoEscala": 1.0,
                          "RuidoVelocidade": 2.2, "Suavidade": 0.3, "Contorno": 0.035}, **S)
print("GOKAKYU_MATERIAL_OK")
