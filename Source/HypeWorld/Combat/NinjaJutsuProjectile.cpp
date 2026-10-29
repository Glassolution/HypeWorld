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
#include "Interfaces/Interface_PostProcessVolume.h"

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
	FirePostProcess->Settings.bOverride_AutoExposureBias = false;
	FirePostProcess->Settings.BloomThreshold = 2.f;
	for (UStaticMeshComponent* Glow : IgniteGlows) { Glow->SetVisibility(false); }
	for (const TPair<TObjectPtr<UStaticMeshComponent>, FVector>& Flame : IgniteFlames) { Flame.Key->SetVisibility(false); }
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
	Light->SetVisibility(LightIntensity > 0.f);   // sem luz própria (ex.: chamas do Hōsenka, que têm a luz do grupo)

	if (FireballMesh && FireballMaterial)
	{
		FireBody->SetStaticMesh(FireballMesh);
		UMaterialInstanceDynamic* BodyMID = UMaterialInstanceDynamic::Create(FireballMaterial, this);
		BodyMID->SetScalarParameterValue(TEXT("Seed"), float(Launch.VisualSeed % 997) / 97.f);
		FireBody->SetMaterial(0, BodyMID);
	}
	if (TongueMesh && TongueMaterial && GetNetMode() != NM_DedicatedServer)
	{
		auto MakePiece = [this](UStaticMesh* Mesh, UMaterialInterface* Material, int32 Priority)
		{
			UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this);
			Comp->SetStaticMesh(Mesh);
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Comp->SetCastShadow(false);
			Comp->bReceivesDecals = false;
			Comp->SetCanEverAffectNavigation(false);
			Comp->TranslucencySortPriority = Priority;
			Comp->SetupAttachment(Root);
			Comp->RegisterComponent();
			Comp->SetVisibility(false);
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Material, this);
			MID->SetScalarParameterValue(TEXT("Seed"), FMath::FRand() * 10.f);
			Comp->SetMaterial(0, MID);
			return TPair<UStaticMeshComponent*, UMaterialInstanceDynamic*>(Comp, MID);
		};
		// Ordem de desenho: lóbulos (corpo opaco) < línguas < línguas amarelas de dentro < silhuetas escuras
		const int32 Lobes = GlowMesh && LobeMaterial ? LobeCount : 0;
		const int32 Inner = InnerTongueMaterial ? InnerTongueCount : 0;
		const int32 Dark = DarkTongueMaterial ? DarkTongueCount : 0;
		for (int32 Index = 0; Index < Lobes + TongueCount + Inner + Dark; ++Index)
		{
			FTongue& Tongue = Tongues.AddDefaulted_GetRef();
			Tongue.bLobe = Index < Lobes;
			Tongue.bInner = !Tongue.bLobe && Index >= Lobes + TongueCount && Index < Lobes + TongueCount + Inner;
			Tongue.bDark = Index >= Lobes + TongueCount + Inner;
			UMaterialInterface* Material = Tongue.bLobe ? LobeMaterial.Get() : Tongue.bInner ? InnerTongueMaterial.Get()
				: Tongue.bDark ? DarkTongueMaterial.Get() : TongueMaterial.Get();
			const auto Piece = MakePiece(Tongue.bLobe ? GlowMesh.Get() : TongueMesh.Get(), Material,
				Tongue.bLobe ? 0 : Tongue.bInner ? 2 : Tongue.bDark ? 3 : 1);
			Tongue.Mesh = Piece.Key;
			Tongue.MID = Piece.Value;
			RespawnTongue(Tongue, true);
		}
		if (IgnitionRayMaterial)
		{
			for (int32 Index = 0; Index < IgnitionRays; ++Index)
			{
				// Raios num leque em volta da direção do voo (mais para os lados e para cima)
				const float Angle = (Index + FMath::FRandRange(-0.3f, 0.3f)) * UE_TWO_PI / FMath::Max(IgnitionRays, 1);
				const FVector Dir = FVector(FMath::FRandRange(0.1f, 0.6f), FMath::Cos(Angle), FMath::Sin(Angle) * 0.8f + 0.2f).GetSafeNormal();
				Rays.Add(TPair<TObjectPtr<UStaticMeshComponent>, FVector>(MakePiece(TongueMesh, IgnitionRayMaterial, 4).Key, Dir));
			}
		}
	}
	// Ignição: brilho que engole o personagem (desenhado por cima de tudo) e chamas subindo em coluna
	if (GetNetMode() != NM_DedicatedServer)
	{
		auto MakeIgnite = [this](UStaticMesh* Mesh, UMaterialInterface* Material, int32 Priority)
		{
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
			return Comp;
		};
		if (MouthBurstTongues > 0 && TongueMesh && TongueMaterial)
		{
			// Rajada na boca: lóbulos atrás, línguas, línguas claras do miolo na frente (direção no espaço do lançamento)
			const int32 Lobes = GlowMesh && LobeMaterial ? MouthBurstLobes : 0;
			const int32 Inner = InnerTongueMaterial ? FMath::Min(MouthBurstInner, MouthBurstTongues) : 0;
			for (int32 Index = 0; Index < Lobes + MouthBurstTongues; ++Index)
			{
				FBurstFlame& Flame = BurstFlames.AddDefaulted_GetRef();
				Flame.bLobe = Index < Lobes;
				const bool bInner = !Flame.bLobe && Index >= Lobes + MouthBurstTongues - Inner;
				UMaterialInterface* Material = Flame.bLobe ? LobeMaterial.Get() : bInner ? InnerTongueMaterial.Get() : TongueMaterial.Get();
				Flame.Mesh = MakeIgnite(Flame.bLobe ? GlowMesh.Get() : TongueMesh.Get(), Material, Flame.bLobe ? 9 : bInner ? 12 : 10);
				Flame.MID = Cast<UMaterialInstanceDynamic>(Flame.Mesh->GetMaterial(0));
				// Leque aberto para os lados e para cima (de trás do personagem as línguas aparecem de perfil, não de ponta)
				const float Spread = bInner ? 0.35f : 1.1f;
				Flame.Dir = FVector(bInner ? 0.7f : 1.f, FMath::FRandRange(-Spread, Spread),
					bInner ? FMath::FRandRange(0.6f, 1.f) : FMath::FRandRange(0.15f, 0.8f)).GetSafeNormal();
				Flame.Size = bInner ? FMath::FRandRange(0.75f, 0.9f) : FMath::FRandRange(0.7f, 1.15f);
				Flame.Delay = bInner ? 0.f : FMath::FRandRange(0.f, 0.03f);
				Flame.Reach = FMath::FRandRange(0.7f, 1.1f);
			}
		}
		if (GlowMesh && IgniteHaloMaterial) { IgniteGlows.Add(MakeIgnite(GlowMesh, IgniteHaloMaterial, 10)); }
		if (GlowMesh && IgniteGlowMaterial) { IgniteGlows.Add(MakeIgnite(GlowMesh, IgniteGlowMaterial, 11)); }
		if (TongueMesh && IgniteFlameMaterial)
		{
			for (int32 Index = 0; Index < IgniteFlameCount; ++Index)
			{
				// x/y = deslocamento em volta da boca (x raio do brilho), z = tamanho relativo
				const float Angle = FMath::FRand() * UE_TWO_PI;
				const float Dist = Index == 0 ? 0.f : FMath::Sqrt(FMath::FRand()) * 0.35f;
				IgniteFlames.Add(TPair<TObjectPtr<UStaticMeshComponent>, FVector>(MakeIgnite(TongueMesh, IgniteFlameMaterial, 9),
					FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, FMath::FRandRange(0.7f, 1.15f))));
			}
		}
		// Exposição do mapa (volume sem limite de maior prioridade): o +1 EV da ignição soma em cima dela
		float BestPriority = -FLT_MAX;
		for (IInterface_PostProcessVolume* Volume : GetWorld()->PostProcessVolumes)
		{
			const FPostProcessVolumeProperties Props = Volume ? Volume->GetProperties() : FPostProcessVolumeProperties();
			if (Volume && Props.bIsEnabled && Props.bIsUnbound && Props.Settings && Props.Settings->bOverride_AutoExposureBias
				&& Props.Priority > BestPriority && Props.Settings != &FirePostProcess->Settings)
			{
				BestPriority = Props.Priority;
				BaseExposure = Props.Settings->AutoExposureBias;
			}
		}
	}
}

