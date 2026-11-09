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
SPAWN_RATE = MOD + "Emitter/SpawnRate.SpawnRate"
COLLISION = "/Niagara/Modules/Collision/Collision.Collision"
SELF, ONCE = "NewEnumerator1", "NewEnumerator2"
FOUNTAIN, MESHT, RIBBON = T + "Fountain", T + "UpwardMeshBurst", T + "LocationBasedRibbon"
R = 210.0          # FullRadius do BP (cm): a bola na boca fica com o centro R à frente (08/11: 155 -> 210)
V = 126.0          # raio VISUAL de referência = FullRadius x GokakyuSize (0,6)


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
                 offset=None, flipbook=False, burst=None, from_point=False, rot=40, wind_local=False, start=None,
                 color=None, collision=False, flat=None):
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
        if start is not None:
            once(s, em, 100.0, start)   # liga em 'start' e fica ligado (100 s)
        if wind_local:
            pin(s, em, "GravityForce", "Coordinate Space", "Local")
        if collision:   # para no chão (fogo que fica no caminho)
            module(s, em, COLLISION, "ParticleUpdate")

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
        const(s, em, "InitializeParticle.Color", self.color or (1, 1, 1, 1), required=False)
        if self.flat:
            const(s, em, "ShapeLocation.Non Uniform Scale", self.flat, required=False)
        if self.collision:
            const(s, em, "Collision.Restitution", 0.0, required=False)
            const(s, em, "Collision.Friction", 1.0, required=False)
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
        if self.start is not None:
            const(s, em, "EmitterState.Loop Duration", 100.0)
            const(s, em, "EmitterState.Loop Delay", self.start)


class Mesh:
    """UpwardMeshBurst virado camada de malha: parada (sem velocidade/gravidade), cor branca, girando no eixo X."""

    def __init__(self, s, em, *, meshes, material, life, scale, count=1, at=0.0, dur=0.6, offset=None,
                 spin=0.0, rotation=None, sort=0, kill=False, rate=0.0, start=None):
        self.__dict__.update(locals())
        del self.__dict__["self"]
        emitter(s, em, "bLocalSpace", "True")
        if start is not None:
            once(s, em, 100.0, start)
        else:
            once(s, em, dur)
        if rate:
            module(s, em, SPAWN_RATE, "EmitterUpdate")
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
        const(s, em, "EmitterState.Loop Duration", 100.0 if self.start is not None else self.dur)
        if self.start is not None:
            const(s, em, "EmitterState.Loop Delay", self.start)
        if self.rate:
            const(s, em, "SpawnRate.SpawnRate", self.rate)
        const(s, em, "SpawnBurst_Instantaneous.Spawn Count", 0 if self.rate else self.count)
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
K = V / 93.0    # fator de tamanho em relação à 1ª versão aprovada (raio visual 93 cm)
SOLTA = 0.32    # GokakyuHoldTime do BP: instante da soltura (s depois do cuspe)

