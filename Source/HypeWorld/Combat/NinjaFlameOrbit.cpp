// Chamas em órbita (Katon: Hōsenka), replicadas.

#include "NinjaFlameOrbit.h"
#include "NinjaJutsuData.h"
#include "NinjaJutsuProjectile.h"
#include "HypeWorldCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"

ANinjaFlameOrbit::ANinjaFlameOrbit()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);   // cada máquina segue o personagem e calcula as chamas pelo relógio
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetCastShadows(false);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetVolumetricScatteringIntensity(0.f);
	Light->bUseInverseSquaredFalloff = false;
	Light->SetLightFalloffExponent(3.f);
}

void ANinjaFlameOrbit::Init(const UNinjaJutsuData* InJutsu)
{
	Jutsu = InJutsu;
	StartTime = Now();
	const int32 Count = FMath::Clamp(InJutsu ? InJutsu->OrbitFlames : 0, 0, 31);
	AliveMask = (1 << Count) - 1;
}

void ANinjaFlameOrbit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ANinjaFlameOrbit, Jutsu, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ANinjaFlameOrbit, StartTime, COND_InitialOnly);
	DOREPLIFETIME(ANinjaFlameOrbit, AliveMask);
	DOREPLIFETIME(ANinjaFlameOrbit, bEnded);
}

double ANinjaFlameOrbit::Now() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? GameState->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
}

float ANinjaFlameOrbit::GetElapsed() const
{
	return float(FMath::Max(Now() - StartTime, 0.0));
}

AHypeWorldCharacter* ANinjaFlameOrbit::GetCaster() const
{
	return Cast<AHypeWorldCharacter>(GetOwner());
}

int32 ANinjaFlameOrbit::GetFlameCount() const
{
	return Jutsu ? FMath::Clamp(Jutsu->OrbitFlames, 0, 31) : 0;
}

int32 ANinjaFlameOrbit::GetFlamesLeft() const
{
	return FMath::CountBits(uint32(AliveMask));
}

float ANinjaFlameOrbit::GetTimeLeft(float& OutDuration) const
{
	OutDuration = Jutsu ? Jutsu->OrbitDuration : 1.f;
	return FMath::Max(OutDuration - GetElapsed(), 0.f);
}

float ANinjaFlameOrbit::GetJutsuRange() const
{
	if (Jutsu && Jutsu->ProjectileRange > 0.f) { return Jutsu->ProjectileRange; }
	const ANinjaJutsuProjectile* Defaults = Jutsu && Jutsu->ProjectileClass ? Jutsu->ProjectileClass->GetDefaultObject<ANinjaJutsuProjectile>() : nullptr;
	return Defaults ? Defaults->Range : 3000.f;
}

FVector ANinjaFlameOrbit::GetCenter() const
{
	const AActor* Caster = GetOwner();
	return Caster ? Caster->GetActorLocation() : GetActorLocation();
}

void ANinjaFlameOrbit::EvaluateFlame(int32 Index, float T, const FVector& Center, FVector& OutPos, FVector& OutTangent) const
{
	const int32 Count = FMath::Max(GetFlameCount(), 1);
	const float Radius = Jutsu ? Jutsu->OrbitRadius : 200.f;
	const float Omega = FMath::DegreesToRadians(Jutsu ? Jutsu->OrbitSpeed : 120.f);
	const float Angle = Index * UE_TWO_PI / Count + Omega * T;
	// Alturas espalhadas (cintura até acima da cabeça), subindo e descendo devagar; raio respirando
	const float Height = FMath::Lerp(HeightRange.X, HeightRange.Y, FMath::Frac(Index * 0.618f)) + Bob * FMath::Sin(T * 2.1f + Index * 1.7f);
	const float R = Radius * (1.f + 0.08f * FMath::Sin(T * 1.6f + Index * 1.3f));
	OutPos = Center + FVector(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R, Height);
	OutTangent = FVector(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f) * FMath::Sign(Omega == 0.f ? 1.f : Omega);
}

