import unreal,json,time,math,traceback
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Alto100/AA')
W=next(w for w in unreal.ObjectIterator(unreal.World) if unreal.GameplayStatics.get_player_pawn(w,0))
P=unreal.GameplayStatics.get_player_pawn(W,0);PC=unreal.GameplayStatics.get_player_controller(W,0)
def cmd(s):unreal.SystemLibrary.execute_console_command(W,s,PC)
GROUPS=['ViewDistance','AntiAliasing','Shadow','GlobalIllumination','Reflection','PostProcess','Texture','Effects','Foliage','Shading','Landscape']
NAMES=['sg.ResolutionQuality']+['sg.'+g+'Quality' for g in GROUPS]+['r.ScreenPercentage','r.AntiAliasingMethod','r.TSR.ShadingRejection.Flickering','r.TSR.History.ScreenPercentage','r.TSR.RejectionAntiAliasingQuality','r.TSR.History.UpdateQuality','r.TemporalAA.Quality','r.TemporalAA.Upsampling','grass.DensityScale','foliage.DensityScale','r.Shadow.CSM.MaxCascades','r.Shadow.MaxCSMResolution','r.Lumen.FinalGatherMethod','r.VSync','t.MaxFPS','t.OverrideFPS']
def state():return {n:unreal.SystemLibrary.get_console_variable_string_value(n) for n in NAMES}
cmd('Scalability 2')
for g in GROUPS:cmd('sg.'+g+'Quality 2')
for c in ['sg.ResolutionQuality 100','r.ScreenPercentage 100','r.DynamicRes.OperationMode 0','r.VSync 0','t.MaxFPS 0','t.OverrideFPS 0','t.IdleWhenNotForeground 0']:cmd(c)
PC.set_ignore_move_input(True);PC.set_ignore_look_input(True)
PC.set_control_rotation(unreal.Rotator(pitch=4,yaw=90,roll=0))
CAM=P.get_component_by_class(unreal.CameraComponent)
CAM.set_absolute(True,True,False)
CAM.set_world_location(unreal.Vector(0,-312.028006,250.352636),False,True)
CAM.set_world_rotation(unreal.Rotator(pitch=-15,yaw=90,roll=0),False,True)
CAM.set_field_of_view(90)
# Camera-only closed route: 10 s, 2 m lateral, 2 m forward. Pawn and wind unchanged.
def pose(t):
    a=2*math.pi*(t%10)/10
    CAM.set_world_location(unreal.Vector(200*math.sin(a),-312.028006+100*(1-math.cos(a)),250.352636),False,True)
    CAM.set_world_rotation(unreal.Rotator(pitch=-15,yaw=90,roll=0),False,True)
CASES=[('tsr_atual',2,4,1),('tsr_flickering0',2,4,0),('tsr_q1',1,4,0),('taa',2,2,1)]
def select(case):
    name,q,method,flicker=case
    cmd(f'sg.AntiAliasingQuality {q}');cmd(f'r.AntiAliasingMethod {method}');cmd(f'r.TSR.ShadingRejection.Flickering {flicker}')
    cmd('r.ScreenPercentage 100');cmd('r.TSR.History.ScreenPercentage 100')
    (OUT/(name+'_estado.json')).write_text(json.dumps(state(),indent=2))
EVENTS=[];JOURNAL=[];ACTIVE=None;ROUTE0=0;T0=time.perf_counter()
def begin(name,i):
    global ROUTE0,ACTIVE
    ROUTE0=time.perf_counter();ACTIVE=name
    cmd(f'CsvProfile STARTFILE=aa_{name}_r{i}.csv');cmd('CsvProfile START')
def end():
    global ACTIVE
    cmd('CsvProfile STOP');ACTIVE=None;pose(0)
for index,case in enumerate(CASES):
    base=index*130
    EVENTS.append((base,'select_'+case[0],lambda case=case:select(case)))
    for i in range(1,4):
        start=base+25+(i-1)*35
        EVENTS.extend([(start,f'start_{case[0]}_{i}',lambda n=case[0],i=i:begin(n,i)),(start+30,f'stop_{case[0]}_{i}',end)])
    EVENTS.append((base+127,'shot_'+case[0],lambda n=case[0]:cmd('Shot filename="'+str(OUT/(n+'.png'))+'" -nosuffix')))
def tick(dt):
    try:
        elapsed=time.perf_counter()-T0
        if EVENTS and elapsed>=EVENTS[0][0]:
            scheduled,label,fn=EVENTS.pop(0);fn();JOURNAL.append({'event':label,'time':elapsed});(OUT/'status.json').write_text(json.dumps(JOURNAL,indent=2))
        if ACTIVE:pose(time.perf_counter()-ROUTE0)
        if not EVENTS:
            unreal.unregister_slate_post_tick_callback(HANDLE);pose(0)
            (OUT/'concluido.json').write_text(json.dumps(state(),indent=2))
    except Exception:
        unreal.unregister_slate_post_tick_callback(HANDLE);cmd('CsvProfile STOP');(OUT/'ERRO.txt').write_text(traceback.format_exc())
HANDLE=unreal.register_slate_post_tick_callback(tick)
print('AA_STARTED',CAM.get_path_name())
