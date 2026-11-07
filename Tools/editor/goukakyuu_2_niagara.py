"""Gōkakyū, etapa 1: saída da boca e voo até o impacto, com os FOGOS PRONTOS do Perfect Fire (Vefects).

Pedido do Felipe (06/11): usar as chamas do pack, com o material e os emissores originais da Vefects, mudando só
números (duração, quantidade, velocidade, tamanho, espaço local/mundo) para virar um "cuspe" em vez de fogueira.
Cria duas CÓPIAS do NS_Fire_Big (o pack original não é alterado):
- NS_Katon_Goukakyuu_Saida: dispara UMA vez na boca, virado para a mira (X local = mira), tudo em espaço local
- NS_Katon_Goukakyuu_Voo:   preso à bola; NE_Flame_01 = corpo (segue a bola), NE_Flame_02 = cauda (fica no mundo)
Fagulhas, cinzas, calor e luz são os do próprio pack (a luz do código fica apagada nesse modo).
Se o asset existir mas não for cópia do pack (versão antiga remontada), ele vai para /Game/_ToDelete.
Pode rodar de novo: só reaplica os números. Rodar com o editor aberto.
"""
import re
import time
import unreal

EAL = unreal.EditorAssetLibrary
NT = unreal.NinjaEditorTools
FOLDER = "/Game/HypeWorld/Jutsus/Katon/VFX/Goukakyuu"
PACK = "/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Big"
SELF, ONCE = "NewEnumerator1", "NewEnumerator2"   # Life Cycle: o próprio emissor; Loop Behavior: uma vez só


def copy_of_pack(name, src=PACK):
    path = f"{FOLDER}/{name}"
    if EAL.does_asset_exist(path):
        old = unreal.load_asset(path)
        if "== Emissor NE_Flame_01" in NT.describe_niagara_system(old, False):
            return old
        dest = f"/Game/_ToDelete/{name}_remontado_{int(time.time())}"
        assert EAL.rename_asset(path, dest), dest
        print("versão remontada movida para", dest)
    s = EAL.duplicate_asset(src, path)
    assert s, path
    return s


def const(s, em, param, value, required=True):
    n = NT.set_niagara_constant(s, em, param, value)
    assert n > 0 or not required, f"{em}.{param}: {n}"


def pin(s, em, module, name, value):
    assert NT.set_niagara_module_pin(s, em, module, name, value), f"{em}.{module}.{name}"


def vec(v):
    return ",".join(map(str, v))


def tune(s, em, *, rate=None, life=None, size=None, vel=None, radius=None, local=None, once=None, grow=None,
         tint=None, sort=None):
    """Muda só números de um emissor do pack. once = duração em s (dispara uma vez só);
    grow = multiplica a curva de tamanho ao longo da vida do próprio pack (>1 = a chama cresce mais);
    tint = "Scale Color" do módulo Color do pack (multiplica a cor da curva; <1 no verde/azul = mais laranja/vermelho)."""
    if sort is not None:
        # ordem de desenho entre camadas (maior = desenhado por cima): miolo > laranja > vermelho > rastro
        assert NT.set_niagara_renderer_property(s, em, 0, "SortOrderHint", str(sort)), em
    if grow is not None:
        const(s, em, "ScaleSpriteSize.Uniform Curve Scale", str(grow))
    if tint is not None:
        const(s, em, "Color.Scale Color", vec(tint))
    if local is not None:
        assert NT.set_niagara_emitter_property(s, em, "bLocalSpace", "True" if local else "False"), em
    if rate is not None:
        const(s, em, "SpawnRate.SpawnRate", str(rate))
    if life:
        const(s, em, "InitializeParticle.Lifetime Min", str(life[0]))
        const(s, em, "InitializeParticle.Lifetime Max", str(life[1]))
    if size:
        const(s, em, "InitializeParticle.Uniform Sprite Size Min", str(size[0]))
        const(s, em, "InitializeParticle.Uniform Sprite Size Max", str(size[1]))
    if vel:
        const(s, em, "RandomRangeVector.Minimum", vec(vel[0]))
        const(s, em, "RandomRangeVector.Maximum", vec(vel[1]))
        const(s, em, "SolveForcesAndVelocity.Speed Limit", "5000", required=False)
    if radius is not None:
        const(s, em, "SphereLocation.Sphere Radius", str(radius))
        const(s, em, "SphereLocation.Non Uniform Scale", "1,1,1")
    if once is not None:
        pin(s, em, "EmitterState", "Life Cycle Mode", SELF)
        pin(s, em, "EmitterState", "Loop Behavior", ONCE)
        const(s, em, "EmitterState.Loop Duration", str(once), required=False)


