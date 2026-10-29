import unreal,json,traceback
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Alto60')
AL=unreal.EditorAssetLibrary;EL=unreal.EditorLevelLibrary
ES=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
D='/Game/HypeWorld/VisualLab/Alto60'
try:
    assert ES.load_level('/Game/Maps/Tutorial/L_Tutorial')
    mapping=json.loads((OUT.parent/'Alto100/Integrado/assets_criados.json').read_text())['bush_materials']
    cut=json.loads((OUT.parent/'Alto100/Sombras/csm3_1536_config.json').read_text())['disabled']
    suffix={p.split(':PersistentLevel.')[1] for p in cut}
    base={'shadow_cuts':[],'bush_materials':[],'contact':[]}
    for a in EL.get_all_level_actors():
        if isinstance(a,unreal.DirectionalLight):
            light=a.get_component_by_class(unreal.DirectionalLightComponent)
            light.set_editor_property('dynamic_shadow_distance_movable_light',5000.)
            light.set_editor_property('dynamic_shadow_cascades',3)
            light.set_editor_property('contact_shadow_length',12.)
            light.set_editor_property('contact_shadow_length_in_ws',True)
            light.set_editor_property('contact_shadow_casting_intensity',0.5)
            light.set_editor_property('contact_shadow_non_casting_intensity',0.)
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            mesh=c.get_editor_property('static_mesh')
            if not mesh:continue
            path=c.get_path_name();name=mesh.get_name().lower();is_bush='bush' in name
            # Match the exact cuts measured on the copied level, by actor/component suffix.
            if path.split(':PersistentLevel.')[-1] in suffix:
                c.set_cast_shadow(False);base['shadow_cuts'].append(path)
            c.set_editor_property('cast_contact_shadow',is_bush)
            if is_bush:
                base['contact'].append(path)
                for slot,m in enumerate(c.get_materials()):
                    if m and m.get_path_name() in mapping:
                        c.set_material(slot,AL.load_asset(mapping[m.get_path_name()]));base['bush_materials'].append(path)
                    elif m and m.get_path_name() in mapping.values():base['bush_materials'].append(path)
        # Mark owning actors/packages so native map save captures component changes.
        a.modify()
    assert len(base['shadow_cuts'])==len(suffix),(len(base['shadow_cuts']),len(suffix))
    assert ES.save_current_level()
    (OUT/'base_aplicada.json').write_text(json.dumps(base,indent=2))
    assert ES.new_level_from_template(D+'/L_Base_Aprovada','/Game/Maps/Tutorial/L_Tutorial')
    assert ES.new_level_from_template(D+'/L_Alto60',D+'/L_Base_Aprovada')
    # Low-poly opaque mesh for hidden shadow proxies; no edits to visible tree meshes.
    proxy_path=D+'/SM_ProxySombra'
    proxy=AL.load_asset(proxy_path) if AL.does_asset_exist(proxy_path) else AL.duplicate_asset('/Engine/BasicShapes/Sphere',proxy_path)
    dyn=unreal.DynamicMesh()
    unreal.GeometryScript_Primitives.append_sphere_lat_long(dyn,unreal.GeometryScriptPrimitiveOptions(),unreal.Transform(),50.,8,12)
    options=unreal.GeometryScriptCopyMeshToAssetOptions()
    options.set_editor_property('replace_materials',True)
    options.set_editor_property('new_materials',[AL.load_asset('/Engine/BasicShapes/BasicShapeMaterial')])
    unreal.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(dyn,proxy,options,unreal.GeometryScriptMeshWriteLOD())
    AL.save_loaded_asset(proxy)
    records=[];rocks=[]
    for a in list(EL.get_all_level_actors()):
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            mesh=c.get_editor_property('static_mesh')
            if not mesh:continue
            name=mesh.get_name().lower()
            if any(x in name for x in ['rock','stone','boulder','cliff']):
                c.set_force_disable_nanite(True);rocks.append(c.get_path_name());a.modify()
            if not any(x in name for x in ['tree','tronco','toco']):continue
            if not c.get_editor_property('cast_shadow'):continue
            c.set_cast_shadow(False);a.modify()
            b=mesh.get_bounds();center=b.origin;ext=b.box_extent
            trs=[c.get_instance_transform(i,world_space=True) for i in range(c.get_instance_count())] if isinstance(c,unreal.InstancedStaticMeshComponent) else [c.get_world_transform()]
            for i,tr in enumerate(trs):
                # Crown occupies upper ~55% of source bounds; tree transform/layout preserved.
                local=unreal.Vector(center.x,center.y,center.z+ext.z*.38)
                pos=unreal.MathLibrary.transform_location(tr,local)
                scale=tr.scale3d
                size=unreal.Vector(max(ext.x,40)*scale.x/50,max(ext.y,40)*scale.y/50,max(ext.z*.57,60)*scale.z/50)
                actor=EL.spawn_actor_from_class(unreal.StaticMeshActor,pos,tr.rotation.rotator())
                actor.set_actor_label('VL_ProxySombra_'+str(len(records)))
                pc=actor.get_component_by_class(unreal.StaticMeshComponent)
                pc.set_static_mesh(proxy);actor.set_actor_scale3d(size)
                pc.set_cast_shadow(True);pc.set_editor_property('cast_hidden_shadow',True)
                pc.set_editor_property('cast_contact_shadow',False)
                actor.set_actor_hidden_in_game(True)
                actor.modify();records.append({'tree':c.get_path_name(),'instance':i,'proxy':actor.get_name()})
    for a in EL.get_all_level_actors():
        if isinstance(a,unreal.DirectionalLight):
            a.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property('dynamic_shadow_distance_movable_light',3000.)
    assert ES.save_current_level()
    (OUT/'lab_criado.json').write_text(json.dumps({'map':D+'/L_Alto60','proxies':records,'rocks_no_nanite':rocks},indent=2))
except Exception:
    (OUT/'EDITOR_ERRO.txt').write_text(traceback.format_exc());raise
