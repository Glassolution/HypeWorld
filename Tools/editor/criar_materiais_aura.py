"""Materiais da aura de chakra estilo anime (referências do Felipe).
- M_Chakra_ToonFlame: chama desenhada — preenchimento azul translúcido + contorno branco ondulado, buracos e labaredas (carregando)
- M_Chakra_Burst: coroa de espinhos, borda branca, rastros girando por dentro (chakra no máximo)
- M_Chakra_GroundGlow: brilho azul no chão
- M_Chakra_GroundBolts: raios piscando no chão (máximo)
Parâmetros comuns: Color, Intensity, Noise (textura gerada no jogo), SwapUV/FlipV (orientação do plano).
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/criar_materiais_aura.py
"""
import math
import unreal

FOLDER = "/Game/Ninja/VFX/Materials"
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
DEFAULT_TEX = unreal.load_asset("/Engine/EngineResources/DefaultTexture")
E = unreal


class G:
    def __init__(self, name, blend, two_sided=True):
        path = f"{FOLDER}/{name}"
        mat = unreal.load_asset(path)
        if mat:
            MEL.delete_all_material_expressions(mat)
        else:
            mat = tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
        mat.set_editor_property("blend_mode", blend)
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        mat.set_editor_property("two_sided", two_sided)
        self.mat, self.path, self.x = mat, path, -3000

    def node(self, cls, **props):
        n = MEL.create_material_expression(self.mat, cls, self.x, 0)
        self.x += 120
        for k, v in props.items():
            n.set_editor_property(k, v)
        return n

    def link(self, a, b, pin=""):
        MEL.connect_material_expressions(a, "", b, pin)

    def bin(self, cls, a, b):
        n = self.node(cls)
        self.link(a, n, "A"); self.link(b, n, "B")
        return n

    def c(self, v): return self.node(E.MaterialExpressionConstant, r=v)
    def mul(self, a, b): return self.bin(E.MaterialExpressionMultiply, a, b)
    def add(self, a, b): return self.bin(E.MaterialExpressionAdd, a, b)
    def sub(self, a, b): return self.bin(E.MaterialExpressionSubtract, a, b)
    def div(self, a, b): return self.bin(E.MaterialExpressionDivide, a, b)
    def mx(self, a, b): return self.bin(E.MaterialExpressionMax, a, b)
    def un(self, cls, a):
        n = self.node(cls); self.link(a, n); return n
    def absn(self, a): return self.un(E.MaterialExpressionAbs, a)
    def frac(self, a): return self.un(E.MaterialExpressionFrac, a)
    def one_minus(self, a): return self.un(E.MaterialExpressionOneMinus, a)
    def sat(self, a): return self.un(E.MaterialExpressionSaturate, a)
    def sine(self, a): return self.un(E.MaterialExpressionSine, a)
    def power(self, a, exp):
        n = self.node(E.MaterialExpressionPower, const_exponent=exp); self.link(a, n, "Base"); return n
    def lerp(self, a, b, t):
        n = self.node(E.MaterialExpressionLinearInterpolate)
        self.link(a, n, "A"); self.link(b, n, "B"); self.link(t, n, "Alpha"); return n
    def smooth(self, lo, hi, v):
        n = self.node(E.MaterialExpressionSmoothStep)
        self.link(self.c(lo) if isinstance(lo, (int, float)) else lo, n, "Min")
        self.link(self.c(hi) if isinstance(hi, (int, float)) else hi, n, "Max")
        self.link(v, n, "Value"); return n
    def mask(self, a, r=False, g=False, b=False):
        n = self.node(E.MaterialExpressionComponentMask, r=r, g=g, b=b, a=False); self.link(a, n); return n
    def append(self, a, b): return self.bin(E.MaterialExpressionAppendVector, a, b)
    def time(self, speed=1.0): return self.mul(self.node(E.MaterialExpressionTime), self.c(speed))
    def scalar(self, name, v): return self.node(E.MaterialExpressionScalarParameter, parameter_name=name, default_value=v)
    def color(self): return self.node(E.MaterialExpressionVectorParameter, parameter_name="Color", default_value=unreal.LinearColor(0.15, 0.55, 1.0, 1.0))
    def rgb(self, r, g, b): return self.node(E.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(r, g, b, 1.0))

    def noise(self, uv, scale_x, scale_y, speed_x, speed_y):
        """Ruído (0..1, linear) amostrado em uv*escala, deslizando no tempo."""
        coord = self.append(self.mul(self.mask(uv, r=True), self.c(scale_x)), self.mul(self.mask(uv, g=True), self.c(scale_y)))
        pan = self.node(E.MaterialExpressionPanner, speed_x=speed_x, speed_y=speed_y)
        self.link(coord, pan, "Coordinate")
        tex = self.node(E.MaterialExpressionTextureSampleParameter2D, parameter_name="Noise", texture=DEFAULT_TEX,
                        sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        self.link(pan, tex, "UVs")
        return self.power(self.mask(tex, r=True), 0.4545)   # a textura é sRGB: volta para linear

    def smooth_noise(self, x, h, sx, sy, speed_up, evolve, levels=2, rise=None, agit=None, seed=None):
        """Ruído procedural suave (0..1) do próprio material: subindo (speed_up*rise) e mudando de forma (evolve*agit)."""
        up = self.time(speed_up) if rise is None else self.mul(self.time(speed_up), rise)
        ev = self.time(evolve) if agit is None else self.mul(self.time(evolve), agit)
        px = self.mul(x, self.c(sx)) if seed is None else self.add(self.mul(x, self.c(sx)), seed)
        pos = self.append(self.append(px, self.sub(self.mul(h, self.c(sy)), up)), ev)
        n = self.node(E.MaterialExpressionNoise, scale=1.0, levels=levels, output_min=0.0, output_max=1.0, turbulence=False,
                      noise_function=unreal.NoiseFunction.NOISEFUNCTION_GRADIENT_ALU, quality=1)
        self.link(pos, n, "World Position")   # nome real da entrada (com "Position" a ligação falha calada)
        return n

    def plane_uv(self):
        """(x horizontal 0..1, h altura 0 embaixo..1 em cima) do plano virado para a câmera."""
        tc = self.node(E.MaterialExpressionTextureCoordinate)
        u, v = self.mask(tc, r=True), self.mask(tc, g=True)
        swap, flip = self.scalar("SwapUV", 1.0), self.scalar("FlipV", 1.0)
        x = self.lerp(u, v, swap)
        vv = self.lerp(v, u, swap)
        h = self.lerp(self.one_minus(vv), vv, flip)
        return x, h, self.append(x, h)

    def out(self, emissive, opacity=None):
        MEL.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        if opacity is not None:
            MEL.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    def finish(self):
        MEL.delete_unused_expressions(self.mat)
        MEL.recompile_material(self.mat)
        print("OK", self.path, "nós:", MEL.get_num_material_expressions(self.mat))
        return self.mat.get_outermost()


packages = []

# --- 1) Chama desenhada (aura) -------------------------------------------------------------------------------------
# Especificação do Felipe (25/10, referência Referencias/Aura/ref_estilo_anime.webp): fogo de energia cel-shaded.
# - Silhueta = campo de ruído com limiar (positivo dentro): base larga (~2,5x o corpo), 3 a 5 línguas de alturas
#   diferentes (ruído em "cristas" ao longo da largura), lados ondulando (eixo balança + ondinhas finas), nunca simétrico.
# - Buracos (ilhas sem azul, cada uma com contorno) e gotas soltas que sobem (mesmo estilo).
# - Contorno NÍTIDO branco-ciano #E8FCFF bem brilhante; faixa interna clara #7FE6FF; miolo azul translúcido (0,4) com
#   gradiente #4A4DFF embaixo -> #1E8BFF -> #3FC8FF em cima. Distâncias medidas na tela (ddx/ddy) = borda sem blur.
# - Grow (0..1): a aura cresce de baixo para cima ao ligar. Dissolve (0..1): ao desligar, ela se parte em pedaços e
#   solta gotas. ChargeLevel (0..1): mais carga = línguas mais altas. FrontLayer = 1: camada na FRENTE do personagem
#   (só contornos e pouco azul, some acima da cintura) para ele parecer "dentro" da aura.
# Ruído procedural do material (nó Noise, ALU), não textura: a entrada se chama "World Position".
g = G("M_Chakra_ToonFlame", unreal.BlendMode.BLEND_TRANSLUCENT)
inten = g.scalar("Intensity", 1.0)
aspect, speed, seed = g.scalar("Aspect", 1.05), g.scalar("Speed", 1.0), g.scalar("Seed", 0.0)
base_w, edge_w, band_w = g.scalar("BaseWidth", 0.30), g.scalar("EdgeWidth", 0.0055), g.scalar("BandWidth", 0.02)
glow_w, fill_op, edge_glow = g.scalar("GlowWidth", 0.008), g.scalar("FillOpacity", 0.5), g.scalar("EdgeGlow", 12.0)
holes_amt, drops_amt = g.scalar("Holes", 0.8), g.scalar("Drops", 1.0)
grow, dissolve = g.scalar("Grow", 1.0), g.scalar("Dissolve", 0.0)
charge, front = g.scalar("ChargeLevel", 1.0), g.scalar("FrontLayer", 0.0)
x, h, xh = g.plane_uv()
X = g.mul(g.sub(x, g.c(0.5)), aspect)            # horizontal na mesma escala da altura (o ruído não estica)
t = g.mul(g.node(E.MaterialExpressionTime), speed)


def n3(px_scale, py_scale, rise, evolve, off, px=None, py_src=None):
    """Ruído suave 0..1 em (X*sx, h*sy - t*rise, t*evolve): forma subindo e mudando."""
    px = g.add(g.mul(X if px is None else px, g.c(px_scale)), g.add(seed, g.c(off)))
    py = g.sub(g.mul(h if py_src is None else py_src, g.c(py_scale)), g.mul(t, g.c(rise)))
    # a forma quase não muda no lugar (evolução lenta): o movimento vem de SUBIR (rise) e da ondulação lenta (sway).
    # Evolução rápida fazia as bordas "pularem" (Felipe, 24/10: chakra flui, não pulsa)
    pz = g.add(g.mul(t, g.c(evolve * 0.25)), g.c(off * 0.37))
    n = g.node(E.MaterialExpressionNoise, scale=1.0, levels=2, output_min=0.0, output_max=1.0, turbulence=False,
               noise_function=unreal.NoiseFunction.NOISEFUNCTION_GRADIENT_ALU, quality=1)
    g.link(g.append(g.append(px, py), pz), n, "World Position")   # nome real da entrada (com "Position" falha calado)
    return n


def mn(a, b): return g.bin(E.MaterialExpressionMin, a, b)
def mxn(a, b): return g.bin(E.MaterialExpressionMax, a, b)
def ridge(n): return g.one_minus(g.absn(g.sub(g.mul(n, g.c(2.0)), g.c(1.0))))   # cristas: pontas finas


# lados ondulam: o eixo balança devagar com a altura (distorção lenta ~0,08) + ondinhas rápidas na borda
sway = g.mul(g.sub(n3(0.7, 1.4, 0.6, 0.25, 31.0), g.c(0.5)), g.mul(g.smooth(0.0, 0.8, h), g.c(0.16)))
Xs = g.add(X, sway)
ax = g.absn(Xs)
wig = g.mul(g.sub(n3(5.5, 5.5, 1.6, 0.8, 17.0, px=Xs), g.c(0.5)), g.c(0.014))
# corpo: coluna larga (cerca de 2,5x o personagem) que afina pouco até as línguas
halfw = g.add(g.c(0.04), g.mul(base_w, g.power(g.one_minus(h), 0.35)))
n1 = n3(2.4, 2.6, 0.9, 0.3, 0.0, px=Xs)
n2 = n3(4.0, 5.0, 1.5, 0.5, 7.0, px=Xs)
waves = g.add(g.mul(g.sub(ridge(n1), g.c(0.55)), g.c(0.8)), g.mul(g.sub(n2, g.c(0.5)), g.c(0.5)))
amp = g.add(g.c(0.04), g.mul(h, g.c(0.16)))
f_main = g.add(g.add(g.sub(halfw, ax), g.mul(waves, amp)), wig)
# línguas: a altura de cada "coluna" vem de um ruído ao longo da largura (cristas = pontas), 3 a 5 picos irregulares
tn = n3(3.4, 0.0, 0.0, 0.22, 43.0, px=Xs)
tn2 = n3(7.0, 0.0, 0.0, 0.45, 51.0, px=Xs)
tongue = g.add(g.mul(g.lerp(ridge(tn), g.smooth(0.25, 0.75, tn), g.c(0.35)), g.c(0.66)), g.mul(g.sub(tn2, g.c(0.5)), g.c(0.12)))   # meio crista, meio arredondado
top_h = g.add(g.add(g.c(0.30), tongue), g.mul(g.sub(charge, g.c(1.0)), g.c(0.18)))
top_h = g.sub(top_h, g.mul(g.absn(X), g.c(0.35)))                     # pontas mais altas no meio
top = g.mul(g.add(g.sub(top_h, h), g.add(g.mul(waves, g.c(0.06)), wig)), g.c(1.2))
top = mn(top, g.mul(g.sub(g.c(0.97), h), g.c(1.5)))
side = g.sub(g.sub(g.mul(aspect, g.c(0.5)), g.c(0.02)), ax)
f_main = mn(mn(f_main, side), top)
# buracos e fendas (cada um com contorno): mais no alto; ao dissolver, viram muitos (a aura se parte em pedaços)
hn = n3(5.5, 4.5, 1.2, 0.4, 3.0, px=Xs)
thr = g.add(g.sub(g.c(0.74), g.mul(h, g.c(0.10))), g.mul(g.one_minus(g.smooth(0.05, 0.30, h)), g.c(0.2)))
thr = g.sub(g.add(thr, g.mul(g.one_minus(holes_amt), g.c(0.5))), g.mul(dissolve, g.c(0.35)))
f_holes = mn(f_main, g.mul(g.sub(thr, hn), g.c(1.0)))
f_holes = g.sub(f_holes, g.mul(dissolve, g.c(0.12)))
# gotas: chamas pequenas alongadas que se soltam perto da aura e sobem rápido (mais ao dissolver)
dn = n3(7.0, 3.2, 2.6, 0.6, 11.0)
near = g.mul(mn(f_main, g.c(0.0)), g.c(1.0))
low = g.mul(g.one_minus(g.smooth(0.25, 0.45, h)), g.c(0.4))
dthr = g.sub(g.add(g.c(0.70), g.mul(g.one_minus(drops_amt), g.c(0.5))), g.mul(dissolve, g.c(0.12)))
f_drop = g.sub(g.add(g.sub(dn, dthr), near), low)
f_drop = mn(mn(f_drop, side), g.mul(g.sub(g.c(0.97), h), g.c(1.5)))
f = mxn(f_holes, f_drop)
# Grow: cresce de baixo para cima ao ligar
f = mn(f, g.mul(g.sub(g.mul(grow, g.c(1.15)), h), g.c(3.0)))

# distância até a borda, em "alturas do plano": |f| / |gradiente na tela| * (altura por pixel)
def grad_len(v):
    dx, dy = g.un(E.MaterialExpressionDDX, v), g.un(E.MaterialExpressionDDY, v)
    return g.un(E.MaterialExpressionSquareRoot, g.add(g.add(g.mul(dx, dx), g.mul(dy, dy)), g.c(1e-10)))
px_h = grad_len(h)                                  # altura do plano por pixel
sdist = g.mul(g.div(f, grad_len(f)), px_h)          # distância com sinal (+ dentro)
aa = g.mul(px_h, g.c(0.75))                         # anti-serrilhado mínimo (borda nítida)
inside = g.smooth(g.mul(aa, g.c(-1.0)), aa, sdist)
dist = g.absn(sdist)
line = g.one_minus(g.smooth(edge_w, g.add(edge_w, aa), dist))                  # contorno (dos dois lados da borda)
band = g.mul(inside, g.one_minus(g.smooth(band_w, g.add(band_w, aa), sdist)))  # faixa clara logo dentro
halo = g.mul(g.one_minus(inside), g.power(g.sat(g.one_minus(g.div(g.sub(dist, edge_w), glow_w))), 2.0))
fade_bottom = g.smooth(0.0, 0.06, h)

def vparam(name, r, gg, b): return g.node(E.MaterialExpressionVectorParameter, parameter_name=name, default_value=unreal.LinearColor(r, gg, b, 1.0))
# cores do miolo (parâmetros: a chama dos pés usa um azul mais forte com o mesmo material)
c_base, c_mid, c_top = vparam("ColorBase", 0.068, 0.074, 1.0), vparam("ColorMid", 0.013, 0.258, 1.0), vparam("ColorTop", 0.05, 0.58, 1.0)
fill_col = g.lerp(g.lerp(c_base, c_mid, g.smooth(0.0, 0.3, h)), c_top, g.smooth(0.55, 1.0, h))
band_col = g.mul(g.rgb(0.212, 0.79, 1.0), g.c(1.1))
line_col = g.mul(g.rgb(0.807, 0.973, 1.0), edge_glow)
halo_col = g.mul(g.rgb(0.25, 0.65, 1.0), g.c(3.0))
fill_glow = g.scalar("FillGlow", 1.0)   # brilho do miolo (pés: mais forte)
emis = g.lerp(g.lerp(g.lerp(halo_col, g.mul(fill_col, fill_glow), inside), band_col, band), line_col, line)
fill_a = g.mul(inside, g.lerp(fill_op, g.c(0.55), band))
fill_a = g.mul(fill_a, g.lerp(g.c(1.0), g.c(0.25), front))      # camada da frente: pouco azul (o corpo aparece)
opac = mxn(mxn(line, fill_a), g.mul(halo, g.c(0.15)))
opac = g.mul(opac, g.lerp(g.c(1.0), g.one_minus(g.smooth(0.12, 0.28, h)), front))   # frente: só embaixo, em volta das pernas
# sem teste de profundidade (o chão não corta a base, que fica atrás dos pés); quem esconde a chama é o corpo
# (Custom Depth do personagem; a camada da frente ignora) e paredes bem à frente (mais de 80 cm)
g.mat.set_editor_property("disable_depth_test", True)
pd = g.node(E.MaterialExpressionPixelDepth)
cdep = g.mask(g.node(E.MaterialExpressionSceneTexture, scene_texture_id=unreal.SceneTextureId.PPI_CUSTOM_DEPTH), r=True)
sdep = g.mask(g.node(E.MaterialExpressionSceneDepth), r=True)
body = mxn(g.sat(g.mul(g.sub(g.add(cdep, g.c(5.0)), pd), g.c(0.2))), front)
vis = g.mul(body, g.sat(g.mul(g.sub(g.add(sdep, g.c(80.0)), pd), g.c(0.1))))
g.out(g.mul(emis, inten), g.mul(g.mul(g.mul(opac, g.sat(inten)), fade_bottom), vis))
packages.append(g.finish())

# --- 2) Coroa de espinhos (máximo) ----------------------------------------------------------------------------------
g = G("M_Chakra_Burst", unreal.BlendMode.BLEND_TRANSLUCENT)
col, inten = g.color(), g.scalar("Intensity", 1.0)
x, h, xh = g.plane_uv()
px, py = g.sub(x, g.c(0.5)), g.sub(h, g.c(0.45))
d = g.node(E.MaterialExpressionDistance); g.link(xh, d, "A"); g.link(g.node(E.MaterialExpressionConstant2Vector, r=0.5, g=0.45), d, "B")
at = g.node(E.MaterialExpressionArctangent2); g.link(py, at, "Y"); g.link(px, at, "X")
a01 = g.add(g.div(at, g.c(2.0 * math.pi)), g.c(0.5))                     # ângulo 0..1
na = g.noise(g.append(a01, g.time(0.0)), 3.0, 1.0, 0.0, 0.15)            # comprimento de cada espinho varia
tri = g.mul(g.absn(g.sub(g.frac(g.add(g.mul(a01, g.c(24.0)), g.mul(na, g.c(0.35)))), g.c(0.5))), g.c(2.0))
spike = g.power(g.one_minus(tri), 3.0)                                  # pontas finas
top = g.smooth(-0.15, 0.25, py)                                          # espinhos em cima e nos lados
radius = g.add(g.c(0.27), g.mul(g.mul(spike, top), g.add(g.c(0.07), g.mul(na, g.c(0.16)))))
field = g.sub(radius, d)
fill = g.mul(g.smooth(0.0, 0.006, field), g.smooth(0.0, 0.1, h))
edge = g.mul(fill, g.one_minus(g.smooth(0.006, 0.03, field)))
rim = g.power(g.sat(g.div(d, radius)), 2.5)                              # mais claro perto da borda
swirl = g.frac(g.sub(g.add(g.mul(a01, g.c(3.0)), g.mul(d, g.c(2.6))), g.time(0.9)))
streak = g.mul(g.mul(g.smooth(0.9, 0.97, swirl), g.one_minus(g.smooth(0.97, 1.0, swirl))), fill)
nb = g.noise(xh, 4.0, 4.0, 0.3, -0.6)
bolt = g.mul(g.one_minus(g.smooth(0.0, 0.035, g.absn(g.sub(nb, g.c(0.5))))), fill)
white = g.add(g.add(g.mul(edge, g.c(9.0)), g.mul(streak, g.c(4.0))), g.mul(bolt, g.c(3.0)))
emis = g.mul(g.add(g.mul(col, g.add(g.c(0.8), g.mul(rim, g.c(4.0)))), g.mul(g.rgb(0.8, 0.95, 1.0), white)), inten)
opac = g.mul(g.sat(g.mul(fill, g.add(g.add(g.c(0.15), g.mul(rim, g.c(0.75))), g.add(g.add(edge, streak), bolt)))), inten)
g.out(emis, opac)
packages.append(g.finish())

# --- 3) Brilho no chão: azul-arroxeado (#5A4BFF) em volta dos pés, aditivo ~0,5 (pulsa junto pelo Intensity) ------------------------------------------
g = G("M_Chakra_GroundGlow", unreal.BlendMode.BLEND_ADDITIVE)
col, inten = g.color(), g.scalar("Intensity", 1.0)
tc = g.node(E.MaterialExpressionTextureCoordinate)
d = g.node(E.MaterialExpressionDistance); g.link(tc, d, "A"); g.link(g.node(E.MaterialExpressionConstant2Vector, r=0.5, g=0.5), d, "B")
dd = g.mul(d, g.c(2.0))
glow = g.power(g.one_minus(g.smooth(0.0, 1.0, dd)), 2.2)
tint = g.lerp(g.rgb(0.102, 0.069, 1.0), g.rgb(0.05, 0.12, 1.0), g.smooth(0.0, 0.6, dd))     # #5A4BFF no meio, azul na borda
g.out(g.mul(g.mul(tint, g.mul(glow, g.c(0.5))), inten))
packages.append(g.finish())

# --- 4) Raios no chão (máximo) --------------------------------------------------------------------------------------
g = G("M_Chakra_GroundBolts", unreal.BlendMode.BLEND_ADDITIVE)
col, inten = g.color(), g.scalar("Intensity", 1.0)
tc = g.node(E.MaterialExpressionTextureCoordinate)
d = g.node(E.MaterialExpressionDistance); g.link(tc, d, "A"); g.link(g.node(E.MaterialExpressionConstant2Vector, r=0.5, g=0.5), d, "B")
dd = g.mul(d, g.c(2.0))
n1 = g.noise(tc, 2.5, 2.5, 0.02, 0.03)
n2 = g.noise(tc, 5.0, 5.0, -0.05, 0.04)
l1 = g.one_minus(g.smooth(0.0, 0.03, g.absn(g.sub(n1, g.c(0.5)))))
l2 = g.mul(g.one_minus(g.smooth(0.0, 0.02, g.absn(g.sub(n2, g.c(0.5))))), g.c(0.6))
flicker = g.add(g.c(0.55), g.mul(g.sine(g.time(31.0)), g.c(0.45)))
mask_r = g.mul(g.one_minus(g.smooth(0.35, 1.0, dd)), g.smooth(0.08, 0.2, dd))
lines = g.mul(g.mul(g.add(l1, l2), mask_r), flicker)
g.out(g.mul(g.mul(g.add(col, g.rgb(0.5, 0.6, 0.7)), g.mul(lines, g.c(2.5))), inten))
packages.append(g.finish())

print("salvo:", unreal.EditorLoadingAndSavingUtils.save_packages(packages, False))
