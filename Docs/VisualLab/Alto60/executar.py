import unreal,json,time,math,traceback
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Alto60')
ROOT=OUT.parents[2];FRAMES=ROOT/'Saved/VisualLab/Alto60Frames'
source=(OUT.parent/'Alto100/AA/coletar_aa.py').read_text().split('CASES=')[0]
exec(source.replace("Alto100/AA')","Alto60')"))
lab=json.loads((OUT/'lab_criado.json').read_text())
TREES=[];ROCKS=[];PROXIES=[]
for a in unreal.GameplayStatics.get_all_actors_of_class(W,unreal.Actor):
    if a.get_name() in {r['proxy'] for r in lab['proxies']}:
        PROXIES.extend(a.get_components_by_class(unreal.StaticMeshComponent));continue
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        mesh=c.get_editor_property('static_mesh')
        if not mesh:continue
        low=mesh.get_name().lower()
        if any(t in low for t in ['tree','tronco','toco']):TREES.append(c)
        if any(t in low for t in ['rock','stone','boulder','cliff']):ROCKS.append(c)
LIGHT=unreal.GameplayStatics.get_all_actors_of_class(W,unreal.DirectionalLight)[0].get_component_by_class(unreal.DirectionalLightComponent)
NAMES+=['r.DynamicRes.OperationMode','r.FidelityFX.FSR.Enabled','r.FidelityFX.FSR.QualityMode','r.FidelityFX.FI.Enabled','r.Velocity.EnableLandscapeGrass','r.FidelityFX.FSR.ForceVertexDeformationOutputsVelocity','r.VolumetricFog','r.SSR.Quality','r.Water.SingleLayer.Reflection','r.Water.SingleLayer.RefractionDownsampleFactor','r.Shadow.FilterMethod','r.DistanceFieldShadowing']
def profile(name):
    aggressive=name=='alto60'
    cmd('r.FidelityFX.FSR.Enabled 0');cmd('r.FidelityFX.FI.Enabled 0')
    for s in ['sg.ResolutionQuality 100','sg.AntiAliasingQuality 1','r.ScreenPercentage 100','r.DynamicRes.OperationMode 0','r.AntiAliasingMethod 4','r.TSR.History.ScreenPercentage 100','r.Lumen.FinalGatherMethod 0','r.Shadow.CSM.MaxCascades 3','r.Shadow.MaxCSMResolution 1536','r.Shadow.RadiusThreshold 0.06','r.DistanceFieldShadowing 1']:
        cmd(s)
    for c in TREES:c.set_cast_shadow(not aggressive)
    for c in PROXIES:c.set_cast_shadow(aggressive)
    for c in ROCKS:c.set_force_disable_nanite(aggressive)
    LIGHT.set_dynamic_shadow_distance_movable_light(3000. if aggressive else 5000.)
    vals={'r.Shadow.FilterMethod':0 if aggressive else 1,'r.VolumetricFog':0 if aggressive else 1,'r.SSR.Quality':0 if aggressive else 2,'r.Water.SingleLayer.Reflection':0 if aggressive else 1,'r.Water.SingleLayer.RefractionDownsampleFactor':4 if aggressive else 1,'r.Water.SingleLayer.SSRTAA':0 if aggressive else 1,'r.FidelityFX.FSR.QualityMode':0,'r.FidelityFX.FSR.Sharpness':0,'r.FidelityFX.FSR.ForceVertexDeformationOutputsVelocity':0 if aggressive else 1}
    for k,v in vals.items():cmd(k+' '+str(v))
    cmd('r.FidelityFX.FSR.Enabled '+str(int(aggressive)))
    cmd('r.Velocity.EnableLandscapeGrass '+str(int(not aggressive)))
    st={n:unreal.SystemLibrary.get_console_variable_string_value(n) for n in NAMES}
    assert st['r.ScreenPercentage']=='100' and st['r.FidelityFX.FI.Enabled']=='0'
    (OUT/(name+'_estado.json')).write_text(json.dumps({'cvars':st,'tree_components':len(TREES),'proxies':len(PROXIES),'rocks':len(ROCKS),'shadow_distance_cm':3000 if aggressive else 5000},indent=2))
    pose(0)
profile('alto60');cmd('t.OverrideFPS 0')
EVENTS=[];START=time.perf_counter();J=[]
for i in range(1,4):
    t=45+(i-1)*35
    EVENTS.extend([(t,'start_'+str(i),lambda i=i:(cmd(f'CsvProfile STARTFILE=alto60_r{i}.csv'),cmd('CsvProfile START'))),(t+30,'stop',lambda:cmd('CsvProfile STOP'))])
