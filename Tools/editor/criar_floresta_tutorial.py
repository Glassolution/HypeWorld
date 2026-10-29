"""Mini floresta do tutorial (25/10/2026, pedido do Felipe): /Game/Maps/Tutorial/L_Tutorial_Forest.
- Landscape 252 x 252 m (4x4 componentes de 63 quads, 1 m por quad). Área jogável = oval de 120 x 80 m (semi-eixos 60/40 m),
  ondulação de até ~3 m, centro mais plano; fora do anel o chão sobe em morros (fundo).
- Camadas do landscape (pintáveis depois no modo Landscape): Grama e Terra (visuais, caminhos de terra) + máscaras sem
  mistura que o PCG lê: Mata (onde nascem árvores), Borda (arbustos perto das pedras), Vegetacao (grama/flores/pedrinhas).
- Anel de pedras grandes (2 fileiras desencontradas, 3 a 6 m acima do chão, enterradas) + Blocking Volume invisível atrás
  (80 m de altura: o super pulo sobe ~26 m).
- Paredões de pedra no fundo, sol a 40°, céu, Sky Light, névoa leve, pós-processo (bloom suave, saturação um pouco alta).
- PlayerStart no centro (clareira de 15 m de raio) e 4 marcadores do tutorial (Target Point + texto em cima).
A vegetação vem do PCG: Tools/editor/criar_pcg_floresta.py (rodar depois deste).
Rodar com ue_exec -f, editor fora do Play. Refaz o mapa do zero se rodar de novo.
"""
import math
import random
import unreal

MAPA = "/Game/Maps/Tutorial/L_Tutorial_Forest"
CAMADAS = "/Game/Maps/Tutorial/Camadas"
MESH = "/Game/Ninja/Ambiente/Floresta/Meshes/"
A, B = 60.0, 40.0              # semi-eixos do oval jogável (m)
CLAREIRA = 15.0                # raio da clareira do PlayerStart (m)
COMP, QUADS = 4, 63
N = COMP * QUADS + 1           # 253 vértices por lado
ORIGEM = -(N - 1) * 50.0       # canto do landscape (cm): centro do oval na origem
random.seed(7)


