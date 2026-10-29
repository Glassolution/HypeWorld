import unreal,json
OUT=r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/CameraZoom'
def props(o,names):
    d={}
    for n in names:
        try:d[n]=str(o.get_editor_property(n))
        except Exception as e:d[n]=str(e)
    return d
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
p=unreal.GameplayStatics.get_player_pawn(w,0) if w else unreal.get_default_object(unreal.load_asset('/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter').generated_class())
arm=p.get_component_by_class(unreal.SpringArmComponent);cam=p.get_component_by_class(unreal.CameraComponent)
r={'arm':props(arm,['target_arm_length','socket_offset','target_offset','relative_rotation','use_pawn_control_rotation','enable_camera_lag','camera_lag_speed','enable_camera_rotation_lag','do_collision_test','probe_size']),'camera':props(cam,['field_of_view','relative_rotation']),'character':props(p,['zoom_action','mouse_look_action','look_action','camera_zoom_min','camera_zoom_max','camera_zoom_step','camera_zoom_speed'])}
r['contexts']={}
for path in ['/Game/Input/IMC_Default','/Game/Input/IMC_MouseLook']:
    ctx=unreal.load_asset(path);r['contexts'][path]=[]
    for m in ctx.get_editor_property('default_key_mappings').get_editor_property('mappings'):
        r['contexts'][path].append({'key':str(m.get_editor_property('key')),'action':str(m.get_editor_property('action')),'modifiers':str(m.get_editor_property('modifiers'))})
json.dump(r,open(OUT+'/before.json','w'),indent=2);print(json.dumps(r,indent=2))


