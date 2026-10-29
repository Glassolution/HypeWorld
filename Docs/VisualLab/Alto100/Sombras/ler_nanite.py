meshes={};details=[]
for c in SHADOW_COMPONENTS:
    if not isinstance(c,unreal.StaticMeshComponent):continue
    m=c.get_editor_property('static_mesh')
    if not m:continue
    path=m.get_path_name()
    if path not in meshes:
        meshes[path]={'nanite_triangles':m.get_num_nanite_triangles(),'nanite_vertices':m.get_num_nanite_vertices()}
    details.append({'component':c.get_path_name(),'mesh':path,'disallow_nanite':bool(c.get_editor_property('disallow_nanite')),'casts':bool(c.get_editor_property('cast_shadow'))})
(SOUT/'nanite_dados.json').write_text(json.dumps({'meshes':meshes,'components':details},indent=2))
lights=[]
for a in unreal.GameplayStatics.get_all_actors_of_class(W,unreal.DirectionalLight):
    c=a.get_component_by_class(unreal.DirectionalLightComponent);row={'actor':a.get_name()}
    for n in ['dynamic_shadow_distance_movable_light','dynamic_shadow_cascades','distance_field_shadow_distance','light_source_angle','shadow_cascade_distribution_exponent']:
        try:row[n]=str(c.get_editor_property(n))
        except Exception:pass
    lights.append(row)
(SOUT/'luzes_antes.json').write_text(json.dumps(lights,indent=2))
print({p:v['nanite_triangles'] for p,v in meshes.items() if v['nanite_triangles']>0})
