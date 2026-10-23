// Movimento ninja: corrida ninja, dash e corrida na parede, com predição no cliente (pronto para multiplayer).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NinjaMovementComponent.generated.h"

class UAnimSequenceBase;

/** Modos de movimento próprios (usados com MOVE_Custom). */
UENUM(BlueprintType)
enum ENinjaMoveMode : uint8
{
	NMOVE_None		UMETA(Hidden),
	NMOVE_Dash		UMETA(DisplayName = "Dash"),
	NMOVE_WallRun	UMETA(DisplayName = "Corrida na parede"),
};

UCLASS()
class HYPEWORLD_API UNinjaMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	friend class FSavedMove_Ninja;

public:

	// ---------- Ajustes (editáveis no Blueprint) ----------

	/** Velocidade da corrida ninja (segurando o botão de correr). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Corrida")
	float SprintSpeed = 800.f;

	/** Corrida normal (segurar Shift). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Corrida")
	float RunSpeed = 500.f;

	/** Aceleração acima da velocidade de corrida normal (sobe para a corrida ninja em ~0,35 s, sem tranco). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Corrida")
	float SprintAcceleration = 1100.f;

	/** Modo foco (segurar botão direito): de frente para a câmera, andando de lado/de costas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Foco")
	float FocusSpeed = 170.f;

	/** Velocidade andando de costas no modo foco (a animação de costas é mais lenta). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Foco")
	float FocusBackSpeed = 110.f;

	/** Velocidade máxima enquanto ataca (o embalo da corrida diminui suave até aqui). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Corrida")
	float AttackMaxSpeed = 250.f;

	/** Animação cujo deslocamento o dash segue (arranca, acelera e freia junto com ela). Vazio = velocidade constante. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Dash")
	UAnimSequenceBase* DashRootMotionAnim = nullptr;

	/** Velocidade de reprodução da animação do dash (1 = normal). O personagem toca a montagem nessa mesma velocidade. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Dash")
	float DashAnimRate = 1.2f;

	/** Até que segundo da animação o dash anda; o resto da animação é só a aterrissagem. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Dash")
	float DashAnimMoveEnd = 0.75f;

	/** Fração da distância da animação que o dash percorre (MM_Dash anda ~8,8 m; 0,7 = ~6,2 m). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Dash")
	float DashDistanceScale = 0.7f;

	/** Sem animação: velocidade constante do dash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Dash")
	float DashSpeed = 2400.f;

	/** Sem animação: quanto tempo o dash dura (segundos). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Dash")
	float DashDuration = 0.2f;

	/** Tempo de espera entre dashes (segundos), contado do início do dash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Dash")
	float DashCooldown = 0.8f;

	/** Duração real do dash (segue a animação quando houver uma). */
	float GetDashDuration() const;

	/** Velocidade correndo na parede. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Parede")
	float WallRunSpeed = 1100.f;

	/** Tempo máximo correndo na parede (segundos). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Parede")
	float WallRunMaxDuration = 1.5f;

	/** Velocidade mínima no ar para grudar na parede. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Parede")
	float WallRunMinSpeed = 400.f;

	/** Fração da gravidade aplicada na parede (0 = não cai, 1 = cai normal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Parede")
	float WallRunGravityScale = 0.15f;

	/** Velocidade máxima para cima ao grudar na parede (o resto do impulso do pulo é cortado). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Parede")
	float WallRunEntryMaxUpSpeed = 100.f;

	/** Distância extra (além da cápsula) para detectar a parede. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Parede")
	float WallDetectDistance = 40.f;

	/** Impulso para longe da parede no pulo de parede. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja|Parede")
	float WallJumpOffForce = 700.f;

	// ---------- Comandos (chamados pelo personagem) ----------

	void SetSprinting(bool bSprint) { bWantsToSprint = bSprint; }
	void RequestDash() { bWantsToDash = true; }
	void SetFocusing(bool bFocus) { bWantsToFocus = bFocus; }
	void SetRunning(bool bRun) { bWantsToRun = bRun; }

	UFUNCTION(BlueprintPure, Category="Ninja")
	bool IsRunning() const { return bWantsToRun && !bWantsToSprint && IsMovingOnGround(); }

	UFUNCTION(BlueprintPure, Category="Ninja")
	bool IsFocusing() const { return bWantsToFocus && !bWantsToSprint && IsMovingOnGround(); }

	UFUNCTION(BlueprintPure, Category="Ninja")
	bool IsSprinting() const { return bWantsToSprint && IsMovingOnGround(); }

	UFUNCTION(BlueprintPure, Category="Ninja")
	bool IsDashing() const { return IsCustomMode(NMOVE_Dash); }

	UFUNCTION(BlueprintPure, Category="Ninja")
	bool IsWallRunning() const { return IsCustomMode(NMOVE_WallRun); }

	bool IsCustomMode(ENinjaMoveMode Mode) const { return MovementMode == MOVE_Custom && CustomMovementMode == Mode; }

	/** Procura uma parede quase vertical na direção indicada (horizontal). */
	bool TraceForWall(const FVector& Direction, FHitResult& OutHit) const;

	/** Acha a parede ao lado do personagem. OutSide: +1 parede à direita, -1 à esquerda. */
	bool FindWallBeside(FHitResult& OutHit, float& OutSide) const;

	// ---------- UCharacterMovementComponent ----------

	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;
	virtual bool CanAttemptJump() const override;
	virtual bool DoJump(bool bReplayingMoves, float DeltaTime) override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

