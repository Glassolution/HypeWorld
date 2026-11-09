"""Novas animações, em cópia. Espaço UE em cm; aplicação no Blender em metros."""
from pathlib import Path
import json,math
import bpy
from mathutils import Matrix,Quaternion,Vector
R=Path(__file__).resolve().parents[1]
helper=R/'Dependencies/rig_math.py'
ns={'__file__':str(helper)}
exec(compile(helper.read_text().rsplit('\nbuild()',1)[0],str(helper),'exec'),ns)
decode,encode,world,blend,setrot=(ns[k] for k in ('decode','encode','world','blend','set_rotation'))
NAMES=ns['NAMES'];PARENTS=ns['PARENTS']
raw=json.loads((R/'Diagnostico/Clipes atuais.json').read_text())
ORDER=['Serpente','Carneiro','Macaco','Javali','Cavalo','Tigre']
old={n:decode(raw[n+'_Fixo']['frames'][0] if n!='Serpente' else raw[n]['frames'][-1]) for n in ORDER}
neutral=decode(raw['Serpente']['frames'][0]);bind=decode(ns['SOURCE']['ref'])
base=world(old['Tigre'])
rig=bpy.data.objects['Silent_Stance_Rig']
obj=bpy.data.objects['SK_Uchiha_Robe_Carneiro_Diagnostico']
keys=obj.data.shape_keys.key_blocks
C=Matrix.Diagonal((100,-100,100,1));CINV=C.inverted()
ref=world(bind);refinv={n:Matrix.LocRotScale(*ref[n]).inverted() for n in NAMES}
finger_names=[n for n in NAMES if any(n.startswith(d+'_') for d in ('thumb','index','middle','ring','pinky'))]

def ease(x):
    x=max(0.,min(1.,x));return x*x*x*(10+x*(-15+6*x))
def window(t,a,b):return ease((t-a)/(b-a))
def clearance(t):return window(t,0,.32)*(1-window(t,.68,1))
def at(frames,t,length):
    f=max(0,min(len(frames)-1,t/length*(len(frames)-1)));i=int(f)
    return blend(decode(frames[i]),decode(frames[min(i+1,len(frames)-1)]),f-i)

def arm(pose,side,wrist,pole,hand_rotation):
    """Two-bone IK com polo estável e menor giro da orientação de referência.

    Os twists herdados continuam relativos ao antebraço: não concentrar giro na mão.
    """
    u,e,h=[f'{n}_{side}' for n in ('upperarm','lowerarm','hand')]
    g=world(pose);a,b,c=[g[n][0] for n in (u,e,h)]
    l1,l2=(b-a).length,(c-b).length
    dlt=wrist-a;distance=min(max(dlt.length,abs(l1-l2)+.001),l1+l2-.001)
    axis=dlt.normalized();along=(l1*l1-l2*l2+distance*distance)/(2*distance)
    lateral=pole-a-axis*(pole-a).dot(axis)
    if lateral.length<1e-5:lateral=Vector((-1 if side=='r' else 1,0,-.3))
    target=a+axis*along+lateral.normalized()*math.sqrt(max(0,l1*l1-along*along))
    q=(b-a).rotation_difference(target-a)@g[u][1]
    setrot(pose,u,q)
    q=(c-b).rotation_difference(wrist-target)@g[e][1]
    setrot(pose,e,q);setrot(pose,h,hand_rotation)
    return pose

def stable_pose(name):
    p=decode(encode(old[name]));g=world(p)
    for side in ('r','l'):
        h='hand_'+side
        arm(p,side,g[h][0],base['lowerarm_'+side][0],g[h][1])
    return p

def secondary(p,t,sign=1.):
    amount=math.sin(math.pi*t)**2
    for bone,axis,degrees in [('spine_03',(0,0,1),.32*sign),('spine_04',(1,0,0),.20)]:
        v,q,s=p[bone];p[bone]=(v,Quaternion(Vector(axis),math.radians(degrees*amount))@q,s)
    return p

