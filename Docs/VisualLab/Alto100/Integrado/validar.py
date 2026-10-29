from pathlib import Path
import json
from PIL import Image
P=Path(__file__).resolve().parent
d=json.loads((P/'desempenho.json').read_text());report={}
for n,v in d.items():
    assert len(v['runs'])==3,(n,len(v['runs']))
    durations=[x['seconds'] for x in v['runs'].values()]
    assert all(29<t<31 for t in durations),(n,durations)
    size=list(Image.open(P/(n+'.png')).size);assert size==[1920,1080],(n,size)
    report[n]={'size':size,'durations':durations,'runs':3}
(P/'validacao.json').write_text(json.dumps(report,indent=2))
print('validated',len(report),'options')
