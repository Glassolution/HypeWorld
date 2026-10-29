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
def percentile(a,p):
    a=sorted(a);i=(len(a)-1)*p;j=int(i);return a[j]+(a[min(j+1,len(a)-1)]-a[j])*(i-j)
def summarize(rows):
    result={}
    for k in rows[0]:
        a=[r[k] for r in rows if k in r]
        result[k]={'median':statistics.median(a),'mean':statistics.mean(a),'p95':percentile(a,.95)}
    ft=[r['FrameTime'] for r in rows]
    result['summary']={'frames':len(rows),'duration_s':sum(ft)/1000,'fps_mean':1000/statistics.mean(ft),'fps_1percent_low':1000/statistics.mean(sorted(ft,reverse=True)[:math.ceil(len(ft)*.01)]),'frame_p99_ms':percentile(ft,.99),'worst_frame_ms':max(ft),'gpu_gap_to_16_6_ms':result['GPUTime']['median']-16.6,'frame_gap_to_16_6_ms':result['FrameTime']['median']-16.6}
    return result
allrows=[];runs={}
for i in range(1,4):
    p=CSV/f'alto100_r{i}.csv'
    if not p.exists() or p.stat().st_size==0:continue
    rows=read(p);runs[str(i)]=summarize(rows);allrows.extend(rows);shutil.copy2(p,OUT/p.name)
result={'runs':runs,'aggregate':summarize(allrows)}
(OUT/'desempenho.json').write_text(json.dumps(result,indent=2))
print('RUNS',len(runs));print(json.dumps(result['aggregate']['summary'],indent=2))
print(json.dumps({k:round(v['median'],4) for k,v in result['aggregate'].items() if k!='summary' and (v['median']>.01 or k.startswith('GPU/'))},indent=2))