void ANinjaJutsuProjectile::RespawnTongue(FTongue& Tongue, bool bStagger)
{
	const float R01 = FMath::Sqrt(FMath::FRand());
	const float Angle = FMath::FRand() * UE_TWO_PI;
	Tongue.Phase = FMath::FRand() * 10.f;
	// Leque da ignição: para a frente, aberto para os lados e um pouco para cima
	Tongue.Fan = FVector(1.f, FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-0.45f, 0.75f)).GetSafeNormal();
	if (Tongue.bLobe)
	{
		// Lóbulo: bolha de chama grande no miolo da coluna, sobe devagar, incha e se renova
		Tongue.Pos = FVector(FMath::FRandRange(-0.35f, 0.15f), FMath::Cos(Angle) * R01 * 0.35f, FMath::FRandRange(-0.35f, 0.45f));
		Tongue.Vel = FVector(-TongueDrag * 0.4f, 0.f, TongueRise * FMath::FRandRange(0.25f, 0.45f));
		Tongue.Life = FMath::FRandRange(0.45f, 0.75f);
		Tongue.Height = FMath::FRandRange(0.3f, 0.45f);   // raio do lóbulo (x raio da bola)
		Tongue.Age = bStagger ? FMath::FRand() * Tongue.Life : 0.f;
		return;
	}
	// Língua: nasce na base/interior; as de fora num anel mais largo e menores, as do centro maiores
	const float Spread = Tongue.bInner || Tongue.bDark ? 0.18f : 0.5f;
	Tongue.Pos = FVector(FMath::FRandRange(-0.3f, 0.25f), FMath::Cos(Angle) * R01 * Spread, FMath::Sin(Angle) * R01 * Spread * 0.5f - 0.45f);
	Tongue.Vel = FVector(-TongueDrag * FMath::FRandRange(0.7f, 1.2f), FMath::FRandRange(-0.25f, 0.25f), TongueRise * FMath::FRandRange(0.8f, 1.2f));
	Tongue.Life = FMath::FRandRange(TongueLife.X, TongueLife.Y) * (Tongue.bInner || Tongue.bDark ? 1.3f : 1.f);
	Tongue.Height = (Tongue.bInner ? 1.3f : Tongue.bDark ? 1.1f : FMath::Lerp(1.5f, 0.7f, R01)) * FMath::FRandRange(0.8f, 1.2f);
	Tongue.Width = (Tongue.bInner ? 0.26f : Tongue.bDark ? 0.3f : FMath::Lerp(0.62f, 0.38f, R01)) * FMath::FRandRange(0.85f, 1.15f);
	Tongue.Age = bStagger ? FMath::FRand() * Tongue.Life : 0.f;
}

