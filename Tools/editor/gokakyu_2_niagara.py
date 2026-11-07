"""Gōkakyū novo (07/11): sistemas Niagara do NS_Katon_Gokakyu (voo) e NS_Katon_Gokakyu_Saida (estouro na boca).

O núcleo e as cascas de chamas são malhas no projétil (C++, M_FireToon); aqui ficam as partículas:
- NS_Katon_Gokakyu (preso à bola, X = direção do voo):
    Chamas  (modelo Fountain do Niagara): poucas chamas soltas com contorno nítido (MI_FireToon_Chama) saindo para trás
            e subindo; somem pela erosão do material (o alfa da cor cai com a vida)
    Rastro  (modelo LocationBasedRibbon): fita de fogo atrás da bola, afinando e se dissolvendo (MI_FireToon_Rastro)
    Faiscas (emissor NE_Sparkles do Perfect Fire): brasas pequenas e brilhantes para os lados
    Fumaca  (emissor NE_Smokes do Perfect Fire): mínima e clara
- NS_Katon_Gokakyu_Saida (parado na boca, X = mira): estouro de chamas para a frente + faíscas, uma vez só.
Os sistemas antigos (NS_Katon_Goukakyuu_*) não são tocados. Pode rodar de novo (recria os dois; a versão anterior vai
para /Game/_ToDelete). Rodar com o editor aberto e fora do Play.
"""
import time
import unreal

EAL = unreal.EditorAssetLibrary
NT = unreal.NinjaEditorTools
FOLDER = "/Game/HypeWorld/Jutsus/Katon/VFX/Gokakyu"
T = "/Niagara/DefaultAssets/Templates/Emitters/"
PACK = "/Game/Vefects/Free_Fire/Shared/Particles/"
ESTILO = getattr(unreal, "_gokakyu_estilo", "Soft")   # "Soft" = estilo Zenkai (padrão desde 07/11); "Toon" = contorno
MI = FOLDER + "/MI_Fire" + ESTILO + "_{0}.MI_Fire" + ESTILO + "_{0}"
SELF, ONCE = "NewEnumerator1", "NewEnumerator2"


def new_system(name, emitters):
    path = f"{FOLDER}/{name}"
    if EAL.does_asset_exist(path):
        dest = f"/Game/_ToDelete/{name}_{int(time.time())}"
        assert EAL.rename_asset(path, dest), dest
        print("versão anterior movida para", dest)
    ems = [unreal.load_asset(src) for src, _ in emitters]
    assert all(ems), emitters
    s = NT.create_niagara_system(FOLDER, name, ems, [n for _, n in emitters])
    assert s, name
    return s


def const(s, em, param, value, required=True):
    n = NT.set_niagara_constant(s, em, param, value)
    assert n > 0 or not required, f"{em}.{param}: {n}"


def vec(v):
    return ",".join(map(str, v))


def rend(s, em, prop, value):
    assert NT.set_niagara_renderer_property(s, em, 0, prop, value), f"{em}.{prop}"


def emitter(s, em, prop, value):
    assert NT.set_niagara_emitter_property(s, em, prop, value), f"{em}.{prop}"


def once(s, em, dur):
    assert NT.set_niagara_module_pin(s, em, "EmitterState", "Life Cycle Mode", SELF), em
    assert NT.set_niagara_module_pin(s, em, "EmitterState", "Loop Behavior", ONCE), em
    const(s, em, "EmitterState.Loop Duration", str(dur), required=False)


