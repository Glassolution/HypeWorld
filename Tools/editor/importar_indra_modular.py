# Indra modular (01/11/2026): importa SK_Indra_Head (cabeça careca), SK_Hair_Indra e SK_Outfit_Indra
# (SourceAssets/Customization/IndraModular, gerados pelo script do Blender Tools/customization/indra_cabeca_modular) no SKEL_Indra,
# sem mexer na pose de referência, e confere a bind pose de cada uma contra o SK_Indra (< 0,1 cm por osso).
# Materiais: os mesmos do SK_Indra (M_Indra / M_Indra_Olhos). O crânio novo usa M_Indra_Cranio = cópia do M_Indra com
# um leve escurecimento no topo (cor de vértice R, "Couro", 0 na emenda -> 1 no alto), que só aparece onde o cabelo
# tem falhas. Pode rodar de novo: substitui as malhas e reaproveita o material.
import unreal, sys, math
from pathlib import Path

ROOT = Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0, str(ROOT / 'Content/Python'))
import indra_character_standard as standard

DATA = ROOT / 'SourceAssets/Customization/IndraModular'
INDRA_DIR = '/Game/HypeWorld/Personagens/Indra'
FOLDER = INDRA_DIR + '/Modular'
PECAS = ('SK_Indra_Head', 'SK_Hair_Indra', 'SK_Outfit_Indra')
E = unreal.EditorAssetLibrary
A = unreal.AssetToolsHelpers.get_asset_tools()
L = unreal.MaterialEditingLibrary

unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX false')
skel = unreal.load_asset(INDRA_DIR + '/SKEL_Indra')
indra = unreal.load_asset(INDRA_DIR + '/SK_Indra')
m_indra = unreal.load_asset(INDRA_DIR + '/M_Indra')
m_olhos = unreal.load_asset(INDRA_DIR + '/M_Indra_Olhos')
esperado = standard._reference_bones(indra)


def material_cranio():
    path = FOLDER + '/M_Indra_Cranio'
    if E.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = E.duplicate_asset(INDRA_DIR + '/M_Indra', path)
    tex = L.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)

    def node(cls, x, y, **props):
        n = L.create_material_expression(mat, cls, x, y)
        for k, v in props.items():
            n.set_editor_property(k, v)
        return n
    vc = node(unreal.MaterialExpressionVertexColor, -600, 300)
    forca = node(unreal.MaterialExpressionScalarParameter, -600, 420, parameter_name='Couro Strength', default_value=1.0)
    alfa = node(unreal.MaterialExpressionMultiply, -400, 340)
    L.connect_material_expressions(vc, 'R', alfa, 'A')
    L.connect_material_expressions(forca, '', alfa, 'B')
    escuro = node(unreal.MaterialExpressionVectorParameter, -600, 520, parameter_name='Couro Tint',
                  default_value=unreal.LinearColor(0.80, 0.76, 0.74, 1))
    um = node(unreal.MaterialExpressionConstant3Vector, -600, 640, constant=unreal.LinearColor(1, 1, 1, 1))
    lerp = node(unreal.MaterialExpressionLinearInterpolate, -250, 420)
    L.connect_material_expressions(um, '', lerp, 'A')
    L.connect_material_expressions(escuro, '', lerp, 'B')
    L.connect_material_expressions(alfa, '', lerp, 'Alpha')
    mul = node(unreal.MaterialExpressionMultiply, -100, 200)
    L.connect_material_expressions(tex, 'RGB', mul, 'A')
    L.connect_material_expressions(lerp, '', mul, 'B')
    L.connect_material_property(mul, '', unreal.MaterialProperty.MP_BASE_COLOR)
    L.set_base_material_usage(mat, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH, True)
    L.recompile_material(mat)
    E.save_loaded_asset(mat, False)
    return mat


m_cranio = material_cranio()
for nome in PECAS:
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
    data.set_editor_property('vertex_color_import_option', unreal.VertexColorImportOption.REPLACE)
    task = unreal.AssetImportTask()
    task.filename = str(DATA / (nome + '.fbx'))
    task.destination_path = FOLDER
    task.destination_name = nome
    task.automated = True
    task.replace_existing = True
    task.save = True
    task.options = opts
    A.import_asset_tasks([task])
    mesh = unreal.load_asset(FOLDER + '/' + nome)
    assert mesh, nome
    assert mesh.get_editor_property('skeleton') == skel, nome
    real = standard._reference_bones(mesh)
    pior = max(math.dist(real[b]['position_cm'], e['position_cm']) for b, e in esperado.items())
    assert pior < 0.1, (nome, 'bind pose diferente do SK_Indra', pior)
    slots = mesh.get_editor_property('materials')
    for i, slot in enumerate(slots):
        sn = str(slot.get_editor_property('imported_material_slot_name'))
        mat = m_cranio if sn == 'M_Indra_Cranio' else (m_olhos if sn.startswith('M_Indra_Olhos') else m_indra)
        slot.set_editor_property('material_interface', mat)
        slots[i] = slot
    mesh.call_method('SetMaterials', (slots,))
    E.save_loaded_asset(mesh, False)
    unreal.log('INDRA_MODULAR %s ok: %d ossos, pior diferença %.4f cm, materiais %s' % (
        nome, len(real), pior, [str(s.get_editor_property('imported_material_slot_name')) for s in slots]))
unreal.log('INDRA_MODULAR_PRONTO')
