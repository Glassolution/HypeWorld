from pathlib import Path
import subprocess,json,hashlib,shutil
from PIL import Image
OUT=Path(__file__).resolve().parent;ROOT=OUT.parents[3];FRAMES=ROOT/'Saved/VisualLab/AA1bFrames'
FF=ROOT/'Saved/VisualLab/Tools/imageio_ffmpeg/binaries/ffmpeg-win-x86_64-v7.1.exe'
meta=OUT/'videos_verificados.json';manifest=json.loads(meta.read_text()) if meta.exists() else {}
for folder in FRAMES.iterdir():
    name=folder.name
    if name in manifest:continue
    if not (folder/'COMPLETE.json').exists():continue
    expected=json.loads((folder/'COMPLETE.json').read_text())['frames']
    files=sorted(folder.glob('*.png'));assert len(files)==expected
    for p in files:
        with Image.open(p) as im:assert im.size==(1920,1080)
    output=OUT/(name+'.mp4')
    subprocess.run([str(FF),'-y','-v','error','-framerate','30','-i',str(folder/'%04d.png'),'-frames:v',str(expected),'-c:v','libx264','-preset','fast','-crf','12','-pix_fmt','yuv420p','-movflags','+faststart',str(output)],check=True)
    subprocess.run([str(FF),'-v','error','-i',str(output),'-f','null','-'],check=True)
    shutil.copy2(folder/'0030.png',OUT/(name+'.png'))
    manifest[name]={'frames':expected,'duration':expected/30,'resolution':[1920,1080],'sha256':hashlib.sha256(output.read_bytes()).hexdigest()}
    meta.write_text(json.dumps(manifest,indent=2));print(name,flush=True)
