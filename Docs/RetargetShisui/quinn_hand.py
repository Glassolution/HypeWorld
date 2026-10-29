from pathlib import Path
import re,json,math,collections
OUT=Path(__file__).parent
text=(OUT/'Quinn_reference.fbx').read_text(encoding='utf-8')
headers=list(re.finditer(r'(?m)^\t(Geometry|Deformer|Model): (\d+), "([^"]*)", "([^"]*)"',text))
objects={}
for i,h in enumerate(headers):objects[int(h[2])]={'type':h[1],'name':h[3].split('::')[-1],'subtype':h[4],'text':text[h.start():headers[i+1].start() if i+1<len(headers) else len(text)]}
parents=collections.defaultdict(list)
for a,b in re.findall(r'C: "OO",(\d+),(\d+)',text):parents[int(a)].append(int(b))
def array(obj,field):
    m=re.search(r'\b'+field+r': \*\d+ \{\s*a:\s*(.*?)\}',obj['text'],re.S)
    return [float(v) for v in m[1].replace('\n','').replace('\r','').split(',') if v.strip()] if m else []
geometries={i:array(o,'Vertices') for i,o in objects.items() if o['type']=='Geometry' and o['subtype']=='Mesh'}
geometry=max(geometries,key=lambda i:len(geometries[i]));flat=geometries[geometry];points=[flat[i:i+3] for i in range(0,len(flat),3)]
clusters={i:o for i,o in objects.items() if o['type']=='Deformer' and o['subtype']=='Cluster'}
bone_for={}
for i,o in objects.items():
    if o['type']=='Model' and o['subtype']=='LimbNode':
        for par in parents[i]:
            if par in clusters:bone_for[par]=o['name']
scores=collections.defaultdict(float);transforms={}
for i,o in clusters.items():
    if not any(geometry in parents[skin] for skin in parents[i]):continue
    name=bone_for.get(i,'')
    if name in ['hand_l','middle_03_l']:
        matrix=array(o,'TransformLink');transforms[name]=matrix[12:15]
    if name.endswith('_l') and any(s in name for s in ['hand','thumb','index','middle','ring','pinky']):
        for v,w in zip(array(o,'Indexes'),array(o,'Weights')):scores[int(v)]+=w
wrist=transforms['hand_l'];tip=transforms['middle_03_l'];axis=[tip[k]-wrist[k] for k in range(3)];length=math.sqrt(sum(x*x for x in axis));axis=[x/length for x in axis]
projections=[sum((points[i][k]-wrist[k])*axis[k] for k in range(3)) for i,score in scores.items() if score>.5]
result={'mesh_wrist_to_tip_cm':max(projections),'mesh_span_cm':max(projections)-min(projections),'wrist':wrist,'middle_03':tip,'hand_vertices':len(projections),'note':'Projected wrist-to-fingertip mesh extent using vertices with cumulative hand/finger skin weight above 0.5; same criterion as Shisui.'}
(OUT/'quinn_hand.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
