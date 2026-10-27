// Explosão de fogo estilizada (formas 3D + material mestre), só visual.

#include "NinjaFireExplosion.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraFunctionLibrary.h"

namespace
{
	float EaseOut(float X)
	{
		X = FMath::Clamp(X, 0.f, 1.f);
		return 1.f - FMath::Cube(1.f - X);
	}

	float Smooth(float A, float B, float X)
	{
		return FMath::SmoothStep(A, B, X);
	}
}

ANinjaFireExplosion::ANinjaFireExplosion()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;   // cada máquina cria a sua a partir da explosão replicada do projétil
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ANinjaFireExplosion::Init(const FVector& InCenter, float InBallRadius, bool bInHasFloor, const FVector& InFloorPoint, const FVector& InFloorNormal)
{
	Center = InCenter;
	BallRadius = FMath::Max(InBallRadius, 30.f);
	// Só conta como "no chão" se o chão estiver perto da bola; explodindo no alto, nada fica preso ao chão
	bHasFloor = bInHasFloor && FVector::Dist(InCenter, InFloorPoint) < BallRadius * 2.2f;
	FloorPoint = InFloorPoint;
	FloorNormal = InFloorNormal.GetSafeNormal();
}

ANinjaFireExplosion::FPiece& ANinjaFireExplosion::AddPiece(UStaticMesh* Mesh, UMaterialInterface* Material, int32 Kind, const FVector& Origin, float Delay, float Life)
{
	FPiece& Piece = Pieces.AddDefaulted_GetRef();
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this);
	Comp->SetStaticMesh(Mesh);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCastShadow(false);
	Comp->bReceivesDecals = false;
	Comp->SetCanEverAffectNavigation(false);
	Comp->SetupAttachment(RootComponent);
	Comp->SetUsingAbsoluteLocation(true);
	Comp->SetUsingAbsoluteRotation(true);
	Comp->SetUsingAbsoluteScale(true);
	Comp->RegisterComponent();
	Comp->SetVisibility(false);
	if (Material)
	{
		Piece.MID = UMaterialInstanceDynamic::Create(Material, this);
		Piece.MID->SetScalarParameterValue(TEXT("Seed"), FMath::FRand() * 10.f);
		Comp->SetMaterial(0, Piece.MID);
	}
	Piece.Mesh = Comp;
	Piece.Kind = Kind;
	Piece.Origin = Origin;
	Piece.Delay = Delay;
	Piece.Life = Life;
	Piece.Phase = FMath::FRand() * 10.f;
	return Piece;
}

void ANinjaFireExplosion::BeginPlay()
{
	Super::BeginPlay();
	const FVector Base = bHasFloor ? FloorPoint : Center - FVector(0.f, 0.f, BallRadius * 0.6f);

	FPiece& Dome = AddPiece(SphereMesh, DomeMaterial, 0, bHasFloor ? FloorPoint + FVector(0, 0, BallRadius * 0.35f) : Center, 0.f, DomeLife);
	Dome.Size = BallRadius;
	FPiece& Column = AddPiece(ColumnMesh, ColumnMaterial, 1, Base, 0.06f, ColumnLife);
	Column.Size = ColumnRadius;
	Column.Height = ColumnHeight;
	if (bHasFloor)
	{
		AddPiece(RingMesh, RingMaterial, 2, FloorPoint + FloorNormal * 6.f, 0.02f, RingLife).Size = RingRadius;
		if (HeatRingMaterial)
		{
			AddPiece(RingMesh, HeatRingMaterial, 5, FloorPoint + FloorNormal * 10.f, 0.02f, RingLife).Size = RingRadius * 1.08f;
		}
		// Fogo residual agrupado no ponto do impacto, preso ao chão
		for (int32 i = 0; i < GroundFlames; ++i)
		{
			const float Angle = (i + FMath::FRandRange(-0.25f, 0.25f)) * 2.f * PI / FMath::Max(GroundFlames, 1);
			const float Dist = i == 0 ? 0.f : FMath::FRandRange(0.4f, 1.f) * GroundFlameSpread;
			const FVector Offset(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
			FPiece& Flame = AddPiece(FlameMesh, GroundFlameMaterial, 3, FloorPoint + FVector::VectorPlaneProject(Offset, FloorNormal) - FloorNormal * 4.f,
				0.12f + 0.05f * i, FMath::FRandRange(GroundFlameLife.X, GroundFlameLife.Y));
			Flame.Height = FMath::FRandRange(GroundFlameHeight.X, GroundFlameHeight.Y) * (i == 0 ? 1.15f : 1.f);
			Flame.Size = Flame.Height * 0.42f;
		}
	}
	// Fumaça: puffs que nascem na nuvem, sobem devagar, vão com o vento e se desfazem
	for (int32 i = 0; i < SmokePuffs; ++i)
	{
		const FVector Offset = FMath::VRand() * FVector(1.f, 1.f, 0.5f) * BallRadius * 0.7f + FVector(0.f, 0.f, BallRadius * 0.6f);
		FPiece& Puff = AddPiece(SphereMesh, SmokeMaterial, 4, Center + Offset, 0.18f + FMath::FRand() * 0.3f, FMath::FRandRange(SmokeLife.X, SmokeLife.Y));
		Puff.Size = FMath::FRandRange(SmokeRadius.X, SmokeRadius.Y);
		const FVector Spread = FVector(Offset.X, Offset.Y, 0.f).GetSafeNormal() * FMath::FRandRange(15.f, 40.f);
		Puff.Velocity = SmokeWind + Spread + FVector(0.f, 0.f, SmokeRise * FMath::FRandRange(0.8f, 1.25f));
	}

	if (EmberSystem && GetNetMode() != NM_DedicatedServer)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, EmberSystem, Center, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::AutoRelease);
	}
	float End = 0.f;
	for (const FPiece& Piece : Pieces)
	{
		End = FMath::Max(End, Piece.Delay + Piece.Life);
	}
	SetLifeSpan(End + 0.3f);
}

