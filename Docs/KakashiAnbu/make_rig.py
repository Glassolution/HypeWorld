from pathlib import Path
import numpy as np,json,struct
from scipy.sparse import coo_matrix
OUT=Path(__file__).parent
d=np.load(OUT/'geometry.npz');P=d['P'].copy();N=d['N'];UV=d['UV'];T=d['T'];MAT=d['MAT'];PART=d['PART'];components=np.load(OUT/'components.npy')
ground=-float(P[:,2].min());P[:,2]+=ground
B=[]
def bone(name,parent,p):B.append((name,parent,np.array(p,float)+[0,0,ground]))
bone('root',None,(0,0,-ground))
for name,parent,p in [('pelvis','root',(0,1,104)),('spine_01','pelvis',(0,1,110)),('spine_02','spine_01',(0,1,119)),('spine_03','spine_02',(0,1,129)),('spine_04','spine_03',(0,1,139)),('spine_05','spine_04',(0,1,149)),('neck_01','spine_05',(0,1,159)),('neck_02','neck_01',(0,1,164)),('head','neck_02',(0,1,169)),('head_end','head',(0,1,188)),('Boca','head',(0,-8,168))]:bone(name,parent,p)
for side,sign in [('l',1),('r',-1)]:
    for name,parent,p in [('clavicle','spine_05',(4,3,155)),('upperarm','clavicle',(19,3,152)),('lowerarm','upperarm',(43,5.5,151)),('hand','lowerarm',(69,4.5,151)),('thigh','pelvis',(9,1,101)),('calf','thigh',(10,-2,55)),('foot','calf',(9.5,1,10)),('ball','foot',(9.5,-11,3)),('ball_end','ball',(9.5,-20,2))]:
        bone(name+'_'+side,parent if parent in ['spine_05','pelvis'] else parent+'_'+side,(p[0]*sign,p[1],p[2]))
    for i,(x,y,z) in enumerate([(74,-1,150.7),(77,-4.2,150.2),(79.5,-7.3,150),(81.2,-9.7,149.8)],1):bone('thumb_%02d_'%i+side,'hand_'+side if i==1 else 'thumb_%02d_'%(i-1)+side,(sign*x,y,z))
    for name,y,length in [('index',-3,11.3),('middle',0.3,13.2),('ring',3.6,11.8),('pinky',6.7,8.8)]:
        start=77.5 if name!='pinky' else 76.2
        bone(name+'_metacarpal_'+side,'hand_'+side,(sign*73.5,y,151.5))
        for i,t in enumerate([0,.40,.72,1.],1):bone(name+'_%02d_'%i+side,name+'_metacarpal_'+side if i==1 else name+'_%02d_'%(i-1)+side,(sign*(start+length*t),y,151.5))
names=[b[0] for b in B];IDX={n:i for i,n in enumerate(names)};pos={n:p for n,_,p in B};children={}
for name,parent,p in B:
    if parent:children.setdefault(parent,[]).append(name)
leaves={n for n in names if n not in children};deform=[n for n in names if n not in leaves and n!='root']
W=np.zeros((len(P),len(B)),float)
for name in deform:
    targets=children.get(name,[])
    if name.startswith('hand_'):targets=['middle_metacarpal_'+name[-1],'index_metacarpal_'+name[-1],'pinky_metacarpal_'+name[-1],'thumb_01_'+name[-1]]
    distances=[]
    for end in targets:
        a=pos[name];v=pos[end]-a;t=np.clip(((P-a)@v)/max(v@v,.0001),0,1);distances.append(np.linalg.norm(P-(a+t[:,None]*v),axis=1))
    distance=np.min(distances,axis=0)
    radius=1.2 if any(f in name for f in ['thumb','index','middle','ring','pinky']) else 3.5
    W[:,IDX[name]]=1/(distance*distance+radius*radius)**2
x,y,z=P.T;z=z-ground
allowed=np.zeros_like(W,bool)
torso=[n for n in deform if n in ['pelvis','spine_01','spine_02','spine_03','spine_04','spine_05','neck_01','neck_02','head']]
allowed[:,[IDX[n] for n in torso]]=True
armmask=np.zeros(len(P),bool)
for side,sign in [('l',1),('r',-1)]:
    arm=[n for n in deform if n.endswith('_'+side) and not n.startswith(('thigh','calf','foot','ball'))]
    mask=(x*sign>19)&(z>138)&(z<165);armmask|=mask;allowed[mask]=False;allowed[np.ix_(mask,[IDX[n] for n in arm])]=True
    shoulder=mask&(x*sign<29);allowed[np.ix_(shoulder,[IDX[n] for n in ['spine_04','spine_05']])]=True
    leg=(x*sign>=0)&(z<104)&~armmask;allowed[leg]=False;legbones=['pelvis','spine_01']+[n+'_'+side for n in ['thigh','calf','foot','ball']];allowed[np.ix_(leg,[IDX[n] for n in legbones])]=True
    low=leg&(z<78);allowed[np.ix_(low,[IDX['pelvis'],IDX['spine_01']])]=False
    foot=leg&(z<18);allowed[foot]=False;allowed[np.ix_(foot,[IDX['foot_'+side],IDX['ball_'+side]])]=True
