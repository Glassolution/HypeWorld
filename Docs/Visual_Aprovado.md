# Visual aprovado: grama e terreno do L_Tutorial

> **Regra:** antes de qualquer mudança na grama ou no terreno, leia este arquivo.
> Não altere os itens abaixo sem perguntar ao usuário. Se uma mudança precisar mexer
> em algum deles, avise antes, diga qual item muda e por quê.

Última atualização: 2026-10-25 (tufos em duas escalas; chão no tom da grama, sem manchas).

## Assets (cópias do projeto; o pack StyleHex não é alterado)

| O quê | Asset |
|---|---|
| Material do terreno (Grass Output, máscara de tufos) | `/Game/Maps/Tutorial/Tutorial_Assets/Materiais/M_Tut_Terreno_VT` |
| Instância do terreno (cor do chão, parâmetros dos tufos) | `.../Materiais/MI_Tut_TerrenoBase` (filha: `MI_Tut_Terreno`, usada no Landscape) |
| Grass types | `.../Foliage/LGT_Tut_Grama_1`, `LGT_Tut_Grama_2` |
| Material da grama | `.../Materiais/M_Tut_Grama_Master` → `MI_Tut_GramaBase` → `MI_Tut_Grama` |
| Vento | `.../Materiais/MPC_Wind` (usado por `M_Tut_Grama_Master` e `M_Tut_Flores_Master`) |

Só são lidos do pack, sem nenhuma alteração: os meshes `SM_Fol_Grass_Clump_1..5`, a textura `T_Noise_1_Mask` (ruído dos tufos) e as RVTs `VT_Terrain_Color_Demo` / `VT_Terrain_Grass_Color_Demo`.

## Altura e escala (LGT_Tut_Grama_1 e _2, iguais)

| Mesh | Escala Z | Escala XY | Densidade |
|---|---|---|---|
| Clump 1 e 2 | 0,40–0,55 | 1,1–1,4 | 280 |
| Clump 3 | 0,36–0,495 | 1,1–1,4 | 280 |
| Clump 4 | 0,36–0,495 | 1,1–1,4 | 28 |
| Clump 5 | 0,384–0,528 | 1,1–1,4 | 28 |

- A altura chega mais ou menos no calcanhar do personagem.
- Uma tentativa de +20% de altura foi recusada e desfeita (commit `4c33339`).
- A densidade é alta de propósito: o preset Médio reduz a grama a 40% (`grass.DensityScale`) e a máscara tira os espaços. Com 170, os tufos ficavam ralos no jogo.
- Cull: 3000–4000 cm. Alinhar à superfície e rotação aleatória ligados.

## Tufos e chão visível (M_Tut_Terreno_VT → Grass Output)

A densidade de cada grass type é multiplicada pela máscara:

```
tufos   = saturate((ruído R do T_Noise_1_Mask na escala TufoEscala − TufoLimiar) × TufoContraste)
áreas   = lerp(AreaMin, 1, saturate((ruído G na escala AreaEscala − 0,4) × 2,5))
máscara = tufos × áreas × (1 − saturate(Layer 4 × 1,6))   ← a trilha (Layer 4) fica sem grama
```

Valores aprovados (parâmetros em `MI_Tut_TerrenoBase`, grupo "Tufos da grama"):

| Parâmetro | Valor | Efeito |
|---|---|---|
| TufoEscala | 120 cm | tamanho dos tufos (~0,5–1,5 m) |
| TufoLimiar | 0,36 | quanto maior, mais chão aparece |
| TufoContraste | 5,0 | borda dos tufos |
| AreaEscala | 420 cm | manchas mais cheias e mais vazias (2–5 m) |
| AreaMin | 0,6 | densidade mínima nas áreas vazias |

- Meta: 25–35% de chão visível entre os tufos, conferida **no jogo, no preset Médio**, na visão do jogador, como no Zenkai. O editor mostra a grama mais cheia, porque usa densidade 100%.
- **Histórico:** os espaços sumiram duas vezes:
  1. Lâminas mais largas (XY 1,1–1,4) com limiar baixo (0,12) fecharam os buracos.
  2. A versão antiga tinha só uma escala de ruído (650 cm).

  Se mexer em XY ou na densidade, reajuste o TufoLimiar e confira os espaços.

## Cor

| Item | Parâmetro | Valor |
|---|---|---|
| Chão entre os tufos (mesmo tom da grama, um pouco mais escuro; a base dos tufos se funde com ele) | `MI_Tut_TerrenoBase` → TL Fsbf Grass 1 e 2 Custom Color | (0,17; 0,43; 0,08), Brightness 2,0 |
| Textura do chão (pouco contraste, detalhe em escala grande) | TL Fsbf Grass 1 e 2 Tilling Near / Far, HueShift Strenght; Macro Contrast | 900 / 3000, 0,04; 0,12 |
| Base das lâminas | vem do chão, pelo RVT (Use RVT as Base Color = on) | — |
| Ponta das lâminas (verde natural, levemente amarelada) | `MI_Tut_Grama` → Top Color | (0,28; 0,55; 0,12) |
| Contraste do degradê | `MI_Tut_Grama` → Grass RVT Vertex Paint Gradient Contrast | 1,0 |

A cor do chão também muda a base das lâminas, porque a grama lê o RVT do terreno. Por isso não há linha entre o tufo e o chão: os dois usam exatamente a mesma cor.

- **Histórico:** um chão verde-oliva escuro, (0,15; 0,40; 0,10) com brilho 1,25, foi recusado porque parecia mancha ou buraco. O chão deve ficar a ~85–90% do brilho da grama, com o mesmo tom.

## Material, normais, sombra e AO

- `M_Tut_Grama_Master`: Default Lit, dupla face. Normal map desligado. Specular 0 e Roughness/AO do passe de grama aprovados no commit `4c33339`.
- `MI_Tut_Grama`: Pixel Depth Offset 0 e PDO Dither 0. Isso tirou o pontilhado.
- Grass types: Cast Dynamic Shadow **off** e Cast Contact Shadow **off**. Era o contact shadow que escurecia o miolo dos tufos.
- Sem AO extra na grama. O AO macio do bloco 4 do visual é só em troncos e rochas.

## Vento (MPC_Wind), aprovado pelo usuário

| Parâmetro | Valor |
|---|---|
| WindBase | 2,0 cm |
| WindGust | 6,5 cm |
| GustSpeed | 300 cm/s |
| GustScale | 6000 cm |
| GustThreshold | 0,55 |
| SwaySpeed | 1,2 rad/s |
| WindDirection | (0,8; 0,6; 0) |

O WPO do `M_Tut_Grama_Master` e do `M_Tut_Flores_Master` lê esses valores. Árvores e arbustos grandes ficam sem vento.

## Pós-processo que afeta a cor da grama (visual Zenkai, aprovado)

Saturação 0,9 e contraste 0,95, exposição 1,4, Slope 0,82, ganho de sombras (1,05; 1; 0,88), ganho de luzes (1,03; 1; 0,97). Ficam no PostProcessVolume do L_Tutorial.
