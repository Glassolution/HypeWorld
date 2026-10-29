# Bloco AA — protocolo e evidências

Quatro opções temporárias na cópia VisualLab. Sem salvar alterações globais, assets, mapa, vento ou animações. Sem commit.

| Opção | sg.AntiAliasingQuality | r.AntiAliasingMethod | TSR Flickering |
|---|---:|---:|---:|
| TSR atual | 2 | 4 | 1 |
| TSR Flickering=0 | 2 | 4 | 0 |
| TSR qualidade 1 | 1 | 4 | 0 |
| TAA | 2 | 2 | Não se aplica |

Todos os outros grupos: Alto (2), grass/foliage 0,8, resolução 100%, 1920×1080, TSR History Screen Percentage 100. TAA usa a configuração de AA Alto, não tuning customizado. Estados efetivos completos em `*_estado.json`.

## Medição

GTX 1650, UE 5.8.4 DX12/SM6, `-game` Development, VSync/limite de FPS/resolução dinâmica desligados. Três capturas de 30 s por opção, 25 s iniciais de estabilização por opção, 5 s entre capturas. Nenhuma gravação ou codificação durante as medições. O benchmark anterior era parado; este é em movimento e os resultados não são uma diferença isolada de AA em relação ao número histórico de 8,55 ms.

Câmera existente controlada apenas em runtime, FOV 90, pitch -15°, yaw 90°. Percurso fechado de 10 s: x=200 sin(2πt/10), y=-312,028006+100(1-cos(2πt/10)), z=250,352636 cm. Três voltas por captura. O personagem permanece parado, com suas animações originais; só a câmera percorre a trajetória. O vento permanece com valores e funcionamento originais. As opções não compartilham a mesma fase absoluta do vento.

FPS médio = 1000 / média de FrameTime. 1% low = 1000 / média do 1% de frames mais lentos (arredondado para cima). Sem remoção de outliers. GPU e passes são medianas. CSV bruto e resumos por execução permanecem nesta pasta.

## Vídeos e prints

PNG de cada opção em posição inicial, após a medição. Vídeos separados, feitos com `Shot` nativo do runtime, sem HighResShot, montagens, interpolação ou geração de imagem. Sequência de 300 frames para uma volta de 10 segundos, simulação em passos fixos de 1/30 s (`t.OverrideFPS=30`) exclusivamente durante a captura, depois restaurado a 0. Isso permite comparar o AA com o mesmo deslocamento entre frames. Não usar a fluidez destes vídeos para inferir FPS de execução; os resultados em tempo real estão na tabela de desempenho.

Os parâmetros e a velocidade simulada do vento/animações não são modificados. O histórico temporal continua entre frames. Quadros nativos em `Saved/VisualLab/AAFrames`, codificador local em `Saved/VisualLab/Tools`; estes arquivos temporários não entram em Git. Vídeos finais H.264 em 1080p, sem redução de resolução, com compressão de alta qualidade; os PNGs permitem inspeção sem compressão de vídeo.

## Preparação

A primeira tentativa de criar câmera pela API de editor encerrou o runtime antes de qualquer captura válida (`runtime_falha_inicial.log`). A segunda API de criação não estava exposta em Python e retornou erro antes da coleta. O protocolo válido usa o CameraComponent existente com transform absoluto temporário. Não houve alteração ou salvamento de assets nessas tentativas.

Configurações locais anteriores preservadas em `ConfigAntes`; restaurar após encerrar o teste e conferir igualdade. Resolução 100% já autorizada permanece. Recomendações são propostas para avaliação, não alterações globais já adotadas.

## Inspeção visual e custo do TAA

No frame 30 (aproximadamente 1 s de percurso) e no seguinte, TAA mostra múltiplos contornos atrás do cabelo e um rastro claro da túnica sobre a grama recém-revelada. O mesmo ponto nas variantes TSR não mostra esse rastro longo evidente. Os arquivos `*_movimento.png` são cópias byte a byte do frame 30, sem recorte, retoque ou composição. A inspeção de frames não certifica ausência de cintilação sutil durante reprodução. O vento tem fase diferente entre as sequências; diferenças finas entre lâminas não devem ser tratadas como erro de AA por simples subtração de imagens.

ProfileGPU adicional, fora das medições: TAA(MainUpsampling Quality=Medium) 1920×1080 → 1920×1080 = 0,840 ms em um frame. Esse é o nome interno do shader mesmo com sg.AntiAliasingQuality=2, cujo r.TemporalAA.Quality efetivo é 1 no BaseScalability da engine. Não confundir esse valor pontual com mediana do benchmark. TSR mediano em movimento: atual 8,199 ms, Flickering 0 6,321 ms, Q1 5,921 ms.

Os arquivos `*_estado.json` são atualizados a cada seleção de opção, inclusive durante gravação; `*_video_estado.json` preservam a configuração de vídeo. `estados_finais_apos_video.json` registra as quatro configurações verificadas depois da captura, com t.OverrideFPS restaurado a 0. O protocolo de medição usa t.OverrideFPS=0 e nunca grava frames durante os CSVs.