def make_transition(a,b,label,entry=False):
    ga,gb=world(a),world(b);result=[]
    for i in range(31):
        t=i/30
        if i==0:p=decode(encode(a))
        elif i==30:p=decode(encode(b))
        else:
            progress=ease(t);turn=window(t,.12,.88)
            p=blend(a,b,progress)
            if not entry:secondary(p,t,1 if label in ('Serpente_Carneiro','Macaco_Javali','Cavalo_Tigre') else -1)
            # Primeiro desencaixar; depois girar; dedos chegam antes da acomodação do braço.
            release=window(t,.15,.43);arrive=window(t,.48,.88)
            for n in finger_names:
                v,q,s=a[n];_,restq,_=bind[n]
                relaxed=q.slerp(restq,.68 if n.startswith('thumb') else .82)
                p[n]=(v.lerp(b[n][0],progress),relaxed.slerp(b[n][1],arrive) if t>=.48 else q.slerp(relaxed,release),s.lerp(b[n][2],progress))
            for side,sign in [('r',-1),('l',1)]:
                h='hand_'+side;e='lowerarm_'+side
                bump=clearance(t)
                offset=Vector((sign*(1.5 if entry else 2.7),5.0 if side=='r' else 1.0,1.1 if side=='r' else -.5))*bump
                wrist=ga[h][0].lerp(gb[h][0],progress)+offset
                pole=ga[e][0].lerp(base[e][0],progress) if entry else base[e][0]
                arm(p,side,wrist,pole,ga[h][1].slerp(gb[h][1],turn))
        result.append(encode(p))
    return result

def make_breath(tiger):
    source=raw['Sopro'];release=at(source['frames'],.375,source['length'])
    gt,gr=world(tiger),world(release);frames=[]
    for i in range(46):
        t=i/30.
        if t<.375:
            x=t/.375;e=ease(x);p=blend(tiger,release,e)
            secondary(p,x,.5)
            for side,sign in [('r',-1),('l',1)]:
                h='hand_'+side;el='lowerarm_'+side
                position=gt[h][0].lerp(gr[h][0],e)+Vector((sign*2,4 if side=='r' else 0,1))*clearance(x)
                pole=base[el][0].lerp(gr[el][0],e)
                arm(p,side,position,pole,gt[h][1].slerp(gr[h][1],window(x,.1,.9)))
        else:
            p=at(source['frames'],t,source['length'])
            if t>.94:p=blend(p,neutral,ease((t-.94)/.56))
        if i==0:p=tiger
        if i==45:p=neutral
        frames.append(encode(p))
    return frames

def custom_weights(label,t):
    if label=='Sopro':return {'Corr_Carneiro_Punho_Limpo':0.,'Corr_Carneiro_Contato_Limpo':0.,'Corr_Javali_Antebraco':0.,'Corr_Selo_Javali_MCP':0.,'Corr_Javali_Contato':0.,'Corr_Cavalo_Contato':0.}
    if label.endswith('_Fixo'):
        a=b=label[:-5];out=1.;incoming=1.
    elif label=='Serpente':a='Neutro';b='Serpente';out=1-window(t,.08,.38);incoming=window(t,.66,.98)
    else:a,b=label.split('_',1);out=1-window(t,.08,.38);incoming=window(t,.66,.98)
    w=lambda name:(out if a==name else 0.)+(incoming if b==name and a!=b else 0.)
    cover=max(w('Carneiro'),w('Javali'),w('Cavalo'))
    return {'Corr_Carneiro_Punho_Limpo':w('Carneiro'),'Corr_Carneiro_Contato_Limpo':w('Carneiro'),'Corr_Javali_Antebraco':min(1,cover),'Corr_Selo_Javali_MCP':w('Javali'),'Corr_Javali_Contato':w('Javali'),'Corr_Cavalo_Contato':w('Cavalo')}

