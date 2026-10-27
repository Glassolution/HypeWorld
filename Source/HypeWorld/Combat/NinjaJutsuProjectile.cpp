// Projétil de jutsu replicado (Bola de Fogo).

#include "NinjaJutsuProjectile.h"
#include "NinjaJutsuData.h"
#include "HypeWorldCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "LandscapeProxy.h"
#include "NinjaFireExplosion.h"
#include "Components/InstancedStaticMeshComponent.h"

namespace
{
	/** Chão de verdade embaixo da explosão (terreno ou piso), atravessando copas, folhagem e galhos: o fogo residual e o
	 *  queimado não nascem em cima da árvore que a bola acertou */
	bool FindFloorBelow(const AActor* Self, const FVector& From, float MaxDistance, FVector& OutPoint, FVector& OutNormal)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(KatonFloor), false, Self);
		if (const AActor* Caster = Self->GetInstigator())
		{
			Params.AddIgnoredActor(Caster);
		}
		FVector Start = From + FVector(0.f, 0.f, 50.f);
		const FVector End = From - FVector(0.f, 0.f, MaxDistance);
		for (int32 Try = 0; Try < 6; ++Try)
		{
			FHitResult Hit;
			if (!Self->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
			{
				return false;
			}
			const UPrimitiveComponent* Comp = Hit.GetComponent();
			const bool bLandscape = Hit.GetActor() && Hit.GetActor()->IsA(ALandscapeProxy::StaticClass());
			const bool bFoliage = Comp && Comp->IsA(UInstancedStaticMeshComponent::StaticClass());
			if (bLandscape || (!bFoliage && Hit.ImpactNormal.Z > 0.8f))
			{
				OutPoint = Hit.ImpactPoint;
				OutNormal = Hit.ImpactNormal;
				return true;
			}
			Params.AddIgnoredComponent(Comp);
			Start = Hit.ImpactPoint + FVector(0.f, 0.f, -1.f);
		}
		return false;
	}
}

namespace
{
	float EaseOutCubic(float X)
	{
		X = FMath::Clamp(X, 0.f, 1.f);
		return 1.f - FMath::Cube(1.f - X);
	}
}

ANinjaJutsuProjectile::ANinjaJutsuProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);   // cada máquina calcula a trajetória a partir de Launch

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto MakeShell = [this](const TCHAR* Name)
	{
		UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Mesh->SetupAttachment(Root);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->bAffectDistanceFieldLighting = false;
		Mesh->bAffectDynamicIndirectLighting = false;
		Mesh->bReceivesDecals = false;
		return Mesh;
	};
	Core = MakeShell(TEXT("Core"));
	Flames = MakeShell(TEXT("Flames"));
	OuterFlames = MakeShell(TEXT("OuterFlames"));
	Heat = MakeShell(TEXT("Heat"));
	Halo = MakeShell(TEXT("Halo"));
	GroundWave = MakeShell(TEXT("GroundWave"));
	GroundGlow = CreateDefaultSubobject<UDecalComponent>(TEXT("GroundGlow"));
	GroundGlow->SetupAttachment(Root);
	GroundGlow->SetFadeScreenSize(0.01f);
	GroundGlow->SetVisibility(false);
	Ignition = MakeShell(TEXT("Ignition"));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UNiagaraComponent* Spiral = CreateDefaultSubobject<UNiagaraComponent>(*FString::Printf(TEXT("Spiral%d"), Index));
		Spiral->SetupAttachment(Root);
		Spiral->SetAutoActivate(false);
		Spirals.Add(Spiral);
	}

	Trail = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Trail"));
	Trail->SetupAttachment(Root);
	Trail->SetAutoActivate(false);
	BodyFlames = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BodyFlames"));
	BodyFlames->SetupAttachment(Root);
	BodyFlames->SetAutoActivate(false);
	BodyFlames->SetCastShadow(false);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetCastShadows(false);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetIndirectLightingIntensity(0.f);
	Light->SetVolumetricScatteringIntensity(0.f);
	Light->bUseInverseSquaredFalloff = false;
	Light->SetLightFalloffExponent(2.f);
	BloomBounds = CreateDefaultSubobject<USphereComponent>(TEXT("BloomBounds"));
	BloomBounds->SetupAttachment(Root);
	BloomBounds->InitSphereRadius(750.f);
	BloomBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BloomBounds->SetGenerateOverlapEvents(false);
	FirePostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("FirePostProcess"));
	FirePostProcess->SetupAttachment(BloomBounds);
	FirePostProcess->bUnbound = false;
	FirePostProcess->BlendRadius = 350.f;
	FirePostProcess->Priority = 1.f;
	FirePostProcess->Settings.bOverride_BloomIntensity = true;
	FirePostProcess->Settings.BloomIntensity = 0.16f;
	FirePostProcess->Settings.bOverride_BloomThreshold = true;
	FirePostProcess->Settings.BloomThreshold = 2.f;
}

