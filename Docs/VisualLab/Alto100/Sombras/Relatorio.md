# Bloco 2 — sombras

Alto, 1080p nativo, TSR Q1, câmera fixa, 3×30 s por candidato. Sem adoção global ou commit.

**O que pesa:** na triagem, retirar apenas as sombras das árvores/troncos economizou 3,72 ms de ShadowDepths; arbustos/flores, 0,64 ms; rochas, 0,10 ms; personagens, 0,03 ms; outros (terreno/estruturas/props), 0,46 ms. Grama real já não projeta sombra dinâmica. São reduções marginais, não parcelas que se somam; há trabalho compartilhado no passe Nanite.

| Opção | FPS | 1% low | GPU |
|---|---:|---:|---:|
| Original, 4×2048 | 29,87 | 27,45 | 32,85 ms |
| Corte de sombras pequenas | 30,55 | 28,20 | 32,13 ms |
| Cortes + 3×1536 + alcance menor | **33,42** | **30,69** | **29,33 ms** |
| Cortes + 2×2048 + alcance menor | 32,34 | 28,56 | 29,98 ms |

[Prints reais lado a lado com a referência 4](C:/Users/lucas/Downloads/Hype%20World/HypeWorld/Docs/VisualLab/Alto100/Sombras/comparar.html).

**Recomendação:** 3×1536 como candidato visual, economizando **3,52 ms** frente ao controle; 2×2048 custa **0,64 ms a mais**. As sombras grandes permanecem contrastadas e suaves, mas os arbustos ficam mais claros e perdem autosombra; isso precisa de aprovação visual. Ainda faltam **12,73 ms de GPU** para 16,6 ms; 60 FPS não foi atingido.

Cortar sombras pequenas economizou **0,72 ms**. Todos esses deltas destacados passam de 0,5 ms. O alcance CSM efetivo passa de aproximadamente 68 m para 42,5 m; DF ao longe, PCSS e Source Angle preservados. Validação feita neste enquadramento, sem certificar ainda todas as transições ao percorrer o mapa. Detalhes/CSVs na pasta.
