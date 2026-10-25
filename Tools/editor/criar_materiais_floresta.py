"""Materiais da floresta do tutorial (25/10/2026), cores da referência do Felipe, pintados pelo vertex color dos modelos do
Blender (Tools/floresta/gerar_assets_floresta.py). Todos iluminados (Default Lit), opacos, sem textura (leve no Mac):
- M_Folhas: miolo #2F6B2A -> borda iluminada verde-limão #9ED14B (VertexColor R), variação por posição no mundo
- M_Tronco: #7A4A35 com faixas verticais de ruído
- M_Pedra: #9A9E9C, topo mais claro
- M_Grama: base #3E7F25 -> ponta #6DB83A (VertexColor R = altura), dois lados, vento leve (SimpleGrassWind x altura)
- M_Flor_Rosa / M_Flor_Branca: pétala (R), miolo amarelo (G), caule verde (B)
- M_Chao_Floresta (landscape): grama #4E9A2E..#5FAA34 com manchas; camada "Terra" (caminhos) #8E6A43
Rodar: python3 -I Tools/ue_exec.py -f Tools/editor/criar_materiais_floresta.py
"""
import unreal
FONTE = "/Users/luisfelipexavier/Desktop/Hype World/Tools/editor/criar_materiais_aura.py"
_src = open(FONTE).read()
exec(_src[:_src.index("packages = []")].replace('FOLDER = "/Game/Ninja/VFX/Materials"', 'FOLDER = "/Game/Ninja/Ambiente/Floresta/Materiais"'))
packages = []


def srgb(hexcor):
    h = hexcor.lstrip("#")
    return [pow(int(h[i:i + 2], 16) / 255.0, 2.2) for i in (0, 2, 4)]


def novo(nome, dois_lados=False):
    g = G(nome, unreal.BlendMode.BLEND_OPAQUE, two_sided=dois_lados)
    g.mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    return g


def saida(g, base, rough=0.85, spec=0.2, wpo=None):
    MEL.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(g.c(rough), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(g.c(spec), "", unreal.MaterialProperty.MP_SPECULAR)
    if wpo is not None:
        MEL.connect_material_property(wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    packages.append(g.finish())


def hexrgb(g, h):
    return g.rgb(*srgb(h))


def vcor(g, canal):
    vc = g.node(E.MaterialExpressionVertexColor)
    return g.mask(vc, r=canal == "r", g=canal == "g", b=canal == "b")


def ruido_mundo(g, escala, lo=0.0, hi=1.0):
    """Ruído suave pela posição no mundo (escala em cm), 0..1"""
    wp = g.node(E.MaterialExpressionWorldPosition)
    n = g.node(E.MaterialExpressionNoise, scale=1.0 / escala, levels=2, output_min=lo, output_max=hi, turbulence=False)
    g.link(wp, n, "Position")
    return n


# --- Folhas (copas e arbustos) ---
g = novo("M_Folhas")
luz = g.smooth(0.12, 0.9, vcor(g, "r"))
cor = g.lerp(hexrgb(g, "#2F6B2A"), hexrgb(g, "#9ED14B"), luz)
cor = g.mul(cor, g.add(g.c(0.88), g.mul(ruido_mundo(g, 900.0), g.c(0.24))))   # árvores vizinhas com tons diferentes
saida(g, cor, rough=0.9, spec=0.15)

# --- Tronco ---
g = novo("M_Tronco")
cor = g.mul(hexrgb(g, "#7A4A35"), g.add(g.c(0.7), g.mul(vcor(g, "r"), g.c(0.5))))
cor = g.mul(cor, g.add(g.c(0.85), g.mul(ruido_mundo(g, 40.0), g.c(0.3))))
saida(g, cor, rough=0.9, spec=0.1)

# --- Pedra ---
g = novo("M_Pedra")
cor = g.mul(hexrgb(g, "#9A9E9C"), g.add(g.c(0.72), g.mul(vcor(g, "r"), g.c(0.45))))
saida(g, cor, rough=0.8, spec=0.25)

# --- Grama (vento leve; a cor muda um pouco entre tufos) ---
g = novo("M_Grama")   # frente e verso já vêm no modelo (dois lados fazia o verso preto)
alt = vcor(g, "r")
cor = g.lerp(hexrgb(g, "#2E6A1E"), g.lerp(hexrgb(g, "#4E9A2E"), hexrgb(g, "#6DB83A"), ruido_mundo(g, 600.0)), g.smooth(0.0, 0.85, alt))
vento = g.node(E.MaterialExpressionMaterialFunctionCall)
vento.set_editor_property("material_function", unreal.load_asset("/Engine/Functions/Engine_MaterialFunctions01/WorldPositionOffset/SimpleGrassWind"))
MEL.connect_material_expressions(g.c(0.25), "", vento, "WindIntensity")
MEL.connect_material_expressions(alt, "", vento, "WindWeight")
MEL.connect_material_expressions(g.c(0.35), "", vento, "WindSpeed")
MEL.connect_material_expressions(g.rgb(0.0, 0.0, 0.0), "", vento, "AdditionalWPO")   # sem isso o material não compila
saida(g, cor, rough=0.9, spec=0.15, wpo=vento)

# --- Flores ---
for nome, petala in (("M_Flor_Rosa", "#F29AC4"), ("M_Flor_Branca", "#F4F4EE")):
    g = novo(nome)
    cor = g.lerp(hexrgb(g, "#3F8526"), hexrgb(g, petala), vcor(g, "r"))
    cor = g.lerp(cor, hexrgb(g, "#F2C94A"), vcor(g, "g"))
    saida(g, cor, rough=0.8, spec=0.2)

# --- Chão (landscape): grama com manchas + camada Terra (caminhos) ---
g = novo("M_Chao_Floresta")
grama = g.lerp(hexrgb(g, "#3F8526"), hexrgb(g, "#5FAA34"), ruido_mundo(g, 700.0))
grama = g.mul(grama, g.add(g.c(0.85), g.mul(ruido_mundo(g, 150.0), g.c(0.25))))
terra = g.mul(hexrgb(g, "#8E6A43"), g.add(g.c(0.8), g.mul(ruido_mundo(g, 120.0), g.c(0.35))))
peso = g.node(E.MaterialExpressionLandscapeLayerSample, parameter_name="Terra", preview_weight=0.0)
cor = g.lerp(grama, terra, g.smooth(0.25, 0.75, peso))
saida(g, cor, rough=0.95, spec=0.1)

print("salvo:", unreal.EditorLoadingAndSavingUtils.save_packages(packages, False))
