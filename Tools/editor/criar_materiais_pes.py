"""VFX da Infusão de Chakra nos pés (especificação do Felipe, 24/10/2026): mesmo estilo da aura (chama cel-shaded).
- MI_ChakraFeet: instância do M_Chakra_ToonFlame (chama que envolve o pé), azul mais forte, línguas menores e agitadas
- M_Chakra_FeetSole: camada de energia azul-branca colada na sola (aditivo, oval)
- M_Chakra_FeetGround: brilho circular #3A6BFF na superfície em que o pé apoia (aditivo)
- M_Chakra_FeetRing: anel de choque a cada passo (contorno branco + faixa azul), parâmetro Fade
- M_Chakra_FeetSpark: gota alongada (contorno branco + miolo azul), parâmetro Fade
- M_Chakra_FeetTrail: rastro que afina até sumir (contorno claro + azul), parâmetro Fade
Rodar DEPOIS do criar_materiais_aura (usa a mesma classe G):  python3 -I Tools/ue_exec.py -f Tools/editor/criar_materiais_pes.py
"""
import unreal
FONTE = "/Users/luisfelipexavier/Desktop/Hype World/Tools/editor/criar_materiais_aura.py"
_src = open(FONTE).read()
exec(_src[:_src.index("packages = []")])   # classe G e constantes (sem refazer os materiais da aura)
packages = []


def srgb(r, g_, b):
    c = unreal.LinearColor(r / 255.0, g_ / 255.0, b / 255.0, 1.0)
    return [pow(v, 2.2) for v in (c.r, c.g, c.b)]


def sdf_parts(g, f, h_ref, edge_w, aa_k=0.75):
    """Borda nítida a partir de um campo f (+ dentro): (dentro, contorno, distância com sinal)."""
    def grad_len(v):
        dx, dy = g.un(E.MaterialExpressionDDX, v), g.un(E.MaterialExpressionDDY, v)
        return g.un(E.MaterialExpressionSquareRoot, g.add(g.add(g.mul(dx, dx), g.mul(dy, dy)), g.c(1e-10)))
    edge_w = g.c(edge_w) if isinstance(edge_w, float) else edge_w
    px = grad_len(h_ref)
    sd = g.mul(g.div(f, grad_len(f)), px)
    aa = g.mul(px, g.c(aa_k))
    inside = g.smooth(g.mul(aa, g.c(-1.0)), aa, sd)
    line = g.one_minus(g.smooth(edge_w, g.add(edge_w, aa), g.absn(sd)))
    return inside, line, sd


def radial(g):
    tc = g.node(E.MaterialExpressionTextureCoordinate)
    d = g.node(E.MaterialExpressionDistance); g.link(tc, d, "A"); g.link(g.node(E.MaterialExpressionConstant2Vector, r=0.5, g=0.5), d, "B")
    return tc, g.mul(d, g.c(2.0))   # 0 no centro, 1 na borda


WHITE = g_white = None
# --- MI_ChakraFeet: chama em volta do pé ------------------------------------------------------------------------------
base_mat = unreal.load_asset(f"{FOLDER}/M_Chakra_ToonFlame")
mi = unreal.load_asset(f"{FOLDER}/MI_ChakraFeet") or tools.create_asset("MI_ChakraFeet", FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
MEL.set_material_instance_parent(mi, base_mat)
for k, v in {"Aspect": 1.3, "Speed": 1.9, "BaseWidth": 0.42, "EdgeWidth": 0.03, "BandWidth": 0.085, "GlowWidth": 0.045,
             "FillOpacity": 0.6, "FillGlow": 3.5, "EdgeGlow": 20.0, "Holes": 0.6, "Drops": 0.7}.items():
    MEL.set_material_instance_scalar_parameter_value(mi, k, v)
for k, c in {"ColorBase": srgb(10, 108, 255), "ColorMid": srgb(20, 145, 255), "ColorTop": srgb(47, 184, 255)}.items():
    MEL.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(c[0], c[1], c[2], 1.0))
MEL.update_material_instance(mi)
packages.append(mi.get_outermost())

# --- Sola: oval azul-branca, aditivo, com tremulação -------------------------------------------------------------------
g = G("M_Chakra_FeetSole", unreal.BlendMode.BLEND_ADDITIVE)
inten = g.scalar("Intensity", 1.0)
tc, dd = radial(g)
flick = g.add(g.c(0.85), g.mul(g.sine(g.time(23.0)), g.c(0.15)))
core = g.power(g.one_minus(g.smooth(0.0, 1.0, dd)), 1.6)
c1, c2 = srgb(159, 232, 255), srgb(10, 108, 255)
col = g.lerp(g.mul(g.rgb(*c1), g.c(6.0)), g.mul(g.rgb(*c2), g.c(3.0)), g.smooth(0.0, 0.8, dd))
g.out(g.mul(g.mul(col, core), g.mul(flick, inten)))
packages.append(g.finish())