protected:

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

private:

	void PhysDash(float DeltaTime, int32 Iterations);
	void PhysWallRun(float DeltaTime, int32 Iterations);

	void StartDash();
	void EndDash();
	bool TryStartWallRun();
	void EndWallRun();

	bool CanDash() const;

	/** Sai do modo próprio para andar (se houver chão) ou cair. */
	void ExitToWalkingOrFalling();

	// Entradas enviadas ao servidor a cada movimento
	bool bWantsToSprint = false;
	bool bWantsToDash = false;
	bool bWantsToFocus = false;
	bool bWantsToRun = false;

	// Estado do movimento (também guardado em FSavedMove_Ninja para refazer movimentos após correção do servidor)
	float DashTimeLeft = 0.f;
	float DashCooldownLeft = 0.f;
	FVector DashDirection = FVector::ForwardVector;
	bool bAirDashUsed = false;

	float WallRunTimeLeft = 0.f;
	float WallRunCooldownLeft = 0.f;
	FVector WallNormal = FVector::ZeroVector;
};

/** Um "movimento salvo": o que o cliente fez num frame, para o servidor conferir e o cliente refazer se for corrigido. */
class FSavedMove_Ninja : public FSavedMove_Character
{
	using Super = FSavedMove_Character;

public:

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;

	uint8 bSavedWantsToSprint : 1 = 0;
	uint8 bSavedWantsToDash : 1 = 0;
	uint8 bSavedWantsToFocus : 1 = 0;
	uint8 bSavedWantsToRun : 1 = 0;
	uint8 bSavedAirDashUsed : 1 = 0;
	float SavedDashTimeLeft = 0.f;
	float SavedDashCooldownLeft = 0.f;
	FVector SavedDashDirection = FVector::ForwardVector;
	float SavedWallRunTimeLeft = 0.f;
	float SavedWallRunCooldownLeft = 0.f;
	FVector SavedWallNormal = FVector::ZeroVector;
};

class FNetworkPredictionData_Client_Ninja : public FNetworkPredictionData_Client_Character
{
	using Super = FNetworkPredictionData_Client_Character;

public:

	FNetworkPredictionData_Client_Ninja(const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement) {}

	virtual FSavedMovePtr AllocateNewMove() override { return FSavedMovePtr(new FSavedMove_Ninja()); }
};
