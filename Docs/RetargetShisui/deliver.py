from pathlib import Path
import json,re,math,hashlib,subprocess,shutil,html,zlib
OUT=Path(__file__).parent;ROOT=OUT.parents[1]
# Unreal's render-target exporter can leave padding after IEND; validate chunks and remove only that padding.
for png in OUT.rglob('*.png'):
    data=png.read_bytes();assert data[:8]==b'\x89PNG\r\n\x1a\n',png
    pos=8
    while pos<len(data):
        length=int.from_bytes(data[pos:pos+4],'big');kind=data[pos+4:pos+8];body=data[pos+8:pos+8+length]
        crc=int.from_bytes(data[pos+8+length:pos+12+length],'big')
        assert pos+12+length<=len(data) and zlib.crc32(kind+body)==crc,png
        pos+=12+length
        if kind==b'IEND':
            if pos<len(data):png.write_bytes(data[:pos])
            break
before=json.loads((OUT/'before.json').read_text());after=json.loads((OUT/'after.json').read_text());video=json.loads((OUT/'video_done.json').read_text())
model=json.loads((OUT/'mesh_measure.json').read_text());quinn=json.loads((OUT/'quinn_hand.json').read_text())
def mul(a,b):
    x,y,z,w=a;X,Y,Z,W=b
    return [w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z]
def rotated(q,v):return mul(mul(q,[*v,0]),[-q[0],-q[1],-q[2],q[3]])[:3]
def old_head(data,reference):
    p,y,r=[math.radians(float(x))/2 for x in re.search(r'pitch: ([\d.-]+), yaw: ([\d.-]+), roll: ([\d.-]+)',data['head_rotation']).groups()]
    SP,CP=math.sin(p),math.cos(p);SY,CY=math.sin(y),math.cos(y);SR,CR=math.sin(r),math.cos(r)
    q=[CR*SP*SY-SR*CP*CY,-CR*SP*CY-SR*CP*SY,CR*CP*SY-SR*SP*CY,CR*CP*CY+SR*SP*SY]
    # Verify the Euler conversion against the X axis recorded by the original measurement.
    assert sum((a-b)**2 for a,b in zip(rotated(q,[1,0,0]),data['head_axis']))<1e-8
    delta=mul(q,[-reference[0],-reference[1],-reference[2],reference[3]])
    up=rotated(delta,[0,0,1]);return math.degrees(math.atan2(-up[1],up[2]))
for label in ['mannequin','shisui']:before[label]['head_deg']=old_head(before[label],after[label]['reference_head_quat'])
ratio=after['shisui']['mesh_reference_pelvis_cm']/after['mannequin']['mesh_reference_pelvis_cm']
assert abs(after['shisui']['pelvis_cm']-after['mannequin']['pelvis_cm']*ratio)<.001
assert abs(after['shisui']['head_deg']-after['mannequin']['head_deg'])<.001
assert abs(after['shisui']['spine_deg']-after['mannequin']['spine_deg'])<.01
assert max(abs(a-b) for a,b in zip(after['shisui']['knees_deg'],after['mannequin']['knees_deg']))<.25
assert video['frames']==96 and {r['case'] for r in video['rows']}=={'guarda','trote','ninja','tigre'}
assert all(abs(r['source']['head_deg']-r['target']['head_deg'])<.001 for r in video['rows'])
protected=json.loads((OUT/'protected.json').read_text());animation_checks=[];other_changes=[]
for item in protected:
    path=ROOT/item['path'];same=path.exists() and hashlib.sha256(path.read_bytes()).hexdigest()==item['sha256']
    is_animation=any(item['path'].replace('\\','/').startswith(p) for p in ['Content/ANAP/','Content/Ninja/Characters/Animations/','Content/HypeWorld/Animacoes/'])
    if is_animation:
        animation_checks.append({'path':item['path'],'unchanged':same});assert same,item['path']
    elif not same:other_changes.append(item['path'])
table=[
 ['Pelve (cm)',after['mannequin']['pelvis_cm'],before['shisui']['pelvis_cm'],after['shisui']['pelvis_cm']],
 ['Abertura dos pés (cm)',after['mannequin']['feet_width_cm'],before['shisui']['feet_width_cm'],after['shisui']['feet_width_cm']],
 ['Cabeça (graus)',after['mannequin']['head_deg'],before['shisui']['head_deg'],after['shisui']['head_deg']],
 ['Mão na malha (cm)',quinn['mesh_wrist_to_tip_cm'],model['hand_wrist_to_mesh_tip_cm'],model['hand_wrist_to_mesh_tip_cm']]]
