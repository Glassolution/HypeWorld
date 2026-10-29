from audit import *
import statistics,shutil
done=json.loads((OUT/'capture_done.json').read_text());rows=json.loads((OUT/'telemetry.json').read_text())
assert len(rows)==done['frames']
summary=[]
for name,_,_,_,count in done['cases']:
    group=[r for r in rows if r['case']==name]
    assert len(group)==count
    speed=statistics.median(r['speed'] for r in group)
    expected=1400 if name=='corrida_ninja' else (900 if name.startswith('trote_') else 0)
    assert abs(speed-expected)<1,(name,speed,expected)
    assert all('SKM_Quinn_Simple' in r['visible_meshes'] and 'SK_Shisui' not in r['visible_meshes'] for r in group)
    summary.append({'case':name,'speed_cm_s':speed,'frames':len(group),'direction':statistics.median(r['direction'] for r in group),'actor_yaw':statistics.median(r['actor_yaw'] for r in group)})
protected=json.loads((OUT/'protected_before.json').read_text())
changed=[r['path'] for r in protected if not (ROOT/r['path']).is_file() or hashlib.sha256((ROOT/r['path']).read_bytes()).hexdigest()!=r['sha256']]
assert not changed,changed
baseline=json.loads((OUT/'baseline_check.json').read_text())
assert all((ROOT/r['path']).read_bytes()==content('f091ba9',r['path']) for r in baseline if not r['path'].endswith('/ABP_Ninja.uasset'))
for p in ['Source/HypeWorld/Ninja/NinjaMovementComponent.cpp','Source/HypeWorld/Ninja/NinjaMovementComponent.h','Source/HypeWorld/Ninja/NinjaAnimInstance.h']:
    assert (ROOT/p).read_bytes().replace(b'\r\n',b'\n')==content('f091ba9',p).replace(b'\r\n',b'\n'),p
report={'build':'Succeeded','baseline':'f091ba9','unchanged_protected_files':len(protected),'unchanged_animation_assets':len(baseline)-1,'summary':summary,'limitation':'The selected git baseline has an upper-body C++ bone mask at spine_01, not a Layered Blend per Bone graph node. No new animation clips or retarget changes were authored.'}
(OUT/'verification.json').write_text(json.dumps(report,indent=2))
ff=ROOT/'Saved/VisualLab/Tools/imageio_ffmpeg/binaries/ffmpeg-win-x86_64-v7.1.exe'
shutil.copy2('C:/Windows/Fonts/arial.ttf',OUT/'arial.ttf')
labels={'parado':'Parado','guarda_frente':'Guarda - frente','guarda_lado':'Guarda - lado','guarda_costas':'Guarda - costas','trote_frente':'Trote de combate - frente','trote_direita':'Trote de combate - direita','trote_esquerda':'Trote de combate - esquerda','trote_tras':'Trote de combate - tras','corrida_ninja':'Corrida ninja'}
filters=['eq=gamma=1.4','drawbox=x=0:y=0:w=iw:h=72:color=black@0.8:t=fill']
start=0
for r in summary:
    end=start+r['frames'];label=labels[r['case']]+f"  |  {r['speed_cm_s']/100:.1f} m/s"
    filters.append(f"drawtext=fontfile=arial.ttf:text='{label}':fontcolor=white:fontsize=27:x=24:y=16:enable='gte(n,{start})*lt(n,{end})'")
    start=end
filters+=['trim=start_frame=24','setpts=PTS-STARTPTS']
subprocess.run([str(ff),'-y','-v','error','-framerate','24','-i','frames/%04d.png','-frames:v',str(done['frames']-24),'-vf',','.join(filters),'-c:v','libx264','-preset','fast','-crf','18','-pix_fmt','yuv420p','-movflags','+faststart','Etapa1_Manequim.mp4'],cwd=OUT,check=True)
subprocess.run([str(ff),'-v','error','-i',str(OUT/'Etapa1_Manequim.mp4'),'-f','null','-'],check=True)
print(json.dumps(report,indent=2))
(OUT/'Relatorio.md').write_text('Base aprovada para esta restauração: `f091ba9`, com a `A_Pose_Prontidao` original religada.\nRestaurado: `ABP_Ninja`; `NinjaAnimInstance.cpp` recomposto da base com a reconexão autorizada; manequim visível no BP.\nConferidos sem diferenças: 214 assets de animação, incluindo pose, blend spaces, montages, esquivas e IK Rigs/Retargeters; NinjaMovementComponent e header da AnimInstance.\nRemovidos com backup: `NinjaBodyAnimInstance.cpp/.h` e `A_Pose_Prontidao_Base`; `ABP_Shisui` agora herda de AnimInstance, sem torção procedural.\nValidação: compilação passou; trote em quatro direções a 9 m/s, corrida ninja a 14 m/s, guarda parada e manequim visível em todos os quadros.\nPreservados por hash: '+str(len(protected))+' arquivos, incluindo mapa, jutsus e VFX; nenhum commit criado.\nRessalva: a base escolhida usa máscara do tronco em C++ desde spine_01; não contém nó Layered Blend per Bone. Esquivas e Tigre conferidos por integridade, sem novo teste de execução.\nVídeo: [Etapa1_Manequim.mp4](Etapa1_Manequim.mp4). Etapa 2 não iniciada; aguarda aprovação.\n',encoding='utf-8')
