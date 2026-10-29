# Execute only after coletar_aa.py completes. Native Shot frames; no image synthesis.
assert (OUT/'concluido.json').exists() and not EVENTS
FRAMES=OUT.parents[3]/'Saved/VisualLab/AAFrames'
FRAMES.mkdir(parents=True,exist_ok=True)
V={'case':0,'frame':-120,'done':False}
cmd('t.OverrideFPS 30');cmd('t.MaxFPS 0')
select(CASES[0]);pose(0)
for case in CASES:(FRAMES/case[0]).mkdir(exist_ok=True)
def video_tick(dt):
    try:
        i=V['case'];n=V['frame'];name=CASES[i][0]
        if n<0:pose(0)
        elif n<300:
            pose(n/30)
            cmd('Shot filename="'+str(FRAMES/name/f'{n:04d}.png')+'" -nosuffix')
        elif n==300:
            # Allow the last pending screenshot to complete before switching AA.
            pose(0)
        elif n==301:
            (OUT/(name+'_video_estado.json')).write_text(json.dumps(state(),indent=2))
            V['case']+=1
            if V['case']==len(CASES):
                unreal.unregister_slate_post_tick_callback(VHANDLE)
                cmd('t.OverrideFPS 0');select(CASES[0]);pose(0)
                V['done']=True;(OUT/'videos_concluidos.json').write_text(json.dumps(V))
                return
            select(CASES[V['case']]);V['frame']=-121
        V['frame']+=1
        if V['frame']%30==0:(OUT/'video_status.json').write_text(json.dumps(V))
    except Exception:
        unreal.unregister_slate_post_tick_callback(VHANDLE);cmd('t.OverrideFPS 0')
        (OUT/'VIDEO_ERRO.txt').write_text(traceback.format_exc())
VHANDLE=unreal.register_slate_post_tick_callback(video_tick)
print('NATIVE_VIDEO_STARTED',str(FRAMES))
