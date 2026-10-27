// Explosão de fogo estilizada (camadas macias + detalhes de borda dura), só visual.

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

ANinjaFireExplosion::FPiece* ANinjaFireExplosion::AddPiece(UStaticMesh* Mesh, UMaterialInterface* Material, int32 Kind, const FVector& Origin, float Delay, float Life)
{
	if (!Mesh || !Material)
	{
		return nullptr;
	}
	FPiece& Piece = Pieces.AddDefaulted_GetRef();
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this);
	Comp->SetStaticMesh(Mesh);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCastShadow(false);
	Comp->bReceivesDecals = false;
	Comp->SetCanEverAffectNavigation(false);
	// Halo atrás de tudo, núcleo por cima da fumaça, riscos na frente
	Comp->TranslucencySortPriority = Kind == 1 ? -1 : (Kind == 0 ? 1 : (Kind == 2 ? 2 : 0));
	Comp->SetupAttachment(RootComponent);
	Comp->SetUsingAbsoluteLocation(true);
	Comp->SetUsingAbsoluteRotation(true);
	Comp->SetUsingAbsoluteScale(true);
	Comp->RegisterComponent();
	Comp->SetVisibility(false);
	Piece.MID = UMaterialInstanceDynamic::Create(Material, this);
	Piece.MID->SetScalarParameterValue(TEXT("Seed"), FMath::FRand() * 10.f);
	Comp->SetMaterial(0, Piece.MID);
	Piece.Mesh = Comp;
	Piece.Kind = Kind;
	Piece.Origin = Origin;
	Piece.Delay = Delay;
	Piece.Life = Life;
	Piece.Phase = FMath::FRand() * 10.f;
	return &Piece;
}

