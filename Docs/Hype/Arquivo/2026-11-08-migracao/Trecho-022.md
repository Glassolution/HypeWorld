---
status: arquivado
atualizado: 2026-11-08
---
# PLANO POR FASES — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
  - Fluxo: o jogador escolhe o golpe (ChooseAttack) e dispara a habilidade por evento com o id no EventMagnitude (o servidor recebe junto e toca o mesmo); bRetriggerInstancedAbility corta o golpe anterior. Clique durante o golpe fica guardado (ComboInputWindow) e vale na janela; depois do fim, a sequência continua por ComboChainGrace (0,35 s) se o golpe não foi interrompido.
  - Golpes andam pela própria animação (RootMotionFromMontagesOnly só durante golpes; ProcessAttackRootMotion deixa só o avanço para a frente × RootMotionScale e para de avançar com alguém à frente a menos de AttackBodySpacing (85 cm) — corpos não se atravessam). Punhos fechados em golpes/defesa: dedos copiados de MM_Attack_01 (ABP_Ninja → FistHandsPose); fora disso, dedos relaxados da MM_Idle. Acerto: servidor aplica dano + atordoamento (GE) + empurrão + MulticastHitReaction (Rib Hit leve / Stomach Hit forte); o atordoamento bloqueia golpes, andar, pular e dash. Defesa (bGuarding replicado): de frente passa 15%; golpe com bBreaksGuard quebra (metade do dano, 1,1 s atordoado). Pose de defesa procedural (NinjaAnimInstance, Ninja|Defesa). HUD: contador "X GOLPES".
  - **PvP justo (24/10/2026):** apanhou = atordoado (tag Ninja.State.Stunned): não ataca, não carrega chakra, não anda/pula; **jutsus novos devem pôr Ninja.State.Stunned em ActivationBlockedTags** (ou checar `CanAct()`). No servidor (ApplyHit) cada golpe seguido no mesmo alvo atordoa ×0,9 (mín. 45%) e tira −8% (mín. 50%); no `MaxComboHits` (7) ou depois de `MaxComboTime` (3,5 s) apanhando o golpe arremessa (750/350) e o alvo fica `ComboBreakInvulnerability` (1,2 s) invulnerável ("COMBO QUEBRADO"; `bInvulnerable` replicado, golpes passam direto). **Substituição (Kawarimi):** Q apanhando = `TrySubstitution` → servidor tira o atordoamento, teleporta 350 cm para longe de quem bateu, 0,5 s invulnerável, tronco (cilindro marrom com física, 2 s) + nuvem; `MaxSubstitutions` 2 cargas, 1 volta a cada 8 s (HUD "Substituição X/2", replicado só ao dono). Golpes dão um passo automático à frente nos primeiros 0,22 s (`AttackLungeTime`, para quem apanha não sair do alcance; para com alguém colado). Atordoamento dos leves (0,75/0,85/0,95) dimensionado para a corrente de 4 golpes sempre encaixar (5 acertos); depois disso a vítima se solta. Fim do atordoamento: a animação de apanhar sai assim que ela se mexe. Ajustes: BP_ThirdPersonCharacter → Ninja|Combate|Fuga. Teste: `Tools/editor/teste_pvp_justo.py` (variável LIMITE antes do código baixa o limite de combo só no teste).

~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
