// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "InputCoreTypes.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "CombatAttacker.h"
#include "NinjaCombatTypes.h"
#include "NinjaFightingStyle.h"
#include "NinjaPassives.h"
#include "HypeWorldCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UNinjaMovementComponent;
class UAnimMontage;
class UNinjaDodgeDefinition;
class UAbilitySystemComponent;
class UNinjaAttributeSet;
class UNinjaChakraAuraComponent;
class UNinjaChakraFeetComponent;
class UGameplayAbility;
class UMaterialInterface;
class UHypeAppearanceComponent;
class UNinjaJutsuData;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AHypeWorldCharacter : public ACharacter, public IAbilitySystemInterface, public ICombatAttacker
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Sistema de habilidades (GAS): vida, chakra, golpes e jutsus */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UAbilitySystemComponent* AbilitySystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ninja|Aparencia", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UHypeAppearanceComponent> AppearanceComponent;

	/** Vida e chakra */
	UPROPERTY()
	UNinjaAttributeSet* Attributes;

	/** Aura de chakra (aparece ao carregar) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UNinjaChakraAuraComponent* ChakraAura;

	/** Chamas de chakra nos pés (Infusão de Chakra nos pés ligada) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UNinjaChakraFeetComponent* ChakraFeetFx;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Corrida ninja (segurar) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SprintAction;

	/** Dash (apertar) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DashAction;

	/** Atacar (combo corpo a corpo) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AttackAction;

	/** Golpe forte / finalizador */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* HeavyAttackAction;

	/** Carregar chakra (segurar) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ChargeChakraAction;

	/** Modo foco (segurar): de frente para a câmera, anda de lado e de costas */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* FocusAction;

	/** Abre/fecha o menu de combate (Tab) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* CombatMenuAction;

	/** Liga/desliga a infusão de chakra nos pés (passiva) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ChakraFeetAction;

	/** Alt: liga/desliga o andar (o padrão é trotar) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* WalkToggleAction;

	/** Rodinha do mouse: aproxima/afasta a câmera */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ZoomAction;

	/** Animação do dash (usada quando DashVariants não tem nada para a direção) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* DashMontage;

	/** Dashes por direção em relação à câmera (frente, trás, esquerda, direita, no ar), com variações que se alternam */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	FNinjaDashSet DashVariants;

	/** Saindo do dash andando/correndo: quanto tempo (segundos de verdade) a animação do dash leva para dar lugar à locomoção */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação", meta=(ClampMin=0.05))
	float DashExitBlendTime = 0.35f;

	/** Some com a montagem do dash suave, emendando o voo na aterrissagem (nunca no Loop, que é outra animação) */
	void BlendOutDashMontage(UAnimMontage* Montage);

	/** Animação do pulo de parede */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* WallJumpMontage;

	/** Animação do segundo pulo (no ar) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* DoubleJumpMontage;

	/** Animação em loop enquanto carrega chakra */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* ChargeChakraMontage;

	/** Animação em loop enquanto corre na parede */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* WallRunMontage;

	/** Virada para a direita (~90°) ao sair do lugar */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* TurnRightMontage;

	/** Meia-volta (~180°) ao sair do lugar */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* Turn180Montage;

	/** Quanto tempo a virada leva (s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float TurnDuration = 0.45f;

	/** Velocidade a partir da qual a câmera passa a seguir as costas do boneco (cm/s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Câmera")
	float CameraFollowMinSpeed = 400.f;

	/** Rapidez com que a câmera volta para as costas do boneco */
	UPROPERTY(EditAnywhere, Category="Ninja|Câmera")
	float CameraFollowSpeed = 2.5f;

	/** Tempo sem mexer o mouse até a câmera começar a seguir (s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Câmera")
	float CameraFollowDelay = 0.6f;

	/** Camera framing and zoom are independent: scrolling only changes the arm length. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="0"))
	float CameraZoomDefault = 450.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="0"))
	float CameraZoomMin = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="0"))
	float CameraZoomMax = 800.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="0"))
	float CameraZoomStep = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="0"))
	float CameraZoomSpeed = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	FVector CameraTargetOffset = FVector(0.f, 0.f, 60.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	FVector CameraSocketOffset = FVector(0.f, 0.f, 40.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="-89", ClampMax="89"))
	float CameraInitialPitch = -15.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="-89", ClampMax="89"))
	float CameraPitchMin = -60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="-89", ClampMax="89"))
	float CameraPitchMax = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="5", ClampMax="170"))
	float CameraFOV = 80.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	bool bCameraUsePawnControlRotation = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	bool bCameraLagEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="0"))
	float CameraLagSpeed = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	bool bCameraRotationLagEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	bool bCameraCollisionTest = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="1"))
	float CameraProbeSize = 12.f;

	UFUNCTION(BlueprintCallable, Category="Camera")
	void DoCameraZoom(float ScrollDelta);

	/** Corrida ninja: rapidez com que a câmera volta para as costas nas curvas */
	UPROPERTY(EditAnywhere, Category="Ninja|Câmera")
	float SprintCameraFollowSpeed = 8.f;

	/** Correndo rápido, A/D viram em arco: no máximo este ângulo em relação à direção atual (graus) */
	UPROPERTY(EditAnywhere, Category="Ninja|Movimento")
	float SprintSteerAngle = 35.f;

	/** W duas vezes seguidas liga a corrida ninja: tempo máximo entre os dois toques (segundos) */
	UPROPERTY(EditAnywhere, Category="Ninja|Movimento")
	float DoubleTapSprintWindow = 0.3f;

	/** Rapidez de giro do corpo correndo rápido (graus/s): menor = curva mais aberta */
	UPROPERTY(EditAnywhere, Category="Ninja|Movimento")
	float SprintRotationRate = 360.f;

	/** Rapidez de giro no trote (corrida de combate, sem Shift): alta, o corpo vira quase na hora para onde vai */
	UPROPERTY(EditAnywhere, Category="Ninja|Movimento")
	float CombatRunRotationRate = 1080.f;

	/** Animação que toca de vez em quando se ficar parado (olhar em volta) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* IdleBreakMontage;

	/** Segundos parado até tocar a animação de olhar em volta */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float IdleBreakDelay = 30.f;

	/** Inclinação máxima ao fazer curva correndo (graus) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float MaxTurnLean = 15.f;

	/** Inclinação correndo na parede, para longe da parede (graus) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float WallRunLean = 30.f;

	/** Quanto o boneco encosta na parede durante a corrida na parede (cm) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float WallRunMeshOffset = 35.f;

	/** Rapidez com que a inclinação acompanha (maior = mais rápido) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float LeanInterpSpeed = 8.f;

	/** Dash acrobático: quanto o corpo gira para a frente (cambalhota/estrela) durante o dash (graus; 0 = sem giro) */
	UPROPERTY(EditAnywhere, Category="Ninja|Acrobacia")
	float DashFlipDegrees = 360.f;

	/** Pulo do chão: giro no próprio eixo (graus) e quanto tempo leva (s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Acrobacia")
	float JumpSpinDegrees = 360.f;
	UPROPERTY(EditAnywhere, Category="Ninja|Acrobacia")
	float JumpSpinDuration = 0.5f;

	/** Velocidade em que a animação de corrida combina com os pés; acima disso ela é acelerada */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float RunAnimNaturalSpeed = 600.f;

	/** Pares (velocidade do jogo, velocidade natural da animação naquele ponto). A animação acelera na razão entre os dois
	 *  — os pés acompanham o chão em qualquer velocidade. Deve casar com as amostras do BS_Ninja_Locomocao. */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	TArray<FVector2D> AnimSpeedCurve = { FVector2D(155.f, 155.f), FVector2D(420.f, 351.f), FVector2D(800.f, 359.f) };

	/** Velocidade natural das corridas de lado (esquerda, direita) — a animação acelera nessa proporção */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float RunStrafeLeftNatural = 428.f;
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float RunStrafeRightNatural = 342.f;

	/** Acima desta velocidade o ajuste por direção vale (abaixo é andar) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float RunAnimMinSpeed = 300.f;

	/** Velocidade natural do passo lateral e do andar de costas (o corpo fica de frente para a câmera andando) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float WalkStrafeNatural = 164.f;
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float WalkBackNatural = 107.f;

	/** Velocidade natural da corrida de costas (acima de StrafeRunMinSpeed) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float RunBackNatural = 308.f;

	/** Abaixo desta velocidade, de lado/de costas conta como andar (acima, corrida lateral) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float StrafeRunMinSpeed = 250.f;

	/** Quanto a animação de corrida pode ser acelerada no máximo (2,7x = corrida de 4,5 m/s chega a ~12 m/s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float MaxRunAnimRate = 2.7f;

public:
	bool IsPlayingDashMontage() const;
	/** Frente, direita, trás, esquerda. Cada entrada é um asset substituível no editor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ninja|Esquiva de combate")
	TArray<TObjectPtr<UNinjaDodgeDefinition>> CombatDodges;
	UNinjaDodgeDefinition* GetCombatDodgeForDirection(int32 Index) const;
	void PlayCombatDodge(UNinjaDodgeDefinition* Definition);
	UFUNCTION(NetMulticast, Reliable) void MulticastCombatDodge(UNinjaDodgeDefinition* Definition, float FacingYaw);
	UFUNCTION(BlueprintCallable, Category="Ninja|Esquiva") bool TryCancelDodgeForAction();
	UAnimMontage* GetActiveDashMontage() const { return ActiveDashMontage; }
	FVector GetRawMoveDirection() const { return DashMoveDirection; }
	FVector DashMoveDirection = FVector::ZeroVector;
	uint64 DashMoveFrame = 0;
	FVector RawMoveDirection = FVector::ZeroVector;
	float RawMoveTime = -100.f;

	// ---------- Combate ----------

	/** Combo leve (botão esquerdo): um golpe por clique, em ordem. */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	TArray<FNinjaAttack> LightCombo;

	/** Golpes fortes (F). [0] = sozinho; [N] = finalizador depois do N-ésimo golpe leve (se faltar, usa o último). */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	TArray<FNinjaAttack> HeavyAttacks;

	/** Atacar correndo (acima de SprintAttackMinSpeed). */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	FNinjaAttack SprintAttack;

	/** Atacar no ar. */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	FNinjaAttack AirAttack;

	/** Velocidade a partir da qual o ataque vira o golpe de corrida (cm/s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	float SprintAttackMinSpeed = 450.f;

	/** Chakra ganho por golpe acertado */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	float ChakraPerHit = 4.f;

	/** Atacando: quanto da direção das teclas ainda vale (0 = trava, 1 = livre) — dá para corrigir a mira no meio do golpe */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	float AttackSteering = 0.3f;

	/** Clique um pouco antes da janela de combo abrir ainda vale (s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	float ComboInputWindow = 0.45f;

	/** Depois que um golpe termina, o próximo clique ainda continua a sequência por este tempo (s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	float ComboChainGrace = 0.35f;

	/** Defendendo, quanto do dano de frente passa (0,15 = 15%) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	float GuardDamageScale = 0.15f;

	/** Atordoamento ao defender um golpe (s) e ao ter a defesa quebrada (s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	float BlockStun = 0.2f;
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	float GuardBreakStun = 1.1f;

	/** Golpeando, o avanço para quando há alguém à frente a menos desta distância entre os corpos (cm) — eles não se atravessam */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	float AttackBodySpacing = 45.f;

	/** Golpes seguidos contam no contador da tela se vierem antes deste tempo (s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	float ComboCounterTime = 1.6f;

	/** Pausa no impacto (s) — dá peso ao golpe */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	float HitStopDuration = 0.06f;

	/** Reação ao levar golpe leve (sorteia uma) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	TArray<UAnimMontage*> HitReactMontages;

	/** Reação a golpe forte / arremesso / defesa quebrada */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	UAnimMontage* HeavyHitReactMontage;

	/** Animação de nocaute */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	UAnimMontage* KnockoutMontage;

	/** Material do flash de impacto */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	UMaterialInterface* HitFlashMaterial;

	/** Habilidades dadas ao nascer */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	TSubclassOf<UGameplayAbility> ComboAbility;
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	TSubclassOf<UGameplayAbility> ChargeChakraAbility;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UNinjaAttributeSet* GetAttributes() const { return Attributes; }

	// ICombatAttacker (chamados pelos AnimNotifies das montagens de golpe)
	virtual void DoAttackTrace(FName DamageSourceBone) override;
	virtual void CheckCombo() override;
	virtual void CheckChargedAttack() override {}

	/** Golpe pelo id: 0..99 leve, 100+ forte, 200 correndo, 300 no ar. */
	const FNinjaAttack* GetAttack(int32 AttackId) const;

	/** Começo e fim de um golpe (chamados pela habilidade, no jogador e no servidor) */
	void BeginAttack(int32 AttackId);
	bool CanActivateAttack(int32 AttackId) const;
	UFUNCTION(BlueprintPure, Category="Ninja|Combate") bool CanStartLauncher() const;
	UFUNCTION(BlueprintPure, Category="Ninja|Combate") float GetAirComboCooldownRemaining() const;
	UFUNCTION(BlueprintPure, Category="Ninja|Infusão") float GetSuperJumpCooldownRemaining() const;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ninja|Combate") float AirComboCooldown = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ninja|Infusão") float SuperJumpCooldown = 3.f;
	void EndAttack(bool bWasCancelled);

	/** Defendendo (botão direito parado ou andando) */
	UFUNCTION(BlueprintPure, Category="Ninja|Combate")
	bool IsGuarding() const;

	/** Modo de batalha: segurando o botão direito, golpeando, e por BattleModeDuration depois de bater ou apanhar.
	 *  Parado em batalha = pose de luta; fora dela = parado normal. Replicado (todos veem igual). */
	UFUNCTION(BlueprintPure, Category="Ninja|Combate")
	bool IsInBattleMode() const { return bBattleMode; }

	/** Quanto tempo o modo de batalha continua depois do último golpe dado ou levado (s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	float BattleModeDuration = 6.f;

	// ---------- PvP justo: travamento, limite de combo e fuga ----------

	/** Pode agir (atacar, carregar chakra, jutsus)? Atordoado ou nocauteado não. Jutsus novos devem bloquear com a tag Ninja.State.Stunned. */
	UFUNCTION(BlueprintPure, Category="Ninja|Combate")
	bool CanAct() const { return !bKnockedOut && !IsStunned(); }

	/** Golpes seguidos que o mesmo alvo pode levar: no último ele é arremessado e fica invulnerável (fim do combo) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Fuga")
	int32 MaxComboHits = 7;

	/** Tempo máximo apanhando sem parar (s): passou disso, o próximo golpe arremessa e quebra o combo */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Fuga")
	float MaxComboTime = 3.5f;

	/** Cada golpe seguido atordoa menos: atordoamento x (Decay ^ golpes anteriores), nunca abaixo de MinHitstunScale */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Fuga")
	float HitstunDecay = 0.9f;
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Fuga")
	float MinHitstunScale = 0.45f;

	/** Cada golpe seguido tira menos vida (-8% por golpe, nunca abaixo de 50%) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Fuga")
	float ComboDamageDecay = 0.08f;
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Fuga")
	float MinComboDamageScale = 0.5f;

	/** Invulnerável depois de ter o combo quebrado (arremessado pelo limite) (s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Fuga")
	float ComboBreakInvulnerability = 1.2f;

	/** Substituição (Q apanhando): cargas, recarga de cada uma (s), distância do teleporte (cm) e invulnerabilidade (s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Fuga")
	int32 MaxSubstitutions = 2;
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Fuga")
	float SubstitutionRecharge = 8.f;
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Fuga")
	float SubstitutionDistance = 350.f;
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Fuga")
	float SubstitutionInvulnerability = 0.5f;

	// ---------- Combo aéreo e mortal para trás ----------

	/** Lançador acertou: o atacante sobe atrás do adversário (cm/s para cima) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Aéreo")
	float LauncherFollowUpSpeed = 1150.f;

	/** Golpe no ar acertou: atacante e adversário param de cair e ficam suspensos (gravidade fraca) por AirSuspendTime;
	 *  cada acerto ou golpe seguinte do combo aéreo renova. Velocidades = pequeno impulso para cima (cm/s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Aéreo")
	float AirHoverSpeed = 60.f;
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Aéreo")
	float AirJuggleSpeed = 80.f;
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Aéreo")
	float AirSuspendTime = 1.f;

	/** Empurrão máximo dos golpes aéreos que não são o último (cm/s) — pequeno para os dois continuarem juntos */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Aéreo")
	float AirJuggleKnockback = 80.f;

	/** Último golpe do combo aéreo: arremessa para longe (sem frear no ar) e o adversário fica invulnerável (s) —
	 *  cai longe e não dá para emendar golpes no chão */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Aéreo")
	float AirFinisherInvulnerability = 1.6f;

	/** Reações no ar (sorteada) e ao ser lançado para cima */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Aéreo")
	TArray<UAnimMontage*> AirHitReactMontages;
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Aéreo")
	UAnimMontage* LaunchReactMontage;

	/** Modo de batalha + S + Espaço: super pulo para cima e para trás com mortal (para atacar/soltar jutsus do ar) */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Aéreo")
	float BackflipUpSpeed = 1300.f;
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Aéreo")
	float BackflipBackSpeed = 240.f;   // sem frenagem no ar: ~8 m para trás
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Aéreo")
	UAnimMontage* BackflipMontage;
	/** Modo de batalha, no chão: segurar Espaço por isto (s) = lançador (combo aéreo); toque rápido = pulo normal */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Aéreo")
	float LauncherHoldTime = 0.2f;
	/** Gravidade do mortal (× a do personagem): X = subindo, Y = flutuando no topo/descendo, Z = queda máxima (cm/s).
	 *  Pouca gravidade no topo = fica "levitando" no alto para atacar ou se reposicionar */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Aéreo")
	FVector BackflipGravity = FVector(1.f, 1.f, 0.f);

	/** Golpeando: passo automático para a frente no começo de cada golpe (s) — as animações do pack não andam sozinhas */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate")
	float AttackLungeTime = 0.22f;

	/** Substituição: apanhando, some numa nuvem e reaparece longe de quem bateu (gasta 1 carga) */
	UFUNCTION(BlueprintCallable, Category="Ninja|Combate")
	void TrySubstitution();

	UFUNCTION(BlueprintPure, Category="Ninja|Combate")
	int32 GetSubstitutionCharges() const { return SubstitutionCharges; }

	/** Invulnerável (quebra de combo ou substituição): golpes passam direto */
	UFUNCTION(BlueprintPure, Category="Ninja|Combate")
	bool IsInvulnerable() const { return bInvulnerable; }

	/** Golpes seguidos que este personagem está levando agora (servidor) */
	int32 GetTakenComboHits() const { return TakenComboHits; }

	// ---------- Estilos de luta (menu de combate) ----------

	/** Estilos que o jogador pode escolher no menu (Taijutsu). O primeiro é o padrão. */
	UPROPERTY(EditAnywhere, Category="Ninja|Combate|Estilos")
	TArray<TObjectPtr<UNinjaFightingStyle>> FightingStyles;

	/** Escolhe o estilo de luta (falha se ele exige uma arma que o personagem não está segurando) */
	UFUNCTION(BlueprintCallable, Category="Ninja|Combate|Estilos")
	bool SelectFightingStyle(int32 Index);

	UFUNCTION(BlueprintPure, Category="Ninja|Combate|Estilos")
	int32 GetFightingStyleIndex() const { return FightingStyleIndex; }

	UFUNCTION(BlueprintPure, Category="Ninja|Combate|Estilos")
	UNinjaFightingStyle* GetFightingStyle() const;

	UFUNCTION(BlueprintPure, Category="Ninja|Combate|Estilos")
	bool CanUseFightingStyle(const UNinjaFightingStyle* Style) const;

	UFUNCTION(BlueprintPure, Category="Ninja|Combate|Estilos")
	ENinjaWeapon GetEquippedWeapon() const { return EquippedWeapon; }

	/** Base de luta do estilo atual (vazio = a padrão do Anim Blueprint) */
	UAnimSequenceBase* GetBattleIdlePose() const;

	// ---------- Passivas (menu de combate → Passivas, ou tecla própria) ----------

	/** Passivas que o personagem tem (aprendidas, do clã...) — o menu lista estas */
	UPROPERTY(EditAnywhere, Category="Ninja|Passivas")
	TArray<FNinjaPassiveInfo> Passives;

	UFUNCTION(BlueprintPure, Category="Ninja|Passivas")
	bool IsPassiveEnabled(ENinjaPassive Passive) const { return (EnabledPassives & (1u << uint32(Passive))) != 0; }

	/** Liga/desliga uma passiva aprendida (replicado) */
	UFUNCTION(BlueprintCallable, Category="Ninja|Passivas")
	void SetPassiveEnabled(ENinjaPassive Passive, bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="Ninja|Passivas")
	void TogglePassive(ENinjaPassive Passive) { SetPassiveEnabled(Passive, !IsPassiveEnabled(Passive)); }

	bool HasLearnedPassive(ENinjaPassive Passive) const;

	// ---------- Infusão de chakra nos pés: super pulo carregado (segurar Espaço) ----------

	/** Segurar Espaço mais que isto (s) começa a carregar o super pulo (menos = pulo normal) */
	UPROPERTY(EditAnywhere, Category="Ninja|Infusão")
	float SuperJumpHoldThreshold = 0.16f;

	/** Tempo para encher a barra do super pulo (s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Infusão")
	float SuperJumpChargeTime = 1.5f;

	/** Velocidade para a frente e para cima com a barra vazia (X) e cheia (Y), em cm/s */
	UPROPERTY(EditAnywhere, Category="Ninja|Infusão")
	FVector2D SuperJumpForwardSpeed = FVector2D(1800.f, 3000.f);
	UPROPERTY(EditAnywhere, Category="Ninja|Infusão")
	FVector2D SuperJumpUpSpeed = FVector2D(850.f, 1250.f);
	/** Mirando para o céu: velocidade só para cima (vazia X, cheia Y). Entre olhar reto e para cima mistura os dois */
	UPROPERTY(EditAnywhere, Category="Ninja|Infusão")
	FVector2D SuperJumpVerticalSpeed = FVector2D(1434.f, 2271.f);
	/** Inclinação da câmera (graus) em que o salto começa a virar vertical (X) e fica totalmente vertical (Y) */
	UPROPERTY(EditAnywhere, Category="Ninja|Infusão")
	FVector2D SuperJumpVerticalPitch = FVector2D(25.f, 70.f);
	/** Compatibilidade com assets antigos. O salto agora usa gravidade constante e altura definida pelo impulso. */
	UPROPERTY(EditAnywhere, Category="Ninja|Infusão")
	FVector SuperJumpGravity = FVector(1.f, 1.f, 0.f);

	/** Agachado carregando (início + loop) e o salto (impulso → voo → descida) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* SuperJumpChargeMontage;
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* SuperJumpLaunchMontage;

	/** Material do brilho de chakra nos pés (infusão ligada) */
	UPROPERTY(EditAnywhere, Category="Ninja|Infusão")
	UMaterialInterface* ChakraFeetMaterial;

	UFUNCTION(BlueprintPure, Category="Ninja|Infusão")
	bool IsChargingSuperJump() const { return bChargingSuperJump; }

	/** 0..1 quanto da barra do super pulo está cheia */
	UFUNCTION(BlueprintPure, Category="Ninja|Infusão")
	float GetSuperJumpCharge() const { return SuperJumpCharge; }

	/** Velocidade ao longo do chão atual (na parede/teto também) — para as animações */
	float GetSurfaceSpeed() const;

	/** Atordoado por um golpe */
	UFUNCTION(BlueprintPure, Category="Ninja|Combate")
	bool IsStunned() const;

	/** Golpes seguidos acertados agora (0 = sem combo); para o contador da tela */
	UFUNCTION(BlueprintPure, Category="Ninja|Combate")
	int32 GetComboHitCount() const;

	/** Está tocando a animação de um golpe (vale em todas as máquinas, inclusive para os outros jogadores) */
	bool IsPlayingAttackMontage() const;

	/** Para onde o golpe atual mira: a direção da câmera no momento do golpe (sem mira automática — PvP depende de mirar) */
	float GetAttackFacingYaw() const { return AttackFacingYaw; }

	/** Id do golpe atual (-1 = nenhum) */
	UFUNCTION(BlueprintPure, Category="Ninja|Combate")
	int32 GetCurrentAttackId() const { return CurrentAttackId; }

	UFUNCTION(BlueprintPure, Category="Ninja|Combate")
	bool IsChargingChakra() const { return bChargingChakra; }

	UFUNCTION(BlueprintPure, Category="Ninja|Combate")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category="Ninja|Combate")
	float GetChakra() const;

	/** Botão de ataque (combo leve) */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoAttackPressed();

	/** Botão de golpe forte */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoHeavyAttackPressed();

	/** Começa / para de carregar chakra */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoChargePressed();
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoChargeReleased();
	bool CanChargeChakra() const;
	void SetChargingChakra(bool bCharging);

	/** Está no meio de um ataque (movimento travado) */
	UFUNCTION(BlueprintPure, Category="Ninja|Combate")
	bool IsAttacking() const;

	// ---------- Jutsus (barra de slots do HUD) ----------

	/** Jutsu de cada slot da barra (o slot 0 é a tecla 1). Cada um vira um spec de JutsuAbility com a ficha como SourceObject */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ninja|Jutsus")
	TArray<TObjectPtr<UNinjaJutsuData>> JutsuLoadout;

	/** Animações de corpo dos jutsus (selos e lançamento) feitas para outro esqueleto -> a mesma animação retargetada para o
	 *  esqueleto do corpo visível. Só é usada quando o esqueleto da animação da ficha não é o do corpo */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ninja|Jutsus")
	TMap<TObjectPtr<UAnimSequenceBase>, TObjectPtr<UAnimSequenceBase>> JutsuBodyAnimations;

	/** Tecla de cada slot */
	UPROPERTY(EditAnywhere, Category="Ninja|Jutsus")
	TArray<FKey> JutsuSlotKeys = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six };

	/** Habilidade usada por todos os jutsus (vazio = UGA_NinjaJutsu) */
	UPROPERTY(EditAnywhere, Category="Ninja|Jutsus")
	TSubclassOf<UGameplayAbility> JutsuAbility;

	/** Inclinação máxima da mira do jutsu para cima/baixo (graus) — o jutsu vai para onde a câmera olha */
	UPROPERTY(EditAnywhere, Category="Ninja|Jutsus")
	FVector2D JutsuAimPitchLimits = FVector2D(-12.f, 14.f);

	/** Usa o jutsu do slot (tecla do slot) */
	UFUNCTION(BlueprintCallable, Category="Ninja|Jutsus")
	bool TryUseJutsuSlot(int32 Slot);

	const UNinjaJutsuData* GetJutsuInSlot(int32 Slot) const { return JutsuLoadout.IsValidIndex(Slot) ? JutsuLoadout[Slot].Get() : nullptr; }

	/** Segundos de cooldown que faltam no slot (e a duração total) */
	float GetJutsuCooldownRemaining(int32 Slot, float& OutDuration) const;

	/** Quantas vezes o slot foi usado por este jogador (o HUD pulsa o ícone quando muda) */
	int32 GetJutsuUseCount(int32 Slot) const { return JutsuUseCounts.IsValidIndex(Slot) ? JutsuUseCounts[Slot] : 0; }

	UFUNCTION(BlueprintPure, Category="Ninja|Jutsus")
	bool IsCastingJutsu() const { return CurrentJutsu != nullptr; }

	/** Chamas em órbita ativas deste personagem (todas as máquinas; o HUD e o clique esquerdo usam) */
	class ANinjaFlameOrbit* GetActiveFlameOrbit() const;
	/** Lança uma chama da órbita no ponto da mira (o que o clique esquerdo faz); falso sem chamas */
	UFUNCTION(BlueprintCallable, Category="Ninja|Jutsus")
	bool FireFlameOrbitAtCrosshair() { return TryFireFlameOrbit(); }
	void SetActiveFlameOrbit(class ANinjaFlameOrbit* Orbit);
	bool CanStartJutsu() const;

	/** Chamados pela habilidade (no jogador e no servidor) */
	void BeginJutsu(const UNinjaJutsuData* Jutsu);
	void EndJutsuSeals();
	void EndJutsu(bool bWasCancelled);

	/** AnimNotify NinjaJutsuRelease (todas as máquinas): fogo na boca, tremida; no servidor cria o projétil */
	void OnJutsuRelease();

	/** Servidor: acerto de jutsu (dano em área já escalado pela distância) */
	void ApplyJutsuHit(AHypeWorldCharacter* Target, const UNinjaJutsuData* Jutsu, float Scale, const FHitResult& Hit);

	/** Tremida de câmera (só no jogador local): amplitude em graus, some em Duration */
	UFUNCTION(BlueprintCallable, Category="Ninja|Câmera")
	void AddCameraShake(float Degrees, float Duration);

	/** Constructor */
	AHypeWorldCharacter(const FObjectInitializer& ObjectInitializer);

