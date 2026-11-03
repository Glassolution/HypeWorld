# Piloto do traje modular (01/11/2026): importa SK_Outfit_ItachiAnbu (gerado por Tools/customization/traje_itachi_piloto)
# no SKEL_Indra, confere a bind pose e cria o material de traje com pele:
# - M_Traje_Pele (cópia do M_Indra): fora da máscara = textura; na máscara = textura / SkinRef * SkinTone.
# - MI_Traje_ItachiAnbu_1/2: textura do uniforme + máscara de pele + SkinRef (cor da pele pintada nessa textura).
# - M_Indra_Cranio ganha o mesmo SkinTone (textura / SkinRef * SkinTone) e passa a ser o material de toda a pele da cabeça.
# Com o valor padrão (a cor do rosto) o rosto não muda. Pode rodar de novo: substitui a malha e reaproveita o resto.
import unreal, sys, math, json
from pathlib import Path

ROOT = Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0, str(ROOT / 'Content/Python'))
import indra_character_standard as standard

DATA = ROOT / 'SourceAssets/Customization/IndraModular/ItachiAnbu'
INDRA_DIR = '/Game/HypeWorld/Personagens/Indra'
MOD = INDRA_DIR + '/Modular'
FOLDER = MOD + '/Trajes/ItachiAnbu'
E = unreal.EditorAssetLibrary
A = unreal.AssetToolsHelpers.get_asset_tools()
L = unreal.MaterialEditingLibrary
NOME = 'SK_Outfit_ItachiAnbu'
PELE_ROSTO_SRGB = (0.933, 0.808, 0.741)   # cor medida na textura do Indra (testa, nuca)