void ANinjaJutsuProjectile::UpdateTongues(float DeltaSeconds, float Radius, float Visible, float Burst)
{
	for (FTongue& Tongue : Tongues)
	{
		Tongue.Age += DeltaSeconds;
		if (Tongue.Age > Tongue.Life) { RespawnTongue(Tongue, false); }
		const float X = Tongue.Age / Tongue.Life;
		UStaticMeshComponent* Mesh = Tongue.Mesh;
		Mesh->SetVisibility(Visible > 0.01f);
		if (Tongue.bLobe)
		{
			// Lóbulo: incha rápido, gira e encolhe um pouco antes de se renovar (sempre há vários cheios)
			Tongue.Pos += Tongue.Vel * DeltaSeconds;
			const float Size = Tongue.Height * Radius * FMath::Lerp(0.55f, 1.f, FMath::SmoothStep(0.f, 0.35f, X)) * FMath::Lerp(1.f, 0.7f, FMath::SmoothStep(0.6f, 1.f, X)) * Visible;
			const float GlowRadius = FMath::Max(GlowMesh->GetBounds().BoxExtent.GetMax(), 1.f);
			Mesh->SetRelativeLocationAndRotation((Tongue.Pos + Tongue.Fan * 0.6f * Burst) * Radius, FRotator(Tongue.Phase * 30.f, Tongue.Age * 120.f + Tongue.Phase * 36.f, 0.f));
			Tongue.MID->SetScalarParameterValue(TEXT("Intensity"), 1.f + 1.8f * Burst);
			Mesh->SetRelativeScale3D(FVector(Size / GlowRadius) * FVector(1.f, 1.f, 1.15f));
			Tongue.MID->SetScalarParameterValue(TEXT("Erosion"), FMath::SmoothStep(0.7f, 1.f, X) * 0.6f);
			continue;
		}
		// Empuxo para cima acelerando + enrolar leve (curl barato: velocidade lateral girando com a fase)
		Tongue.Vel.Z += TongueRise * 1.5f * DeltaSeconds;
		const float Curl = Tongue.Phase + Tongue.Age * 11.f;
		Tongue.Pos += (Tongue.Vel + FVector(0.f, FMath::Cos(Curl), FMath::Sin(Curl) * 0.3f) * 0.6f) * DeltaSeconds;
		// Eixo da língua = direção da velocidade (sobe e deita para trás com o voo)
		// Na ignição as línguas apontam no leque (rajada saindo da boca) e giram aos poucos para cima, virando a bola
		const FVector Dir = FMath::Lerp((Tongue.Vel + FVector(0.f, 0.f, TongueRise * 0.6f)).GetSafeNormal(), Tongue.Fan, Burst).GetSafeNormal();
		const float Grow = FMath::SmoothStep(0.f, 0.3f, X);
		const float Height = Tongue.Height * Radius * FMath::Lerp(0.45f, 1.f, Grow) * (1.f + 0.35f * X) * Visible;
		const float Width = Tongue.Width * Radius * FMath::Lerp(0.6f, 1.f, Grow) * (1.f - 0.45f * X) * Visible;
		const bool bWave = Tongue.bInner || Tongue.bDark;
		const FQuat Wobble(Dir, bWave ? 0.6f * FMath::Sin(Tongue.Age * 14.f + Tongue.Phase) : 0.f);   // as de dentro ondulam
		Mesh->SetRelativeLocationAndRotation((Tongue.Pos + Tongue.Fan * 0.75f * Burst) * Radius, Wobble * FRotationMatrix::MakeFromZ(Dir).ToQuat());
		Tongue.MID->SetScalarParameterValue(TEXT("Intensity"), 1.f + 1.8f * Burst);
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
	for (const TPair<TObjectPtr<UStaticMeshComponent>, FVector>& Ray : Rays)
	{
		Ray.Key->SetVisibility(false);
	}
}

void ANinjaJutsuProjectile::OnRep_Launch()
{
	if (bLaunched || !Launch.Jutsu)
	{
		return;
	}
	bLaunched = true;
	// Velocidade/alcance do Data Asset (mesmo valor em todas as máquinas: o Jutsu vem na saída replicada)
	if (Launch.Jutsu->ProjectileSpeed > 0.f) { Speed = Launch.Jutsu->ProjectileSpeed; }
	if (Launch.Jutsu->ProjectileRange > 0.f) { Range = Launch.Jutsu->ProjectileRange; }
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
	if (LaunchBend > 0.f)
	{
		// Sai da órbita fazendo uma curva: desvia para o lado e volta para a linha da mira
		const FVector Side = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal() * ((Launch.VisualSeed & 1) ? 1.f : -1.f);
		OutCenter += (Side + FVector(0.f, 0.f, 0.35f)) * LaunchBend * FMath::Sin(UE_PI * FMath::Clamp(T / FMath::Max(BendTime, 0.01f), 0.f, 1.f));
	}
}

void ANinjaJutsuProjectile::UpdateIgnition(float T)
{
	// Envelope: sobe em 0,05 s, segura até 0,1 s e cai até 0,3 s (por baixo a bola já está se formando)
	const float Env = FMath::SmoothStep(0.f, 0.05f, T) * (1.f - FMath::SmoothStep(0.1f, 0.3f, T));
	const FVector Mouth = FVector(Launch.Start) + FVector(0.f, 0.f, 40.f);
	const float GlowRadius = GlowMesh ? FMath::Max(GlowMesh->GetBounds().BoxExtent.GetMax(), 1.f) : 50.f;
	for (int32 Index = 0; Index < IgniteGlows.Num(); ++Index)
	{
		// 0 = halo grande (laranja -> vermelho-rosado), 1 = miolo branco-amarelado; os dois por cima do personagem
		UStaticMeshComponent* Glow = IgniteGlows[Index];
		Glow->SetVisibility(Env > 0.01f);
		if (Env <= 0.01f) { continue; }
		const float Size = IgniteRadius * (Index == 0 ? 1.f : 0.55f) * FMath::Lerp(0.6f, 1.f, FMath::SmoothStep(0.f, 0.08f, T));
		Glow->SetWorldLocationAndRotation(Mouth + FVector(0.f, 0.f, Index == 0 ? 60.f : 20.f), FRotator(0.f, T * 200.f, 0.f));
		Glow->SetWorldScale3D(FVector(Size / GlowRadius) * FVector(1.f, 1.f, 1.2f));
		if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Glow->GetMaterial(0)))
		{
			MID->SetScalarParameterValue(TEXT("Intensity"), Env);
		}
	}
	// Chamas da ignição: sobem com força da boca, formando uma coluna de IgniteColumn em ~0,3 s, e se desfazem
	const float Rise = FMath::SmoothStep(0.f, 0.3f, T);
	const float FlameEnv = FMath::SmoothStep(0.f, 0.04f, T) * (1.f - FMath::SmoothStep(0.2f, 0.4f, T));
	for (const TPair<TObjectPtr<UStaticMeshComponent>, FVector>& Flame : IgniteFlames)
	{
		Flame.Key->SetVisibility(FlameEnv > 0.01f);
		if (FlameEnv <= 0.01f) { continue; }
		const FVector Info = Flame.Value;
		const FVector Base = Mouth + FVector(Info.X, Info.Y, 0.f) * IgniteRadius + FVector(0.f, 0.f, 120.f * Rise - 150.f);
		const float Height = IgniteColumn * Info.Z * FMath::Lerp(0.25f, 1.f, Rise);
		const float Width = IgniteRadius * 0.2f * (1.15f - 0.4f * Rise) / Info.Z;
		const FVector Up = FVector(Info.X * 0.6f, Info.Y * 0.6f, 1.f).GetSafeNormal();
		Flame.Key->SetWorldLocationAndRotation(Base, FRotationMatrix::MakeFromZ(Up).Rotator() + FRotator(0.f, T * 150.f, 0.f));
		Flame.Key->SetWorldScale3D(FVector(Width / 40.f, Width / 40.f, Height / 100.f));
		if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Flame.Key->GetMaterial(0)))
		{
			MID->SetScalarParameterValue(TEXT("Intensity"), FlameEnv);
			MID->SetScalarParameterValue(TEXT("Erosion"), FMath::SmoothStep(0.2f, 0.5f, T));
		}
	}
	// Blendable temporário só da ignição: +1 EV sobre a exposição do mapa, bloom mais forte e limiar menor
	FirePostProcess->bUnbound = true;
	FirePostProcess->Settings.bOverride_AutoExposureBias = Env > 0.001f;
	FirePostProcess->Settings.AutoExposureBias = BaseExposure + IgnitionExposure * Env;
	FirePostProcess->Settings.bOverride_SceneColorTint = false;
	FirePostProcess->Settings.BloomIntensity = FMath::Lerp(0.55f, IgnitionBloom, Env);
	FirePostProcess->Settings.BloomThreshold = FMath::Lerp(2.f, 0.6f, Env);
	// Sem ignição (só bloom normal e sem exposição): nenhum pós-processo de tela por projétil (rajadas baratas)
	if (IgnitionBloom <= 0.55f && IgnitionExposure <= 0.f) { FirePostProcess->bEnabled = false; }
}

