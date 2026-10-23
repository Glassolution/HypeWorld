// Aura de chakra (estilo Shinobi Striker): brilho no corpo, chamas subindo, anéis no chão, faíscas e luz.
// Montada em tempo de execução com malhas simples + materiais; liga/desliga suave com SetAuraActive.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "NinjaChakraAuraComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UTexture2D;

UCLASS(ClassGroup=(Ninja), meta=(BlueprintSpawnableComponent))
class HYPEWORLD_API UNinjaChakraAuraComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	UNinjaChakraAuraComponent();

	/** Liga/desliga a aura (com transição suave). */
	void SetAuraActive(bool bActive);

	UPROPERTY(EditAnywhere, Category="Aura|Materiais") UMaterialInterface* ShellMaterial = nullptr;
	UPROPERTY(EditAnywhere, Category="Aura|Materiais") UMaterialInterface* FlameMaterial = nullptr;
	UPROPERTY(EditAnywhere, Category="Aura|Materiais") UMaterialInterface* RingMaterial = nullptr;
	UPROPERTY(EditAnywhere, Category="Aura|Materiais") UMaterialInterface* SparkMaterial = nullptr;

	/** Cor do chakra. */
	UPROPERTY(EditAnywhere, Category="Aura") FLinearColor ChakraColor = FLinearColor(0.15f, 0.55f, 1.f);

	/** Rapidez para acender/apagar. */
	UPROPERTY(EditAnywhere, Category="Aura") float FadeSpeed = 5.f;

	/** Quantas faíscas sobem ao mesmo tempo. */
	UPROPERTY(EditAnywhere, Category="Aura") int32 SparkCount = 28;

	/** Brilho máximo da luz azul. */
	UPROPERTY(EditAnywhere, Category="Aura") float LightIntensity = 6000.f;

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	void BuildAura();
	UMaterialInstanceDynamic* MakeMID(UMaterialInterface* Base);
	void SetPartsVisible(bool bVisible);

	struct FSpark
	{
		float Angle = 0.f;
		float Radius = 0.f;
		float Height = 0.f;
		float Speed = 0.f;
		float Size = 1.f;
	};
	TArray<FSpark> Sparks;

	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Shell;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> FlameInner;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> FlameOuter;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> GroundRing;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> SparkMesh;
	UPROPERTY() TObjectPtr<UPointLightComponent> Light;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> MIDs;
	UPROPERTY() TObjectPtr<UTexture2D> NoiseTexture;

	bool bWantActive = false;
	float Alpha = 0.f;
	float Time = 0.f;
};