def linear(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def cor_linear(rgb):
    return unreal.LinearColor(linear(rgb[0]), linear(rgb[1]), linear(rgb[2]), 1)


SKIN_PADRAO = cor_linear(PELE_ROSTO_SRGB)
unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX false')
skel = unreal.load_asset(INDRA_DIR + '/SKEL_Indra')
esperado = standard._reference_bones(unreal.load_asset(INDRA_DIR + '/SK_Indra'))


def node(mat, cls, x, y, **props):
    n = L.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        n.set_editor_property(k, v)
    return n


def tom_de_pele(mat, entrada, x, y, ref):
    """Devolve o nó entrada / SkinRef * SkinTone."""
    tom = node(mat, unreal.MaterialExpressionVectorParameter, x, y, parameter_name='SkinTone', default_value=SKIN_PADRAO)
    pele = node(mat, unreal.MaterialExpressionVectorParameter, x, y + 160, parameter_name='SkinRef', default_value=ref)
    razao = node(mat, unreal.MaterialExpressionDivide, x + 220, y + 40)
    L.connect_material_expressions(tom, '', razao, 'A')
    L.connect_material_expressions(pele, '', razao, 'B')
    mul = node(mat, unreal.MaterialExpressionMultiply, x + 420, y)
    L.connect_material_expressions(entrada[0], entrada[1], mul, 'A')
    L.connect_material_expressions(razao, '', mul, 'B')
    return mul


def so_na_pele(mat):
    """O tom só vale onde a textura tem cor de pele: sobrancelhas, contornos e boca ficam iguais ao M_Indra."""
    if 'Pele Tolerancia' in [str(n) for n in L.get_scalar_parameter_names(mat)]:
        return
    mul = L.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
    atual = L.get_inputs_for_material_expression(mat, mul)[0]
    tex = L.get_inputs_for_material_expression(mat, atual)[0]
    ref = node(mat, unreal.MaterialExpressionConstant3Vector, -700, 1100, constant=SKIN_PADRAO)
    dist = node(mat, unreal.MaterialExpressionDistance, -500, 1100)
    L.connect_material_expressions(tex, 'RGB', dist, 'A')
    L.connect_material_expressions(ref, '', dist, 'B')
    tol = node(mat, unreal.MaterialExpressionScalarParameter, -500, 1240, parameter_name='Pele Tolerancia', default_value=0.3)
    razao = node(mat, unreal.MaterialExpressionDivide, -300, 1100)
    L.connect_material_expressions(dist, '', razao, 'A')
    L.connect_material_expressions(tol, '', razao, 'B')
    fora = node(mat, unreal.MaterialExpressionSaturate, -150, 1100)
    L.connect_material_expressions(razao, '', fora, '')
    pele = node(mat, unreal.MaterialExpressionOneMinus, 0, 1100)
    L.connect_material_expressions(fora, '', pele, '')
    lerp = node(mat, unreal.MaterialExpressionLinearInterpolate, 150, 900)
    L.connect_material_expressions(atual, '', lerp, 'A')
    L.connect_material_expressions(mul, '', lerp, 'B')
    L.connect_material_expressions(pele, '', lerp, 'Alpha')
    L.connect_material_property(lerp, '', unreal.MaterialProperty.MP_BASE_COLOR)


def cranio_com_tom():
    mat = unreal.load_asset(MOD + '/M_Indra_Cranio')
    if 'SkinTone' not in [str(n) for n in L.get_vector_parameter_names(mat)]:
        atual = L.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
        mul = tom_de_pele(mat, (atual, ''), -700, 760, SKIN_PADRAO)
        L.connect_material_property(mul, '', unreal.MaterialProperty.MP_BASE_COLOR)
    so_na_pele(mat)
    L.recompile_material(mat)
    E.save_loaded_asset(mat, False)
    return mat


def ajustar_mascara(mat, mascara):
    # o nó precisa de uma textura padrão do mesmo tipo da máscara, senão o material não compila no Metal e a Unreal
    # usa o material padrão (cinza quadriculado) em silêncio
    lerp = L.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
    no = L.get_inputs_for_material_expression(mat, lerp)[2]
    no.set_editor_property('texture', mascara)
    no.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    L.recompile_material(mat)
    E.save_loaded_asset(mat, False)


def material_traje():
    path = FOLDER + '/M_Traje_Pele'
    if E.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = E.duplicate_asset(INDRA_DIR + '/M_Indra', path)
    tex = L.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
    tex.set_editor_property('parameter_name', 'Textura')
    masc = node(mat, unreal.MaterialExpressionTextureSampleParameter2D, -900, 500, parameter_name='SkinMask',
                sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    mul = tom_de_pele(mat, (tex, 'RGB'), -900, 800, SKIN_PADRAO)
    lerp = node(mat, unreal.MaterialExpressionLinearInterpolate, -200, 300)
    L.connect_material_expressions(tex, 'RGB', lerp, 'A')
    L.connect_material_expressions(mul, '', lerp, 'B')
    L.connect_material_expressions(masc, 'R', lerp, 'Alpha')
    L.connect_material_property(lerp, '', unreal.MaterialProperty.MP_BASE_COLOR)
    L.set_base_material_usage(mat, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH, True)
    L.recompile_material(mat)
    E.save_loaded_asset(mat, False)
    return mat


def importar_mascara(k):
    task = unreal.AssetImportTask()
    task.filename = str(DATA / ('T_ItachiAnbu_SkinMask_%s.png' % k))
    task.destination_path = FOLDER
    task.automated = True
    task.replace_existing = True
    task.save = True
    A.import_asset_tasks([task])
    tex = unreal.load_asset(FOLDER + '/T_ItachiAnbu_SkinMask_%s' % k)
    tex.set_editor_property('srgb', False)
    tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
    E.save_loaded_asset(tex, False)
    return tex


info = json.loads((DATA / 'info.json').read_text())
m_cranio = cranio_com_tom()
m_traje = material_traje()
ajustar_mascara(m_traje, importar_mascara('1'))
ref = cor_linear(info['pele_ref_srgb'])
instancias = {}
for k in ('1', '2'):
    path = FOLDER + '/MI_Traje_ItachiAnbu_' + k
    mi = unreal.load_asset(path) if E.does_asset_exist(path) else A.create_asset(
        'MI_Traje_ItachiAnbu_' + k, FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property('parent', m_traje)
    L.set_material_instance_texture_parameter_value(mi, 'Textura', unreal.load_asset('/Game/HypeWorld/Personagens/ItachiAnbu/T_KakashiAnbu_' + k))
    L.set_material_instance_texture_parameter_value(mi, 'SkinMask', importar_mascara(k))
    L.set_material_instance_vector_parameter_value(mi, 'SkinRef', ref)
    L.update_material_instance(mi)
    E.save_loaded_asset(mi, False)
    instancias[k] = mi

opts = unreal.FbxImportUI()
opts.set_editor_property('import_as_skeletal', True)
opts.set_editor_property('import_mesh', True)
opts.set_editor_property('import_animations', False)
opts.set_editor_property('import_materials', False)
opts.set_editor_property('import_textures', False)
opts.set_editor_property('create_physics_asset', False)
opts.set_editor_property('skeleton', skel)
data = opts.get_editor_property('skeletal_mesh_import_data')
data.set_editor_property('update_skeleton_reference_pose', False)
data.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
task = unreal.AssetImportTask()
task.filename = str(DATA / (NOME + '.fbx'))
task.destination_path = FOLDER
task.destination_name = NOME
task.automated = True
task.replace_existing = True
task.save = True
task.options = opts
A.import_asset_tasks([task])
mesh = unreal.load_asset(FOLDER + '/' + NOME)
assert mesh and mesh.get_editor_property('skeleton') == skel, NOME
real = standard._reference_bones(mesh)
pior = max(math.dist(real[b]['position_cm'], e['position_cm']) for b, e in esperado.items())
assert pior < 0.1, ('bind pose diferente do SK_Indra', pior)
slots = mesh.get_editor_property('materials')
for i, slot in enumerate(slots):
    sn = str(slot.get_editor_property('imported_material_slot_name'))
    slot.set_editor_property('material_interface', instancias['2' if sn.endswith('_2') else '1'])
    slots[i] = slot
mesh.call_method('SetMaterials', (slots,))
# limites na cena vêm do PHYS_Indra (o corpo todo), senão a malha some quando a pose sai dos limites dela
mesh.set_editor_property('physics_asset', unreal.load_asset(INDRA_DIR + '/PHYS_Indra'))
E.save_loaded_asset(mesh, False)
unreal.log('TRAJE ok: %d ossos, pior diferença %.4f cm, slots %s' % (
    len(real), pior, [str(s.get_editor_property('imported_material_slot_name')) for s in slots]))
unreal.log('TRAJE_PRONTO')
