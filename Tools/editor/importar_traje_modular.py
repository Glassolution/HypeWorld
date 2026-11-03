# Trajes modulares (03/11/2026): importa SK_Outfit_<Personagem> (gerado por Tools/customization/traje_itachi_piloto) no
# SKEL_Indra, confere a bind pose e cria as instâncias do M_Traje_Pele do piloto (sem alterar esse material).
# Uso no editor: PERSONAGEM = 'KakashiAnbu'; PASTA_TEXTURAS = '/Game/HypeWorld/Personagens/KakashiAnbu'; PREFIXO_TEXTURA = 'T_KakashiAnbu_'
# e depois este arquivo. Com TEXTURAS (lista de caminhos das texturas, uma por slot) em vez de PASTA_TEXTURAS/PREFIXO_TEXTURA,
# vale para personagens com outro número de slots. Pode rodar de novo: substitui a malha e reaproveita o resto.
import unreal, sys, math, json
from pathlib import Path

ROOT = Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0, str(ROOT / 'Content/Python'))
import indra_character_standard as standard

DATA = ROOT / 'SourceAssets/Customization/IndraModular' / PERSONAGEM
INDRA_DIR = '/Game/HypeWorld/Personagens/Indra'
TRAJES = INDRA_DIR + '/Modular/Trajes'
FOLDER = TRAJES + '/' + PERSONAGEM
E = unreal.EditorAssetLibrary
A = unreal.AssetToolsHelpers.get_asset_tools()
L = unreal.MaterialEditingLibrary
NOME = 'SK_Outfit_' + PERSONAGEM


def linear(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def importar_mascara(k):
    nome = 'T_%s_SkinMask_%s' % (PERSONAGEM, k)
    task = unreal.AssetImportTask()
    task.filename = str(DATA / (nome + '.png'))
    task.destination_path = FOLDER
    task.automated = True
    task.replace_existing = True
    task.save = True
    A.import_asset_tasks([task])
    tex = unreal.load_asset(FOLDER + '/' + nome)
    tex.set_editor_property('srgb', False)
    tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
    E.save_loaded_asset(tex, False)
    return tex


unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX false')
skel = unreal.load_asset(INDRA_DIR + '/SKEL_Indra')
esperado = standard._reference_bones(unreal.load_asset(INDRA_DIR + '/SK_Indra'))
info = json.loads((DATA / 'info.json').read_text())
ref = unreal.LinearColor(*[linear(c) for c in info['pele_ref_srgb']], 1)
m_traje = unreal.load_asset(TRAJES + '/ItachiAnbu/M_Traje_Pele')
if 'TEXTURAS' not in globals():
    TEXTURAS = [PASTA_TEXTURAS + '/' + PREFIXO_TEXTURA + k for k in ('1', '2')]
instancias = {}
for i_, caminho in enumerate(TEXTURAS):
    k = str(i_ + 1)
    nome_mi = 'MI_Traje_%s_%s' % (PERSONAGEM, k)
    path = FOLDER + '/' + nome_mi
    mi = unreal.load_asset(path) if E.does_asset_exist(path) else A.create_asset(
        nome_mi, FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property('parent', m_traje)
    L.set_material_instance_texture_parameter_value(mi, 'Textura', unreal.load_asset(caminho))
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
    slot.set_editor_property('material_interface', instancias[sn.rsplit('_', 1)[1]])
    slots[i] = slot
mesh.call_method('SetMaterials', (slots,))
# limites na cena vêm do PHYS_Indra (o corpo todo), senão a malha some quando a pose sai dos limites dela
mesh.set_editor_property('physics_asset', unreal.load_asset(INDRA_DIR + '/PHYS_Indra'))
E.save_loaded_asset(mesh, False)
unreal.log('TRAJE ok: %s, %d ossos, pior diferença %.4f cm, slots %s' % (
    NOME, len(real), pior, [str(s.get_editor_property('imported_material_slot_name')) for s in slots]))
unreal.log('TRAJE_PRONTO')
