"""Gōkakyū v2 (08/11), passo 3: os 3 sistemas Niagara (skill anime-fireball-vfx, seções 2, 3 e 5).

Tempos contados do cuspe (notify Katon_Spit, quando o projétil nasce). O C++ segura a bola na frente da boca até
GokakyuHoldTime (0,32 s) e então ela dispara; o núcleo e as 2 cascas são malhas do C++ (SM_Katon_Core/Shell).
  NS_Katon_Gokakyu_v2_Carga   (saída: parado na boca, X = mira; one-shot)
      0-0,1 ignição branca na boca + clarão | 0,05-0,27 brasas sugadas para o centro | 0,06-0,32 anel encolhendo
      0-0,42 esfera de fresnel crescendo com overshoot (pico ~0,3) | 0,29 flash do pico
      0,32 liberação em 2 fases: línguas disparando para a frente e uma fita que surge e encolhe rápido
  NS_Katon_Gokakyu_v2         (bola: preso ao projétil, X = voo; corpo em loop enquanto voa)
      4 fitas espirais girando em sentidos/velocidades diferentes (surgem no disparo, 0,3 s)
      línguas de chama (flipbook 8x8 nosso) soltas por distância e indo para trás = rastro
      cauda (fita do Niagara), brasas (Spark Trail), faíscas da frente (Front Sparks), halo aditivo
  NS_Katon_Gokakyu_v2_Impacto (no chão, escala 2,4 do C++; one-shot)
      onda de choque abrindo, esfera estourando, fitas abrindo, línguas, brasas, fogo residual (pack Free_Fire)
Pode rodar de novo (a versão anterior de cada sistema vai para /Game/_ToDelete). Editor aberto, fora do Play.
"""
import time
import unreal

EAL = unreal.EditorAssetLibrary
NT = unreal.NinjaEditorTools
FOLDER = "/Game/HypeWorld/VFX/Katon/Niagara"
MAT = "/Game/HypeWorld/VFX/Katon/Materiais/"
MESH = "/Game/HypeWorld/VFX/Katon/Meshes/"
T = "/Niagara/DefaultAssets/Templates/Emitters/"
PACK = "/Game/Vefects/Free_Fire/Shared/Particles/"
MOD = "/Niagara/Modules/"
SPAWN_PER_UNIT = MOD + "Emitter/SpawnPerUnit.SpawnPerUnit"
INHERIT = MOD + "Update/Velocity/InheritVelocity.InheritVelocity"
CURL = MOD + "Update/Forces/V2/CurlNoiseForce.CurlNoiseForce"
SUBUV = MOD + "Update/SubUV/V2/SubUVAnimation.SubUVAnimation"
SELF, ONCE = "NewEnumerator1", "NewEnumerator2"
FOUNTAIN, MESHT, RIBBON = T + "Fountain", T + "UpwardMeshBurst", T + "LocationBasedRibbon"
R = 155.0          # FullRadius do BP (cm): a bola na boca fica com o centro R à frente
V = 93.0           # raio VISUAL da bola = FullRadius x GokakyuSize (0,6); colisão/dano continuam com R


def mat(name):
    return MAT + name + "." + name


def mesh(name):
    return MESH + name + "." + name


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


def fmt(v):
    if isinstance(v, str):
        return v
    if isinstance(v, (tuple, list)):
        return ",".join(map(str, v))
    return str(v)


def const(s, em, param, value, required=True):
    n = NT.set_niagara_constant(s, em, param, fmt(value))
    assert n > 0 or not required, f"{em}.{param}: {n}"
    if n == 0:
        print("   (sem constante) %s.%s" % (em, param))


def pin(s, em, module, name, value, required=True):
    ok = NT.set_niagara_module_pin(s, em, module, name, value)
    assert ok or not required, f"{em} {module}.{name}"
    if not ok:
        print("   (sem pino) %s %s.%s" % (em, module, name))