EVENTS.extend([(153,'shot',lambda:cmd('Shot filename="'+str(OUT/'alto60.png')+'" -nosuffix')),(157,'profile',lambda:(cmd('r.ProfileGPU.ShowUI 0'),cmd('ProfileGPU'))),(162,'videos',lambda:start_videos())])
MOVE=P.get_component_by_class(unreal.CharacterMovementComponent)
START_TR=P.get_actor_transform();BODY=next(c for c in P.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=='Indra')
CLIPS=['frente_viradas','strafe_re','camera_grama','parada'];VIDEO={'active':False,'done':False};TELE=[]
def reset_clip():
    P.do_move(0.,0.);P.do_sprint_end();MOVE.stop_movement_immediately()
    P.set_actor_transform(START_TR,False,True)
    PC.reset_ignore_move_input();PC.set_control_rotation(unreal.Rotator(0,90,0))
    pose(0)
def start_videos():
    VIDEO.update(active=True,case=0,clip=0,frame=-120)
    cmd('t.OverrideFPS 60');profile('base');reset_clip()
def vtick():
    n=VIDEO['frame'];name=['base','alto60'][VIDEO['case']];clip=CLIPS[VIDEO['clip']]
    folder=FRAMES/name/clip;folder.mkdir(exist_ok=True,parents=True)
    if n<0:P.do_move(0.,0.);pose(0)
    elif n<360:
        t=n/60.;right=0.;forward=0.
        if n==0 and clip in ['frente_viradas','parada']:P.do_sprint_start()
        if clip=='frente_viradas':
            forward=1. if t<1.4 else 0.;right=0. if t<1.4 else (1. if t<2.7 else (-1. if t<4.2 else 1.))
        elif clip=='strafe_re':right=1. if t<1.5 else (-1. if t<3 else 0.);forward=-1. if 3<=t<5 else 0.
        elif clip=='parada':forward=1. if t<2.5 else 0.
        P.do_move(right,forward)
        if clip=='camera_grama':
            CAM.set_world_location(unreal.Vector(0,-312.028006,250.352636),False,True)
            CAM.set_world_rotation(unreal.Rotator(-26,90+45*math.sin(t*4),0),False,True)
        else:
            # Follow the same input-driven route, keeping body/hair/sleeves legible.
            loc=P.get_actor_location()
            CAM.set_world_location(unreal.Vector(loc.x,loc.y-500,loc.z+190),False,True)
            CAM.set_world_rotation(unreal.Rotator(-15,90,0),False,True)
        cmd('Shot filename="'+str(folder/f'{n:04d}.png')+'" -nosuffix')
        TELE.append({'profile':name,'clip':clip,'frame':n,'speed':P.get_velocity().length(),'position':[P.get_actor_location().x,P.get_actor_location().y,P.get_actor_location().z]})
    elif n==362:
        (folder/'COMPLETE.json').write_text(json.dumps({'frames':360,'fps':60}))
        VIDEO['clip']+=1
        if VIDEO['clip']==4:
            VIDEO['clip']=0;VIDEO['case']+=1
            if VIDEO['case']==2:
                VIDEO.update(active=False,done=True);cmd('t.OverrideFPS 0');reset_clip();profile('base');PC.set_ignore_move_input(True)
                (OUT/'video_telemetria.json').write_text(json.dumps(TELE))
                (OUT/'concluido.json').write_text(json.dumps({'done':True}));return
            profile('alto60')
        reset_clip();VIDEO['frame']=-121
    VIDEO['frame']+=1
def tick(dt):
    try:
        elapsed=time.perf_counter()-START
        if EVENTS and elapsed>=EVENTS[0][0]:
            _,label,fn=EVENTS.pop(0);fn();J.append({'event':label,'time':elapsed});(OUT/'status.json').write_text(json.dumps(J))
        if VIDEO['active']:vtick()
        if VIDEO.get('frame',0)%60==0:(OUT/'video_status.json').write_text(json.dumps(VIDEO))
        if VIDEO['done']:unreal.unregister_slate_post_tick_callback(HANDLE)
    except Exception:
        unreal.unregister_slate_post_tick_callback(HANDLE);cmd('CsvProfile STOP');cmd('t.OverrideFPS 0');P.do_move(0.,0.);(OUT/'RUNTIME_ERRO.txt').write_text(traceback.format_exc())
HANDLE=unreal.register_slate_post_tick_callback(tick)
