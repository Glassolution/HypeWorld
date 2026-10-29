import unreal,json,os
OUT=r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/CameraZoom'
BP='/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter'
bp=unreal.load_asset(BP);cdo=unreal.get_default_object(bp.generated_class())
settings={'camera_zoom_default':450.,'camera_zoom_min':200.,'camera_zoom_max':800.,'camera_zoom_step':60.,'camera_zoom_speed':8.,'camera_target_offset':unreal.Vector(0,0,60),'camera_socket_offset':unreal.Vector(0,0,40),'camera_initial_pitch':-15.,'camera_pitch_min':-60.,'camera_pitch_max':30.,'camera_fov':80.,'camera_lag_speed':12.,'camera_probe_size':12.,'camera_use_pawn_control_rotation':True,'camera_lag_enabled':True,'camera_rotation_lag_enabled':False,'camera_collision_test':True}
for n,v in settings.items():cdo.set_editor_property(n,v)
arm=cdo.get_component_by_class(unreal.SpringArmComponent);cam=cdo.get_component_by_class(unreal.CameraComponent)
for n,v in {'target_arm_length':450.,'target_offset':unreal.Vector(0,0,60),'socket_offset':unreal.Vector(0,0,40),'use_pawn_control_rotation':True,'enable_camera_lag':True,'camera_lag_speed':12.,'enable_camera_rotation_lag':False,'do_collision_test':True,'probe_size':12.}.items():arm.set_editor_property(n,v)
cam.set_editor_property('field_of_view',80.);cam.set_editor_property('relative_rotation',unreal.Rotator())
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
json.dump({n:str(v) for n,v in settings.items()},open(OUT+'/settings.json','w'),indent=2)
open(OUT+'/setup_done.txt','w').write('saved')
print('CAMERA_SETUP_SAVED')
