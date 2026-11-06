# Guia para continuar os trajes modulares no Codex (Grupo 3)

Estado em 03/11/2026, último commit `e5bd76d`. Leia também `CLAUDE.md`, seção "Trajes modulares do Indra".
"Medido" = conferi nos arquivos. "Não testado" = é o caminho previsto; ninguém rodou ainda.

## 1. O que é o Grupo 3

| Ordem | Traje | Fonte | Nome a usar |
|---|---|---|---|
| 1 | Sasuke low poly | `SourceAssets/Customization/Hair/naruto__sasuke_low_poly__rig__texture.glb` | `SasukeLowPoly` |
| 2 | Naruto low poly | mesmo GLB | `NarutoLowPoly` |
| 3 | Naruto Hokage (por último) | `SourceAssets/Customization/Hair/naruto.zip` → `source/Naruto Shippuden.fbx`, `textures/main.png`, `hat.png`, `cape.png` | `NarutoHokage` |

**Não use o nome `Sasuke`:** `SK_Outfit_Sasuke` é o traje já aprovado e seria sobrescrito.

Fora daqui: trajes Meshy (Blue Shinobi, Headless Armor, Teal Tactical, Shadow Striker). Shisui já foi pulado e não deve ser retomado sem pedido do Felipe.

## 2. Regras do Felipe

**Não tocar:** cabeça, crânio, anel do pescoço, Skeleton, ABP e animações do Indra, Itachi aprovado, arquivos originais, combate e mapas.

Para cada traje:
1. Cortar no anel padrão do pescoço, com encaixe nas arestas e normais copiadas da cabeça.
2. Normalizar altura e ombros para o Indra, sem distorcer o desenho.
3. Passar os pesos para o Skeleton ORIGINAL do Indra.
4. Criar a máscara de pele (braços, mãos e pernas expostos) usando o `SkinTone` do rosto.
5. Importar como `SK_Outfit_<Personagem>` e registrar em `DT_CustomizationParts`, slot Traje.
6. Testar parado, correndo e atacando, com close na emenda do pescoço.
7. Dar nota de 1 a 5 em relação à cabeça do Indra (resolução, textura, combina ou destoa).

Regras de grupo:
- Commit ao fim de cada traje aprovado; push ao fim do grupo.
- Traje travado por mais de 2 tentativas: pular, anotar o motivo e seguir.
- Nota 1 ou 2 nos Grupos 3 e 4: importa, mas com `Oculto=true`.
- **Pare ao fim do Grupo 3** e espere o OK do Felipe antes do Grupo 4.

**Entrega do grupo (máximo 8 linhas):** tabela `traje | status | nota | problema`, foto de cada traje (frente, costas e close do pescoço) e hash do commit.

Regras gerais: responder em português simples, nunca apagar assets (mover para `/Content/_ToDelete/`), não gastar créditos do Higgsfield sem aprovação, não dizer "pronto" sem prova.

## 3. Ferramentas

Tudo no Mac do Felipe. Não precisa de Higgsfield nem de Blender MCP.

| Ferramenta | Para quê | Comando |
|---|---|---|
| Blender 5.2.3 em segundo plano | Gerar o traje | `blender -b --factory-startup --python Tools/customization/traje_itachi_piloto.py -- --personagem <Nome>` |
| Unreal 5.8.4 + Python Remote Execution | Importar, registrar, testar | `python3 -I Tools/ue_exec.py -f <arquivo.py>` (editor aberto) |
| UnrealBuildTool | Só se mudar C++ | `Build.sh`, **com a Unreal totalmente fechada** (compilar com ela aberta já causou crash) |
| ffmpeg (`/opt/homebrew/bin`) | Juntar fotos | `hstack` / `vstack`, sem `drawtext` |

Repositório oficial: `/Users/luisfelipexavier/Documents/Unreal Projects/HypeWorld`, branch `main`. Edite sempre aqui; `Desktop/Hype World` é cópia.

## 4. Como o processo funciona

