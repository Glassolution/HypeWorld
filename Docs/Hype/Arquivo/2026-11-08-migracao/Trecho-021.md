---
status: arquivado
atualizado: 2026-11-08
---
# PLANO POR FASES — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
- **Dash = estrela (handspring) do SayMotion/DeepMotion:** FBX em `Mixamo/Captura/dash_estrela/` → `Tools/editor/importar_deepmotion.py` (2 passadas: modelo+esqueleto, depois só a animação com nome próprio; 30 Hz + SNAP_TO_CLOSEST, senão o Interchange recusa; editor fora do Play) → `Tools/editor/retarget_deepmotion.py` (IK_DeepMotion + RTG_DeepMotion_Para_Manequim, esqueleto hips/spine/leftarm... sem prefixo; mesma receita do Mixamo) → `A_Ninja_DM_DashEstrela` (2,67 s). Trecho usado 0,80–1,80 s (mergulho → ponta-cabeça → pousa de braços erguidos); antes é corrida lenta e depois ele "cai" — descartados. Montagem `AM_Dash_Estrela` (DashMontage). Movimento: DashRootMotionAnim = essa animação com DashAnimStart 0,80, DashAnimMoveEnd 1,0, DashAnimRate 1,33 (~0,75 s, como o vídeo), DashDistanceScale 1,1, DashMinSpeedFraction 0,75 (nunca mais lento que 75% da corrida); correndo, sai do dash já na velocidade da corrida.
- **Acrobacias (referência: vídeo do Felipe, analisado quadro a quadro com ffmpeg):** dash = cambalhota/estrela para a frente (mergulho → de ponta-cabeça com as pernas para cima → pouso), pulo do chão = giro no próprio eixo, segundo pulo = braços abertos, curva correndo = inclinação forte (MaxTurnLean 32). Feitas por código, só visuais (UpdateAcrobatics no personagem: gira a malha em volta do centro da cápsula — FQuat(RightVector, +ângulo) = cabeça para a frente/baixo, medido; sobe 20 cm no meio da volta; dash cortado termina a volta a 900°/s). Ajustes: BP_ThirdPersonCharacter → Ninja|Acrobacia (DashFlipDegrees, JumpSpinDegrees, JumpSpinDuration). Braços abertos: NinjaAnimInstance (ArmsSpreadAlpha). Para analisar vídeo: ffmpeg (/opt/homebrew/bin) extrai quadros; sem drawtext.
- Idle: MM_Idle (calma); AM_Ninja_IdleOlhar após 30 s parado. Dedos relaxados vêm da MM_Idle (ABP_Ninja → RelaxedHandsPose).
- **Combate (Fase 2):** GAS no próprio personagem (`AbilitySystem`, replicação Mixed). `Source/HypeWorld/Combat/`: `NinjaAttributeSet` (Health/MaxHealth/Chakra/MaxChakra/Damage), `NinjaEffects` (GE_NinjaDamage, GE_NinjaStun com a tag Ninja.State.Stunned, GE_NinjaChakraCharge; SetByCaller por nome), `NinjaCombatTypes` (FNinjaAttack = ficha do golpe, ENinjaHitReaction, tags nativas), `NinjaAbilities` (GA_NinjaMeleeCombo, GA_NinjaChargeChakra, LocalPredicted), `NinjaChakraAuraComponent` (aura por malhas+materiais, sem Niagara), `NinjaDamageNumber` (números e flash), `NinjaTrainingDummy` (bAlwaysGuard: Boneco_Treino_2 sempre defende). Dano só no servidor; desvio = dash dá invulnerabilidade. Materiais: `Tools/editor/criar_materiais_chakra.py`; configuração: `Tools/editor/configurar_combate.py`.
- **Combate PvP:** base = golpes da Epic feitos para este boneco (`MM_Attack_01/02/03`, `MM_ChargedAttack` em /Game/Characters/Mannequins/Anims/Unarmed/Attack) — são uma corrente: o 1º termina na pose em que o 2º começa (~0,55 s) e o 2º na do 3º, então emendam sem salto; a janela de combo abre logo antes desse ponto. Chutes do Mixamo (pasta Luta → `Tools/editor/importar_luta.py`) entram INTEIROS (guarda → chute → guarda) só como finalizadores; recortes curtos do Mixamo ficaram feios (descartados do combo, assets mantidos). Montagens por `Tools/editor/criar_combate_pvp.py` com `UNinjaEditorTools::BuildAttackMontage` ([início, fim] + AnimNotifies DoAttackTrace/CheckCombo) em `/Game/Ninja/Characters/Animations/Combate/AM_Golpe_*`. Regra: acertos e janela de combo ANTES do início do blend out (a habilidade termina no blend out).
  - Listas no personagem (BP_ThirdPersonCharacter → Ninja|Combate): LightCombo (LMB: direto, gancho esquerdo, chute+soco), HeavyAttacks (F: [0] soco carregado sozinho; [N] finalizador depois do N-ésimo leve: chute circular, chute frontal, chute giratório), SprintAttack (investida), AirAttack (chute aéreo). Ids: 0..99 leve, 100+ forte, 200 correndo, 300 ar.
  - **Sem mira automática (PvP depende de mirar):** o golpe vai para onde a câmera olha no momento do golpe (AttackFacingYaw; o corpo vira pelo RotationRate, sem estalo); a câmera não gira sozinha durante e logo após golpes. Nada de puxar até o alvo.

~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
