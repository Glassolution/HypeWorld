#include "NinjaChakraFeetComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NinjaVisibleBody.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace NinjaFeet
{
	UMaterialInterface* LoadDefault(const TCHAR* Name)
	{
		return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Ninja/VFX/Materials/%s.%s"), Name, Name));
	}
}

UNinjaChakraFeetComponent::UNinjaChakraFeetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;   // depois da animação: os ossos dos pés já estão na pose do quadro
}

void UNinjaChakraFeetComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!ShellMaterial)  { ShellMaterial = NinjaFeet::LoadDefault(TEXT("M_ChakraFeetShell")); }
	if (!WispMaterial)   { WispMaterial = NinjaFeet::LoadDefault(TEXT("M_Chakra_FeetWisp")); }
	if (!GroundMaterial) { GroundMaterial = NinjaFeet::LoadDefault(TEXT("M_Chakra_FeetGround")); }
}

UNinjaChakraFeetComponent::FPart UNinjaChakraFeetComponent::MakePart(UMaterialInterface* Material)
{
	static UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	FPart Part;
	Part.Mesh = NewObject<UStaticMeshComponent>(GetOwner());
	Part.Mesh->SetStaticMesh(Plane);
	Part.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part.Mesh->SetCastShadow(false);
	Part.Mesh->SetUsingAbsoluteLocation(true);
	Part.Mesh->SetUsingAbsoluteRotation(true);
	Part.Mesh->SetUsingAbsoluteScale(true);
	Part.Mesh->SetupAttachment(this);
	Part.Mesh->RegisterComponent();
	Part.Mesh->SetVisibility(false);
	Keep.Add(Part.Mesh);
	if (Material)
	{
		Part.MID = UMaterialInstanceDynamic::Create(Material, this);
		Part.Mesh->SetMaterial(0, Part.MID);
		Keep.Add(Part.MID);
	}
	return Part;
}

void UNinjaChakraFeetComponent::Build()
{
	bBuilt = true;
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	Body = GetNinjaVisibleBody(Owner);

	// Casca: cópia da malha do corpo seguindo a mesma pose; o material só mostra (e infla) a região dos pés
	if (Body && ShellMaterial)
	{
		Shell = NewObject<USkeletalMeshComponent>(Owner);
		Shell->SetSkeletalMeshAsset(Body->GetSkeletalMeshAsset());
		Shell->SetupAttachment(Body);
		Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shell->SetCastShadow(false);
		Shell->RegisterComponent();
		Shell->SetLeaderPoseComponent(Body);
		ShellMID = UMaterialInstanceDynamic::Create(ShellMaterial, this);
		ShellMID->SetScalarParameterValue(TEXT("FootRadius"), FootRadius);
		ShellMID->SetScalarParameterValue(TEXT("AnkleHeight"), AnkleHeight);
		ShellMID->SetScalarParameterValue(TEXT("Inflate"), Inflate);
		for (int32 i = 0; i < Shell->GetNumMaterials(); ++i)
		{
			Shell->SetMaterial(i, ShellMID);
		}
		Shell->SetVisibility(false);
	}

	const FName Bones[2][2] = { { FootBoneL, BallBoneL }, { FootBoneR, BallBoneR } };
	for (int32 i = 0; i < 2; ++i)
	{
		FFoot& Foot = Feet.AddDefaulted_GetRef();
		Foot.Bone = Bones[i][0];
		Foot.Ball = Bones[i][1];
		Foot.Ground = MakePart(GroundMaterial);
		for (int32 w = 0; w < 6; ++w)
		{
			Foot.Wisps.AddDefaulted_GetRef().Part = MakePart(WispMaterial);
		}
		Foot.Light = NewObject<UPointLightComponent>(GetOwner());
		Foot.Light->SetUsingAbsoluteLocation(true);
		Foot.Light->SetupAttachment(this);
		Foot.Light->SetCastShadows(false);
		Foot.Light->SetLightColor(FLinearColor(FColor(47, 140, 255)));
		Foot.Light->SetAttenuationRadius(LightRadius);
		Foot.Light->SetIntensity(0.f);
		Foot.Light->RegisterComponent();
		Keep.Add(Foot.Light);
	}
}

