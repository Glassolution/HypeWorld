assert not GI_EVENTS
rows=[]
for a in unreal.GameplayStatics.get_all_actors_of_class(W,unreal.Actor):
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        mesh=c.get_editor_property('static_mesh')
        if not mesh:continue
        if not any(t in mesh.get_name().lower() for t in ['tree','bush']):continue
        mats=[]
        for m in c.get_materials():
            if not m:continue
            base=m.get_base_material()
            mats.append({'path':m.get_path_name(),'base':base.get_path_name(),'blend':str(base.get_editor_property('blend_mode'))})
        rows.append({'component':c.get_path_name(),'mesh':mesh.get_path_name(),'materials':mats,'bounds':str(unreal.SystemLibrary.get_component_bounds(c)),'transform':str(c.get_world_transform()),'instances':c.get_instance_count() if isinstance(c,unreal.InstancedStaticMeshComponent) else None})
(OUT/'vegetacao.json').write_text(json.dumps(rows,indent=2))
print('vegetation components',len(rows))
