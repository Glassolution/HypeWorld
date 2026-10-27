// Habilidade genérica de jutsu (GAS): uma por slot, com a ficha (UNinjaJutsuData) como SourceObject do spec.
// Selos -> lançamento (montagem com o AnimNotify NinjaJutsuRelease) -> volta à prontidão. Custo de chakra e cooldown
// vêm da ficha (SetByCaller), então jutsus novos não precisam de código.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayEffect.h"
#include "NinjaJutsuAbility.generated.h"

class UNinjaJutsuData;

namespace NinjaSetByCaller
{
	/** Chakra gasto pelo jutsu (positivo; o GE subtrai) */
	inline const FName ChakraCost(TEXT("Data.ChakraCost"));
}

UCLASS()
class HYPEWORLD_API UGA_NinjaJutsu : public UGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_NinjaJutsu();

	/** Ficha do jutsu deste spec */
	static const UNinjaJutsuData* GetJutsuData(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo);

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:

	UFUNCTION() void OnSealsDone();
	UFUNCTION() void OnCastFinished();
	UFUNCTION() void OnCastCancelled();

	UPROPERTY() TObjectPtr<const UNinjaJutsuData> Jutsu;
};

/** Gasto de chakra do jutsu (instantâneo, SetByCaller "Data.ChakraCost") */
UCLASS()
class HYPEWORLD_API UGE_NinjaJutsuCost : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_NinjaJutsuCost();
};

/** Cooldown do jutsu: dura SetByCaller "Data.Duration" e dá a CooldownTag da ficha (tag dinâmica do spec) */
UCLASS()
class HYPEWORLD_API UGE_NinjaJutsuCooldown : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_NinjaJutsuCooldown();
};

/** Instante em que o jutsu sai (na montagem de lançamento): o personagem solta o efeito da boca e o servidor cria o projétil */
UCLASS(meta=(DisplayName="Ninja Jutsu Release"))
class HYPEWORLD_API UAnimNotify_NinjaJutsuRelease : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override { return TEXT("NinjaJutsuRelease"); }
};
