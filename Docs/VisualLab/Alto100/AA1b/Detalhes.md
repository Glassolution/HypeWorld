# Bloco 1b: protocolo

Execução somente na cópia VisualLab, 1920×1080, escala 100%, Alto completo, foliage/grass 80%, mesma câmera e trajetória do bloco AA. Vento, animações, assets e configuração global não são salvos ou alterados. Isolamentos somente em componentes de runtime.

## Diagnóstico causal

Cinco clips de 3 s, primeiros 90 frames do percurso anterior, 60 frames de aquecimento. Original; chakra dos pés oculto (shell, ground, wisps e luzes); toon com blendable weight 0; casaco externo oculto (slot 3); slots 2 e 3 ocultos (remove toda túnica, mas também partes do corpo). O slot 2 compartilha geometria entre roupa inferior, pele/pernas; portanto o último teste não é remoção exclusivamente da roupa. Cada teste restaura os demais elementos. Os isolamentos não são propostas visuais de produção e não têm benchmark de desempenho próprio.

Inspeção dos frames 30: rastro persiste sem chakra, sem toon e no cabelo mesmo após ocultar as seções da túnica. Isso descarta esses elementos como causa única; o histórico de TAA mal rejeitado continua sendo a hipótese principal. Não foi feita alteração em Output Velocity/Responsive AA, pois a condição de causalidade translúcida não foi demonstrada. Não confundir redução de contraste sem toon com eliminação do histórico.

## Ajustes

Confirmado no código local do UE 5.8.4, `Engine/Source/Runtime/Renderer/Private/PostProcess/TemporalAA.cpp`: `r.TemporalAACurrentFrameWeight`, padrão 0,04. A própria descrição alerta que valores baixos favorecem borrão/ghosting e altos deixam jitter mais aparente. Testes: 0,04; 0,10; 0,20; 0,30, todos com r.TemporalAA.Quality=1, conforme o Alto da engine.

Teste adicional: `r.TemporalAA.Quality=3`, peso 0,04. A descrição do código local informa filtro da qualidade 1 mais anti-ghosting. Isso não é sg.AntiAliasingQuality=3, não altera outros grupos e não aumenta resolução interna. Todos os testes usam sg.AntiAliasingQuality=2 e r.AntiAliasingMethod=2.

Medições: 3×30 s por opção, 25 s de aquecimento, 5 s entre capturas; câmera repete três voltas de 10 s. Sem gravação/encoder durante medição. FPS médio=1000/média FrameTime; 1% low=1000/média do 1% de frames mais lentos; GPU mediana. Sem remoção de outliers. Resultados brutos nos CSV.

Vídeos separados do benchmark, Shot nativo, 30 passos de simulação por segundo, sem interpolação, retoque ou montagem. O relógio fixo é exclusivo da gravação e não representa FPS real. Parâmetros/velocidade simulada do vento e animações preservados, fase absoluta diferente entre casos. Os ajustes têm 10 s e os isolamentos 3 s. O vídeo de peso 0,04 pode reutilizar a tomada de TAA do bloco AA, com os mesmos parâmetros e trajetória; as medições de peso 0,04 são novas neste bloco.

Arquivos em Saved/VisualLab/AA1bFrames mantêm os PNGs nativos. O comparador usa vídeos comprimidos H.264 CRF12, sem reduzir resolução. A ausência absoluta de cintilação não pode ser certificada por um print isolado.
