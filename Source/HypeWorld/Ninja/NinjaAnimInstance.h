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
	float StepSway = 0.f;						// -1..1 pé de apoio na corrida ninja (+1 = direito no chão)
	float ArmSway = 0.f;						// StepSway atrasado: as mãos seguem o corpo depois
	float ArmBob = 0.f;							// cm que as mãos ficam para trás no sobe e desce do corpo (flutuam)
	float ChargeAlpha = 0.f;					// 0..1 pose de concentrar chakra (base larga, mãos juntas no peito)
	float GuardAlpha = 0.f;						// 0..1 pose de defesa (antebraços na frente do rosto)
	float FistAlpha = 0.f;						// 0..1 punhos fechados (golpes e defesa)
	float ArmsSpreadAlpha = 0.f;				// 0..1 braços abertos (dash e segundo pulo)
	float BattleAlpha = 0.f;					// 0..1 pose de luta parada (modo de batalha)
	float BattleTime = 0.f;						// relógio da pose de luta (ela é um loop)
	float Time = 0.f;							// relógio da animação (respiração)
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

	/** Modo de batalha parado: mistura a pose de luta por cima do parado normal. */
	void ApplyBattleIdle(FPoseContext& Output) const;
	const UAnimSequenceBase* BattleIdle = nullptr;

	/** Abre os dedos de uma mão (puxa para a pose de referência, que é a mão aberta). */
	void OpenHand(FCompactPose& Pose, bool bRight, float Alpha, bool bOnlyIndexAndMiddle = false) const;

	FNinjaPoseInputs Inputs;
	bool bProceduralPose = true;
	const UAnimSequenceBase* RelaxedHands = nullptr;
	const UAnimSequenceBase* FistHands = nullptr;
	float FistPoseTime = 0.f;
	float LandCrouchDepth = 0.f;
	float LookDownSpineAngle = 0.f;
	float LookDownHeadAngle = 0.f;
	float AirLegLift = 0.f;
	float LandSpineAngle = 0.f;
	float SprintExtraLean = 0.f;
	float SprintArmsDrop = 0.f;
	float SprintArmsInward = 0.f;
	float SprintStepSway = 0.f;
	float SprintHandFloat = 0.f;
	float SprintFootStraight = 0.f;
	float SprintCrouchDepth = 0.f;
	float SprintArmLock = 0.f;
	float SprintStraighten = 0.f;
	float SprintFootNarrow = 0.f;
	float SprintHandRelax = 0.f;
	float GuardHandForward = 0.f;
	float GuardHandSpread = 0.f;
	float ChargeCrouchDepth = 0.f;
	float ChargeStanceWidth = 0.f;
	float ChargeToeOut = 0.f;
	float ChargeKneeOut = 0.f;
	float ChargeStraighten = 0.f;
	float ChargeStagger = 0.f;
	float ChargeLean = 0.f;
	float ChargeHandForward = 0.f;
	float ChargeHandDrop = 0.f;
};

/** Classe-mãe do Animation Blueprint do ninja (ABP_Ninja). */
UCLASS()
class HYPEWORLD_API UNinjaAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	friend struct FNinjaAnimInstanceProxy;