**Blender, `Tools/customization/traje_itachi_piloto.py`:**
- Encaixe de cada osso: `p -> T + Q*A*(p-H0)`. O comprimento do osso muda; a grossura usa uma escala única (`ESCALA`), então o desenho não deforma.
- `ESCALA = min(cabeça-pélvis do Indra / da origem, 1,8 / ALTURA_ORIGEM)`. Por isso o "6 m" deve se resolver sozinho, desde que `ALTURA_ORIGEM` esteja certa.
- Pesos: `MAPA` renomeia, `REPARTE` divide braço e perna entre os ossos auxiliares do Indra. Osso sem equivalente (`conhecido()`) segue o primeiro pai que tiver equivalente.
- Pescoço: corta em `CORTE_Z - FAIXA` e costura uma faixa até o anel da cabeça (36 vértices em 1,50 m, `anel_pescoco.json`), com mesma posição, pesos e normais. `FAIXA` vale 0,0225 para quem tem gola inteira.
- Costuras do rip: malhas extraídas de jogos são triângulos soltos. Arestas abertas com par nas duas pontas, até 4 mm (`COSTURA`), recebem a mesma posição e os mesmos pesos, sem soldar.
- Pele: máscara por cor das texturas, salva como `T_<Nome>_SkinMask_<n>.png`, e `info.json` com a cor de referência.
- Saída: `SourceAssets/Customization/IndraModular/<Nome>/`.

**Unreal, importar e registrar** (no editor, por um arquivo temporário):
```python
PERSONAGEM = 'SasukeLowPoly'
TEXTURAS = ['/Game/HypeWorld/Personagens/SasukeLowPoly/T_...']   # uma por slot, nesta ordem
exec(open('<repo>/Tools/editor/importar_traje_modular.py', encoding='utf-8').read())
```
O importador recusa a malha se a bind pose diferir do `SK_Indra` em 0,1 cm ou mais. Depois:
```python
PERSONAGEM = 'SasukeLowPoly'; NOME_MENU = 'Sasuke (low poly)'; OCULTO = False
exec(open('<repo>/Tools/editor/registrar_traje_catalogo.py', encoding='utf-8').read())
```
**O catálogo só é lido quando o editor abre:** reinicie o editor depois de registrar.

**Teste:** `Tools/editor/rodar_teste_traje.sh <Nome>` deixa as fotos em `Saved/TesteTraje/<Nome>/` (use `congelado` como 2º argumento para comparar duas versões no mesmo instante do golpe). Veja as fotos de verdade antes de aprovar; o log não mostra fios nem frestas. Copie as aprovadas para `Docs/Personalizacao/IndraModular/Trajes/<Nome>/frente_costas_pescoco.png`.

**Menu real (opcional):** o jogo aberto com `-TrocaTrajeShots` troca os trajes pela mesma função das setas e registra as malhas visíveis no log. O esperado são 4 visíveis: cabeça, casca dos pés (`SK_Base_Body`), traje e cabelo.

## 5. O que muda no script para o Grupo 3

O script ainda não aceita essas fontes. Ajustes previstos (não testados):

**Nos dois casos:**
- Ele pega UMA malha (`src = next(...)`), e estas fontes têm uma malha por material: é preciso juntar as do corpo antes (`bpy.ops.object.join`).
- Acrescente os personagens em `ORIGENS_FBX` e `RENOMEAR` sem mudar o caminho dos que já existem.
- As texturas ainda não existem na Unreal, e o importador espera `.uasset`. Coloque os PNG em `SourceAssets/Customization/IndraModular/Origem/<Nome>/` e importe para `/Game/HypeWorld/Personagens/<Nome>/` antes do `importar_traje_modular`.
- `ALTURA_ORIGEM` tem que medir o personagem inteiro, sem chapéu. Medir só o que sobrou da malha já mudou o Itachi aprovado sem avisar.

