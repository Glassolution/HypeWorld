from pathlib import Path
import numpy as np,json,struct,copy
OUT=Path(__file__).parent;OLD=OUT.parent/'KakashiAnbu';SS=Path(r'C:/Users/lucas/AppData/Local/Temp/claude/C--Users-lucas-Downloads-Hype-World/c85f280b-5e9c-44e3-acd2-5c1cc16c1c08/scratchpad/ssrc')
HAIR=Path(r'C:/Users/lucas/Downloads/personagem para customizacao/Cabelos/itachi.glb')
def load(path):
    raw=path.read_bytes();n=struct.unpack_from('<I',raw,12)[0];return json.loads(raw[20:20+n]),raw[28+n:]
g,raw=load(OLD/'SK_KakashiAnbu.glb');blob=bytearray(raw);hg,hblob=load(HAIR)
def read(gl,buffer,index):
    a=gl['accessors'][index];v=gl['bufferViews'][a['bufferView']];count={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']];dt=np.dtype({5126:'<f4',5125:'<u4',5123:'<u2',5121:'u1'}[a['componentType']]);return np.ndarray((a['count'],count),dtype=dt,buffer=buffer,offset=v.get('byteOffset',0)+a.get('byteOffset',0),strides=(v.get('byteStride',dt.itemsize*count),dt.itemsize)).copy()
def binary(data):
    while len(blob)%4:blob.append(0)
    g['bufferViews'].append({'buffer':0,'byteOffset':len(blob),'byteLength':len(data)});blob.extend(data);return len(g['bufferViews'])-1
def add(array,typ,component,target=None):
    view=binary(array.tobytes())
    if target:g['bufferViews'][view]['target']=target
    a={'bufferView':view,'componentType':component,'count':len(array),'type':typ}
    if typ=='VEC3':a.update(min=array.min(0).tolist(),max=array.max(0).tolist())
    g['accessors'].append(a);return len(g['accessors'])-1
def texture(path,name):
    index=len(g['images']);g['images'].append({'bufferView':binary(path.read_bytes()),'mimeType':'image/png','name':name});g['textures'].append({'source':index});return len(g['textures'])-1
def material(name,texture_index):
    g['materials'].append({'name':name,'doubleSided':True,'pbrMetallicRoughness':{'baseColorTexture':{'index':texture_index},'metallicFactor':0.,'roughnessFactor':1.}});return len(g['materials'])-1
data=np.load(OLD/'geometry.npz');P=data['P'];components=np.load(OLD/'components.npy');ground=-float(P[:,2].min());head_index=next(i for i,node in enumerate(g['nodes']) if node.get('name')=='head')
body_primitives=[];removed=0;neck_pieces=[]
body_geometry=np.load(OLD/'geometry.npz');skin=np.load(OLD/'rig_weights.npz')
for primitive in g['meshes'][0]['primitives']:
    triangles=read(g,raw,primitive['indices']).reshape(-1,3);center=P[triangles].mean(1)
    region=(abs(center[:,0])<25)&~np.isin(components[triangles[:,0]],[7,21,22]);above=(P[triangles,2]>162.5)
    old_head=region&above.any(1);removed+=int(old_head.sum())
    crossing=triangles[old_head&~above.all(1)]
    clip_points=[];clip_normals=[];clip_uv=[];clip_joints=[];clip_weights=[]
    for tri in crossing:
        poly=[(P[v].copy(),np.eye(3)[i]) for i,v in enumerate(tri)];clipped=[]
        for i,(point,bary) in enumerate(poly):
            previous,previous_bary=poly[i-1];inside=point[2]<=162.5;previous_inside=previous[2]<=162.5
            if inside!=previous_inside:
                t=(162.5-previous[2])/(point[2]-previous[2]);clipped.append((previous+t*(point-previous),previous_bary+t*(bary-previous_bary)))
            if inside:clipped.append((point,bary))
        for i in range(1,len(clipped)-1):
            for point,bary in [clipped[0],clipped[i],clipped[i+1]]:
                point=point.copy();point[2]+=ground;clip_points.append(point);normal=bary@body_geometry['N'][tri];clip_normals.append(normal/max(np.linalg.norm(normal),1e-12));clip_uv.append(bary@body_geometry['UV'][tri])
                contributions={}
                for source_vertex,factor in zip(tri,bary):
                    for joint,weight in zip(skin['J'][source_vertex],skin['W'][source_vertex]):contributions[int(joint)]=contributions.get(int(joint),0)+factor*weight
                top=sorted(contributions.items(),key=lambda item:-item[1])[:4];total=sum(weight for _,weight in top);clip_joints.append([joint for joint,_ in top]);clip_weights.append([weight/total for _,weight in top])
    if clip_points:neck_pieces.append((np.array(clip_points),np.array(clip_normals),np.array(clip_uv),np.array(clip_joints,dtype='<u2'),np.array(clip_weights,dtype='<f4'),primitive['material']))
    triangles=triangles[~old_head]
    if len(triangles):
        result=copy.deepcopy(primitive);result['indices']=add(triangles.reshape(-1).astype('<u4'),'SCALAR',5125,34963);body_primitives.append(result)
def conv(p):return np.stack([p[:,0],p[:,2],-p[:,1]],axis=1)
def skinned_primitive(points,normals,uv,triangles,mat,joints=None,weights=None):
    if joints is None:
        joints=np.zeros((len(points),4),'<u2');joints[:,0]=head_index;weights=np.zeros((len(points),4),'<f4');weights[:,0]=1.
    ap=add((conv(points)/100).astype('<f4'),'VEC3',5126,34962);an=add(conv(normals).astype('<f4'),'VEC3',5126,34962);au=add(uv.astype('<f4'),'VEC2',5126,34962);aj=add(joints,'VEC4',5123,34962);aw=add(weights,'VEC4',5126,34962);ai=add(triangles.reshape(-1).astype('<u4'),'SCALAR',5125,34963)
    return {'attributes':{'POSITION':ap,'NORMAL':an,'TEXCOORD_0':au,'JOINTS_0':aj,'WEIGHTS_0':aw},'indices':ai,'material':mat}
for points,normals,uv,joints,weights,mat in neck_pieces:body_primitives.append(skinned_primitive(points,normals,uv,np.arange(len(points)).reshape(-1,3),mat,joints,weights))
# Neutral unmasked face. The supplied Itachi file contains hairstyle/headband but no face geometry.
face=np.load(SS/'base.npz');face_components=np.load(OUT/'face_components.npy')
face_texture=texture(SS/'tex0.png','T_ItachiAnbu_FaceBase');eye_texture=texture(SS/'tex1.png','T_ItachiAnbu_EyesBase');face_mats=[material('M_ItachiAnbu_FaceBase',face_texture),material('M_ItachiAnbu_EyesBase',eye_texture)]
selected=np.isin(face_components[face['T'][:,0]],[16,18,24,25,26,29])
for mi in [0,1]:
    selection=np.flatnonzero(selected&(face['MAT']==mi));vid=face['T'][selection].reshape(-1);points=face['P'][vid].copy();points[:,0]*=.96;points[:,1]+=1.;points[:,2]+=11.3+ground
    normals=face['N'].reshape(-1,3,3)[selection].reshape(-1,3).copy();normals[:,0]/=.96;normals/=np.maximum(np.linalg.norm(normals,axis=1,keepdims=True),1e-12)
    uv=face['UV'].reshape(-1,3,2)[selection].reshape(-1,2)
    body_primitives.append(skinned_primitive(points,normals,uv,np.arange(len(points)).reshape(-1,3),face_mats[mi]))
hair_texture_map={}
for i,image in enumerate(hg['images']):
    view=hg['bufferViews'][image['bufferView']];payload=hblob[view.get('byteOffset',0):view.get('byteOffset',0)+view['byteLength']];path=OUT/f'head_texture_{i}.png';path.write_bytes(payload);hair_texture_map[i]=texture(path,f'T_ItachiAnbu_Head_{i}')
hair_materials=[]
for mat in hg['materials']:
    mat=copy.deepcopy(mat);original=mat['pbrMetallicRoughness']['baseColorTexture']['index'];mat['pbrMetallicRoughness']['baseColorTexture']['index']=hair_texture_map[hg['textures'][original]['source']];mat['pbrMetallicRoughness']['roughnessFactor']=1.;g['materials'].append(mat);hair_materials.append(len(g['materials'])-1)
for mesh in hg['meshes']:
    for primitive in mesh['primitives']:
        attrs=primitive['attributes'];points=read(hg,hblob,attrs['POSITION']).astype(float)*100;points[:,2]+=12.+ground
        body_primitives.append(skinned_primitive(points,read(hg,hblob,attrs['NORMAL']),read(hg,hblob,attrs['TEXCOORD_0']),read(hg,hblob,primitive['indices']).reshape(-1,3),hair_materials[primitive['material']]))
g['meshes'][0]={'name':'ItachiAnbu','primitives':body_primitives}
for node in g['nodes']:
    if node.get('mesh')==0:node['name']='SK_ItachiAnbu'
g['asset']={'version':'2.0','generator':'Hype World - Itachi ANBU local character assembly','extras':{'uniform_source':g['asset'].get('extras',{}),'hair_headband_source':hg['asset'].get('extras',{}),'face_source':'Neutral unmasked base face from existing Shisui character; geometry and textures retained in copy.'}}
g['buffers']=[{'byteLength':len(blob)}];text=json.dumps(g).encode();text+=b' '*(-len(text)%4);blob.extend(b'\0'*(-len(blob)%4))
result=struct.pack('<III',0x46546C67,2,12+8+len(text)+8+len(blob))+struct.pack('<II',len(text),0x4E4F534A)+text+struct.pack('<II',len(blob),0x004E4942)+blob
(OUT/'SK_ItachiAnbu.glb').write_bytes(result);(OUT/'assembly.json').write_text(json.dumps({'body':'Kakashi ANBU uniform, original hands/accessories and rig','head':'Itachi hairstyle and headband plus neutral unmasked base face','removed_original_head_triangles':removed,'primitives':len(body_primitives),'itachi_source':hg['asset'],'face_translation_cm':[0,1,11.3],'hair_translation_cm':[0,0,12.]},indent=2))
print('ITACHI_ASSEMBLED',len(result),'bytes','removed_old_head_triangles',removed)
