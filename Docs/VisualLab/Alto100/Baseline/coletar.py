import unreal,json,time,traceback
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Alto100/Baseline')
W=next(w for w in unreal.ObjectIterator(unreal.World) if unreal.GameplayStatics.get_player_pawn(w,0))
P=unreal.GameplayStatics.get_player_pawn(W,0);PC=unreal.GameplayStatics.get_player_controller(W,0)
def cmd(s):unreal.SystemLibrary.execute_console_command(W,s,PC)
GROUPS=['ViewDistance','AntiAliasing','Shadow','GlobalIllumination','Reflection','PostProcess','Texture','Effects','Foliage','Shading','Landscape']
NAMES=['sg.ResolutionQuality']+['sg.'+g+'Quality' for g in GROUPS]+['r.ScreenPercentage','r.ScreenPercentage.Default','r.SecondaryScreenPercentage.GameViewport','r.DynamicRes.OperationMode','r.AntiAliasingMethod','r.TSR.ShadingRejection.Flickering','r.TSR.History.ScreenPercentage','grass.DensityScale','foliage.DensityScale','r.VSync','t.MaxFPS','r.DynamicGlobalIlluminationMethod','r.Lumen.DiffuseIndirect.Allow','r.Lumen.FinalGatherMethod','r.Lumen.HardwareRayTracing','r.ReflectionMethod','r.DistanceFieldAO','r.AOQuality','r.AmbientOcclusionLevels','r.AmbientOcclusion.Method','r.Shadow.Virtual.Enable','r.Shadow.FilterMethod','r.Shadow.CSM.MaxCascades','r.Shadow.MaxCSMResolution','r.DistanceFieldShadowing','r.VelocityOutputPass','r.Velocity.EnableVertexDeformation','r.Tonemapper.Sharpen']
def vals():return {n:unreal.SystemLibrary.get_console_variable_string_value(n) for n in NAMES}
def state():
    cam=P.get_component_by_class(unreal.CameraComponent)
    return {'world':W.get_path_name(),'cvars':vals(),'player_location':str(P.get_actor_location()),'player_rotation':str(P.get_actor_rotation()),'camera_location':str(cam.get_world_location()),'camera_rotation':str(cam.get_world_rotation()),'fov':cam.get_editor_property('field_of_view')}
(OUT/'runtime_antes.json').write_text(json.dumps(vals(),indent=2))
presets={}
for level in [1,2,3,4]:
    cmd('Scalability '+str(level));presets[level]=unreal.SystemLibrary.get_console_variable_float_value('sg.ResolutionQuality')
(OUT/'presets_resolucao.json').write_text(json.dumps(presets,indent=2))
assert all(v==100 for v in presets.values()),presets
cmd('Scalability 2')
for g in GROUPS:cmd('sg.'+g+'Quality 2')
for c in ['sg.ResolutionQuality 100','r.ScreenPercentage 100','r.DynamicRes.OperationMode 0','r.VSync 0','t.MaxFPS 0','t.IdleWhenNotForeground 0'] :cmd(c)
PC.set_ignore_move_input(True);PC.set_ignore_look_input(True)
PC.set_control_rotation(unreal.Rotator(pitch=4,yaw=90,roll=0))
pps={}
for a in unreal.GameplayStatics.get_all_actors_of_class(W,unreal.PostProcessVolume):
    s=a.get_editor_property('settings');e={}
    for n in ['override_dynamic_global_illumination_method','dynamic_global_illumination_method','override_reflection_method','reflection_method','override_ambient_occlusion_intensity','ambient_occlusion_intensity','ambient_occlusion_radius','weighted_blendables']:
        try:e[n]=str(s.get_editor_property(n))
        except Exception as err:e[n]=str(err)
    pps[a.get_name()]=e
(OUT/'postprocess_efetivo.json').write_text(json.dumps(pps,indent=2))
def shot():
    (OUT/'alto100.json').write_text(json.dumps(state(),indent=2))
    cmd('Shot filename="'+str(OUT/'alto100.png').replace('\\','/')+'" -nosuffix')
def start(i):
    (OUT/f'alto100_r{i}_estado.json').write_text(json.dumps(state(),indent=2))
    cmd(f'CsvProfile STARTFILE=alto100_r{i}.csv');cmd('CsvProfile START')
EVENTS=[(40,'screenshot',shot)]
for i in range(1,4):
    t=50+(i-1)*40
    EVENTS.extend([(t,f'start_r{i}',lambda i=i:start(i)),(t+30,f'stop_r{i}',lambda:cmd('CsvProfile STOP'))])
EVENTS.extend([(165,'gpu_detail',lambda:(cmd('r.ProfileGPU.ShowUI 0'),cmd('ProfileGPU')))])
T0=time.perf_counter();JOURNAL=[]
def tick(dt):
    try:
        elapsed=time.perf_counter()-T0
        if EVENTS and elapsed>=EVENTS[0][0]:
            scheduled,label,fn=EVENTS.pop(0);fn();JOURNAL.append({'event':label,'elapsed':elapsed,'scheduled':scheduled})
            (OUT/'status.json').write_text(json.dumps(JOURNAL,indent=2))
        if not EVENTS:
            unreal.unregister_slate_post_tick_callback(HANDLE)
            (OUT/'concluido.json').write_text(json.dumps(state(),indent=2))
    except Exception:
        unreal.unregister_slate_post_tick_callback(HANDLE);cmd('CsvProfile STOP')
        (OUT/'ERRO.txt').write_text(traceback.format_exc())
HANDLE=unreal.register_slate_post_tick_callback(tick)
print('ALTO100_INICIADO',presets)
