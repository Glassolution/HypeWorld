# Load only after AA1b is finished. Ablations are temporary; geometry stays visible.
assert not EVENTS and V['done']
SOUT=OUT.parent/'Sombras';SOUT.mkdir(exist_ok=True)
choose(DIAG[0]);cmd('sg.AntiAliasingQuality 1');cmd('r.AntiAliasingMethod 4');cmd('r.TSR.ShadingRejection.Flickering 0');cmd('r.TSR.History.ScreenPercentage 100')
pose(0);cmd('t.OverrideFPS 0')
GROUPS_SH={k:[] for k in ['rochas','arvores','arbustos_flores','personagens','outros','grama']}
ORIG_SH={};DETAIL=[]
for c in SHADOW_COMPONENTS:
    if not c:continue
    mesh=None
    if isinstance(c,unreal.StaticMeshComponent):mesh=c.get_editor_property('static_mesh')
    elif isinstance(c,unreal.SkeletalMeshComponent):mesh=c.get_editor_property('skeletal_mesh_asset')
    path=mesh.get_path_name() if mesh else '';low=mesh.get_name().lower() if mesh else ''
    if isinstance(c,unreal.SkeletalMeshComponent):group='personagens'
    elif 'grass' in low or 'grama' in low:group='grama'
    elif any(s in low for s in ['rock','stone','boulder','cliff']):group='rochas'
    elif any(s in low for s in ['tree','tronco','toco']):group='arvores'
    elif any(s in low for s in ['bush','flower','fern']):group='arbustos_flores'
    else:group='outros'
    casts=bool(c.get_editor_property('cast_shadow'));ORIG_SH[c]=casts
    if casts:GROUPS_SH[group].append(c)
    DETAIL.append({'component':c.get_path_name(),'mesh':path,'group':group,'cast_shadow':casts})
(SOUT/'grupos.json').write_text(json.dumps(DETAIL,indent=2))
def shadow_case(group):
    for c,old in ORIG_SH.items():c.set_cast_shadow(old)
    if group!='baseline':
        for c in GROUPS_SH[group]:c.set_cast_shadow(False)
    (SOUT/(group+'_estado.json')).write_text(json.dumps({'cvars':state(),'disabled':[c.get_path_name() for c in GROUPS_SH.get(group,[])]},indent=2))
SG_CASES=['baseline','rochas','arvores','arbustos_flores','personagens','outros']
SEVENTS=[];SJ=[];ST0=time.perf_counter()
for index,group in enumerate(SG_CASES):
    t=index*45
    SEVENTS.extend([(t,'select_'+group,lambda g=group:shadow_case(g)),(t+10,'start_'+group,lambda g=group:(cmd('CsvProfile STARTFILE=shadowdiag_'+g+'.csv'),cmd('CsvProfile START'))),(t+40,'stop_'+group,lambda:cmd('CsvProfile STOP'))])
def stick(dt):
    try:
        elapsed=time.perf_counter()-ST0
        if SEVENTS and elapsed>=SEVENTS[0][0]:
            _,label,fn=SEVENTS.pop(0);fn();SJ.append({'event':label,'time':elapsed});(SOUT/'status.json').write_text(json.dumps(SJ,indent=2))
        if not SEVENTS:
            unreal.unregister_slate_post_tick_callback(SSH);shadow_case('baseline');(SOUT/'triagem_concluida.json').write_text(json.dumps({k:len(v) for k,v in GROUPS_SH.items()},indent=2))
    except Exception:
        unreal.unregister_slate_post_tick_callback(SSH);cmd('CsvProfile STOP');shadow_case('baseline');(SOUT/'ERRO.txt').write_text(traceback.format_exc())
SSH=unreal.register_slate_post_tick_callback(stick)
