from pathlib import Path
source=(Path(__file__).resolve().parent.parent/'AA1b/codificar.py').read_text()
exec(source.replace('AA1bFrames','IntegradoFrames').replace("OUT/(name+'.png')","OUT/(name+'_video_frame.png')"))
