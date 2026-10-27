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
	FireBody = MakeShell(TEXT("FireBody"));
	FireBody->SetVisibility(false);
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
	Light->SetLightFalloffExponent(5.f);   // poça de luz local em volta da bola, sem lavar o chão inteiro
	// Ordem das camadas translúcidas (mesmo centro): halo atrás, corpo, núcleo, casca na frente
	Halo->TranslucencySortPriority = -1;
	Core->TranslucencySortPriority = 1;
	OuterFlames->TranslucencySortPriority = 2;
	Ignition->TranslucencySortPriority = -1;
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

void ANinjaJutsuProjectile::Prewarm(AActor* Context, const UNinjaJutsuData* Jutsu)
{
	UWorld* World = Context ? Context->GetWorld() : nullptr;
	if (!World || !Jutsu || !Jutsu->ProjectileClass || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	static TArray<TPair<TWeakObjectPtr<UWorld>, TWeakObjectPtr<UClass>>> Done;
	Done.RemoveAll([](const TPair<TWeakObjectPtr<UWorld>, TWeakObjectPtr<UClass>>& Entry) { return !Entry.Key.IsValid() || !Entry.Value.IsValid(); });
	const TPair<TWeakObjectPtr<UWorld>, TWeakObjectPtr<UClass>> Key(World, Jutsu->ProjectileClass.Get());
	if (Done.Contains(Key))
	{
		return;
	}
	Done.Add(Key);
	const FVector Start = Context->GetActorLocation() - FVector(0.f, 0.f, 20000.f);
	const FTransform At(Start);
	if (ANinjaJutsuProjectile* Copy = World->SpawnActorDeferred<ANinjaJutsuProjectile>(Jutsu->ProjectileClass, At, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Copy->bPrewarm = true;
		Copy->SetReplicates(false);   // no host não pode virar um projétil de verdade nos clientes
		Copy->Launch.Jutsu = Jutsu;
		Copy->Launch.Start = Start;
		Copy->Launch.Direction = FVector::ForwardVector;
		Copy->FinishSpawning(At);
		if (Copy->MuzzleSystem)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(Copy, Copy->MuzzleSystem, Start, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::AutoRelease);
		}
	}
}

TArray<USceneComponent*> ANinjaJutsuProjectile::SpawnAnticipation(USceneComponent* Body, FName Socket, const UNinjaJutsuData* Jutsu)
{
	TArray<USceneComponent*> Spawned;
	const ANinjaJutsuProjectile* Defaults = Jutsu && Jutsu->ProjectileClass ? Jutsu->ProjectileClass->GetDefaultObject<ANinjaJutsuProjectile>() : nullptr;
	if (!Body || !Defaults || !Body->GetWorld() || Body->GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return Spawned;
	}
	if (!Body->DoesSocketExist(Socket)) { Socket = NAME_None; }
	AActor* Owner = Body->GetOwner();
	if (Defaults->AnticipationSystem)
	{
		Spawned.Add(UNiagaraFunctionLibrary::SpawnSystemAttached(Defaults->AnticipationSystem, Body, Socket, FVector::ZeroVector,
			FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true, true, ENCPoolMethod::None));
	}
	if (Defaults->GlowMesh && Defaults->AnticipationMaterial)
	{
		UStaticMeshComponent* Glow = NewObject<UStaticMeshComponent>(Owner);
		Glow->SetStaticMesh(Defaults->GlowMesh);
		Glow->SetMaterial(0, Defaults->AnticipationMaterial);
		Glow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Glow->SetCastShadow(false);
		Glow->bReceivesDecals = false;
		Glow->SetupAttachment(Body, Socket);
		Glow->SetRelativeScale3D(FVector(0.32f));
		Glow->RegisterComponent();
		Spawned.Add(Glow);
	}
	if (Defaults->AnticipationLight > 0.f)
	{
		UPointLightComponent* Glow = NewObject<UPointLightComponent>(Owner);
		Glow->SetCastShadows(false);
		Glow->SetIntensityUnits(ELightUnits::Candelas);
		Glow->SetIntensity(Defaults->AnticipationLight);
		Glow->SetLightColor(Defaults->LightColor);
		Glow->SetAttenuationRadius(260.f);
		Glow->SetVolumetricScatteringIntensity(0.f);
		Glow->SetupAttachment(Body, Socket);
		Glow->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
		Glow->RegisterComponent();
		Spawned.Add(Glow);
	}
	Spawned.Remove(nullptr);
	return Spawned;
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
	return bLaunched ? float(GetWorld()->GetTimeSeconds() - LocalLaunchTime) : 0.f;
}

AHypeWorldCharacter* ANinjaJutsuProjectile::GetCaster() const
{
	return Cast<AHypeWorldCharacter>(GetInstigator() ? GetInstigator() : GetOwner());
}

void ANinjaJutsuProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetupVisuals();
	if (HasAuthority() || bPrewarm)
	{
		OnRep_Launch();
	}
}

