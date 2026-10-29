import unreal,json,time
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Grama/Passo1')
W=next(w for w in unreal.ObjectIterator(unreal.World) if unreal.GameplayStatics.get_player_pawn(w,0))
P=unreal.GameplayStatics.get_player_pawn(W,0); PC=unreal.GameplayStatics.get_player_controller(W,0)
def cmd(s): unreal.SystemLibrary.execute_console_command(W,s,PC)
GROUPS=['ViewDistance','AntiAliasing','Shadow','GlobalIllumination','Reflection','PostProcess','Texture','Effects','Foliage','Shading','Landscape']
NAMES=['sg.'+x+'Quality' for x in GROUPS]+['r.ScreenPercentage','r.AntiAliasingMethod','r.TSR.ShadingRejection.Flickering','grass.DensityScale','foliage.DensityScale','r.VSync','t.MaxFPS','r.DynamicRes.OperationMode','r.Tonemapper.Sharpen','r.Shadow.FilterMethod','r.Shadow.CSM.MaxCascades','r.Shadow.MaxCSMResolution','t.IdleWhenNotForeground']
def vals():return {n:unreal.SystemLibrary.get_console_variable_string_value(n) for n in NAMES}
ORIGINAL=vals()
if not (OUT/'runtime_antes.json').exists(): (OUT/'runtime_antes.json').write_text(json.dumps(ORIGINAL,indent=2))
for x in GROUPS:cmd('sg.'+x+'Quality 1')
for c in ['r.ScreenPercentage 67','r.AntiAliasingMethod 4','r.DynamicRes.OperationMode 0','r.VSync 0','t.MaxFPS 0','t.IdleWhenNotForeground 0','grass.DensityScale 0.4','foliage.DensityScale 0.4']:
    cmd(c)
# Same stationary player and follow camera for every grass and toon capture.
PC.set_ignore_move_input(True); PC.set_ignore_look_input(True)
PC.set_control_rotation(unreal.Rotator(pitch=4,yaw=90,roll=0))
CAM=P.get_component_by_class(unreal.CameraComponent)
def state():
    return {'world':W.get_path_name(),'cvars':vals(),'player_location':str(P.get_actor_location()),'player_rotation':str(P.get_actor_rotation()),'camera_location':str(CAM.get_world_location()),'camera_rotation':str(CAM.get_world_rotation()),'fov':CAM.get_editor_property('field_of_view'),'control':str(PC.get_control_rotation())}
def shot(name):
    (OUT/(name+'.json')).write_text(json.dumps(state(),indent=2))
    cmd('Shot filename="'+str(OUT/(name+'.png')).replace('\\','/')+'" -nosuffix')
def density(v):
    cmd('grass.DensityScale '+str(v))
def profile(name):
    cmd('CsvProfile STARTFILE='+name+'.csv');cmd('CsvProfile START')
    (OUT/(name+'_state.json')).write_text(json.dumps(state(),indent=2))
print(json.dumps(state(),indent=2))
