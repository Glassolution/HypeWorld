# Explicit Rotator keywords avoid the Python struct's positional roll/pitch/yaw order.
VIDEO['active']=False;EVENTS.clear();cmd('t.OverrideFPS 0');P.do_move(0.,0.);P.do_sprint_end();MOVE.stop_movement_immediately()
CAPTURE_CAMERA=unreal.GameplayStatics.get_all_actors_of_class(W,unreal.CameraActor)[0]
CAM=CAPTURE_CAMERA.get_component_by_class(unreal.CameraComponent)
PC.set_view_target_with_blend(CAPTURE_CAMERA,0.)
P.set_actor_transform(START_TR,False,True)
PC.set_control_rotation(unreal.Rotator(pitch=0,yaw=90,roll=0))
def pose(t):
    CAM.set_world_location(unreal.Vector(0,-312.028006,250.352636),False,True)
    CAM.set_world_rotation(unreal.Rotator(pitch=-15,yaw=90,roll=0),False,True)
    CAM.set_field_of_view(90)
pose(0)
source=(OUT/'executar.py').read_text()
part=source.split('def reset_clip():',1)[1].split('def tick(dt):',1)[0]
part='def reset_clip():'+part
part=part.replace('unreal.Rotator(0,90,0)','unreal.Rotator(pitch=0,yaw=90,roll=0)').replace('unreal.Rotator(-15,90,0)','unreal.Rotator(pitch=-15,yaw=90,roll=0)').replace('unreal.Rotator(-26,90+45*math.sin(t*4),0)','unreal.Rotator(pitch=-26,yaw=90+45*math.sin(t*4),roll=0)')
exec(part)
profile('alto60');pose(0);START=time.perf_counter();J=[]
for i in range(1,4):
    t=20+(i-1)*35
    EVENTS.extend([(t,'start_'+str(i),lambda i=i:(cmd(f'CsvProfile STARTFILE=alto60_final_r{i}.csv'),cmd('CsvProfile START'))),(t+30,'stop',lambda:cmd('CsvProfile STOP'))])
EVENTS.extend([(128,'shot',lambda:cmd('Shot filename="'+str(OUT/'alto60.png')+'" -nosuffix')),(131,'profile',lambda:(cmd('r.ProfileGPU.ShowUI 0'),cmd('ProfileGPU'))),(136,'videos',lambda:start_videos())])
VIDEO.update(done=False)
(OUT/'correcao_captura.json').write_text(json.dumps({'reason':'Correct positional Rotator and use independent CameraActor; preliminary benchmark excluded.','resolution':[1920,1080],'camera':[0,-312.028006,250.352636],'pitch':-15,'yaw':90,'final_csv_prefix':'alto60_final'},indent=2))