# ------------------------------------------------- saída: um cuspe curto na direção da mira, que se funde com a bola
saida = copy_of_pack("NS_Katon_Goukakyuu_Saida")
# teste 8 (06/11, Felipe: "quando o fogo entra em contato com o oxigênio ele explode"): a saída vira uma explosão
# de 6-8 m na boca (prints 01-02): muitas chamas grandes do pack, para todos os lados com mais força para a frente,
# crescendo ao longo da vida; clarão de luz bem mais forte
# teste 9: estouro ~4 m e quase todo branco-amarelo -> maior, mais espalhado e com laranja/vermelho por fora
# teste 10: ainda ~4,5 m e claro demais -> perto do dobro e com as chamas de fora mais vermelhas
# teste 12 (Felipe: "o brilho está fraco e dura muito pouco, mal dá para perceber"): chamas vivem ~50% mais e
# brilham mais (sem voltar ao branco do teste 10); o clarão de luz/bloom vem do código do projétil
# teste 13: chamas da saída esticadas até o alvo também acendiam o caminho -> vida um pouco menor
tune(saida, "NE_Flame_01", local=True, once=0.1, rate=450, life=(0.45, 0.65), size=(650, 950), grow=2.0,
     vel=((-600, -1200, -150), (1300, 1200, 1000)), radius=200, tint=(0.85, 0.38, 0.1))
tune(saida, "NE_Flame_02", local=True, once=0.1, rate=450, life=(0.35, 0.55), size=(450, 700), grow=1.8,
     vel=((200, -1100, 0), (1900, 1100, 800)), radius=170, tint=(0.7, 0.18, 0.04))
# teste 11: com tint ~1 o brilho do pack (emissivo 20-40) estourava em branco; menos brilho + menos verde/azul
# deixa a explosão laranja/vermelha por fora como no print 02 (o miolo, onde as chamas se somam, continua claro)
assert NT.set_niagara_renderer_property(
    saida, "NE_Flame_02", 0, "Material",
    "/Game/Vefects/Free_Fire/Shared/Materials/MI_VFX_Fire_01.MI_VFX_Fire_01")
# clarão da explosão: luz do pack mais forte e mais avermelhada (no Zenkai a pedra e o paredão ficam rosa e vermelhos)
# teste 9: com sol forte a luz do pack quase não aparecia -> muito mais forte e com alcance menor (concentrada)
assert NT.set_niagara_renderer_property(saida, "NE_Lights", 0, "ColorAdd", "(X=1500000.0,Y=450000.0,Z=0.0)")
assert NT.set_niagara_renderer_property(saida, "NE_Lights", 0, "RadiusScale", "0.6")
tune(saida, "NE_Ashes", local=True, once=0.1, rate=40, life=(0.4, 0.6))
tune(saida, "NE_Heat", local=True, once=0.15, rate=80)
# teste 13: com vida 0,6-0,9 as luzes do pack voavam para a frente e deixavam um rastro aceso no chão até o alvo
# -> vida curta: só o estouro na boca
tune(saida, "NE_Lights", local=True, once=0.1, rate=80, life=(0.15, 0.3))
# fagulhas do pack: o estouro já existe; sem a taxa contínua, mais traços, para todos os lados e mais para a frente
tune(saida, "NE_Sparkles", local=True, once=0.3, rate=0, life=(0.25, 0.4), vel=((300, -1200, -400), (2400, 1200, 1000)))
const(saida, "NE_Sparkles", "RandomRangeInt.Minimum", "24")
const(saida, "NE_Sparkles", "RandomRangeInt.Maximum", "32")
assert NT.compile_niagara_system(saida, True, unreal.Vector(2500, 2500, 2500))
EAL.save_loaded_asset(saida, False)