# ================================================================================================ carga (saída)
# 09/11: a carga e o pico acontecem ANTES da soltura; no instante da soltura só o clarão de saída (0,1 s), um anel de
# choque rápido e uma rajada para a frente que acompanha a bola. Nada grande fica parado na boca depois da soltura
# (antes: 18 chamas de 1,6-2,7 m com freio forte + fita da 2ª fase paravam ali e pareciam a bola travada).
montar("NS_Katon_Gokakyu_v2_Carga", [
    # 0-0,1 s: ponto branco-amarelo forte na boca + clarão
    (FOUNTAIN, "Ignicao", sp(life=(0.12, 0.16), size=(70, 110), speed=(0, 1), cone=0, axis=(1, 0, 0), radius=4,
                             material="MI_Katon_Ignicao", rate=75, local=True, offset=(25, 0, 0), sort=6,
                             burst=(0.04, 0.0))),
    (FOUNTAIN, "ClaraoBoca", sp(life=(0.3, 0.35), size=(300 * K, 360 * K), speed=(0, 1), cone=0, axis=(1, 0, 0),
                                radius=4, material="MI_Katon_HaloFlash", rate=60, local=True, offset=(60, 0, 0),
                                sort=-1, burst=(0.03, 0.0), rot=0)),
    # 0,05-0,27 s: brasas sugadas para o centro da bola
    (FOUNTAIN, "Sugadas", sp(life=(0.22, 0.28), size=(7, 13), speed=(-1150, -900), cone=0, axis=(1, 0, 0),
                             radius=230 * K, material="MI_Katon_Brasa", rate=180, local=True, offset=C,
                             surface=True, from_point=True, stretch=(1.0, 4.0), sort=4, burst=(0.2, 0.05))),
    # 0-0,33 s: esfera de fresnel crescendo na frente da boca com overshoot (pico ~0,3 s); some na soltura
    (MESHT, "EsferaCarga", me(meshes=["SM_Katon_Shell"], material="MI_Katon_CargaEsfera", life=0.33,
                              scale=V * 1.05 / 52, offset=C, spin=0.3, sort=2)),
    # 0,06-0,32 s: anel encolhendo para o centro
    (MESHT, "Anel", me(meshes=["SM_Katon_Shockwave"], material="MI_Katon_Anel", life=0.26, scale=V * 1.2 / 100,
                       at=0.06, offset=C, rotation=(0, 0.25, 0), spin=0.5, sort=1)),
    # SOLTURA: clarão branco-amarelo grande na boca (~0,1 s)
    # 09/11: 700-800 cobria a tela inteira na câmera do jogo; metade = clarão grande na boca, sem estourar a tela
    (FOUNTAIN, "FlashSaida", sp(life=(0.09, 0.11), size=(330 * K, 380 * K), speed=(0, 1), cone=0, axis=(1, 0, 0),
                                radius=4, material="MI_Katon_FlashSaida", rate=60, local=True, offset=(R * 0.5, 0, 0),
                                sort=7, burst=(0.03, SOLTA), rot=0)),
    # SOLTURA: anel de choque rápido abrindo na frente da boca
    (MESHT, "AnelSaida", me(meshes=["SM_Katon_Shockwave"], material="MI_Katon_Onda", life=0.18,
                            scale=V * 0.3 / 100, at=SOLTA, offset=(R * 0.6, 0, 0), rotation=(0, 0.25, 0), sort=3)),
    # SOLTURA: rajada de chamas e faíscas para a frente, rápidas (acompanham a bola, não param na boca)
    (FOUNTAIN, "RajadaSaida", sp(life=(0.1, 0.18), size=(120 * K, 200 * K), speed=(3200, 4800), cone=18,
                                 axis=(1, 0, 0), radius=40 * K, material="MI_Katon_Lingua", rate=400, drag=1.0,
                                 local=True, offset=C, flipbook=True, sort=3, burst=(0.04, SOLTA))),
    (FOUNTAIN, "FaiscasSaida", sp(life=(0.15, 0.3), size=(8, 16), speed=(2500, 5000), cone=30, axis=(1, 0, 0),
                                  radius=30 * K, material="MI_Katon_Brasa", rate=500, drag=1.5, local=True, offset=C,
                                  stretch=(1.0, 4.0), sort=4, burst=(0.04, SOLTA))),
], unreal.Vector(1800, 1800, 1800))

# ================================================================================================ bola (voo)
# Fogo vivo: a silhueta é quebrada o tempo todo; chamas nascem na superfície, ficam para trás (herdam só parte da
# velocidade + vento contrário ao voo no espaço da bola), as pontas sobem e apagam. Borda vermelha por fora, cauda
# contínua que esfria (amarelo -> laranja -> vermelho) e vira fumaça, fogo que fica no chão ao longo do caminho.
# Tudo do corpo começa na soltura: presa na boca, o vento empurraria fogo para cima do personagem.
VOO = dict(start=0.3)
FITA = dict(material="MI_Katon_Fita", life=0.7, scale=V / 60.0, rate=2.8, kill=True, sort=2, rotation=(0, 0, 0),
            start=0.3)