void ANinjaJutsuProjectile::SetupVisuals()
{
	const TPair<UStaticMeshComponent*, UMaterialInterface*> Shells[] = {
		{Core, CoreMaterial}, {Flames, FlameMaterial}, {OuterFlames, ShellMaterial ? ShellMaterial.Get() : OuterFlameMaterial.Get()}, {Heat, HeatMaterial}};
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
	// Ignition virou a cauda boca -> bola (o clarão da saída agora é o núcleo + halo + luz, sem placa)
	Ignition->SetStaticMesh(TailMesh);
	if (TailMaterial) { TailMID = UMaterialInstanceDynamic::Create(TailMaterial, this); Ignition->SetMaterial(0, TailMID); }
	Ignition->SetVisibility(false);
	for (UNiagaraComponent* Spiral : Spirals) { Spiral->SetAsset(SpiralSystem); }
	BodyFlames->SetAsset(BodySystem);
	Heat->SetStaticMesh(IgnitionMesh);
	Halo->SetStaticMesh(GlowMesh ? GlowMesh.Get() : IgnitionMesh.Get());
	GroundWave->SetStaticMesh(IgnitionMesh);
	Halo->SetVisibility(false);
	GroundWave->SetVisibility(false);
	if (HaloMaterial) { HaloMID = UMaterialInstanceDynamic::Create(HaloMaterial, this); Halo->SetMaterial(0, HaloMID); }
	if (RibbonMesh && RibbonMaterial)
	{
		// Sem a onda de chão antiga, o GroundWave vira a fita espiral do voo
		GroundWave->SetStaticMesh(RibbonMesh);
		GroundWave->SetMaterial(0, RibbonMaterial);
		GroundWave->TranslucencySortPriority = 2;
	}
	else if (ShockwaveMaterial) { WaveMID = UMaterialInstanceDynamic::Create(ShockwaveMaterial, this); GroundWave->SetMaterial(0, WaveMID); }
	if (GroundGlowMaterial) { GlowMID = UMaterialInstanceDynamic::Create(GroundGlowMaterial, this); GroundGlow->SetDecalMaterial(GlowMID); }
	Light->SetLightColor(LightColor);
	Light->SetAttenuationRadius(LightRadius);

	if (FireballMesh && FireballMaterial)
	{
		FireBody->SetStaticMesh(FireballMesh);
		UMaterialInstanceDynamic* BodyMID = UMaterialInstanceDynamic::Create(FireballMaterial, this);
		BodyMID->SetScalarParameterValue(TEXT("Seed"), float(Launch.VisualSeed % 997) / 97.f);
		FireBody->SetMaterial(0, BodyMID);
	}
	if (TongueMesh && TongueMaterial && GetNetMode() != NM_DedicatedServer)
	{
		for (int32 Index = 0; Index < TongueCount + InnerTongueCount; ++Index)
		{
			FTongue& Tongue = Tongues.AddDefaulted_GetRef();
			Tongue.bInner = Index >= TongueCount && InnerTongueMaterial;
			UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
			Mesh->SetStaticMesh(TongueMesh);
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetCastShadow(false);
			Mesh->bReceivesDecals = false;
			Mesh->SetCanEverAffectNavigation(false);
			Mesh->TranslucencySortPriority = Tongue.bInner ? 1 : 0;   // línguas amarelas de dentro por cima do corpo
			Mesh->SetupAttachment(Root);
			Mesh->RegisterComponent();
			Mesh->SetVisibility(false);
			Tongue.MID = UMaterialInstanceDynamic::Create(Tongue.bInner ? InnerTongueMaterial.Get() : TongueMaterial.Get(), this);
			Tongue.MID->SetScalarParameterValue(TEXT("Seed"), FMath::FRand() * 10.f);
			Mesh->SetMaterial(0, Tongue.MID);
			Tongue.Mesh = Mesh;
			RespawnTongue(Tongue, true);
		}
	}
}

