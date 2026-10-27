// Habilidade genérica de jutsu (GAS).

#include "NinjaJutsuAbility.h"
#include "NinjaJutsuData.h"
#include "NinjaEffects.h"
#include "NinjaAttributeSet.h"
#include "NinjaCombatTypes.h"
#include "HypeWorldCharacter.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimMontage.h"

// ---------------------------------------------------------------------------------------------------------------------
// Efeitos
// ---------------------------------------------------------------------------------------------------------------------

UGE_NinjaJutsuCost::UGE_NinjaJutsuCost()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = NinjaSetByCaller::ChakraCost;
	FGameplayModifierInfo Mod;
	Mod.Attribute = UNinjaAttributeSet::GetChakraAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Mod);
}

UGE_NinjaJutsuCooldown::UGE_NinjaJutsuCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = NinjaSetByCaller::Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
}

// ---------------------------------------------------------------------------------------------------------------------
// Habilidade
// ---------------------------------------------------------------------------------------------------------------------

UGA_NinjaJutsu::UGA_NinjaJutsu()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationBlockedTags.AddTag(TAG_Ninja_State_Stunned);
}

const UNinjaJutsuData* UGA_NinjaJutsu::GetJutsuData(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo)
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;
	return Spec ? Cast<UNinjaJutsuData>(Spec->SourceObject.Get()) : nullptr;
}

bool UGA_NinjaJutsu::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const AHypeWorldCharacter* Ninja = ActorInfo ? Cast<AHypeWorldCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Ninja || !GetJutsuData(Handle, ActorInfo) || !Ninja->CanStartJutsu())
	{
		return false;
	}
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

bool UGA_NinjaJutsu::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UNinjaJutsuData* Data = GetJutsuData(Handle, ActorInfo);
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!Data || !ASC)
	{
		return false;
	}
	return ASC->GetNumericAttribute(UNinjaAttributeSet::GetChakraAttribute()) >= Data->ChakraCost;
}

void UGA_NinjaJutsu::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UNinjaJutsuData* Data = GetJutsuData(Handle, ActorInfo);
	if (Data && Data->ChakraCost > 0.f)
	{
		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, UGE_NinjaJutsuCost::StaticClass(), 1.f);
		Spec.Data->SetSetByCallerMagnitude(NinjaSetByCaller::ChakraCost, -Data->ChakraCost);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

bool UGA_NinjaJutsu::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UNinjaJutsuData* Data = GetJutsuData(Handle, ActorInfo);
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	return !(Data && ASC && Data->CooldownTag.IsValid() && ASC->HasMatchingGameplayTag(Data->CooldownTag));
}

void UGA_NinjaJutsu::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UNinjaJutsuData* Data = GetJutsuData(Handle, ActorInfo);
	if (Data && Data->Cooldown > 0.f && Data->CooldownTag.IsValid())
	{
		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, UGE_NinjaJutsuCooldown::StaticClass(), 1.f);
		Spec.Data->SetSetByCallerMagnitude(NinjaSetByCaller::Duration, Data->Cooldown);
		Spec.Data->DynamicGrantedTags.AddTag(Data->CooldownTag);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

void UGA_NinjaJutsu::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(GetAvatarActorFromActorInfo());
	Jutsu = GetJutsuData(Handle, ActorInfo);
	if (!Ninja || !Jutsu || (!Jutsu->CastMontage && !Jutsu->BodyCastAnimation) || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Ninja->BeginJutsu(Jutsu);
	if (Jutsu->BodyCastAnimation)
	{
		// A montagem unica do corpo e apresentada tambem nos proxies. O notify do clipe solta o jutsu.
		UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this,
			Jutsu->GetSealsDuration() + Jutsu->BodyCastAnimation->GetPlayLength());
		Wait->OnFinish.AddDynamic(this, &UGA_NinjaJutsu::OnCastFinished);
		Wait->ReadyForActivation();
		return;
	}

	// Selos tocam no corpo visível (o personagem cuida deles); o lançamento começa um pouco antes do último selo acabar
	const float SealsTime = FMath::Max(Jutsu->GetSealsDuration() - Jutsu->CastOverlap, 0.f);
	if (SealsTime > 0.f)
	{
		UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, SealsTime);
		Wait->OnFinish.AddDynamic(this, &UGA_NinjaJutsu::OnSealsDone);
		Wait->ReadyForActivation();
	}
	else
	{
		OnSealsDone();
	}
}

void UGA_NinjaJutsu::OnSealsDone()
{
	if (AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(GetAvatarActorFromActorInfo()))
	{
		Ninja->EndJutsuSeals();
	}
	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Jutsu->CastMontage, 1.f, NAME_None, true, 1.f);
	Task->OnCompleted.AddDynamic(this, &UGA_NinjaJutsu::OnCastFinished);
	Task->OnBlendOut.AddDynamic(this, &UGA_NinjaJutsu::OnCastFinished);
	Task->OnInterrupted.AddDynamic(this, &UGA_NinjaJutsu::OnCastCancelled);
	Task->OnCancelled.AddDynamic(this, &UGA_NinjaJutsu::OnCastCancelled);
	Task->ReadyForActivation();
}

void UGA_NinjaJutsu::OnCastFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_NinjaJutsu::OnCastCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_NinjaJutsu::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(GetAvatarActorFromActorInfo());
	if (Ninja && IsEndAbilityValid(Handle, ActorInfo))
	{
		Ninja->EndJutsu(bWasCancelled);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
// Notify
// ---------------------------------------------------------------------------------------------------------------------

void UAnimNotify_NinjaJutsuRelease::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (AHypeWorldCharacter* Ninja = MeshComp ? Cast<AHypeWorldCharacter>(MeshComp->GetOwner()) : nullptr)
	{
		Ninja->OnJutsuRelease();
	}
}
