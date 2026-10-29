from pathlib import Path
import json
P=Path(__file__).resolve().parent
labels={'ao_atual':'AO atual','ao_leve':'AO menos amostras','ao_off':'AO desligado (diagnóstico)','controle':'Controle vegetação','arbustos_material':'Arbustos: material','arbustos_contact':'Arbustos: material + contact','copas_opacas':'Uma copa opaca','tsr_q1':'TSR Q1','fsr_native':'FSR Native AA','combinacao':'Combinação candidata'}
d=json.loads((P/'desempenho.json').read_text())
rows=''
for n,v in d.items():
    s=v['aggregate'];m=s['medians'];rows+=f'<tr><td>{labels[n]}</td><td>{s["fps"]:.2f}</td><td>{s["low1"]:.2f}</td><td>{m["GPUTime"]:.2f}</td><td>{m.get("GPU/ShadowDepths",0):.2f}</td><td>{m.get("GPU/Basepass",0)+m.get("GPU/NaniteBasePass",0):.2f}</td><td>{m["GPUTime"]-16.6:.2f}</td></tr>'
opts=''.join(f'<option value="{n}">{labels[n]}</option>' for n in d if (P/(n+'.png')).exists())
html='''<!doctype html><html lang="pt-BR"><meta charset="utf-8"><title>VisualLab — AO, vegetação e Native AA</title><style>body{font:16px system-ui;background:#12171d;color:#eef3f7;margin:24px auto;padding:0 20px;max-width:1800px}p{color:#bac7d3}a{color:#9bd8ff}select,button{font:inherit;background:#263440;color:white;padding:10px}section{display:grid;grid-template-columns:1fr 1fr;gap:16px}img,video{width:100%}th,td{padding:8px 14px;text-align:left;border-bottom:1px solid #42515f}table{border-collapse:collapse} @media(max-width:800px){section{grid-template-columns:1fr}}</style><h1>AO, vegetação e Native AA — 1920×1080, 100%</h1><p>Câmera fixa comum. 3×30 s por candidato. Base: Irradiance Volume, sombras 3×1536, cortes pequenos. Prints reais sem retoque. Experimentos isolados; deltas entre sessões exigem o controle correspondente. BasePass abaixo soma o passe convencional e o Nanite.</p><table><tr><th>Opção</th><th>FPS</th><th>1% low</th><th>GPU ms</th><th>ShadowDepths ms</th><th>BasePass ms</th><th>Falta p/16,6 ms</th></tr>ROWS</table><h2>Comparar renders</h2><section>'''.replace('ROWS',rows)
for i in range(2):html+=f'<article><select id="s{i}">{opts}</select><a id="a{i}"><img id="i{i}"></a></article>'
html+='''</section><h2>Referência: copas Zenkai</h2><img style="max-width:1100px" src="zenkai_copas.png"><h2>Referência: iluminação</h2><img style="max-width:1100px" src="../Baseline/Referencias/zenkai_4.png">'''
if all((P/(n+'.mp4')).exists() for n in ['tsr_q1','fsr_native']):
    html+='''<h2>Grama e corrida — mesma câmera e input</h2><p>Vídeos nativos com simulação fixa a 30 Hz; FPS de desempenho vem dos CSVs, não da reprodução.</p><button onclick="[v0,v1].forEach(v=>{v.currentTime=0;v.play()})">Reproduzir juntos</button><button onclick="[v0,v1].forEach(v=>v.pause())">Pausar</button><section><article><h3>TSR Q1</h3><video id="v0" src="tsr_q1.mp4" controls muted loop></video></article><article><h3>FSR Native AA</h3><video id="v1" src="fsr_native.mp4" controls muted loop></video></article></section>'''
html+='''<p><a href="Relatorio.md">Relatório</a> · <a href="Detalhes.md">Detalhes técnicos</a> · <a href="desempenho.json">Medições completas</a></p><script>function set(i){let n=document.getElementById('s'+i).value;document.getElementById('i'+i).src=n+'.png';document.getElementById('a'+i).href=n+'.png'}s0.value='tsr_q1';s1.value='fsr_native';[s0,s1].forEach((s,i)=>{s.onchange=()=>set(i);set(i)})</script></html>'''
(P/'comparar.html').write_text(html,encoding='utf-8')