void ANinjaFireExplosion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	for (FPiece& Piece : Pieces)
	{
		UpdatePiece(Piece, Elapsed - Piece.Delay);
	}
}

void ANinjaFireExplosion::UpdatePiece(FPiece& Piece, float Age)
{
	UStaticMeshComponent* Mesh = Piece.Mesh;
	if (!Mesh)
	{
		return;
	}
	const bool bAlive = Age >= 0.f && Age <= Piece.Life;
	Mesh->SetVisibility(bAlive);
	if (!bAlive)
	{
		return;
	}
	const float L = Piece.Life;
	const float X = Age / L;
	float Erosion = 0.f;
	FVector Location = Piece.Origin;
	FVector Scale(1.f);
	FRotator Rotation = FRotator::ZeroRotator;
	switch (Piece.Kind)
	{
	case 0:		// cúpula: cresce rápido, sobe e se desfaz de cima para baixo
	{
		const float R = Piece.Size * FMath::Lerp(0.55f, DomeScale, EaseOut(Age / FMath::Max(DomeGrowTime, 0.01f)));
		Scale = FVector(R / 50.f, R / 50.f, R / 50.f * 0.85f);
		Location += FVector(0.f, 0.f, DomeRise * EaseOut(X));
		Rotation = FRotator(0.f, Age * 40.f, 0.f);
		Erosion = Smooth(0.25f, 1.f, X);
		break;
	}
	case 1:		// coluna: sobe até a altura cheia e se desfaz
	{
		const float H = Piece.Height * EaseOut(Age / (0.55f * L));
		const float R = Piece.Size * FMath::Lerp(1.f, 0.6f, X);
		Scale = FVector(R / 50.f, R / 50.f, FMath::Max(H, 1.f) / 100.f);
		Rotation = FRotator(0.f, -Age * 60.f, 0.f);
		Erosion = Smooth(0.2f, 1.f, X);
		break;
	}
	case 2:		// anel de choque rente ao chão
	case 5:		// distorção de calor só no anel
	{
		const float R = Piece.Size * FMath::Lerp(0.3f, 1.f, EaseOut(X));
		Scale = FVector(R / 50.f, R / 50.f, 0.7f);
		Rotation = FRotationMatrix::MakeFromZ(FloorNormal).Rotator();
		Erosion = Smooth(0.3f, 1.f, X);
		break;
	}
	case 3:		// chama no chão: acende, tremula e diminui até apagar
	{
		const float In = EaseOut(Age / 0.18f);
		const float Out = FMath::Lerp(1.f, 0.35f, Smooth(0.45f, 1.f, X));
		const float FlickZ = 1.f + 0.12f * FMath::Sin(Age * 14.f + Piece.Phase) + 0.06f * FMath::Sin(Age * 23.f + Piece.Phase * 2.f);
		const float FlickXY = 1.f + 0.07f * FMath::Sin(Age * 11.f + Piece.Phase * 3.f);
		Scale = FVector(Piece.Size / 40.f * FlickXY, Piece.Size / 40.f * FlickXY, Piece.Height / 100.f * FlickZ) * In * Out;
		Rotation = FRotationMatrix::MakeFromZ(FloorNormal).Rotator() + FRotator(0.f, Piece.Phase * 36.f + Age * 25.f, 0.f);
		Erosion = Smooth(0.5f, 1.f, X) * 0.9f;
		break;
	}
	default:	// fumaça: sobe, vai com o vento, cresce e se desfaz
	{
		Location += Piece.Velocity * Age;
		const float R = Piece.Size * FMath::Lerp(0.45f, 1.5f, EaseOut(X * 1.3f));
		Scale = FVector(R / 50.f);
		Rotation = FRotator(0.f, Piece.Phase * 36.f + Age * 15.f, 0.f);
		Erosion = Smooth(0.3f, 1.f, X);
		break;
	}
	}
	Mesh->SetWorldLocationAndRotation(Location, Rotation);
	Mesh->SetWorldScale3D(Scale);
	if (Piece.MID)
	{
		Piece.MID->SetScalarParameterValue(TEXT("Erosion"), Erosion);
		Piece.MID->SetScalarParameterValue(TEXT("Age"), X);
	}
}
