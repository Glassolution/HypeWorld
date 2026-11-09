---
status: arquivado
atualizado: 2026-11-08
---
# PLANO POR FASES — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
  - **Combo aéreo e mortal para trás (24/10/2026):** Espaço no meio do combo (golpeando ou até ~0,6 s depois, no chão) = **lançador** (`Launchers` do estilo, id 400+; Punho de Chakra BeniHien_Uppercut, Taijutsu KickUp, Blindado ChakraGloves_B_Uppercut_Large): adversário lançado 1150 cm/s para cima (reação `LaunchReactMontage` = Beaten_SpinBlowOff_Large_Up) e o atacante sobe junto (`LauncherFollowUpSpeed`). **Regra do Espaço (atual, 24/10/2026, pedido do Felipe):** em modo de batalha (inclui os 6 s automáticos depois de bater/apanhar), no chão: S + Espaço = mortal para trás; **toque rápido = pulo normal**; **segurar Espaço `LauncherHoldTime` (0,2 s) = lançador**, também no meio do combo (`bLauncherHoldPending` + `UpdateLauncherHold`; soltar antes = `TapJump`; golpeando, o toque não pula); segurar Espaço + clicar = lançador na hora (`bSpaceHeld`). Teste medido: toque subiu 2,4 m sem montagem; segurar 0,45 s tocou AM_G_BeniHien_Uppercut_Combo sem sair do chão. Fora do modo de batalha Espaço = pulo/super pulo. Lançador (bBreaksGuard) abre a defesa e lança mesmo assim; no ar não existe defesa (ApplyHit só defende no chão) — o Boneco_Treino_2 (defende sempre) é lançado e apanha no ar. Lançador guardado no meio de um golpe não é apagado pelos cliques seguintes (antes era: o bug do "não levanta"). No ar, o ataque segue `AirAttacks` **em ordem** (3 golpes). **Suspensão:** cada acerto no ar deixa atacante e alvo com gravidade ×0,1 (`UNinjaMovementComponent::SuspendInAir`, `AirSuspendGravityScale`, override de `GetGravityZ`) por `AirSuspendTime` 1 s, renovada a cada acerto e a cada golpe aéreo seguinte (BeginAttack); pequeno impulso `AirHoverSpeed` 60 / `AirJuggleSpeed` 80, empurrão máx. `AirJuggleKnockback` 80; acaba ao pousar. **Último golpe aéreo** (reação KNOCKBACK, `IsAirFinisher`): arremessa 1500 para longe + 450 para cima, sem frear no ar (`bInSuperJump` no alvo), alvo `AirFinisherInvulnerability` 1,6 s invulnerável — medido: cai a ~16 m, sem golpes no chão. Suspensão/arremesso vão no `MulticastHitReaction` (bAirJuggle, bThrownAway). Reações no ar `AirHitReactMontages` (Beaten_Aerial_*). **Modo de batalha + S + Espaço** = `DoBackflipJump`: 1500 para cima + 280 para trás (sem frenagem no ar, `bInSuperJump`) ≈ 5,9 m de altura e 7 m para trás em ~2,1 s, `AM_Mortal_Tras` (DashStep_Behind 0,05–0,8, RateScale 0,75); o corpo fica de frente para a câmera o voo todo, mesmo segurando S (`bInBackflip` no movimento; antes virava para trás no ar). Ritmo mais acelerado: golpes leves rate 1,35, fortes 1,25, correndo/ar 1,3; DashCooldown 0,5; limite de combo 10 golpes / 5 s. Teste: `Tools/editor/teste_combo_aereo.py`.
  - Python: `unreal.Rotator(a, b, c)` é (roll, pitch, yaw) — usar sempre `unreal.Rotator(pitch=..., yaw=..., roll=...)`.
  - Testes: `teste_combo` contra bonecos e teste de rede (2 jogadores, listen server). Para testar rede por script, o cliente tem que "apertar a tecla" (EnhancedInputLocalPlayerSubsystem.inject_input_vector_for_action, achado por unreal.ObjectIterator); chamar funções do personagem do cliente direto pelo Python faz as mensagens para o servidor rodarem localmente (FEditorScriptExecutionGuard) e trava o editor em loop. Jogadores na rede: Play com 2 jogadores = `PlayNumberOfClients`/`PlayNetMode` em `Saved/Config/MacEditor/EditorPerProjectUserSettings.ini` (editor fechado).
- Editor regrava `bThrottleCPUWhenNotForeground=True` em `~/Library/Application Support/Epic/UnrealEngine/5.8/Saved/Config/MacEditor/EditorSettings.ini` ao fechar: corrigir o arquivo antes de abrir e desligar em memória após abrir, senão testes rodam a 3 FPS.
- Teste automático: `Tools/editor/teste_movimento.py` (com o jogo em Play). Com o editor em segundo plano o FPS cai muito e os números ficam imprecisos.
Pendências da Fase 0 (verificado em 23/10/2026):
- [x] Projeto C++ criado (HypeWorld)
- [x] Git LFS 3.8.1 instalado; repositório Git no projeto (commit `1e7bf48`). Remoto: https://github.com/Glassolution/HypeWorld (privado, conta Glassolution)

~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