report={'pelvis_ratio':ratio,'table':table,'spine_degrees':{'mannequin':after['mannequin']['spine_deg'],'before':before['shisui']['spine_deg'],'after':after['shisui']['spine_deg']},'knee_degrees':{'mannequin':after['mannequin']['knees_deg'],'after':after['shisui']['knees_deg']},'hand_mesh':model,'animation_assets_verified':len(animation_checks),'animation_checks':animation_checks,'other_workspace_changes_observed':other_changes,'motion_checks':[{'case':r['case'],'speed':r['speed'],'source_head_degrees':r['source']['head_deg'],'target_head_degrees':r['target']['head_deg']} for r in video['rows']],'limitations':['Feet spacing follows the model proportions: 32.33 cm versus mannequin 35.63 cm; no limb lengths or mesh geometry changed.','Head angle is the head-bone orientation delta relative to the mesh reference pose, not a clinical facial landmark.','The face skin estimate excludes the headband; the visible facial height already exceeds the hand length.']}
(OUT/'verification.json').write_text(json.dumps(report,indent=2))
ff=ROOT/'Saved/VisualLab/Tools/imageio_ffmpeg/binaries/ffmpeg-win-x86_64-v7.1.exe'
shutil.copy2('C:/Windows/Fonts/arial.ttf',OUT/'arial.ttf')
def render(args):subprocess.run([str(ff),'-y','-v','error',*args],cwd=OUT,check=True)
for view,title,left,right in [('front','Frente','Shisui','Manequim'),('side','Lado','Manequim','Shisui'),('back','Costas','Manequim','Shisui')]:
    vf=f"eq=gamma=1.12,drawbox=x=0:y=0:w=iw:h=90:color=black@0.85:t=fill,drawtext=fontfile=arial.ttf:text='{title} - mesmo clipe original em 0.5 s':fontcolor=white:fontsize=25:x=24:y=16,drawtext=fontfile=arial.ttf:text='{left}':fontcolor=white:fontsize=23:x=270:y=54,drawtext=fontfile=arial.ttf:text='{right}':fontcolor=white:fontsize=23:x=870:y=54"
    render(['-i',f'after_{view}.png','-vf',vf,f'Comparacao_{view}.png'])
render(['-framerate','16','-i','video_frames/%04d.png','-frames:v','96','-vf',"eq=gamma=1.12,drawbox=x=0:y=0:w=iw:h=70:color=black@0.85:t=fill,drawtext=fontfile=arial.ttf:text='Manequim':fontcolor=white:fontsize=22:x=190:y=40,drawtext=fontfile=arial.ttf:text='Shisui':fontcolor=white:fontsize=22:x=665:y=40,drawtext=fontfile=arial.ttf:text='Trote de combate - 9 m/s':fontcolor=white:fontsize=23:x=24:y=9:enable='lt(t,3)',drawtext=fontfile=arial.ttf:text='Corrida ninja - 14 m/s':fontcolor=white:fontsize=23:x=24:y=9:enable='gte(t,3)'",'-c:v','libx264','-preset','fast','-crf','18','-pix_fmt','yuv420p','-movflags','+faststart','Comparacao_movimento.mp4'])
render(['-i','Comparacao_movimento.mp4','-f','null','-'])
decode=subprocess.run([str(ff),'-hide_banner','-xerror','-i','Comparacao_movimento.mp4','-f','null','-'],cwd=OUT,capture_output=True,text=True,check=True)
decoded_frames=int(re.findall(r'frame=\s*(\d+)',decode.stderr)[-1]);assert decoded_frames==96,decoded_frames
rows=''.join('<tr><td>'+html.escape(r[0])+'</td>'+''.join(f'<td>{x:.2f}</td>' for x in r[1:])+'</tr>' for r in table)
page='''<!doctype html><meta charset="utf-8"><title>Comparação do retarget Shisui</title><style>body{font:16px system-ui;max-width:1200px;margin:32px auto;padding:0 20px;background:#17191c;color:#eee}table{border-collapse:collapse;width:100%;margin:20px 0}td,th{padding:10px;border-bottom:1px solid #555;text-align:left}img,video{width:100%;margin:10px 0 22px}p{line-height:1.5}</style><h1>Shisui e manequim — retarget corrigido</h1><p>Somente IK_Shisui e RTG_Manequim_Para_Shisui foram alterados. A comparação parada usa CharaSelect_Idle original, congelada em 0,5 s nos dois modelos.</p><table><tr><th>Medida</th><th>Manequim</th><th>Shisui antes</th><th>Shisui depois</th></tr>'''+rows+'''</table><p>A pelve usa a proporção real das malhas (95 / 98,6932 = 0,962579), aplicada uma vez pela Unreal. O espaçamento dos pés conserva as proporções do modelo; joelhos diferem no máximo 0,22°. Coluna: 6,476° no manequim e 6,472° no Shisui.</p><p>Ângulo da cabeça: rotação relativa à referência de cada malha. Comprimento da mão: extensão dos vértices do punho à ponta, em repouso; não é a distância óssea entre articulações. Ossos e dedos com escala 1. Mão Shisui 16,18 cm, rosto visível estimado 17,97 cm; a estimativa anterior de 14,31 cm cortava o queixo. Não foi reduzida a malha.</p><p>Auto Align mais compensação manual da coluna/crânio; translação apenas na pelve; IK apenas nos pés. Guarda, trote, corrida ninja e Tigre comparados no retarget em tempo real. Os clipes de jutsu já exportados permaneceram intactos.</p><h2>Frente</h2><img src="Comparacao_front.png"><h2>Lado</h2><img src="Comparacao_side.png"><h2>Costas</h2><img src="Comparacao_back.png"><h2>Trote e corrida ninja</h2><video controls src="Comparacao_movimento.mp4"></video>'''
(OUT/'Comparacao.html').write_text(page,encoding='utf-8')
(OUT/'Medidas.md').write_text('| Medida | Manequim | Shisui antes | Shisui depois |\n|---|---:|---:|---:|\n'+'\n'.join('| '+r[0]+' | '+' | '.join(f'{x:.2f}' for x in r[1:])+' |' for r in table)+'\n\nÂngulo relativo à pose de referência da malha; comprimento da mão medido nos vértices em repouso. Pelve proporcional à razão 0,962579. Espaçamento dos pés conserva as proporções do modelo.\n',encoding='utf-8')
print('DELIVERED',json.dumps({'table':table,'animations_preserved':len(animation_checks),'other_workspace_changes':len(other_changes),'video_frames':video['frames']},ensure_ascii=False))
