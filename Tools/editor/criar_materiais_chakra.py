"""Cria os materiais da aura de chakra e do flash de impacto em /Game/Ninja/VFX/Materials.
Todos aditivos e sem luz (brilham sozinhos). Parâmetros: Color, Intensity e (quando usa) a textura Noise.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/criar_materiais_chakra.py
"""
import unreal

FOLDER = "/Game/Ninja/VFX/Materials"
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
DEFAULT_TEX = unreal.load_asset("/Engine/EngineResources/DefaultTexture")


class G:
    """Ajuda a montar o grafo do material com poucas linhas."""

    def __init__(self, name, two_sided=False, usages=()):
        path = f"{FOLDER}/{name}"
        mat = unreal.load_asset(path)
        if mat:
            MEL.delete_all_material_expressions(mat)
        else:
            mat = tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        mat.set_editor_property("two_sided", two_sided)
        for u in usages:
            MEL.set_material_usage(mat, u)
        self.mat, self.path, self.x = mat, path, -1600

    def node(self, cls, **props):
        n = MEL.create_material_expression(self.mat, cls, self.x, 0)
        self.x += 140
        for k, v in props.items():
            n.set_editor_property(k, v)
        return n

    def link(self, a, b, pin="", out=""):
        MEL.connect_material_expressions(a, out, b, pin)

    def op(self, cls, a, b=None, **props):
        n = self.node(cls, **props)
        self.link(a, n, "A" if b is not None or cls not in (unreal.MaterialExpressionOneMinus, unreal.MaterialExpressionSaturate, unreal.MaterialExpressionAbs, unreal.MaterialExpressionFrac) else "")
        if b is not None:
            self.link(b, n, "B")
        return n

    def mul(self, a, b): return self.op(unreal.MaterialExpressionMultiply, a, b)
    def add(self, a, b): return self.op(unreal.MaterialExpressionAdd, a, b)
    def sub(self, a, b): return self.op(unreal.MaterialExpressionSubtract, a, b)
    def one_minus(self, a): return self.op(unreal.MaterialExpressionOneMinus, a)
    def sat(self, a): return self.op(unreal.MaterialExpressionSaturate, a)
    def absn(self, a): return self.op(unreal.MaterialExpressionAbs, a)
    def frac(self, a): return self.op(unreal.MaterialExpressionFrac, a)
    def const(self, v): return self.node(unreal.MaterialExpressionConstant, r=v)
    def power(self, a, exp):
        n = self.node(unreal.MaterialExpressionPower, const_exponent=exp)
        self.link(a, n, "Base")
        return n

    def params(self):
        color = self.node(unreal.MaterialExpressionVectorParameter, parameter_name="Color", default_value=unreal.LinearColor(0.05, 0.35, 1.0, 1.0))
        inten = self.node(unreal.MaterialExpressionScalarParameter, parameter_name="Intensity", default_value=1.0)
        return color, inten

    def noise(self, scale_u, scale_v, speed_x, speed_y):
        uv = self.node(unreal.MaterialExpressionTextureCoordinate, u_tiling=scale_u, v_tiling=scale_v)
        pan = self.node(unreal.MaterialExpressionPanner, speed_x=speed_x, speed_y=speed_y)
        self.link(uv, pan, "Coordinate")
        tex = self.node(unreal.MaterialExpressionTextureSampleParameter2D, parameter_name="Noise", texture=DEFAULT_TEX,
                        sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        self.link(pan, tex, "UVs")
        return tex

    def fresnel(self, exponent):
        return self.node(unreal.MaterialExpressionFresnel, exponent=exponent, base_reflect_fraction=0.0)

    def emissive(self, n):
        MEL.connect_material_property(n, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    def finish(self):
        MEL.delete_unused_expressions(self.mat)
        MEL.recompile_material(self.mat)
        print("OK", self.path, "nós:", MEL.get_num_material_expressions(self.mat))
        return self.mat.get_outermost()


packages = []

# 1) Brilho no corpo: borda azul forte + ruído subindo; incha 2,5 cm para fora da pele
g = G("M_Chakra_Shell", usages=[unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH])
color, inten = g.params()
n = g.noise(3.0, 2.0, 0.0, -0.7)
body = g.mul(g.fresnel(2.5), g.add(g.const(0.25), n))
g.emissive(g.mul(g.mul(color, inten), g.mul(body, g.const(2.2))))
normal = g.node(unreal.MaterialExpressionVertexNormalWS)
MEL.connect_material_property(g.mul(g.mul(normal, g.const(2.5)), inten), "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
packages.append(g.finish())

# 2) Chamas: duas camadas de ruído subindo, somem em cima; sem tampas e com bordas suaves (labaredas, não um tubo)
g = G("M_Chakra_Flame", two_sided=True)
color, inten = g.params()
n1 = g.noise(2.0, 1.0, 0.04, -0.9)
n2 = g.noise(4.0, 2.0, -0.07, -1.5)
uv = g.node(unreal.MaterialExpressionTextureCoordinate)
v = g.node(unreal.MaterialExpressionComponentMask, r=False, g=True, b=False, a=False)
g.link(uv, v)
height = g.one_minus(v)                                     # 0 embaixo, 1 em cima (cilindro da engine: V=0 no topo)
flame = g.sat(g.sub(g.mul(g.mul(n1, n2), g.const(2.8)), g.mul(height, g.const(1.5))))
bottom_fade = g.mul(g.sat(g.mul(height, g.const(6.0))), g.sat(g.mul(g.one_minus(height), g.const(3.0))))   # some embaixo e em cima
normal = g.node(unreal.MaterialExpressionVertexNormalWS)
nz = g.node(unreal.MaterialExpressionComponentMask, r=False, g=False, b=True, a=False)
g.link(normal, nz)
no_caps = g.power(g.one_minus(g.absn(nz)), 4.0)            # tampas de cima/baixo somem
f = g.fresnel(1.2)
inv = g.one_minus(f)
band = g.mul(g.mul(f, g.mul(inv, inv)), g.const(6.75))   # pico a 1/3 do caminho até a borda, zero na silhueta
shape = g.mul(g.mul(g.mul(flame, bottom_fade), no_caps), g.add(g.const(0.15), band))
core = g.mul(g.power(flame, 4.0), g.const(0.5))             # um toque de branco só no miolo das labaredas
g.emissive(g.mul(inten, g.add(g.mul(g.mul(color, shape), g.const(2.2)), g.mul(core, color))))
packages.append(g.finish())

# 3) Anéis no chão: duas ondas expandindo + brilho no centro
g = G("M_Chakra_Ring", two_sided=True)
color, inten = g.params()
uv = g.node(unreal.MaterialExpressionTextureCoordinate)
center = g.node(unreal.MaterialExpressionConstant2Vector, r=0.5, g=0.5)
dist = g.node(unreal.MaterialExpressionDistance)
g.link(uv, dist, "A")
g.link(center, dist, "B")
d = g.mul(dist, g.const(2.0))                               # 0 no centro, 1 na borda
time = g.mul(g.node(unreal.MaterialExpressionTime), g.const(0.65))
rings = None
for offset in (0.0, 0.5):
    r = g.frac(g.add(time, g.const(offset)))
    ring = g.power(g.sat(g.sub(g.const(1.0), g.mul(g.absn(g.sub(d, r)), g.const(22.0)))), 2.0)
    ring = g.mul(ring, g.one_minus(r))
    rings = ring if rings is None else g.add(rings, ring)
glow = g.mul(g.power(g.sat(g.one_minus(g.mul(d, g.const(2.2)))), 2.0), g.const(0.6))
g.emissive(g.mul(g.mul(color, inten), g.mul(g.add(rings, glow), g.const(3.0))))
packages.append(g.finish())

# 4) Faíscas: pontos bem brilhantes, puxados para o branco
g = G("M_Chakra_Spark", usages=[unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES])
color, inten = g.params()
g.emissive(g.mul(g.add(g.mul(color, g.const(0.6)), g.const(0.5)), g.mul(inten, g.const(14.0))))
packages.append(g.finish())

# 5) Flash de impacto: esfera brilhante, mais forte na borda
g = G("M_HitFlash")
color, inten = g.params()
g.emissive(g.mul(g.mul(color, inten), g.mul(g.power(g.fresnel(1.0), 2.0), g.const(6.0))))   # casca brilhante, miolo vazio
packages.append(g.finish())

print("salvo:", unreal.EditorLoadingAndSavingUtils.save_packages(packages, False))