void ANinjaJutsuProjectile::UpdateMouthBurst(float T)
{
	if (BurstFlames.Num() == 0) { return; }
	const FQuat Frame = FVector(Launch.Direction).ToOrientationQuat();
	const float Time = FMath::Max(MouthBurstTime, 0.02f);
	const float GlowRadius = GlowMesh ? FMath::Max(GlowMesh->GetBounds().BoxExtent.GetMax(), 1.f) : 50.f;
	for (const FBurstFlame& Flame : BurstFlames)
	{
		// Sai da boca já grande, avança no cone, estica e se rasga até sumir em Time (a cauda e a bola seguem dali)
		const float U = (T - Flame.Delay) / Time;
		Flame.Mesh->SetVisibility(U > 0.f && U < 1.f);
		if (U <= 0.f || U >= 1.f) { continue; }
		const FVector Dir = Frame.RotateVector(Flame.Dir);
		const float Push = 1.f - FMath::Square(1.f - U);
		const float Grow = FMath::SmoothStep(0.f, 0.25f, U);
		const float Size = MouthBurstSize * Flame.Size;
		if (Flame.bLobe)
		{
			const float LobeSize = Size * 0.6f * FMath::Lerp(0.6f, 1.f, Grow) * (1.f + 0.3f * U);
			Flame.Mesh->SetWorldLocationAndRotation(FVector(Launch.Start) + Dir * MouthBurstReach * Flame.Reach * 0.45f * Push,
				FRotator(Flame.Reach * 90.f, T * 300.f + Flame.Size * 200.f, 0.f));
			Flame.Mesh->SetWorldScale3D(FVector(LobeSize / GlowRadius) * FVector(1.25f, 1.f, 1.f));
		}
		else
		{
			const float Height = Size * FMath::Lerp(0.8f, 2.2f, Grow) * (1.f + 0.4f * U);
			const float Width = Size * 0.45f * (1.f - 0.35f * U);
			const FQuat Roll(Dir, Flame.Size * 7.f + T * 6.f);
			Flame.Mesh->SetWorldLocationAndRotation(FVector(Launch.Start) + Dir * MouthBurstReach * Flame.Reach * Push * 0.6f,
				Roll * FRotationMatrix::MakeFromZ(Dir).ToQuat());
			Flame.Mesh->SetWorldScale3D(FVector(Width / 40.f, Width / 40.f, Height / 100.f));
		}
		if (Flame.MID)
		{
			Flame.MID->SetScalarParameterValue(TEXT("Intensity"), 1.f + (MouthBurstIntensity - 1.f) * FMath::Pow(1.f - U, 1.5f));
			Flame.MID->SetScalarParameterValue(TEXT("Erosion"), FMath::SmoothStep(0.4f, 1.f, U));
		}
	}
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
	// Com línguas não há esfera de corpo (nem no voo nem na ignição)
	Flames->SetVisibility(Tongues.Num() == 0);
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
	Heat->SetVisibility(T > 0.45f);   // distorção só depois da ignição: o passe de distorção não soma no pico dela
	const float InnerScale = BodySystem ? 0.85f : 1.f;
	Core->SetRelativeScale3D(Stretch * Base * ShellScales.X * InnerScale * (1.f + 0.3f * Flash));
	Flames->SetRelativeScale3D(Stretch * Base * ShellScales.Y * InnerScale);
	Flames->SetRelativeLocation(FVector(-Radius * 0.25f * (Stretch.X - 1.f), 0.f, 0.f));   // corpo puxado para trás: gota
	if (Tongues.Num() > 0)
	{
		// Núcleo no centro da coluna (a borda some por Fresnel no material); brilho laranja radial grande na ignição
		Core->SetRelativeLocation(FVector(0.f, 0.f, Radius * 0.05f));
		Flames->SetRelativeLocation(FVector::ZeroVector);
		Flames->SetRelativeScale3D(FVector(Base * 1.2f * (0.6f + 0.4f * Flash)));
		// Ignição = começo da bola: as mesmas chamas nascem na boca já grandes, em leque e superexpostas, e encolhem para a bola
		UpdateTongues(GetWorld()->GetDeltaSeconds(), Radius, FMath::SmoothStep(0.06f, 0.25f, T), 0.f);
	}
	if (FireballMesh && FireballMaterial)
	{
		// Corpo: chama oval inclinada para trás, girando devagar no próprio eixo vertical e pulsando
		const float BodyRadius = FMath::Max(FireballMesh->GetBounds().BoxExtent.X, 1.f);
		const float Pulse = 1.f + 0.05f * FMath::Sin(T * 13.f) + 0.03f * FMath::Sin(T * 29.f + 1.3f);
		FireBody->SetVisibility(true);
		FireBody->SetRelativeLocation(FVector(-Radius * 0.1f, 0.f, Radius * 0.3f));
		FireBody->SetRelativeRotation(FRotator(FireballLean, 0.f, 0.f).Quaternion() * FQuat(FVector::UpVector, FMath::DegreesToRadians(SpinAngle * 0.35f)));
		FireBody->SetRelativeScale3D(FVector(Radius * FireballScale / BodyRadius * Pulse * FMath::SmoothStep(0.04f, 0.2f, T)));
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
	// Núcleo apagado no clarão (sem disco branco liso): a luz da ignição vem das próprias chamas
	for (int32 Index = 0; Index < FMath::Min(MIDs.Num(), 3); ++Index)
	{
		if (MIDs[Index]) { MIDs[Index]->SetScalarParameterValue(TEXT("Intensity"), Index == 0 ? 1.f - 0.9f * Flash : 1.f + Flash); }
	}
	if (Tongues.Num() > 0 && MIDs.IsValidIndex(1) && MIDs[1])
	{
		MIDs[1]->SetScalarParameterValue(TEXT("Intensity"), 2.f * Flash);   // brilho radial da ignição apagando
	}
	// Raios finos da ignição: disparam do centro e somem com o clarão
	for (const TPair<TObjectPtr<UStaticMeshComponent>, FVector>& Ray : Rays)
	{
		Ray.Key->SetVisibility(Flash > 0.02f);
		if (Flash > 0.02f)
		{
			const float Len = Radius * FMath::Lerp(1.2f, 2.6f, FMath::SmoothStep(0.f, 0.1f, T));
			Ray.Key->SetRelativeLocationAndRotation(Ray.Value * Radius * 0.2f, FRotationMatrix::MakeFromZ(Ray.Value).ToQuat());
			Ray.Key->SetRelativeScale3D(FVector(0.09f, 0.09f, Len / 100.f));
			if (UMaterialInstanceDynamic* RayMID = Cast<UMaterialInstanceDynamic>(Ray.Key->GetMaterial(0)))
			{
				RayMID->SetScalarParameterValue(TEXT("Intensity"), Flash);
			}
		}
	}
	UpdateSceneIntegration(Center, Radius, T, false);
	if (GlowMesh)
	{
		// Halo vermelho: esfera macia grande em volta (o brilho de anime); enorme no clarão da saída
		const float GlowRadius = FMath::Max(GlowMesh->GetBounds().BoxExtent.GetMax(), 1.f);
		// Com línguas: halo alto (acompanha a coluna de chamas) e sem esticar para a frente
		const bool bColumn = Tongues.Num() > 0;
		// na ignição o halo é redondo e grande (brilho radial), depois vira a forma alta da coluna
		const FVector HaloShape = FMath::Lerp(bColumn ? FVector(1.f, 1.f, 1.45f) : Stretch, FVector(1.f), Flash);
		Halo->SetRelativeLocation(FVector(bColumn ? -Radius * 0.3f : 0.f, 0.f, bColumn ? Radius * 0.35f : 0.f));
		Halo->SetRelativeRotation(FRotator::ZeroRotator);
		const float HaloIn = HaloFadeIn > 0.f ? FMath::SmoothStep(0.f, HaloFadeIn, T) : 1.f;
		Halo->SetRelativeScale3D(HaloShape * Radius * HaloScale * HaloIn * FMath::Lerp(bColumn ? 1.4f : 1.9f, bColumn ? 2.1f : 2.7f, Flash) / GlowRadius);
		Halo->SetVisibility(HaloMID != nullptr);
		if (HaloMID) { HaloMID->SetScalarParameterValue(TEXT("Intensity"), 1.f + 1.5f * Flash); }
	}
	const float Flicker = 1.f + .065f * FMath::Sin(T * 41.f) + .035f * FMath::Sin(T * 73.f);
	// Pico da luz na ignição (tinge personagem, chão e cenário), caindo em IgnitionLightTime
	const float LightFlash = FMath::Square(1.f - FMath::SmoothStep(0.f, FMath::Max(IgnitionLightTime, 0.01f), T));
	Light->SetIntensity(LightIntensity * Flicker * FMath::Lerp(0.5f, 1.f, Grow) * (1.f + IgnitionLightBoost * LightFlash));
	Light->SetAttenuationRadius(FMath::Lerp(LightRadius, FMath::Max(IgniteLightRadius, LightRadius), LightFlash));
	UpdateIgnition(T);
	UpdateMouthBurst(T);
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
	for (const FBurstFlame& Flame : BurstFlames) { Flame.Mesh->SetVisibility(false); }
	for (UNiagaraComponent* Spiral : Spirals) { Spiral->Deactivate(); }
	FirePostProcess->Settings.BloomIntensity = 0.65f;
	// A explosão volta ao pós-processo de antes (só perto, sem exposição/tom da ignição)
	FirePostProcess->bUnbound = false;
	FirePostProcess->Settings.bOverride_SceneColorTint = false;
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