def rend(s, em, prop, value, index=0):
    assert NT.set_niagara_renderer_property(s, em, index, prop, value), f"{em}.{prop}={value}"


def emitter(s, em, prop, value):
    assert NT.set_niagara_emitter_property(s, em, prop, value), f"{em}.{prop}"


def module(s, em, script, stage, index=-1):
    assert NT.add_niagara_module(s, em, script, stage, index), f"{em} {script}"


def once(s, em, dur, delay=0.0):
    pin(s, em, "EmitterState", "Life Cycle Mode", SELF)
    pin(s, em, "EmitterState", "Loop Behavior", ONCE)
    if delay > 0:
        pin(s, em, "EmitterState", "UseLoopDelay", "true")
    return ("once", em, dur, delay)


# ------------------------------------------------------------------------------------------------ emissores
class Sprite:
    """Fountain virado chama/brasa. Estrutura (módulos/pinos) antes da 1ª compilação, valores depois."""

    def __init__(self, s, em, *, life, size, speed, cone, axis, radius, material, rise=(0, 0, 0), drag=0.0,
                 rate=0, spacing=None, inherit=None, curl=None, sort=0, surface=False, stretch=None, local=False,
                 offset=None, flipbook=False, burst=None, from_point=False, rot=40):
        self.__dict__.update(locals())
        del self.__dict__["self"]
        emitter(s, em, "bLocalSpace", "True" if local else "False")
        if spacing:
            module(s, em, SPAWN_PER_UNIT, "EmitterUpdate")
        if inherit is not None:
            module(s, em, INHERIT, "ParticleSpawn")
            pin(s, em, "InheritVelocity", "Speed Limit", "false", required=False)
        if curl:
            module(s, em, CURL, "ParticleUpdate", 2)
        if flipbook:
            module(s, em, SUBUV, "ParticleUpdate")
        if surface:
            pin(s, em, "ShapeLocation", "Surface Only", "true")
        if stretch:
            pin(s, em, "InitializeParticle", "Sprite Size Mode", "NewEnumerator2")
        if offset:
            pin(s, em, "InitializeParticle", "UsePositionOffset", "true")
        if from_point:
            pin(s, em, "AddVelocity", "Velocity Mode", "NewEnumerator1")
        if burst:
            once(s, em, burst[0], burst[1])

    def values(self):
        s, em = self.s, self.em
        const(s, em, "SpawnRate.SpawnRate", self.rate)
        if self.spacing:
            const(s, em, "SpawnPerUnit.Spawn Spacing", self.spacing)
            const(s, em, "SpawnPerUnit.Max Movement Threshold", 20000)
        if self.inherit is not None:
            const(s, em, "InheritVelocity.Inherited Velocity Amount Scale", (self.inherit,) * 3)
        if self.curl:
            const(s, em, "CurlNoiseForce.Noise Strength", self.curl[0])
            const(s, em, "CurlNoiseForce.Noise Frequency", self.curl[1])
        const(s, em, "InitializeParticle.Lifetime Min", self.life[0])
        const(s, em, "InitializeParticle.Lifetime Max", self.life[1])
        const(s, em, "InitializeParticle.Color", (1, 1, 1, 1), required=False)
        if self.stretch:
            w, l = self.stretch
            const(s, em, "InitializeParticle.Sprite Size Min", (self.size[0] * w, self.size[0] * l))
            const(s, em, "InitializeParticle.Sprite Size Max", (self.size[1] * w, self.size[1] * l))
            rend(s, em, "Alignment", "VelocityAligned")
        else:
            const(s, em, "InitializeParticle.Uniform Sprite Size Min", self.size[0])
            const(s, em, "InitializeParticle.Uniform Sprite Size Max", self.size[1])
            # rotação aleatória SÓ no nascimento (nenhum módulo gira por quadro)
            const(s, em, "InitializeParticle.Sprite Rotation Angle Min", -self.rot, required=False)
            const(s, em, "InitializeParticle.Sprite Rotation Angle Max", self.rot, required=False)
        if self.offset:
            const(s, em, "InitializeParticle.Position Offset", self.offset)
        const(s, em, "RandomRangeFloat.Minimum", self.speed[0])
        const(s, em, "RandomRangeFloat.Maximum", self.speed[1])
        const(s, em, "AddVelocity.Cone Axis", self.axis)
        const(s, em, "AddVelocity.Cone Angle", self.cone)
        if self.from_point and self.offset:
            const(s, em, "AddVelocity.Velocity Origin", self.offset, required=False)
        const(s, em, "ShapeLocation.Sphere Radius", self.radius)
        const(s, em, "GravityForce.Gravity", self.rise)
        const(s, em, "Drag.Drag", self.drag)
        const(s, em, "SolveForcesAndVelocity.Speed Limit", 8000, required=False)
        rend(s, em, "Material", mat(self.material))
        rend(s, em, "SortMode", "ViewDepth")
        rend(s, em, "SortOrderHint", str(self.sort))
        if self.flipbook:
            rend(s, em, "SubImageSize", "(X=8.000000,Y=8.000000)")
            rend(s, em, "bSubImageBlend", "True")
        if self.burst:
            const(s, em, "EmitterState.Loop Duration", self.burst[0])
            if self.burst[1] > 0:
                const(s, em, "EmitterState.Loop Delay", self.burst[1])


