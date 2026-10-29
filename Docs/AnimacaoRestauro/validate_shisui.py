import unreal,json,traceback
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/AnimacaoRestauro/CorrecaoShisui')
if (OUT/'validation_error.txt').exists():(OUT/'validation_error.txt').unlink()
les=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not les.is_in_play_in_editor()
CHECK={'frame':0,'rows':[],'initialized':False}
def validate_tick(dt):
    try:
        w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if not w:return
        p=unreal.GameplayStatics.get_player_pawn(w,0)
        if not p:return
        n=CHECK['frame'];mesh=p.get_editor_property('mesh');anim=mesh.get_anim_instance()
        if not CHECK['initialized']:
            CHECK['initialized']=True
            CHECK['transform']=p.get_actor_transform()
            p.do_sprint_end()
            if not p.is_in_battle_mode():p.do_focus_start();p.do_focus_end()
            unreal.SystemLibrary.execute_console_command(w,'t.OverrideFPS 30')
            CHECK['camera']=unreal.NinjaEditorTools.spawn_local_actor(w,unreal.SceneCapture2D,unreal.Vector(),unreal.Rotator())
            cc=CHECK['camera'].capture_component2d
            CHECK['rt']=unreal.RenderingLibrary.create_render_target2d(w,960,720,unreal.TextureRenderTargetFormat.RTF_RGBA8)
            cc.set_editor_property('texture_target',CHECK['rt']);cc.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
            cc.set_editor_property('capture_every_frame',False);cc.set_editor_property('capture_on_movement',False);cc.set_editor_property('fov_angle',50.)
            settings=cc.get_editor_property('post_process_settings')
            for key,value in [('override_auto_exposure_method',True),('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL),('override_auto_exposure_bias',True),('auto_exposure_bias',1.4)]:settings.set_editor_property(key,value)
            cc.set_editor_property('post_process_settings',settings);cc.set_editor_property('post_process_blend_weight',1.)
            CHECK['cc']=cc
        moving=90<=n<180
        p.do_move(0.,1. if moving else 0.)
        style=p.get_fighting_style();battle_pose=style.get_editor_property('battle_idle') if style else anim.get_editor_property('battle_idle_pose')
        bodies=[c for c in p.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_editor_property('skeletal_mesh_asset') and c.get_editor_property('skeletal_mesh_asset').get_name()=='SK_Shisui']
        assert bodies and bodies[0].is_visible(),'Shisui is not visible'
        assert not mesh.is_visible(),'Mannequin is visible'
        assert anim.get_editor_property('combat_ready_pose') is None,'Static readiness override is active'
        assert battle_pose,'Animated battle pose is missing'
        if n in [60,150]:
            target=p.get_actor_location()+unreal.Vector(0,0,0)
            forward=p.get_actor_forward_vector();side=p.get_actor_right_vector()
            eye=target+forward*330+side*150+unreal.Vector(0,0,90)
            CHECK['camera'].set_actor_location(eye,False,True);CHECK['camera'].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,target),False)
            CHECK['cc'].capture_scene()
            shot='trote.png' if moving else 'guarda.png'
            if (OUT/shot).exists():(OUT/shot).unlink()
            unreal.RenderingLibrary.export_render_target(w,CHECK['rt'],str(OUT),shot)
        if n in [65,155]:
            velocity=p.get_velocity()
            CHECK['rows'].append({'case':'combat_trot' if moving else 'battle_idle','battle':p.is_in_battle_mode(),'battle_pose':battle_pose.get_path_name(),'speed':(velocity.x**2+velocity.y**2)**.5,'visible_body':bodies[0].get_editor_property('skeletal_mesh_asset').get_path_name(),'mannequin_visible':mesh.is_visible(),'static_ready_override':None})
        CHECK['frame']+=1
        if n>=185:
            assert len(CHECK['rows'])==2 and all(r['battle'] for r in CHECK['rows'])
            # The Tutorial route has slopes and obstacles; this check exercises the running pose, not flat-ground speed.
            assert CHECK['rows'][0]['speed']<1 and CHECK['rows'][1]['speed']>200
            (OUT/'validation.json').write_text(json.dumps({'passed':True,'checks':CHECK['rows']},indent=2))
            p.do_move(0.,0.);unreal.SystemLibrary.execute_console_command(w,'t.OverrideFPS 0')
            unreal.unregister_slate_post_tick_callback(SHISUI_VALIDATION_HANDLE);les.editor_request_end_play()
    except Exception:
        (OUT/'validation_error.txt').write_text(traceback.format_exc());unreal.unregister_slate_post_tick_callback(SHISUI_VALIDATION_HANDLE);les.editor_request_end_play()
SHISUI_VALIDATION_HANDLE=unreal.register_slate_post_tick_callback(validate_tick)
les.editor_request_begin_play()
print('SHISUI_VALIDATION_STARTED')
