// Retoques procedurais na pose do ninja, aplicados DEPOIS do gráfico de animação:
// mão na parede, olhar para o chão e preparar as pernas antes de pousar, agachar ao pousar.
// Só visual: não muda colisão nem movimento.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "NinjaAnimInstance.generated.h"

/** Dados que o jogo calcula e a pose usa (a pose é calculada em outra thread, por isso copiamos). */
struct FNinjaPoseInputs
{
	// Direções do personagem no espaço do boneco
	FVector ForwardCS = FVector::ForwardVector;
	FVector RightCS = FVector::RightVector;
	FVector UpCS = FVector::UpVector;

	float WallHandAlpha = 0.f;					// 0..1 mão na parede
	float WallSide = 1.f;						// +1 parede à direita, -1 à esquerda
	FVector WallHandTargetCS = FVector::ZeroVector;
	FVector WallNormalCS = FVector::ZeroVector;	// aponta para fora da parede

	float LookDownAlpha = 0.f;					// 0..1 olhando o chão / pernas se preparando para pousar
	float LandAlpha = 0.f;						// 0..1 agachamento do pouso
	float SprintAlpha = 0.f;					// 0..1 pose da corrida ninja (braços esticados para trás)
};

USTRUCT()
struct FNinjaAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FNinjaAnimInstanceProxy() {}
	FNinjaAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

protected:

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode) override;

private:

	void ApplyNinjaPose(FPoseContext& Output) const;

	/** Copia os dedos de uma animação de mão relaxada (as do Mixamo não convertem bem os dedos). */
	void ApplyRelaxedHands(FPoseContext& Output) const;

	/** Abre os dedos de uma mão (puxa para a pose de referência, que é a mão aberta). */
	void OpenHand(FCompactPose& Pose, bool bRight, float Alpha) const;

	FNinjaPoseInputs Inputs;
	const UAnimSequenceBase* RelaxedHands = nullptr;
	float LandCrouchDepth = 0.f;
	float LookDownSpineAngle = 0.f;
	float LookDownHeadAngle = 0.f;
	float AirLegLift = 0.f;
	float LandSpineAngle = 0.f;
	float SprintExtraLean = 0.f;
	float SprintArmsDrop = 0.f;
};

/** Classe-mãe do Animation Blueprint do ninja (ABP_Ninja). */
UCLASS()
class HYPEWORLD_API UNinjaAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	friend struct FNinjaAnimInstanceProxy;

public:

	// ---------- Ajustes (editáveis no Animation Blueprint, em Class Defaults) ----------

	/** Animação de onde vêm os dedos relaxados (MM_Idle do manequim). Vazio = dedos da própria animação. */
	UPROPERTY(EditAnywhere, Category="Ninja|Mãos")
	UAnimSequenceBase* RelaxedHandsPose = nullptr;

	/** Distância do chão (cm) em que ele começa a olhar para baixo e preparar o pouso. */
	UPROPERTY(EditAnywhere, Category="Ninja|Pouso")
	float PrepareLandDistance = 400.f;

	/** Velocidade de queda (cm/s) a partir da qual o pouso tem agachamento. */
	UPROPERTY(EditAnywhere, Category="Ninja|Pouso")
	float LandMinFallSpeed = 600.f;

	/** Velocidade de queda (cm/s) do agachamento máximo. */
	UPROPERTY(EditAnywhere, Category="Ninja|Pouso")
	float LandMaxFallSpeed = 1800.f;

	/** Quanto o quadril desce no pouso mais forte (cm). */
	UPROPERTY(EditAnywhere, Category="Ninja|Pouso")
	float LandCrouchDepth = 30.f;

	/** Duração do agachamento do pouso (s). */
	UPROPERTY(EditAnywhere, Category="Ninja|Pouso")
	float LandDuration = 0.45f;

	/** Inclinação do tronco para frente no pouso (graus). */
	UPROPERTY(EditAnywhere, Category="Ninja|Pouso")
	float LandSpineAngle = 20.f;

	/** Quanto as pernas encolhem no ar antes de pousar (cm). */
	UPROPERTY(EditAnywhere, Category="Ninja|Pouso")
	float AirLegLift = 25.f;

	/** Inclinação do tronco olhando o chão (graus). */
	UPROPERTY(EditAnywhere, Category="Ninja|Pouso")
	float LookDownSpineAngle = 12.f;

	/** Inclinação da cabeça olhando o chão (graus). */
	UPROPERTY(EditAnywhere, Category="Ninja|Pouso")
	float LookDownHeadAngle = 30.f;

	/** Velocidade no chão a partir da qual entra a pose de corrida ninja (cm/s). */
	UPROPERTY(EditAnywhere, Category="Ninja|Corrida")
	float SprintPoseMinSpeed = 700.f;

	/** Inclinação extra do tronco na corrida ninja (graus). */
	UPROPERTY(EditAnywhere, Category="Ninja|Corrida")
	float SprintExtraLean = 8.f;

	/** Quanto os braços apontam para baixo na corrida ninja (0 = horizontais para trás, 1 = bem para baixo). */
	UPROPERTY(EditAnywhere, Category="Ninja|Corrida")
	float SprintArmsDrop = 0.05f;

	/** Rapidez com que a mão vai até a parede / volta. */
	UPROPERTY(EditAnywhere, Category="Ninja|Parede")
	float WallHandBlendSpeed = 10.f;

protected:

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FNinjaAnimInstanceProxy(this); }

private:

	FNinjaPoseInputs PoseInputs;

	bool bWasInAir = false;
	float LastVelocityZ = 0.f;
	float LandStrength = 0.f;
	float LandTime = 0.f;
};
