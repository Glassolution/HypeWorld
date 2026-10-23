// Atributos de combate do ninja (vida e chakra), replicados para todos os jogadores.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "NinjaAttributeSet.generated.h"

UCLASS()
class HYPEWORLD_API UNinjaAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	UNinjaAttributeSet();

	UPROPERTY(BlueprintReadOnly, Category="Ninja", ReplicatedUsing=OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UNinjaAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category="Ninja", ReplicatedUsing=OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UNinjaAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category="Ninja", ReplicatedUsing=OnRep_Chakra)
	FGameplayAttributeData Chakra;
	ATTRIBUTE_ACCESSORS_BASIC(UNinjaAttributeSet, Chakra)

	UPROPERTY(BlueprintReadOnly, Category="Ninja", ReplicatedUsing=OnRep_MaxChakra)
	FGameplayAttributeData MaxChakra;
	ATTRIBUTE_ACCESSORS_BASIC(UNinjaAttributeSet, MaxChakra)

	/** "Meta" atributo: o dano chega aqui e é convertido em perda de vida (só no servidor, não replica). */
	UPROPERTY(BlueprintReadOnly, Category="Ninja")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS_BASIC(UNinjaAttributeSet, Damage)

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	UFUNCTION() void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_Chakra(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxChakra(const FGameplayAttributeData& OldValue);
};
