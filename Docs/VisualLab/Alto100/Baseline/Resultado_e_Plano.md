# Baseline Alto nativo — 26/10/2026

Estado: medição concluída; otimizações ainda não implementadas. Resolução 100% explicitamente autorizada pelo usuário. Bloco aguardando avaliação; nenhum commit de aprovação criado.

## Resultado

GTX 1650, DX12/SM6, UE 5.8.4, execução Development `-game`, editor fechado, saída 1920×1080. Alto completo: todos os grupos de qualidade em 2, foliage/grass 80%, TSR nativo, Flickering=1, histórico 100%, CSM 4×2048, PCSS ligado, VSM desligado. VSync, limite de FPS e resolução dinâmica desligados durante a coleta.

Três capturas de 30 segundos, após aquecimento, mesma câmera do experimento anterior: posição (0, -312.028006, 250.352636), rotação (-15, 90, 0), FOV 90. Sem alterar vento, animações ou layout. Cópia nativa do mapa: `/Game/HypeWorld/VisualLab/Grama/L_Grama_Baseline`.

| Captura | FPS médio | 1% low | GPU mediana |
|---|---:|---:|---:|
| 1 | 26,92 | 24,62 | 36,66 ms |
| 2 | 26,88 | 25,05 | 36,73 ms |
| 3 | 26,70 | 24,41 | 36,89 ms |
| Agregado | **26,83** | **24,57** | **36,76 ms** |

2.414 frames, 89,97 segundos efetivamente registrados. 1% low = 1000 dividido pela média dos tempos do 1% de frames mais lentos, sem remover outliers. Frame mediano 37,24 ms; p99 39,74 ms. Faltam **20,16 ms de GPU** para 16,6 ms, redução de aproximadamente 55%. A meta ainda não foi atingida. Esta câmera parada não valida desempenho mínimo no mapa inteiro; a validação final precisa incluir percurso em movimento.

## Imagens reais

| Baseline Alto 100%, render do jogo | Referência 4 fornecida |
|---|---|
| ![Baseline](C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Alto100/Baseline/alto100.png) | ![Referência](C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Alto100/Baseline/Referencias/zenkai_4.png) |

Não há “depois otimizado” neste bloco. A captura anterior em Médio 67% permanece em `../../Grama/Passo1/grama_40.png`: comparação histórica, não A/B isolado de resolução, pois preset e densidade também diferem. Nenhuma imagem foi gerada ou retocada. Referência 4 orienta sol quente, sombra fria e atmosfera; referência 3 mostra melhor os contatos de paredes e pavimento.

## GPU por passe

Medianas agregadas dos CSV, salvo AO explicitamente identificado como amostra única. Marcadores podem ser hierárquicos; não somar indiscriminadamente esta tabela.

| Marcador | ms |
|---|---:|
| BasePass | 2,599 |
| Nanite BasePass | 0,937 |
| Nanite VisBuffer | 1,986 |
| Prepass | 0,141 |
| Velocity | 1,043 |
| Lumen Scene Update | 0,334 |
| Lumen Scene Lighting | 0,824 |
| Lumen Screen Probe Gather, incluindo AO local | 3,274 |
| ShadowDepths | **7,553** |
| ShadowProjection | **2,305** |
| Distance Field Shadows, marcador relacionado às sombras | 0,150 |
| TSR | **8,555** |
| Outros pós-processos, marcador CSV Postprocessing | 1,292 |
| Translucency | 0,125 |
| Translucent Lighting | 0,143 |
| Render Deferred Lighting | 1,463 |
| SSR | 0,209 |
| Water Depth Prepass / Water | 0,394 / 0,229 |
| Volumetric Fog / Fog | 0,365 / 0,211 |
| Sky Atmosphere / LUTs | 0,026 / 0,121 |
| Unaccounted | 0,832 |

O ProfileGPU adicional encontrou `ShortRangeAO_ScreenSpace_HorizonSearch` em **0,233 ms em um frame diagnóstico**, dentro do Lumen. Não existe mediana independente de AO nos CSV; esse valor não deve ser somado novamente à GI. Não foi observado passe independente de SSAO/DFAO nesse frame. O marcador PostProcessing da árvore ProfileGPU inclui TSR; o marcador CSV mostrado acima é separado, portanto os dois formatos não devem ser misturados.

Nesse frame diagnóstico, TSR consumiu 8,128 ms, dos quais 5,419 ms em RejectShading. Isso aponta uma prioridade concreta de investigação; não prova que desativar Flickering economize esse valor. A árvore confirma entrada e saída 1920×1080.

410 draws medianos, 69 em ShadowDepths, 52 em BasePass e 45 em velocity. Aproximadamente 1,28 milhão de primitivas; VRAM usada 2.433 MB para orçamento de 3.199 MB. Não há evidência aqui de saturação de VRAM. GameThread mediano 4,47 ms; tempo alto do RenderThread pode incluir espera de GPU.

Correção do diagnóstico anterior: embora DefaultEngine indique método global de GI 0, o volume de pós-processo sobrescreve para Lumen. Confirmado em `postprocess_efetivo.json` e nos passes GPU. Reflexos efetivos são Screen Space. Toon permanece com peso 1.

## Orçamento proposto, não medido