**Low poly (GLB, medido):**
- Ossos 568 (Rigify), dos quais 73 são `DEF-`; `MCH-`, `ORG-` e `VIS_` não deveriam ter peso (confirme). O rig tem escala 100.
- Duas skins no mesmo arquivo: `Object_575`–`579` = Sasuke (skin 0) e `Object_1152`–`1157` = Naruto (skin 1). O `arm_i = next(...)` pegaria sempre o primeiro: escolha a armature pela malha.
- Os nomes dos ossos têm um número no fim e ele muda entre as skins (`DEF-spine_017` / `DEF-spine_0429`). Tire esse sufixo antes de renomear.
- Materiais do Sasuke: `tex2_2`, `tex2_3`, `tex2_4` (corpo), `tex1_1` (faixa de altura da cabeça), `Outline` (descartar).
- Materiais do Naruto: `tex01`, `tex02`, `hands`, `palms` (corpo), `face`, `Outline` (descartar). Descarte também o `Cylinder`.
- `tex2_3` e `tex01` chegam ao topo do personagem, então podem trazer cabelo ou cabeça junto: é o mesmo risco que reprovou o Shisui. Olhe isso primeiro.
- Mapa de ossos sugerido (não testado):

| Rigify | Rig do script |
|---|---|
| `DEF-spine`, `.001`, `.002`, `.003` | `pelvis`, `spine_01`, `spine_02`, `spine_03` |
| `DEF-spine.004`, `.005`, `.006` | `neck_01`, `neck_02`, `head` |
| `DEF-shoulder.L`, `DEF-upper_arm.L`, `DEF-forearm.L`, `DEF-hand.L` | `clavicle_l`, `upperarm_l`, `lowerarm_l`, `hand_l` |
| `DEF-thigh.L`, `DEF-shin.L`, `DEF-foot.L`, `DEF-toe.L` | `thigh_l`, `calf_l`, `foot_l`, `ball_l` |
| `DEF-thumb.01.L`, `DEF-f_index.01.L` etc. | `thumb_01_l`, `index_01_l` etc. |
| `DEF-palm.01..04.L` | `index/middle/ring/pinky_metacarpal_l` |

Igual para `.R`. Os `.001` de braço e perna ficam sem nome novo e seguem o pai. Confira a posição de cada junta antes de confiar: a pélvis e a coluna são a parte mais provável de errar.

**Naruto Hokage (FBX, medido):**
- 243 ossos; altura com chapéu 6,70 m, topo do cabelo 6,57 m. Malhas: `Upper Body`, `Lower Body`, `Hokage Cape`, `+Hokage Hat`, `Face`, `Hair`, `Eyes`, `Teeth`, `Tongue`. Materiais com o mesmo nome da malha.
- Corpo: `Upper Body`, `Lower Body`, `Hokage Cape`. Use 6,57 m como altura.
- Ossos: `pelvis`, `spine lower`, `spine upper`, `head neck lower`, `head neck upper`, `arm left shoulder 1` (clavícula), `arm left shoulder 2` (braço), `arm left elbow`, `arm left wrist`, `arm left finger 1a..5c`, `leg left thigh`... Liste o restante das pernas no Blender e confirme qual dedo é o polegar.
- Capa: 44 ossos, sem física no projeto. Devem cair no pai (coluna), então ela anda dura e pode atravessar as pernas ao correr. É limitação a registrar na nota, não motivo para adicionar física.
- Chapéu: 37 ossos e fica acima do corte do pescoço, então o script o removeria. Faça a 1ª versão sem chapéu e avise no relatório; encostar na cabeça é proibido.

**Antes de cada commit, confira que não quebrou os aprovados:** regenere e compare com o FBX do commit, depois restaure.
```bash
cp SourceAssets/Customization/IndraModular/ItachiAnbu/SK_Outfit_ItachiAnbu.fbx /tmp/ref.fbx
blender -b --factory-startup --python Tools/customization/traje_itachi_piloto.py -- --personagem ItachiAnbu
blender -b --factory-startup --python Tools/customization/comparar_fbx_traje.py -- /tmp/ref.fbx "$PWD/SourceAssets/Customization/IndraModular/ItachiAnbu/SK_Outfit_ItachiAnbu.fbx"
git checkout -- SourceAssets/Customization/IndraModular/ItachiAnbu
```
Tem que dar `v 4105 4105 f 3835 3835 desloc max mm 0.0`. Repita com `KakashiAnbu` (`v 4103 4103 f 3823 3823`, 0.0 mm).