class Mesh:
    """UpwardMeshBurst virado camada de malha: parada (sem velocidade/gravidade), cor branca, girando no eixo X."""

    def __init__(self, s, em, *, meshes, material, life, scale, count=1, at=0.0, dur=0.6, offset=None,
                 spin=0.0, rotation=None, sort=0, kill=False):
        self.__dict__.update(locals())
        del self.__dict__["self"]
        emitter(s, em, "bLocalSpace", "True")
        once(s, em, dur)
        pin(s, em, "InitializeParticle", "AdjustHue", "false", required=False)
        pin(s, em, "SpawnBurst_Instantaneous", "UseLoopCountLimit", "true", required=False)
        if offset:
            pin(s, em, "InitializeParticle", "UsePositionOffset", "true")
        if rotation:
            pin(s, em, "InitialMeshOrientation", "Use Rotation Vector", "true", required=False)
        if kill:
            # o C++ só desativa o sistema no impacto: as fitas (vida longa) morrem na hora, nada fica preso no ar
            pin(s, em, "EmitterState", "Inactive Response", "NewEnumerator1")

    def values(self):
        s, em = self.s, self.em
        const(s, em, "EmitterState.Loop Duration", self.dur)
        const(s, em, "SpawnBurst_Instantaneous.Spawn Count", self.count)
        const(s, em, "SpawnBurst_Instantaneous.Spawn Time", self.at)
        const(s, em, "SpawnBurst_Instantaneous.Loop Count Limit", 1, required=False)
        const(s, em, "InitializeParticle.Lifetime Min", self.life)
        const(s, em, "InitializeParticle.Lifetime Max", self.life)
        const(s, em, "InitializeParticle.Color", (1, 1, 1, 1))
        const(s, em, "InitializeParticle.Alpha Scale Range", (1, 1), required=False)
        const(s, em, "InitializeParticle.Hue Shift Range", (0, 0), required=False)
        const(s, em, "InitializeParticle.Mesh Uniform Scale Min", self.scale)
        const(s, em, "InitializeParticle.Mesh Uniform Scale Max", self.scale)
        if self.offset:
            const(s, em, "InitializeParticle.Position Offset", self.offset)
        const(s, em, "RandomRangeFloat.Minimum", 0)
        const(s, em, "RandomRangeFloat.Maximum", 0)
        const(s, em, "GravityForce.Gravity", (0, 0, 0))
        const(s, em, "Drag.Drag", 0)
        const(s, em, "Drag.Rotational Drag", 0, required=False)
        # giro contínuo em volta do eixo X (voo): eixo fixo e taxa constante (nada aleatório por quadro)
        const(s, em, "RandomRangeVector.Minimum", (1, 0, 0))
        const(s, em, "RandomRangeVector.Maximum", (1, 0, 0))
        const(s, em, "UpdateMeshOrientation.Rotation Rate", self.spin)
        if self.rotation:
            const(s, em, "InitialMeshOrientation.Rotation", self.rotation)
        rend(s, em, "Meshes", "(" + ",".join("(Mesh=\"%s\")" % mesh(m) for m in self.meshes) + ")")
        rend(s, em, "bOverrideMaterials", "True")
        rend(s, em, "OverrideMaterials", "((ExplicitMat=\"%s\"))" % mat(self.material))
        rend(s, em, "SortMode", "ViewDepth")
        rend(s, em, "SortOrderHint", str(self.sort))


