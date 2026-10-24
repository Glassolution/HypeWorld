// Boneco de treino.

#include "NinjaTrainingDummy.h"
#include "NinjaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"

ANinjaTrainingDummy::ANinjaTrainingDummy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AutoPossessAI = EAutoPossessAI::Disabled;
	bReplicates = true;
	GetCharacterMovement()->bRunPhysicsWithNoController = true;   // sem controle, mas cai e é empurrado normalmente
}

void ANinjaTrainingDummy::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		GetAbilitySystemComponent()->SetNumericAttributeBase(UNinjaAttributeSet::GetMaxHealthAttribute(), DummyHealth);
		GetAbilitySystemComponent()->SetNumericAttributeBase(UNinjaAttributeSet::GetHealthAttribute(), DummyHealth);
		bGuarding = bAlwaysGuard;
	}
}

void ANinjaTrainingDummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Sem apanhar por um tempo: vida volta ao máximo (para treinar combos à vontade)
	TimeSinceHit += DeltaSeconds;
	if (HasAuthority() && !bKnockedOut && TimeSinceHit > RegenDelay && GetAttributes()->GetHealth() < GetAttributes()->GetMaxHealth())
	{
		RestoreHealth();
	}
}

void ANinjaTrainingDummy::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue < Data.OldValue)
	{
		TimeSinceHit = 0.f;
	}
	Super::OnHealthChanged(Data);
}

void ANinjaTrainingDummy::OnKnockedOut()
{
	Super::OnKnockedOut();
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &ANinjaTrainingDummy::RestoreHealth, RespawnDelay, false);
	}
}

void ANinjaTrainingDummy::RestoreHealth()
{
	GetAbilitySystemComponent()->SetNumericAttributeBase(UNinjaAttributeSet::GetHealthAttribute(), GetAttributes()->GetMaxHealth());
}