Tetos de projeto com categorias sem sobreposição, a validar em A/B. Não são promessas de custo ou resultados já obtidos.

| Categoria | Teto proposto ms |
|---|---:|
| BasePass incluindo Nanite | 2,8 |
| Visibilidade, prepass, HZB | 2,0 |
| Velocity | 0,6 |
| GI sem AO contabilizado abaixo | 1,8 |
| AO | 0,5 |
| ShadowDepths | 2,0 |
| Projeção de sombras incluindo DF | 1,0 |
| AA nativo | 1,0 |
| Outros pós-processos | 0,8 |
| Translucência e sua iluminação | 0,3 |
| Iluminação direta | 1,0 |
| Água | 0,4 |
| Céu e fog | 0,4 |
| Outros | 0,8 |
| Total de trabalho | **15,4** |
| Reserva até 16,6 ms | **1,2** |

Orçamento agressivo: exige reduzir cerca de 58% da GPU atual para preservar a reserva. Se uma técnica não couber, o orçamento e o compromisso visual precisam ser revistos com os resultados, sem reduzir resolução. 1% low acima de 50 exige média dos frames mais lentos abaixo de 20 ms, além do alvo normal de 16,6 ms.

## Sequência proposta: um bloco por aprovação

1. **AA:** TSR atual contra TAA, ambos 100%, câmera/percurso reproduzível, grama com vento original. Capturar vídeo real para cintilação, ghosting e perda de detalhe, além de PNGs e 3×30 s por opção. TSR atual custa 8,55 ms; TAA ainda não foi medido. Se TAA perder qualidade demais, testar qualidade interna do TSR preservando resolução nativa. Não desativar velocity da vegetação indiscriminadamente.
2. **Sombras:** comparar CSM atual com menos cascatas e resolução menor (candidatos: duas cascatas, 1024/1536), mantendo cobertura útil e DF ao longe. Medir PCSS e transições; não usar VSM. Source Angle não é garantia isolada de suavidade no CSM clássico. ShadowDepths e projeção atuais tornam esse bloco indispensável.
3. **GI/AO:** primeira candidata é Lumen Irradiance Volume/Lite a 100%, preservando iluminação indireta e medindo qualidade junto do custo. Comparar Lumen High e Sky Light + DFAO sem Lumen. O ~1 ms anterior em Médio não se transfere para 1080p nativo nem representa automaticamente toda a GI. Escolha final depende desses A/B. A documentação do UE 5.8 descreve Lite como alternativa de menor custo, com qualidade inferior, baseada em probes de irradiância em world space ([Epic](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-performance-guide-for-unreal-engine)).
4. **AO por necessidade visual:** primeiro aproveitar AO já assado/canal existente; assar AO não cria passe dinâmico, mas novas texturas têm memória/banda e multiplicar Base Color pode sujar luz direta. DFAO para volumes grandes; SSAO/GTAO leve para contatos pequenos; contact shadows seletivas por último. Esta ordem é uma estratégia, não uma classificação universal de custo: DFAO pode custar mais que SSAO leve e precisa ser medido ([Epic DFAO](https://dev.epicgames.com/documentation/unreal-engine/distance-field-ambient-occlusion-in-unreal-engine?lang=en-US)). AO=1 no material da grama não elimina por si só AO de tela/Lumen/DFAO; verificar também essas contribuições.
5. **Grama:** medir custo isolado, ampliar lâminas e reduzir elementos/overdraw preservando 25–35% de espaços e vento. O BasePass medido é da cena inteira, não só grama. Cor verde e variação ampla compartilhada com terreno/RVT em cópias, após orçamento estabilizado.
6. **Visual:** sol mais quente, contribuição fria do céu nas sombras, exposição/bloom contidos e vegetação coerente. Fazer A/B real com performance; destacar qualquer acréscimo superior a 0,5 ms. Sem alterar layout, vento ou animações.

CSM perto e Distance Field Shadows longe são uma combinação documentada, mas o alcance e custo locais ainda precisam ser medidos ([Epic](https://dev.epicgames.com/documentation/en-us/unreal-engine/distance-field-soft-shadows-in-unreal-engine)). A capacidade de rodar outro jogo não determina o custo destes passes nesta engine; a meta será avaliada com medições deste projeto.

## Persistência e evidências

Alterações globais limitadas à autorização de resolução: DefaultEngine remove o padrão fixo 67; DefaultScalability mapeia Médio/Alto/Épico/Cinemático para 100; DefaultGameUserSettings define resolução 100. Aplicar cada um desses quatro presets em runtime retornou 100, registrado em `presets_resolucao.json`.

Preferências locais anteriores restauradas após encerrar o jogo; somente ResolutionQuality passa de 0 para 100 em GameUserSettings local. As demais configurações locais são idênticas aos backups (`restauracao_config.json`). O preset padrão anterior e o tamanho de janela anterior foram preservados; 1080p/Alto foram condições do teste.

Evidências: `alto100_r1.csv`, `alto100_r2.csv`, `alto100_r3.csv`, `desempenho.json`, estados por captura, `runtime.log`, `alto100.png`, scripts de coleta/análise e backups de configuração nesta pasta. Nenhuma otimização visual, alteração de material ou aprovação foi incorporada a Visual_Aprovado.md neste bloco.
