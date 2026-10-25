"""Cria M_UI_Minimapa: mostra a textura "Map" (a câmera do minimapa) ignorando a transparência dela e recorta um círculo.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/criar_material_minimapa.py
"""
import unreal

PATH = "/Game/Ninja/UI/M_UI_Minimapa"
MEL = unreal.MaterialEditingLibrary

if unreal.EditorAssetLibrary.does_asset_exist(PATH):
    mat = unreal.load_asset(PATH)
    MEL.delete_all_material_expressions(mat)
else:
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_UI_Minimapa", "/Game/Ninja/UI", unreal.Material, unreal.MaterialFactoryNew())

mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

# Cor: a imagem da câmera (só RGB)
tex = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 0)
tex.set_editor_property("parameter_name", "Map")
tex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/DefaultTexture"))
MEL.connect_material_property(tex, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# Opacidade: círculo com borda suave = saturate((0.5 - distância ao centro) * 60)
uv = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -900, 300)
center = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant2Vector, -900, 420)
center.set_editor_property("r", 0.5)
center.set_editor_property("g", 0.5)
dist = MEL.create_material_expression(mat, unreal.MaterialExpressionDistance, -700, 350)
MEL.connect_material_expressions(uv, "", dist, "A")
MEL.connect_material_expressions(center, "", dist, "B")
half = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -700, 480)
half.set_editor_property("r", 0.5)
sub = MEL.create_material_expression(mat, unreal.MaterialExpressionSubtract, -500, 380)
MEL.connect_material_expressions(half, "", sub, "A")
MEL.connect_material_expressions(dist, "", sub, "B")
mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 380)
mul.set_editor_property("const_b", 60.0)
MEL.connect_material_expressions(sub, "", mul, "A")
sat = MEL.create_material_expression(mat, unreal.MaterialExpressionSaturate, -200, 380)
MEL.connect_material_expressions(mul, "", sat, "")
MEL.connect_material_property(sat, "", unreal.MaterialProperty.MP_OPACITY)

MEL.recompile_material(mat)
print("salvo:", unreal.EditorAssetLibrary.save_asset(PATH, only_if_is_dirty=False))
print("nós:", MEL.get_num_material_expressions(mat))