void UNinjaChakraFeetComponent::SetFeetActive(bool bActive)
{
	if (!bBuilt)
	{
		Build();
	}
	bWantActive = bActive;
}

bool UNinjaChakraFeetComponent::TraceSurface(const FVector& From, const FVector& Down, FHitResult& OutHit) const
{
	// Chão/parede (Visibility) e água (canal Agua, que normalmente só sobrepõe): fica o mais perto
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NinjaFeetTrace), false, GetOwner());
	const FVector To = From + Down * 70.f;
	bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, From, To, ECC_Visibility, Params);
	TArray<FHitResult> Water;
	GetWorld()->LineTraceMultiByChannel(Water, From, To, ECC_GameTraceChannel1, Params);
	for (const FHitResult& W : Water)
	{
		if (!bHit || W.Distance < OutHit.Distance)
		{
			OutHit = W;
			bHit = true;
		}
	}
	return bHit;
}

void UNinjaChakraFeetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!bBuilt || !Owner || !Body)
	{
		return;
	}
	Alpha = FMath::FInterpConstantTo(Alpha, bWantActive ? 1.f : 0.f, DeltaTime, 1.f / FMath::Max(RevealTime, 0.01f));
	Intensity = FMath::FInterpConstantTo(Intensity, TargetIntensity, DeltaTime, 1.f);   // devagar: nunca salta
	const float Reveal = FMath::SmoothStep(0.f, 1.f, Alpha);
	const FVector Up = -Owner->GetCharacterMovement()->GetGravityDirection();
	const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FVector CamLoc = Cam ? Cam->GetCameraLocation() : Owner->GetActorLocation() + Owner->GetActorForwardVector() * 400.f;

	for (FFoot& Foot : Feet)
	{
		const FVector Ankle = Body->GetSocketLocation(Foot.Bone);
		const FVector Ball = Body->GetSocketLocation(Foot.Ball);
		// Centro da sola: entre tornozelo e peito do pé, na altura do mais baixo dos dois, um pouco abaixo
		const FVector Mid = (Ankle + Ball) * 0.5f;
		const float Lowest = FMath::Min(FVector::DotProduct(Ankle, Up), FVector::DotProduct(Ball, Up));
		Foot.SoleCenter = Mid - Up * (FVector::DotProduct(Mid, Up) - Lowest + 3.f);
		UpdateFoot(Foot, DeltaTime, Reveal, Up, CamLoc);
	}

	if (Shell && ShellMID)
	{
		Shell->SetVisibility(Reveal > 0.f);
		ShellMID->SetScalarParameterValue(TEXT("Reveal"), Reveal);
		ShellMID->SetScalarParameterValue(TEXT("Intensity"), 0.75f + 0.25f * Intensity);
		ShellMID->SetVectorParameterValue(TEXT("Up"), FLinearColor(Up.X, Up.Y, Up.Z));
		ShellMID->SetVectorParameterValue(TEXT("FootL"), FLinearColor(Feet[0].SoleCenter.X, Feet[0].SoleCenter.Y, Feet[0].SoleCenter.Z));
		ShellMID->SetVectorParameterValue(TEXT("FootR"), FLinearColor(Feet[1].SoleCenter.X, Feet[1].SoleCenter.Y, Feet[1].SoleCenter.Z));
	}
}