void ANinjaFlameOrbit::BeginPlay()
{
	Super::BeginPlay();
	if (AHypeWorldCharacter* Caster = GetCaster())
	{
		Caster->SetActiveFlameOrbit(this);
	}
	SetActorLocation(GetCenter());
	Light->SetLightColor(LightColor);
	Light->SetAttenuationRadius(LightRadius);
	if (HasAuthority() && Jutsu)
	{
		SetLifeSpan(Jutsu->OrbitDuration + FadeTime + 1.f);
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		BuildVisuals();
	}
}

void ANinjaFlameOrbit::EndPlay(const EEndPlayReason::Type Reason)
{
	if (AHypeWorldCharacter* Caster = GetCaster())
	{
		if (Caster->GetActiveFlameOrbit() == this) { Caster->SetActiveFlameOrbit(nullptr); }
	}
	Super::EndPlay(Reason);
}

void ANinjaFlameOrbit::BuildVisuals()
{
	if (bVisualsBuilt || !Jutsu || !FlameMesh || !FlameMaterial) { return; }
	bVisualsBuilt = true;
	auto MakeMesh = [this](UStaticMesh* Mesh, UMaterialInterface* Material, int32 Priority, FFlame& Flame)
	{
		if (!Mesh || !Material) { return (UStaticMeshComponent*)nullptr; }
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this);
		Comp->SetStaticMesh(Mesh);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(false);
		Comp->bReceivesDecals = false;
		Comp->SetCanEverAffectNavigation(false);
		Comp->TranslucencySortPriority = Priority;
		Comp->SetUsingAbsoluteLocation(true);
		Comp->SetUsingAbsoluteRotation(true);
		Comp->SetUsingAbsoluteScale(true);
		Comp->SetupAttachment(Root);
		Comp->RegisterComponent();
		Comp->SetVisibility(false);
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Material, this);
		MID->SetScalarParameterValue(TEXT("Seed"), FMath::FRand() * 10.f);
		Comp->SetMaterial(0, MID);
		Flame.MIDs.Add(MID);
		return Comp;
	};
	for (int32 Index = 0; Index < GetFlameCount(); ++Index)
	{
		FFlame& Flame = Flames.AddDefaulted_GetRef();
		Flame.Halo = MakeMesh(GlowMesh, HaloMaterial, 0, Flame);
		Flame.Body = MakeMesh(FlameMesh, FlameMaterial, 1, Flame);
		Flame.Tongue = MakeMesh(TongueMesh, TongueMaterial, 1, Flame);
		Flame.Core = MakeMesh(GlowMesh, CoreMaterial, 2, Flame);
		if (EmberSystem)
		{
			Flame.Embers = UNiagaraFunctionLibrary::SpawnSystemAttached(EmberSystem, Root, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset, false, false, ENCPoolMethod::None);
			if (Flame.Embers) { Flame.Embers->SetUsingAbsoluteLocation(true); }
		}
		Flame.TongueAge = FMath::FRand() * 0.3f;
	}
}