protected:

	virtual void BeginPlay() override;
	virtual void PawnClientRestart() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Vida mudou (em todas as máquinas): número de dano, flash, reação, nocaute */
	virtual void OnHealthChanged(const FOnAttributeChangeData& Data);

	/** Nocauteado (vida zero) */
	virtual void OnKnockedOut();

	/** Servidor: aplica um acerto em Target (dano, defesa, atordoamento, empurrão, reação em todos) */
	void ApplyHit(AHypeWorldCharacter* Target, const FNinjaAttack& Attack, bool bFinalHit, const FHitResult& Hit);

	/** Reação de quem apanhou, em todas as máquinas */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastHitReaction(ENinjaHitReaction Reaction, FVector_NetQuantizeNormal FromDirection, bool bComboBreak, bool bAirJuggle, bool bThrownAway);

	/** Último golpe (com reação de arremesso) da sequência aérea */
	bool IsAirFinisher(const FNinjaAttack& Attack, int32 AttackId) const;

	// Fuga do combo (servidor decide)
	UFUNCTION(Server, Reliable)
	void ServerSubstitution();
	void DoSubstitution();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSubstitution(FVector_NetQuantize From, FVector_NetQuantize To);

	UPROPERTY(ReplicatedUsing=OnRep_EnabledPassives)
	uint32 EnabledPassives = 0;
	UFUNCTION()
	void OnRep_EnabledPassives();
	UFUNCTION(Server, Reliable)
	void ServerSetPassiveEnabled(ENinjaPassive Passive, bool bEnabled);
	void DoToggleChakraFeet();
	void UpdateChakraFeetVisual();
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> ChakraFeetGlow;

	// Super pulo
	bool bJumpKeyHeld = false;
	float JumpHoldTime = 0.f;
	float SuperJumpCharge = 0.f;
	UPROPERTY(ReplicatedUsing=OnRep_ChargingSuperJump)
	bool bChargingSuperJump = false;
	UFUNCTION()
	void OnRep_ChargingSuperJump();
	UFUNCTION(Server, Reliable)
	void ServerSetChargingSuperJump(bool bCharging);
	UFUNCTION(Server, Reliable)
	void ServerSuperJump(FVector_NetQuantize10 LaunchVelocity);
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSuperJumpLaunch(float UpSpeed);
	void UpdateSuperJump(float DeltaSeconds);
	void ReleaseSuperJump();
	bool CanStartSuperJump() const;
	/** Para de segurar/carregar o super pulo sem pular */
	void CancelSuperJumpCharge();
	/** Espaço apertado agora (jogador local) */
	bool bSpaceHeld = false;
	/** Modo de batalha: Espaço apertado, esperando para saber se é toque (pulo) ou segurar (lançador) */
	bool bLauncherHoldPending = false;
	float LauncherHoldElapsed = 0.f;
	void UpdateLauncherHold(float DeltaSeconds);
	/** Pulo normal de um toque rápido (o "soltar" vem um instante depois, senão o pulo nem começa) */
	void TapJump();

	UPROPERTY(Replicated)
	int32 FightingStyleIndex = 0;
	UPROPERTY(Replicated)
	ENinjaWeapon EquippedWeapon = ENinjaWeapon::None;
	UFUNCTION(Server, Reliable)
	void ServerSelectFightingStyle(int32 Index);

	// Variedade: o último combo/finalizador/golpe sorteado (o próximo sorteio evita repetir)
	mutable int32 LastComboString = INDEX_NONE;
	mutable int32 LastFinisher = INDEX_NONE;
	mutable int32 LastSprintAttack = INDEX_NONE;
	mutable int32 LastAirAttack = INDEX_NONE;
	mutable int32 LastLauncher = INDEX_NONE;

	/** Golpeando ou logo depois de um golpe (a sequência ainda emenda) */
	bool IsInComboContext() const;
	void DoBackflipJump();
	UFUNCTION(Server, Reliable)
	void ServerBackflipJump(FVector_NetQuantize10 LaunchVelocity);
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastBackflip();
	static int32 PickVariant(int32 Num, int32 Last);

	// Dash: montagem tocando agora e contador (escolhe a variação igual em todas as máquinas)
	UPROPERTY()
	TObjectPtr<UAnimMontage> ActiveDashMontage;
	int32 DashCount = 0;
	UAnimMontage* PickDashMontage(EMovementMode PrevMovementMode);

	void DoCombatMenu();

	UPROPERTY(Replicated)
	int32 SubstitutionCharges = 2;
	float SubstitutionRechargeTime = 0.f;
	UPROPERTY(Replicated)
	bool bInvulnerable = false;
	float InvulnerableUntil = -100.f;
	int32 TakenComboHits = 0;
	float TakenComboStart = 0.f;
	float LastTakenHitTime = -100.f;
	TWeakObjectPtr<AActor> LastAttacker;
	float AttackStartTime = -100.f;

	void UpdateCombatState(float DeltaSeconds);

	/** Alguém colado à frente (a menos de AttackBodySpacing): golpe não avança */
	bool HasBodyAhead() const;

	UFUNCTION(Server, Reliable)
	void ServerSetGuarding(bool bNewGuarding);

	UPROPERTY(Replicated)
	bool bGuarding = false;

	UPROPERTY(Replicated)
	bool bBattleMode = false;
	float LastBattleTime = -100.f;
	void UpdateBattleMode();

	/** Modo de batalha ligado pelo clique no botão direito (até outro clique ou nocaute) */
	bool bBattleToggle = false;
	void SetBattleToggle(bool bOn);
	UFUNCTION(Server, Reliable)
	void ServerSetBattleToggle(bool bOn);

	/** Corrida automática: desliga sozinha ao parar */
	void UpdateAutoRun(float DeltaSeconds);
	float AutoRunStopTime = 0.f;

	/** W duas vezes = corrida ninja (o W chega como eixo: um toque novo = W apertado depois de um quadro sem ele) */
	void CheckForwardDoubleTap(float Forward);
	uint64 LastForwardFrame = 0;
	float LastForwardTapTime = -100.f;

	UFUNCTION()
	void OnRep_ChargingChakra();

	// Jutsus: selos no corpo visível (em todas as máquinas) e estado do lançamento
	UFUNCTION(NetMulticast, Reliable)
	void MulticastJutsuSeals(const UNinjaJutsuData* Jutsu);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastJutsuEnd(bool bCancelled);
	void PlayJutsuSeals(const UNinjaJutsuData* Jutsu);
	void StopJutsuSeals(float BlendOut);
	void UpdateJutsuSeals(float DeltaSeconds);
	void StartSeal(int32 Index);
	USkeletalMeshComponent* GetJutsuBody() const;
	/** A animação da ficha ou, se ela for de outro esqueleto, a versão do corpo em JutsuBodyAnimations */
	UAnimSequenceBase* GetJutsuBodyAnimation(UAnimSequenceBase* Animation, const USkeletalMeshComponent* Body) const;
	UPROPERTY() TObjectPtr<const UNinjaJutsuData> CurrentJutsu;
	/** Jutsu que está sendo mostrado (selos/lançamento) — também nos outros jogadores */
	UPROPERTY() TObjectPtr<const UNinjaJutsuData> PresentedJutsu;
	UPROPERTY() TObjectPtr<UAnimMontage> SealMontage;
	int32 SealIndex = INDEX_NONE;
	float SealElapsed = 0.f;
	bool bJutsuReleased = false;
	/** Projéteis já soltos neste uso (rajadas) */
	int32 JutsuShotsFired = 0;
	TArray<int32> JutsuUseCounts;
	TWeakObjectPtr<class ANinjaFlameOrbit> ActiveFlameOrbit;
	/** Clique esquerdo com chamas em órbita: o servidor lança a chama na direção do ponto mirado */
	UFUNCTION(Server, Reliable) void ServerFireFlameOrbit(FVector_NetQuantize AimPoint);
	bool TryFireFlameOrbit();
	/** Antecipação do jutsu na boca durante os selos (só visual); some no lançamento ou no cancelamento */
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> JutsuAnticipation;
	void ClearJutsuAnticipation();

	// Tremida de câmera
	void UpdateCameraShake(float DeltaSeconds);
	float ShakeAmplitude = 0.f;
	float ShakeElapsed = 0.f;
	float ShakeDuration = 0.f;
	FRotator CameraBaseRotation = FRotator::ZeroRotator;

	UPROPERTY(ReplicatedUsing=OnRep_ChargingChakra)
	bool bChargingChakra = false;

	bool bKnockedOut = false;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	/** Atordoado não pula */
	virtual bool CanJumpInternal_Implementation() const override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Shift (clique): trote <-> corrida ninja (o trote é o padrão; não precisa segurar) */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintStart();

	/** Alt (clique): andar <-> trote */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoWalkToggle();

	/** Sai da corrida ninja e volta ao trote */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintEnd();

	/** Pede um dash */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoDash();

	/** Liga/desliga o modo foco */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoFocusStart();
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoFocusEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Braços abertos no ar (segundo pulo) — usado pela pose do ABP */
	bool IsSpreadingArms() const;

	/** Componente de movimento ninja */
	UFUNCTION(BlueprintPure, Category="Ninja")
	UNinjaMovementComponent* GetNinjaMovement() const;

	/** Saindo do lugar: toca a virada se a direção for muito diferente (chamado pelo movimento antes de girar) */
	void HandleMoveStart(const FVector& MoveAcceleration);

