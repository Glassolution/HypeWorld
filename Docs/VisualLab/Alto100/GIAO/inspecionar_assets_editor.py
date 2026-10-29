import unreal,json,traceback
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Alto100/GIAO')
try:
    rows=json.loads((OUT/'vegetacao.json').read_text())
    materials={};meshes={}
    ML=unreal.MaterialEditingLibrary
    for row in rows:
        if row['mesh'] not in meshes:
            mesh=unreal.load_asset(row['mesh'])
            meshes[row['mesh']]={'bounds':str(mesh.get_bounds()),'slots':[(str(s.material_slot_name),str(s.material_interface.get_path_name())) for s in mesh.static_materials]}
        for m in row['materials']:
            if m['base'] in materials:continue
            mat=unreal.load_asset(m['base'])
            materials[m['base']]={'attributes':mat.get_editor_property('use_material_attributes'),'blend':str(mat.blend_mode),'bc':str(ML.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR)),'attrs':str(ML.get_material_property_input_node(mat,unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES))}
    docs={n:str(getattr(unreal,n).__doc__) for n in ['GeometryScriptCopyMeshFromAssetOptions','GeometryScriptMeshReadLOD','GeometryScriptCopyMeshToAssetOptions','GeometryScriptMeshWriteLOD']}
    for cls,fn in [('GeometryScript_AssetUtils','copy_mesh_from_static_mesh'),('GeometryScript_AssetUtils','copy_mesh_to_static_mesh'),('GeometryScript_Materials','delete_triangles_by_material_id')]:
        docs[cls+'.'+fn]=str(getattr(getattr(unreal,cls),fn).__doc__)
    (OUT/'assets_editor.json').write_text(json.dumps({'materials':materials,'meshes':meshes,'docs':docs},indent=2))
except Exception:(OUT/'assets_editor_ERRO.txt').write_text(traceback.format_exc())
