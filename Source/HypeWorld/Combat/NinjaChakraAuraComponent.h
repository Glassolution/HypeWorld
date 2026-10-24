// Aura de chakra estilo anime (referências do Felipe):
//  - carregando: chama azul "desenhada" (preenchimento azul translúcido + contorno branco ondulado, com buracos e labaredas)
//  - chakra no máximo: explosão com coroa de espinhos, rastros girando, raios no chão e faíscas
// Feita com planos virados para a câmera + materiais (o desenho vem todo do material); liga/desliga suave.

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
class UAudioComponent;
class USoundBase;
class UTexture2D;

UCLASS(ClassGroup=(Ninja), meta=(BlueprintSpawnableComponent))
class HYPEWORLD_API UNinjaChakraAuraComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	UNinjaChakraAuraComponent();

	/** Liga/desliga a aura (com transição suave). */
	void SetAuraActive(bool bActive);

	/** Chakra no máximo: troca para a aura de explosão. */
	void SetAuraFull(bool bFull);

	UPROPERTY(EditAnywhere, Category="Aura|Materiais") UMaterialInterface* ShellMaterial = nullptr;
	UPROPERTY(EditAnywhere, Category="Aura|Materiais") UMaterialInterface* FlameMaterial = nullptr;		// chama desenhada (carregando)
	UPROPERTY(EditAnywhere, Category="Aura|Materiais") UMaterialInterface* BurstMaterial = nullptr;		// coroa de espinhos (máximo)
	UPROPERTY(EditAnywhere, Category="Aura|Materiais") UMaterialInterface* GroundMaterial = nullptr;	// brilho azul no chão
	UPROPERTY(EditAnywhere, Category="Aura|Materiais") UMaterialInterface* BoltMaterial = nullptr;		// raios no chão (máximo)
	UPROPERTY(EditAnywhere, Category="Aura|Materiais") UMaterialInterface* SparkMaterial = nullptr;

	/** Sons (opcionais): começar a carregar, laço enquanto carrega, estouro ao encher, laço no máximo. */
	UPROPERTY(EditAnywhere, Category="Aura|Som") USoundBase* ChargeStartSound = nullptr;
	UPROPERTY(EditAnywhere, Category="Aura|Som") USoundBase* ChargeLoopSound = nullptr;
	UPROPERTY(EditAnywhere, Category="Aura|Som") USoundBase* FullBurstSound = nullptr;
	UPROPERTY(EditAnywhere, Category="Aura|Som") USoundBase* FullLoopSound = nullptr;

	/** Cor do chakra. */
	UPROPERTY(EditAnywhere, Category="Aura") FLinearColor ChakraColor = FLinearColor(0.15f, 0.55f, 1.f);

	/** Tamanho da chama (cm): largura e altura. */
	UPROPERTY(EditAnywhere, Category="Aura") FVector2D FlameSize = FVector2D(330.f, 520.f);

	/** Tamanho da explosão no máximo (cm): largura e altura. */
	UPROPERTY(EditAnywhere, Category="Aura") FVector2D BurstSize = FVector2D(620.f, 560.f);

	/** Rapidez para acender/apagar. */
	UPROPERTY(EditAnywhere, Category="Aura") float FadeSpeed = 5.f;

	/** Quantas faíscas sobem ao mesmo tempo. */
	UPROPERTY(EditAnywhere, Category="Aura") int32 SparkCount = 40;

	/** Brilho máximo da luz azul. */
	UPROPERTY(EditAnywhere, Category="Aura") float LightIntensity = 6000.f;

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	void BuildAura();
	UMaterialInstanceDynamic* MakeMID(UMaterialInterface* Base);
	void SetPartsVisible(bool bVisible);
	void FaceCamera(UStaticMeshComponent* Plane, const FVector2D& Size, float BottomZ, float BackOffset) const;
	void PlayLoop(USoundBase* Sound);

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
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Flame;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Burst;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Ground;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Bolts;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> SparkMesh;
	UPROPERTY() TObjectPtr<UPointLightComponent> Light;
	UPROPERTY() TObjectPtr<UAudioComponent> LoopAudio;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> MIDs;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlameMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BurstMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BoltMID;
	UPROPERTY() TObjectPtr<UTexture2D> NoiseTexture;

	bool bWantActive = false;
	bool bWantFull = false;
	float Alpha = 0.f;		// aura ligada (0..1)
	float FullAlpha = 0.f;	// explosão do máximo (0..1)
	float FullTime = 0.f;	// tempo desde que encheu (estouro de entrada)
	float Time = 0.f;
};
