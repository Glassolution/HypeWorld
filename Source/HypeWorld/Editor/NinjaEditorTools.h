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

	/** Montagem com vários trechos em sequência (Animations[i] de Starts[i] a Ends[i]; End < 0 = até o fim).
	 *  bLoopLast = o último trecho repete sozinho (seção "Loop" que aponta para ela mesma).
	 *  bHoldLastPose = não volta sozinha para a animação de base no fim (fica na última pose até alguém parar). */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool BuildSequenceMontage(UAnimMontage* Montage, const TArray<UAnimSequenceBase*>& Animations, const TArray<float>& Starts,
		const TArray<float>& Ends, float BlendIn, float BlendOut, bool bLoopLast, bool bHoldLastPose);
};
