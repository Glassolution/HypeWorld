// Ferramentas chamadas pelos scripts do editor (Tools/editor/*.py) para coisas que o Python não alcança.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NinjaEditorTools.generated.h"

class UBlendSpace;
class UAnimMontage;
class UAnimSequenceBase;

UCLASS()
class HYPEWORLD_API UNinjaEditorTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Reconstrói os dados internos (triangulação) de um blend space depois de trocar amostras por script. */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool RebuildBlendSpace(UBlendSpace* BlendSpace);

	/** Recorta um golpe de uma animação: a montagem toca só [StartTime, EndTime] da animação.
	 *  HitTimes = momentos de acerto (AnimNotify DoAttackTrace); ComboTime = abre a janela do próximo golpe (CheckCombo, <0 = sem).
	 *  Tempos no relógio da animação original. */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool BuildAttackMontage(UAnimMontage* Montage, UAnimSequenceBase* Animation, float StartTime, float EndTime,
		const TArray<float>& HitTimes, float ComboTime, float BlendIn, float BlendOut);
};
