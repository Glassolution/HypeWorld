BODY=next(c for c in P.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=='Indra')
FX=[c for c in P.get_components_by_class(unreal.MeshComponent) if any(m and '.ChakraFeetFx.' in m.get_path_name() for m in c.get_materials())]
FX += [c for c in P.get_components_by_class(unreal.PointLightComponent) if c.get_name() in ['PointLightComponent_1','PointLightComponent_2']]
FX_ORIGINAL={c:c.get_editor_property('hidden_in_game') for c in FX}
def choose(case):
    name,weight,quality,fx,toon,hide=case
    cmd('r.AntiAliasingMethod 2');cmd('r.TemporalAACurrentFrameWeight '+str(weight));cmd('r.TemporalAA.Quality '+str(quality))
    for c in FX:c.set_hidden_in_game(FX_ORIGINAL[c] if fx else True)
    PP.add_or_update_blendable(TOON,1.0 if toon else 0.0)
    for slot in [2,3]:BODY.show_material_section(slot,slot,slot not in hide,0)
    (OUT/(name+'_estado.json')).write_text(json.dumps({'cvars':state(),'case':case,'fx_components':[c.get_name() for c in FX]},indent=2))
DIAG=[('original',.04,1,True,True,[]),('sem_chakra',.04,1,False,True,[]),('sem_toon',.04,1,True,False,[]),('sem_casaco',.04,1,True,True,[3]),('sem_tunica_slots23',.04,1,True,True,[2,3])]
FRAMES=OUT.parents[3]/'Saved/VisualLab/AA1bFrames';FRAMES.mkdir(exist_ok=True,parents=True)
def record(cases,count=90):
    global VC,V,VH,COUNT
    VC=cases;COUNT=count;V={'case':0,'frame':-60,'done':False};cmd('t.OverrideFPS 30');choose(VC[0]);pose(0)
    for case in VC:(FRAMES/case[0]).mkdir(exist_ok=True)
    VH=unreal.register_slate_post_tick_callback(vtick)
def vtick(dt):
    try:
        n=V['frame'];name=VC[V['case']][0]
        if n<0:pose(0)
        elif n<COUNT:
            pose(n/30);cmd('Shot filename="'+str(FRAMES/name/f'{n:04d}.png')+'" -nosuffix')
        elif n==COUNT+1:
            (FRAMES/name/'COMPLETE.json').write_text(json.dumps({'frames':COUNT}))
            V['case']+=1
            if V['case']==len(VC):
                unreal.unregister_slate_post_tick_callback(VH);cmd('t.OverrideFPS 0');choose(DIAG[0]);pose(0);V['done']=True
                (OUT/'video_concluido.json').write_text(json.dumps({'cases':[x[0] for x in VC],'frames':COUNT}));return
            choose(VC[V['case']]);V['frame']=-61
        V['frame']+=1
        if V['frame']%30==0:(OUT/'video_status.json').write_text(json.dumps(V))
    except Exception:
        unreal.unregister_slate_post_tick_callback(VH);cmd('t.OverrideFPS 0');(OUT/'VIDEO_ERRO.txt').write_text(traceback.format_exc())
record(DIAG)