class Ribbon:
    def __init__(self, s, em, *, life, width, rate, material, sort=0):
        self.__dict__.update(locals())
        del self.__dict__["self"]
        emitter(s, em, "bLocalSpace", "False")

    def values(self):
        s, em = self.s, self.em
        const(s, em, "SpawnRate.SpawnRate", self.rate, required=False)
        const(s, em, "InitializeParticle.Lifetime Min", self.life, required=False)
        const(s, em, "InitializeParticle.Lifetime Max", self.life, required=False)
        const(s, em, "InitializeParticle.Lifetime", self.life, required=False)
        const(s, em, "InitializeParticle.Ribbon Width", self.width)
        const(s, em, "InitializeParticle.Color", (1, 1, 1, 1), required=False)
        rend(s, em, "Material", mat(self.material))
        rend(s, em, "SortOrderHint", str(self.sort))


def montar(name, plan, extent):
    """plan: lista de (template, nome, fábrica(s, nome) -> objeto com values())"""
    s = new_system(name, [(tpl, em) for tpl, em, _ in plan])
    objs = [f(s, em) for _, em, f in plan]
    NT.compile_niagara_system(s, False, unreal.Vector(0, 0, 0))   # cria as constantes dos módulos novos
    for o in objs:
        o.values()
    assert NT.compile_niagara_system(s, True, extent), name
    EAL.save_loaded_asset(s, False)
    print("OK", name)
    return s


def sp(**kw):
    return lambda s, em: Sprite(s, em, **kw)


def me(**kw):
    return lambda s, em: Mesh(s, em, **kw)


def rb(**kw):
    return lambda s, em: Ribbon(s, em, **kw)


C = (R, 0, 0)   # centro da bola na boca (X = mira)

