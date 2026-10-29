exec(open(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/RetargetShisui/common.py',encoding='utf-8').read())
import traceback
les=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);assert not les.is_in_play_in_editor()
FR=OUT/'video_frames';FR.mkdir(exist_ok=True)
VS={'case':-1,'frame':0,'global':0,'rows':[],'initialized':False}
CASES=[('guarda',False,False,48),('trote',True,False,48),('ninja',True,True,48),('tigre',False,False,24)]
def vtick(dt):
    try:
        w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if not w:return
        p=unreal.GameplayStatics.get_player_pawn(w,0)
        if not p:return
        source=p.get_editor_property('mesh');target=next(x for x in p.get_components_by_class(unreal.SkeletalMeshComponent) if x.get_editor_property('skeletal_mesh_asset') and x.get_editor_property('skeletal_mesh_asset').get_name()=='SK_Shisui')
        if not VS['initialized']:
            VS['initialized']=True;unreal.SystemLibrary.execute_console_command(w,'t.OverrideFPS 16');unreal.SystemLibrary.execute_console_command(w,'t.MaxFPS 16')
            VS['display']=unreal.NinjaEditorTools.spawn_local_actor(w,unreal.SkeletalMeshActor,unreal.Vector(),unreal.Rotator(yaw=-90))
            dc=VS['display'].skeletal_mesh_component;dc.set_skeletal_mesh_asset(unreal.load_asset(SPATH));dc.set_leader_pose_component(source)
            VS['cam']=unreal.NinjaEditorTools.spawn_local_actor(w,unreal.SceneCapture2D,unreal.Vector(),unreal.Rotator());cc=VS['cam'].capture_component2d;VS['cc']=cc
            VS['rt']=unreal.RenderingLibrary.create_render_target2d(w,960,600,unreal.TextureRenderTargetFormat.RTF_RGBA8);cc.set_editor_property('texture_target',VS['rt']);cc.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR);cc.set_editor_property('capture_every_frame',False);cc.set_editor_property('capture_on_movement',False);cc.set_editor_property('fov_angle',50.)
            s=cc.get_editor_property('post_process_settings')
            for k,v in [('override_auto_exposure_method',True),('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL),('override_auto_exposure_bias',True),('auto_exposure_bias',1.5)]:s.set_editor_property(k,v)
            cc.set_editor_property('post_process_settings',s);cc.set_editor_property('post_process_blend_weight',1.)
        if VS['case']==-1 or VS['frame']>=CASES[VS['case']][3]:
            VS['case']+=1;VS['frame']=-25
            if VS['case']>=len(CASES):
                (OUT/'video_done.json').write_text(json.dumps({'frames':VS['global'],'fps':16,'rows':VS['rows']},indent=2));unreal.SystemLibrary.execute_console_command(w,'t.OverrideFPS 0');unreal.SystemLibrary.execute_console_command(w,'t.MaxFPS 0');unreal.unregister_slate_post_tick_callback(VIDEO_HANDLE);les.editor_request_end_play();return
            p.do_move(0.,0.);p.do_sprint_end();p.get_editor_property('character_movement').stop_movement_immediately();p.set_actor_location(unreal.Vector(0,0,50146),False,True);p.set_actor_rotation(unreal.Rotator(),True)
            unreal.GameplayStatics.get_player_controller(w,0).set_control_rotation(unreal.Rotator())
            if not p.is_in_battle_mode():p.do_focus_start();p.do_focus_end()
            if CASES[VS['case']][2]:p.do_sprint_start()
        name,moving,sprint,length=CASES[VS['case']];n=VS['frame']
        p.do_move(0.,1. if moving else 0.)
        if name=='tigre' and n==-15:source.get_anim_instance().play_slot_animation_as_dynamic_montage(unreal.load_asset('/Game/HypeWorld/Jutsus/Katon/Animacoes/A_Selo_Tigre'),'DefaultSlot',.1,.1,1.,8)
        loc=p.get_actor_location();VS['display'].set_actor_location(loc+unreal.Vector(0,-190,-96),False,True)
        center=loc+unreal.Vector(0,-95,-5);eye=center+unreal.Vector(-520,0,120) if moving else center+unreal.Vector(520,0,100)
        VS['cam'].set_actor_location(eye,False,True);VS['cam'].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,center),False)
        if n>=0:
            VS['cc'].capture_scene()
            if moving:
                filename=f'{VS["global"]:04d}.png'
                if (FR/filename).exists():(FR/filename).unlink()
                unreal.RenderingLibrary.export_render_target(w,VS['rt'],str(FR),filename);VS['global']+=1
            if n==20:
                filename=name+'_comparison.png'
                if (OUT/filename).exists():(OUT/filename).unlink()
                unreal.RenderingLibrary.export_render_target(w,VS['rt'],str(OUT),filename)
                VS['rows'].append({'case':name,'source':metrics(source),'target':metrics(target),'speed':p.get_velocity().length()})
        VS['frame']+=1
    except Exception:
        (OUT/'video_error.txt').write_text(traceback.format_exc());unreal.unregister_slate_post_tick_callback(VIDEO_HANDLE);les.editor_request_end_play()
VIDEO_HANDLE=unreal.register_slate_post_tick_callback(vtick);les.editor_request_begin_play();print('VIDEO_TEST_STARTED')
