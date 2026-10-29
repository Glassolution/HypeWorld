import csv,json,statistics,math,shutil
from pathlib import Path
OUT=Path(__file__).resolve().parent;ROOT=OUT.parents[3];CSV=ROOT/'Saved/Profiling/CSV'
def read(path):
    rows=[]
    for r in csv.DictReader(path.open(encoding='utf-8-sig')):
        try:
            if float(r['FrameTime'])<=0 or float(r['GPUTime'])<=0:continue
        except (ValueError,TypeError,KeyError):continue
        row={}
        for k,v in r.items():
            if k and (k.startswith(('GPU/','RHI/','DrawCall/','GPUMem/')) or k in ['FrameTime','GPUTime','GameThreadTime','RenderThreadTime']):
                try:row[k]=float(v)
                except (ValueError,TypeError):pass
        rows.append(row)
    return rows
def summary(rows):
    ft=[r['FrameTime'] for r in rows];count=math.ceil(len(ft)*.01)
    return {'frames':len(rows),'seconds':sum(ft)/1000,'fps':1000/statistics.mean(ft),'low1':1000/statistics.mean(sorted(ft,reverse=True)[:count]),'medians':{k:statistics.median([r[k] for r in rows if k in r]) for k in rows[0]}}
result={}
for name in ['tsr_atual','tsr_flickering0','tsr_q1','taa']:
    rows=[];runs={}
    for i in range(1,4):
        path=CSV/f'aa_{name}_r{i}.csv'
        if not path.exists():continue
        try:sample=read(path)
        except PermissionError:continue
        if not sample:continue
        runs[str(i)]=summary(sample);rows.extend(sample);shutil.copy2(path,OUT/path.name)
    if rows:result[name]={'runs':runs,'aggregate':summary(rows)}
(OUT/'desempenho.json').write_text(json.dumps(result,indent=2))
for name,data in result.items():
    s=data['aggregate'];m=s['medians']
    print(name,'runs',len(data['runs']),'FPS',round(s['fps'],2),'low1',round(s['low1'],2),'GPU',round(m['GPUTime'],3),'AA', {k:round(v,3) for k,v in m.items() if 'Temporal' in k or 'Postprocess' in k})
