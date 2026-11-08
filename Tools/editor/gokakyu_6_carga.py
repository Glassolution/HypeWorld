"""Gōkakyū (07/11, 5ª especificação): FASE DE CARGA antes do disparo, como no Zenkai.

Cronometrada até o disparo de verdade: T = selos do DA + tempo do notify NinjaJutsuRelease no sopro (lido da animação;
hoje 0,55 + 0,375 = 0,925 s). Fases (fração de T), todas no NS_Katon_Gokakyu_Carga, preso à boca e virado para a frente
do personagem (X = frente, Z = cima):
  Ignição  0-0,2   ponto branco-amarelo forte na boca (Miolo) + faíscas (pontinhos, nada de raio de clip-art)
  Carga    0,2-0,75 línguas de chama nascendo em volta do corpo, subindo e girando (curl) + brasas subindo
                    + Aura: sprite aditivo enorme (~5-6x a altura) laranja #FF8A2A com centro branco-amarelo, crescendo
  Feixe    ~0,62-0,85 3 raios longos e retos (branco-amarelo -> laranja) na direção do disparo, rápidos
  Coluna   0,75-1  o fogo se alonga para a frente (chamas rápidas + miolo na frente)
Luz (ignição -> pico -> flash do feixe) e vinheta quente na tela: C++ (ANinjaJutsuProjectile::UpdateAnticipation).
Não feito: distorção de calor (refração) — fica para depois se precisar.
Rodar depois do gokakyu_4_jato.py (usa M_FireJato/M_FireHalo e as instâncias Miolo/Brasa). Editor aberto, fora do Play.
"""
import time
import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
NT = unreal.NinjaEditorTools
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = "/Game/HypeWorld/Jutsus/Katon/VFX/Gokakyu"
DA = "/Game/HypeWorld/Jutsus/Katon/DA_Katon_Goukakyuu"
BP = "/Game/HypeWorld/Jutsus/Katon/BP_Katon_Goukakyuu"
T_FOUNTAIN = "/Niagara/DefaultAssets/Templates/Emitters/Fountain"
CURL = "/Niagara/Modules/Update/Forces/V2/CurlNoiseForce.CurlNoiseForce"
SELF, ONCE = "NewEnumerator1", "NewEnumerator2"

# ---------------------------------------------------------------- tempo até o disparo (pela animação)
da = unreal.load_asset(DA)
seals = sum(s.get_editor_property("duration") for s in da.get_editor_property("seals"))
cast = da.get_editor_property("body_cast_animation")
notify = 0.375
for ev in unreal.AnimationLibrary.get_animation_notify_events(cast):
    if str(ev.get_editor_property("notify_name")) == "NinjaJutsuRelease":
        notify = unreal.AnimationLibrary.get_anim_notify_event_trigger_time(ev)
T = seals + notify / max(da.get_editor_property("cast_recovery_rate"), 0.05)
print("tempo até o disparo: selos %.3f + notify %.3f = %.3f s" % (seals, notify, T))