W*=allowed
# Keep skull, eyes, hair, mask and sword rigid instead of blending them with shoulders or the torso.
head=((z>163)&(abs(x)<25)&(y<14))|(PART==0)
sword=np.isin(components,[7,21,22])
W[head]=0;W[head,IDX['head']]=1
W[sword]=0;W[sword,IDX['spine_05']]=1
# Loose glove straps follow the hand as accessories, without fingers pulling them apart.
for side,sign,groups in [('l',1,[76,77]),('r',-1,[2,3])]:
    strap=np.isin(components,groups);W[strap]=0;W[strap,IDX['hand_'+side]]=1
W/=np.maximum(W.sum(1,keepdims=True),1e-30)
edges=np.concatenate([T[:,[0,1]],T[:,[1,2]],T[:,[2,0]]]);edges=np.concatenate([edges,edges[:,::-1]])
A=coo_matrix((np.ones(len(edges)),(edges[:,0],edges[:,1])),shape=(len(P),len(P))).tocsr();A.data[:]=1;degree=np.maximum(np.asarray(A.sum(1)).ravel(),1)
smooth=~(head|sword)
for _ in range(3):
    neighbor=(A@W)/degree[:,None];W[smooth]=.65*W[smooth]+.35*neighbor[smooth];W[smooth]*=allowed[smooth];W/=np.maximum(W.sum(1,keepdims=True),1e-30)
indices=np.argsort(-W,axis=1)[:,:4];weights=np.take_along_axis(W,indices,axis=1);weights/=weights.sum(1,keepdims=True)
assert np.isfinite(weights).all() and np.allclose(weights.sum(1),1)
def conv(v):return np.stack([v[:,0],v[:,2],-v[:,1]],axis=1)
blob=bytearray();views=[];accessors=[]
def add(array,typ,component,target=None):
    while len(blob)%4:blob.append(0)
    view={'buffer':0,'byteOffset':len(blob),'byteLength':array.nbytes}
    if target:view['target']=target
    views.append(view);blob.extend(array.tobytes());a={'bufferView':len(views)-1,'componentType':component,'count':len(array),'type':typ}
    if typ=='VEC3':a.update(min=array.min(0).tolist(),max=array.max(0).tolist())
    accessors.append(a);return len(accessors)-1
ap=add((conv(P)/100).astype('<f4'),'VEC3',5126,34962);an=add(conv(N).astype('<f4'),'VEC3',5126,34962);au=add(UV.astype('<f4'),'VEC2',5126,34962)
aj=add(indices.astype('<u2'),'VEC4',5123,34962);aw=add(weights.astype('<f4'),'VEC4',5126,34962)
primitives=[]
for material in sorted(set(MAT.tolist())):
    ai=add(T[MAT==material].reshape(-1).astype('<u4'),'SCALAR',5125,34963);primitives.append({'attributes':{'POSITION':ap,'NORMAL':an,'TEXCOORD_0':au,'JOINTS_0':aj,'WEIGHTS_0':aw},'indices':ai,'material':int(material)})
inverse=[];nodes=[]
for name,parent,p in B:
    gp=conv(p[None])[0]/100;matrix=np.eye(4);matrix[:3,3]=-gp;inverse.append(matrix.T.reshape(-1))
    local=gp-(conv(pos[parent][None])[0]/100 if parent else 0);nodes.append({'name':name,'translation':local.tolist()})
for name,parent,p in B:
    if parent:nodes[IDX[parent]].setdefault('children',[]).append(IDX[name])
ib=add(np.array(inverse,dtype='<f4'),'MAT4',5126)
mesh_node=len(nodes);nodes.append({'name':'SK_KakashiAnbu','mesh':0,'skin':0})
source=json.loads((OUT/'source.json').read_text());images=[]
for i in range(3):
    raw=(OUT/f'texture_{i}.png').read_bytes()
    while len(blob)%4:blob.append(0)
    views.append({'buffer':0,'byteOffset':len(blob),'byteLength':len(raw)});blob.extend(raw);images.append({'bufferView':len(views)-1,'mimeType':'image/png','name':f'T_KakashiAnbu_{i}'})
g={'asset':{'version':'2.0','generator':'Hype World - Kakashi ANBU rig copy','extras':source['asset'].get('extras',{})},'scene':0,'scenes':[{'nodes':[0,mesh_node]}],'nodes':nodes,'skins':[{'joints':list(range(len(B))),'inverseBindMatrices':ib,'skeleton':0}],'meshes':[{'name':'KakashiAnbu','primitives':primitives}],'materials':source['materials'],'images':images,'textures':source['textures'],'buffers':[{'byteLength':len(blob)}],'bufferViews':views,'accessors':accessors}
j=json.dumps(g).encode();j+=b' '*(-len(j)%4);blob.extend(b'\0'*(-len(blob)%4))
file=struct.pack('<III',0x46546C67,2,12+8+len(j)+8+len(blob))+struct.pack('<II',len(j),0x4E4F534A)+j+struct.pack('<II',len(blob),0x004E4942)+blob
(OUT/'SK_KakashiAnbu.glb').write_bytes(file);(OUT/'skeleton.json').write_text(json.dumps([(n,pa,p.tolist()) for n,pa,p in B],indent=2));np.savez(OUT/'rig_weights.npz',P=P,J=indices,W=weights)
print('RIG_READY',len(B),'bones',len(P),'vertices',len(file),'bytes')
