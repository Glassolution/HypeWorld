// Passivas: habilidades sempre ativas que o jogador liga/desliga (tecla própria ou menu de combate → Passivas).
// Vêm de aprendizado, do clã etc. A primeira: Infusão de Chakra nos pés (andar na água, paredes e teto, super pulo).

#pragma once

#include "CoreMinimal.h"
#include "NinjaPassives.generated.h"

UENUM(BlueprintType)
enum class ENinjaPassive : uint8
{
	ChakraFeet	UMETA(DisplayName="Infusão de Chakra nos pés"),
};

/** Ficha de uma passiva (para o menu): nome, descrição, de onde veio e se já foi aprendida. */
USTRUCT(BlueprintType)
struct FNinjaPassiveInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Passiva")
	ENinjaPassive Id = ENinjaPassive::ChakraFeet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Passiva")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Passiva")
	FText Description;

	/** De onde veio (Aprendida, Clã, Especialização...) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Passiva")
	FText Origin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Passiva")
	bool bLearned = true;

	/** Começa ligada */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Passiva")
	bool bEnabledByDefault = true;
};
