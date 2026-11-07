"""Gōkakyū como JATO CONTÍNUO de fogo (07/11, especificação do Felipe com os quadros do Zenkai).

Não é uma esfera voando: um emissor invisível segue a bola e solta chamas por DISTÂNCIA (Spawn Per Unit) no mundo.
Cada chama nasce pequena e muito brilhante, cresce ~2,5x, sobe, perde velocidade (drag), herda parte da velocidade
da bola e é levada por curl noise lento (vento). A sobreposição forma o volume; a cor vem da IDADE em degraus:
  recém-nascida #FFF6D0 / #FFD447 (superexposta) -> meia-idade #FF9A2A / #FF6A16 -> velha #E0301C -> vermelho-escuro
  transparente sumindo (sem contorno preto).
Material M_FireJato (Translúcido, Unlit; Modo 0 = chama, Modo 1 = risco de calor) + instâncias.
Sistemas (pasta Gokakyu; o NS_Katon_Gokakyu da bola fica como backup):
  NS_Katon_Gokakyu_Jato        Cabeca (chamas novas, densas, envolvendo a frente), Jato (cauda longa que fica no ar),
                               Riscos (2-4 riscos alaranjados paralelos), Brasas (faíscas do pack)
  NS_Katon_Gokakyu_Jato_Saida  estouro na boca
  NS_Katon_Gokakyu_Jato_Impacto onda de fogo para cima e para os lados + fogo no chão ~2 s + brasas
Pode rodar de novo (a versão anterior de cada sistema vai para /Game/_ToDelete). Editor aberto, fora do Play.
"""
import time
import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
NT = unreal.NinjaEditorTools
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = "/Game/HypeWorld/Jutsus/Katon/VFX/Gokakyu"
# ruídos próprios de 1024 px (gokakyu_5_ruido.py); os do pack (T_VFX_Noise_01/08) tinham 256 px
NOISE_A = "/Game/HypeWorld/Jutsus/Katon/VFX/Gokakyu/Texturas/T_Gokakyu_Ruido_A"
NOISE_B = "/Game/HypeWorld/Jutsus/Katon/VFX/Gokakyu/Texturas/T_Gokakyu_Ruido_B"
T = "/Niagara/DefaultAssets/Templates/Emitters/"
PACK = "/Game/Vefects/Free_Fire/Shared/Particles/"
MOD = "/Niagara/Modules/"
SPAWN_PER_UNIT = MOD + "Emitter/SpawnPerUnit.SpawnPerUnit"
INHERIT = MOD + "Update/Velocity/InheritVelocity.InheritVelocity"
CURL = MOD + "Update/Forces/V2/CurlNoiseForce.CurlNoiseForce"
SELF, ONCE = "NewEnumerator1", "NewEnumerator2"

# ================================================================================================ material
path = f"{FOLDER}/M_FireJato"
mat = unreal.load_asset(path)
if mat:
    MEL.delete_all_material_expressions(mat)
else:
    mat = tools.create_asset("M_FireJato", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)
for flag in ("used_with_niagara_sprites", "used_with_niagara_ribbons"):
    mat.set_editor_property(flag, True)
Y = [0]


def node(cls):
    Y[0] += 70
    return MEL.create_material_expression(mat, cls, -900, Y[0])


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


