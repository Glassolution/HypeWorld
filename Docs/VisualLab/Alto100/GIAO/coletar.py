NAMES.extend(['r.Lumen.DiffuseIndirect.Allow','r.DistanceFieldAO','r.AOQuality','r.DynamicGlobalIlluminationMethod','r.DynamicRes.OperationMode'])
GI_CASES=['lumen_high','irradiance_volume','skylight_dfao']
def select_gi(name):
    s=PP.get_editor_property('settings')
    s.set_editor_property('override_dynamic_global_illumination_method',True)
    s.set_editor_property('dynamic_global_illumination_method',unreal.DynamicGlobalIlluminationMethod.NONE if name=='skylight_dfao' else unreal.DynamicGlobalIlluminationMethod.LUMEN)
    PP.set_editor_property('settings',s)
    cmd('r.Lumen.DiffuseIndirect.Allow '+('0' if name=='skylight_dfao' else '1'))
    cmd('r.Lumen.FinalGatherMethod '+('0' if name=='irradiance_volume' else '1'))
    cmd('r.DistanceFieldAO 1')
    pose(0)
    (OUT/(name+'_estado.json')).write_text(json.dumps({'cvars':state(),'gi':str(s.get_editor_property('dynamic_global_illumination_method')),'camera':str(CAM.get_world_location()),'rotation':str(CAM.get_world_rotation())},indent=2))
GI_EVENTS=[];GI_LOG=[];GI_T0=time.perf_counter()
for ix,name in enumerate(GI_CASES):
    t=ix*140
    GI_EVENTS.append((t,'select_'+name,lambda n=name:select_gi(n)))
    for i in range(1,4):
        start=t+30+(i-1)*35
        GI_EVENTS.extend([(start,f'start_{name}_{i}',lambda n=name,i=i:(cmd(f'CsvProfile STARTFILE=gi_{n}_r{i}.csv'),cmd('CsvProfile START'))),(start+30,'stop',lambda:cmd('CsvProfile STOP'))])
    GI_EVENTS.append((t+134,'shot_'+name,lambda n=name:cmd('Shot filename="'+str(OUT/(n+'.png'))+'" -nosuffix')))
def gi_tick(dt):
    try:
        elapsed=time.perf_counter()-GI_T0
        if GI_EVENTS and elapsed>=GI_EVENTS[0][0]:
            _,label,fn=GI_EVENTS.pop(0);fn();GI_LOG.append({'event':label,'time':elapsed});(OUT/'status.json').write_text(json.dumps(GI_LOG,indent=2))
        if not GI_EVENTS:
            unreal.unregister_slate_post_tick_callback(GI_HANDLE);(OUT/'concluido.json').write_text(json.dumps({'done':True}))
    except Exception:
        unreal.unregister_slate_post_tick_callback(GI_HANDLE);cmd('CsvProfile STOP');(OUT/'ERRO.txt').write_text(traceback.format_exc())
GI_HANDLE=unreal.register_slate_post_tick_callback(gi_tick)
