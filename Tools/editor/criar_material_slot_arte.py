"""Slots de jutsu com a arte do Felipe (25/10/2026): M_UI_JutsuSlotArt desenha o PNG recortado do slot
(Tools/hud/recortar_slots_jutsu.py: moldura de pincel + arte do jutsu + etiqueta sem número) e aplica os estados só
DENTRO do círculo da arte (a moldura fica igual):
  Selected (mais claro), Cooldown (0..1: dessaturado/escuro + varredura no sentido horário), NoChakra (avermelhado),
  Flash (branco ao usar), Empty (círculo escuro e moldura cinza).
Círculo: DiscCenter (UV), DiscRadius (fração da LARGURA da caixa), Aspect (altura / largura da caixa).
Rodar: python3 -I Tools/ue_exec.py -f Tools/editor/criar_material_slot_arte.py
"""
import math
import unreal
FONTE = "/Users/luisfelipexavier/Desktop/Hype World/Tools/editor/criar_materiais_jutsu.py"
_src = open(FONTE).read()
exec(_src[:_src.index("# --- Brilho externo")])   # G, ui, srgb, edge_in, vec (cria materiais em /Game/Ninja/UI/HUD/Jutsu)

g = G("M_UI_JutsuSlotArt", unreal.BlendMode.BLEND_TRANSLUCENT)
ui(g)
selected, cooldown, nochakra = g.scalar("Selected", 0.0), g.scalar("Cooldown", 0.0), g.scalar("NoChakra", 0.0)
flash, empty = g.scalar("Flash", 0.0), g.scalar("Empty", 0.0)
center = g.node(E.MaterialExpressionVectorParameter, parameter_name="DiscCenter", default_value=unreal.LinearColor(0.5, 0.4348, 0.0, 0.0))
disc_r = g.scalar("DiscRadius", 121.0 / 460.0)
aspect = g.scalar("Aspect", 1.0)

tc = g.node(E.MaterialExpressionTextureCoordinate)
tex = g.node(E.MaterialExpressionTextureSampleParameter2D, parameter_name="Slot",
             texture=unreal.load_asset("/Game/Ninja/UI/HUD/Jutsu/Slots/T_JS_Slot1"), sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
g.link(tc, tex, "UVs")
col = g.mask(tex, r=True, g=True, b=True)
ta = g.node(E.MaterialExpressionMultiply); MEL.connect_material_expressions(tex, "A", ta, "A"); g.link(g.c(1.0), ta, "B")   # saída A (alfa)

# p = distância ao centro do círculo em unidades da largura (pixels quadrados); r = 1 na borda da arte
p = g.mul(g.sub(tc, g.mask(center, r=True, g=True)), g.append(g.c(1.0), aspect))
d = g.node(E.MaterialExpressionDistance); g.link(p, d, "A"); g.link(g.node(E.MaterialExpressionConstant2Vector, r=0.0, g=0.0), d, "B")
r = g.div(d, disc_r)
dx, dy = g.un(E.MaterialExpressionDDX, r), g.un(E.MaterialExpressionDDY, r)
aa = g.un(E.MaterialExpressionSquareRoot, g.add(g.add(g.mul(dx, dx), g.mul(dy, dy)), g.c(1e-8)))
inside = edge_in(g, r, g.c(1.0), aa)

lum = g.node(E.MaterialExpressionDotProduct); g.link(col, lum, "A"); g.link(g.rgb(0.3, 0.59, 0.11), lum, "B")
art = g.mul(col, g.add(g.c(1.0), g.mul(selected, g.c(0.18))))
art = g.lerp(art, g.mul(g.mul(lum, g.c(0.6)), g.rgb(1.0, 0.18, 0.15)), g.mul(nochakra, g.c(0.85)))
on_cd = g.smooth(0.0, 0.001, cooldown)
art = g.lerp(art, g.mul(g.add(g.mul(lum, g.c(0.7)), g.c(0.02)), g.rgb(0.42, 0.44, 0.48)), on_cd)
ang = g.node(E.MaterialExpressionArctangent2)
g.link(g.mask(p, r=True), ang, "Y"); g.link(g.mul(g.mask(p, g=True), g.c(-1.0)), ang, "X")
a01 = g.frac(g.add(g.div(ang, g.c(2.0 * math.pi)), g.c(1.0)))      # 0 no topo, cresce no sentido horário
sweep = g.mul(g.smooth(g.sub(g.one_minus(cooldown), g.c(0.004)), g.add(g.one_minus(cooldown), g.c(0.004)), a01), on_cd)
art = g.mul(art, g.one_minus(g.mul(sweep, g.c(0.6))))
art = g.lerp(art, g.rgb(1.0, 1.0, 1.0), g.mul(flash, g.c(0.7)))
art = g.lerp(art, g.rgb(*srgb(21, 26, 34)), empty)
frame = g.lerp(col, g.mul(lum, g.rgb(0.8, 0.8, 0.8)), empty)        # vazio: moldura em cinza
g.out(g.lerp(frame, art, inside), ta)
packages.append(g.finish())

print("salvo:", unreal.EditorLoadingAndSavingUtils.save_packages(packages, False))
