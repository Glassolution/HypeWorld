exec(open(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/RetargetShisui/common.py',encoding='utf-8').read())
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Engine/Maps/Entry')
e=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
floor=e.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,50000));floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));floor.set_actor_scale3d(unreal.Vector(200,200,1))
for yaw,intensity in [(-45,6),(135,5),(45,4)]:
    light=e.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,50500),unreal.Rotator(pitch=-40,yaw=yaw));light.light_component.set_editor_property('intensity',intensity);light.light_component.set_editor_property('cast_shadows',False)
perf=unreal.get_default_object(unreal.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings'));perf.set_editor_property('bThrottleCPUWhenNotForeground',False)
print('SCENE_READY')