void ANinjaJutsuProjectile::RespawnTongue(FTongue& Tongue, bool bStagger)
{
	// Nasce na base/interior: as de fora num anel mais largo e menores, as do centro maiores
	const float Spread = Tongue.bInner ? 0.15f : 0.5f;
	const float R01 = FMath::Sqrt(FMath::FRand());
	const float Angle = FMath::FRand() * UE_TWO_PI;
	Tongue.Pos = FVector(FMath::FRandRange(-0.3f, 0.25f), FMath::Cos(Angle) * R01 * Spread, FMath::Sin(Angle) * R01 * Spread * 0.5f - 0.45f);
	Tongue.Vel = FVector(-TongueDrag * FMath::FRandRange(0.7f, 1.2f), FMath::FRandRange(-0.25f, 0.25f), TongueRise * FMath::FRandRange(0.8f, 1.2f));
	Tongue.Life = FMath::FRandRange(TongueLife.X, TongueLife.Y) * (Tongue.bInner ? 1.3f : 1.f);
	Tongue.Height = (Tongue.bInner ? 1.3f : FMath::Lerp(1.5f, 0.7f, R01)) * FMath::FRandRange(0.8f, 1.2f);
	Tongue.Width = (Tongue.bInner ? 0.26f : FMath::Lerp(0.62f, 0.38f, R01)) * FMath::FRandRange(0.85f, 1.15f);
	Tongue.Phase = FMath::FRand() * 10.f;
	Tongue.Age = bStagger ? FMath::FRand() * Tongue.Life : 0.f;
}

void ANinjaJutsuProjectile::UpdateTongues(float DeltaSeconds, float Radius, float Visible)
{
	for (FTongue& Tongue : Tongues)
	{
		Tongue.Age += DeltaSeconds;
		if (Tongue.Age > Tongue.Life) { RespawnTongue(Tongue, false); }
		const float X = Tongue.Age / Tongue.Life;
		// Empuxo para cima acelerando + enrolar leve (curl barato: velocidade lateral girando com a fase)
		Tongue.Vel.Z += TongueRise * 1.5f * DeltaSeconds;
		const float Curl = Tongue.Phase + Tongue.Age * 11.f;
		Tongue.Pos += (Tongue.Vel + FVector(0.f, FMath::Cos(Curl), FMath::Sin(Curl) * 0.3f) * 0.6f) * DeltaSeconds;
		// Eixo da língua = direção da velocidade (sobe e deita para trás com o voo)
		const FVector Dir = (Tongue.Vel + FVector(0.f, 0.f, TongueRise * 0.6f)).GetSafeNormal();
		const float Grow = FMath::SmoothStep(0.f, 0.3f, X);
		const float Height = Tongue.Height * Radius * FMath::Lerp(0.45f, 1.f, Grow) * (1.f + 0.35f * X) * Visible;
		const float Width = Tongue.Width * Radius * FMath::Lerp(0.6f, 1.f, Grow) * (1.f - 0.45f * X) * Visible;
		UStaticMeshComponent* Mesh = Tongue.Mesh;
		Mesh->SetVisibility(Visible > 0.01f);
		const FQuat Wobble(Dir, Tongue.bInner ? 0.6f * FMath::Sin(Tongue.Age * 14.f + Tongue.Phase) : 0.f);   // as de dentro ondulam
		Mesh->SetRelativeLocationAndRotation(Tongue.Pos * Radius, Wobble * FRotationMatrix::MakeFromZ(Dir).ToQuat());
		Mesh->SetRelativeScale3D(FVector(Width / 40.f, Width / 40.f, FMath::Max(Height, 1.f) / 100.f));
		Tongue.MID->SetScalarParameterValue(TEXT("Erosion"), FMath::SmoothStep(0.4f, 1.f, X));
	}
}

