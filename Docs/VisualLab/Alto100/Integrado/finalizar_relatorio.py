from pathlib import Path
import json
P=Path(__file__).resolve().parent
d=json.loads((P/'desempenho.json').read_text())
labels={'ao_atual':'AO atual — controle','ao_leve':'AO menos amostras','ao_off':'AO desligado (diagnóstico)','controle':'Arbustos — controle','arbustos_material':'Arbustos: material','arbustos_contact':'Arbustos: material + contact','tsr_q1':'TSR Q1 — controle retomada','copas_opacas':'Uma copa opaca','fsr_native':'FSR Native AA','combinacao':'Combinação candidata com FSR'}
text='# Resultado dos experimentos\n\n1920×1080, 100%, mesma câmera fixa, Alto com Irradiance Volume e sombras provisórias. Cada opção: 3×30 s. Vídeos cancelados a pedido do usuário.\n\n| Opção | FPS | 1% low | GPU | Delta GPU vs controle da sessão |\n|---|---:|---:|---:|---:|\n'
for n in labels:
    s=d[n]['aggregate'];ref='ao_atual' if n.startswith('ao_') else ('controle' if n in ['controle','arbustos_material','arbustos_contact'] else 'tsr_q1')
    delta=s['medians']['GPUTime']-d[ref]['aggregate']['medians']['GPUTime']
    text+=f'| {labels[n]} | {s["fps"]:.2f} | {s["low1"]:.2f} | {s["medians"]["GPUTime"]:.2f} ms | {delta:+.2f} ms |\n'
text+='''
[Prints reais lado a lado e referências](comparar.html).

**AO:** manter o atual. Com Irradiance Volume, o SSAO de 2,09 ms não executa: há ShortRangeAO em meia resolução, snapshot 0,24 ms. Menos amostras reduz o passe para 0,13 ms, sem ganho total mensurável; desligar tudo economiza só 0,27 ms. AO assado em assets maiores não foi produzido neste experimento.

**Arbustos:** cópias com degradê inferior e escurecimento central recuperam volume discretamente, sem custo total mensurável. Material + contact ficou equivalente em desempenho; a diferença visual é pequena neste enquadramento. Contact configurado em 12 cm e intensidade 0,5, com grama excluída. Candidato para aprovação visual.

**Copa:** protótipo opaco de uma árvore. ShadowDepths 3,93 → 3,80 ms; BasePass convencional + Nanite 3,53 → 3,55 ms. GPU total 30,32 → 29,01 ms, mas o ganho total não pode ser atribuído todo ao alpha: há mudança de malha/componente, cobertura e variação de outros passes. Visual escuro e de volumes sobrepostos, distante do Zenkai: não recomendo adoção. Nenhuma troca no mapa original.

**FSR Native AA:** modo 0 e resolução 100% confirmados; provider FSR 3.1.5, sem Frame Generation. GPU total economiza 2,42 ms na sessão. Os passes de AA indicam TSR 5,74 ms contra FSR dispatch 3,83 ms mais preparação ~0,76 ms; não atribuir todo o delta do frame ao AA. Os prints não validam rastro ou cintilação em movimento. Manter TSR Q1 como escolha provisória até aprovação de qualidade do FSR.

**Combinação medida:** Irradiance Volume + sombras aprovadas provisoriamente + AO atual + material/contact nos arbustos + FSR Native AA, com copas originais. 27,78 ms, 35,25 FPS, 1% low 30,28. Faltam **11,18 ms** para 16,6 ms; 60 FPS ainda não atingido. Trata-se de uma combinação candidata, não de adoção aprovada. O acumulado é medido diretamente; não é a soma dos ganhos isolados. As decisões já aprovadas provisoriamente com TSR tiveram controle de 30,32 ms nesta sessão; a sessão anterior marcou 27,93 ms. Essa variação impede comparar deltas entre sessões como se fossem um único teste.

Mudanças maiores que 0,5 ms: copa (−1,31 ms total, visual reprovado como candidato), FSR (−2,42 ms) e combinação (−2,54 ms). Nenhum acréscimo isolado acima de 0,5 ms foi encontrado.

CSVs, estados e snapshots em arquivos da pasta. Prints 1920×1080 verificados; 30 capturas de ~30 s validadas. Sem commit ou adoção global.
'''
(P/'Relatorio.md').write_text(text,encoding='utf-8')
