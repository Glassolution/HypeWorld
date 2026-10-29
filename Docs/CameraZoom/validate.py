import unreal,time,json,traceback
OUT=r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/CameraZoom'
W=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
P=unreal.GameplayStatics.get_player_pawn(W,0);PC=unreal.GameplayStatics.get_player_controller(W,0)
ARM=P.get_component_by_class(unreal.SpringArmComponent);CAM=P.get_component_by_class(unreal.CameraComponent)
INPUT=next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if not s.get_name().startswith('Default__'))
ACTION=P.get_editor_property('zoom_action')
def scroll(amount):
    value=unreal.InputActionValue(amount,0.,0.,unreal.InputActionValueType.AXIS1D)
    INPUT.inject_input_vector_for_action(ACTION,unreal.Vector(amount,0,0),[],[])
def vec(v):return [v.x,v.y,v.z]
def state():
    rot=PC.get_control_rotation()
    relative=CAM.get_editor_property('relative_rotation')
    return {'arm':ARM.get_editor_property('target_arm_length'),'target_offset':vec(ARM.get_editor_property('target_offset')),'socket_offset':vec(ARM.get_editor_property('socket_offset')),'control_rotation':[rot.pitch,rot.yaw,rot.roll],'camera_relative_rotation':[relative.pitch,relative.yaw,relative.roll],'fov':CAM.field_of_view,'position':vec(P.get_actor_location()),'collision_fixed':ARM.is_collision_fix_applied()}
INITIAL=state();DATA={'initial':INITIAL,'frames':[],'cases':{}}
assert abs(INITIAL['arm']-450)<.1,INITIAL
assert abs(((PC.get_control_rotation().pitch+180)%360-180)+15)<.1,INITIAL
assert abs(CAM.field_of_view-80)<.1
CASES=[('padrao',450,0),('passo',390,1),('minimo',200,100),('maximo',800,-100)]
S={'i':0,'phase':'setup','t':0,'previous':None}
def cmd(c):unreal.SystemLibrary.execute_console_command(W,c)
def tick(dt):
    try:
        now=time.monotonic();name,target,sign=CASES[S['i']]
        st=state()
        assert st['target_offset']==INITIAL['target_offset'] and st['socket_offset']==INITIAL['socket_offset'],st
        assert st['camera_relative_rotation']==INITIAL['camera_relative_rotation'],st
        assert all(abs(a-b)<.01 for a,b in zip(st['control_rotation'],INITIAL['control_rotation'])),st
        assert all(abs(a-b)<1 for a,b in zip(st['position'],INITIAL['position'])),st
        if S['phase']=='setup':
            if sign:scroll(float(sign))
            S['previous']=st['arm'];S['phase']='wait';S['t']=now
        elif S['phase']=='wait':
            DATA['frames'].append({'case':name,'time':now-S['t'],'arm':st['arm']})
            if sign>0:assert st['arm']<=S['previous']+.001
            if sign<0:assert st['arm']>=S['previous']-.001
            S['previous']=st['arm']
            if now-S['t']>2.5:
                assert abs(st['arm']-target)<.1,(name,st)
                DATA['cases'][name]=st
                if name!='passo':cmd('Shot filename="'+OUT+'/'+name+'.png" -nosuffix')
                S['phase']='flush';S['t']=now
        elif S['phase']=='flush' and now-S['t']>1:
            S['i']+=1
            if S['i']==len(CASES):
                scroll((800-450)/60.)
                json.dump(DATA,open(OUT+'/validation.json','w'),indent=2);unreal.unregister_slate_post_tick_callback(H);open(OUT+'/validation_done.txt','w').write('pass');return
            S['phase']='setup'
    except Exception:
        unreal.unregister_slate_post_tick_callback(H);open(OUT+'/validation_error.txt','w').write(traceback.format_exc())
H=unreal.register_slate_post_tick_callback(tick)



