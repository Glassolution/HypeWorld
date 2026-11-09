---
status: arquivado
atualizado: 2026-11-08
---
# 5. O plano (pipeline) — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
## 5. O plano (pipeline)
0. **Atualização 24/10:** o dash já foi feito (SayMotion → recorte; ver CLAUDE.md). O Higgsfield MCP também tem 3D: personagem a partir de imagem/texto, esqueleto automático e animações de uma **biblioteca pronta de ~678 clipes** (busca com `animation_actions`, prévia em GIF), saída GLB. Primeiro passo útil numa sessão nova: buscar na biblioteca estrela/mortal/pulo com giro e mostrar as prévias ao Felipe (checar antes se a busca gasta crédito).
1. **Higgsfield (MCP já conectado: `claude mcp list` → `higgsfield ... ✔ Connected`)** gera um **vídeo realista** de uma pessoa fazendo o movimento. Primeiro listar as ferramentas/modelos que o MCP oferece de verdade (a página oficial fala de "30+ modelos de imagem e vídeo"; guias de terceiros citam generate_image, generate_video, create_character, get_generation_status, list_characters — **confirmar**).
   - Começar por **UM** movimento: o **dash/estrela**. Mostrar modelo + custo e esperar aprovação.
   - O vídeo tem que passar nas regras do DeepMotion (abaixo): pessoa real (não desenho), vista **de lado**, **corpo inteiro o tempo todo**, **roupa justa** (sem casaco/saia), **câmera parada**, fundo liso e contrastante, boa luz, 1080p+, 30 fps+, uma pessoa só, sem cortes. Exemplo de prompt (ajustar ao modelo): "Realistic live-action video, side view, static camera, full body visible head to toe, a male athlete in tight dark clothes sprints from left to right and performs a fast front handspring: dives forward with arms open, both hands touch the ground, legs straight up, flips over and lands on one foot, then keeps running. Plain light gray studio background, even lighting, 1080p, 30fps, no camera movement, no cuts."
2. **DeepMotion Animate 3D** (conta Freemium do Felipe já criada e logada: 25 créditos/mês + bônus, saldo visto: 55; ~2 créditos para 1,2 s; clipes até 20 s; 30 fps; licença comercial indisponível = ok, jogo privado). Felipe sobe o vídeo no site (portal → Animate 3D → Launch). Configuração usada: Single person; Physics Filter desligado; Hand/Head tracking desligados; Upper Body Only desligado; Foot Locking Auto; MP4 Output desligado. Se pedir downsample para 30 FPS, aceitar. Baixar em **FBX** → `Desktop/Hype World/Mixamo/Captura/`.
   - O vídeo de gameplay foi **recusado** pelo DeepMotion ("doesn't meet the requirements"): personagem de jogo, casaco cobrindo o corpo, câmera em movimento. Não tentar de novo com ele.
3. **Eu coloco no jogo:** importar o FBX e converter para o manequim pelo mesmo caminho das animações do Mixamo. Modelo pronto para copiar: `Tools/editor/importar_luta.py` (importa com o esqueleto `/Game/Ninja/Characters/Mixamo/SK_XBot_Skeleton` via Interchange e converte com `/Game/Ninja/Characters/Retarget/RTG_XBot_Para_Manequim`). **Atenção:** o FBX do DeepMotion pode ter outro esqueleto (não X Bot) — verificar os nomes dos ossos antes; se não bater, criar um IK Rig/Retargeter para ele (ver `Tools/editor/retarget_mixamo.py`).
   - Analisar a animação convertida quadro a quadro (posições de mão/pé/quadril com `unreal.AnimationLibrary.get_bone_pose_for_time`, compondo a cadeia de ossos — ver o método em CLAUDE.md, seção Combate PvP) e comparar com o vídeo de referência.
   - Encaixar no **dash**: hoje o dash é o modo de movimento `NMOVE_Dash` em `Source/HypeWorld/Ninja/NinjaMovementComponent` e **segue o root motion** de `DashRootMotionAnim` (hoje `MM_Dash`; `DashAnimRate` 1,2, `DashAnimMoveEnd` 0,75, `DashDistanceScale` 0,7); a montagem visual é `DashMontage` no personagem (`AHypeWorldCharacter::OnMovementModeChanged`). Trocar a animação = criar montagem da nova animação e apontar `DashRootMotionAnim` + `DashMontage` para ela (ajustar tempos). O dash dá invulnerabilidade (PvP).
   - Depois repetir para o **pulo** (animações no ABP_Ninja: Jump = `A_Ninja_MX_Jumping_Up`, Fall Loop = `A_Ninja_MX_Falling_Idle`; nós do ABP são trocados por `export_text/import_text` como em `Tools/editor/criar_locomocao_mixamo.py`) e, se valer, a **corrida**.
4. Plano B se o Higgsfield não der bom resultado ou acabarem os créditos: **SayMotion** (texto → animação 3D, mesma conta DeepMotion).


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
