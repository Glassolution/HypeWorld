exec(open(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/RetargetShisui/common.py',encoding='utf-8').read())
import traceback
LABEL=globals().pop('MEASURE_LABEL','before')
les=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);assert not les.is_in_play_in_editor()
MS={'frame':0,'init':False}
def measure_tick(dt):
    try:
        w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if not w:return
        p=unreal.GameplayStatics.get_player_pawn(w,0)
        if not p:return
        mesh=p.get_editor_property('mesh');body=next(x for x in p.get_components_by_class(unreal.SkeletalMeshComponent) if x.get_editor_property('skeletal_mesh_asset') and x.get_editor_property('skeletal_mesh_asset').get_name()=='SK_Shisui')
        if not MS['init']:
            MS['init']=True;p.do_move(0.,0.);p.get_editor_property('character_movement').stop_movement_immediately();p.set_actor_location(unreal.Vector(0,0,50146),False,True);p.set_actor_rotation(unreal.Rotator(),True)
            mesh.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE);mesh.play_animation(unreal.load_asset(IDLE),True);mesh.set_play_rate(0.);mesh.set_position(.5,False)
            MS['display']=unreal.NinjaEditorTools.spawn_local_actor(w,unreal.SkeletalMeshActor,unreal.Vector(0,-190,50050),unreal.Rotator(yaw=-90))
            dc=MS['display'].skeletal_mesh_component;dc.set_skeletal_mesh_asset(unreal.load_asset(SPATH));dc.set_leader_pose_component(mesh)
            MS['cam']=unreal.NinjaEditorTools.spawn_local_actor(w,unreal.SceneCapture2D,unreal.Vector(),unreal.Rotator())
            cc=MS['cam'].capture_component2d;MS['cc']=cc;MS['rt']=unreal.RenderingLibrary.create_render_target2d(w,1280,800,unreal.TextureRenderTargetFormat.RTF_RGBA8)
            cc.set_editor_property('texture_target',MS['rt']);cc.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR);cc.set_editor_property('capture_every_frame',False);cc.set_editor_property('capture_on_movement',False);cc.set_editor_property('fov_angle',46.)
            s=cc.get_editor_property('post_process_settings')
            for k,v in [('override_auto_exposure_method',True),('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL),('override_auto_exposure_bias',True),('auto_exposure_bias',1.5)]:s.set_editor_property(k,v)
            cc.set_editor_property('post_process_settings',s);cc.set_editor_property('post_process_blend_weight',1.)
        n=MS['frame']
        if n==60:
            report={'clip':IDLE,'time_seconds':.5,'mannequin':metrics(mesh),'shisui':metrics(body),'shisui_location':vec(body.get_editor_property('relative_location')),'shisui_rotation':str(body.get_editor_property('relative_rotation'))}
            (OUT/(LABEL+'.json')).write_text(json.dumps(report,indent=2));print('MEASURED',LABEL,report['mannequin']['pelvis_cm'],report['shisui']['pelvis_cm'])
        views={65:('front',unreal.Vector(540,-95,50220)),80:('side',unreal.Vector(-95,570,50220)),95:('back',unreal.Vector(-540,-95,50220))}
        if n in views:
            name,eye=views[n];target=unreal.Vector(0,-95,50145)
            MS['display'].set_actor_location(unreal.Vector(0,-190,50050),False,True)
            if name=='side':
                target=unreal.Vector(-95,0,50145);MS['display'].set_actor_location(unreal.Vector(-190,0,50050),False,True)
            MS['cam'].set_actor_location(eye,False,True);MS['cam'].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,target),False);MS['cc'].capture_scene();MS['view']=name
        if n in [68,83,98]:
            name=LABEL+'_'+MS['view']+'.png'
            if (OUT/name).exists():(OUT/name).unlink()
            unreal.RenderingLibrary.export_render_target(w,MS['rt'],str(OUT),name)
        MS['frame']+=1
        if n>=100:unreal.unregister_slate_post_tick_callback(MEASURE_HANDLE);les.editor_request_end_play()
    except Exception:
        (OUT/(LABEL+'_error.txt')).write_text(traceback.format_exc());unreal.unregister_slate_post_tick_callback(MEASURE_HANDLE);les.editor_request_end_play()
MEASURE_HANDLE=unreal.register_slate_post_tick_callback(measure_tick)
les.editor_request_begin_play()
print('MEASURE_STARTED',LABEL)
