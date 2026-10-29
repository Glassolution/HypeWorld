assert not SEVENTS
LIGHT=unreal.GameplayStatics.get_all_actors_of_class(W,unreal.DirectionalLight)[0].get_component_by_class(unreal.DirectionalLightComponent)
L_ORIG=float(LIGHT.get_editor_property('dynamic_shadow_distance_movable_light'))
SH_CV=['r.Shadow.CSM.MaxCascades','r.Shadow.MaxCSMResolution','r.Shadow.MaxResolution','r.Shadow.DistanceScale','r.Shadow.RadiusThreshold','r.Shadow.FilterMethod','r.DistanceFieldShadowing']
CV_ORIG={n:unreal.SystemLibrary.get_console_variable_string_value(n) for n in SH_CV}
# Correct classification of runtime water meshes whose package includes '/Grama/'.
WATER_MISCLASS=[c for c in GROUPS_SH['grama'] if isinstance(c,unreal.StaticMeshComponent) and 'Water' in c.get_editor_property('static_mesh').get_name()]
GROUPS_SH['grama']=[c for c in GROUPS_SH['grama'] if c not in WATER_MISCLASS]
GROUPS_SH['outros']+=WATER_MISCLASS
(SOUT/'classificacao_corrigida.json').write_text(json.dumps({'water_previously_classified_by_package':[c.get_path_name() for c in WATER_MISCLASS],'grass_actual_casters':len(GROUPS_SH['grama'])},indent=2))
SMALL=[]
for c in GROUPS_SH['outros']:
    if not isinstance(c,unreal.StaticMeshComponent):continue
    origin,extent,radius=unreal.SystemLibrary.get_component_bounds(c)
    if radius<100:SMALL.append((c,origin))
CS=[('original',4,2048,L_ORIG,False),('cortes_pequenos',4,2048,L_ORIG,True),('csm3_1536',3,1536,5000.,True),('csm2_2048',2,2048,5000.,True)]
def apply_cs(case):
    name,casc,res,distance,small=case
    shadow_case('baseline')
    for n,v in CV_ORIG.items():cmd(n+' '+v)
    cmd('r.Shadow.CSM.MaxCascades '+str(casc));cmd('r.Shadow.MaxCSMResolution '+str(res))
    LIGHT.set_dynamic_shadow_distance_movable_light(distance)
    cut=[]
    if small:
        cmd('r.Shadow.RadiusThreshold 0.06')
        for c in GROUPS_SH['arbustos_flores']:c.set_cast_shadow(False);cut.append(c.get_path_name())
        # Shadow-only distance cutoff for small props; no geometry culling.
        loc=CAM.get_world_location()
        for c,origin in SMALL:
            if (origin-loc).length()>1500:c.set_cast_shadow(False);cut.append(c.get_path_name())
    (SOUT/(name+'_config.json')).write_text(json.dumps({'light_distance_cm':distance,'cvars':{n:unreal.SystemLibrary.get_console_variable_string_value(n) for n in SH_CV},'aa':state(),'disabled':cut,'small_prop_shadow_cutoff_cm':1500 if small else None},indent=2))
CE=[];CJ=[];CT0=time.perf_counter()
for index,case in enumerate(CS):
    t=index*135;CE.append((t,'select_'+case[0],lambda c=case:apply_cs(c)))
    for i in range(1,4):
        s=t+25+(i-1)*35
        CE.extend([(s,f'start_{case[0]}_{i}',lambda n=case[0],i=i:(cmd(f'CsvProfile STARTFILE=shadow_{n}_r{i}.csv'),cmd('CsvProfile START'))),(s+30,'stop',lambda:cmd('CsvProfile STOP'))])
    CE.append((t+128,'shot_'+case[0],lambda n=case[0]:cmd('Shot filename="'+str(SOUT/(n+'.png'))+'" -nosuffix')))
def ctick(dt):
    try:
        elapsed=time.perf_counter()-CT0
        if CE and elapsed>=CE[0][0]:
            _,label,fn=CE.pop(0);fn();CJ.append({'event':label,'time':elapsed});(SOUT/'cascatas_status.json').write_text(json.dumps(CJ,indent=2))
        if not CE:
            unreal.unregister_slate_post_tick_callback(CH);(SOUT/'cascatas_concluidas.json').write_text(json.dumps({'done':True}))
    except Exception:
        unreal.unregister_slate_post_tick_callback(CH);cmd('CsvProfile STOP');apply_cs(CS[0]);(SOUT/'CASCATAS_ERRO.txt').write_text(traceback.format_exc())
CH=unreal.register_slate_post_tick_callback(ctick)
