// Habilidades de combate (GAS): combo corpo a corpo e carregar chakra.
// LocalPredicted: rodam na hora no jogador e o servidor confirma (pronto para multiplayer).

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "NinjaAbilities.generated.h"

class UAnimMontage;

/** Um golpe do combo: o jogador escolhe qual (id no EventMagnitude do evento) e o servidor toca o mesmo.
 *  Cada golpe novo reinicia a habilidade (bRetriggerInstancedAbility), cortando o anterior na janela de combo. */
UCLASS()
class HYPEWORLD_API UGA_NinjaMeleeCombo : public UGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_NinjaMeleeCombo();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:

	UFUNCTION() void OnMontageFinished();
	UFUNCTION() void OnMontageCancelled();
};

/** Segurar para carregar chakra: parado, aura acesa, chakra subindo até soltar o botão. */
UCLASS()
class HYPEWORLD_API UGA_NinjaChargeChakra : public UGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_NinjaChargeChakra();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Chakra ganho por segundo carregando. */
	UPROPERTY(EditDefaultsOnly, Category="Ninja")
	float ChakraPerSecond = 45.f;

private:

	FActiveGameplayEffectHandle ChargeEffect;
};
