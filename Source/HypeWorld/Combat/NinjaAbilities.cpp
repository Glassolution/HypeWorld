// Habilidades de combate (GAS): combo corpo a corpo e carregar chakra.

#include "NinjaAbilities.h"
#include "NinjaEffects.h"
#include "NinjaCombatTypes.h"
#include "HypeWorldCharacter.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

// ---------------------------------------------------------------------------------------------------------------------
// Combo
// ---------------------------------------------------------------------------------------------------------------------

UGA_NinjaMeleeCombo::UGA_NinjaMeleeCombo()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	bRetriggerInstancedAbility = true;							// golpe novo corta o anterior (janela de combo)
	ActivationBlockedTags.AddTag(TAG_Ninja_State_Stunned);		// atordoado não ataca
}

void UGA_NinjaMeleeCombo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(GetAvatarActorFromActorInfo());
	const int32 AttackId = TriggerEventData ? FMath::RoundToInt(TriggerEventData->EventMagnitude) : 0;
	const FNinjaAttack* Attack = Ninja ? Ninja->GetAttack(AttackId) : nullptr;
	if (!Ninja || !Attack || !Attack->Montage || !Ninja->CanActivateAttack(AttackId)
		|| Ninja->IsChargingChakra() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Ninja->BeginAttack(AttackId);

	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Attack->Montage, Attack->PlayRate, NAME_None, true, 1.f);
	Task->OnCompleted.AddDynamic(this, &UGA_NinjaMeleeCombo::OnMontageFinished);
	Task->OnBlendOut.AddDynamic(this, &UGA_NinjaMeleeCombo::OnMontageFinished);
	Task->OnInterrupted.AddDynamic(this, &UGA_NinjaMeleeCombo::OnMontageCancelled);
	Task->OnCancelled.AddDynamic(this, &UGA_NinjaMeleeCombo::OnMontageCancelled);
	Task->ReadyForActivation();
}

void UGA_NinjaMeleeCombo::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(GetAvatarActorFromActorInfo());
	if (Ninja && IsEndAbilityValid(Handle, ActorInfo))
	{
		Ninja->EndAttack(bWasCancelled);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_NinjaMeleeCombo::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_NinjaMeleeCombo::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

// ---------------------------------------------------------------------------------------------------------------------
// Carregar chakra
// ---------------------------------------------------------------------------------------------------------------------

UGA_NinjaChargeChakra::UGA_NinjaChargeChakra()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationBlockedTags.AddTag(TAG_Ninja_State_Stunned);
}

void UGA_NinjaChargeChakra::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(GetAvatarActorFromActorInfo());
	if (!Ninja || !Ninja->CanChargeChakra() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Ninja->SetChargingChakra(true);

	// O chakra só sobe no servidor (ele é quem manda nos atributos); o resultado replica para todos
	if (HasAuthority(&ActivationInfo))
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGE_NinjaChakraCharge::StaticClass(), 1.f, ASC->MakeEffectContext());
		Spec.Data->SetSetByCallerMagnitude(NinjaSetByCaller::ChakraPerTick, ChakraPerSecond * 0.1f);
		ChargeEffect = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void UGA_NinjaChargeChakra::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ChargeEffect.IsValid() && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ChargeEffect);
		ChargeEffect.Invalidate();
	}
	if (AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(GetAvatarActorFromActorInfo()))
	{
		Ninja->SetChargingChakra(false);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