void ANinjaJutsuProjectile::InitLaunch(const UNinjaJutsuData* InJutsu, const FVector& InStart, const FVector& InDirection)
{
	Launch.Jutsu = InJutsu;
	Launch.Start = InStart;
	Launch.Direction = InDirection.GetSafeNormal();
	Launch.StartTime = Now();
	Launch.VisualSeed = FMath::RandRange(1, 100000);
	if (HasActorBegunPlay() && HasAuthority()) { OnRep_Launch(); }
}

void ANinjaJutsuProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ANinjaJutsuProjectile, Launch, COND_InitialOnly);
	DOREPLIFETIME(ANinjaJutsuProjectile, Impact);
}

double ANinjaJutsuProjectile::Now() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? GameState->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
}

float ANinjaJutsuProjectile::GetFlightTime() const
{
	return bLaunched ? float(Now() - Launch.StartTime) : 0.f;
}

AHypeWorldCharacter* ANinjaJutsuProjectile::GetCaster() const
{
	return Cast<AHypeWorldCharacter>(GetInstigator() ? GetInstigator() : GetOwner());
}

void ANinjaJutsuProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetupVisuals();
	if (HasAuthority())
	{
		OnRep_Launch();
	}
}

void ANinjaJutsuProjectile::SetupVisuals()
{
	const TPair<UStaticMeshComponent*, UMaterialInterface*> Shells[] = {
		{Core, CoreMaterial}, {Flames, FlameMaterial}, {OuterFlames, OuterFlameMaterial}, {Heat, HeatMaterial}};
	for (const TPair<UStaticMeshComponent*, UMaterialInterface*>& Shell : Shells)
	{
		Shell.Key->SetStaticMesh(BallMesh);
		Shell.Key->SetVisibility(false);
		if (Shell.Value)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Shell.Value, this);
			MID->SetScalarParameterValue(TEXT("Seed"), float(Launch.VisualSeed % 997) / 97.f);
			Shell.Key->SetMaterial(0, MID);
			MIDs.Add(MID);
		}
		else
		{
			MIDs.Add(nullptr);
			Shell.Key->SetHiddenInGame(true);
		}
	}
	if (TrailSystem)
	{
		Trail->SetAsset(TrailSystem);
	}
	Ignition->SetStaticMesh(IgnitionMesh);
	Ignition->SetMaterial(0, IgnitionMaterial);
	Ignition->SetVisibility(false);
	for (UNiagaraComponent* Spiral : Spirals) { Spiral->SetAsset(SpiralSystem); }
	BodyFlames->SetAsset(BodySystem);
	Heat->SetStaticMesh(IgnitionMesh);
	Halo->SetStaticMesh(IgnitionMesh);
	GroundWave->SetStaticMesh(IgnitionMesh);
	Halo->SetVisibility(false);
	GroundWave->SetVisibility(false);
	if (HaloMaterial) { HaloMID = UMaterialInstanceDynamic::Create(HaloMaterial, this); Halo->SetMaterial(0, HaloMID); }
	if (ShockwaveMaterial) { WaveMID = UMaterialInstanceDynamic::Create(ShockwaveMaterial, this); GroundWave->SetMaterial(0, WaveMID); }
	if (GroundGlowMaterial) { GlowMID = UMaterialInstanceDynamic::Create(GroundGlowMaterial, this); GroundGlow->SetDecalMaterial(GlowMID); }
	Light->SetLightColor(LightColor);
	Light->SetAttenuationRadius(LightRadius);
}

