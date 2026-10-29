from pathlib import Path
exec((Path(__file__).resolve().parent.parent/'AA/analisar_aa.py').read_text().split('result={}')[0])
result={}
for prefix,names in [('ao',['ao_atual','ao_leve','ao_off']),('veg',['controle','arbustos_material','arbustos_contact']),('final',['tsr_q1','copas_opacas','fsr_native','combinacao'])]:
    for name in names:
        rows=[];runs={}
        for i in range(1,4):
            path=CSV/f'{prefix}_{name}_r{i}.csv'
            if not path.exists():continue
            try:sample=read(path)
            except PermissionError:continue
            if not sample:continue
            runs[str(i)]=summary(sample);rows.extend(sample);shutil.copy2(path,OUT/path.name)
        if rows:result[name]={'runs':runs,'aggregate':summary(rows),'prefix':prefix}
(OUT/'desempenho.json').write_text(json.dumps(result,indent=2))
for name,d in result.items():
    s=d['aggregate'];m=s['medians'];print(name,len(d['runs']),round(s['fps'],2),round(s['low1'],2),round(m['GPUTime'],3),'SD',round(m.get('GPU/ShadowDepths',0),3),'Base',round(m.get('GPU/Basepass',0)+m.get('GPU/NaniteBasePass',0),3))