def apply_pose(pose):
    """Aplica em dados, uma atualização; não depende de seleção/interface para cada osso."""
    rig.data.pose_position='POSE';g=world(pose)
    targets={n:CINV@Matrix.LocRotScale(*g[n])@refinv[n]@C@rig.data.bones[n].matrix_local for n in NAMES}
    for n in NAMES:
        bone=rig.data.bones[n];par=bone.parent
        kwargs={'parent_matrix':targets[par.name],'parent_matrix_local':par.matrix_local} if par else {}
        rig.pose.bones[n].matrix_basis=bone.convert_local_to_pose(targets[n],bone.matrix_local,invert=True,**kwargs)
    bpy.context.view_layer.update()
    error=max((rig.pose.bones[n].matrix.translation-targets[n].translation).length for n in NAMES)
    assert error<.00002,('Aplicacao divergiu',error)
    return error

def setup_view():
    for ob in bpy.context.scene.objects:
        if ob.type=='MESH':ob.hide_set(ob!=obj)
    obj.hide_set(False);rig.hide_set(False)
    for ob in bpy.context.selected_objects:ob.select_set(False)
    rig.select_set(True);bpy.context.view_layer.objects.active=rig
    if rig.mode!='POSE':bpy.ops.object.mode_set(mode='POSE')
    for area in bpy.context.screen.areas:
        if area.type=='VIEW_3D':
            s=area.spaces.active;s.overlay.show_overlays=False;s.shading.type='SOLID';s.shading.light='STUDIO';s.shading.color_type='TEXTURE';s.shading.show_shadows=False
            s.lens=70;s.region_3d.view_perspective='PERSP';s.region_3d.view_location=Vector((.015,-.22,1.05));s.region_3d.view_distance=1.8;s.region_3d.view_rotation=Vector((0,-1,.02)).to_track_quat('Z','Y')

def shapes(pose,label,t):
    for k in keys:
        if k.name!='Basis':k.value=0
    # Copiar apenas a função já aprovada que traduz flexão de dedos em corretivos nativos.
    data={'bpy':bpy,'math':math,'bind':bind,'ref':ref,'mesh':obj,'world':world,'Vector':Vector,'Quaternion':Quaternion}
    file=R/'Dependencies/curvas_nativas.py'
    exec(compile(file.read_text(),str(file),'exec'),data)
    data['native_curves'](pose,obj)
    for name,value in custom_weights(label,t).items():
        if name in keys:keys[name].value=value

def generate():
    poses={n:stable_pose(n) for n in ORDER}
    clips={'Serpente':make_transition(neutral,poses['Serpente'],'Entrada',True)}
    for n in ORDER[1:]:clips[n+'_Fixo']=[encode(poses[n])]*2
    for a,b in zip(ORDER,ORDER[1:]):clips[a+'_'+b]=make_transition(poses[a],poses[b],a+'_'+b)
    clips['Sopro']=make_breath(poses['Tigre'])
    clips['Biblioteca']=[encode(poses[n]) for n in ORDER]
    (R/'Blender/Clipes candidatos.json').write_text(json.dumps(clips,separators=(',',':')))
    (R/'Blender/Poses candidatas.json').write_text(json.dumps({n:encode(p) for n,p in poses.items()},separators=(',',':')))
    report={}
    for n in ORDER:
        before,after=world(old[n]),world(poses[n]);report[n]={}
        for side in ('r','l'):
            h,e='hand_'+side,'lowerarm_'+side
            report[n][side]={'hand_delta_cm':(before[h][0]-after[h][0]).length,'elbow_before':list(before[e][0]),'elbow_after':list(after[e][0]),'elbow_delta_cm':(before[e][0]-after[e][0]).length}
    (R/'Blender/Postura dos bracos.json').write_text(json.dumps(report,indent=2))
    setup_view();apply_pose(poses['Javali']);shapes(poses['Javali'],'Javali_Fixo',0);bpy.context.view_layer.update()
    print('CANDIDATOS',len(clips),'clipes; maos preservadas; ajustes de cotovelos:',report)