# ---------------------------------------------------------------- instâncias
def mi(name, parent, scalars, vectors=None):
    p = f"{FOLDER}/{name}"
    inst = unreal.load_asset(p) or tools.create_asset(name, FOLDER, unreal.MaterialInstanceConstant,
                                                      unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(inst, unreal.load_asset(parent))
    for k, v in scalars.items():
        MEL.set_material_instance_scalar_parameter_value(inst, k, v)
    for k, v in (vectors or {}).items():
        MEL.set_material_instance_vector_parameter_value(inst, k, unreal.LinearColor(*v))
    MEL.update_material_instance(inst)
    EAL.save_loaded_asset(inst, False)
    return p + "." + name


JATO, HALO = f"{FOLDER}/M_FireJato", f"{FOLDER}/M_FireHalo"
MI_MIOLO = f"{FOLDER}/MI_FireJato_Miolo.MI_FireJato_Miolo"
MI_BRASA = f"{FOLDER}/MI_FireJato_Brasa.MI_FireJato_Brasa"
MI_CABECA = f"{FOLDER}/MI_FireJato_Cabeca.MI_FireJato_Cabeca"
MI_LINGUA = mi("MI_FireJato_Lingua", JATO, {"Modo": 0, "RuidoEscala": 1.1, "RuidoVelocidade": 1.4, "CalorBase": 0.05,
                                            "Brilho": 1.1, "Suavidade": 0.4, "SomePerto": 150.0})
MI_FEIXE = mi("MI_FireJato_Feixe", JATO, {"Modo": 1, "RuidoEscala": 0.6, "RuidoVelocidade": 2.0, "CalorBase": 0.3,
                                          "CalorMax": 1.0, "Brilho": 2.5, "RiscoAlfa": 0.95, "SomePerto": 1.0})
MI_AURA = mi("MI_FireHalo_Aura", HALO, {"Brilho": 1.1, "Cresce": 0.3},
             {"Cor": (1.0, 0.254, 0.023, 1), "CorMiolo": (1.0, 0.82, 0.45, 1)})   # #FF8A2A, centro branco-amarelo

# ---------------------------------------------------------------- vinheta quente (pós-processo, só na tela de quem lança)
vpath = f"{FOLDER}/M_PP_Gokakyu_Vinheta"
vm = unreal.load_asset(vpath)
if vm:
    MEL.delete_all_material_expressions(vm)
else:
    vm = tools.create_asset("M_PP_Gokakyu_Vinheta", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
vm.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
scene = MEL.create_material_expression(vm, unreal.MaterialExpressionSceneTexture, -800, 0)
scene.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
spos = MEL.create_material_expression(vm, unreal.MaterialExpressionScreenPosition, -800, 150)
inten = MEL.create_material_expression(vm, unreal.MaterialExpressionScalarParameter, -800, 250)
inten.set_editor_property("parameter_name", "Intensidade")
inten.set_editor_property("default_value", 0.0)
vc = MEL.create_material_expression(vm, unreal.MaterialExpressionCustom, -350, 0)
vc.set_editor_property("code", r"""
float d = length((UV - 0.5) * float2(1.6, 1.0));
float v = smoothstep(0.45, 1.0, d) * saturate(I);
// bordas puxam para vermelho-alaranjado (nunca pretas)
return lerp(C.rgb, C.rgb * float3(1.15, 0.45, 0.25) + float3(0.10, 0.015, 0.0), v);
""")
vc.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
pins = []
for n in ("C", "UV", "I"):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    pins.append(ci)
vc.set_editor_property("inputs", pins)
assert MEL.connect_material_expressions(scene, "Color", vc, "C")
assert MEL.connect_material_expressions(spos, "ViewportUV", vc, "UV")
assert MEL.connect_material_expressions(inten, "", vc, "I")
MEL.connect_material_property(vc, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(vm)
EAL.save_loaded_asset(vm, False)


# ---------------------------------------------------------------- Niagara
def const(s, em, param, value, required=True):
    v = value if isinstance(value, str) else ",".join(map(str, value)) if isinstance(value, (tuple, list)) else str(value)
    n = NT.set_niagara_constant(s, em, param, v)
    if n <= 0:
        assert not required, f"{em}.{param}: {n}"
        print("  (sem constante)", em, param)


def rend(s, em, prop, value):
    assert NT.set_niagara_renderer_property(s, em, 0, prop, value), f"{em}.{prop}"


def fase(s, em, inicio, dur):
    """Emissor toca uma vez, começando em inicio (s) e soltando partículas por dur (s)."""
    assert NT.set_niagara_module_pin(s, em, "EmitterState", "Life Cycle Mode", SELF), em
    assert NT.set_niagara_module_pin(s, em, "EmitterState", "Loop Behavior", ONCE), em
    if inicio > 0:
        assert NT.set_niagara_module_pin(s, em, "EmitterState", "UseLoopDelay", "true"), em
    return inicio, dur


CFG = {
    # ignição: ponto branco-amarelo na boca, a carga inteira (o miolo da coluna também)
    "Miolo": dict(fase=(0.0, T), rate=40, life=(0.12, 0.18), size=(60, 120), speed=(0, 1), cone=0, axis=(1, 0, 0),
                  radius=8, rise=(0, 0, 0), drag=0, mat=MI_MIOLO, sort=5),
    "Faiscas": dict(fase=(0.0, 0.08), rate=160, life=(0.3, 0.5), size=(6, 12), speed=(200, 500), cone=70,
                    axis=(1, 0, 0.3), radius=10, rise=(0, 0, 200), drag=2.0, mat=MI_BRASA, sort=3),
    # carga: línguas em volta do corpo (a boca fica ~1,6 m do chão: nascem de ~0,9 m abaixo), subindo e girando
    "Linguas": dict(fase=(0.2 * T, 0.55 * T), rate=110, life=(0.35, 0.55), size=(90, 200), speed=(120, 300),
                    cone=25, axis=(0, 0, 1), radius=75, flat=(1, 1, 1.3), offset=(0, 0, -90), rise=(0, 0, 250),
                    drag=1.0, curl=(250, 4), stretch=(0.6, 1.5), mat=MI_LINGUA, sort=1),
    "Brasas": dict(fase=(0.2 * T, 0.6 * T), rate=35, life=(0.6, 0.9), size=(6, 14), speed=(150, 350), cone=30,
                   axis=(0, 0, 1), radius=90, offset=(0, 0, -60), rise=(0, 0, 150), drag=1.0, mat=MI_BRASA, sort=3),
    "Aura": dict(fase=(0.12 * T, 0.68 * T), rate=10, life=(0.45, 0.55), size=(1000, 1150), speed=(0, 1), cone=0,
                 axis=(1, 0, 0), radius=10, rise=(0, 0, 0), drag=0, mat=MI_AURA, sort=-1),
    # feixe: 3 raios longos e retos para a frente (alinhados à velocidade), aparecem e somem rápido
    "Feixe": dict(fase=(0.62 * T, 0.03), rate=100, life=(0.18, 0.24), size=(1, 1), sprite=((50, 1300), (90, 1700)),
                  speed=(30, 60), cone=3, axis=(1, 0, 0), radius=30, offset=(750, 0, 0), rise=(0, 0, 0), drag=10.0,
                  mat=MI_FEIXE, sort=4),
    # coluna: o fogo se alonga para a frente antes do disparo
    "Coluna": dict(fase=(0.75 * T, 0.25 * T), rate=140, life=(0.22, 0.32), size=(140, 260), speed=(900, 1500), cone=8,
                   axis=(1, 0, 0), radius=20, rise=(0, 0, 0), drag=3.0, mat=MI_CABECA, sort=2),
}

name = "NS_Katon_Gokakyu_Carga"
path = f"{FOLDER}/{name}"
if EAL.does_asset_exist(path):
    dest = f"/Game/_ToDelete/{name}_{int(time.time())}"
    assert EAL.rename_asset(path, dest), dest
    print("versão anterior movida para", dest)
fountain = unreal.load_asset(T_FOUNTAIN)
s = NT.create_niagara_system(FOLDER, name, [fountain] * len(CFG), list(CFG))
assert s, name
for em, c in CFG.items():
    assert NT.set_niagara_emitter_property(s, em, "bLocalSpace", "True"), em
    fase(s, em, *c["fase"])
    if c.get("curl"):
        assert NT.add_niagara_module(s, em, CURL, "ParticleUpdate", 2), em
    if c.get("offset"):
        assert NT.set_niagara_module_pin(s, em, "InitializeParticle", "UsePositionOffset", "true"), em
    if c.get("stretch") or c.get("sprite"):
        assert NT.set_niagara_module_pin(s, em, "InitializeParticle", "Sprite Size Mode", "NewEnumerator2"), em
NT.compile_niagara_system(s, False, unreal.Vector(0, 0, 0))   # cria as constantes
for em, c in CFG.items():
    inicio, dur = c["fase"]
    const(s, em, "EmitterState.Loop Duration", round(dur, 3), required=False)
    if inicio > 0:
        const(s, em, "EmitterState.Loop Delay", round(inicio, 3), required=False)
    const(s, em, "SpawnRate.SpawnRate", c["rate"])
    const(s, em, "InitializeParticle.Lifetime Min", c["life"][0])
    const(s, em, "InitializeParticle.Lifetime Max", c["life"][1])
    if c.get("sprite"):
        const(s, em, "InitializeParticle.Sprite Size Min", c["sprite"][0])
        const(s, em, "InitializeParticle.Sprite Size Max", c["sprite"][1])
        rend(s, em, "Alignment", "VelocityAligned")
    elif c.get("stretch"):
        w, l = c["stretch"]
        const(s, em, "InitializeParticle.Sprite Size Min", (c["size"][0] * w, c["size"][0] * l))
        const(s, em, "InitializeParticle.Sprite Size Max", (c["size"][1] * w, c["size"][1] * l))
        rend(s, em, "Alignment", "VelocityAligned")
    else:
        const(s, em, "InitializeParticle.Uniform Sprite Size Min", c["size"][0])
        const(s, em, "InitializeParticle.Uniform Sprite Size Max", c["size"][1])
    const(s, em, "InitializeParticle.Sprite Rotation Angle Min", 0 if c.get("sprite") or c.get("stretch") else -40)
    const(s, em, "InitializeParticle.Sprite Rotation Angle Max", 0 if c.get("sprite") or c.get("stretch") else 40)
    if c.get("offset"):
        const(s, em, "InitializeParticle.Position Offset", c["offset"])
    const(s, em, "RandomRangeFloat.Minimum", c["speed"][0])
    const(s, em, "RandomRangeFloat.Maximum", c["speed"][1])
    const(s, em, "AddVelocity.Cone Axis", c["axis"])
    const(s, em, "AddVelocity.Cone Angle", c["cone"])
    const(s, em, "ShapeLocation.Sphere Radius", c["radius"])
    if c.get("flat"):
        const(s, em, "ShapeLocation.Non Uniform Scale", c["flat"])
    const(s, em, "GravityForce.Gravity", c["rise"])
    const(s, em, "Drag.Drag", c["drag"])
    if c.get("curl"):
        const(s, em, "CurlNoiseForce.Noise Strength", c["curl"][0])
        const(s, em, "CurlNoiseForce.Noise Frequency", c["curl"][1])
    rend(s, em, "Material", c["mat"])
    rend(s, em, "SortMode", "ViewDepth")
    rend(s, em, "SortOrderHint", str(c["sort"]))
assert NT.compile_niagara_system(s, True, unreal.Vector(1600, 1600, 1200)), name
EAL.save_loaded_asset(s, False)
print("OK", path)

# ---------------------------------------------------------------- liga no DA e no BP
da.set_editor_property("gokakyu_charge_system", s)
EAL.save_loaded_asset(da, False)
bp = unreal.load_asset(BP)
cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("gokakyu_charge_vignette", vm)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
EAL.save_loaded_asset(bp, False)
print("GOKAKYU_CARGA_OK T=%.3f" % T)