uv = node(unreal.MaterialExpressionTextureCoordinate)
time_n = node(unreal.MaterialExpressionTime)
age = node(unreal.MaterialExpressionParticleRelativeTime)
rnd = node(unreal.MaterialExpressionParticleRandom)
# soft particles: some antes de encostar no chão/paredes (sem corte reto)
depth_fade = node(unreal.MaterialExpressionDepthFade)
depth_fade.set_editor_property("fade_distance_default", 90.0)
inputs = [("UV", uv), ("Time", time_n), ("Age", age), ("Rnd", rnd), ("DF", depth_fade),
          ("RA", texobj("RuidoA", NOISE_A)), ("RB", texobj("RuidoB", NOISE_B)),
          ("Modo", scalar("Modo", 0.0)), ("Bri", scalar("Brilho", 1.0)), ("Esc", scalar("RuidoEscala", 1.0)),
          ("Vel", scalar("RuidoVelocidade", 1.0)), ("Dist", scalar("Distorcao", 0.22)),
          ("Cresce", scalar("CresceDe", 0.4)), ("FMax", scalar("CalorMax", 1.0)), ("Risco", scalar("RiscoAlfa", 0.3)),
          ("Velha", scalar("VelhaIdade", 1.0)), ("Tint", vector("Tint", (1, 1, 1, 1))),
          ("Suav", scalar("Suavidade", 0.4)), ("CalorB", scalar("CalorBase", 0.0)),
          ("PD", node(unreal.MaterialExpressionPixelDepth)), ("Perto", scalar("SomePerto", 450.0)),
          # degradê do Felipe (3ª especificação, hex sRGB -> linear): sem marrom, sem contorno escuro
          ("C0", vector("Cor0_Miolo", (1.0, 0.939, 0.745, 1))),      # #FFF8E0
          ("C1", vector("Cor1_MioloQuente", (1.0, 0.745, 0.254, 1))),# #FFE08A
          ("C2", vector("Cor2_Corpo", (1.0, 0.434, 0.039, 1))),      # #FFB038
          ("C3", vector("Cor3_CorpoQuente", (1.0, 0.195, 0.013, 1))),# #FF7A1E
          ("C4", vector("Cor4_Borda", (1.0, 0.068, 0.010, 1)))]       # #FF4A1A
CODE = r"""
float age = saturate(Age / max(Velha, 0.05));
// 3 variações por partícula (escala do ruído e proporção da chama): nada se repete como carimbo
float var = floor(frac(Rnd * 7.31) * 3.0);
float esc = Esc * (0.75 + var * 0.3);
float t = Time * Vel;
float2 p = UV * esc + Rnd * 17.3;
// ruídos próprios de 1024 px (T_Gokakyu_Ruido_A/B) rolando para cima com distorção de UV
float2 d2 = Texture2DSample(RB, RBSampler, p * 0.5 + float2(t * 0.04, t * 0.3)).rg - 0.5;
float n1 = Texture2DSample(RA, RASampler, p + d2 * Dist + float2(-t * 0.03, t * 0.5)).r;
float n2 = Texture2DSample(RA, RASampler, p * 2.1 - d2 * Dist * 0.6 + float2(t * 0.11, t * 0.75)).r;
float n = n1 * 0.62 + n2 * 0.38;
float2 q = (UV - 0.5) * 2.0;
float rq = length(q);
float v;
if (Modo < 0.5) {
    // chama: nasce pequena e cresce; proporção varia (redonda / alta / larga)
    float g = lerp(Cresce, 1.0, 1.0 - pow(1.0 - saturate(age / 0.45), 2.0));
    float2 c = q / g;
    c.x *= 0.85 + var * 0.15;
    float r = length(c);
    v = pow(saturate(1.0 - r), 0.75) * 1.2 + (n - 0.5) * 0.8 * saturate(r + 0.2);
} else {
    // risco de calor: fino na largura (U), longo no comprimento (V), some nas pontas
    float across = saturate(1.0 - abs(UV.x - 0.5) * 2.0);
    float along = sin(saturate(UV.y) * 3.14159);
    v = pow(across, 14.0) * along * (0.55 + 0.9 * n);
    rq = abs(UV.y - 0.5) * 2.0 * 0.9;
}
// erosão com transição LARGA (borda de gás, nada recortado)
float thr = lerp(0.02, 0.6, pow(age, 1.3));
float a = smoothstep(thr - 0.06, thr + Suav, v);
// máscara radial do quadrado: chega a 0 ANTES da borda do sprite (sem retângulos)
a *= 1.0 - smoothstep(0.6, 0.95, rq);
// soft particles (chão/paredes) e chamas coladas na câmera
a *= DF * saturate((PD - 120.0) / max(Perto, 1.0));
// calor = juventude + miolo; degradê CONTÍNUO e saturado, laranja predominante
// (1º acabamento 0,95 + ganho 2,4-14: o tonemapper clareava o corpo até branco-creme) laranja predominante
float h = min(saturate(0.8 + CalorB - age * 1.25 + (v - thr) * 0.5), FMax);
float3 col = lerp(C4.rgb, C3.rgb, smoothstep(0.05, 0.38, h));
col = lerp(col, C2.rgb, smoothstep(0.32, 0.62, h));
col = lerp(col, C1.rgb, smoothstep(0.6, 0.82, h));
col = lerp(col, C0.rgb, smoothstep(0.8, 0.97, h));
float gain = lerp(1.6, 12.0, pow(h, 3.0));   // corpo ~2-4 (laranja saturado), miolo estoura; borda some pela transparência
// envelhecendo: laranja -> vermelho-alaranjado ficando transparente
a *= 1.0 - smoothstep(0.6, 1.0, age);
if (Modo > 0.5) { a *= Risco; }
return float4(col * gain * Bri * Tint.rgb, a);
"""
c = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -350, 0)
c.set_editor_property("code", CODE)
c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
pins = []
for name, _ in inputs:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", name)
    pins.append(ci)