void ANinjaFireExplosion::BeginPlay()
{
	Super::BeginPlay();
	// Explodindo rente ao chão, o centro sobe um pouco: a bola de fogo não fica meio enterrada
	const FVector Mid = bHasFloor ? FVector(Center.X, Center.Y, FMath::Max(Center.Z, FloorPoint.Z + BallRadius * 0.45f)) : Center;
	if (FPiece* Halo = AddPiece(SphereMesh, HaloMaterial, 1, Mid, 0.f, HaloLife)) { Halo->Size = BallRadius; }
	if (FPiece* Dome = AddPiece(SphereMesh, DomeMaterial, 0, Mid, 0.f, DomeLife)) { Dome->Size = BallRadius; }
	if (FPiece* Heat = AddPiece(SphereMesh, HeatMaterial, 5, Mid, 0.02f, 0.5f)) { Heat->Size = BallRadius; }

	// Riscos finos em raios (meia esfera de cima, mais abertos para os lados)
	for (int32 i = 0; i < Streaks; ++i)
	{
		FVector Dir = FMath::VRand();
		Dir.Z = FMath::Abs(Dir.Z) * 0.9f + 0.05f;
		Dir.Normalize();
		if (FPiece* Streak = AddPiece(FlameMesh, StreakMaterial, 2, Mid + Dir * BallRadius * 0.3f, 0.06f + FMath::FRand() * 0.08f,
			FMath::FRandRange(StreakLife.X, StreakLife.Y)))
		{
			Streak->Velocity = Dir * StreakSpeed * FMath::FRandRange(0.75f, 1.2f);
			Streak->Height = FMath::FRandRange(StreakLength.X, StreakLength.Y);
		}
	}
	// Fumaça marrom-escura: nasce dentro do clarão, sobe e se desfaz
	for (int32 i = 0; i < SmokePuffs; ++i)
	{
		const FVector Offset = FMath::VRand() * FVector(1.f, 1.f, 0.4f) * BallRadius * 0.6f + FVector(0.f, 0.f, BallRadius * 0.35f);
		if (FPiece* Puff = AddPiece(SphereMesh, SmokeMaterial, 3, Mid + Offset, 0.1f + FMath::FRand() * 0.12f, FMath::FRandRange(SmokeLife.X, SmokeLife.Y)))
		{
			Puff->Size = FMath::FRandRange(SmokeRadius.X, SmokeRadius.Y);
			const FVector Spread = FVector(Offset.X, Offset.Y, 0.f).GetSafeNormal() * FMath::FRandRange(40.f, 90.f);
			Puff->Velocity = SmokeWind + Spread + FVector(0.f, 0.f, SmokeRise * FMath::FRandRange(0.8f, 1.25f));
		}
	}
	// Puffs vermelhos da dissipação: ficam no lugar do clarão, encolhem e somem
	for (int32 i = 0; i < EmberPuffs; ++i)
	{
		const FVector Offset = FMath::VRand() * FVector(1.f, 1.f, 0.5f) * BallRadius * 0.55f + FVector(0.f, 0.f, BallRadius * 0.1f);
		if (FPiece* Puff = AddPiece(SphereMesh, EmberPuffMaterial, 4, Mid + Offset, 0.3f + FMath::FRand() * 0.12f,
			FMath::FRandRange(EmberPuffLife.X, EmberPuffLife.Y)))
		{
			Puff->Size = FMath::FRandRange(EmberPuffRadius.X, EmberPuffRadius.Y);
			Puff->Velocity = FVector(0.f, 0.f, FMath::FRandRange(15.f, 40.f));
		}
	}

	if (GetNetMode() != NM_DedicatedServer)
	{
		for (UNiagaraSystem* System : {EmberSystem.Get(), DriftEmberSystem.Get()})
		{
			if (System)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, System, Mid, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::AutoRelease);
			}
		}
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
	const float X = Age / Piece.Life;
	float Erosion = 0.f;
	float Intensity = 1.f;
	FVector Location = Piece.Origin;
	FVector Scale(1.f);
	FRotator Rotation = FRotator::ZeroRotator;
	switch (Piece.Kind)
	{
	case 0:		// núcleo: estoura quase na hora (branco-amarelo), esfria para laranja, encolhe e some
	{
		const float R = Piece.Size * FMath::Lerp(0.5f, DomeScale, EaseOut(Age / FMath::Max(DomeGrowTime, 0.01f))) * FMath::Lerp(1.f, 0.6f, Smooth(0.3f, 1.f, X));
		Scale = FVector(R / 50.f);
		Location += FVector(0.f, 0.f, 40.f * EaseOut(X));
		Rotation = FRotator(0.f, Age * 90.f, 0.f);
		Erosion = Smooth(0.35f, 1.f, X);
		Intensity = 1.f + 2.5f * (1.f - Smooth(0.f, 0.3f, X));
		break;
	}
	case 1:		// halo vermelho grande e translúcido, apagando
	{
		const float R = Piece.Size * HaloScale * FMath::Lerp(0.6f, 1.f, EaseOut(X * 4.f));
		Scale = FVector(R / 50.f);
		Intensity = 1.6f * FMath::Square(1.f - X);
		break;
	}
	case 2:		// risco: dispara em linha reta, estica rápido e afina até sumir
	{
		const FVector Dir = Piece.Velocity.GetSafeNormal();
		Location += Piece.Velocity * Piece.Life * EaseOut(X);
		const float Len = Piece.Height * FMath::Lerp(0.4f, 1.f, EaseOut(X * 3.f)) * FMath::Lerp(1.f, 0.5f, X);
		const float Width = FMath::Lerp(5.f, 1.5f, X) / 40.f;
		Scale = FVector(Width, Width, Len / 100.f);
		Location -= Dir * Len;   // a ponta (topo do cone) vai na frente
		Rotation = FRotationMatrix::MakeFromZ(Dir).Rotator();
		Intensity = 1.f - Smooth(0.6f, 1.f, X);
		break;
	}
	case 3:		// fumaça: incha rápido, sobe, vai com o vento e se desfaz
	{
		Location += Piece.Velocity * Age;
		const float R = Piece.Size * FMath::Lerp(0.4f, 1.3f, EaseOut(X * 2.f));
		Scale = FVector(R / 50.f, R / 50.f, R / 50.f * 0.85f);
		Rotation = FRotator(0.f, Piece.Phase * 36.f + Age * 20.f, 0.f);
		Erosion = Smooth(0.25f, 1.f, X);
		break;
	}
	case 4:		// puff vermelho: já nasce cheio, sobe devagar, encolhe e apaga
	{
		Location += Piece.Velocity * Age;
		const float R = Piece.Size * FMath::Lerp(1.f, 0.15f, Smooth(0.15f, 1.f, X));
		Scale = FVector(R / 50.f);
		Rotation = FRotator(0.f, Piece.Phase * 36.f - Age * 30.f, 0.f);
		Erosion = Smooth(0.45f, 1.f, X);
		Intensity = 1.f - Smooth(0.5f, 1.f, X);
		break;
	}
	default:	// calor: distorção pequena em volta do clarão
	{
		const float R = Piece.Size * FMath::Lerp(1.f, 2.f, EaseOut(X));
		Scale = FVector(R / 50.f);
		Erosion = X;
		break;
	}
	}
	Mesh->SetWorldLocationAndRotation(Location, Rotation);
	Mesh->SetWorldScale3D(Scale);
	if (Piece.MID)
	{
		Piece.MID->SetScalarParameterValue(TEXT("Erosion"), Erosion);
		Piece.MID->SetScalarParameterValue(TEXT("Intensity"), Intensity);
		Piece.MID->SetScalarParameterValue(TEXT("Age"), X);
	}
}
