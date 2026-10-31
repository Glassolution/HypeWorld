# Personalização — corpo base, roupas e cabelos

## 1. Corpo base atual

**`/Game/HypeWorld/Characters/Base/SK_Base_Body`**, fonte `Content/HypeWorld/Characters/Base/SK_Base_Body.fbx`. É a **etapa 10, de 30/10/2026**.

O que mudou em relação à versão anterior:

- **Ombros:** mais largos, com 2,40 larguras de cabeça no idle. A linha do pescoço até o ombro desce cerca de 21°, o deltoide ganhou volume na lateral do braço e as costas foram suavizadas.
- **Cabeça:** 5% menor, em volta da junta da cabeça. O rosto mantém as proporções.
- **Ossos:** a clavícula e o braço inteiro (upperarm e filhos) andaram para fora, e os ossos da cabeça escalaram junto. Nomes, hierarquia e orientação (roll) continuam os mesmos.
  - Consequência: **a bind pose não é mais a do Indra** (`Docs/PadraoPersonagem/Indra_Referencia.fbx`) nesses ossos.
  - O Skeleton é o mesmo (`SKEL_Indra`), e a pose de referência dele não foi alterada.

Outros arquivos do corpo:

- **Texturas:** `T_Base_Body_BaseColor` e `T_Base_Body_Normal` vêm de `ferramentas_ia/corpo_base/shading_novo/*_limpo.png`. São os PNG do Meshy sem os riscos e com o vão entre as ilhas de UV preenchido. Cor e UVs não mudaram.
- **Máscara do couro cabeludo:** `T_Base_Body_ScalpMask` vem de `Customization/Masks/T_Base_Body_ScalpMask.png`. Ela foi refeita sem vazar para o braço, o short e a perna.
- **Olhos:** são projetados pelo material (`MI_Base_Body`: `EyeCenterX` 4,18, `EyeCenterZ` 161,66, raios ×0,95 para a cabeça nova).
- **Blender:** `ferramentas_ia/corpo_base/blend_novo/etapa10_ombros.blend`. Os parâmetros estão em `etapa10_params.json`.

**Regra:** toda roupa e cabelo tem que ter a bind pose do SK_Base_Body, não a do SK_Indra.

- Use `indra_character_standard.validate_mesh(peça, SK_Base_Body)`.
- Sem o segundo argumento, a função compara com o padrão do Indra e **recusa** o corpo novo. Por isso, `apply_mesh` no SK_Base_Body também recusaria.

## 2. Onde estão os scripts

| Script | Faz o quê |
|---|---|
| `Tools/customization/ajustar_cabelos.py` | Encaixa cabelos dos GLB de `Hair/` na cabeça do corpo base e grava `Fitted/<id>/<id>.fbx` |
| `Tools/customization/fit_indra_cosmetics.py` | Encaixe original dos trajes Uchiha dos ZIP em `Clothes/`. Os cabelos ali foram substituídos pelo script acima |
| `Tools/customization/gerar_mascara_couro.py` | Máscara do couro cabeludo na UV do corpo |
| `Tools/editor/importar_cosmeticos_indra.py` | Importa todas as peças de `Fitted/`, cria os MI e copia a bind pose do SK_Base_Body |
| `Tools/editor/reimportar_cabelos.py` | Reimporta cabelos (`IDS=[...]`) por cima, com o mesmo MI e a bind pose do SK_Base_Body |
| `ferramentas_ia/customizacao/ombros_pecas.py` | Reajusta **todas** as peças existentes quando o corpo muda (campo de deslocamento do corpo + Surface Deform + pesos) |
| `ferramentas_ia/customizacao/corrigir_cabelos_andar.py` | Cabelo comprido que entra no tronco nas poses reais do jogo (`corpo_base/poses_jogo.json`): empurra para fora |
| `ferramentas_ia/corpo_base/etapa10_ombros.py` | Gera o corpo base atual a partir da etapa 9 |
| `ferramentas_ia/corpo_base/limpar_riscos.py` | Limpa os riscos da textura (braço/short), sem mudar cor nem UV |
| `ferramentas_ia/corpo_base/ue_ombros_importar.py` | Importa na Unreal o corpo e as peças de `saida_ombros/` (Interchange, slots de material devolvidos) |
| `ferramentas_ia/corpo_base/ue_ombros_olhos.py` | Ajusta os olhos do material quando a cabeça escala. **Rodar uma vez só** |

Os scripts de encaixe têm no topo qual corpo base usam como referência.

As versões antigas (FBX, máscara, texturas e este README) ficam em `SourceAssets/_Old_Versions/<data>_<motivo>/`, com os mesmos caminhos.

## 3. Encaixar uma peça nova

### Cabelo

1. Coloque o GLB em `SourceAssets/Customization/Hair/`.
2. Rode, olhe as fotos e tire o `--sem-fbx` quando estiver bom:
   ```
   blender -b --factory-startup --python Tools/customization/ajustar_cabelos.py -- --ids Hair_<nome> --fotos <pasta> --sem-fbx
   ```