void ANinjaJutsuProjectile::OnRep_Launch()
{
	if (bLaunched || !Launch.Jutsu)
	{
		return;
	}
	bLaunched = true;
	LastCenter = Launch.Start;
	SetActorLocation(Launch.Start);
	SetActorRotation(FVector(Launch.Direction).Rotation());
	for (UMaterialInstanceDynamic* MID : MIDs)
	{
		if (MID) { MID->SetScalarParameterValue(TEXT("Seed"), float(Launch.VisualSeed % 997) / 97.f); }
	}
	if (Impact.bValid) { OnRep_Impact(); }
}

void ANinjaJutsuProjectile::OnRep_Impact()
{
	if (!Impact.bValid || !bLaunched || bExploded) { return; }
	PlayExplosion(Impact.Location);
	ExplodeTime = GetWorld()->GetTimeSeconds() - FMath::Max(0.0, Now() - Impact.ServerTime);
}

void ANinjaJutsuProjectile::Evaluate(float T, FVector& OutCenter, float& OutRadius, bool& bOutReachedRange) const
{
	const FVector Dir = Launch.Direction;
	const float Grow = EaseOutCubic(T / FMath::Max(GrowTime, 0.01f));
	float Distance = GrowDistance * Grow + Speed * FMath::Max(T - GrowTime, 0.f);
	bOutReachedRange = Distance >= Range;
	Distance = FMath::Min(Distance, Range);
	OutRadius = FMath::Lerp(StartRadius, FullRadius, Grow);
	OutCenter = FVector(Launch.Start) + Dir * Distance + FVector(0.f, 0.f, GrowLift * Grow);
}