bool ANinjaFlameOrbit::FireFlame(const FVector& AimPoint)
{
	if (!HasAuthority() || bEnded || !Jutsu || !Jutsu->ProjectileClass || AliveMask == 0) { return false; }
	const double Clock = Now();
	if (Clock - LastFireTime < Jutsu->OrbitFireInterval) { return false; }
	// Lança a chama que está mais à frente na direção da mira (sai da órbita fazendo uma curva curta)
	const float T = GetElapsed();
	const FVector Center = GetCenter();
	const FVector AimFlat = FVector(AimPoint - Center).GetSafeNormal2D();
	int32 Best = INDEX_NONE;
	float BestDot = -2.f;
	FVector BestPos = Center;
	for (int32 Index = 0; Index < GetFlameCount(); ++Index)
	{
		if (!(AliveMask & (1 << Index))) { continue; }
		FVector Pos, Tangent;
		EvaluateFlame(Index, T, Center, Pos, Tangent);
		const float Dot = FVector::DotProduct(FVector(Pos - Center).GetSafeNormal2D(), AimFlat);
		if (Dot > BestDot) { BestDot = Dot; Best = Index; BestPos = Pos; }
	}
	if (Best == INDEX_NONE) { return false; }
	AliveMask &= ~(1 << Best);
	LastFireTime = Clock;
	ForceNetUpdate();
	const FVector Dir = (AimPoint - BestPos).GetSafeNormal();
	const FTransform SpawnAt(Dir.Rotation(), BestPos);
	if (ANinjaJutsuProjectile* Shot = GetWorld()->SpawnActorDeferred<ANinjaJutsuProjectile>(Jutsu->ProjectileClass, SpawnAt, GetOwner(),
		Cast<APawn>(GetOwner()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Shot->InitLaunch(Jutsu, BestPos, Dir);
		Shot->FinishSpawning(SpawnAt);
	}
	if (AliveMask == 0) { EndOrbit(); }
	return true;
}

void ANinjaFlameOrbit::EndOrbit()
{
	if (bEnded) { return; }
	bEnded = true;
	OnRep_Ended();
	if (HasAuthority())
	{
		ForceNetUpdate();
		SetLifeSpan(FadeTime + 0.5f);
	}
}

void ANinjaFlameOrbit::OnRep_Ended()
{
	LocalEndTime = GetWorld()->GetTimeSeconds();
}

void ANinjaFlameOrbit::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Jutsu) { return; }
	const float T = GetElapsed();
	if (HasAuthority() && !bEnded && T > Jutsu->OrbitDuration) { EndOrbit(); }
	const FVector Center = GetCenter();
	SetActorLocation(Center);
	if (GetNetMode() == NM_DedicatedServer) { return; }
	BuildVisuals();

	const float Fade = bEnded ? 1.f - FMath::SmoothStep(0.f, FadeTime, float(GetWorld()->GetTimeSeconds() - LocalEndTime)) : 1.f;
	const float GlowRadius = GlowMesh ? FMath::Max(GlowMesh->GetBounds().BoxExtent.GetMax(), 1.f) : 50.f;
	const float BodyRadius = FlameMesh ? FMath::Max(FlameMesh->GetBounds().BoxExtent.X, 1.f) : 50.f;
	int32 Alive = 0;
	for (int32 Index = 0; Index < Flames.Num(); ++Index)
	{
		FFlame& Flame = Flames[Index];
		const bool bAlive = (AliveMask & (1 << Index)) != 0;
		// Invocação: cada chama acende com um pulo de escala e brilho, uma depois da outra
		const float Born = T - Index * IgniteStagger;
		const float Grow = FMath::SmoothStep(0.f, 0.12f, Born);
		const float Ignite = FMath::Clamp(1.f - FMath::Abs(Born - 0.06f) / 0.12f, 0.f, 1.f);
		const float Show = bAlive ? Grow * Fade : 0.f;
		const bool bVisible = Show > 0.01f;
		auto SetVis = [bVisible](UStaticMeshComponent* Comp) { if (Comp) { Comp->SetVisibility(bVisible); } };
		SetVis(Flame.Body); SetVis(Flame.Core); SetVis(Flame.Halo); SetVis(Flame.Tongue);
		if (Flame.Embers)
		{
			if (bVisible && !Flame.Embers->IsActive()) { Flame.Embers->Activate(true); }
			else if (!bVisible && Flame.Embers->IsActive()) { Flame.Embers->Deactivate(); }
		}
		if (!bVisible) { continue; }
		++Alive;
		FVector Pos, Tangent;
		EvaluateFlame(Index, T, Center, Pos, Tangent);
		// Fogo vivo: pulsa; inclina e estica para trás no giro (as pontas do modelo viram a cauda)
		const float Pulse = 1.f + 0.07f * FMath::Sin(T * 15.f + Index * 2.3f) + 0.04f * FMath::Sin(T * 31.f + Index);
		const float Size = FlameSize * Show * Pulse * (1.f + 0.35f * Ignite);
		const FVector Up = (FVector::UpVector * 0.55f - Tangent * SwirlLean).GetSafeNormal();
		const FQuat Facing = FRotationMatrix::MakeFromZ(Up).ToQuat() * FQuat(FVector::UpVector, T * 2.f + Index);
		if (Flame.Body)
		{
			Flame.Body->SetWorldLocationAndRotation(Pos, Facing);
			Flame.Body->SetWorldScale3D(FVector(Size / BodyRadius) * FVector(1.f, 1.f, 1.6f));
		}
		if (Flame.Core)
		{
			Flame.Core->SetWorldLocation(Pos - Up * Size * 0.25f);
			Flame.Core->SetWorldScale3D(FVector(Size * 0.3f / GlowRadius));
		}
		if (Flame.Halo)
		{
			Flame.Halo->SetWorldLocation(Pos);
			Flame.Halo->SetWorldScale3D(FVector(Size * 1.5f / GlowRadius));
		}
		for (UMaterialInstanceDynamic* MID : Flame.MIDs)
		{
			MID->SetScalarParameterValue(TEXT("Intensity"), Fade * (1.f + 3.f * Ignite));
		}
		// Uma língua de chama se solta para trás/cima e se desfaz; quando some, nasce outra
		Flame.TongueAge += DeltaSeconds;
		if (Flame.TongueAge > Flame.TongueLife)
		{
			Flame.TongueAge = 0.f;
			Flame.TongueLife = FMath::FRandRange(0.22f, 0.38f);
			Flame.TonguePos = Pos + FMath::VRand() * Size * 0.3f;
			Flame.TongueVel = -Tangent * FMath::FRandRange(60.f, 120.f) + FVector(0.f, 0.f, FMath::FRandRange(80.f, 160.f));
		}
		if (Flame.Tongue)
		{
			const float X = Flame.TongueAge / Flame.TongueLife;
			Flame.TonguePos += Flame.TongueVel * DeltaSeconds;
			const float Len = Size * FMath::Lerp(0.8f, 1.6f, X);
			const float Width = Size * 0.32f * (1.f - 0.6f * X);
			Flame.Tongue->SetWorldLocationAndRotation(Flame.TonguePos, FRotationMatrix::MakeFromZ(Flame.TongueVel).ToQuat());
			Flame.Tongue->SetWorldScale3D(FVector(Width / 40.f, Width / 40.f, Len / 100.f));
			if (Flame.MIDs.IsValidIndex(2) && Flame.Body && Flame.Halo)   // halo, corpo, língua, núcleo
			{
				Flame.MIDs[2]->SetScalarParameterValue(TEXT("Erosion"), FMath::SmoothStep(0.3f, 1.f, X));
			}
		}
		if (Flame.Embers) { Flame.Embers->SetWorldLocation(Pos); }
	}
	// Uma luz para o grupo: acompanha o personagem, mais forte com mais chamas e com o clarão da invocação
	const float Flash = 1.f - FMath::SmoothStep(0.f, 0.35f, T);
	const float Share = Flames.Num() > 0 ? float(Alive) / Flames.Num() : 0.f;
	Light->SetWorldLocation(Center + FVector(0.f, 0.f, 60.f));
	Light->SetIntensity(LightIntensity * (0.35f + 0.65f * Share) * Fade * (1.f + LightFlash * Flash) * FMath::SmoothStep(0.f, 0.05f, T));
	Light->SetAttenuationRadius(LightRadius * (1.f + 0.6f * Flash));
}