private:

	/** Inclina o boneco nas curvas e na parede (só visual, roda em todas as máquinas) */
	void UpdateLean(float DeltaSeconds);

	/** Acelera a animação de corrida quando o boneco vai mais rápido que ela, para os pés não "patinarem" */
	void UpdateRunAnimRate();

	/** Câmera volta suave para trás do boneco quando ele corre e o mouse está parado */
	void UpdateCameraFollow(float DeltaSeconds);

	/** Rodinha do mouse: muda a distância desejada da câmera; UpdateCameraZoom chega nela suave */
	void Zoom(const FInputActionValue& Value);
	void UpdateCameraZoom(float DeltaSeconds);
	void ApplyCameraSettings();
	void InitializeLocalCamera();
	bool bCameraInitialPitchApplied = false;
	float CameraZoomGoal = 0.f;

	/** Ao sair do lugar numa direção muito diferente, toca a animação de virar e gira no mesmo ritmo */
	void UpdateTurnInPlace(float DeltaSeconds);

	float LastLookInputTime = -100.f;
	float StillTime = 0.f;
	float TurnTimeLeft = 0.f;
	float DefaultRotationRate = 720.f;

	/** Conta o tempo parado e toca a animação de olhar em volta depois de IdleBreakDelay */
	void UpdateIdleBreak(float DeltaSeconds);

	float IdleTime = 0.f;

	/** Detecta o pulo duplo pela velocidade (vale também para os outros jogadores na rede) e toca a animação */
	void UpdateDoubleJump();
	void PlayJumpMontage(UAnimMontage* Montage, float UpSpeed);
	bool bWasFalling = false;
	float DoubleJumpLastVelZ = 0.f;

	/** Último toque no Shift (para detectar o toque duplo) */
	float LastSprintTapTime = -100.f;

