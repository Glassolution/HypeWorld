from pathlib import Path
import subprocess,json,hashlib
from PIL import Image
OUT=Path(__file__).resolve().parent;ROOT=OUT.parents[3]
FRAMES=ROOT/'Saved/VisualLab/AAFrames'
FFMPEG=ROOT/'Saved/VisualLab/Tools/imageio_ffmpeg/binaries/ffmpeg-win-x86_64-v7.1.exe'
manifest=json.loads((OUT/'videos_verificados.json').read_text()) if (OUT/'videos_verificados.json').exists() else {}
for name in ['tsr_atual','tsr_flickering0','tsr_q1','taa']:
    if name in manifest:continue
    files=sorted((FRAMES/name).glob('*.png'))
    if len(files)!=300:continue
    for p in files:
        with Image.open(p) as im:assert im.size==(1920,1080),(p,im.size)
    output=OUT/(name+'.mp4')
    args=[str(FFMPEG),'-y','-v','error','-framerate','30','-i',str(FRAMES/name/'%04d.png'),'-frames:v','300','-c:v','libx264','-preset','fast','-crf','12','-pix_fmt','yuv420p','-movflags','+faststart',str(output)]
    subprocess.run(args,check=True)
    # Decode validation: verify every output frame can be read.
    subprocess.run([str(FFMPEG),'-v','error','-i',str(output),'-f','null','-'],check=True)
    manifest[name]={'frames':len(files),'size':[1920,1080],'simulation_fps':30,'duration_s':10,'bytes':output.stat().st_size,'sha256':hashlib.sha256(output.read_bytes()).hexdigest(),'first_frame':str(files[0]),'last_frame':str(files[-1])}
    print(name,output.stat().st_size,flush=True)
    (OUT/'videos_verificados.json').write_text(json.dumps(manifest,indent=2))
(OUT/'videos_verificados.json').write_text(json.dumps(manifest,indent=2))