# ------------------------------------------------- voo: miolo branco-amarelo + casca laranja (presos à bola)
# 06/11 (Felipe, câmera do jogo): a bola era um "pompom" amarelo pequeno, sem camadas nem rastro. Referência Zenkai:
# miolo branco-amarelo -> camada laranja fina -> casca vermelha por fora -> rastro de luz vermelha/fumaça levado pelo
# vento. Este sistema = as 2 camadas de dentro; o NS_Katon_Goukakyuu_Rastro = casca vermelha + rastro + fumaça.
voo = copy_of_pack("NS_Katon_Goukakyuu_Voo")
tune(voo, "NE_Flame_01", local=True, rate=120, life=(0.25, 0.4), size=(420, 560), sort=3,
     vel=((-100, -100, -60), (100, 100, 150)), radius=70, tint=(1.0, 0.85, 0.6))
tune(voo, "NE_Flame_02", local=True, rate=90, life=(0.25, 0.4), size=(600, 800), sort=2, tint=(1.0, 0.42, 0.1),
     vel=((-150, -150, -80), (150, 150, 200)), radius=140)
assert NT.set_niagara_renderer_property(
    voo, "NE_Flame_02", 0, "Material",
    "/Game/Vefects/Free_Fire/Shared/Materials/MI_VFX_Fire_01.MI_VFX_Fire_01")
# teste 7: cinzas e fagulhas do pack para trás deixavam a cauda "granulada" -> poucas
tune(voo, "NE_Ashes", local=False, rate=5, life=(0.3, 0.5))
tune(voo, "NE_Sparkles", local=False, rate=8)
tune(voo, "NE_Lights", local=True, rate=12, life=(0.3, 0.5))
# luz do pack: mesma cor laranja (60000; 24000; 0), o dobro de forte para pintar o chão como no Zenkai
assert NT.set_niagara_renderer_property(voo, "NE_Lights", 0, "ColorAdd", "(X=500000.0,Y=160000.0,Z=0.0)")
assert NT.set_niagara_renderer_property(voo, "NE_Lights", 0, "RadiusScale", "0.5")
tune(voo, "NE_Heat", local=True, rate=15)
assert NT.compile_niagara_system(voo, False, unreal.Vector(0, 0, 0))
EAL.save_loaded_asset(voo, False)

# ------------------------------------------------- rastro: casca vermelha + rastro de chamas e fumaça com vento
# cópia do NS_Fire_Big_Smoke (mesmas chamas + as 3 fumaças do pack). As chamas do rastro e a fumaça ficam no mundo
# (a bola anda 30 m/s e elas ficam para trás) e o "vento" empurra tudo para o lado (+Y) e para cima
# teste 14 (câmera do jogo): vento (250-550 para o lado) espalhava as chamas do rastro como pétalas soltas -> vento
# mais fraco, rastro mais denso e com chamas maiores (uma faixa contínua) e fumaça mais visível
# teste 15 (07/11, câmera do jogo): com esse vento o rastro andava só ~0,7 m para o lado em 0,4 s e ficava escondido
# atrás da bola. O que espalhava era a FAIXA larga de velocidades sorteadas, não a força: vento forte e quase igual
# para todas as partículas = elas andam juntas e o rastro vira uma faixa inteira deslocada pelo vento (não pétalas)
VENTO_MIN, VENTO_MAX = (-40, 430, 190), (40, 500, 240)
rastro = copy_of_pack("NS_Katon_Goukakyuu_Rastro", "/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Big_Smoke")
# Medido 07/11 (sistema sozinho andando 18 m/s, câmera de cima): o AddVelocity do pack segue a rotação do sistema
# (= a bola), então o vento (+Y, +Z) sopra para a DIREITA da bola e para cima em qualquer direção de arremesso.
# As chamas do pack só aparecem bem no começo da vida (o rastro ficava ~5 m e branco de tanta chama somada) ->
# vivem mais e são bem menos fortes (vermelho escuro somado vira vermelho vivo, não branco).
# Perto da câmera o rastro e a fumaça não são desenhados (na saída cobriam a tela de vermelho).
def longe_da_camera(em, dist=600):
    assert NT.set_niagara_renderer_property(rastro, em, 0, "bEnableCameraDistanceCulling", "True"), em
    assert NT.set_niagara_renderer_property(rastro, em, 0, "MinCameraDistance", str(dist)), em


