// Boneco de treino: apanha, mostra o dano, cai quando a vida zera e levanta de novo com a vida cheia.

#pragma once

#include "CoreMinimal.h"
#include "HypeWorldCharacter.h"
#include "NinjaTrainingDummy.generated.h"

UCLASS()
class HYPEWORLD_API ANinjaTrainingDummy : public AHypeWorldCharacter
{
	GENERATED_BODY()

public:

	ANinjaTrainingDummy(const FObjectInitializer& ObjectInitializer);

	/** Vida do boneco */
	UPROPERTY(EditAnywhere, Category="Ninja|Boneco")
	float DummyHealth = 1000.f;

	/** Segundos sem apanhar para a vida voltar ao máximo */
	UPROPERTY(EditAnywhere, Category="Ninja|Boneco")
	float RegenDelay = 4.f;

	/** Segundos caído antes de levantar */
	UPROPERTY(EditAnywhere, Category="Ninja|Boneco")
	float RespawnDelay = 2.5f;

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnHealthChanged(const FOnAttributeChangeData& Data) override;
	virtual void OnKnockedOut() override;

private:

	void RestoreHealth();

	float TimeSinceHit = 0.f;
	FTimerHandle RespawnTimer;
};
