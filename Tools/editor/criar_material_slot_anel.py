"""Slots de jutsu no estilo da referência do Felipe (06/11/2026, barra "Pied de l'Aube"): M_UI_JutsuSlotRing.
Desenha numa imagem quadrada (o widget UNinjaJutsuSlotWidget põe a imagem com o diâmetro do slot):
  - aro escuro grosso (quase preto, puxado para o verde) com borda de fora irregular, de pincel;
  - a arte do jutsu preenchendo o círculo de dentro (vem do recorte dos slots: só o círculo da arte é usado, a moldura
    e a etiqueta do PNG antigo ficam de fora);
  - sombra suave por dentro do aro, sombra escura por fora.
Estados: Selected (arte um pouco mais clara), Cooldown (0..1: arte escura e dessaturada), NoChakra (avermelhado),
Flash (branco ao usar), Empty (miolo escuro, sem arte).
Arte: ArtCenter (UV do centro do círculo na textura), ArtRadius (raio em fração da largura), ArtZoom (<1 corta a borda).
Pode rodar de novo: refaz o mesmo material (não duplica). O M_UI_JutsuSlotArt antigo fica como estava.
Rodar: python3 -I Tools/ue_exec.py -f Tools/editor/criar_material_slot_anel.py
"""
import math
import unreal
FONTE = "/Users/luisfelipexavier/Desktop/Hype World/Tools/editor/criar_materiais_jutsu.py"
_src = open(FONTE).read()
exec(_src[:_src.index("# --- Brilho externo")])   # G, ui, srgb, edge_in, vec (cria materiais em /Game/Ninja/UI/HUD/Jutsu)

g = G("M_UI_JutsuSlotRing", unreal.BlendMode.BLEND_TRANSLUCENT)
ui(g)
selected, cooldown, nochakra = g.scalar("Selected", 0.0), g.scalar("Cooldown", 0.0), g.scalar("NoChakra", 0.0)
flash, empty = g.scalar("Flash", 0.0), g.scalar("Empty", 0.0)
art_center = g.node(E.MaterialExpressionVectorParameter, parameter_name="ArtCenter", default_value=unreal.LinearColor(0.5, 200.0 / 460.0, 0.0, 0.0))
art_radius = g.scalar("ArtRadius", 121.0 / 460.0)
art_zoom = g.scalar("ArtZoom", 0.9)
inner_base = 0.76      # raio da arte (1 = borda da imagem)

tc = g.node(E.MaterialExpressionTextureCoordinate)
p = g.sub(tc, g.node(E.MaterialExpressionConstant2Vector, r=0.5, g=0.5))
d = g.node(E.MaterialExpressionDistance); g.link(tc, d, "A"); g.link(g.node(E.MaterialExpressionConstant2Vector, r=0.5, g=0.5), d, "B")
r = g.mul(d, g.c(2.0))
dx, dy = g.un(E.MaterialExpressionDDX, r), g.un(E.MaterialExpressionDDY, r)
aa = g.un(E.MaterialExpressionSquareRoot, g.add(g.add(g.mul(dx, dx), g.mul(dy, dy)), g.c(1e-8)))
ang = g.node(E.MaterialExpressionArctangent2)
g.link(g.mask(p, g=True), ang, "Y"); g.link(g.mask(p, r=True), ang, "X")
a01 = g.frac(g.add(g.div(ang, g.c(2.0 * math.pi)), g.c(1.0)))   # ângulo 0..1 (frequências inteiras = sem emenda)


def wave(terms):
    """soma de senos do ângulo: [(voltas, fase 0..1, amplitude)]"""
    acc = None
    for k, ph, amp in terms:
        s = g.mul(g.sine(g.add(g.mul(a01, g.c(float(k))), g.c(ph))), g.c(amp))
        acc = s if acc is None else g.add(acc, s)
    return acc


# Borda de fora irregular (pincel) e borda de dentro quase redonda
outer = g.add(g.c(0.93), wave([(3, 0.11, 0.014), (5, 0.37, 0.012), (8, 0.71, 0.010), (13, 0.23, 0.007), (21, 0.58, 0.004)]))
inner = g.add(g.c(inner_base), wave([(7, 0.42, 0.006), (13, 0.05, 0.004)]))
in_outer = edge_in(g, r, outer, aa)
in_inner = edge_in(g, r, inner, aa)

