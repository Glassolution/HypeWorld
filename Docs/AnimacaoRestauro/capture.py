import unreal,math,json,traceback
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/AnimacaoRestauro')
FRAMES=OUT/'frames';FRAMES.mkdir(exist_ok=True)
for marker in ['capture_done.json','capture_error.txt','progress.json']:
    if (OUT/marker).exists():(OUT/marker).unlink()
les=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not les.is_in_play_in_editor()
assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_name()=='Entry'
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in eas.get_all_level_actors():
    if actor.actor_has_tag('ANIM_RESTORE_TEST'):eas.destroy_actor(actor)
floor=eas.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,50000))
floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
floor.set_actor_scale3d(unreal.Vector(1000,1000,1))
floor.set_editor_property('tags',['ANIM_RESTORE_TEST'])
for yaw,intensity in [(-45,4),(135,3),(45,2)]:
    a=eas.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,50500),unreal.Rotator(pitch=-40,yaw=yaw))
    a.light_component.set_editor_property('intensity',intensity)
    a.set_editor_property('tags',['ANIM_RESTORE_TEST'])
cap=eas.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(0,0,50300))
cap.set_editor_property('tags',['ANIM_RESTORE_TEST'])
perf=unreal.get_default_object(unreal.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings'))
perf.set_editor_property('bThrottleCPUWhenNotForeground',False)
# Every captured frame follows one game tick, with a fixed game delta. No input keys or desktop capture.
CASES=[('parado',0,0,False,72),('guarda_frente',0,0,False,72),('guarda_lado',0,0,False,48),('guarda_costas',0,0,False,48),('trote_frente',0,1,False,72),('trote_direita',1,0,False,72),('trote_esquerda',-1,0,False,72),('trote_tras',0,-1,False,72),('corrida_ninja',0,1,True,96)]
S={'case':-1,'frame':-1,'global':0,'rows':[],'wait':0,'initialized':False}
def cmd(w,s):unreal.SystemLibrary.execute_console_command(w,s)
def tick(dt):
    try:
        w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if not w:return
        p=unreal.GameplayStatics.get_player_pawn(w,0)
        if not p:return
        pc=unreal.GameplayStatics.get_player_controller(w,0);m=p.get_editor_property('character_movement');mesh=p.get_editor_property('mesh')
        if not S['initialized']:
            S['initialized']=True;S['p']=p
            assert mesh.is_visible(), 'Base mannequin must be visible in the saved Blueprint'
            cmd(w,'t.OverrideFPS 24');cmd(w,'t.MaxFPS 24');cmd(w,'t.IdleWhenNotForeground 0')
            S['cap']=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.SceneCapture2D)[0]
            cc=S['cap'].capture_component2d
            S['rt']=unreal.RenderingLibrary.create_render_target2d(w,960,720,unreal.TextureRenderTargetFormat.RTF_RGBA8)
            cc.set_editor_property('texture_target',S['rt']);cc.set_editor_property('capture_every_frame',False);cc.set_editor_property('capture_on_movement',False)
            cc.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR);cc.set_editor_property('fov_angle',45.)
            settings=cc.get_editor_property('post_process_settings')
            for k,v in [('override_auto_exposure_method',True),('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL),('override_auto_exposure_bias',True),('auto_exposure_bias',1.),('override_bloom_intensity',True),('bloom_intensity',0.)]:settings.set_editor_property(k,v)
            cc.set_editor_property('post_process_settings',settings);cc.set_editor_property('post_process_blend_weight',1.)
            S['cc']=cc
        if S['case']==-1 or S['frame']>=CASES[S['case']][4]:
            S['case']+=1
            if S['case']>=len(CASES):
                (OUT/'telemetry.json').write_text(json.dumps(S['rows'],indent=2));(OUT/'capture_done.json').write_text(json.dumps({'frames':S['global'],'fps':24,'cases':CASES}))
                cmd(w,'t.OverrideFPS 0');cmd(w,'t.MaxFPS 0');unreal.unregister_slate_post_tick_callback(HANDLE);les.editor_request_end_play();print('CAPTURE_DONE');return
            name,right,forward,sprint,length=CASES[S['case']]
            p.do_move(0.,0.);p.do_sprint_end();m.stop_movement_immediately()
            p.set_actor_location(unreal.Vector(0,0,50142.15),False,True);p.set_actor_rotation(unreal.Rotator(),True);pc.set_control_rotation(unreal.Rotator())
            want_battle=name!='parado'
            if p.is_in_battle_mode()!=want_battle:p.do_focus_start();p.do_focus_end()
            if sprint:p.do_sprint_start()
            S['frame']=-48
        name,right,forward,sprint,length=CASES[S['case']]
        p.do_move(float(right),float(forward))
        if S['frame']>=0:
            offset=unreal.Vector(-350,0,115)
            if name in ['parado','guarda_frente']:offset=unreal.Vector(350,0,70)
            elif name=='guarda_lado':offset=unreal.Vector(0,350,70)
            target=p.get_actor_location()+unreal.Vector(0,0,-5);eye=target+offset
            S['cap'].set_actor_location(eye,False,True);S['cap'].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,target),False)
            S['cc'].capture_scene()
            output=FRAMES/f'{S["global"]:04d}.png'
            if output.exists():output.unlink()
            unreal.RenderingLibrary.export_render_target(w,S['rt'],str(FRAMES),f'{S["global"]:04d}.png')
            anim=mesh.get_anim_instance()
            S['rows'].append({'case':name,'frame':S['global'],'speed':p.get_velocity().length(),'direction':anim.get_editor_property('Direction'),'actor_yaw':p.get_actor_rotation().yaw,'battle':p.is_in_battle_mode(),'visible_meshes':[c.get_editor_property('skeletal_mesh_asset').get_name() for c in p.get_components_by_class(unreal.SkeletalMeshComponent) if c.is_visible() and c.get_editor_property('skeletal_mesh_asset')],'hand_scale':str(mesh.get_socket_transform('hand_l',unreal.RelativeTransformSpace.RTS_COMPONENT).scale3d)})
            S['global']+=1
        S['frame']+=1
        if S['global']%24==0:(OUT/'progress.json').write_text(json.dumps({k:S[k] for k in ['case','frame','global']}))
    except Exception:
        (OUT/'capture_error.txt').write_text(traceback.format_exc());unreal.unregister_slate_post_tick_callback(HANDLE);les.editor_request_end_play()
HANDLE=unreal.register_slate_post_tick_callback(tick)
les.editor_request_begin_play()
print('CAPTURE_STARTED')
