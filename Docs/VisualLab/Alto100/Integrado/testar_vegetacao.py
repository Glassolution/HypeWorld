from pathlib import Path
exec((Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Alto100/Integrado/preparar.py')).read_text().split("start_suite(['ao_atual'")[0])
ASSETS=json.loads((OUT/'assets_criados.json').read_text())
BUSH=[];CONTACT=[];TREES=[]
CARRIER=unreal.find_object(None,'/Game/HypeWorld/VisualLab/Grama/L_Grama_Baseline.L_Grama_Baseline:PersistentLevel.InstancedFoliageActor_0.FoliageInstancedStaticMeshComponent_18')
assert CARRIER.get_instance_count()==0
CARRIER_ORIG=(CARRIER.get_editor_property('static_mesh'),list(CARRIER.get_materials()),CARRIER.get_editor_property('cast_shadow'))
CARRIER.set_mobility(unreal.ComponentMobility.MOVABLE)
LIGHT_CONTACT={k:LIGHT.get_editor_property(k) for k in ['contact_shadow_length','contact_shadow_length_in_ws','contact_shadow_casting_intensity','contact_shadow_non_casting_intensity']}
for a in unreal.GameplayStatics.get_all_actors_of_class(W,unreal.Actor):
    for c in a.get_components_by_class(unreal.PrimitiveComponent):
        CONTACT.append((c,c.get_editor_property('cast_contact_shadow')))
        if not isinstance(c,unreal.StaticMeshComponent):continue
        mesh=c.get_editor_property('static_mesh')
        if not mesh:continue
        if 'bush' in mesh.get_name().lower() and c!=CARRIER:
            BUSH.append((c,list(c.get_materials())))
        if mesh.get_path_name() in ASSETS['trees'] and isinstance(c,unreal.InstancedStaticMeshComponent):
            candidates=[]
            for i in range(c.get_instance_count()):
                tr=c.get_instance_transform(i,world_space=True);loc=tr.translation;dy=loc.y+312.028006
                if dy>500 and dy<4500 and abs(loc.x)<dy*.85:candidates.append((dy,i,tr))
            candidates.sort(key=lambda x:x[0])
            for _,i,tr in candidates[:2-len(TREES)]:
                TREES.append((c,i,tr,unreal.load_asset(ASSETS['trees'][mesh.get_path_name()])))
assert 1<=len(TREES)<=2,len(TREES)
(OUT/'selecionadas.json').write_text(json.dumps({'trees':[{'component':c.get_path_name(),'index':i,'transform':str(tr)} for c,i,tr,a in TREES],'bushes':[c.get_path_name() for c,m in BUSH],'contact_before':{k:str(v) for k,v in LIGHT_CONTACT.items()}},indent=2))
def set_bush(enabled,contact=False):
    for c,materials in BUSH:
        for i,m in enumerate(materials):
            path=m.get_path_name() if m else ''
            c.set_material(i,unreal.load_asset(ASSETS['bush_materials'][path]) if enabled and path in ASSETS['bush_materials'] else m)
    for c,flag in CONTACT:
        if unreal.SystemLibrary.is_valid(c):c.set_editor_property('cast_contact_shadow',flag)
    for k,v in LIGHT_CONTACT.items():LIGHT.set_editor_property(k,v)
    if contact:
        # Restrict the newly enabled contact effect to bushes; grass remains excluded.
        for c,flag in CONTACT:
            if unreal.SystemLibrary.is_valid(c):c.set_editor_property('cast_contact_shadow',any(c==b for b,m in BUSH))
        LIGHT.set_editor_property('contact_shadow_length',12.)
        LIGHT.set_editor_property('contact_shadow_length_in_ws',True)
        LIGHT.set_editor_property('contact_shadow_casting_intensity',0.5)
        LIGHT.set_editor_property('contact_shadow_non_casting_intensity',0.)
def set_trees(enabled):
    CARRIER.clear_instances()
    for c,i,tr,newmesh in TREES:
        if enabled:
            hidden=unreal.Transform(location=tr.translation,rotation=tr.rotation.rotator(),scale=unreal.Vector(0,0,0))
            c.update_instance_transform(i,hidden,world_space=True,mark_render_state_dirty=True,teleport=True)
            CARRIER.set_static_mesh(newmesh)
            for slot in range(2):CARRIER.set_material(slot,newmesh.get_material(slot))
            CARRIER.set_cast_shadow(True)
            CARRIER.add_instance(tr,world_space=True)
        else:c.update_instance_transform(i,tr,world_space=True,mark_render_state_dirty=True,teleport=True)
    if not enabled:
        CARRIER.set_static_mesh(CARRIER_ORIG[0]);CARRIER.set_cast_shadow(CARRIER_ORIG[2])
        for slot,m in enumerate(CARRIER_ORIG[1]):CARRIER.set_material(slot,m)
def choose_veg(name):
    base_ao();set_bush(name in ['arbustos_material','arbustos_contact'],name=='arbustos_contact');set_trees(name=='copas_opacas');pose(0)
    (OUT/(name+'_estado.json')).write_text(json.dumps(state(),indent=2))
start_suite(['controle','arbustos_material','arbustos_contact','copas_opacas'],choose_veg,'veg')