def chamas(s, em, *, rate, life, size, speed, cone, axis, radius, rise, drag=1.5):
    emitter(s, em, "bLocalSpace", "False")
    const(s, em, "SpawnRate.SpawnRate", str(rate))
    const(s, em, "InitializeParticle.Lifetime Min", str(life[0]))
    const(s, em, "InitializeParticle.Lifetime Max", str(life[1]))
    const(s, em, "InitializeParticle.Uniform Sprite Size Min", str(size[0]))
    const(s, em, "InitializeParticle.Uniform Sprite Size Max", str(size[1]))
    const(s, em, "RandomRangeFloat.Minimum", str(speed[0]))
    const(s, em, "RandomRangeFloat.Maximum", str(speed[1]))
    const(s, em, "AddVelocity.Cone Axis", vec(axis))
    const(s, em, "AddVelocity.Cone Angle", str(cone))
    const(s, em, "ShapeLocation.Sphere Radius", str(radius))
    const(s, em, "GravityForce.Gravity", vec((0, 0, rise)))
    const(s, em, "Drag.Drag", str(drag))
    const(s, em, "SolveForcesAndVelocity.Speed Limit", "5000")
    rend(s, em, "Material", MI.format("Chama"))


# ------------------------------------------------------------------------------------------------ voo
voo = new_system("NS_Katon_Gokakyu", [(T + "Fountain", "Chamas"), (T + "LocationBasedRibbon", "Rastro"),
                                      (PACK + "NE_Sparkles", "Faiscas"), (PACK + "NE_Smokes", "Fumaca")])
# chamas soltas: poucas, nascem na borda da bola e vão para trás (eixo -X da bola) subindo; vida curta
chamas(voo, "Chamas", rate=45, life=(0.22, 0.38), size=(110, 190), speed=(250, 500), cone=35, axis=(-1, 0, 0.25),
       radius=90, rise=500)
# rastro: fita do tamanho da bola, vive pouco (afina e some pela erosão do material)
emitter(voo, "Rastro", "bLocalSpace", "False")
const(voo, "Rastro", "InitializeParticle.Lifetime", "0.3")
const(voo, "Rastro", "InitializeParticle.Ribbon Width", "230")
rend(voo, "Rastro", "Material", MI.format("Rastro"))
# faíscas do pack: poucas, para os lados
emitter(voo, "Faiscas", "bLocalSpace", "False")
const(voo, "Faiscas", "SpawnRate.SpawnRate", "30", required=False)
# fumaça do pack: mínima e clara
emitter(voo, "Fumaca", "bLocalSpace", "False")
const(voo, "Fumaca", "SpawnRate.SpawnRate", "6", required=False)
const(voo, "Fumaca", "Color.Scale Alpha", "0.25", required=False)
const(voo, "Fumaca", "Color.Scale Color", "1.3,1.25,1.2", required=False)
const(voo, "Fumaca", "InitializeParticle.Lifetime Min", "0.6", required=False)
const(voo, "Fumaca", "InitializeParticle.Lifetime Max", "0.9", required=False)
assert NT.compile_niagara_system(voo, False, unreal.Vector(0, 0, 0))
EAL.save_loaded_asset(voo, False)
print(NT.describe_niagara_system(voo, False)[:200])

# ------------------------------------------------------------------------------------------------ saída
saida = new_system("NS_Katon_Gokakyu_Saida", [(T + "Fountain", "Estouro"), (PACK + "NE_Sparkles", "Faiscas")])
chamas(saida, "Estouro", rate=170, life=(0.2, 0.34), size=(110, 210), speed=(500, 1200), cone=55, axis=(1, 0, 0.1),
       radius=60, rise=300, drag=4.0)
emitter(saida, "Estouro", "bLocalSpace", "True")
once(saida, "Estouro", 0.1)
emitter(saida, "Faiscas", "bLocalSpace", "True")
once(saida, "Faiscas", 0.2)
const(saida, "Faiscas", "RandomRangeInt.Minimum", "20", required=False)
const(saida, "Faiscas", "RandomRangeInt.Maximum", "28", required=False)
assert NT.compile_niagara_system(saida, True, unreal.Vector(1500, 1500, 1500))
EAL.save_loaded_asset(saida, False)
print("GOKAKYU_NIAGARA_OK")
