// Chamas em órbita (Katon: Hōsenka): o jutsu invoca N chamas que giram em volta do personagem; cada clique lança uma.
// Replicado: o servidor cria, guarda quais chamas ainda existem e lança os projéteis (dano no servidor). As posições
// saem do mesmo relógio (hora de início no servidor) em todas as máquinas, então ninguém replica movimento.
// Visual por chama: o modelo FireBall (material de fogo vivo), núcleo, halo macio, uma língua que se solta e brasas.
// Uma luz só para o grupo (com o clarão da invocação).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NinjaFlameOrbit.generated.h"

class UNinjaJutsuData;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraSystem;
class UNiagaraComponent;
class UPointLightComponent;
class AHypeWorldCharacter;

UCLASS(Blueprintable)
class HYPEWORLD_API ANinjaFlameOrbit : public AActor
{
	GENERATED_BODY()

public:

	ANinjaFlameOrbit();

	/** Servidor, antes de FinishSpawning */
	void Init(const UNinjaJutsuData* InJutsu);

	/** Servidor: lança a chama mais à frente na direção de AimPoint. Falso se não sobrou chama ou ainda no intervalo */
	bool FireFlame(const FVector& AimPoint);

	int32 GetFlamesLeft() const;
	int32 GetFlameCount() const;
	/** Tempo que falta (s) e duração total */
	float GetTimeLeft(float& OutDuration) const;
	bool IsActive() const { return !bEnded && GetFlamesLeft() > 0; }
	/** Alcance do disparo (cm), do Data Asset ou do projétil */
	float GetJutsuRange() const;
	const UNinjaJutsuData* GetJutsu() const { return Jutsu; }

	// ---------- Visual ----------
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") TObjectPtr<UStaticMesh> FlameMesh;
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") TObjectPtr<UMaterialInterface> FlameMaterial;
	/** Esfera (raio 50) do núcleo e do halo */
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") TObjectPtr<UStaticMesh> GlowMesh;
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") TObjectPtr<UMaterialInterface> CoreMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") TObjectPtr<UMaterialInterface> HaloMaterial;
	/** Língua que se solta (base na origem, altura 100, raio 40) */
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") TObjectPtr<UStaticMesh> TongueMesh;
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") TObjectPtr<UMaterialInterface> TongueMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") TObjectPtr<UNiagaraSystem> EmberSystem;
	/** Raio visível de cada chama (cm), quanto ela estica/inclina no giro e a pulsação */
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") float FlameSize = 22.f;
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") float SwirlLean = 0.9f;
	/** Alturas da órbita em relação ao centro do personagem (cm, mín/máx) e oscilação vertical (cm) */
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") FVector2D HeightRange = FVector2D(-15.f, 115.f);
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") float Bob = 15.f;
	/** Invocação: as chamas acendem uma depois da outra (s) com um clarão */
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") float IgniteStagger = 0.04f;
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") FLinearColor LightColor = FLinearColor(1.f, 0.42f, 0.1f);
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") float LightIntensity = 35.f;
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") float LightRadius = 700.f;
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") float LightFlash = 4.f;
	/** Fim: as chamas restantes se desfazem neste tempo (s) */
	UPROPERTY(EditDefaultsOnly, Category="Órbita|Visual") float FadeTime = 0.45f;

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated) TObjectPtr<const UNinjaJutsuData> Jutsu;
	UPROPERTY(Replicated) double StartTime = 0.0;
	/** Bit i = chama i ainda em órbita */
	UPROPERTY(Replicated) int32 AliveMask = 0;
	UPROPERTY(ReplicatedUsing=OnRep_Ended) bool bEnded = false;
	UFUNCTION() void OnRep_Ended();

	UPROPERTY(VisibleAnywhere, Category="Órbita") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category="Órbita") TObjectPtr<UPointLightComponent> Light;

private:

	struct FFlame
	{
		TObjectPtr<UStaticMeshComponent> Body;
		TObjectPtr<UStaticMeshComponent> Core;
		TObjectPtr<UStaticMeshComponent> Halo;
		TObjectPtr<UStaticMeshComponent> Tongue;
		TObjectPtr<UNiagaraComponent> Embers;
		TArray<TObjectPtr<UMaterialInstanceDynamic>> MIDs;
		FVector TonguePos = FVector::ZeroVector;
		FVector TongueVel = FVector::ZeroVector;
		float TongueAge = 10.f;
		float TongueLife = 0.3f;
		bool bShown = true;
	};

	/** Centro da órbita, posição e direção do giro da chama i no instante T (s desde o início) */
	FVector GetCenter() const;
	void EvaluateFlame(int32 Index, float T, const FVector& Center, FVector& OutPos, FVector& OutTangent) const;
	float GetElapsed() const;
	double Now() const;
	void BuildVisuals();
	void EndOrbit();
	AHypeWorldCharacter* GetCaster() const;

	TArray<FFlame> Flames;
	double LastFireTime = -100.0;
	double LocalEndTime = 0.0;
	bool bVisualsBuilt = false;
};
