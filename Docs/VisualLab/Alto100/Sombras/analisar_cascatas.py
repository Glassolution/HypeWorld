from pathlib import Path
exec((Path(__file__).resolve().parent.parent/'AA/analisar_aa.py').read_text().split('result={}')[0])
result={}
for n in ['original','cortes_pequenos','csm3_1536','csm2_2048']:
    rows=[];runs={}
    for i in range(1,4):
        p=CSV/f'shadow_{n}_r{i}.csv'
        if not p.exists():continue
        try:r=read(p)
        except PermissionError:continue
        if not r:continue
        runs[str(i)]=summary(r);rows.extend(r);shutil.copy2(p,OUT/p.name)
    if rows:result[n]={'runs':runs,'aggregate':summary(rows)}
(OUT/'desempenho_cascatas.json').write_text(json.dumps(result,indent=2))
for n,d in result.items():
    s=d['aggregate'];m=s['medians'];print(n,len(d['runs']),round(s['fps'],2),round(s['low1'],2),round(m['GPUTime'],3),'ShadowDepth',round(m.get('GPU/ShadowDepths',0),3),'Projection',round(m.get('GPU/ShadowProjection',0),3))
