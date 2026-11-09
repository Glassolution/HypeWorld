---
status: arquivado
atualizado: 2026-11-08
---
# PLANO POR FASES — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
## PLANO POR FASES

Uma fase só começa quando a anterior estiver funcionando e testada.

| Fase | Objetivo | Entregas |
|---|---|---|
| **0 – Ambiente** | Ferramentas conversando entre si | Projeto C++ criado, Git + LFS configurados, MCPs de Unreal, Blender e Higgsfield conectados ao Claude Code |
| **1 – Movimento** | Andar e correr como ninja | Câmera em 3ª pessoa, corrida ninja, pulo duplo, dash, corrida em parede. Personagem placeholder, sem gastar crédito |
| **2 – Combate básico** | Bater num boneco de treino e sentir impacto | GAS configurado, combo corpo a corpo, desvio, vida e chakra, números de dano |
| **3 – Jutsus + VFX** | 3 jutsus marcantes | Um projétil (estilo bola de fogo), uma área (explosão) e uma transformação (estilo modo sábio), com efeitos no Niagara |
| **4 – Multiplayer** | Eu + 2 amigos lutando juntos online | Replicação de movimento, combate e jutsus; sistema de hospedar e entrar na partida |
| **5 – Personagem e progressão** | Criar e evoluir o próprio ninja | Criador de personagem, clãs, XP e níveis, árvore de jutsus, save |
| **6 – Mundo** | Explorar | Um vilarejo bem feito primeiro, depois a região aberta em volta, crescendo aos poucos |
| **7 – Arte final** | Visual definitivo | Troca dos placeholders por personagens e animações finais |

**Status atual:** Fase 2 (combate básico + combate PvP com combos) implementada e testada por script, inclusive em rede (host x cliente); aguardando teste jogando. Fase 1 concluída.
- **24/10/2026 — RECOMEÇO DAS ANIMAÇÕES DO ZERO (pedido do Felipe):** tudo voltou ao padrão da Epic por `Tools/editor/voltar_animacoes_padrao.py`: ABP_Ninja com MM_Idle, BS_Idle_Walk_Run, MM_Jump, MM_Fall_Loop, MM_Land; dash = MM_Dash/AM_Ninja_Dash; reações só MM_HitReact; golpes: combo leve MM_Attack_01/02/03 + soco carregado (chutes do Mixamo e golpe aéreo fora); sem idle de olhar; inclinação/aceleração da animação/acrobacias zeradas; **todos os retoques por código desligados** (ABP_Ninja → `bProceduralPose = false`: dedos, corrida ninja, parede, pouso, chakra, defesa, punho, braços abertos). Nada foi apagado — o que está descrito abaixo (Mixamo, DeepMotion, poses) continua no projeto, só desligado, e serve de referência para religar.
- **Packs do Fab (avaliação):** FBX baixados em `~/Downloads/*.zip` → extraídos em `Desktop/Hype World/Fab/<pack>/`. `Tools/editor/importar_pack_fbx.py` (1º FBX com malha cria o esqueleto do pack; todos entram como animação nesse esqueleto, 30 Hz + SNAP_TO_CLOSEST; IK Rig automático + correções (coluna até Chest, cadeia Neck própria) + RTG para o manequim; saída `/Game/Ninja/Characters/Packs/<Pack>/Anims/A_MF_*`). `Tools/editor/galeria_animacoes.py` monta uma galeria no Lvl_Treino (pasta do Outliner `Galeria/<Pack>`, área livre em x -2600..-1280, y -1100..1100, atrás do ponto de início, bonecos de frente para ele, nome numerado em cima, cada um em loop). Motifect Locomotion (45 clipes) importado — **gerado por IA (Motifect AI), não é captura real**; esqueleto Hips/Spine1/Spine2/Chest/Neck1/Neck2, pernas LeftLeg/LeftShin. Ainda baixados e não importados: motifect_combat_complete, motifect_martial_arts, combat_essentials_fight_mocap_pack.
- **Pack comprado "Anime Ninja Animation" (Update 1.0, 24/10/2026):** zip `~/Downloads/AnimeNinjaAnimationUpdate1.0.zip` (era um projeto UE 5.8 inteiro); só `Content/ANAP` foi copiado para o projeto (`/Game/ANAP`, 233 MB): 379 animações JÁ no esqueleto do manequim (`SK_Mannequin`) — sem retarget. Pastas: `Animations/Base` (81: corrida, andar, viradas, dash step 4 direções, desvio, pulo/pulo duplo, queda, pouso, parede, carregar chakra chão/ar, pulo de chakra, levantar), `Animations/Attack/HandBlades` (158: mão, chutes, chakra fist/claws/katana, aéreos, à distância), `Animations/Misc` (85: reações a golpes, voar longe, cair, atordoado, choque etc.), `Animations/Pose` (56: idles/poses de vitória de personagens). Animações SEM root motion ligado (no lugar). Mapas-galeria do próprio pack: `/Game/ANAP/ANAP_Demo` (324 bonecos) e `/Game/ANAP/ANAP_Update1` (57). Cópia extraída em `Desktop/Hype World/Fab/anime_ninja_update1/`.

~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
