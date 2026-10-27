#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NinjaDodgeDefinition.generated.h"

class UAnimMontage;
class UCurveFloat;

/** Uma esquiva de combate. Troque a montagem neste asset para substituir a animação. */
UCLASS(BlueprintType)
class HYPEWORLD_API UNinjaDodgeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Esquiva") TObjectPtr<UAnimMontage> Montage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Esquiva", meta=(ClampMin="0.4", ClampMax="0.6")) float Duration = 0.55f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Esquiva", meta=(ClampMin="300", ClampMax="500")) float Distance = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Esquiva") bool bUseRootMotion = false;
	/** Distância horizontal da raiz na montagem, antes da escala aplicada pelo personagem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Esquiva") float AuthoredRootDistance = 400.f;
	/** Rotação do corpo durante a acrobacia; volta a zero no pouso. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Esquiva") float BodyYawOffset = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Esquiva") float CancelStart = 0.43f;
	/** Curva de distância acumulada: tempo normalizado 0..1, distância normalizada 0..1. Vazia = ease-out quadrático. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Esquiva") TObjectPtr<UCurveFloat> DistanceCurve;
};