# ================================================================================================ carga (saída)
montar("NS_Katon_Gokakyu_v2_Carga", [
    # 0-0,1 s: ponto branco-amarelo forte na boca + clarão
    (FOUNTAIN, "Ignicao", sp(life=(0.12, 0.16), size=(70, 110), speed=(0, 1), cone=0, axis=(1, 0, 0), radius=4,
                             material="MI_Katon_Ignicao", rate=75, local=True, offset=(25, 0, 0), sort=6,
                             burst=(0.04, 0.0))),
    (FOUNTAIN, "ClaraoBoca", sp(life=(0.3, 0.35), size=(300, 360), speed=(0, 1), cone=0, axis=(1, 0, 0), radius=4,
                                material="MI_Katon_HaloFlash", rate=60, local=True, offset=(60, 0, 0), sort=-1,
                                burst=(0.03, 0.0), rot=0)),
    # 0,05-0,27 s: brasas sugadas para o centro da bola (velocidade a partir do centro, negativa)
    (FOUNTAIN, "Sugadas", sp(life=(0.22, 0.28), size=(7, 13), speed=(-900, -700), cone=0, axis=(1, 0, 0),
                             radius=230, material="MI_Katon_Brasa", rate=180, local=True, offset=C, surface=True,
                             from_point=True, stretch=(1.0, 4.0), sort=4, burst=(0.2, 0.05))),
    # 0-0,42 s: esfera de fresnel crescendo na frente da boca, overshoot ~15% no pico (~0,3 s)
    (MESHT, "EsferaCarga", me(meshes=["SM_Katon_Shell"], material="MI_Katon_CargaEsfera", life=0.42, scale=1.9,
                              offset=C, spin=0.3, sort=2)),
    # 0,06-0,32 s: anel encolhendo para o centro (de frente para a mira)
    (MESHT, "Anel", me(meshes=["SM_Katon_Shockwave"], material="MI_Katon_Anel", life=0.26, scale=1.1, at=0.06,
                       offset=C, rotation=(0, 0.25, 0), spin=0.5, sort=1)),
    # 0,29 s: flash do pico
    (FOUNTAIN, "FlashPico", sp(life=(0.3, 0.32), size=(600, 700), speed=(0, 1), cone=0, axis=(1, 0, 0), radius=4,
                               material="MI_Katon_HaloFlash", rate=60, local=True, offset=C, sort=-1,
                               burst=(0.03, 0.29), rot=0)),
    # 0,32 s: liberação, 1ª fase = línguas disparando para a frente e perdendo força rápido
    (FOUNTAIN, "Liberacao", sp(life=(0.16, 0.28), size=(120, 200), speed=(900, 1700), cone=28, axis=(1, 0, 0),
                               radius=60, material="MI_Katon_Lingua", rate=300, drag=7.0, local=True,
                               offset=C, flipbook=True, sort=3, burst=(0.06, 0.32))),
    # 0,32 s: liberação, 2ª fase = fita que surge grande e encolhe rápido (dá peso)
    (MESHT, "FitaLiberacao", me(meshes=["SM_Katon_Ribbon_02"], material="MI_Katon_FitaLiberacao", life=0.16,
                                scale=2.1, at=0.32, offset=(R + 30, 0, 0), spin=2.0, sort=3)),
], unreal.Vector(1500, 1500, 1500))

# ================================================================================================ bola (voo)
# vida 2 s (o voo dura ~1 s): as curvas de cor/tamanho do modelo são por idade normalizada; com 60 s as fitas
# ficavam presas no começo da curva (invisíveis). No impacto o C++ desativa e elas morrem na hora (kill)
# orientação inicial FIXA (o modelo sorteia uma rotação qualquer e a fita tombava em vez de envolver a bola);
# escala 1,0: a curva de tamanho do modelo ainda aumenta a malha com a idade
FITA = dict(material="MI_Katon_Fita", life=2.0, scale=1.0, at=0.3, dur=2.5, kill=True, sort=2, rotation=(0, 0, 0))
montar("NS_Katon_Gokakyu_v2", [
    # fitas: 0,5-1,5 volta/s, sentidos diferentes (Rotation Rate em voltas/s)
    (MESHT, "Fita01", me(meshes=["SM_Katon_Ribbon_01"], spin=0.8, **FITA)),
    (MESHT, "Fita02", me(meshes=["SM_Katon_Ribbon_02"], spin=-1.2, **FITA)),
    (MESHT, "Fita03", me(meshes=["SM_Katon_Ribbon_03"], spin=1.5, **FITA)),
    (MESHT, "Fita04", me(meshes=["SM_Katon_Ribbon_04"], spin=-0.6, **FITA)),
    # línguas: soltas por distância, indo para trás (forma o rastro); Shape esfera = largura
    (FOUNTAIN, "Linguas", sp(life=(0.25, 0.4), size=(110, 180), speed=(150, 400), cone=35, axis=(-1, 0, 0),
                             radius=V * 0.8, material="MI_Katon_Lingua", spacing=28, rate=0, inherit=0.3,
                             surface=True, flipbook=True, rise=(0, 0, 120), drag=2.5, sort=1)),
    # cauda: sprites soltos por distância no mundo (a fita do modelo LocationBasedRibbon anda sozinha em curva
    # e virava uma faixa retangular), mais transparentes, esfriando com a idade, quase sem velocidade
    (FOUNTAIN, "Cauda", sp(life=(0.45, 0.6), size=(150, 230), speed=(20, 80), cone=30, axis=(-1, 0, 0),
                           radius=V * 0.5, material="MI_Katon_Cauda", spacing=40, rate=0, inherit=0.1,
                           rise=(0, 0, 90), drag=1.5, flipbook=True, sort=0)),
    # Spark Trail: tamanho aleatório não uniforme, curl noise, sem escala por idade
    (FOUNTAIN, "SparkTrail", sp(life=(0.5, 0.9), size=(6, 14), speed=(150, 400), cone=70, axis=(-1, 0, 0.3),
                                radius=V * 0.9, material="MI_Katon_Brasa", spacing=45, rate=0, inherit=0.3,
                                curl=(220, 1.0), stretch=(1.0, 2.5), rise=(0, 0, 150), drag=1.5, sort=4)),
    # Front Sparks: vida bem mais curta, curl mais forte
    (FOUNTAIN, "FrontSparks", sp(life=(0.15, 0.25), size=(6, 12), speed=(300, 700), cone=60, axis=(1, 0, 0),
                                 radius=V * 0.9, material="MI_Katon_Brasa", spacing=60, rate=0, inherit=0.6,
                                 curl=(600, 2.0), surface=True, drag=2.0, sort=4)),
    # halo: 1 sprite aditivo grande e suave preso à bola
    (FOUNTAIN, "Halo", sp(life=(0.35, 0.45), size=(300, 360), speed=(0, 1), cone=0, axis=(1, 0, 0), radius=5,
                          material="MI_Katon_Halo", rate=10, local=True, sort=-1, rot=0)),
], unreal.Vector(2200, 1200, 1200))

