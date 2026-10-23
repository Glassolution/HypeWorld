// Atributos de combate do ninja (vida e chakra).

#include "NinjaAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UNinjaAttributeSet::UNinjaAttributeSet()
{
	InitMaxHealth(500.f);
	InitHealth(500.f);
	InitMaxChakra(200.f);
	InitChakra(100.f);
	InitDamage(0.f);
}

void UNinjaAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetChakraAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxChakra());
	}
}

void UNinjaAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float LocalDamage = GetDamage();
		SetDamage(0.f);
		if (LocalDamage > 0.f)
		{
			SetHealth(FMath::Clamp(GetHealth() - LocalDamage, 0.f, GetMaxHealth()));
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetChakraAttribute())
	{
		SetChakra(FMath::Clamp(GetChakra(), 0.f, GetMaxChakra()));
	}
}

void UNinjaAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UNinjaAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNinjaAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNinjaAttributeSet, Chakra, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNinjaAttributeSet, MaxChakra, COND_None, REPNOTIFY_Always);
}

void UNinjaAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UNinjaAttributeSet, Health, OldValue); }
void UNinjaAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UNinjaAttributeSet, MaxHealth, OldValue); }
void UNinjaAttributeSet::OnRep_Chakra(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UNinjaAttributeSet, Chakra, OldValue); }
void UNinjaAttributeSet::OnRep_MaxChakra(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UNinjaAttributeSet, MaxChakra, OldValue); }
