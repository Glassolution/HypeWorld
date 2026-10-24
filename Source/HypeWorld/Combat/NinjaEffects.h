// Efeitos (GameplayEffects) do combate, configurados em C++.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "NinjaEffects.generated.h"

namespace NinjaSetByCaller
{
	/** Nome do valor de dano passado para o GE_NinjaDamage. */
	inline const FName Damage(TEXT("Data.Damage"));
	/** Chakra ganho a cada pulso (0,1 s) do GE_NinjaChakraCharge. */
	inline const FName ChakraPerTick(TEXT("Data.ChakraPerTick"));
	/** Duração (s) do GE_NinjaStun. */
	inline const FName Duration(TEXT("Data.Duration"));
}

/** Dano instantâneo: soma no atributo Damage (que vira perda de vida). Valor via SetByCaller "Data.Damage". */
UCLASS()
class HYPEWORLD_API UGE_NinjaDamage : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_NinjaDamage();
};

/** Atordoado por um tempo (SetByCaller "Data.Duration"): dá a tag Ninja.State.Stunned, que bloqueia golpes e movimento. */
UCLASS()
class HYPEWORLD_API UGE_NinjaStun : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_NinjaStun();
};

/** Carregar chakra: infinito, a cada 0,1 s soma chakra. Valor via SetByCaller "Data.ChakraPerTick". */
UCLASS()
class HYPEWORLD_API UGE_NinjaChakraCharge : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_NinjaChakraCharge();
};
