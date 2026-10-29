from pathlib import Path
exec((Path(__file__).resolve().parent.parent/'AA/analisar_aa.py').read_text().split('result={}')[0])
results={}
for n in ['baseline','rochas','arvores','arbustos_flores','personagens','outros']:
    p=CSV/f'shadowdiag_{n}.csv'
    if not p.exists():continue
    try:rows=read(p)
    except PermissionError:continue
    if not rows:continue
    results[n]=summary(rows);shutil.copy2(p,OUT/p.name)
(OUT/'triagem_resultados.json').write_text(json.dumps(results,indent=2))
for n,s in results.items():
    m=s['medians'];print(n,'FPS',round(s['fps'],2),'1low',round(s['low1'],2),'GPU',round(m['GPUTime'],3),'ShadowDepths',round(m.get('GPU/ShadowDepths',0),3))
