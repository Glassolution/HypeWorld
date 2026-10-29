SOUT=OUT.parent/'Sombras';SOUT.mkdir(exist_ok=True)
SHADOW_COMPONENTS=[];rows=[]
for c in unreal.ObjectIterator(unreal.PrimitiveComponent):
    if not c.get_path_name().startswith(W.get_path_name()+':'):continue
    row={'component':c.get_path_name(),'class':c.get_class().get_name(),'cast_shadow':bool(c.get_editor_property('cast_shadow'))}
    for k in ['cast_dynamic_shadow','cast_static_shadow','cast_contact_shadow','visible','hidden_in_game','bounds_scale']:
        try:row[k]=str(c.get_editor_property(k))
        except Exception:pass
    if isinstance(c,unreal.StaticMeshComponent):
        m=c.get_editor_property('static_mesh');row['mesh']=m.get_path_name() if m else None
        if m:
            try:row['nanite']=bool(m.get_editor_property('nanite_settings').get_editor_property('enabled'))
            except Exception:row['nanite']=None
    elif isinstance(c,unreal.SkeletalMeshComponent):
        m=c.get_editor_property('skeletal_mesh_asset');row['mesh']=m.get_path_name() if m else None
    else:row['mesh']=None
    SHADOW_COMPONENTS.append(c);rows.append(row)
(SOUT/'inventario.json').write_text(json.dumps(rows,indent=2))
print('SHADOW_COMPONENTS',len(rows),'casters',sum(x['cast_shadow'] for x in rows))