# casca vermelha: mais densa e mais junta (raio 190 -> 150) para não abrir em línguas soltas
tune(rastro, "NE_Flame_01", local=True, rate=110, life=(0.25, 0.4), size=(700, 900), sort=1,
     vel=((-120, -120, -60), (120, 120, 180)), radius=150, tint=(0.5, 0.07, 0.015))
# rastro de chamas: nasce menor e cresce enquanto o vento leva
# teste 16: com vida 0,9-1,2 s o fim da vida das chamas do pack (fiapos finos) virava uma cortina de pétalas
# vermelhas cobrindo meia tela -> vida curta (cauda de cometa junto da bola); o rastro longo é a luz/fumaça abaixo
tune(rastro, "NE_Flame_02", local=False, rate=320, life=(0.35, 0.5), size=(380, 520), grow=1.6, sort=0,
     vel=(VENTO_MIN, VENTO_MAX), radius=110, tint=(0.38, 0.045, 0.01))
assert NT.set_niagara_renderer_property(
    rastro, "NE_Flame_02", 0, "Material",
    "/Game/Vefects/Free_Fire/Shared/Materials/MI_VFX_Fire_01.MI_VFX_Fire_01")
longe_da_camera("NE_Flame_02")
# fumaça: o material do pack (MI_VFX_Smoke, iluminado e corroído com a vida) quase some no dia claro — nem o
# NS_Fire_Big_Smoke original mostra fumaça neste mapa. Cor escura e alfa bem acima de 1 (o alfa da partícula
# multiplica a opacidade do material), maior e vivendo mais que as chamas: fica atrás e por fora do fogo
# teste 17: com o material de fumaça (brilho 1) o vermelho virou uma nuvem marrom-escura. A "faixa de luz
# vermelha" usa o material de chama rasteira do próprio pack (MI_VFX_FireQuad_02: brilho 20, máscara de poeira
# esticada = macia, sem as línguas do MI_VFX_Fire_01), vermelho escuro (brilha vermelho, não branco), com vento
tune(rastro, "NE_Smokes_02", local=False, rate=130, life=(0.6, 0.9), size=(750, 1000), sort=-1,
     vel=((-40, 380, 180), (40, 440, 230)), radius=120, tint=(0.55, 0.05, 0.01))
const(rastro, "NE_Smokes_02", "Color.Scale Alpha", "1.0")
assert NT.set_niagara_renderer_property(
    rastro, "NE_Smokes_02", 0, "Material",
    "/Game/Vefects/Free_Fire/Shared/Materials/MI_VFX_FireQuad_02.MI_VFX_FireQuad_02")
const(rastro, "NE_Smokes_02", "SphereLocation.Offset", "0,0,0")
longe_da_camera("NE_Smokes_02")
# fumaça escura de vida longa do pack, mais fraca, para a cauda do rastro não acabar seca
# teste 17: com alfa 3 (+ a 02 marrom) virou uma parede preta do lado -> mais transparente e menor; 1,2 sumia
tune(rastro, "NE_Smokes_01", local=False, rate=25, life=(1.3, 1.8), size=(700, 950), sort=-2,
     vel=((-40, 500, 260), (40, 580, 330)), radius=130, tint=(0.16, 0.12, 0.11))
const(rastro, "NE_Smokes_01", "Color.Scale Alpha", "2.2")
const(rastro, "NE_Smokes_01", "SphereLocation.Offset", "0,0,0")
longe_da_camera("NE_Smokes_01")
tune(rastro, "NE_Smokes_03", rate=0)
tune(rastro, "NE_Ashes", local=False, rate=0)
tune(rastro, "NE_Heat", local=True, rate=0)
tune(rastro, "NE_Sparkles", local=False, rate=10)
tune(rastro, "NE_Lights", local=True, rate=10, life=(0.3, 0.5))
assert NT.set_niagara_renderer_property(rastro, "NE_Lights", 0, "ColorAdd", "(X=400000.0,Y=40000.0,Z=0.0)")
assert NT.set_niagara_renderer_property(rastro, "NE_Lights", 0, "RadiusScale", "0.5")
assert NT.compile_niagara_system(rastro, False, unreal.Vector(0, 0, 0))
EAL.save_loaded_asset(rastro, False)
print("GOUKAKYUU_NIAGARA_OK")