# ================================================================================================ impacto
montar("NS_Katon_Gokakyu_v2_Impacto", [
    (MESHT, "Onda", me(meshes=["SM_Katon_Shockwave"], material="MI_Katon_Onda", life=0.5, scale=1.0, sort=1)),
    (MESHT, "Estouro", me(meshes=["SM_Katon_Shell"], material="MI_Katon_Estouro", life=0.3, scale=1.2,
                          offset=(0, 0, 40), spin=0.5, sort=2)),
    (MESHT, "FitasAbrindo", me(meshes=["SM_Katon_Ribbon_01", "SM_Katon_Ribbon_03"], material="MI_Katon_FitaImpacto",
                               life=0.4, scale=0.7, count=3, offset=(0, 0, 40), spin=1.2, rotation=(0, -0.25, 0),
                               sort=2)),
    (FOUNTAIN, "Chamas", sp(life=(0.4, 0.7), size=(200, 360), speed=(500, 1100), cone=70, axis=(0, 0, 1), radius=40,
                            material="MI_Katon_Lingua", rate=600, rise=(0, 0, 300), drag=4.0, local=True,
                            flipbook=True, sort=3, burst=(0.08, 0.0))),
    (FOUNTAIN, "Brasas", sp(life=(0.5, 1.0), size=(8, 16), speed=(400, 1000), cone=80, axis=(0, 0, 1), radius=40,
                            material="MI_Katon_Brasa", rate=300, rise=(0, 0, 150), drag=1.5, local=True,
                            stretch=(1.0, 3.0), sort=4, burst=(0.08, 0.0))),
    (FOUNTAIN, "Clarao", sp(life=(0.35, 0.4), size=(700, 850), speed=(0, 1), cone=0, axis=(0, 0, 1), radius=4,
                            material="MI_Katon_HaloFlash", rate=60, local=True, offset=(0, 0, 60), sort=-1,
                            burst=(0.03, 0.0), rot=0)),
    # fogo residual no chão: cópia do emissor do pack (o original não é tocado)
    (PACK + "NE_FireFlame", "FogoResidual", lambda s, em: type("X", (), {"values": lambda self: None})()),
], unreal.Vector(2600, 2600, 2000))
print("GOKAKYU_V2_NIAGARA_OK")
