from pathlib import Path
exec((Path(__file__).resolve().parent.parent/'AA/analisar_aa.py').read_text().split('result={}')[0])
result={}
for name in ['lumen_high','irradiance_volume','skylight_dfao']:
    rows=[];runs={}
    for i in range(1,4):
        path=CSV/f'gi_{name}_r{i}.csv'
        if not path.exists():continue
        try:sample=read(path)
        except PermissionError:continue
        if not sample:continue
        runs[str(i)]=summary(sample);rows.extend(sample);shutil.copy2(path,OUT/path.name)
    if rows:result[name]={'runs':runs,'aggregate':summary(rows)}
(OUT/'desempenho.json').write_text(json.dumps(result,indent=2))
for name,d in result.items():
    s=d['aggregate'];print(name,len(d['runs']),round(s['fps'],2),round(s['low1'],2),round(s['medians']['GPUTime'],3))
