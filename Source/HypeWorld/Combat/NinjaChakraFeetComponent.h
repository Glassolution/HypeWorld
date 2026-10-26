// Infusão de Chakra nos pés (2ª versão, Felipe 24/10/2026): como no anime, uma CAMADA de energia azul colada em cada pé,
// seguindo o formato dele — não é chama. Cópia da malha do corpo (mesma pose) com M_ChakraFeetShell: inflada ~3 cm só
// na região dos pés (máscara por distância até cada pé, posições passadas todo quadro), borda brilhante (Fresnel), forte na
// sola e sumindo até o tornozelo, correntes subindo continuamente. Fiapos suaves soltando do topo, brilho estável na
// superfície embaixo (chão, parede, água) e luz azul fixa. Nada pulsa: o movimento é só o fluxo contínuo do material.
// Liga subindo da sola ao tornozelo em 0,3 s e desliga recolhendo para a sola em 0,3 s.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "NinjaChakraFeetComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UPointLightComponent;

UCLASS(ClassGroup=(Ninja), meta=(BlueprintSpawnableComponent))
class HYPEWORLD_API UNinjaChakraFeetComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	UNinjaChakraFeetComponent();

	/** Liga/desliga (sobe da sola / recolhe para a sola) */
	void SetFeetActive(bool bActive);

	/** 0..1: brilho e quantidade de fiapos (mais forte na parede/correndo) — muda devagar, sem saltos */
	void SetIntensity(float InIntensity) { TargetIntensity = FMath::Clamp(InIntensity, 0.f, 1.f); }

	UPROPERTY(EditAnywhere, Category="Chakra nos pés|Materiais") TObjectPtr<UMaterialInterface> ShellMaterial;
	UPROPERTY(EditAnywhere, Category="Chakra nos pés|Materiais") TObjectPtr<UMaterialInterface> WispMaterial;
	UPROPERTY(EditAnywhere, Category="Chakra nos pés|Materiais") TObjectPtr<UMaterialInterface> GroundMaterial;

	/** Casca: raio em volta do centro da sola (cm), até onde sobe (tornozelo, cm) e quanto fica afastada do pé (cm) */
	/** Ossos do tornozelo e do peito do pé no corpo visível (outro personagem com retarget tem outros nomes) */
	UPROPERTY(EditAnywhere, Category="Chakra nos pés|Ossos") FName FootBoneL = TEXT("foot_l");
	UPROPERTY(EditAnywhere, Category="Chakra nos pés|Ossos") FName BallBoneL = TEXT("ball_l");
	UPROPERTY(EditAnywhere, Category="Chakra nos pés|Ossos") FName FootBoneR = TEXT("foot_r");
	UPROPERTY(EditAnywhere, Category="Chakra nos pés|Ossos") FName BallBoneR = TEXT("ball_r");

	UPROPERTY(EditAnywhere, Category="Chakra nos pés") float FootRadius = 20.f;
	UPROPERTY(EditAnywhere, Category="Chakra nos pés") float AnkleHeight = 16.f;
	UPROPERTY(EditAnywhere, Category="Chakra nos pés") float Inflate = 3.f;
	/** Subir ao ligar / recolher ao desligar (s) */
	UPROPERTY(EditAnywhere, Category="Chakra nos pés") float RevealTime = 0.3f;

	/** Fiapos: por segundo por pé (Intensity 1), vida (s), velocidade para cima (cm/s), tamanho (largura x altura, cm) */
	UPROPERTY(EditAnywhere, Category="Chakra nos pés") float WispRate = 4.f;
	UPROPERTY(EditAnywhere, Category="Chakra nos pés") FVector2D WispLife = FVector2D(0.8f, 1.2f);
	UPROPERTY(EditAnywhere, Category="Chakra nos pés") FVector2D WispSpeed = FVector2D(15.f, 30.f);
	UPROPERTY(EditAnywhere, Category="Chakra nos pés") FVector2D WispSize = FVector2D(4.f, 12.f);

	/** Brilho na superfície (diâmetro, cm); aparece com o pé a menos de GroundGlowDistance dela */
	UPROPERTY(EditAnywhere, Category="Chakra nos pés") float GroundGlowSize = 70.f;
	UPROPERTY(EditAnywhere, Category="Chakra nos pés") float GroundGlowDistance = 45.f;

	/** Luz em cada pé (intensidade fixa) */
	UPROPERTY(EditAnywhere, Category="Chakra nos pés") float LightIntensity = 400.f;
	UPROPERTY(EditAnywhere, Category="Chakra nos pés") float LightRadius = 150.f;

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	struct FPart
	{
		UStaticMeshComponent* Mesh = nullptr;
		UMaterialInstanceDynamic* MID = nullptr;
	};
	struct FWisp
	{
		FPart Part;
		float Age = -1.f;
		float Life = 1.f;
		float Speed = 20.f;
		float Phase = 0.f;
		FVector Start = FVector::ZeroVector;
		FVector Side = FVector::ZeroVector;
	};
	struct FFoot
	{
		FName Bone;
		FName Ball;
		FPart Ground;
		UPointLightComponent* Light = nullptr;
		TArray<FWisp> Wisps;
		float WispCarry = 0.f;
		FVector SoleCenter = FVector::ZeroVector;
	};

	void Build();
	FPart MakePart(UMaterialInterface* Material);
	void UpdateFoot(FFoot& Foot, float DeltaTime, float Shown, const FVector& Up, const FVector& CamLoc);
	bool TraceSurface(const FVector& From, const FVector& Down, FHitResult& OutHit) const;

	TArray<FFoot> Feet;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Shell;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;   // corpo visível (de onde vêm a malha e os pés)
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ShellMID;
	UPROPERTY() TArray<TObjectPtr<UObject>> Keep;   // segura os componentes e MIDs criados em tempo de jogo

	bool bBuilt = false;
	bool bWantActive = false;
	float Alpha = 0.f;
	float Intensity = 0.6f;
	float TargetIntensity = 0.6f;
};
