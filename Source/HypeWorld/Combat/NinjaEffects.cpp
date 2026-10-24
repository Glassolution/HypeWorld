// Efeitos (GameplayEffects) do combate.

#include "NinjaEffects.h"
#include "NinjaAttributeSet.h"
#include "NinjaCombatTypes.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

namespace
{
	FGameplayModifierInfo MakeSetByCallerMod(const FGameplayAttribute& Attribute, FName DataName)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataName = DataName;

		FGameplayModifierInfo Mod;
		Mod.Attribute = Attribute;
		Mod.ModifierOp = EGameplayModOp::Additive;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		return Mod;
	}
}

UGE_NinjaDamage::UGE_NinjaDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeSetByCallerMod(UNinjaAttributeSet::GetDamageAttribute(), NinjaSetByCaller::Damage));
}

UGE_NinjaStun::UGE_NinjaStun()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = NinjaSetByCaller::Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	FInheritedTagContainer Tags;
	Tags.Added.AddTag(TAG_Ninja_State_Stunned);
	TargetTags->SetAndApplyTargetTagChanges(Tags);
	GEComponents.Add(TargetTags);
}

UGE_NinjaChakraCharge::UGE_NinjaChakraCharge()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(0.1f);
	bExecutePeriodicEffectOnApplication = false;
	Modifiers.Add(MakeSetByCallerMod(UNinjaAttributeSet::GetChakraAttribute(), NinjaSetByCaller::ChakraPerTick));
}
