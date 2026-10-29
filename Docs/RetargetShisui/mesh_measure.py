from pathlib import Path
import struct,ast,zipfile,json,math,collections
OUT=Path(__file__).parent
D=Path(r'C:/Users/lucas/AppData/Local/Temp/claude/C--Users-lucas-Downloads-Hype-World/c85f280b-5e9c-44e3-acd2-5c1cc16c1c08/scratchpad/ssrc')
def npy(b):
    size=struct.unpack_from('<H',b,8)[0];h=ast.literal_eval(b[10:10+size].decode());fmt={'<f8':'d','<f4':'f','<i8':'q','<i2':'h'}[h['descr']]
    flat=[x[0] for x in struct.iter_unpack('<'+fmt,b[10+size:])]
    width=h['shape'][-1] if len(h['shape'])>1 else 1
    return [flat[i:i+width] for i in range(0,len(flat),width)]
z=zipfile.ZipFile(D/'base.npz');P=npy(z.read('P.npy'));colors=npy((D/'vcol.npy').read_bytes())
z=zipfile.ZipFile(D/'pesos.npz');J=npy(z.read('J.npy'));W=npy(z.read('W.npy'))
b=(D/'SK_Shisui.glb').read_bytes();n=struct.unpack_from('<I',b,12)[0];glb=json.loads(b[20:20+n]);nodes=glb['nodes'];names=[nodes[i].get('name','') for i in glb['skins'][0]['joints']]
print('JOINTS',names[:20]);print('BOUNDS',[(min(p[k] for p in P),max(p[k] for p in P)) for k in range(3)])
ref=json.loads((OUT/'audit.json').read_text())['refs']['shisui']['bones']
hand=[]
for p,j,w in zip(P,J,W):
    score=sum(weight for idx,weight in zip(j,w) if names[idx].endswith('_l') and any(s in names[idx] for s in ['hand','thumb','index','middle','ring','pinky']))
    if score>.5:hand.append(p)
wrist=ref['hand_l']['p'];tip=ref['middle_03_l']['p'];axis=[tip[k]-wrist[k] for k in range(3)];length=math.sqrt(sum(x*x for x in axis));axis=[x/length for x in axis]
projections=[sum((p[k]-wrist[k])*axis[k] for k in range(3)) for p in hand]
face=[p for p,c,j,w in zip(P,colors,J,W) if sum(weight for idx,weight in zip(j,w) if names[idx]=='head')>.5 and p[2]>145 and abs(p[0])<8 and p[1]<-3.5 and c[0]>170 and c[1]>140 and c[2]>110 and c[0]>c[1]+4 and c[1]>c[2]+3]
result={'hand_weighted_vertex_count':len(hand),'hand_wrist_to_mesh_tip_cm':max(projections),'hand_mesh_span_cm':max(projections)-min(projections),'face_skin_z_bounds_cm':[min(p[2] for p in face),max(p[2] for p in face)],'face_skin_height_estimate_cm':max(p[2] for p in face)-min(p[2] for p in face),'face_vertex_count':len(face),'note':'Mesh hand length is projected along wrist-middle axis; face estimate uses skin-colored frontal vertices, excluding dark hair and headband. It is a mesh estimate, not a clinical anatomical landmark measurement.'}
(OUT/'mesh_measure.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
