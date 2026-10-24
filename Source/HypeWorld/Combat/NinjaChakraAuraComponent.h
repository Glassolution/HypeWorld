// Aura de chakra estilo anime (especificação do Felipe): fogo de energia cel-shaded em volta do corpo — línguas que sobem,
// contorno branco-ciano nítido, miolo azul translúcido, buracos e gotas soltas. Duas camadas viradas para a câmera: uma
// atrás do personagem e outra na frente (só contornos, até a cintura), para ele parecer "dentro" da aura.
// Liga crescendo de baixo para cima (Grow) e desliga se partindo em pedaços (Dissolve). A mesma aura carregando e no máximo.
// Feita com planos + material (o Python não monta Niagara de forma confiável).

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

	/** Chakra no máximo: o visual não muda (só troca o som, se houver). */
	void SetAuraFull(bool bFull);

	/** Quanto já carregou (0..1): línguas mais altas e mais brilho */
	void SetChargeLevel(float Level) { ChargeLevel = FMath::Clamp(Level, 0.f, 1.f); }

	UPROPERTY(EditAnywhere, Category="Aura|Materiais") UMaterialInterface* ShellMaterial = nullptr;
	UPROPERTY(EditAnywhere, Category="Aura|Materiais") UMaterialInterface* FlameMaterial = nullptr;		// chama desenhada
	UPROPERTY(EditAnywhere, Category="Aura|Materiais") UMaterialInterface* GroundMaterial = nullptr;	// brilho no chão
	UPROPERTY(EditAnywhere, Category="Aura|Materiais") UMaterialInterface* SparkMaterial = nullptr;

	/** Sons (opcionais): começar a carregar, laço enquanto carrega, estouro ao encher, laço no máximo. */
	UPROPERTY(EditAnywhere, Category="Aura|Som") USoundBase* ChargeStartSound = nullptr;
	UPROPERTY(EditAnywhere, Category="Aura|Som") USoundBase* ChargeLoopSound = nullptr;
	UPROPERTY(EditAnywhere, Category="Aura|Som") USoundBase* FullBurstSound = nullptr;
	UPROPERTY(EditAnywhere, Category="Aura|Som") USoundBase* FullLoopSound = nullptr;

	/** Cor do chakra. */
	UPROPERTY(EditAnywhere, Category="Aura") FLinearColor ChakraColor = FLinearColor(0.15f, 0.55f, 1.f);

	/** Tamanho da chama (cm): largura e altura. */
	UPROPERTY(EditAnywhere, Category="Aura") FVector2D FlameSize = FVector2D(330.f, 315.f);

	/** Quanto a chama fica atrás do personagem, olhando da câmera (cm). Pouco = parece envolver o corpo. */
	UPROPERTY(EditAnywhere, Category="Aura") float FlameBackOffset = 20.f;

	/** Onde começa a base da chama em relação aos pés (cm; negativo = abaixo do chão). */
	UPROPERTY(EditAnywhere, Category="Aura") float FlameBottomOffset = -10.f;

	/** Até onde a base da chama chega na frente do corpo (cm, no chão): cobre o pé que fica mais perto da câmera. */
	UPROPERTY(EditAnywhere, Category="Aura") float FlameFrontReach = 50.f;

	/** Camada da frente (pouco azul, só em volta das pernas): tamanho relativo e quanto fica à frente do corpo (cm) */
	UPROPERTY(EditAnywhere, Category="Aura") float FrontLayerScale = 0.85f;
	UPROPERTY(EditAnywhere, Category="Aura") float FrontLayerOffset = 35.f;

	/** Pulsação do tamanho (fração e vezes por segundo) */
	UPROPERTY(EditAnywhere, Category="Aura") float PulseAmount = 0.05f;
	UPROPERTY(EditAnywhere, Category="Aura") float PulseRate = 2.f;

	/** Tempo para a aura crescer ao ligar e para se desfazer ao desligar (s). */
	UPROPERTY(EditAnywhere, Category="Aura") float GrowTime = 0.2f;
	UPROPERTY(EditAnywhere, Category="Aura") float DissolveTime = 0.25f;

	/** Quantas faíscas (bolinhas) sobem ao mesmo tempo (0 = nenhuma). */
	UPROPERTY(EditAnywhere, Category="Aura") int32 SparkCount = 0;

	/** Brilho máximo da luz azul. */
	UPROPERTY(EditAnywhere, Category="Aura") float LightIntensity = 6000.f;
	UPROPERTY(EditAnywhere, Category="Aura") FLinearColor LightColor = FLinearColor(0.05f, 0.4f, 1.f);   // #3FA9FF

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	void BuildAura();
	UMaterialInstanceDynamic* MakeMID(UMaterialInterface* Base);
	void SetPartsVisible(bool bVisible);
	void FaceCamera(UStaticMeshComponent* Plane, const FVector2D& Size, float BottomZ, float BackOffset, float FrontReach) const;
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
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Ground;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> SparkMesh;
	UPROPERTY() TObjectPtr<UPointLightComponent> Light;
	UPROPERTY() TObjectPtr<UAudioComponent> LoopAudio;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> MIDs;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlameMID;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> FrontFlame;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FrontFlameMID;
	UPROPERTY() TObjectPtr<UTexture2D> NoiseTexture;

	bool bWantActive = false;
	bool bWantFull = false;
	float Alpha = 0.f;		// aura ligada (0..1)
	float ChargeLevel = 1.f;
	float Time = 0.f;
};
