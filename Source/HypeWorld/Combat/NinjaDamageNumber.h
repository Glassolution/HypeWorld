// Número de dano que salta e sobe na tela (só visual, criado em cada máquina).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "NinjaDamageNumber.generated.h"

class UTextBlock;
class UWidgetComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class HYPEWORLD_API UNinjaDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void SetNumber(const FString& Text, const FLinearColor& Color, float FontSize);

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

private:

	UPROPERTY() TObjectPtr<UTextBlock> Label;
};

UCLASS()
class HYPEWORLD_API ANinjaDamageNumber : public AActor
{
	GENERATED_BODY()

public:

	ANinjaDamageNumber();

	/** Cria o número no mundo (local, sem replicar). */
	static void Spawn(UWorld* World, const FVector& Location, const FString& Text, const FLinearColor& Color, float FontSize = 34.f);

	virtual void Tick(float DeltaSeconds) override;

private:

	UPROPERTY() TObjectPtr<UWidgetComponent> Widget;

	FVector Drift = FVector::ZeroVector;
	float Age = 0.f;
	float Lifetime = 0.9f;
};

/** Flash rápido no ponto do golpe (esfera que cresce e some). */
UCLASS()
class HYPEWORLD_API ANinjaHitFlash : public AActor
{
	GENERATED_BODY()

public:

	ANinjaHitFlash();

	static void Spawn(UWorld* World, const FVector& Location, UMaterialInterface* Material, const FLinearColor& Color);

	virtual void Tick(float DeltaSeconds) override;

private:

	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MID;
	float Age = 0.f;
};