def suave(a, b, x):
    t = max(0.0, min(1.0, (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


def ondas(x, y):
    """Ondulação suave (~ -1..1), em metros"""
    return (0.55 * math.sin(x * 0.09 + 1.3) * math.cos(y * 0.11 + 0.4) + 0.35 * math.sin(x * 0.21 + y * 0.17 + 2.1)
            + 0.25 * math.sin(x * 0.05 - y * 0.07 + 0.8)) / 1.15


def oval(x, y):
    return math.sqrt((x / A) ** 2 + (y / B) ** 2)


def altura(x, y):
    """Altura do chão em metros (a mesma conta posiciona pedras, PlayerStart e marcadores)"""
    e = oval(x, y)
    h = 1.4 * ondas(x, y) * suave(10.0, 26.0, math.hypot(x, y))   # centro mais plano
    h += 0.6 * suave(0.9, 1.08, e)                                  # anel um pouco mais alto
    if e > 1.08:
        h += min(24.0, (e - 1.08) * 26.0) * (0.75 + 0.25 * ondas(x * 0.6, y * 0.6))   # morros do fundo
    return h


# Caminhos de terra: da borda da clareira até o anel (curvos)
CAMINHOS = []
for ang, curva in ((15.0, 6.0), (140.0, -7.0), (250.0, 5.0)):
    pts = []
    for k in range(41):
        t = k / 40.0
        a = math.radians(ang) + math.radians(curva) * math.sin(t * math.pi * 1.5)
        r_dir = 1.0 / math.sqrt((math.cos(a) / A) ** 2 + (math.sin(a) / B) ** 2)   # distância até o anel nessa direção
        r = CLAREIRA - 2.0 + t * (r_dir * 0.97 - CLAREIRA + 2.0)
        pts.append((math.cos(a) * r, math.sin(a) * r))
    CAMINHOS.append(pts)


def dist_caminho(x, y):
    best = 1e9
    for pts in CAMINHOS:
        for (px, py) in pts:
            d = (x - px) ** 2 + (y - py) ** 2
            if d < best:
                best = d
    return math.sqrt(best)


world_ctx = None


def ator(classe, loc, rot=None, nome=None, pasta=None):
    a = unreal.EditorLevelLibrary.spawn_actor_from_class(classe, loc, rot or unreal.Rotator(0, 0, 0))
    if nome:
        a.set_actor_label(nome)
    if pasta:
        a.set_folder_path(pasta)
    return a


def malha(mesh_nome, loc, rot, escala, nome, pasta, colisao=True):
    a = ator(unreal.StaticMeshActor, loc, rot, nome, pasta)
    c = a.static_mesh_component
    c.set_static_mesh(unreal.load_asset(MESH + mesh_nome))
    c.set_world_scale3d(escala)
    if not colisao:
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return a


def cm(x, y, z_extra=0.0):
    return unreal.Vector(x * 100.0, y * 100.0, (altura(x, y) + z_extra) * 100.0)


# ------------------------------------------------------------------ level ---------------------------------------------------
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.new_level(MAPA)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
print("level:", world.get_path_name())

# ------------------------------------------------------------------ landscape -----------------------------------------------
camadas = [("Grama", True), ("Terra", True), ("Mata", False), ("Borda", False), ("Vegetacao", False)]
infos = [unreal.NinjaEditorTools.create_landscape_layer_info("%s/LI_%s" % (CAMADAS, nome), nome, mistura) for nome, mistura in camadas]
for li in infos:
    unreal.EditorAssetLibrary.save_loaded_asset(li, False)

alturas = []
pesos = {nome: [] for nome, _ in camadas}
for j in range(N):
    y = (ORIGEM + j * 100.0) / 100.0
    for i in range(N):
        x = (ORIGEM + i * 100.0) / 100.0
        alturas.append(int(round(32768 + altura(x, y) * 100.0 * 128.0 / 100.0)))
        e = oval(x, y)
        r = math.hypot(x, y)
        dc = dist_caminho(x, y) if e < 1.05 else 99.0
        terra = (1.0 - suave(1.4, 2.6, dc)) * (1.0 - suave(0.97, 1.02, e))
        pesos["Terra"].append(int(round(255 * terra)))
        pesos["Grama"].append(255 - int(round(255 * terra)))
        mata = suave(CLAREIRA + 2.0, CLAREIRA + 6.0, r) * suave(3.5, 6.0, dc) * (1.0 - suave(0.86, 0.92, e))
        pesos["Mata"].append(int(round(255 * mata)))
        borda = suave(0.72, 0.8, e) * (1.0 - suave(0.93, 0.97, e)) * suave(2.5, 4.0, dc)
        pesos["Borda"].append(int(round(255 * borda)))
        veg = (1.0 - suave(0.95, 0.99, e)) * suave(1.8, 2.8, dc)
        pesos["Vegetacao"].append(int(round(255 * veg)))
tudo = []
for nome, _ in camadas:
    tudo += pesos[nome]
mat_chao = unreal.load_asset("/Game/Ninja/Ambiente/Floresta/Materiais/M_Chao_Floresta")
land = unreal.NinjaEditorTools.create_landscape(world, unreal.Vector(ORIGEM, ORIGEM, 0.0), unreal.Vector(100.0, 100.0, 100.0),
                                                COMP, COMP, QUADS, alturas, mat_chao, infos, tudo)
land.set_actor_label("Chao_Floresta")
land.set_folder_path("Terreno")
print("landscape:", land.get_name())

# ------------------------------------------------------------------ anel de pedras ------------------------------------------
GRANDES = ["SM_Pedra_Grande_A", "SM_Pedra_Grande_B", "SM_Pedra_Grande_C"]
ALTURA_BASE = {"SM_Pedra_Grande_A": 4.4, "SM_Pedra_Grande_B": 3.2, "SM_Pedra_Grande_C": 3.9}
n_pedras = 0
for fileira, (fator, passo, esc) in enumerate(((1.0, 4.3, (1.0, 1.35)), (1.075, 5.0, (1.3, 1.7)))):
    a_, b_ = A * fator, B * fator
    perim = 2 * math.pi * math.sqrt((a_ * a_ + b_ * b_) / 2)
    qtd = int(perim / passo)
    for k in range(qtd):
        t = 2 * math.pi * (k + (0.5 if fileira else 0.0) + random.uniform(-0.15, 0.15)) / qtd
        x, y = a_ * math.cos(t), b_ * math.sin(t)
        nome = random.choice(GRANDES)
        s = random.uniform(*esc)
        alvo = random.uniform(3.2, 6.0) if fileira == 0 else random.uniform(4.0, 6.0)   # altura acima do chão (m)
        sz = (alvo + 1.0) / ALTURA_BASE[nome]                                         # +1 m enterrado
        rot = unreal.Rotator(pitch=random.uniform(-8, 8), yaw=random.uniform(0, 360), roll=random.uniform(-8, 8))
        malha(nome, cm(x, y, -1.0), rot, unreal.Vector(s, s, sz), "Pedra_Anel_%d_%02d" % (fileira + 1, k), "Borda/Pedras")
        n_pedras += 1
print("pedras do anel:", n_pedras)

# ------------------------------------------------------------------ bloqueio invisível --------------------------------------
SEG = 48
a_, b_ = A * 1.15, B * 1.15
for k in range(SEG):
    t0, t1 = 2 * math.pi * k / SEG, 2 * math.pi * (k + 1) / SEG
    p0 = (a_ * math.cos(t0), b_ * math.sin(t0))
    p1 = (a_ * math.cos(t1), b_ * math.sin(t1))
    mx, my = (p0[0] + p1[0]) / 2, (p0[1] + p1[1]) / 2
    comp = math.hypot(p1[0] - p0[0], p1[1] - p0[1]) * 1.2
    yaw = math.degrees(math.atan2(p1[1] - p0[1], p1[0] - p0[0]))
    v = unreal.NinjaEditorTools.spawn_blocking_box(world, unreal.Vector(mx * 100, my * 100, 3500.0), unreal.Rotator(pitch=0, yaw=yaw, roll=0),
                                                   unreal.Vector(comp * 100, 300.0, 9000.0))
    v.set_actor_label("Bloqueio_%02d" % k)
    v.set_folder_path("Borda/Bloqueio")
print("bloqueio:", SEG, "caixas")

# ------------------------------------------------------------------ paredões do fundo ---------------------------------------
for k in range(26):
    t = 2 * math.pi * (k + random.uniform(-0.3, 0.3)) / 26
    f = random.uniform(1.55, 1.85)
    x, y = A * f * math.cos(t), B * f * 1.25 * math.sin(t)
    s = random.uniform(5.0, 8.5)
    malha(random.choice(GRANDES), cm(x, y, -4.0), unreal.Rotator(pitch=random.uniform(-6, 6), yaw=random.uniform(0, 360), roll=random.uniform(-6, 6)),
          unreal.Vector(s, s, s * random.uniform(1.1, 1.6)), "Paredao_%02d" % k, "Fundo", colisao=False)

# ------------------------------------------------------------------ obstáculos do pulo (perto do TP_Pular) ------------------
TP = {"TP_Andar": 200.0, "TP_Pular": 95.0, "TP_Dash": 330.0, "TP_Jutsu": 265.0}
ang_pulo = math.radians(TP["TP_Pular"])
for k, (dr, da, s) in enumerate(((2.5, -0.18, 0.32), (4.0, 0.05, 0.45), (5.5, 0.22, 0.6))):
    r = 12.0 + dr
    x, y = r * math.cos(ang_pulo + da), r * math.sin(ang_pulo + da)
    malha("SM_Pedra_Grande_C", cm(x, y, -0.2), unreal.Rotator(pitch=0, yaw=random.uniform(0, 360), roll=0), unreal.Vector(s, s, s),
          "Pulo_Pedra_%d" % (k + 1), "Tutorial/Obstaculos")
# ------------------------------------------------------------------ PlayerStart e marcadores --------------------------------
ps = ator(unreal.PlayerStart, cm(0, 0, 1.0), unreal.Rotator(pitch=0, yaw=TP["TP_Dash"], roll=0), "PlayerStart", "Tutorial")
for nome, ang in TP.items():
    r = 10.5
    x, y = r * math.cos(math.radians(ang)), r * math.sin(math.radians(ang))
    tp = ator(unreal.TargetPoint, cm(x, y, 0.1), unreal.Rotator(pitch=0, yaw=ang + 180, roll=0), nome, "Tutorial/Estacoes")
    txt = ator(unreal.TextRenderActor, cm(x, y, 2.6), unreal.Rotator(pitch=0, yaw=ang + 180, roll=0), nome + "_Texto", "Tutorial/Estacoes")
    tc = txt.text_render
    tc.set_text(nome)
    tc.set_editor_property("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER)
    tc.set_world_size(60.0)
    tc.set_text_render_color(unreal.Color(255, 230, 120, 255))
    txt.attach_to_actor(tp, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)

# ------------------------------------------------------------------ luz, céu, névoa, pós-processo ---------------------------
sol = ator(unreal.DirectionalLight, unreal.Vector(0, 0, 3000), unreal.Rotator(pitch=-40.0, yaw=-35.0, roll=0), "Sol", "Luz")
lc = sol.light_component
lc.set_mobility(unreal.ComponentMobility.MOVABLE)
lc.set_intensity(9.0)
lc.set_light_color(unreal.LinearColor(1.0, 0.9, 0.76, 1.0))
lc.set_editor_property("atmosphere_sun_light", True)
lc.set_editor_property("enable_light_shaft_occlusion", True)
lc.set_editor_property("enable_light_shaft_bloom", True)
lc.set_editor_property("bloom_scale", 0.15)
lc.set_editor_property("bloom_threshold", 0.6)
lc.set_editor_property("dynamic_shadow_distance_movable_light", 9000.0)
lc.set_editor_property("dynamic_shadow_cascades", 3)
ator(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), None, "Ceu", "Luz")
sky = ator(unreal.SkyLight, unreal.Vector(0, 0, 500), None, "Luz_Ceu", "Luz")
skc = sky.light_component
skc.set_mobility(unreal.ComponentMobility.MOVABLE)
skc.set_editor_property("real_time_capture", True)
skc.set_intensity(1.2)
fog = ator(unreal.ExponentialHeightFog, unreal.Vector(0, 0, -200), None, "Nevoa", "Luz")
fc = fog.component
fc.set_editor_property("fog_density", 0.012)
fc.set_editor_property("fog_height_falloff", 0.15)
fc.set_editor_property("start_distance", 2500.0)
fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.45, 0.62, 0.9, 1.0))
fc.set_editor_property("directional_inscattering_luminance", unreal.LinearColor(0.55, 0.45, 0.3, 1.0))
pp = ator(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), None, "PosProcesso", "Luz")
pp.set_editor_property("unbound", True)
st = pp.settings
for prop, val in (("override_bloom_intensity", True), ("bloom_intensity", 0.6), ("override_bloom_threshold", True), ("bloom_threshold", 1.0),
                  ("override_color_saturation", True), ("color_saturation", unreal.Vector4(1.15, 1.15, 1.15, 1.0)),
                  ("override_color_contrast", True), ("color_contrast", unreal.Vector4(1.05, 1.05, 1.05, 1.0)),
                  ("override_vignette_intensity", True), ("vignette_intensity", 0.25),
                  ("override_auto_exposure_bias", True), ("auto_exposure_bias", 0.3)):
    st.set_editor_property(prop, val)
pp.set_editor_property("settings", st)

print("salvo:", les.save_current_level())
