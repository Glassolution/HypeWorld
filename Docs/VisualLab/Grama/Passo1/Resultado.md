# Passo 1 — baseline renderizada e densidade

26/10/2026. Resultado para revisão; não é aprovação de mudança visual. Os passos 2–6 não foram executados.

## Capturas reais

PNGs originais do framebuffer do jogo, 1920×1080, sem montagem, retoque, redimensionamento ou geração por IA. Abra os arquivos para comparar em 100% de zoom.

| Médio, grama 40% | Médio, grama 100% |
|---|---|
| ![Grama 40%](grama_40.png) | ![Grama 100%](grama_100.png) |

Todos os grupos `sg.*Quality` usados — ViewDistance, AntiAliasing, Shadow, GlobalIllumination, Reflection, PostProcess, Texture, Effects, Foliage, Shading e Landscape — foram confirmados em 1 (Médio). TSR=4, `r.ScreenPercentage=67`, `r.TSR.ShadingRejection.Flickering=0`; VSync, limite de FPS e resolução dinâmica desligados.

**Variável isolada:** `grass.DensityScale=0.4` versus `1.0`. `foliage.DensityScale=0.4` permanece igual para não alterar outras plantas. A segunda imagem reproduz a densidade de grama de 100%, não o preset Épico inteiro que estava salvo anteriormente. Não alterei distância de visão, árvores, sombras, flores, materiais ou qualquer outro grupo entre A e B.

Câmera idêntica: posição (0; −312,028; 250,353) cm, pitch −15°, yaw 90°, FOV horizontal 90°. Personagem na posição (0; 60; 90,916) cm. O vento e a animação continuam ativos e não têm necessariamente a mesma fase entre screenshots. A interface foi omitida pelo comando nativo de screenshot, igualmente nos dois casos.

## Desempenho medido

GTX 1650, driver 617.14, Unreal 5.8.4, DX12/SM6. Sessão dedicada `UnrealEditor.exe -game`, com editor fechado. Não é um build Shipping. Três capturas alternadas de aproximadamente 30 segundos por condição, com 15 segundos de estabilização após cada troca. Câmera parada. Nenhuma captura de imagem dentro dos intervalos medidos. Valores de GPU abaixo são medianas; FPS é 1000 dividido pelo tempo médio do frame; 1% low é 1000 dividido pela média do 1% de frames mais lentos. Não houve remoção de outliers.

| Métrica | 40% | 100% | Diferença 100 − 40 |
|---|---:|---:|---:|
| GPU total mediana | 15,28 ms | 16,32 ms | **+1,04 ms** |
| GPU p95 | 16,20 ms | 17,27 ms | +1,07 ms |
| Tempo de frame mediano | 15,88 ms | 17,00 ms | **+1,12 ms** |
| FPS médio | 62,7 | 58,7 | −4,0 |
| 1% low | 50,0 FPS | 47,9 FPS | −2,1 |
| BasePass GPU | 0,76 ms | 1,32 ms | **+0,56 ms** |
| ShadowDepths GPU | 1,75 ms | 1,76 ms | +0,02 ms |
| TSR GPU | 3,59 ms | 3,60 ms | +0,02 ms |
| Pós-processo GPU | 0,89 ms | 0,89 ms | +0,003 ms |

**Alerta >0,5 ms:** GPU total e BasePass aumentam acima do limite solicitado. As medianas de GPU por repetição foram 15,278 / 15,296 / 15,275 ms a 40%, e 16,273 / 16,351 / 16,340 ms a 100%; o aumento é consistente. Passes podem se sobrepor; não somar seus tempos para reconstruir o total.

Este teste mede somente esse enquadramento parado. Não demonstra estabilidade em movimento nem desempenho de todo o mapa. Os CSVs brutos, estados runtime e `desempenho.json` ficam nesta pasta; `analisar.py` reproduz os resultados.

## Leitura visual

A 40%, os espaços voltam a separar os tufos. A 100%, o primeiro plano fecha bastante e a área de meia distância vira uma textura mais contínua e fina. A densidade alta explica parte importante do tapete observado, mas 40% sozinho ainda não entrega as lâminas largas/legíveis e a cor da referência. Não foi medida cobertura exata de 25–35% neste passo; isso será critério do teste de geometria/densidade no passo 4.

Próximo teste proposto, ainda não executado: manter essa baseline a 40% e comparar 67%, 75% e 100% de resolução interna, depois isolar Flickering=1. Não escolher uma configuração global antes dos A/B e da sua aprovação.

## Indra — apenas diagnóstico do toon

| Toon ligado | Toon desligado temporariamente |
|---|---|
| ![Toon ligado](indra_toon_ligado.png) | ![Toon desligado](indra_toon_desligado.png) |

Mesma câmera, saída, iluminação, grama 40%, material e personagem. Apenas o peso do blendable `MI_PP_Personagem` no volume da cópia em runtime mudou de 1 para 0; foi restaurado a 1 depois. O asset do material não foi editado.

**O toon contribui claramente para achatar a túnica.** Sem ele, surgem gradientes de volume no peito, mangas e parte inferior da roupa; o cabelo também recupera volume e fica mais escuro. Isso não prova que o PP seja a única causa: as dobras finas continuam limitadas pela malha/material. Desligá-lo também escurece pele e tecido, portanto a versão desligada não está sendo proposta como acabamento final. Não fiz benchmark específico de toon neste teste visual rápido.

## Integridade e preparação

A cópia usada é `/Game/HypeWorld/VisualLab/Grama/L_Grama_Baseline`, criada nativamente pelo Unreal. A cópia bruta inicial não exibiu grama; seus previews foram descartados dos resultados e nenhum perfil dela foi usado. As imagens finais de grama foram recapturadas após a coleta, com a densidade estabilizada e sem troca de estado no mesmo frame.

Nenhum asset de produção, vento, animação ou layout foi editado. O teste não salvou escolhas gráficas nos arquivos Config do projeto. O Unreal chegou a regravar o GameUserSettings local durante a preparação; a restauração da cópia anterior e sua verificação por hash estão documentadas em `integridade_final.json`. `Visual_Aprovado.md` permanece sem alteração e não há commit de aprovação deste passo.
