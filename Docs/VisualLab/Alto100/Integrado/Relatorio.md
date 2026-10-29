# Resultado dos experimentos

1920×1080, 100%, mesma câmera fixa, Alto com Irradiance Volume e sombras provisórias. Cada opção: 3×30 s. Vídeos cancelados a pedido do usuário.

| Opção | FPS | 1% low | GPU | Delta GPU vs controle da sessão |
|---|---:|---:|---:|---:|
| AO atual — controle | 34.66 | 31.29 | 28.27 ms | +0.00 ms |
| AO menos amostras | 34.57 | 31.27 | 28.31 ms | +0.05 ms |
| AO desligado (diagnóstico) | 34.97 | 31.83 | 28.00 ms | -0.27 ms |
| Arbustos — controle | 34.35 | 30.59 | 28.60 ms | +0.00 ms |
| Arbustos: material | 34.35 | 30.71 | 28.60 ms | -0.00 ms |
| Arbustos: material + contact | 34.41 | 30.34 | 28.57 ms | -0.03 ms |
| TSR Q1 — controle retomada | 32.40 | 21.96 | 30.32 ms | +0.00 ms |
| Uma copa opaca | 33.78 | 27.89 | 29.01 ms | -1.31 ms |
| FSR Native AA | 35.12 | 30.71 | 27.90 ms | -2.42 ms |
| Combinação candidata com FSR | 35.25 | 30.28 | 27.78 ms | -2.54 ms |

[Prints reais lado a lado e referências](comparar.html).

**AO:** manter o atual. Com Irradiance Volume, o SSAO de 2,09 ms não executa: há ShortRangeAO em meia resolução, snapshot 0,24 ms. Menos amostras reduz o passe para 0,13 ms, sem ganho total mensurável; desligar tudo economiza só 0,27 ms. AO assado em assets maiores não foi produzido neste experimento.

**Arbustos:** cópias com degradê inferior e escurecimento central recuperam volume discretamente, sem custo total mensurável. Material + contact ficou equivalente em desempenho; a diferença visual é pequena neste enquadramento. Contact configurado em 12 cm e intensidade 0,5, com grama excluída. Candidato para aprovação visual.

**Copa:** protótipo opaco de uma árvore. ShadowDepths 3,93 → 3,80 ms; BasePass convencional + Nanite 3,53 → 3,55 ms. GPU total 30,32 → 29,01 ms, mas o ganho total não pode ser atribuído todo ao alpha: há mudança de malha/componente, cobertura e variação de outros passes. Visual escuro e de volumes sobrepostos, distante do Zenkai: não recomendo adoção. Nenhuma troca no mapa original.

**FSR Native AA:** modo 0 e resolução 100% confirmados; provider FSR 3.1.5, sem Frame Generation. GPU total economiza 2,42 ms na sessão. Os passes de AA indicam TSR 5,74 ms contra FSR dispatch 3,83 ms mais preparação ~0,76 ms; não atribuir todo o delta do frame ao AA. Os prints não validam rastro ou cintilação em movimento. Manter TSR Q1 como escolha provisória até aprovação de qualidade do FSR.

**Combinação medida:** Irradiance Volume + sombras aprovadas provisoriamente + AO atual + material/contact nos arbustos + FSR Native AA, com copas originais. 27,78 ms, 35,25 FPS, 1% low 30,28. Faltam **11,18 ms** para 16,6 ms; 60 FPS ainda não atingido. Trata-se de uma combinação candidata, não de adoção aprovada. O acumulado é medido diretamente; não é a soma dos ganhos isolados. As decisões já aprovadas provisoriamente com TSR tiveram controle de 30,32 ms nesta sessão; a sessão anterior marcou 27,93 ms. Essa variação impede comparar deltas entre sessões como se fossem um único teste.

Mudanças maiores que 0,5 ms: copa (−1,31 ms total, visual reprovado como candidato), FSR (−2,42 ms) e combinação (−2,54 ms). Nenhum acréscimo isolado acima de 0,5 ms foi encontrado.

CSVs, estados e snapshots em arquivos da pasta. Prints 1920×1080 verificados; 30 capturas de ~30 s validadas. Sem commit ou adoção global.
