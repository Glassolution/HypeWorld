"""Slots de jutsu do HUD (referência do Felipe, 24/10/2026): materiais de UI (domínio User Interface), sem PNG fixo.
- M_UI_JutsuGlow: brilho radial suave na cor do jutsu (Color, Intensity)
- M_UI_JutsuSlot: aro escuro #0E1116 + fundo #151A22 + ícone recortado em círculo + vinheta e reflexo de "gema" +
  contorno fino na cor do jutsu (Selected) + estados: Cooldown (0..1, varredura escura no sentido horário, ícone
  dessaturado), NoChakra (avermelhado), Flash (branco ao usar), Empty (slot vazio), Colorize (ícones brancos -> cor do jutsu)
Bordas por smoothstep com a largura de 1 pixel (DDX/DDY): sem serrilhado em qualquer resolução.
Rodar: python3 -I Tools/ue_exec.py -f Tools/editor/criar_materiais_jutsu.py
"""
import math
import unreal
FONTE = "/Users/luisfelipexavier/Desktop/Hype World/Tools/editor/criar_materiais_aura.py"
_src = open(FONTE).read()
exec(_src[:_src.index("packages = []")].replace('FOLDER = "/Game/Ninja/VFX/Materials"', 'FOLDER = "/Game/Ninja/UI/HUD/Jutsu"'))
packages = []


def ui(g):
    g.mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)


def srgb(r, g_, b):
    return [pow(v / 255.0, 2.2) for v in (r, g_, b)]


def radius(g):
    """r = 0 no centro, 1 na borda da imagem; aa = 1 pixel em unidades de r; p = uv - 0,5."""
    tc = g.node(E.MaterialExpressionTextureCoordinate)
    p = g.sub(tc, g.node(E.MaterialExpressionConstant2Vector, r=0.5, g=0.5))
    d = g.node(E.MaterialExpressionDistance); g.link(tc, d, "A"); g.link(g.node(E.MaterialExpressionConstant2Vector, r=0.5, g=0.5), d, "B")
    r = g.mul(d, g.c(2.0))
    dx, dy = g.un(E.MaterialExpressionDDX, r), g.un(E.MaterialExpressionDDY, r)
    aa = g.un(E.MaterialExpressionSquareRoot, g.add(g.add(g.mul(dx, dx), g.mul(dy, dy)), g.c(1e-8)))
    return tc, p, r, aa


def edge_in(g, r, edge, aa):
    """1 dentro de r < edge, 0 fora, transição de 1 pixel."""
    return g.one_minus(g.smooth(g.sub(edge, aa), g.add(edge, aa), r))


def vec(g, name, rgb):
    return g.node(E.MaterialExpressionVectorParameter, parameter_name=name, default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))


# --- Brilho externo ----------------------------------------------------------------------------------------------------
g = G("M_UI_JutsuGlow", unreal.BlendMode.BLEND_TRANSLUCENT)
ui(g)
col, inten = vec(g, "Color", srgb(255, 138, 43)), g.scalar("Intensity", 0.45)
tc, p, r, aa = radius(g)
glow = g.power(g.one_minus(g.smooth(0.5, 1.0, r)), 1.6)   # o slot cobre r < 0,74: o brilho visível é o anel de fora, sumindo suave
g.out(g.mul(col, g.c(2.6)), g.mul(g.mul(glow, inten), g.c(1.4)))
packages.append(g.finish())

# --- Slot (aro + ícone + estados) --------------------------------------------------------------------------------------
g = G("M_UI_JutsuSlot", unreal.BlendMode.BLEND_TRANSLUCENT)
ui(g)
theme = vec(g, "Color", srgb(255, 138, 43))
selected, cooldown, nochakra = g.scalar("Selected", 0.0), g.scalar("Cooldown", 0.0), g.scalar("NoChakra", 0.0)
flash, empty, colorize = g.scalar("Flash", 0.0), g.scalar("Empty", 0.0), g.scalar("Colorize", 1.0)
inner_r = g.scalar("InnerRadius", 0.87)       # aro = 3 px de 23 px de raio
icon_zoom = g.scalar("IconZoom", 0.9)         # quanto do ícone aparece (recorta a borda da arte)
tc, p, r, aa = radius(g)
disc = edge_in(g, r, g.c(1.0), aa)
inner = edge_in(g, r, inner_r, aa)