void ANinjaJutsuProjectile::ApplyBallVisual(const FVector& Center, float Radius, float T)
{
	// Corpo anterior dentro das chamas vivas, com mais presença sem aumentar a emissão.
	if (BodySystem) { Radius *= 1.5f; }
	SetActorLocation(Center);
	const float MeshRadius = BallMesh ? FMath::Max(BallMesh->GetBounds().BoxExtent.GetMax(), 1.f) : 50.f;
	const float Base = Radius / MeshRadius;
	const float Grow = FMath::Clamp(T / FMath::Max(GrowTime, 0.01f), 0.f, 1.f);
	Core->SetVisibility(true);
	Flames->SetVisibility(true);
	OuterFlames->SetVisibility(!BodySystem);
	if (BodySystem)
	{
		BodyFlames->SetRelativeScale3D(FVector(Radius / 155.f));
		if (!BodyFlames->IsActive()) { BodyFlames->Activate(true); }
		// Spawn scaling is evaluated by Niagara; the solver and the atlas keep ticking between launches.
		float Distance = 0.f;
		if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			Distance = FVector::Distance(Camera->GetCameraLocation(), Center);
		}
		BodyFlames->SetVariableFloat(TEXT("User.SpawnScale"), Distance > 1800.f ? 0.55f : 1.f);
	}
	Heat->SetVisibility(Grow > 0.5f);
	const float InnerScale = BodySystem ? 0.85f : 1.f;
	Core->SetRelativeScale3D(FVector(1.4f, 1.f, 1.f) * Base * ShellScales.X * InnerScale);
	Flames->SetRelativeScale3D(FVector(1.55f, 1.f, 1.f) * Base * ShellScales.Y * InnerScale);
	OuterFlames->SetRelativeScale3D(FVector(1.65f, 1.f, 1.f) * Base * ShellScales.Z);
	Heat->SetRelativeScale3D(FVector(1.5f, 1.f, 1.f) * Base * HeatScale);
	// As cascas giram em sentidos opostos (o material também corre o ruído): chamas "rodando" em volta do núcleo
	Flames->SetRelativeRotation(FRotator(0.f, 0.f, SpinAngle));
	OuterFlames->SetRelativeRotation(FRotator(0.f, 0.f, -SpinAngle * 0.7f));
	for (int32 Index = 0; Index < Spirals.Num(); ++Index)
	{
		const float Angle = T * 28.f + Index * UE_TWO_PI / 3.f;
		Spirals[Index]->SetRelativeLocation(FVector(-Radius * 0.35f, FMath::Cos(Angle) * Radius * 1.12f, FMath::Sin(Angle) * Radius * 1.12f));
		if (SpiralSystem && Grow > 0.5f && !Spirals[Index]->IsActive()) { Spirals[Index]->Activate(true); }
	}
	Ignition->SetVisibility(IgnitionMesh && IgnitionMaterial && T < 0.25f);
	if (T < 0.25f)
	{
		const FVector Position = FVector(Launch.Start) + FVector(Launch.Direction) * (80.f + T * 320.f);
		Ignition->SetWorldLocation(Position);
		if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			Ignition->SetWorldRotation(FRotationMatrix::MakeFromZ(Camera->GetCameraLocation() - Position).Rotator());
		}
		Ignition->SetWorldScale3D(FVector(3.8f, 4.8f, 1.f) * FMath::Clamp(T / 0.06f, 0.1f, 1.f));
	}
	FirePostProcess->Settings.BloomIntensity = T < 0.08f ? 0.95f : 0.75f;
	FirePostProcess->bEnabled = true;
	for (UMaterialInstanceDynamic* MID : MIDs)
	{
		if (MID)
		{
			MID->SetScalarParameterValue(TEXT("Grow"), Grow);
			MID->SetScalarParameterValue(TEXT("Dissolve"), 0.f);
			MID->SetScalarParameterValue(TEXT("Radius"), Radius);
			MID->SetVectorParameterValue(TEXT("TravelDirection"), FLinearColor(Launch.Direction.X, Launch.Direction.Y, Launch.Direction.Z));
		}
	}
	for (int32 Index = 0; Index < FMath::Min(MIDs.Num(), 3); ++Index)
	{
		if (MIDs[Index]) { MIDs[Index]->SetScalarParameterValue(TEXT("Intensity"), Index == 0 ? 1.8f : 1.25f); }
	}
	UpdateSceneIntegration(Center, Radius, T, false);
	const float Flicker = 1.f + .065f * FMath::Sin(T * 41.f) + .035f * FMath::Sin(T * 73.f);
	Light->SetIntensity(LightIntensity * Flicker * FMath::Lerp(0.5f, 1.f, Grow) * (T < .08f ? 1.8f : 1.f));
	if (!Trail->IsActive() && Grow >= 0.6f && TrailSystem)
	{
		Trail->Activate(true);
	}
}

void ANinjaJutsuProjectile::UpdateSceneIntegration(const FVector& Center, float Radius, float T, bool bImpact)
{
	if (GetNetMode() == NM_DedicatedServer) { return; }
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FRotator Facing = FRotationMatrix::MakeFromZ(Camera->GetCameraLocation() - Center).Rotator();
		Halo->SetWorldLocation(Center);
		Halo->SetWorldRotation(Facing);
		Halo->SetWorldScale3D(FVector(Radius * .032f, Radius * .032f, 1.f));
		Halo->SetVisibility(HaloMaterial && (!bImpact || T < .55f));
		if (HaloMID) { HaloMID->SetScalarParameterValue(TEXT("Intensity"), bImpact ? 2.2f * FMath::Exp(-T * 8.f) : .9f); }
		Heat->SetWorldLocation(Center - (bImpact ? FVector::ZeroVector : FVector(Launch.Direction) * Radius * .5f));
		Heat->SetWorldRotation(Facing);
		Heat->SetWorldScale3D(FVector(Radius * .035f, Radius * .035f, 1.f));
		Heat->SetVisibility(HeatMaterial && (!bImpact || T < .75f));
	}
	const double Clock = GetWorld()->GetTimeSeconds();
	if ((!bImpact && Clock - LastGroundTrace > .1) || LastGroundTrace < 0.)
	{
		LastGroundTrace = Clock;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(KatonGroundGlow), false, this);
		Params.AddIgnoredActor(GetCaster());
		bHasGround = GetWorld()->LineTraceSingleByChannel(Hit, Center + FVector(0,0,80), Center - FVector(0,0,1200), ECC_Visibility, Params);
		if (bHasGround) { GroundPoint = Hit.ImpactPoint; GroundNormal = Hit.ImpactNormal; }
	}
	GroundGlow->SetVisibility(GlowMID && bHasGround && (!bImpact || T < 2.5f));
	if (bHasGround)
	{
		GroundGlow->SetWorldLocation(GroundPoint + GroundNormal * 3.f);
		GroundGlow->SetWorldRotation((-GroundNormal).Rotation());
		GroundGlow->DecalSize = FVector(100.f, Radius * 2.3f, Radius * 2.3f);
		if (GlowMID) { GlowMID->SetScalarParameterValue(TEXT("Intensity"), bImpact ? 1.8f * FMath::Exp(-T * 1.6f) : 1.f); }
		GroundWave->SetVisibility(bImpact && ShockwaveMaterial && T < .6f);
		if (bImpact)
		{
			GroundWave->SetWorldLocation(GroundPoint + GroundNormal * 8.f);
			GroundWave->SetWorldRotation(FRotationMatrix::MakeFromZ(GroundNormal).Rotator());
			GroundWave->SetWorldScale3D(FVector(16.f,16.f,1.f));
			if (WaveMID) { WaveMID->SetScalarParameterValue(TEXT("Progress"), FMath::Clamp(T / .6f, 0.f, 1.f)); }
		}
	}
}

void ANinjaJutsuProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bLaunched)
	{
		return;
	}
	SpinAngle = FMath::Fmod(SpinAngle + SpinSpeed * DeltaSeconds, 3600.f);

	if (!bExploded)
	{
		const float T = float(Now() - Launch.StartTime);
		FVector Center;
		float Radius;
		bool bReachedRange;
		Evaluate(FMath::Max(T, 0.f), Center, Radius, bReachedRange);
		ApplyBallVisual(Center, Radius, T);
		if (HasAuthority())
		{
			ServerCheckHit(LastCenter, Center, Radius, T);
			if (!bExploded && bReachedRange)
			{
				Explode(Center);
			}
		}
		LastCenter = Center;
		LastRadius = Radius;
		return;
	}

	// Depois da explosão: onda de fogo crescendo e se desfazendo, luz do clarão caindo
	const float E = float(GetWorld()->GetTimeSeconds() - ExplodeTime);
	if (StylizedExplosionClass)
	{
		// A explosão estilizada cuida de todo o visual; aqui fica só o clarão da luz
		UPrimitiveComponent* OldLayers[] = {Core.Get(), Flames.Get(), OuterFlames.Get(), Heat.Get(), Halo.Get(), GroundWave.Get()};
		for (UPrimitiveComponent* Old : OldLayers)
		{
			Old->SetVisibility(false);
		}
		GroundGlow->SetVisibility(false);
		FirePostProcess->bEnabled = false;
		const float StylFlash = FMath::Clamp(1.f - E / FMath::Max(FlashTime, 0.01f), 0.f, 1.f);
		Light->SetIntensity(LightIntensity * (FlashMultiplier * FMath::Square(StylFlash) + .28f * FMath::Exp(-E * 1.5f)));
		Light->SetAttenuationRadius(LightRadius);
		return;
	}
	const float MeshRadius = BallMesh ? FMath::Max(BallMesh->GetBounds().BoxExtent.GetMax(), 1.f) : 50.f;
	const float W = FMath::Clamp(E / FMath::Max(WaveTime, 0.01f), 0.f, 1.f);
	const float WaveRadius = FullRadius * FMath::Lerp(.65f, WaveScale, EaseOutCubic(W));
	Core->SetRelativeLocation(FVector(0,0,WaveRadius * .08f));
	Flames->SetRelativeLocation(FVector(0,0,WaveRadius * .08f));
	Flames->SetRelativeScale3D(FVector(WaveRadius * ShellScales.Y / MeshRadius));
	OuterFlames->SetRelativeScale3D(FVector(WaveRadius * ShellScales.Z / MeshRadius * 1.05f));
	Core->SetRelativeScale3D(FVector(FullRadius * ShellScales.X / MeshRadius * FMath::Lerp(1.2f, 0.2f, FMath::Clamp(E / 0.25f, 0.f, 1.f))));
	Core->SetVisibility(E < 0.08f);
	Flames->SetVisibility(W < 1.f);
	OuterFlames->SetVisibility(W < 1.f);
	for (int32 i = 0; i < MIDs.Num(); ++i)
	{
		if (MIDs[i])
		{
			MIDs[i]->SetScalarParameterValue(TEXT("Dissolve"), i == 0 ? 0.f : FMath::Pow(W, 0.8f));
			MIDs[i]->SetScalarParameterValue(TEXT("Intensity"), i == 0 ? 4.f : 2.1f * (1.f - W));
		}
	}
	const float Flash = FMath::Clamp(1.f - E / FMath::Max(FlashTime, 0.01f), 0.f, 1.f);
	Light->SetIntensity(LightIntensity * (FlashMultiplier * FMath::Square(Flash) + .28f * FMath::Exp(-E * 1.5f)));
	Light->SetAttenuationRadius(LightRadius);
	UpdateSceneIntegration(GetActorLocation(), WaveRadius, E, true);
}