public:
	/** Janela do toque duplo no Shift para a corrida ninja (s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Movimento")
	float SprintDoubleTapWindow = 0.3f;
private:

	/** Congela a animação por um instante (impacto) */
	void HitStop();

	/** Escolhe o próximo golpe para o botão apertado (-1 = nenhum) */
	int32 ChooseAttack(ENinjaAttackInput Input) const;

	/** Dispara o golpe pela habilidade (com o id no evento, para o servidor tocar o mesmo) */
	void StartAttack(ENinjaAttackInput Input);

	/** Usa o clique guardado durante o golpe (se ainda valer) */
	void ConsumeBufferedAttack();

	/** Liga/desliga a defesa conforme o botão direito e o estado (só no jogador) */
	void UpdateGuard();

	/** Ajusta o avanço da animação do golpe: só para a frente, escalado */
	FTransform ProcessAttackRootMotion(const FTransform& InRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds);

	int32 CurrentAttackId = INDEX_NONE;
	int32 LastChainAttackId = INDEX_NONE;	// golpe que acabou de terminar (a sequência continua até ChainExpireTime)
	float ChainExpireTime = -100.f;
	float GroundComboReadyUntil = -100.f;
	float AirComboReadyUntil = -100.f;
	float NextLauncherTime = -100.f;
	float NextSuperJumpTime = -100.f;
	int32 AttackHitIndex = 0;			// acertos já feitos no golpe atual
	int32 AttackHitCount = 1;			// acertos que o golpe tem (AnimNotifies DoAttackTrace)
	bool bComboWindowOpen = false;
	ENinjaAttackInput BufferedInput = ENinjaAttackInput::None;
	float CachedAttackInputTime = -100.f;
	float AttackFacingYaw = 0.f;
	float LastAttackEndTime = -100.f;
	bool bWantsGuard = false;
	int32 ComboHits = 0;
	float LastComboHitTime = -100.f;
	FTimerHandle HitStopTimer;

	FRotator BaseMeshRotation = FRotator::ZeroRotator;
	FVector BaseMeshLocation = FVector::ZeroVector;
	float CurrentLean = 0.f;
	float CurrentWallOffset = 0.f;
	float LastYaw = 0.f;

	/** Acrobacias só visuais (giram a malha em volta do centro do corpo; roda em todas as máquinas) */
	void UpdateAcrobatics(float DeltaSeconds);
	float FlipAngle = 0.f;			// cambalhota do dash (0..DashFlipDegrees)
	float DashElapsed = 0.f;
	float SpinTime = -1.f;			// tempo desde o pulo do chão (-1 = sem giro)
	bool bAirArmsSpread = false;	// segundo pulo: braços abertos até pousar
	bool bWasOnGround = true;
	float LastVelocityZ = 0.f;
};