VENTO = (-700, 0, 160)   # cm/s², no espaço da bola (X = voo): empurra para trás e as pontas sobem
# 08/11: nome novo. O asset NS_Katon_Gokakyu_v2 (mesmo conteúdo) nascia desativado no jogo depois das reconstruções
# no mesmo caminho, enquanto uma cópia idêntica com outro nome funciona; ele fica no projeto, sem uso
montar("NS_Katon_Gokakyu_v2_Voo", [
    # 2 fitas só (abraçam e se dissolvem); 4 davam cara de bola de energia
    # 09/11: fitas fora (arcos rígidos liam como bola de energia; o fogo agora é de labaredas)
    # (MESHT, "Fita01", me(meshes=["SM_Katon_Ribbon_01"], spin=0.8, **FITA)),
    # (MESHT, "Fita03", me(meshes=["SM_Katon_Ribbon_03"], spin=-1.2, **FITA)),
    # chamas queimando: flipbook nascendo na superfície, empurradas para trás, renovando sempre
    (FOUNTAIN, "Labaredas", sp(life=(0.3, 0.45), size=(V * 1.5, V * 2.2), speed=(150, 350), cone=0,
                               axis=(1, 0, 0), radius=V * 0.85, material="MI_Katon_Labareda", rate=28, inherit=0.85,
                               surface=True, from_point=True, flipbook=True, rise=VENTO, wind_local=True, drag=1.0,
                               sort=2, **VOO)),
    (FOUNTAIN, "LabaredasTras", sp(life=(0.45, 0.6), size=(V * 1.8, V * 2.7), speed=(100, 250), cone=0,
                                   axis=(1, 0, 0), radius=V * 0.75, material="MI_Katon_Labareda", rate=20,
                                   inherit=0.7, surface=True, from_point=True, flipbook=True,
                                   offset=(-V * 0.45, 0, 0), rise=VENTO, wind_local=True, drag=1.0, sort=1, **VOO)),
    # contorno VERMELHO: labaredas por fora (#E0281C -> #8A1A10, pontas transparentes), mais densas atrás
    (FOUNTAIN, "BordaVermelha", sp(life=(0.35, 0.5), size=(V * 1.6, V * 2.4), speed=(150, 300), cone=0,
                                   axis=(1, 0, 0), radius=V * 1.05, material="MI_Katon_BordaVermelha", rate=26,
                                   inherit=0.8, surface=True, from_point=True, flipbook=True, rise=VENTO,
                                   wind_local=True, drag=1.0, sort=0, **VOO)),
    (FOUNTAIN, "BordaVermelhaTras", sp(life=(0.45, 0.65), size=(V * 2.0, V * 3.0), speed=(100, 250), cone=0,
                                       axis=(1, 0, 0), radius=V * 0.95, material="MI_Katon_BordaVermelha", rate=24,
                                       inherit=0.65, surface=True, from_point=True, flipbook=True,
                                       offset=(-V * 0.6, 0, 0), rise=VENTO, wind_local=True, drag=1.0, sort=-1, **VOO)),
    # 09/11 custo: 50 sprites de ~3,8 m vivos davam 2,8 ms de translúcido; espaçamento 85 e 1,6-2,4 V
    # cauda contínua: chamas grandes soltas por distância no mundo, muito sobrepostas (coluna sem "carimbos"),
    # quase sem herdar a velocidade (ficam no ar), curvando com curl lento e esfriando com a idade
    (FOUNTAIN, "CaudaFogo", sp(life=(0.55, 0.85), size=(V * 1.6, V * 2.4), speed=(50, 200), cone=40, axis=(-1, 0, 0),
                               radius=V * 0.6, material="MI_Katon_CaudaFogo", spacing=85, rate=12, inherit=0.05,
                               rise=(0, 0, 180), drag=1.2, curl=(250, 0.25), flipbook=True, sort=0, **VOO)),
    # fumaça: pouca, escura-avermelhada translúcida, saindo de trás e do fim da cauda, subindo devagar
    (FOUNTAIN, "Fumaca", sp(life=(1.0, 1.5), size=(V * 2.2, V * 3.2), speed=(20, 80), cone=50, axis=(-1, 0, 0),
                            radius=V * 0.5, material="MI_Katon_Fumaca", spacing=120, rate=6, inherit=0.0,
                            offset=(-V * 0.6, 0, 0), rise=(0, 0, 120), drag=1.0, curl=(150, 0.3),
                            color=(1, 1, 1, 0.55), sort=-3, **VOO)),
    # fogo no chão ao longo do caminho (~1 s): chamas pequenas caem rápido e param no chão (colisão)
    (FOUNTAIN, "RastroChao", sp(life=(0.9, 1.2), size=(60 * K, 100 * K), speed=(2500, 3000), cone=5,
                                axis=(0, 0, -1), radius=V * 0.4, material="MI_Katon_CaudaFogo", spacing=160, rate=4,
                                inherit=0.0, rise=(0, 0, -3000), drag=0.0, collision=True, flipbook=True, sort=0,
                                **VOO)),
    # Spark Trail / Front Sparks / halo (como aprovados)
    (FOUNTAIN, "SparkTrail", sp(life=(0.5, 0.9), size=(6, 14), speed=(150, 400), cone=70, axis=(-1, 0, 0.3),
                                radius=V * 0.9, material="MI_Katon_Brasa", spacing=55, rate=0, inherit=0.3,
                                curl=(220, 1.0), stretch=(1.0, 2.5), rise=(0, 0, 150), drag=1.5, sort=4)),
    (FOUNTAIN, "FrontSparks", sp(life=(0.15, 0.25), size=(6, 12), speed=(300, 700), cone=60, axis=(1, 0, 0),
                                 radius=V * 0.9, material="MI_Katon_Brasa", spacing=70, rate=0, inherit=0.6,
                                 curl=(600, 2.0), surface=True, drag=2.0, sort=4)),
    (FOUNTAIN, "Halo", sp(life=(0.35, 0.45), size=(145 * K, 175 * K), speed=(0, 1), cone=0, axis=(1, 0, 0),
                          radius=5, material="MI_Katon_Halo", rate=10, local=True, sort=-1, rot=0)),
], unreal.Vector(3000, 1800, 1800))