void ANinjaJutsuProjectile::ServerCheckHit(const FVector& From, const FVector& To, float Radius, float T)
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NinjaJutsuProjectile), false, this);
	Params.bFindInitialOverlaps = true;
	if (AActor* Caster = GetCaster())
	{
		Params.AddIgnoredActor(Caster);
		TArray<AActor*> Attached;
		Caster->GetAttachedActors(Attached);
		Params.AddIgnoredActors(Attached);
	}
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	FHitResult Hit;
	const float CollisionRadius = FMath::Max(Radius * CollisionRadiusScale, 15.f);
	if (GetWorld()->SweepSingleByObjectType(Hit, From, To, FQuat::Identity, Objects, FCollisionShape::MakeSphere(CollisionRadius), Params))
	{
		Explode(Hit.ImpactPoint + Hit.ImpactNormal * 3.f);
	}
}

void ANinjaJutsuProjectile::Explode(const FVector& Location)
{
	if (!HasAuthority() || bExploded || Impact.bValid)
	{
		return;
	}
	Impact.bValid = true;
	Impact.Location = Location;
	Impact.ServerTime = Now();
	ApplyAreaDamage(Location);
	MulticastExplode(Location);
	ForceNetUpdate();
	SetLifeSpan(LingerTime + 0.5f);
}

void ANinjaJutsuProjectile::MulticastExplode_Implementation(FVector_NetQuantize10 Location)
{
	if (!bExploded)
	{
		PlayExplosion(Location);
	}
}

void ANinjaJutsuProjectile::PlayExplosion(const FVector& Location)
{
	BodyFlames->Deactivate();
	bExploded = true;
	ExplodeTime = GetWorld()->GetTimeSeconds();
	SetActorLocation(Location);
	SetActorRotation(FRotator::ZeroRotator);
	LastGroundTrace = -100.;
	UpdateSceneIntegration(Location, FullRadius, 0.f, true);
	Trail->Deactivate();
	Ignition->SetVisibility(false);
	for (UNiagaraComponent* Spiral : Spirals) { Spiral->Deactivate(); }
	FirePostProcess->Settings.BloomIntensity = 0.65f;
	FirePostProcess->bEnabled = true;
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	FVector FloorPoint, FloorNormal;
	const bool bFloor = FindFloorBelow(this, Location, FullRadius * 3.f, FloorPoint, FloorNormal);
	if (StylizedExplosionClass)
	{
		const FTransform At(Location);
		if (ANinjaFireExplosion* Fx = GetWorld()->SpawnActorDeferred<ANinjaFireExplosion>(StylizedExplosionClass, At, this))
		{
			Fx->Init(Location, FullRadius, bFloor, FloorPoint, FloorNormal);
			Fx->FinishSpawning(At);
		}
	}
	else if (ExplosionSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionSystem, Location, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::AutoRelease);
	}
	if (GroundFireSystem && bFloor && !StylizedExplosionClass)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, GroundFireSystem, FloorPoint + FloorNormal * 15.f,
			FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::AutoRelease);
	}

	// Queimado no chão: só se a explosão foi perto do chão
	if (ScorchDecal)
	{
		if (bFloor)
		{
			const FRotator DecalRot = (-FloorNormal).Rotation() + FRotator(0.f, 0.f, FMath::FRandRange(0.f, 360.f));
			if (UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(this, ScorchDecal, FVector(120.f, ScorchSize, ScorchSize),
				FloorPoint, DecalRot, ScorchHold + ScorchFade + 0.1f))
			{
				Decal->SetFadeOut(ScorchHold, ScorchFade, false);
				Decal->SetFadeScreenSize(0.f);
			}
		}
	}

	// Tremida para quem está perto (cada jogador local)
	if (const UNinjaJutsuData* Jutsu = Launch.Jutsu)
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PC = It->Get();
			AHypeWorldCharacter* Viewer = PC && PC->IsLocalController() ? Cast<AHypeWorldCharacter>(PC->GetPawn()) : nullptr;
			if (Viewer)
			{
				const float Falloff = 1.f - FMath::Clamp(FVector::Dist(Viewer->GetActorLocation(), Location) / FMath::Max(Jutsu->ExplosionShakeRadius, 1.f), 0.f, 1.f);
				if (Falloff > 0.f)
				{
					Viewer->AddCameraShake(Jutsu->ExplosionShake * Falloff, 0.55f);
				}
			}
		}
	}
}