# --- Chão: brilho circular #3A6BFF -------------------------------------------------------------------------------------
g = G("M_Chakra_FeetGround", unreal.BlendMode.BLEND_ADDITIVE)
inten = g.scalar("Intensity", 1.0)
tc, dd = radial(g)
glow = g.power(g.one_minus(g.smooth(0.0, 1.0, dd)), 2.0)
g.out(g.mul(g.mul(g.rgb(*srgb(58, 107, 255)), g.mul(glow, g.c(0.6 * 2.0))), inten))
packages.append(g.finish())

# --- Anel de choque do passo: faixa azul entre duas linhas brancas nítidas ---------------------------------------------
g = G("M_Chakra_FeetRing", unreal.BlendMode.BLEND_TRANSLUCENT)
fade, inten = g.scalar("Fade", 1.0), g.scalar("Intensity", 1.0)
tc, dd = radial(g)
f_out = g.sub(g.c(0.95), dd)
f_in = g.sub(dd, g.c(0.72))
f = g.bin(E.MaterialExpressionMin, f_out, f_in)
inside, line, sd = sdf_parts(g, f, g.mask(tc, r=True), 0.022)
emis = g.lerp(g.mul(g.rgb(*srgb(30, 120, 255)), g.c(3.0)), g.mul(g.rgb(0.807, 0.973, 1.0), g.c(18.0)), line)
opac = g.bin(E.MaterialExpressionMax, line, g.mul(inside, g.c(0.45)))
g.out(g.mul(emis, inten), g.mul(opac, fade))
packages.append(g.finish())

# --- Faísca/gota: alongada para cima, ponta fina; h = altura (0 embaixo) --------------------------------------------------
g = G("M_Chakra_FeetSpark", unreal.BlendMode.BLEND_TRANSLUCENT)
fade, inten = g.scalar("Fade", 1.0), g.scalar("Intensity", 1.0)
x, h, xh = g.plane_uv()
X = g.absn(g.sub(x, g.c(0.5)))
width = g.mul(g.power(g.sat(g.sine(g.mul(g.add(g.mul(h, g.c(0.9)), g.c(0.05)), g.c(3.14159)))), 0.8), g.mul(g.one_minus(h), g.c(0.42)))
f = g.sub(width, X)
inside, line, sd = sdf_parts(g, f, h, 0.06)
fill = g.lerp(g.rgb(*srgb(10, 108, 255)), g.rgb(*srgb(47, 184, 255)), h)
emis = g.lerp(g.mul(fill, g.c(2.5)), g.mul(g.rgb(0.807, 0.973, 1.0), g.c(18.0)), line)
opac = g.bin(E.MaterialExpressionMax, line, g.mul(inside, g.c(0.6)))
g.out(g.mul(emis, inten), g.mul(opac, fade))
packages.append(g.finish())

# --- Rastro: h = 0 no pé, 1 na ponta; afina e some -----------------------------------------------------------------------
g = G("M_Chakra_FeetTrail", unreal.BlendMode.BLEND_TRANSLUCENT)
fade, inten = g.scalar("Fade", 1.0), g.scalar("Intensity", 1.0)
x, h, xh = g.plane_uv()
X = g.absn(g.sub(x, g.c(0.5)))
width = g.mul(g.c(0.46), g.power(g.one_minus(h), 0.7))
f = g.sub(width, X)
inside, line, sd = sdf_parts(g, f, x, 0.07)
emis = g.lerp(g.mul(g.rgb(*srgb(20, 130, 255)), g.c(3.0)), g.mul(g.rgb(0.807, 0.973, 1.0), g.c(12.0)), line)
opac = g.mul(g.bin(E.MaterialExpressionMax, line, g.mul(inside, g.c(0.5))), g.one_minus(g.smooth(0.3, 1.0, h)))
g.out(g.mul(emis, inten), g.mul(opac, fade))
packages.append(g.finish())

# =====================================================================================================================
# 2ª versão (Felipe, 24/10/2026): NÃO é chama. Casca de energia azul colada no pé, seguindo o formato dele (cópia da malha
# do corpo inflada 3 cm só nos pés), borda brilhante (Fresnel), correntes subindo sem pulsar. As chamas/faíscas/anéis acima
# ficaram sem uso.
# =====================================================================================================================
g = G("M_ChakraFeetShell", unreal.BlendMode.BLEND_TRANSLUCENT, two_sided=False)
g.mat.set_editor_property("used_with_skeletal_mesh", True)
inten, reveal = g.scalar("Intensity", 1.0), g.scalar("Reveal", 1.0)
radius_p, ankle, inflate = g.scalar("FootRadius", 20.0), g.scalar("AnkleHeight", 16.0), g.scalar("Inflate", 3.0)
def vparam(name, v): return g.node(E.MaterialExpressionVectorParameter, parameter_name=name, default_value=unreal.LinearColor(v[0], v[1], v[2], 1.0))
def rgb3(a): return g.mask(a, r=True, g=True, b=True)
def dot(a, b):
    n = g.node(E.MaterialExpressionDotProduct); g.link(a, n, "A"); g.link(b, n, "B"); return n
