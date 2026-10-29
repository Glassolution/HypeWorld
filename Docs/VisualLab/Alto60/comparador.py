from pathlib import Path
import json
P=Path(__file__).resolve().parent
d=json.loads((P/'desempenho.json').read_text())['aggregate'];m=d['medians']
html=f'''<!doctype html><html lang="pt-BR"><meta charset="utf-8"><title>Alto-60 — 1080p nativo</title><style>body{{font:16px system-ui;background:#12171d;color:#eef3f7;max-width:1800px;margin:24px auto;padding:0 20px}}section{{display:grid;grid-template-columns:1fr 1fr;gap:16px}}video,img{{width:100%}}button,select{{font:inherit;background:#263440;color:white;padding:10px}}a{{color:#9bd8ff}}p{{color:#bac7d3}}@media(max-width:800px){{section{{grid-template-columns:1fr}}}}</style><h1>Alto-60 — 1920×1080, 100%</h1><p>Base aprovada no commit 5130bb0. Perfil agressivo experimental. 3×30 s, GTX 1650, -game, câmera de referência fixa.</p><p><b>{d['fps']:.2f} FPS · 1% low {d['low1']:.2f} · GPU {m['GPUTime']:.2f} ms · faltam {m['GPUTime']-16.6:.2f} ms para 16,6 ms.</b></p><p>Vídeos 1920×1080 a 60 quadros/s, CRF 10. Cada quadro foi renderizado com passo de simulação de 1/60 s; isso não significa que o jogo alcançou 60 FPS em tempo real. Sem interpolação ou quadros repetidos para simular desempenho.</p>'''
for ix,(clip,title) in enumerate([('frente_viradas','Frente e viradas bruscas'),('strafe_re','Strafe e marcha à ré'),('camera_grama','Câmera rápida e grama com vento'),('parada','Corrida e parada brusca')]):
    html+=f'<h2>{title}</h2><button onclick="sync({ix})">Reiniciar e reproduzir juntos</button><button onclick="pause({ix})">Pausar os dois</button><section>'
    for name,label in [('base','Base aprovada · TSR Q1'),('alto60','Alto-60 · FSR Native AA')]:
        html+=f'<article><h3>{label}</h3><video id="v{ix}_{name}" src="{name}_{clip}.mp4" controls muted loop preload="metadata"></video></article>'
    html+='</section>'
html+='<h2>Quadros extraídos para inspeção</h2><p>Recortes dos renders originais, sem retoque. As imagens destacam os trechos mais problemáticos; a análise distingue deformação de animação de resíduo temporal.</p>'
for name in ['comparacao_1.png','comparacao_2.png','comparacao_3.png']:
    if (P/name).exists():html+=f'<a href="{name}"><img src="{name}"></a>'
html+='<h2>Print da câmera de benchmark</h2><img src="alto60.png"><p><a href="Detalhes.md">Detalhes e limitações</a> · <a href="desempenho.json">Medições completas</a> · <a href="analise_2880_quadros.json">Varredura dos quadros</a></p>'
html+='''<script>function sync(i){['base','alto60'].forEach(n=>{let v=document.getElementById('v'+i+'_'+n);v.currentTime=0;v.play()})}function pause(i){['base','alto60'].forEach(n=>document.getElementById('v'+i+'_'+n).pause())}</script></html>'''
(P/'comparar.html').write_text(html,encoding='utf-8')