c.set_editor_property("inputs", pins)
for name, n in inputs:
    assert MEL.connect_material_expressions(n, "", c, name), name


def mask(r, g, b, a, y):
    m = MEL.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -150, y)
    for k, v in (("r", r), ("g", g), ("b", b), ("a", a)):
        m.set_editor_property(k, v)
    MEL.connect_material_expressions(c, "", m, "")
    return m


MEL.connect_material_property(mask(True, True, True, False, -60), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.connect_material_property(mask(False, False, False, True, 60), "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(mat)
EAL.save_loaded_asset(mat, False)
print("OK", path)


def mi(name, scalars):
    p = f"{FOLDER}/{name}"
    inst = unreal.load_asset(p) or tools.create_asset(name, FOLDER, unreal.MaterialInstanceConstant,
                                                      unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(inst, mat)
    for k, v in scalars.items():
        MEL.set_material_instance_scalar_parameter_value(inst, k, v)
    MEL.update_material_instance(inst)
    EAL.save_loaded_asset(inst, False)
    return p + "." + name


MI_CHAMA = mi("MI_FireJato_Chama", {"Modo": 0, "RuidoEscala": 1.0, "RuidoVelocidade": 0.8})
# cabeça = parte mais densa e brilhante (miolo amarelo-branco); cauda = laranja/vermelho esfriando
MI_CABECA = mi("MI_FireJato_Cabeca", {"Modo": 0, "RuidoEscala": 1.0, "RuidoVelocidade": 1.0, "CalorBase": 0.06,
                                      "Brilho": 1.3, "Suavidade": 0.4})
MI_CAUDA = mi("MI_FireJato_Cauda", {"Modo": 0, "RuidoEscala": 0.9, "RuidoVelocidade": 0.7, "CalorBase": -0.08,
                                    "Brilho": 0.9, "Suavidade": 0.45})
MI_CHAO = mi("MI_FireJato_Chao", {"Modo": 0, "RuidoEscala": 1.1, "RuidoVelocidade": 0.6, "CalorMax": 0.6})
MI_RISCO = mi("MI_FireJato_Risco", {"Modo": 1, "RuidoEscala": 0.8, "RuidoVelocidade": 1.0, "CalorMax": 0.55,
                                    "RiscoAlfa": 0.3})


# ---------------------------------------------------------------- halo: aura laranja grande, aditiva e suave
hpath = f"{FOLDER}/M_FireHalo"
halo = unreal.load_asset(hpath)
if halo:
    MEL.delete_all_material_expressions(halo)
else:
    halo = tools.create_asset("M_FireHalo", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
halo.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
halo.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
halo.set_editor_property("used_with_niagara_sprites", True)
hc = MEL.create_material_expression(halo, unreal.MaterialExpressionCustom, -350, 0)
hc.set_editor_property("code", r"""
float r = length((UV - 0.5) * 2.0);
float m = exp(-r * r * 3.2) * (1.0 - smoothstep(0.7, 1.0, r));   // gaussiana que chega a 0 antes da borda
float vida = sin(saturate(Age) * 3.14159);                        // aparece e some suave
return Cor.rgb * Bri * m * vida * DF;
""")
hc.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
h_df = MEL.create_material_expression(halo, unreal.MaterialExpressionDepthFade, -800, 140)
h_df.set_editor_property("fade_distance_default", 150.0)
hb = MEL.create_material_expression(halo, unreal.MaterialExpressionScalarParameter, -800, 210)
hb.set_editor_property("parameter_name", "Brilho")
hb.set_editor_property("default_value", 0.45)
hcor = MEL.create_material_expression(halo, unreal.MaterialExpressionVectorParameter, -800, 280)
hcor.set_editor_property("parameter_name", "Cor")
hcor.set_editor_property("default_value", unreal.LinearColor(1.0, 0.254, 0.023, 1))   # #FF8A2A
hins = [("UV", MEL.create_material_expression(halo, unreal.MaterialExpressionTextureCoordinate, -800, 0)),
        ("Age", MEL.create_material_expression(halo, unreal.MaterialExpressionParticleRelativeTime, -800, 70)),
        ("DF", h_df), ("Bri", hb), ("Cor", hcor)]
pins = []
for name, _ in hins:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", name)
    pins.append(ci)
hc.set_editor_property("inputs", pins)
for name, n in hins:
    assert MEL.connect_material_expressions(n, "", hc, name), name
MEL.connect_material_property(hc, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(halo)
EAL.save_loaded_asset(halo, False)
hmi = f"{FOLDER}/MI_FireHalo"
inst = unreal.load_asset(hmi) or tools.create_asset("MI_FireHalo", FOLDER, unreal.MaterialInstanceConstant,
                                                    unreal.MaterialInstanceConstantFactoryNew())
MEL.set_material_instance_parent(inst, halo)
MEL.update_material_instance(inst)
EAL.save_loaded_asset(inst, False)
MI_HALO = hmi + ".MI_FireHalo"


# ================================================================================================ Niagara
def new_system(name, emitters):
    p = f"{FOLDER}/{name}"
    if EAL.does_asset_exist(p):
        dest = f"/Game/_ToDelete/{name}_{int(time.time())}"
        assert EAL.rename_asset(p, dest), dest
        print("versão anterior movida para", dest)
    ems = [unreal.load_asset(src) for src, _ in emitters]
    assert all(ems), emitters
    s = NT.create_niagara_system(FOLDER, name, ems, [n for _, n in emitters])
    assert s, name
    return s


def const(s, em, param, value, required=True):
    n = NT.set_niagara_constant(s, em, param, value if isinstance(value, str) else
                                ",".join(map(str, value)) if isinstance(value, (tuple, list)) else str(value))
    assert n > 0 or not required, f"{em}.{param}: {n}"


def rend(s, em, prop, value):
    assert NT.set_niagara_renderer_property(s, em, 0, prop, value), f"{em}.{prop}"


def emitter(s, em, prop, value):
    assert NT.set_niagara_emitter_property(s, em, prop, value), f"{em}.{prop}"


def module(s, em, script, stage, index=-1):
    assert NT.add_niagara_module(s, em, script, stage, index), f"{em} {script}"


def once(s, em, dur):
    assert NT.set_niagara_module_pin(s, em, "EmitterState", "Life Cycle Mode", SELF), em
    assert NT.set_niagara_module_pin(s, em, "EmitterState", "Loop Behavior", ONCE), em
    const(s, em, "EmitterState.Loop Duration", dur, required=False)


def chamas(s, em, *, life, size, speed, cone, axis, radius, rise, drag, material=MI_CHAMA, rate=None,
           spacing=None, inherit=None, curl=None, sort=0, surface=False, flat=None, stretch=None, local=False):
    """Emissor Fountain virado chama: mundo (ou local), tamanho/vida/velocidade, vento (gravidade) e opcionais."""
    emitter(s, em, "bLocalSpace", "True" if local else "False")
    if spacing:
        module(s, em, SPAWN_PER_UNIT, "EmitterUpdate")
    if inherit is not None:
        module(s, em, INHERIT, "ParticleSpawn")
        assert NT.set_niagara_module_pin(s, em, "InheritVelocity", "Speed Limit", "false"), em
    if curl:
        module(s, em, CURL, "ParticleUpdate", 2)
    if surface:   # nasce na casca da esfera (envolve a cabeça), não no miolo
        assert NT.set_niagara_module_pin(s, em, "ShapeLocation", "Surface Only", "true"), em
    if stretch:   # tamanho não-uniforme aleatório (largura x comprimento) + alinhado à velocidade = fiapos de gás
        assert NT.set_niagara_module_pin(s, em, "InitializeParticle", "Sprite Size Mode", "NewEnumerator2"), em
    return dict(life=life, size=size, speed=speed, cone=cone, axis=axis, radius=radius, rise=rise, drag=drag,
                material=material, rate=rate, spacing=spacing, inherit=inherit, curl=curl, sort=sort,
                surface=surface, flat=flat, stretch=stretch)


def chamas_valores(s, em, cfg):
    """Constantes só existem depois da 1ª compilação (módulos novos)."""
    const(s, em, "SpawnRate.SpawnRate", cfg["rate"] or 0)
    if cfg["spacing"]:
        const(s, em, "SpawnPerUnit.Spawn Spacing", cfg["spacing"])
        const(s, em, "SpawnPerUnit.Max Movement Threshold", 20000)
    if cfg["inherit"] is not None:
        const(s, em, "InheritVelocity.Inherited Velocity Amount Scale", (cfg["inherit"],) * 3)
    if cfg["curl"]:
        const(s, em, "CurlNoiseForce.Noise Strength", cfg["curl"][0])
        const(s, em, "CurlNoiseForce.Noise Frequency", cfg["curl"][1])
    const(s, em, "InitializeParticle.Lifetime Min", cfg["life"][0])
    const(s, em, "InitializeParticle.Lifetime Max", cfg["life"][1])
    if cfg["stretch"]:
        w, l = cfg["stretch"]
        const(s, em, "InitializeParticle.Sprite Size Min", (cfg["size"][0] * w, cfg["size"][0] * l))
        const(s, em, "InitializeParticle.Sprite Size Max", (cfg["size"][1] * w, cfg["size"][1] * l))
        const(s, em, "InitializeParticle.Sprite Rotation Angle Min", 0, required=False)
        const(s, em, "InitializeParticle.Sprite Rotation Angle Max", 0, required=False)
        rend(s, em, "Alignment", "VelocityAligned")
    else:
        const(s, em, "InitializeParticle.Uniform Sprite Size Min", cfg["size"][0])
        const(s, em, "InitializeParticle.Uniform Sprite Size Max", cfg["size"][1])
        # rotação aleatória SÓ no nascimento (não há módulo girando por quadro)
        const(s, em, "InitializeParticle.Sprite Rotation Angle Min", -40)
        const(s, em, "InitializeParticle.Sprite Rotation Angle Max", 40)
    const(s, em, "RandomRangeFloat.Minimum", cfg["speed"][0])
    const(s, em, "RandomRangeFloat.Maximum", cfg["speed"][1])
    const(s, em, "AddVelocity.Cone Axis", cfg["axis"])
    const(s, em, "AddVelocity.Cone Angle", cfg["cone"])
    const(s, em, "ShapeLocation.Sphere Radius", cfg["radius"])
    if cfg["flat"]:
        const(s, em, "ShapeLocation.Non Uniform Scale", cfg["flat"])
    const(s, em, "GravityForce.Gravity", cfg["rise"])
    const(s, em, "Drag.Drag", cfg["drag"])
    const(s, em, "SolveForcesAndVelocity.Speed Limit", 6000, required=False)
    rend(s, em, "Material", cfg["material"])
    rend(s, em, "SortMode", "ViewDepth")
    rend(s, em, "SortOrderHint", str(cfg["sort"]))


def montar(name, emitters, cfgs, extra=None, fixed=None):
    s = new_system(name, emitters)
    built = {em: chamas(s, em, **cfg) for em, cfg in cfgs.items()}
    if extra:
        extra(s, "estrutura")
    NT.compile_niagara_system(s, False, unreal.Vector(0, 0, 0))   # cria as constantes dos módulos novos
    for em, cfg in built.items():
        chamas_valores(s, em, cfg)
    if extra:
        extra(s, "valores")
    assert NT.compile_niagara_system(s, bool(fixed), fixed or unreal.Vector(0, 0, 0)), name
    EAL.save_loaded_asset(s, False)
    print("OK", name)
    return s


VENTO = (60, 160, 230)   # vento de lado + calor subindo (cm/s²): a cauda curva e sobe devagar

# ---------------------------------------------------------------- voo: jato contínuo
def voo_extra(s, etapa):
    if etapa == "estrutura":
        emitter(s, "Riscos", "bLocalSpace", "False")
        emitter(s, "Brasas", "bLocalSpace", "False")
        return
    # riscos de calor: poucos, longos, alinhados à velocidade, paralelos ao jato
    const(s, "Riscos", "SpawnRate.SpawnRate", 7)
    const(s, "Riscos", "InitializeParticle.Lifetime Min", 0.35)
    const(s, "Riscos", "InitializeParticle.Lifetime Max", 0.55)
    const(s, "Riscos", "InitializeParticle.Uniform Sprite Size Min", 650)
    const(s, "Riscos", "InitializeParticle.Uniform Sprite Size Max", 900)
    const(s, "Riscos", "InitializeParticle.Sprite Rotation Angle Min", 0)
    const(s, "Riscos", "InitializeParticle.Sprite Rotation Angle Max", 0)
    const(s, "Riscos", "RandomRangeFloat.Minimum", 500)
    const(s, "Riscos", "RandomRangeFloat.Maximum", 800)
    const(s, "Riscos", "AddVelocity.Cone Axis", (-1, 0, 0))
    const(s, "Riscos", "AddVelocity.Cone Angle", 4)
    const(s, "Riscos", "ShapeLocation.Sphere Radius", 120)
    const(s, "Riscos", "GravityForce.Gravity", (0, 0, 60))
    const(s, "Riscos", "Drag.Drag", 1.0)
    rend(s, "Riscos", "Material", MI_RISCO)
    rend(s, "Riscos", "Alignment", "VelocityAligned")
    rend(s, "Riscos", "SortOrderHint", "0")
    # brasas do pack: pequenas e muito brilhantes, da cabeça e da cauda
    const(s, "Brasas", "SpawnRate.SpawnRate", 45, required=False)


voo = montar("NS_Katon_Gokakyu_Jato",
             [(T + "Fountain", "Cabeca"), (T + "Fountain", "Jato"), (T + "Fountain", "Riscos"),
              (PACK + "NE_Sparkles", "Brasas"), (T + "Fountain", "HaloCabeca"), (T + "Fountain", "HaloCauda")],
             {
                 # cabeça: chamas novas e densas na frente, nascendo na superfície e escorrendo para trás pelos lados
                 # (o stretch por velocidade das chamas grandes fazia retângulos: tirado; tamanhos variados)
                 "Cabeca": dict(life=(0.2, 0.34), size=(180, 360), speed=(450, 900), cone=70, axis=(-1, 0, 0),
                                radius=85, rise=VENTO, drag=3.0, spacing=17, rate=30, inherit=0.75,   # 12 = +2 ms de transparência
                                curl=(200, 5), sort=2, surface=True, material=MI_CABECA),
                 # jato/cauda: vive mais, herda pouca velocidade (fica no ar atrás), cresce, sobe e curva com o vento
                 "Jato": dict(life=(0.55, 1.0), size=(140, 280), speed=(80, 260), cone=55, axis=(-1, 0, 0.2),
                              radius=70, rise=VENTO, drag=1.8, spacing=22, rate=15, inherit=0.3,
                              curl=(450, 3), sort=1, material=MI_CAUDA),   # curl forte e lento = S
                 # halo: aura laranja aditiva grande e fraca, presa à cabeça (local) e um pouco na cauda (mundo)
                 "HaloCabeca": dict(life=(0.3, 0.4), size=(750, 900), speed=(0, 1), cone=0, axis=(1, 0, 0),
                                    radius=10, rise=(0, 0, 0), drag=0.0, rate=8, sort=-1, material=MI_HALO,
                                    local=True),
                 "HaloCauda": dict(life=(0.45, 0.6), size=(500, 650), speed=(0, 1), cone=0, axis=(1, 0, 0),
                                   radius=20, rise=(0, 0, 40), drag=1.0, spacing=140, rate=0, inherit=0.25,
                                   sort=-1, material=MI_HALO),
             }, extra=voo_extra)

# ---------------------------------------------------------------- saída: estouro na boca
def saida_extra(s, etapa):
    if etapa == "estrutura":
        once(s, "Estouro", 0.08)
        emitter(s, "Faiscas", "bLocalSpace", "False")
        once(s, "Faiscas", 0.15)
        return
    const(s, "Faiscas", "RandomRangeInt.Minimum", 20, required=False)
    const(s, "Faiscas", "RandomRangeInt.Maximum", 28, required=False)


montar("NS_Katon_Gokakyu_Jato_Saida", [(T + "Fountain", "Estouro"), (PACK + "NE_Sparkles", "Faiscas")],
       {"Estouro": dict(life=(0.25, 0.45), size=(160, 260), speed=(600, 1400), cone=60, axis=(1, 0, 0.1),
                        radius=50, rise=(0, 0, 300), drag=4.0, rate=200)},
       extra=saida_extra, fixed=unreal.Vector(1500, 1500, 1500))

# ---------------------------------------------------------------- impacto: onda de fogo + fogo no chão
def impacto_extra(s, etapa):
    if etapa == "estrutura":
        once(s, "Onda", 0.1)
        once(s, "Chao", 0.15)
        emitter(s, "Brasas", "bLocalSpace", "False")
        once(s, "Brasas", 0.2)
        return
    const(s, "Brasas", "RandomRangeInt.Minimum", 35, required=False)
    const(s, "Brasas", "RandomRangeInt.Maximum", 50, required=False)


# (o C++ cria o impacto com escala 2,4: posições da forma crescem 2,4x, tamanhos e velocidades não)
montar("NS_Katon_Gokakyu_Jato_Impacto",
       [(T + "Fountain", "Onda"), (T + "Fountain", "Chao"), (PACK + "NE_Sparkles", "Brasas")],
       {"Onda": dict(life=(0.6, 1.1), size=(380, 600), speed=(600, 1300), cone=80, axis=(0, 0, 1), radius=40,
                     rise=(0, 0, 350), drag=3.5, rate=800, curl=(200, 6), sort=1),
        "Chao": dict(life=(1.6, 2.3), size=(140, 220), speed=(30, 120), cone=30, axis=(0, 0, 1), radius=90,
                     rise=(0, 0, 120), drag=2.0, rate=200, material=MI_CHAO, flat=(1, 1, 0.08), sort=0)},
       extra=impacto_extra, fixed=unreal.Vector(2500, 2500, 2000))
print("GOKAKYU_JATO_OK")
