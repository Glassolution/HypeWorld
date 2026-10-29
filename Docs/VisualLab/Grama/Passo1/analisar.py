from pathlib import Path
import csv,json,math,statistics,re,shutil
ROOT=Path(__file__).resolve().parents[4]
OUT=Path(__file__).parent
CSV=ROOT/'Saved/Profiling/CSV'
METRICS=['FrameTime','GPUTime','GameThreadTime','RenderThreadTime','GPU/Basepass','GPU/ShadowDepths','GPU/TemporalSuperResolution','GPU/Postprocessing','GPU/CustomDepth','GPU/SlateUI']
def quantile(values,p):
    a=sorted(values);i=(len(a)-1)*p;j=int(i);return a[j]+(a[min(j+1,len(a)-1)]-a[j])*(i-j)
def read(p):
    rows=[]
    for r in csv.DictReader(p.open(encoding='utf-8-sig')):
        try:
            if float(r['FrameTime'])>0 and float(r['GPUTime'])>0:rows.append({k:float(r[k]) for k in METRICS})
        except (ValueError,TypeError,KeyError):pass
    return rows
def summary(rows):
    result={k:{'median':statistics.median(r[k] for r in rows),'p95':quantile([r[k] for r in rows],.95),'mean':statistics.mean(r[k] for r in rows)} for k in METRICS}
    ft=[r['FrameTime'] for r in rows]
    result.update(frames=len(rows),duration_s=sum(ft)/1000,fps=1000/statistics.mean(ft),low1_fps=1000/statistics.mean(sorted(ft,reverse=True)[:max(1,math.ceil(len(ft)*.01))]))
    return result
report={'method':'3 alternating 30-second stationary captures per density; no screenshot inside profile intervals; no outlier removal. FPS=1000/mean frame ms; 1% low=1000/mean slowest 1% frames.','runs':{},'groups':{}}
for density in ['40','100']:
    allrows=[]
    for i in range(1,4):
        name=f'grass_{density}_r{i}';p=CSV/(name+'.csv');rows=read(p)
        report['runs'][name]=summary(rows);allrows+=rows
        shutil.copy2(p,OUT/p.name)
    report['groups'][density]=summary(allrows)
a=report['groups']['40'];b=report['groups']['100']
report['delta_100_minus_40_ms']={k:b[k]['median']-a[k]['median'] for k in METRICS}
report['over_0_5ms']={k:v for k,v in report['delta_100_minus_40_ms'].items() if v>.5}
(OUT/'desempenho.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'groups':report['groups'],'delta':report['delta_100_minus_40_ms'],'runs':{k:{'GPU_ms':v['GPUTime']['median'],'GPU_p95':v['GPUTime']['p95'],'FPS':v['fps'],'low1':v['low1_fps'],'seconds':v['duration_s']} for k,v in report['runs'].items()}},indent=2))