3. Se for comprido, confira as poses reais do jogo com `ferramentas_ia/customizacao/corrigir_cabelos_andar.py`. Antes, a peça precisa estar em `pecas_depois.blend`.
4. Leve para a Unreal:
   - **Peça nova:** `Tools/editor/importar_cosmeticos_indra.py` e uma linha em `Source/HypeWorld/Ninja/HypeFittedCatalog.inl` (`AddFitted`). Depois compile.
   - **Peça que já existe:** `Tools/editor/reimportar_cabelos.py` com `IDS=['Hair_<nome>']`.

### Roupa

1. Coloque o ZIP do Meshy (T-pose, sem cabeça) em `SourceAssets/Customization/Clothes/`.
2. **Antes do encaixe, confira se a peça está centrada no próprio corpo.** O `fit_indra_cosmetics.py` usa o meio da caixa como centro do corpo. Veja a seção 5 (Shadow Striker).
3. Rode o `fit_indra_cosmetics.py`. Ele refaz todas as peças do zero; para uma só, copie o trecho do traje.
4. Rode o `importar_cosmeticos_indra.py`.
5. Inclua a peça no `HypeFittedCatalog.inl`, com a `FNinjaBodyMask`: a parte do corpo que o material esconde por baixo da roupa.

### Em todos os casos

- Teste no Play com as poses parado, andando e correndo.
- O personagem usa a peça por Leader Pose do corpo visível.

### Se o corpo base mudar de novo

1. Gere o corpo novo.
2. Rode `ombros_pecas.py` para mover todas as peças junto.
3. Importe com `ue_ombros_importar.py`.
4. Mova as versões antigas para `_Old_Versions`.

## 4. Camada de postura

Fica em `Source/HypeWorld/Ninja/NinjaAnimInstance.cpp`, no `FNinjaAnimInstanceProxy::ApplyPosture`. Os ajustes ficam no **ABP_Ninja → Class Defaults → categoria "Postura"**.

- Corrige a pose **depois** da locomoção (parado, andar, correr).
- Some sozinha durante montages (golpes, jutsus), dash, modo de batalha, defesa e carga de chakra.
- Postura|Coluna:
  - `PosturaSpine01..05` e `PosturaNeck01/02`: graus, + = para trás.
  - `PosturaColunaEmMovimento`: quanto da coluna vale andando.
- Postura|Cabeça: `PosturaOlharHorizontal` deixa o olhar reto.
- Postura|Ombros:
  - `PosturaClaviculaSobe` e `PosturaClaviculaTras`.
  - `PosturaBracosMantem`: braço pendurado como na animação.
  - `PosturaBracosFora`: 3,5°, só parado. Deixa o vão entre o braço e o tronco.
- Postura|Andar:
  - `PosturaAndarAlpha` e `PosturaAndarBracosDentro` (10°).
  - `PosturaAndarPescoco` e `PosturaAndarOlharHorizontal`.
  - `PosturaAndarVelocidadeMax`.
- `PosturaAlpha` (0..1) liga e desliga tudo.
- `A_Idle_Relaxed` cuida só de braços, mãos e dedos. A cabeça e o pescoço estão nesta camada.

## 5. Problemas conhecidos

- **`Uchiha_Shadow_Striker_Outfit` (quebrado desde o primeiro encaixe):**
  - **Causa:** o ZIP original não está centrado no corpo. A linha do meio do tronco e das pernas fica ~17–19 cm para o lado direito do personagem, e os braços não são simétricos em relação a ela. Como o `fit_indra_cosmetics.py` centra pelo meio da caixa (`(lo.x+hi.x)/2`), metade do tronco direito cai na zona que ele trata como manga (|x| > 0,14 e z > 1,12) e é girada numa aba gigante.
  - **Efeito:** o lado direito do short, o coldre e as faixas da perna ficam 20 a 45 cm fora do eixo (pedaço da perna direita em x −0,70…−0,30 m contra 0…0,30 m na esquerda).
  - Além disso, a redução de 720 mil para 45 mil triângulos (6%) deixa coldre, faixas e braçadeiras serrilhados.
  - **Para corrigir:** centrar pela linha do meio do tronco/pernas (não pela caixa), reduzir em mais passos ou com proteção das partes finas, e refazer pesos e máscara.
- **Antebraço:** ainda há linhas finas de costura de UV com a luz da Unreal. Vêm das normais de vértice calculadas na importação, não da textura; no Blender não aparecem.
- **Cabelos `sakura_young` e `wasabi`:** outra unidade/origem, encaixe manual (ver `ajustar_cabelos.py`).

## 6. Fontes (Windows e Mac)

- `Hair/` tem os 46 GLB e o ZIP de `Downloads/personagem para customizacao/Cabelos`, sem modificação. `zabusa (1).glb` e `zabusa.glb` são cópias idênticas.
- `Eyes/` tem as íris de Kakashi, Minato e Sasuke.
- `Clothes/` tem os ZIP dos trajes Uchiha.
- `Fitted/` tem as peças encaixadas no corpo atual.
- `Masks/` tem a máscara do couro cabeludo.
- O manifesto (`manifest.json`) registra nomes, tamanhos e SHA-256.
- No Mac, com Git LFS: `git pull origin main` e `git lfs pull`. Abra com a Unreal 5.8.4.