public:

	// ---------- Ajustes (editáveis no Animation Blueprint, em Class Defaults) ----------

	/** Liga/desliga TODOS os retoques por código (dedos, corrida ninja, parede, pouso, chakra, defesa, punho, braços abertos).
	 *  Desligado = o boneco mostra só as animações do gráfico, como vieram. */
	UPROPERTY(EditAnywhere, Category="Ninja")
	bool bProceduralPose = true;

	/** Animação de onde vêm os dedos relaxados (MM_Idle do manequim). Vazio = dedos da própria animação. */
	UPROPERTY(EditAnywhere, Category="Ninja|Mãos")
	UAnimSequenceBase* RelaxedHandsPose = nullptr;

	/** Animação de onde vêm os punhos fechados (golpes e defesa) e o momento dela (s). */
	UPROPERTY(EditAnywhere, Category="Ninja|Mãos")
	UAnimSequenceBase* FistHandsPose = nullptr;
	UPROPERTY(EditAnywhere, Category="Ninja|Mãos")
	float FistPoseTime = 0.4f;

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
	float SprintExtraLean = 0.f;

	/** Quanto os braços apontam para baixo na corrida ninja (0 = horizontais para trás, 1 = bem para baixo, negativo = acima da horizontal). */
	UPROPERTY(EditAnywhere, Category="Ninja|Corrida")
	float SprintArmsDrop = 0.05f;

	/** Quanto os braços fecham para o centro das costas na corrida ninja (0 = retos para trás, 0,5 = mãos perto da coluna). */
	UPROPERTY(EditAnywhere, Category="Ninja|Corrida")
	float SprintArmsInward = 0.f;

	/** Quanto o tronco balança para o lado do pé que pisa, a cada passada da corrida ninja (graus). */
	UPROPERTY(EditAnywhere, Category="Ninja|Corrida")
	float SprintStepSway = 5.f;

	/** Quanto as mãos flutuam atrasadas no balanço e no sobe e desce do corpo (0 = presas, 1 = normal). */
	UPROPERTY(EditAnywhere, Category="Ninja|Corrida")
	float SprintHandFloat = 1.f;

	/** Quanto as pontas dos pés apontam para onde ele corre (0 = como a animação, 1 = retas). */
	UPROPERTY(EditAnywhere, Category="Ninja|Corrida")
	float SprintFootStraight = 0.8f;

	/** Quanto o quadril desce na corrida ninja (cm). 0 = pernas livres como na animação (passada natural). */
	UPROPERTY(EditAnywhere, Category="Ninja|Corrida")
	float SprintCrouchDepth = 0.f;

	/** Quanto os braços ficam presos para trás (1 = parados; menos = balançam um pouco com as passadas). */
	UPROPERTY(EditAnywhere, Category="Ninja|Corrida")
	float SprintArmLock = 0.72f;

	/** Quanto o tronco é levantado na corrida ninja (0 = deitado como a animação, 1 = em pé). */
	UPROPERTY(EditAnywhere, Category="Ninja|Corrida")
	float SprintStraighten = 0.3f;

	/** Quanto as pisadas vão para o eixo do corpo (0 = como a animação, 1 = um pé na frente do outro). */
	UPROPERTY(EditAnywhere, Category="Ninja|Corrida")
	float SprintFootNarrow = 0.55f;

	/** Mãos leves: 1 = dedos esticados e firmes, 0 = soltos como na animação. */
	UPROPERTY(EditAnywhere, Category="Ninja|Corrida")
	float SprintHandRelax = 0.45f;

	/** Quanto o quadril desce concentrando chakra (cm). Base de artes marciais: joelhos dobrados 45-60°. */
	UPROPERTY(EditAnywhere, Category="Ninja|Chakra")
	float ChargeCrouchDepth = 9.f;

	/** Distância entre os pés concentrando chakra (cm) — cerca de 1,5x a largura dos ombros. */
	UPROPERTY(EditAnywhere, Category="Ninja|Chakra")
	float ChargeStanceWidth = 46.f;

	/** Quanto as pontas dos pés giram para fora (graus); os joelhos acompanham a direção dos pés. */
	UPROPERTY(EditAnywhere, Category="Ninja|Chakra")
	float ChargeToeOut = 14.f;

	/** Quanto os joelhos abrem para fora (0 = para a frente, 1 = bem abertos). */
	UPROPERTY(EditAnywhere, Category="Ninja|Chakra")
	float ChargeKneeOut = 0.35f;

	/** Quanto a coluna é endireitada (0 = como a animação, 1 = totalmente vertical). */
	UPROPERTY(EditAnywhere, Category="Ninja|Chakra")
	float ChargeStraighten = 0.5f;

	/** Um pé à frente do outro (cm): base assimétrica, mais humana. */
	UPROPERTY(EditAnywhere, Category="Ninja|Chakra")
	float ChargeStagger = 12.f;

	/** Tronco inclinado para a frente, "entrando" no selo (graus). */
	UPROPERTY(EditAnywhere, Category="Ninja|Chakra")
	float ChargeLean = 10.f;

	/** Distância das mãos à frente do pescoço (cm) — no anime ficam perto do queixo. */
	UPROPERTY(EditAnywhere, Category="Ninja|Chakra")
	float ChargeHandForward = 20.f;

	/** Quanto as mãos ficam abaixo da base do pescoço (cm) — 0 = altura do queixo. */
	UPROPERTY(EditAnywhere, Category="Ninja|Chakra")
	float ChargeHandDrop = 6.f;

	/** Defesa: punhos à frente do rosto (cm à frente do pescoço) e afastados do centro (cm). */
	UPROPERTY(EditAnywhere, Category="Ninja|Defesa")
	float GuardHandForward = 24.f;
	UPROPERTY(EditAnywhere, Category="Ninja|Defesa")
	float GuardHandSpread = 9.f;

	/** Parado em modo de batalha (segurar botão direito / depois de bater ou apanhar): pose de luta em loop.
	 *  Vazio = sem pose de luta (só o parado normal do gráfico). */
	UPROPERTY(EditAnywhere, Category="Ninja|Batalha")
	UAnimSequenceBase* BattleIdlePose = nullptr;

	/** Rapidez para entrar/sair da pose de luta. */
	UPROPERTY(EditAnywhere, Category="Ninja|Batalha")
	float BattleBlendSpeed = 6.f;

	/** Andando acima desta velocidade (cm/s) a pose de luta some (vale a animação de andar/correr). */
	UPROPERTY(EditAnywhere, Category="Ninja|Batalha")
	float BattleIdleMaxSpeed = 120.f;

	/** Rapidez com que a mão vai até a parede / volta. */
	UPROPERTY(EditAnywhere, Category="Ninja|Parede")
	float WallHandBlendSpeed = 10.f;

protected:

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FNinjaAnimInstanceProxy(this); }

private:

	FNinjaPoseInputs PoseInputs;

	/** Direção do movimento em relação ao corpo, -180..180 (o gráfico do template limitava a ±45°, sem andar de lado/de costas). */
	float FullDirection = 0.f;

	bool bWasInAir = false;
	bool bTrackingPelvis = false;
	float PelvisHeightSmooth = 0.f;
	float LastVelocityZ = 0.f;
	float LandStrength = 0.f;
	float LandTime = 0.f;
	float BattleAlpha = 0.f;
	float BattleTime = 0.f;
};