void UNinjaChakraFeetComponent::UpdateFoot(FFoot& Foot, float DeltaTime, float Shown, const FVector& Up, const FVector& CamLoc)
{
	const bool bOn = Shown > 0.f;
	const FVector ToeDir = FVector::VectorPlaneProject(GetOwner()->GetActorForwardVector(), Up).GetSafeNormal();

	// --- Brilho na superfície embaixo do pé: estável, mais forte quanto mais perto ---
	FHitResult Hit;
	const bool bHit = bOn && TraceSurface(Foot.SoleCenter + Up * 15.f, -Up, Hit);
	const float Height = bHit ? FVector::DotProduct(Foot.SoleCenter - Hit.ImpactPoint, Up) : 1000.f;
	const float GroundAlpha = bHit ? FMath::Clamp(1.f - Height / FMath::Max(GroundGlowDistance, 1.f), 0.f, 1.f) : 0.f;
	Foot.Ground.Mesh->SetVisibility(GroundAlpha > 0.f);
	if (GroundAlpha > 0.f && Foot.Ground.MID)
	{
		Foot.Ground.MID->SetScalarParameterValue(TEXT("Intensity"), Shown * GroundAlpha * (0.7f + 0.3f * Intensity));
		Foot.Ground.Mesh->SetWorldRotation(FRotationMatrix::MakeFromZX(Hit.ImpactNormal, ToeDir).Rotator());
		Foot.Ground.Mesh->SetWorldScale3D(FVector(GroundGlowSize / 100.f, GroundGlowSize / 100.f, 1.f));
		Foot.Ground.Mesh->SetWorldLocation(Hit.ImpactPoint + Hit.ImpactNormal * 0.6f);
	}

	// --- Fiapos: soltam do topo da casca, sobem devagar balançando de leve, entram e saem por curva suave ---
	if (bOn && bWantActive)
	{
		Foot.WispCarry += DeltaTime * WispRate * (0.6f + 0.6f * Intensity);
		while (Foot.WispCarry >= 1.f)
		{
			Foot.WispCarry -= 1.f;
			if (FWisp* Free = Foot.Wisps.FindByPredicate([](const FWisp& W) { return W.Age < 0.f; }))
			{
				const FVector Out = FVector::VectorPlaneProject(FMath::VRand(), Up).GetSafeNormal();
				Free->Age = 0.f;
				Free->Life = FMath::FRandRange(WispLife.X, WispLife.Y);
				Free->Speed = FMath::FRandRange(WispSpeed.X, WispSpeed.Y);
				Free->Phase = FMath::FRandRange(0.f, 2.f * PI);
				Free->Side = FVector::CrossProduct(Up, Out).GetSafeNormal();
				Free->Start = Foot.SoleCenter + Out * FMath::FRandRange(5.f, 10.f) + Up * AnkleHeight * FMath::FRandRange(0.5f, 0.9f);
			}
		}
	}
	for (FWisp& Wisp : Foot.Wisps)
	{
		if (Wisp.Age < 0.f)
		{
			Wisp.Part.Mesh->SetVisibility(false);
			continue;
		}
		Wisp.Age += DeltaTime;
		const float P = Wisp.Age / Wisp.Life;
		if (P >= 1.f)
		{
			Wisp.Age = -1.f;
			Wisp.Part.Mesh->SetVisibility(false);
			continue;
		}
		// Sobe em velocidade constante; balanço lateral lento (0,6 Hz, 2,5 cm)
		const FVector At = Wisp.Start + Up * Wisp.Speed * Wisp.Age + Wisp.Side * FMath::Sin(Wisp.Age * 2.f * PI * 0.6f + Wisp.Phase) * 2.5f;
		FVector ToCam = FVector::VectorPlaneProject(CamLoc - At, Up).GetSafeNormal();
		if (ToCam.IsNearlyZero())
		{
			ToCam = -ToeDir;
		}
		const float Fade = FMath::SmoothStep(0.f, 0.3f, P) * (1.f - FMath::SmoothStep(0.55f, 1.f, P));
		Wisp.Part.Mesh->SetVisibility(true);
		if (Wisp.Part.MID)
		{
			Wisp.Part.MID->SetScalarParameterValue(TEXT("Fade"), Fade * Shown);
			Wisp.Part.MID->SetScalarParameterValue(TEXT("Intensity"), 0.6f + 0.4f * Intensity);
		}
		Wisp.Part.Mesh->SetWorldRotation(FRotationMatrix::MakeFromZX(ToCam, Up).Rotator());   // plano: frente -> câmera, X -> para cima
		Wisp.Part.Mesh->SetWorldScale3D(FVector(WispSize.Y / 100.f, WispSize.X / 100.f, 1.f));
		Wisp.Part.Mesh->SetWorldLocation(At);
	}

	// --- Luz azul fixa ---
	Foot.Light->SetIntensity(LightIntensity * Shown);
	Foot.Light->SetWorldLocation(Foot.SoleCenter + Up * 12.f);
}
