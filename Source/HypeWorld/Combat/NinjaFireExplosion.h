// Explosão de fogo estilizada (anime), só visual, criada em cada máquina pelo projétil quando ele explode.
// Camadas em formas 3D simples com o material mestre M_Fogo_Estilizado (macio e aditivo, some pela borda):
// núcleo amarelo-laranja que estoura rápido e encolhe, halo vermelho grande e translúcido, riscos finos de borda dura
// saindo em raios, puffs de fumaça marrom-escura que sobem, puffs vermelhos que encolhem e somem na dissipação,
// distorção de calor pequena em volta e faíscas/brasas Niagara. Ritmo: entrada quase instantânea, saída lenta.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NinjaFireExplosion.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraSystem;

UCLASS(Blueprintable)
class HYPEWORLD_API ANinjaFireExplosion : public AActor
{
	GENERATED_BODY()

public:

	ANinjaFireExplosion();

	/** Antes de BeginPlay (SpawnActorDeferred): onde explodiu, raio da bola e o chão embaixo (se houver) */
	void Init(const FVector& InCenter, float InBallRadius, bool bInHasFloor, const FVector& InFloorPoint, const FVector& InFloorNormal);

	/** Malhas (a esfera tem centro na origem e raio 50; a chama tem base na origem e altura 100) */
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UStaticMesh> FlameMesh;
	/** Malha do clarão do núcleo (vazia = SphereMesh); uma chama irregular evita o contorno redondo no impacto */
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UStaticMesh> DomeMesh;

	/** Núcleo (aditivo), halo vermelho, riscos (borda dura), fumaça escura, puffs vermelhos e distorção de calor */
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UMaterialInterface> DomeMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UMaterialInterface> HaloMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UMaterialInterface> StreakMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UMaterialInterface> SmokeMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UMaterialInterface> EmberPuffMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UMaterialInterface> HeatMaterial;

	/** Faíscas finas com gravidade (rajada) e brasas flutuando que se apagam (dissipação) */
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UNiagaraSystem> EmberSystem;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UNiagaraSystem> DriftEmberSystem;

	/** Núcleo: raio máximo (x raio da bola), tempo até o máximo e vida (s) */
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Núcleo") float DomeScale = 1.35f;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Núcleo") float DomeGrowTime = 0.08f;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Núcleo") float DomeLife = 0.5f;
	/** Halo: raio (x raio da bola) e vida (s) */
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Núcleo") float HaloScale = 2.6f;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Núcleo") float HaloLife = 0.6f;

	/** Riscos: quantos, comprimento (cm), velocidade (cm/s) e vida (s, mín/máx) */
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Riscos") int32 Streaks = 14;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Riscos") FVector2D StreakLength = FVector2D(90.f, 170.f);
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Riscos") float StreakSpeed = 1300.f;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Riscos") FVector2D StreakLife = FVector2D(0.16f, 0.28f);

	/** Fumaça marrom: quantos puffs, raio (cm), subida (cm/s), vento (cm/s) e vida (s, mín/máx) */
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Fumaça") int32 SmokePuffs = 6;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Fumaça") FVector2D SmokeRadius = FVector2D(55.f, 90.f);
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Fumaça") float SmokeRise = 70.f;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Fumaça") FVector SmokeWind = FVector(20.f, 15.f, 0.f);
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Fumaça") FVector2D SmokeLife = FVector2D(0.9f, 1.4f);

	/** Puffs vermelhos da dissipação: quantos, raio (cm) e vida (s, mín/máx) */
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Dissipação") int32 EmberPuffs = 5;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Dissipação") FVector2D EmberPuffRadius = FVector2D(45.f, 75.f);
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Dissipação") FVector2D EmberPuffLife = FVector2D(0.8f, 1.3f);

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:

	struct FPiece
	{
		TObjectPtr<UStaticMeshComponent> Mesh;
		TObjectPtr<UMaterialInstanceDynamic> MID;
		FVector Origin = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Delay = 0.f;
		float Life = 1.f;
		float Size = 1.f;
		float Height = 1.f;
		float Phase = 0.f;
		int32 Kind = 0;		// 0 núcleo, 1 halo, 2 risco, 3 fumaça, 4 puff vermelho, 5 calor
	};

	FPiece* AddPiece(UStaticMesh* Mesh, UMaterialInterface* Material, int32 Kind, const FVector& Origin, float Delay, float Life);
	void UpdatePiece(FPiece& Piece, float Age);

	TArray<FPiece> Pieces;
	FVector Center = FVector::ZeroVector;
	FVector FloorPoint = FVector::ZeroVector;
	FVector FloorNormal = FVector::UpVector;
	float BallRadius = 150.f;
	bool bHasFloor = false;
	float Elapsed = 0.f;
};