# Aro: quase preto puxado para o verde, com fios de pincel claros/escuros e um leve realce em cima
bristle = g.sat(g.add(g.mul(wave([(6, 0.3, 0.5), (11, 0.8, 0.35), (19, 0.15, 0.2)]), g.c(0.6)), g.c(0.5)))
ring = g.lerp(g.rgb(*srgb(10, 15, 13)), g.rgb(*srgb(26, 36, 32)), g.mul(bristle, g.c(0.6)))
top = g.sat(g.mul(g.mask(p, g=True), g.c(-2.4)))   # 0 embaixo/meio .. ~1 no topo
ring = g.mul(ring, g.add(g.c(1.0), g.mul(top, g.c(0.35))))

# Arte do jutsu: só o círculo da arte do recorte, um pouco ampliado para cortar a moldura antiga
uv_art = g.add(g.mask(art_center, r=True, g=True), g.mul(g.div(p, g.c(inner_base * 0.5)), g.mul(art_radius, art_zoom)))
tex = g.node(E.MaterialExpressionTextureSampleParameter2D, parameter_name="Slot",
             texture=unreal.load_asset("/Game/Ninja/UI/HUD/Jutsu/Slots/T_JS_Slot1"), sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
g.link(uv_art, tex, "UVs")
col = g.mask(tex, r=True, g=True, b=True)
lum = g.node(E.MaterialExpressionDotProduct); g.link(col, lum, "A"); g.link(g.rgb(0.3, 0.59, 0.11), lum, "B")
art = g.mul(col, g.add(g.c(1.0), g.mul(selected, g.c(0.1))))
art = g.lerp(art, g.mul(g.mul(lum, g.c(0.6)), g.rgb(1.0, 0.18, 0.15)), g.mul(nochakra, g.c(0.85)))
on_cd = g.smooth(0.0, 0.001, cooldown)
grey = g.lerp(art, g.mul(lum, g.rgb(0.55, 0.6, 0.62)), g.c(0.25))
art = g.lerp(art, g.mul(grey, g.c(0.42)), on_cd)          # cooldown: escurece (o número grande fica em cima)
art = g.lerp(art, g.rgb(1.0, 1.0, 1.0), g.mul(flash, g.c(0.7)))
# vazio: miolo escuro com um degradê suave
empty_fill = g.lerp(g.rgb(*srgb(30, 38, 36)), g.rgb(*srgb(14, 19, 18)), g.sat(g.div(r, g.c(inner_base))))
art = g.lerp(art, empty_fill, empty)
# sombra do aro por dentro da arte
inner_shadow = g.smooth(g.mul(inner, g.c(0.72)), inner, r)
art = g.mul(art, g.one_minus(g.mul(inner_shadow, g.c(0.5))))

color = g.lerp(ring, art, in_inner)
# sombra escura por fora do aro (deixa o slot destacado em qualquer fundo)
shadow = g.mul(g.one_minus(g.smooth(outer, g.add(outer, g.c(0.07)), r)), g.c(0.35))
alpha = g.mx(in_outer, shadow)
color = g.mul(color, in_outer)
g.out(color, alpha)
packages.append(g.finish())

print("salvo:", unreal.EditorLoadingAndSavingUtils.save_packages(packages, False))

# Tamanhos da barra no WBP_NinjaHUD (o WBP guarda os valores antigos de 50 px; só depois do C++ novo compilado)
bp = unreal.load_asset("/Game/Ninja/UI/WBP_NinjaHUD")
cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("jutsu_slot_size", 84.0)        # diâmetro do slot com o aro (px em 1080p)
cdo.set_editor_property("jutsu_slot_gap", 12.0)         # pouco espaço entre os círculos, como na referência
cdo.set_editor_property("jutsu_bottom_margin", 0.03)
cdo.set_editor_property("jutsu_hidden_offset", 112.0)   # fora da batalha só o topo do aro aparece
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
print("WBP: slot", cdo.get_editor_property("jutsu_slot_size"), "gap", cdo.get_editor_property("jutsu_slot_gap"))