void ANinjaJutsuProjectile::ApplyAreaDamage(const FVector& Location)
{
	const UNinjaJutsuData* Jutsu = Launch.Jutsu;
	AHypeWorldCharacter* Caster = GetCaster();
	if (!Jutsu || !Caster)
	{
		return;
	}
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NinjaJutsuArea), false, this);
	Params.AddIgnoredActor(Caster);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Location, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Jutsu->DamageRadius), Params);
	TSet<AHypeWorldCharacter*> Done;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AHypeWorldCharacter* Target = Cast<AHypeWorldCharacter>(Overlap.GetActor());
		if (!Target || Target == Caster || Done.Contains(Target))
		{
			continue;
		}
		Done.Add(Target);
		FHitResult Occlusion;
		FCollisionQueryParams VisibilityParams(SCENE_QUERY_STAT(NinjaJutsuOcclusion), false, this);
		VisibilityParams.AddIgnoredActor(Caster);
		if (GetWorld()->LineTraceSingleByChannel(Occlusion, Location, Target->GetActorLocation(), ECC_Visibility, VisibilityParams)
			&& Occlusion.GetActor() != Target) { continue; }
		const float Dist = FVector::Dist(Target->GetActorLocation(), Location);
		const float Scale = FMath::Lerp(1.f, Jutsu->EdgeDamageScale, FMath::Clamp(Dist / FMath::Max(Jutsu->DamageRadius, 1.f), 0.f, 1.f));
		FHitResult Hit;
		Hit.Location = Hit.ImpactPoint = Target->GetActorLocation();
		Hit.HitObjectHandle = FActorInstanceHandle(Target);
		Caster->ApplyJutsuHit(Target, Jutsu, Scale, Hit);
	}
	// Props dinamicos recebem impulso real do Chaos, separadamente do knockback dos personagens.
	TArray<FOverlapResult> Bodies;
	GetWorld()->OverlapMultiByObjectType(Bodies, Location, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_PhysicsBody), FCollisionShape::MakeSphere(Jutsu->DamageRadius), Params);
	TSet<UPrimitiveComponent*> Pushed;
	for (const FOverlapResult& Body : Bodies)
	{
		UPrimitiveComponent* Component = Body.GetComponent();
		if (!Component || !Component->IsSimulatingPhysics() || Pushed.Contains(Component)) { continue; }
		Pushed.Add(Component);
		FHitResult Blocker;
		if (!GetWorld()->LineTraceSingleByChannel(Blocker, Location, Component->GetComponentLocation(), ECC_Visibility, Params)
			|| Blocker.GetComponent() == Component)
		{
			Component->AddRadialImpulse(Location, Jutsu->DamageRadius, Jutsu->PhysicsImpulse, ERadialImpulseFalloff::RIF_Linear, false);
		}
	}
}