void ANinjaJutsuProjectile::HideTongues()
{
	for (FTongue& Tongue : Tongues)
	{
		if (Tongue.Mesh) { Tongue.Mesh->SetVisibility(false); }
	}
}

void ANinjaJutsuProjectile::OnRep_Launch()
{
	if (bLaunched || !Launch.Jutsu)
	{
		return;
	}
	bLaunched = true;
	// O cliente recebe a bola já atrasada: recupera um pouco na hora e o resto aos poucos, sem pular nem ficar parado
	const float Lag = FMath::Max(0.f, float(Now() - Launch.StartTime));
	const float CatchUpNow = FMath::Min(Lag, MaxCatchUp);
	CatchUpDebt = FMath::Min(Lag - CatchUpNow, 0.3f);   // atraso maior é erro do relógio: não dispara a bola
	LocalLaunchTime = GetWorld()->GetTimeSeconds() - CatchUpNow;
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
	// Velocidade sobe de LaunchSpeedScale*Speed até Speed em AccelTime (smoothstep) e não para nunca;
	// a distância é a integral exata dessa velocidade
	const float Accel = FMath::Max(AccelTime, 0.01f);
	const float A = FMath::Clamp(LaunchSpeedScale, 0.f, 1.f);
	const float U = FMath::Clamp(T / Accel, 0.f, 1.f);
	float Distance = Speed * Accel * (A * U + (1.f - A) * (FMath::Cube(U) - 0.5f * FMath::Square(FMath::Square(U))))
		+ Speed * FMath::Max(T - Accel, 0.f);
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
	Flames->SetVisibility(Tongues.Num() == 0);   // com línguas não há esfera de corpo
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
	// Saída (0-0,15 s): só luz (núcleo estourado + halo enorme + luz forte); depois a bola estica em gota -> coluna
	const float Flash = 1.f - FMath::SmoothStep(0.f, 0.15f, T);
	// X = direção do voo; um pouco de Z para a chama "subir" (vista de cima e de trás, vira gota/coluna na tela)
	const float StretchIn = FMath::SmoothStep(0.12f, 0.5f, T);
	const FVector Stretch(1.f + FlightStretch * StretchIn, 1.f, 1.f + 0.25f * StretchIn);
	Heat->SetVisibility(Grow > 0.5f);
	const float InnerScale = BodySystem ? 0.85f : 1.f;
	Core->SetRelativeScale3D(Stretch * Base * ShellScales.X * InnerScale * (1.f + 0.3f * Flash));
	Flames->SetRelativeScale3D(Stretch * Base * ShellScales.Y * InnerScale);
	Flames->SetRelativeLocation(FVector(-Radius * 0.25f * (Stretch.X - 1.f), 0.f, 0.f));   // corpo puxado para trás: gota
	if (Tongues.Num() > 0)
	{
		// Núcleo pequeno e baixo, na base das línguas (a borda some por Fresnel no material)
		Core->SetRelativeLocation(FVector(0.f, 0.f, -Radius * 0.25f));
		UpdateTongues(GetWorld()->GetDeltaSeconds(), Radius, FMath::SmoothStep(0.04f, 0.18f, T));
	}
	if (FireballMesh && FireballMaterial)
	{
		// Corpo: chama oval inclinada para trás, girando devagar no próprio eixo vertical e pulsando
		const float BodyRadius = FMath::Max(FireballMesh->GetBounds().BoxExtent.X, 1.f);
		const float Pulse = 1.f + 0.05f * FMath::Sin(T * 13.f) + 0.03f * FMath::Sin(T * 29.f + 1.3f);
		FireBody->SetVisibility(true);
		FireBody->SetRelativeLocation(FVector(-Radius * 0.1f, 0.f, Radius * 0.3f));
		FireBody->SetRelativeRotation(FRotator(FireballLean, 0.f, 0.f).Quaternion() * FQuat(FVector::UpVector, FMath::DegreesToRadians(SpinAngle * 0.35f)));
		FireBody->SetRelativeScale3D(FVector(Radius * FireballScale / BodyRadius * Pulse * FMath::SmoothStep(0.02f, 0.16f, T)));
	}
	OuterFlames->SetRelativeScale3D(FVector(1.1f, 1.f, 1.f) * Base * ShellScales.Z);
	Heat->SetRelativeScale3D(FVector(1.5f, 1.f, 1.f) * Base * HeatScale);
	// O corpo e a casca giram em sentidos opostos (o material também corre o ruído)
	Flames->SetRelativeRotation(FRotator(0.f, 0.f, SpinAngle));
	OuterFlames->SetRelativeRotation(FRotator(0.f, 0.f, -SpinAngle * 0.7f));
	const bool bShell = ShellMaterial ? (T > float(ShellWindow.X) && T < float(ShellWindow.Y)) : !BodySystem;
	OuterFlames->SetVisibility(bShell);
	for (int32 Index = 0; Index < Spirals.Num(); ++Index)
	{
		const float Angle = T * 28.f + Index * UE_TWO_PI / 3.f;
		Spirals[Index]->SetRelativeLocation(FVector(-Radius * 0.35f, FMath::Cos(Angle) * Radius * 1.12f, FMath::Sin(Angle) * Radius * 1.12f));
		if (SpiralSystem && Grow > 0.5f && !Spirals[Index]->IsActive()) { Spirals[Index]->Activate(true); }
	}
	if (RibbonMesh && RibbonMaterial)
	{
		// Fita: anel inclinado em volta do corpo, girando em torno do eixo do voo (a inclinação balança: espiral)
		const float RibbonRadius = FMath::Max(RibbonMesh->GetBounds().BoxExtent.GetMax(), 1.f);
		// Quase deitada em relação ao voo: de trás e de cima aparece como arcos passando pela frente e por trás da bola
		GroundWave->SetVisibility(T > float(ShellWindow.Y) - 0.05f);
		// Com línguas: anel fino quase deitado, girando em volta do eixo vertical da coluna e subindo/descendo nela
		const bool bColumn = Tongues.Num() > 0;
		GroundWave->SetRelativeLocation(bColumn ? FVector(-Radius * 0.2f, 0.f, Radius * (0.25f + 0.3f * FMath::Sin(T * 7.f)))
			: FVector(-Radius * 0.15f * Stretch.X, 0.f, 0.f));
		const FQuat Spin(bColumn ? FVector::UpVector : FVector::ForwardVector, FMath::DegreesToRadians(SpinAngle * 2.2f));
		GroundWave->SetRelativeRotation(Spin * FRotator(18.f + 10.f * FMath::Sin(T * 9.f), 0.f, 0.f).Quaternion());
		GroundWave->SetRelativeScale3D(FVector(Radius * (bColumn ? 0.75f : 1.05f) / RibbonRadius) * (bColumn ? FVector(1.f, 1.f, 0.8f) : FVector(Stretch.X, 1.f, 2.f)));
	}
	// Cauda: cone com a base na bola e a ponta na boca, se desfazendo até TailTime
	const FVector ToMouth = FVector(Launch.Start) - Center;
	Ignition->SetVisibility(TailMesh && TailMID && T < TailTime && ToMouth.Size() > Radius);
	if (T < TailTime)
	{
		Ignition->SetWorldLocationAndRotation(Center, FRotationMatrix::MakeFromZ(ToMouth).Rotator());
		Ignition->SetWorldScale3D(FVector(Radius * 0.6f / 50.f, Radius * 0.6f / 50.f, ToMouth.Size() / 100.f));
		TailMID->SetScalarParameterValue(TEXT("Erosion"), FMath::SmoothStep(0.05f, TailTime, T));
	}
	FirePostProcess->Settings.BloomIntensity = 0.55f + 0.25f * Flash;
	FirePostProcess->bEnabled = true;
	for (UMaterialInstanceDynamic* MID : MIDs)
	{
		if (MID)
		{
			MID->SetScalarParameterValue(TEXT("Grow"), Grow);
			MID->SetScalarParameterValue(TEXT("Dissolve"), 0.f);
			MID->SetScalarParameterValue(TEXT("Erosion"), 0.f);
			MID->SetScalarParameterValue(TEXT("Radius"), Radius);
			MID->SetVectorParameterValue(TEXT("TravelDirection"), FLinearColor(Launch.Direction.X, Launch.Direction.Y, Launch.Direction.Z));
		}
	}
	if (MIDs.IsValidIndex(2) && MIDs[2])
	{
		// Casca: entra inteira e se rasga até sumir no fim da janela
		MIDs[2]->SetScalarParameterValue(TEXT("Erosion"), FMath::SmoothStep(float(ShellWindow.X) + 0.04f, float(ShellWindow.Y), T));
	}
	for (int32 Index = 0; Index < FMath::Min(MIDs.Num(), 3); ++Index)
	{
		if (MIDs[Index]) { MIDs[Index]->SetScalarParameterValue(TEXT("Intensity"), Index == 0 ? 1.f + 2.f * Flash : 1.f + Flash); }
	}
	UpdateSceneIntegration(Center, Radius, T, false);
	if (GlowMesh)
	{
		// Halo vermelho: esfera macia grande em volta (o brilho de anime); enorme no clarão da saída
		const float GlowRadius = FMath::Max(GlowMesh->GetBounds().BoxExtent.GetMax(), 1.f);
		// Com línguas: halo alto (acompanha a coluna de chamas) e sem esticar para a frente
		const bool bColumn = Tongues.Num() > 0;
		const FVector HaloShape = bColumn ? FVector(1.f, 1.f, 1.45f) : Stretch;
		Halo->SetRelativeLocation(FVector(bColumn ? -Radius * 0.3f : 0.f, 0.f, bColumn ? Radius * 0.35f : 0.f));
		Halo->SetRelativeRotation(FRotator::ZeroRotator);
		Halo->SetRelativeScale3D(HaloShape * Radius * FMath::Lerp(bColumn ? 1.4f : 1.9f, bColumn ? 1.9f : 2.7f, Flash) / GlowRadius);
		Halo->SetVisibility(HaloMID != nullptr);
		if (HaloMID) { HaloMID->SetScalarParameterValue(TEXT("Intensity"), 1.f + 1.5f * Flash); }
	}
	const float Flicker = 1.f + .065f * FMath::Sin(T * 41.f) + .035f * FMath::Sin(T * 73.f);
	Light->SetIntensity(LightIntensity * Flicker * FMath::Lerp(0.5f, 1.f, Grow) * (1.f + 1.2f * Flash));
	if (!Trail->IsActive() && TrailSystem)
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
		if (!RibbonMesh) { GroundWave->SetVisibility(bImpact && ShockwaveMaterial && T < .6f); }
		if (bImpact && !RibbonMesh)
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
		if (CatchUpDebt > 0.f)
		{
			// Corre 1,5x até alcançar o tempo do servidor
			const float Recover = FMath::Min(CatchUpDebt, DeltaSeconds * 0.5f);
			LocalLaunchTime -= Recover;
			CatchUpDebt -= Recover;
		}
		const float T = GetFlightTime();
		FVector Center;
		float Radius;
		bool bReachedRange;
		Evaluate(FMath::Max(T, 0.f), Center, Radius, bReachedRange);
		ApplyBallVisual(Center, Radius, T);
		if (bPrewarm)
		{
			// Cópia de pré-carregamento: só visual, explode sozinha logo depois de sair
			if (T > 0.3f)
			{
				PlayExplosion(Center);
				SetLifeSpan(LingerTime + 0.5f);
			}
		}
		else if (HasAuthority())
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
	FireBody->SetVisibility(false);
	HideTongues();
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
				Decal->SetFadeIn(0.15f, 0.35f);   // aparece por baixo do clarão, sem o quadro cheio no instante do impacto
				Decal->SetFadeOut(ScorchHold, ScorchFade, false);
				Decal->SetFadeScreenSize(0.f);
			}
		}
	}

	// Tremida para quem está perto (cada jogador local)
	if (const UNinjaJutsuData* Jutsu = bPrewarm ? nullptr : Launch.Jutsu.Get())
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
