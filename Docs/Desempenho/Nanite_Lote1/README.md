# Nanite no Lote 1 (11 malhas opacas): medição limpa, 05/11/2026

**Decisão: Lote 1 fica SEM Nanite.** As 11 malhas voltaram aos bytes originais (`shasum` = Git nas 11/11).

## Como foi medido
- Mapa `L_Tutorial`, editor no Mac M1, mesmas 3 câmeras da baseline, sem compilar shader.
- Cada sessão: editor recém-aberto, 90 s parado, 1 rodada de aquecimento descartada e 3 rodadas de 12 s por câmera.
- Ordem alternada para separar efeito real de piora com o tempo: A1, B1, A2, B2, A3 (A = original, B = Nanite).
- Scripts: `Tools/editor/fase2_sessao.sh`, `fase2_medir.py`, `fase2_ligar_nanite.py`, `fase2_comparar.py`. Dados em `A1/ ... B2/`.

## Tempo de quadro (ms, mediana)
| Câmera | A1 | B1 | A2 | B2 | A3 | B1 − média(A1,A2) | B2 − média(A2,A3) |
|---|---|---|---|---|---|---|---|
| P1 Vista geral | 34,31 | 36,64 | 37,25 | 37,70 | 38,05 | +0,86 | +0,05 |
| P2 Ponte | 33,80 | 36,48 | 36,61 | 37,10 | 37,67 | +1,28 | −0,04 |
| P3 Treino | 34,56 | 36,41 | 37,26 | 38,15 | 38,58 | +0,50 | +0,23 |

## GPU (ms, mediana)
| Câmera | A1 | B1 | A2 | B2 | A3 |
|---|---|---|---|---|---|
| P1 | 33,11 | 35,54 | 35,98 | 36,81 | 37,05 |
| P2 | 32,74 | 35,12 | 35,50 | 36,08 | 36,70 |
| P3 | 33,40 | 35,30 | 36,13 | 36,99 | 37,75 |

## Triângulos visíveis (soma do dump de primitivas, última rodada)
| Câmera | A1 | B1 | A2 | B2 | A3 |
|---|---|---|---|---|---|
| P1 | 181.648 | 141.268 | 194.370 | 142.612 | 181.778 |
| P2 | 133.296 | 101.548 | 129.438 | 101.548 | 133.296 |
| P3 | 189.962 | 138.820 | 188.818 | 140.420 | 199.606 |

Passe que desenha a geometria (`BasePassParallel`, ms, mediana de 3 amostras): A 2,04–2,69, B 2,27–2,49, sem diferença clara.

## O que os números dizem
- **Sem ganho medível.** Com Nanite, o quadro fica de −0,04 a +1,28 ms em relação às sessões vizinhas sem Nanite. A variação entre sessões iguais é maior: de A1 para A3 subiu cerca de 4 ms, sem mudar nada.
- **Triângulos caem 20–30%, mas não viram ganho de tempo.** O gasto com geometria é pequeno aqui (cerca de 2,4 ms de 35).
- **O Nanite não roda neste Mac.** O log diz `Falling back to SM5` e `shaderplatform="METAL_SM5"`. O ProfileGPU das 5 sessões não tem nenhuma linha "Nanite". Portanto o lado B mediu a **malha reserva (fallback)**: é o caso de "hardware sem Nanite" do plano, não o desempenho do Nanite em si.
- **Malha reserva é bem mais simples:** `SM_Placeholder_Ponte10` cai de 6.368 para 516 triângulos; `SM_Stone_Boulder_1`, de 1.286 para 1.088.
- **Fotos:** a diferença A×B (0,96–2,93 de 255) fica na mesma faixa de A×A e B×B (0,65–2,88), que é vento e suavização. Só cobre estas 3 câmeras. Lado a lado em `Fotos/`.

## Por que reverter
1. Nenhum ganho medido.
2. Risco na ponte de perto, sem teste: 516 triângulos podem aparecer em hardware sem Nanite.
3. Risco de colisão (**suposição, não testada**): 7 destas malhas usam a malha de render como colisão, e pelo que sei, com Nanite a colisão complexa usa a malha reserva. A ponte poderia mudar para quem anda nela. A Fase 3 trata disso.
4. O ganho real no Windows não pode ser medido neste Mac.

## O que isto NÃO prova
- Nada sobre Nanite real em PC Windows. Se quiser essa resposta, repetir `fase2_sessao.sh` numa máquina com SM6 (o script precisaria de adaptação para Windows).
- Números são do editor (viewport 2283×1210), servem para comparar, não para prever o FPS do jogo.

## Achados
- **Sessão contaminada confirmada:** o estado "revertido" do OpenCode mediu 148/160/143 ms lá e agora cerca de 34 ms (`C_RevertidoOpenCode/`), com as mesmas malhas. O gargalo era a máquina/sessão.
- **Maior custo visto: TSR (suavização de imagem), 13–16 ms de cerca de 35.** Teste rápido com `r.AntiAliasingMethod 0` numa câmera: 28,5 → 15,2 ms, mas sem suavização a imagem muda. Nada foi alterado.
- O "revertido" do OpenCode tinha triângulos por LOD iguais ao original, mas as 4 pedras ficaram com arquivo cerca de 60% menor. Não investigado, pois o original foi restaurado. Cópia em `Saved/Backup_Fase2/Lote1_RevertidoOpenCode`.

## O que falta do plano do OpenCode (não feito aqui)
- Fase 2: o resto (LODs, distância de corte, foliage com máscara caso a caso).
- Fase 3: colisão simples, com teste andando no mapa.
- Fase 4: tabela final de antes e depois.
