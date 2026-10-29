# GI/AO — comparação controlada

1920×1080, resolução interna 100%, TSR Q1, sombras 3×1536 com alcance menor e cortes pequenos. Mesma câmera fixa do bloco de sombras; 3×30 s por opção, GTX 1650, `-game`.

| Opção | FPS | 1% low | GPU |
|---|---:|---:|---:|
| Lumen High — controle desta sessão | 32,93 | 29,85 | 29,79 ms |
| Lumen Irradiance Volume | **35,07** | **31,93** | **27,93 ms** |
| Sky Light + DFAO | 35,15 | 32,06 | 27,88 ms |

[Prints reais lado a lado e referência](comparar.html).

**Recomendação:** Irradiance Volume: economiza **1,86 ms**, mantendo o visual mais próximo do Lumen High neste enquadramento. Sky Light + DFAO economiza **1,90 ms**, mas muda mais o volume/iluminação das copas e fica apenas **0,05 ms** à frente do Irradiance — diferença insuficiente para declarar vantagem robusta.

Ainda faltam **11,33 ms** de GPU para o orçamento de 16,6 ms com Irradiance. Não atingimos 60 FPS nem 1% low acima de 50.

Os dois ganhos destacados ultrapassam 0,5 ms. O caminho sem Lumen executa DFAO de verdade (ConeTraceGlobal, CombineCones e UpdateHistory confirmados no ProfileGPU), mas também ativa o SSAO existente do Alto, mediana **2,09 ms** + setup **0,10 ms**. Não foi afinado neste primeiro A/B. Não confundir o custo do gather com o custo total de GI.

Sem commit, sem adoção global. Arbustos ainda estão com os materiais originais neste bloco; sua recuperação de volume, copas opacas em uma ou duas árvores e FSR Native AA continuam como próximos experimentos. As folhas atuais foram confirmadas como Masked, mas o A/B de copas ainda é necessário para atribuir a economia à troca da geometria/material.
