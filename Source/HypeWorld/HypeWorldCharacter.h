// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "CombatAttacker.h"
#include "NinjaCombatTypes.h"
#include "HypeWorldCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UNinjaMovementComponent;
class UAnimMontage;
class UAbilitySystemComponent;
class UNinjaAttributeSet;
class UNinjaChakraAuraComponent;
class UGameplayAbility;
class UMaterialInterface;
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

	/** Vida e chakra */
	UPROPERTY()
	UNinjaAttributeSet* Attributes;

	/** Aura de chakra (aparece ao carregar) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UNinjaChakraAuraComponent* ChakraAura;
	
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

	/** Animação do dash */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* DashMontage;

	/** Animação do pulo de parede */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* WallJumpMontage;

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

	/** Correndo rápido, A/D viram em arco: no máximo este ângulo em relação à direção atual (graus) */
	UPROPERTY(EditAnywhere, Category="Ninja|Movimento")
	float SprintSteerAngle = 35.f;

	/** Rapidez de giro do corpo correndo rápido (graus/s): menor = curva mais aberta */
	UPROPERTY(EditAnywhere, Category="Ninja|Movimento")
	float SprintRotationRate = 360.f;

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

	/** Quanto a animação de corrida pode ser acelerada no máximo (2,7x = corrida de 4,5 m/s chega a ~12 m/s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float MaxRunAnimRate = 2.7f;

public:

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
	void EndAttack(bool bWasCancelled);

	/** Defendendo (botão direito parado ou andando) */
	UFUNCTION(BlueprintPure, Category="Ninja|Combate")
	bool IsGuarding() const;

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

	/** Constructor */
	AHypeWorldCharacter(const FObjectInitializer& ObjectInitializer);

protected:

	virtual void BeginPlay() override;
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
	void MulticastHitReaction(ENinjaHitReaction Reaction, FVector_NetQuantizeNormal FromDirection);

	UFUNCTION(Server, Reliable)
	void ServerSetGuarding(bool bNewGuarding);

	UPROPERTY(Replicated)
	bool bGuarding = false;

	UFUNCTION()
	void OnRep_ChargingChakra();

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

	/** Começa a corrida ninja */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintStart();

	/** Para a corrida ninja */
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

	/** Ao sair do lugar numa direção muito diferente, toca a animação de virar e gira no mesmo ritmo */
	void UpdateTurnInPlace(float DeltaSeconds);

	float LastLookInputTime = -100.f;
	float StillTime = 0.f;
	float TurnTimeLeft = 0.f;
	float DefaultRotationRate = 720.f;

	/** Conta o tempo parado e toca a animação de olhar em volta depois de IdleBreakDelay */
	void UpdateIdleBreak(float DeltaSeconds);

	float IdleTime = 0.f;

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

