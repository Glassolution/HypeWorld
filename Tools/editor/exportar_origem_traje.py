# Grupo 2 dos trajes (03/11/2026): personagens que só existem na Unreal. Exporta a malha para FBX e as texturas do material
# para PNG em SourceAssets/Customization/IndraModular/Origem/<Personagem>, para servirem de origem ao traje modular.
import unreal
from pathlib import Path

ORIGENS = {'Shisui': '/Game/HypeWorld/Personagens/Shisui/SK_Shisui', 'Sasuke': '/Game/HypeWorld/Personagens/Sasuke/Limpo/SK_Sasuke'}
L = unreal.MaterialEditingLibrary
ROOT = Path(unreal.Paths.project_dir()).resolve()


def exportar(obj, destino, exporter=None):
    t = unreal.AssetExportTask()
    t.object = obj
    t.filename = str(destino)
    t.automated = True
    t.prompt = False
    t.replace_identical = True
    if exporter:
        t.exporter = exporter
    return unreal.Exporter.run_asset_export_task(t)


for nome, path in ORIGENS.items():
    out = ROOT / 'SourceAssets/Customization/IndraModular/Origem' / nome
    out.mkdir(parents=True, exist_ok=True)
    m = unreal.load_asset(path)
    ossos = [str(b) for b in m.get_editor_property('skeleton').get_editor_property('bone_tree')] if False else []
    ok = exportar(m, out / (nome + '.fbx'), unreal.SkeletalMeshExporterFBX())
    unreal.log('ORIGEM %s fbx %s' % (nome, ok))
    for slot in m.get_editor_property('materials'):
        mi = slot.get_editor_property('material_interface')
        sn = str(slot.get_editor_property('material_slot_name'))
        texs = L.get_material_used_textures(mi) if mi else []
        unreal.log('ORIGEM %s slot %s mat %s texs %s' % (nome, sn, mi.get_name() if mi else None, [(t.get_name(), t.blueprint_get_size_x(), t.blueprint_get_size_y()) for t in texs]))
        for t in texs:
            exportar(t, out / (t.get_name() + '.png'), unreal.TextureExporterPNG())