def length(v): return g.un(E.MaterialExpressionSquareRoot, g.add(dot(v, v), g.c(1e-4)))
def noise3(pos):
    n = g.node(E.MaterialExpressionNoise, scale=1.0, levels=2, output_min=0.0, output_max=1.0, turbulence=False,
               noise_function=unreal.NoiseFunction.NOISEFUNCTION_GRADIENT_ALU, quality=1)
    g.link(pos, n, "World Position"); return n
up = rgb3(vparam("Up", (0.0, 0.0, 1.0)))
wp = g.node(E.MaterialExpressionWorldPosition)
feet = []
for name in ("FootL", "FootR"):
    rel = g.sub(wp, rgb3(vparam(name, (0.0, 0.0, -10000.0))))     # do centro da sola do pé até o ponto
    h = dot(rel, up)                                                # altura acima da sola
    horiz = length(g.sub(rel, g.mul(up, h)))
    near = g.mul(g.one_minus(g.smooth(g.mul(radius_p, g.c(0.7)), radius_p, horiz)), g.smooth(-10.0, -4.0, h))
    grad = g.one_minus(g.smooth(g.c(0.0), ankle, h))                # forte na sola, some no tornozelo
    top = g.mul(ankle, reveal)                                      # liga/desliga: sobe da sola / recolhe para a sola
    rev = g.one_minus(g.smooth(g.sub(top, g.c(3.0)), g.add(top, g.c(0.01)), h))
    wmask = g.mul(near, g.one_minus(g.smooth(ankle, g.add(ankle, g.c(6.0)), h)))
    feet.append((rel, near, g.mul(g.mul(near, grad), rev), wmask))
(relL, nearL, visL, wL), (relR, nearR, visR, wR) = feet
M = g.mx(visL, visR)
rel = g.lerp(relR, relL, g.smooth(-0.01, 0.01, g.sub(nearL, nearR)))    # coordenadas do pé (não escorrega ao andar)
t = g.node(E.MaterialExpressionTime)
# fluxo: um ruído lento (que também sobe) empurra as correntes de lado, suave
flow = g.sub(noise3(g.sub(g.mul(rel, g.c(0.05)), g.mul(up, g.mul(t, g.c(0.3))))), g.c(0.5))
relf = g.add(rel, g.mul(g.append(g.append(flow, g.mul(flow, g.c(-0.7))), g.c(0.0)), g.c(7.0)))
n1 = noise3(g.sub(g.mul(relf, g.c(0.09)), g.mul(up, g.mul(t, g.c(0.55)))))
n2 = noise3(g.sub(g.mul(relf, g.c(0.17)), g.mul(up, g.mul(t, g.c(1.05)))))
cur = g.smooth(0.42, 0.72, g.add(g.mul(n1, g.c(0.55)), g.mul(n2, g.c(0.45))))
fres = g.node(E.MaterialExpressionFresnel, exponent=2.5, base_reflect_fraction=0.03)
c_base, c_mid, c_edge = g.rgb(*srgb(10, 108, 255)), g.rgb(*srgb(47, 184, 255)), g.rgb(*srgb(216, 246, 255))
col = g.lerp(g.lerp(c_base, c_mid, g.sat(g.mul(fres, g.c(1.6)))), c_edge, g.smooth(0.6, 1.0, fres))
emis = g.mul(g.mul(col, g.add(g.c(8.0), g.mul(fres, g.c(7.0)))), g.mul(g.add(g.c(0.6), g.mul(cur, g.c(0.8))), inten))
opac = g.mul(g.mul(g.sat(g.mul(g.add(g.c(0.10), g.mul(fres, g.c(0.9))), g.add(g.c(0.55), g.mul(cur, g.c(0.6))))), M), g.sat(inten))
g.out(emis, opac)
nrm = g.node(E.MaterialExpressionVertexNormalWS)
MEL.connect_material_property(g.mul(nrm, g.mul(inflate, g.mx(wL, wR))), "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
packages.append(g.finish())

# --- Fiapo suave (sobe devagar, alongado, sem borda) -------------------------------------------------------------------
g = G("M_Chakra_FeetWisp", unreal.BlendMode.BLEND_ADDITIVE)
fade, inten = g.scalar("Fade", 1.0), g.scalar("Intensity", 1.0)
x, h, xh = g.plane_uv()
dx, dy = g.mul(g.sub(x, g.c(0.5)), g.c(2.0)), g.mul(g.sub(h, g.c(0.5)), g.c(2.0))
d = g.un(E.MaterialExpressionSquareRoot, g.add(g.add(g.mul(dx, dx), g.mul(dy, dy)), g.c(1e-5)))
a = g.power(g.one_minus(g.smooth(0.0, 1.0, d)), 2.0)
col = g.lerp(g.mul(g.rgb(*srgb(47, 184, 255)), g.c(4.0)), g.mul(g.rgb(*srgb(216, 246, 255)), g.c(7.0)), g.power(g.sat(g.one_minus(d)), 3.0))
g.out(g.mul(g.mul(col, a), g.mul(fade, inten)))
packages.append(g.finish())

print("salvo:", unreal.EditorLoadingAndSavingUtils.save_packages(packages, False))
