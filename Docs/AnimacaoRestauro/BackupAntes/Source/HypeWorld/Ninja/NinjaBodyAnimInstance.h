// Corpo visível que recebe a pose do manequim por retarget: depois do gráfico, distribui a torção de mãos, braços,
// coxas e pés em ossos de torção, para os membros não afinarem quando giram em volta do próprio eixo.
// As animações do pack não animam os ossos de torção, então eles são calculados aqui.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "NinjaBodyAnimInstance.generated.h"

/** Um osso de torção: copia Fraction da torção do Driver em volta do eixo AxisFrom->AxisTo (pose de referência).
 *  Fraction negativa desfaz a torção (ossos perto do ombro e do quadril). */
USTRUCT(BlueprintType)
struct FNinjaTwistBone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Torção")
	FName TwistBone;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Torção")
	FName DriverBone;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Torção")
	FName AxisFrom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Torção")
	FName AxisTo;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Torção")
	float Fraction = 0.5f;
};

USTRUCT()
struct FNinjaBodyAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FNinjaBodyAnimInstanceProxy() {}
	FNinjaBodyAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

protected:

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode) override;

private:

	void ApplyTwist(FPoseContext& Output) const;
	TArray<FNinjaTwistBone> TwistBones;
};

UCLASS()
class HYPEWORLD_API UNinjaBodyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	UNinjaBodyAnimInstance();

	/** Ossos de torção do corpo (os que não existirem no esqueleto são ignorados) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ninja|Torção")
	TArray<FNinjaTwistBone> TwistBones;

protected:

	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FNinjaBodyAnimInstanceProxy(this); }
};