# ícone recortado em círculo
iuv = g.add(g.mul(g.div(p, inner_r), icon_zoom), g.node(E.MaterialExpressionConstant2Vector, r=0.5, g=0.5))
tex = g.node(E.MaterialExpressionTextureSampleParameter2D, parameter_name="Icon",
             texture=unreal.load_asset("/Game/Ninja/UI/Textures/T_HUD_Poder_Q"), sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
g.link(iuv, tex, "UVs")
icol = g.mask(tex, r=True, g=True, b=True)
ia = g.node(E.MaterialExpressionMultiply); MEL.connect_material_expressions(tex, "A", ia, "A"); g.link(g.c(1.0), ia, "B")   # saída A (alfa) do ícone
lum = g.node(E.MaterialExpressionDotProduct); g.link(icol, lum, "A"); g.link(g.rgb(0.3, 0.59, 0.11), lum, "B")
# ícones brancos sobre fundo escuro: o claro vira a cor do jutsu (com miolo quase branco), o escuro um tom fundo dela
bright = g.lerp(theme, g.rgb(1.0, 1.0, 1.0), g.c(0.22))
painted = g.lerp(g.mul(theme, g.c(0.34)), g.mul(bright, g.c(1.3)), g.smooth(0.08, 0.9, lum))
icol = g.lerp(icol, painted, colorize)
bg = g.rgb(*srgb(21, 26, 34))
icol = g.lerp(bg, icol, ia)
# vinheta escura na borda + reflexo suave no topo (aparência de botão/gema)
vig = g.smooth(0.45, 1.0, g.div(r, inner_r))
icol = g.mul(icol, g.one_minus(g.mul(vig, g.c(0.55))))
hl_p = g.sub(tc, g.node(E.MaterialExpressionConstant2Vector, r=0.5, g=0.27))
hl_d = g.node(E.MaterialExpressionDistance); g.link(g.mul(hl_p, g.node(E.MaterialExpressionConstant2Vector, r=1.0, g=1.8)), hl_d, "A"); g.link(g.node(E.MaterialExpressionConstant2Vector, r=0.0, g=0.0), hl_d, "B")
hl = g.mul(g.power(g.sat(g.one_minus(g.div(hl_d, g.c(0.32)))), 2.0), g.c(0.22))
icol = g.add(icol, g.mul(g.rgb(1.0, 1.0, 1.0), hl))
# estados: sem chakra (avermelhado e escuro), cooldown (dessaturado e escuro + varredura), flash branco
icol = g.lerp(icol, g.mul(g.mul(lum, g.c(0.55)), g.rgb(1.0, 0.18, 0.15)), g.mul(nochakra, g.c(0.85)))
on_cd = g.smooth(0.0, 0.001, cooldown)
icol = g.lerp(icol, g.mul(g.add(g.mul(lum, g.c(0.8)), g.c(0.02)), g.rgb(0.42, 0.44, 0.48)), on_cd)
ang = g.node(E.MaterialExpressionArctangent2)
g.link(g.mask(p, r=True), ang, "Y"); g.link(g.mul(g.mask(p, g=True), g.c(-1.0)), ang, "X")
a01 = g.frac(g.add(g.div(ang, g.c(2.0 * math.pi)), g.c(1.0)))      # 0 no topo, cresce no sentido horário
sweep = g.mul(g.smooth(g.sub(g.one_minus(cooldown), g.c(0.004)), g.add(g.one_minus(cooldown), g.c(0.004)), a01), on_cd)
icol = g.mul(icol, g.one_minus(g.mul(sweep, g.c(0.6))))
icol = g.lerp(icol, g.rgb(1.0, 1.0, 1.0), g.mul(flash, g.c(0.7)))
icol = g.lerp(icol, bg, empty)
# aro escuro + contorno fino na cor do jutsu (selecionado)
ring = g.rgb(*srgb(14, 17, 22))
outline = g.mul(g.smooth(g.sub(g.c(0.93), aa), g.add(g.c(0.93), aa), r), selected)
rim = g.lerp(ring, g.mul(theme, g.c(1.8)), outline)
color = g.lerp(rim, icol, inner)
alpha = g.mul(disc, g.lerp(g.c(0.92), g.c(1.0), inner))
g.out(color, alpha)
packages.append(g.finish())

print("salvo:", unreal.EditorLoadingAndSavingUtils.save_packages(packages, False))
