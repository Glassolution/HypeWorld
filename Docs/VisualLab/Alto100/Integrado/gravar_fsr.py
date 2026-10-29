assert not BENCH_EVENTS
FRAMES=OUT.parents[3]/'Saved/VisualLab/IntegradoFrames';FRAMES.mkdir(exist_ok=True,parents=True)
VIDEO_CASES=['tsr_q1','fsr_native'];V={'case':0,'frame':-90,'done':False};VLOG=[]
START_TRANSFORM=P.get_actor_transform()
MOVE=P.get_component_by_class(unreal.CharacterMovementComponent)
def reset_video():
    P.do_move(0.,0.);P.do_sprint_end();MOVE.stop_movement_immediately()
    P.set_actor_location(unreal.Vector(0,500,START_TRANSFORM.translation.z),False,True)
    P.set_actor_rotation(unreal.Rotator(0,90,0),True)
    PC.set_control_rotation(unreal.Rotator(0,90,0));PC.reset_ignore_move_input();pose(0)
def begin_video():
    cmd('t.OverrideFPS 30');choose_fsr(VIDEO_CASES[V['case']]);reset_video()
    (FRAMES/VIDEO_CASES[V['case']]).mkdir(exist_ok=True)
def video_tick(dt):
    try:
        n=V['frame'];name=VIDEO_CASES[V['case']]
        if n==60:P.do_sprint_start()
        if 60<=n<240:
            a=(n-60)/30*2.0
            PC.set_control_rotation(unreal.Rotator(0,90,0))
            P.do_move(-math.cos(a),math.sin(a))
        else:P.do_move(0.,0.)
        pose(0)
        if 0<=n<240:
            cmd('Shot filename="'+str(FRAMES/name/f'{n:04d}.png')+'" -nosuffix')
            VLOG.append({'case':name,'frame':n,'position':str(P.get_actor_location()),'speed':P.get_velocity().length()})
        elif n==241:
            (FRAMES/name/'COMPLETE.json').write_text(json.dumps({'frames':240}))
            V['case']+=1
            if V['case']==len(VIDEO_CASES):
                unreal.unregister_slate_post_tick_callback(VHANDLE);cmd('t.OverrideFPS 0');P.do_move(0.,0.);P.do_sprint_end();MOVE.stop_movement_immediately();P.set_actor_transform(START_TRANSFORM,False,True);PC.set_ignore_move_input(True);pose(0)
                V['done']=True;(OUT/'video_concluido.json').write_text(json.dumps(V));(OUT/'video_telemetria.json').write_text(json.dumps(VLOG,indent=2));return
            begin_video();V['frame']=-91
        V['frame']+=1
        if V['frame']%30==0:(OUT/'video_status.json').write_text(json.dumps(V))
    except Exception:
        unreal.unregister_slate_post_tick_callback(VHANDLE);cmd('t.OverrideFPS 0');P.do_move(0.,0.);(OUT/'VIDEO_ERRO.txt').write_text(traceback.format_exc())
begin_video();VHANDLE=unreal.register_slate_post_tick_callback(video_tick)
