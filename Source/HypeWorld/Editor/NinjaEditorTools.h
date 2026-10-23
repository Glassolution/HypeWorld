// Ferramentas chamadas pelos scripts do editor (Tools/editor/*.py) para coisas que o Python não alcança.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NinjaEditorTools.generated.h"

class UBlendSpace;

UCLASS()
class HYPEWORLD_API UNinjaEditorTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Reconstrói os dados internos (triangulação) de um blend space depois de trocar amostras por script. */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool RebuildBlendSpace(UBlendSpace* BlendSpace);
};
