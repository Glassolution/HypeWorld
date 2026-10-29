import json,csv,statistics,math,shutil,subprocess,hashlib
from pathlib import Path
from PIL import Image,ImageDraw,ImageChops,ImageStat
P=Path(__file__).resolve().parent;ROOT=P.parents[2];FR=ROOT/'Saved/VisualLab/Alto60Frames_v2'
FF=ROOT/'Saved/VisualLab/Tools/imageio_ffmpeg/binaries/ffmpeg-win-x86_64-v7.1.exe'
exec((P.parent/'Alto100/AA/analisar_aa.py').read_text().split('result={}')[0].replace('OUT=Path(__file__).resolve().parent;ROOT=OUT.parents[3];CSV=ROOT/','OUT=Path(__file__).resolve().parent;ROOT=OUT.parents[2];CSV=ROOT/'))
allrows=[];runs={}
for i in range(1,4):
    f=CSV/f'alto60_validado_r{i}.csv'
    if not f.exists():continue
    sample=read(f)
    if sample:
        runs[str(i)]=summary(sample);allrows+=sample;shutil.copy2(f,P/f.name)
if allrows:
    result={'runs':runs,'aggregate':summary(allrows)}
    (P/'desempenho.json').write_text(json.dumps(result,indent=2))
    s=result['aggregate'];print('runs',len(runs),'fps',round(s['fps'],2),'low',round(s['low1'],2),'gpu',round(s['medians']['GPUTime'],3),flush=True)
manifest_path=P/'videos.json';manifest=json.loads(manifest_path.read_text()) if manifest_path.exists() else {}
if not FR.exists():raise SystemExit()
for folder in sorted(FR.glob('*/*')):
    if not (folder/'COMPLETE.json').exists():continue
    name=folder.parent.name+'_'+folder.name
    if name in manifest:continue
    files=sorted(folder.glob('*.png'));assert len(files)==360,(name,len(files))
    assert all(Image.open(f).size==(1920,1080) for f in files)
    dst=P/(name+'.mp4')
    subprocess.run([str(FF),'-y','-v','error','-framerate','60','-i',str(folder/'%04d.png'),'-frames:v','360','-c:v','libx264','-preset','fast','-crf','10','-pix_fmt','yuv420p','-movflags','+faststart',str(dst)],check=True)
    subprocess.run([str(FF),'-v','error','-i',str(dst),'-f','null','-'],check=True)
    manifest[name]={'fps':60,'frames':360,'seconds':6,'size':[1920,1080],'sha256':hashlib.sha256(dst.read_bytes()).hexdigest()}
    manifest_path.write_text(json.dumps(manifest,indent=2));print('encoded',name,flush=True)

# Lossless inspection sheets: same consecutive frame numbers on both profiles.
for clip in ['frente_viradas','strafe_re','parada']:
    if not all((FR/n/clip/'COMPLETE.json').exists() for n in ['base','alto60']):continue
    sheet=Image.new('RGB',(1120,4*360),(18,22,27));draw=ImageDraw.Draw(sheet)
    indices=[82,86,90,94] if clip=='frente_viradas' else ([178,182,186,190] if clip=='strafe_re' else [148,152,156,160])
    for row,index in enumerate(indices):
        for col,profile in enumerate(['base','alto60']):
            im=Image.open(FR/profile/clip/f'{index:04d}.png').convert('RGB')
            crop=im.crop((680,180,1240,900));crop=crop.resize((560,336))
            sheet.paste(crop,(col*560,row*360+24));draw.text((col*560+8,row*360+5),f'{profile} {clip} frame {index}',fill='white')
    sheet.save(P/(clip+'_inspecao.png'))

