// Explosão de fogo estilizada (anime), só visual, criada em cada máquina pelo projétil quando ele explode.
// Poucas formas 3D grandes com o material mestre M_Fogo_Estilizado (ruído rolando + máscara de formato + limiar com erosão):
// cúpula principal que cresce e sobe, coluna de chama, anel de choque rente ao chão (com uma distorção de calor pequena
// só nele), 3-5 chamas presas ao chão que tremulam e apagam, puffs de fumaça (M_Fumaca_Estilizada) que sobem e se
// desfazem, e brasas Niagara. Tudo é animado aqui por parâmetros do material (Erosion), sem sprites.

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

	/** Malhas (origem na base, altura 100 cm; a esfera tem centro na origem e raio 50) */
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UStaticMesh> FlameMesh;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UStaticMesh> ColumnMesh;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UStaticMesh> RingMesh;

	/** Material mestre de fogo (instâncias por forma) e de fumaça; distorção pequena só no anel */
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UMaterialInterface> DomeMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UMaterialInterface> ColumnMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UMaterialInterface> RingMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UMaterialInterface> GroundFlameMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UMaterialInterface> SmokeMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UMaterialInterface> HeatRingMaterial;

	/** Brasas (pequenas, caindo com gravidade) */
	UPROPERTY(EditDefaultsOnly, Category="Explosão") TObjectPtr<UNiagaraSystem> EmberSystem;

	/** Cúpula: raio final (x raio da bola), subida (cm), tempo de crescer e tempo até sumir (s) */
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Cúpula") float DomeScale = 2.1f;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Cúpula") float DomeRise = 90.f;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Cúpula") float DomeGrowTime = 0.35f;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Cúpula") float DomeLife = 0.95f;

	/** Coluna: altura final (cm), raio (cm), vida (s) */
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Coluna") float ColumnHeight = 520.f;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Coluna") float ColumnRadius = 120.f;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Coluna") float ColumnLife = 1.3f;

	/** Anel de choque: raio final (cm) e vida (s) */
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Anel") float RingRadius = 520.f;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Anel") float RingLife = 0.55f;

	/** Fogo no chão: quantas chamas, altura (cm) e vida (s, mín/máx) */
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Chão") int32 GroundFlames = 4;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Chão") FVector2D GroundFlameHeight = FVector2D(110.f, 190.f);
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Chão") FVector2D GroundFlameLife = FVector2D(2.f, 2.8f);
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Chão") float GroundFlameSpread = 110.f;

	/** Fumaça: quantos puffs, raio (cm), subida (cm/s), deriva com o vento (cm/s) e vida (s, mín/máx) */
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Fumaça") int32 SmokePuffs = 7;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Fumaça") FVector2D SmokeRadius = FVector2D(70.f, 120.f);
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Fumaça") float SmokeRise = 55.f;
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Fumaça") FVector SmokeWind = FVector(28.f, 21.f, 0.f);
	UPROPERTY(EditDefaultsOnly, Category="Explosão|Fumaça") FVector2D SmokeLife = FVector2D(2.2f, 2.9f);

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
		int32 Kind = 0;		// 0 cúpula, 1 coluna, 2 anel, 3 chama do chão, 4 fumaça, 5 calor do anel
	};

	FPiece& AddPiece(UStaticMesh* Mesh, UMaterialInterface* Material, int32 Kind, const FVector& Origin, float Delay, float Life);
	void UpdatePiece(FPiece& Piece, float Age);

	TArray<FPiece> Pieces;
	FVector Center = FVector::ZeroVector;
	FVector FloorPoint = FVector::ZeroVector;
	FVector FloorNormal = FVector::UpVector;
	float BallRadius = 150.f;
	bool bHasFloor = false;
	float Elapsed = 0.f;
};