# ================================================================================================ impacto
# (o C++ cria o impacto com escala 2,4). Sem fitas abrindo (pareciam arco-íris soltos); fogo residual ~2 s e fumaça
montar("NS_Katon_Gokakyu_v2_Impacto", [
    (MESHT, "Onda", me(meshes=["SM_Katon_Shockwave"], material="MI_Katon_Onda", life=0.5, scale=1.0, sort=1)),
    (MESHT, "Estouro", me(meshes=["SM_Katon_Shell"], material="MI_Katon_Estouro", life=0.3, scale=1.2,
                          offset=(0, 0, 40), spin=0.5, sort=2)),
    # 09/11 custo: perto da câmera o impacto dava ~2,4 ms de translúcido; menos chamas no estouro (64 -> 36, 32 -> 20)
    (FOUNTAIN, "Chamas", sp(life=(0.4, 0.7), size=(240, 420), speed=(500, 1100), cone=70, axis=(0, 0, 1), radius=50,
                            material="MI_Katon_Lingua", rate=450, rise=(0, 0, 300), drag=4.0, local=True,
                            flipbook=True, sort=3, burst=(0.08, 0.0))),
    (FOUNTAIN, "ChamasVermelhas", sp(life=(0.5, 0.8), size=(300, 480), speed=(300, 800), cone=80, axis=(0, 0, 1),
                                     radius=60, material="MI_Katon_BordaVermelha", rate=250, rise=(0, 0, 250),
                                     drag=3.5, local=True, flipbook=True, sort=1, burst=(0.08, 0.0))),
    (FOUNTAIN, "Brasas", sp(life=(0.5, 1.0), size=(8, 16), speed=(400, 1000), cone=80, axis=(0, 0, 1), radius=40,
                            material="MI_Katon_Brasa", rate=300, rise=(0, 0, 150), drag=1.5, local=True,
                            stretch=(1.0, 3.0), sort=4, burst=(0.08, 0.0))),
    (FOUNTAIN, "Clarao", sp(life=(0.35, 0.4), size=(700, 850), speed=(0, 1), cone=0, axis=(0, 0, 1), radius=4,
                            material="MI_Katon_HaloFlash", rate=60, local=True, offset=(0, 0, 60), sort=-1,
                            burst=(0.03, 0.0), rot=0)),
    # fogo residual no chão ~2 s (nasce no 1º décimo de segundo: o C++ desativa o impacto em 0,48 s)
    (FOUNTAIN, "ChaoResidual", sp(life=(1.6, 2.2), size=(120, 220), speed=(10, 60), cone=20, axis=(0, 0, 1),
                                  radius=120, flat=(1, 1, 0.05), material="MI_Katon_Lingua", rate=150,
                                  rise=(0, 0, 60), drag=2.0, local=True, flipbook=True, sort=0, burst=(0.1, 0.0))),
    (FOUNTAIN, "FumacaImpacto", sp(life=(1.5, 2.2), size=(300, 500), speed=(50, 150), cone=40, axis=(0, 0, 1),
                                   radius=80, material="MI_Katon_Fumaca", rate=60, rise=(0, 0, 150), drag=1.0,
                                   local=True, color=(1, 1, 1, 0.55), sort=-2, burst=(0.15, 0.05))),
    # fogo residual do pack (cópia; o original não é tocado)
    (PACK + "NE_FireFlame", "FogoResidual", lambda s, em: type("X", (), {"values": lambda self: None})()),
], unreal.Vector(2600, 2600, 2000))
print("GOKAKYU_V2_NIAGARA_OK")