## 6. Critérios de aprovação

- **Reprova:** cabeça, dentes ou língua na malha; gola em cacos; pedaços soltos ou brancos; emenda abrindo em qualquer pose; bind pose diferente.
- **Aceitável, mas registrar:** gola cortada reta atrás, capa dura.
- **Nota:** 5 = parece feito para o Indra; 4 = mesma resolução (cerca de 512 px) e estilo, com detalhe pequeno; 3 = funciona, destoa um pouco; 1–2 = destoa muito ou tem defeito visível (vai oculto). A régua é minha, derivada do pedido; o Sasuke levou 4 como proposta.

## 7. Armadilhas que já aconteceram

- **Editor em 2º plano roda a 3 FPS:** antes de abrir, corrija `bThrottleCPUWhenNotForeground=False` em `~/Library/Application Support/Epic/UnrealEngine/5.8/Saved/Config/MacEditor/EditorSettings.ini`, que é regravado ao fechar.
- **Memória (8 GB):** evite Blender pesado e Unreal juntos, e nunca compile com a Unreal aberta.
- **Foto:** `screencapture` falha; use o comando `Shot` no Play (`Shot showui` para pegar a interface).
- **Montagem de ataque:** fica em `CharacterMesh0` e dura menos de 0,5 s; congele no 1º tick em que aparecer.
- **BMesh:** não use vértices depois de apagá-los (`ReferenceError`). Apague ilhas da cabeça antes de procurar o laço do corte.
- **Corte alto demais** deixa um dente na gola de costas: foi o que levou a `FAIXA` 0,0225.
- **Material cinza quadriculado** = não compilou no Metal; veja `~/Library/Logs/Unreal Engine/HypeWorldEditor/HypeWorld.log`. Instâncias precisam de uso SkeletalMesh e da chamada `SetMaterials`.
- **Git:** `Content/Maps/Tutorial/L_Tutorial.umap` tem alteração que não é deste trabalho: nunca adicione ao commit. Adicione arquivos por caminho, nunca `git add -A`.
- **zsh:** coloque `==` e padrões com `*` entre aspas.
- **`ue_exec`:** falha se o texto do script tiver ".py" antes do código.

## 8. Ordem de trabalho

1. Conferir `git status` e `git log`.
2. Ajustar o script para o `SasukeLowPoly` e gerar. Olhar uma foto no Blender (`--fotos <pasta>`) antes de importar.
3. Importar, testar, ver as fotos, dar a nota, registrar (`OCULTO=True` se 1–2), reiniciar o editor e confirmar no log `menu:`.
4. Rodar a comparação do Itachi e do Kakashi, e fazer o commit.
5. Repetir com `NarutoLowPoly` e depois `NarutoHokage`.
6. Atualizar `CLAUDE.md`, fazer o push e entregar. **Parar.**

Mensagem de commit em português, explicando o que mudou. Dizer no relatório o que não foi testado (2 jogadores, jogo final).

## 9. Grupo 4 (só depois do OK)

Fontes: `naruto.glb`, `naruto_sage.glb`, `naruto_shippuden-_naruto.glb` (em `SourceAssets/Customization/Hair/`) e o Sasuke Wii (`~/Downloads/naruto-shippuuden-sasuke.zip`, FBX + `ssk_tex01/02`).

**Medido: os três GLB não têm esqueleto** (0 skins, altura abaixo de 1 unidade). O processo atual parte de pesos existentes, então será preciso criar pesos por proximidade ou corrigir a fonte. Esse caminho não existe ainda, e pode ser o motivo de o traje travar. O Sasuke Wii não foi inspecionado.
