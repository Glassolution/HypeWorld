---
status: arquivado
atualizado: 2026-11-08
---
# PLANO POR FASES — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
Ferramenta para ver poses: `Tools/editor/folha_quadros.py` (+ `folha_limpar.py`) — tempo da ESQUERDA para a DIREITA, rótulo no começo da linha (até 24/10 a folha saía invertida e eu li o DashStep_Front ao contrário). Teste: `Tools/editor/teste_pack_anime.py` (Play; fotos com Shot). Gotcha: o ue_exec falha se o texto do script tiver ".py" escrito antes do código (o editor acha que é nome de arquivo).
- **Aura de chakra estilo anime (atual, 24/10/2026 — 4ª versão, especificação escrita do Felipe + referência `Referencias/Aura/ref_estilo_anime.webp`):** fogo de energia cel-shaded. `NinjaChakraAuraComponent`: **duas camadas** viradas para a câmera com `M_Chakra_ToonFlame` — atrás do corpo (`FlameBackOffset` 20) e na frente (`FrontLayerOffset` 35, `FrontLayerScale` 0,85, parâmetro FrontLayer=1, Seed 37: pouco azul, só em volta das pernas, ignora o Custom Depth) para o personagem ficar "dentro". `FlameSize` 330x315 (base ~2,5x o corpo, línguas até ~1,6x a altura), **sem pulsar (24/10/2026, Felipe: "chakra não pulsa, flui")**: tamanho, brilho do chão e luz fixos (`PulseAmount`/`PulseRate` existem mas não são usados); a forma quase não muda no lugar (evolução do ruído ×0,25) — o movimento vem só do ruído subindo continuamente. Câmera lenta (slomo 0,25) conferida: quadros seguidos sem salto. Liga crescendo de baixo para cima (`GrowTime` 0,2 s, parâmetro Grow) e desliga se partindo em pedaços (`DissolveTime` 0,25 s, parâmetro Dissolve: buracos e gotas aumentam) — medido: Grow 0→1 em 0,2 s, Dissolve 0→1 em ~0,25 s e esconde. `ChargeLevel` = chakra/máximo (personagem chama `SetChargeLevel` no Tick): línguas mais altas e mais brilho. Material 100% procedural (nó Noise ALU, entrada "World Position"; a especificação pedia texturas de ruído — o procedural faz o mesmo): coluna larga + eixo balançando (`sway`) + ondinhas na borda (`wig`); altura de cada língua = ruído ao longo da largura metade "crista" metade arredondado (3–5 pontas); buracos com contorno; gotas que se soltam. Contorno NÍTIDO #E8FCFF × `EdgeGlow` 12 (`EdgeWidth` 0,0055 da altura, medido na tela por DDX/DDY, anti-serrilhado mínimo), faixa interna #7FE6FF (`BandWidth` 0,02), miolo `FillOpacity` 0,5 com gradiente #4A4DFF→#1E8BFF→#3FC8FF. Sem teste de profundidade (o chão não corta a base); o corpo esconde a camada de trás pelo Custom Depth (ligado na malha no BeginPlay); a base desce para cair `FlameFrontReach` 50 cm à frente dos pés. Chão `M_Chakra_GroundGlow` #5A4BFF aditivo ~0,5; luz `LightColor` #3FA9FF (intensidade × Alpha × (0,7 + 0,3 × carga), sem pulso). Explosão do máximo (`M_Chakra_Burst`/`GroundBolts`) fora (materiais guardados). **Não feito:** Niagara (o Python não monta sistemas Niagara de forma confiável; as gotas são do material, não partículas, e não há "burst" de 15–20 gotas como partículas), bloom no pós-processo. Materiais por `Tools/editor/criar_materiais_aura.py`; fotos `Referencias/Aura/resultado/aura_v18_*.png`, `aura_dissolve.png`. Sons prontos para ligar: ChargeStartSound, ChargeLoopSound, FullBurstSound, FullLoopSound.

~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
