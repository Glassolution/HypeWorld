# Bloco 1 — AA a 100%

GTX 1650, Alto, 1080p nativo, mesmo percurso de câmera, três capturas de 30 s por opção. Sem commit ou adoção global.

| Opção | FPS médio | 1% low | GPU | Economia de GPU vs. TSR atual |
|---|---:|---:|---:|---:|
| TSR atual, Q2, Flickering 1 | 26,38 | 24,09 | 37,24 ms | — |
| TSR Q2, Flickering 0 | 27,81 | 24,68 | 35,26 ms | **1,98 ms** |
| TSR Q1, Flickering 0 | 28,20 | 25,75 | 34,79 ms | **2,45 ms** |
| TAA, Q2 | **33,28** | **30,39** | **29,41 ms** | **7,83 ms** |

As economias em negrito ultrapassam 0,5 ms. Q1 economiza apenas 0,47 ms adicionais frente ao TSR Q2 com Flickering 0. Ainda faltam 12,81 ms de GPU para 16,6 ms com TAA neste percurso.

[Abrir vídeos e prints lado a lado](C:/Users/lucas/Downloads/Hype%20World/HypeWorld/Docs/VisualLab/Alto100/AA/comparar.html)

| TSR atual | TSR Flickering 0 |
|---|---|
| ![TSR atual](C:/Users/lucas/Downloads/Hype%20World/HypeWorld/Docs/VisualLab/Alto100/AA/tsr_atual.mp4) | ![TSR Flickering 0](C:/Users/lucas/Downloads/Hype%20World/HypeWorld/Docs/VisualLab/Alto100/AA/tsr_flickering0.mp4) |

| TSR qualidade 1 | TAA |
|---|---|
| ![TSR Q1](C:/Users/lucas/Downloads/Hype%20World/HypeWorld/Docs/VisualLab/Alto100/AA/tsr_q1.mp4) | ![TAA](C:/Users/lucas/Downloads/Hype%20World/HypeWorld/Docs/VisualLab/Alto100/AA/taa.mp4) |

Vídeos de 10 s renderizados pelo jogo em passos fixos de 1/30 s para comparação visual; os FPS acima foram medidos em tempo real, sem gravação. Detalhes e CSVs nesta pasta.

| TSR Q1 — frame em movimento | TAA — mesmo ponto do percurso |
|---|---|
| ![TSR Q1 em movimento](C:/Users/lucas/Downloads/Hype%20World/HypeWorld/Docs/VisualLab/Alto100/AA/tsr_q1_movimento.png) | ![TAA em movimento](C:/Users/lucas/Downloads/Hype%20World/HypeWorld/Docs/VisualLab/Alto100/AA/taa_movimento.png) |

**Recomendação:** TSR Q1 é o melhor candidato entre os quatro para aprovação visual: economiza 2,45 ms e não apresenta os rastros longos evidentes no TAA nas amostras inspecionadas. Eu rejeitaria o TAA atual pelo ghosting atrás do cabelo/túnica sobre a grama; não considero o critério completo de ausência de cintilação validado só por frames, e a aprovação deve considerar os vídeos em 1080p.
