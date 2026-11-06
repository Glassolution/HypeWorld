#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SilentStanceAnimInstance.generated.h"

/** Keeps gaze and relaxed locomotion hands anatomical after retargeting. */
UCLASS(Transient, Blueprintable)
class HYPEWORLD_API USilentStanceAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    /** Locomotion only. Combat and montages retain the complete authored finger motion. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Silent Stance|Hands")
    bool bUseRelaxedLocomotionHands = true;

    /** Enables the locomotion hand pose on a diagnostic SkeletalMeshActor without a pawn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category="Silent Stance|Hands")
    bool bPreviewLocomotionHands = false;

    /** Applies the robe stance to the idle/walk comparison actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category="Silent Stance|Preview")
    bool bPreviewIzunaStance = false;

    /** Enables the robe's stationary foot placement on the idle comparison actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category="Silent Stance|Preview")
    bool bPreviewIzunaIdleFeet = false;

protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;
};
