from pathlib import Path
import json
P=Path(__file__).resolve().parent
labels={'lumen_high':'Lumen High','irradiance_volume':'Lumen Irradiance Volume','skylight_dfao':'Sky Light + DFAO'}
d=json.loads((P/'desempenho.json').read_text())
rows=''.join(f'<tr><td>{labels[n]}</td><td>{v["aggregate"]["fps"]:.2f}</td><td>{v["aggregate"]["low1"]:.2f}</td><td>{v["aggregate"]["medians"]["GPUTime"]:.2f} ms</td></tr>' for n,v in d.items())
options=''.join(f'<option value="{n}">{label}</option>' for n,label in labels.items())
html='''<!doctype html><html lang="pt-BR"><meta charset="utf-8"><title>GI/AO — 1080p nativo</title><style>body{font:16px system-ui;background:#12171d;color:#eef3f7;margin:24px auto;padding:0 20px;max-width:1800px}p{color:#bac7d3}a{color:#9bd8ff}select{font:inherit;background:#263440;color:white;padding:10px}section{display:grid;grid-template-columns:1fr 1fr;gap:16px}img{width:100%}th,td{padding:8px 20px;text-align:left;border-bottom:1px solid #42515f}table{border-collapse:collapse}@media(max-width:800px){section{grid-template-columns:1fr}}</style><h1>GI/AO — Alto, 1920×1080, 100%</h1><p>TSR Q1 · sombras 3×1536 · cortes pequenos · mesma câmera fixa do bloco de sombras · 3×30 s por opção. Prints renderizados pelo jogo, sem retoque. Luz, exposição, materiais e vento preservados.</p><table><tr><th>Opção</th><th>FPS</th><th>1% low</th><th>GPU</th></tr>ROWS</table><p>Escolha as opções. Clique no print para abrir os 1920×1080 originais.</p><section>'''.replace('ROWS',rows)
for i in range(2):html+=f'<article><select id="s{i}">{options}</select><a id="a{i}"><img id="i{i}"></a></article>'
html+='''</section><h2>Referência 4</h2><img style="max-width:1000px" src="../Baseline/Referencias/zenkai_4.png"><p><a href="Relatorio.md">Relatório</a> · <a href="Detalhes.md">Protocolo e limitações</a></p><script>function set(i){let n=document.getElementById('s'+i).value;document.getElementById('i'+i).src=n+'.png';document.getElementById('a'+i).href=n+'.png'}s0.value='lumen_high';s1.value='irradiance_volume';[s0,s1].forEach((s,i)=>{s.onchange=()=>set(i);set(i)})</script></html>'''
(P/'comparar.html').write_text(html,encoding='utf-8')
