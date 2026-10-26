// Aura de chakra estilo anime: fogo de energia desenhado em volta do corpo (a mesma carregando e no máximo).

#include "NinjaChakraAuraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NinjaVisibleBody.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Character.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Sound/SoundBase.h"

namespace
{
	/** Ruído suave e repetível (tileable) 128x128, gerado por código — vira as "chamas" no material. */
	UTexture2D* MakeNoiseTexture()
	{
		constexpr int32 Size = 128;
		auto Hash = [](int32 X, int32 Y, int32 Period)
		{
			X = ((X % Period) + Period) % Period;
			Y = ((Y % Period) + Period) % Period;
			uint32 H = uint32(X) * 374761393u + uint32(Y) * 668265263u;
			H = (H ^ (H >> 13)) * 1274126177u;
			return float(H & 0xFFFF) / 65535.f;
		};
		auto ValueNoise = [&Hash](float X, float Y, int32 Period)
		{
			const int32 X0 = FMath::FloorToInt(X), Y0 = FMath::FloorToInt(Y);
			const float FX = FMath::SmoothStep(0.f, 1.f, X - X0), FY = FMath::SmoothStep(0.f, 1.f, Y - Y0);
			const float A = FMath::Lerp(Hash(X0, Y0, Period), Hash(X0 + 1, Y0, Period), FX);
			const float B = FMath::Lerp(Hash(X0, Y0 + 1, Period), Hash(X0 + 1, Y0 + 1, Period), FX);
			return FMath::Lerp(A, B, FY);
		};

		TArray<FColor> Pixels;
		Pixels.SetNumUninitialized(Size * Size);
		for (int32 Y = 0; Y < Size; ++Y)
		{
			for (int32 X = 0; X < Size; ++X)
			{
				float Sum = 0.f, Amp = 0.5f;
				for (int32 Octave = 0; Octave < 4; ++Octave)
				{
					const int32 Period = 4 << Octave;
					Sum += ValueNoise(X * Period / float(Size), Y * Period / float(Size), Period) * Amp;
					Amp *= 0.5f;
				}
				const uint8 V = uint8(FMath::Clamp(Sum / 0.9375f, 0.f, 1.f) * 255.f);
				Pixels[Y * Size + X] = FColor(V, V, V, 255);
			}
		}
		const TConstArrayView64<uint8> Bytes(reinterpret_cast<const uint8*>(Pixels.GetData()), Pixels.Num() * sizeof(FColor));
		UTexture2D* Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8, NAME_None, Bytes);
		Texture->SRGB = true;   // casa com o tipo "Color" do parâmetro Noise nos materiais
		Texture->AddressX = TA_Wrap;
		Texture->AddressY = TA_Wrap;
		Texture->UpdateResource();
		return Texture;
	}
}

UNinjaChakraAuraComponent::UNinjaChakraAuraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UNinjaChakraAuraComponent::SetAuraActive(bool bActive)
{
	if (bActive && !bWantActive)
	{
		SetPartsVisible(true);
		if (ChargeStartSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ChargeStartSound, GetComponentLocation());
		}
		PlayLoop(bWantFull ? FullLoopSound : ChargeLoopSound);
	}
	else if (!bActive && bWantActive && LoopAudio)
	{
		LoopAudio->FadeOut(0.4f, 0.f);
	}
	bWantActive = bActive;
}

void UNinjaChakraAuraComponent::SetAuraFull(bool bFull)
{
	if (bFull == bWantFull)
	{
		return;
	}
	bWantFull = bFull;
	if (bFull && FullBurstSound && bWantActive)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FullBurstSound, GetComponentLocation());
	}
	if (bWantActive)
	{
		PlayLoop(bFull ? FullLoopSound : ChargeLoopSound);
	}
}

void UNinjaChakraAuraComponent::PlayLoop(USoundBase* Sound)
{
	if (!LoopAudio)
	{
		return;
	}
	if (!Sound)
	{
		LoopAudio->FadeOut(0.3f, 0.f);
		return;
	}
	LoopAudio->SetSound(Sound);
	LoopAudio->FadeIn(0.2f);
}

void UNinjaChakraAuraComponent::BeginPlay()
{
	Super::BeginPlay();
	// A chama desenha sem teste de profundidade (o chão não corta a base); o material usa o Custom Depth do corpo
	// para o personagem continuar na frente dela
	if (USkeletalMeshComponent* Body = GetNinjaVisibleBody(Cast<ACharacter>(GetOwner())))
	{
		Body->SetRenderCustomDepth(true);
	}
	BuildAura();
	SetPartsVisible(false);
}

UMaterialInstanceDynamic* UNinjaChakraAuraComponent::MakeMID(UMaterialInterface* Base)
{
	if (!Base)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
	MID->SetVectorParameterValue(TEXT("Color"), ChakraColor);
	MID->SetScalarParameterValue(TEXT("Intensity"), 0.f);
	if (NoiseTexture)
	{
		MID->SetTextureParameterValue(TEXT("Noise"), NoiseTexture);
	}
	MIDs.Add(MID);
	return MID;
}

void UNinjaChakraAuraComponent::BuildAura()
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner)
	{
		return;
	}
	NoiseTexture = MakeNoiseTexture();

	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	auto MakeStatic = [this, Owner](UStaticMesh* Mesh, UMaterialInstanceDynamic* MID, bool bUseAbsoluteRotation)
	{
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner);
		Comp->SetStaticMesh(Mesh);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(false);
		Comp->SetupAttachment(this);
		Comp->SetUsingAbsoluteRotation(bUseAbsoluteRotation);   // planos virados para a câmera giram sozinhos
		Comp->RegisterComponent();
		if (MID)
		{
			Comp->SetMaterial(0, MID);
		}
		return Comp;
	};

	const float FeetZ = -Owner->GetSimpleCollisionHalfHeight();

	// 1) Brilho leve no corpo (cópia da malha seguindo a pose, inflada pelo material)
	USkeletalMeshComponent* Body = GetNinjaVisibleBody(Owner);
	if (Body && ShellMaterial)
	{
		Shell = NewObject<USkeletalMeshComponent>(Owner);
		Shell->SetSkeletalMeshAsset(Body->GetSkeletalMeshAsset());
		Shell->SetupAttachment(Body);
		Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shell->SetCastShadow(false);
		Shell->RegisterComponent();
		Shell->SetLeaderPoseComponent(Body);
		UMaterialInstanceDynamic* MID = MakeMID(ShellMaterial);
		for (int32 i = 0; i < Shell->GetNumMaterials(); ++i)
		{
			Shell->SetMaterial(i, MID);
		}
	}

	// 2) Chama desenhada: plano virado para a câmera atrás do corpo + camada da frente (outra semente, só contornos)
	FlameMID = MakeMID(FlameMaterial);
	Flame = MakeStatic(Plane, FlameMID, true);
	FrontFlameMID = MakeMID(FlameMaterial);
	if (FrontFlameMID)
	{
		FrontFlameMID->SetScalarParameterValue(TEXT("FrontLayer"), 1.f);
		FrontFlameMID->SetScalarParameterValue(TEXT("Seed"), 37.f);
		FrontFlame = MakeStatic(Plane, FrontFlameMID, true);
	}

	// 3) Brilho no chão
	Ground = MakeStatic(Plane, MakeMID(GroundMaterial), false);
	Ground->SetRelativeLocation(FVector(0.f, 0.f, FeetZ + 1.f));
	Ground->SetRelativeScale3D(FVector(4.f, 4.f, 1.f));

	// 4) Faíscas subindo
	if (Sphere && SparkCount > 0)
	{
		SparkMesh = NewObject<UInstancedStaticMeshComponent>(Owner);
		SparkMesh->SetStaticMesh(Sphere);
		SparkMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SparkMesh->SetCastShadow(false);
		SparkMesh->SetupAttachment(this);
		SparkMesh->RegisterComponent();
		if (UMaterialInstanceDynamic* MID = MakeMID(SparkMaterial))
		{
			SparkMesh->SetMaterial(0, MID);
		}
		for (int32 i = 0; i < SparkCount; ++i)
		{
			FSpark Spark;
			Spark.Angle = FMath::FRandRange(0.f, 2.f * PI);
			Spark.Radius = FMath::FRandRange(40.f, 140.f);
			Spark.Height = FMath::FRandRange(0.f, 300.f);
			Spark.Speed = FMath::FRandRange(90.f, 260.f);
			Spark.Size = FMath::FRandRange(0.012f, 0.035f);
			Sparks.Add(Spark);
			SparkMesh->AddInstance(FTransform::Identity);
		}
	}

	// 5) Luz azul
	Light = NewObject<UPointLightComponent>(Owner);
	Light->SetupAttachment(this);
	Light->SetLightColor(LightColor);
	Light->SetIntensity(0.f);
	Light->SetAttenuationRadius(600.f);
	Light->SetCastShadows(false);
	Light->RegisterComponent();

	// 6) Som em laço (carregando / no máximo)
	LoopAudio = NewObject<UAudioComponent>(Owner);
	LoopAudio->SetupAttachment(this);
	LoopAudio->bAutoActivate = false;
	LoopAudio->RegisterComponent();
}

void UNinjaChakraAuraComponent::SetPartsVisible(bool bShowParts)
{
	for (USceneComponent* Part : TArray<USceneComponent*>{ Shell, Flame, FrontFlame, Ground, SparkMesh, Light })
	{
		if (Part)
		{
			Part->SetVisibility(bShowParts);
		}
	}
}

void UNinjaChakraAuraComponent::FaceCamera(UStaticMeshComponent* Plane, const FVector2D& Size, float BottomZ, float BackOffset, float FrontReach) const
{
	// Plano de pé (só gira no eixo vertical), de frente para a câmera de quem está olhando, um pouco atrás do personagem
	const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FVector Center = GetComponentLocation();
	const FVector Bottom = Center + FVector(0.f, 0.f, BottomZ);
	FVector ToCam = Cam ? (Cam->GetCameraLocation() - Bottom) : GetOwner()->GetActorForwardVector();
	// A base desce seguindo a linha câmera -> chão: na tela ela fica no chão FrontReach cm à frente dos pés (cobre o pé de
	// trás da pose), mesmo o plano estando BackOffset cm atrás do corpo
	const float HorizDist = FVector(ToCam.X, ToCam.Y, 0.f).Size();
	const float Drop = HorizDist > 1.f ? ToCam.Z / HorizDist * (BackOffset + FrontReach) : 0.f;
	ToCam.Z = 0.f;
	ToCam = ToCam.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	// Plano da engine: 100x100 cm em X/Y, frente = +Z. Frente -> câmera, X do plano -> para cima.
	Plane->SetWorldRotation(FRotationMatrix::MakeFromZX(ToCam, FVector::UpVector).Rotator());
	Plane->SetWorldScale3D(FVector(Size.Y / 100.f, Size.X / 100.f, 1.f));
	Plane->SetWorldLocation(Bottom + FVector(0.f, 0.f, Size.Y * 0.5f - Drop) - ToCam * BackOffset);
}

void UNinjaChakraAuraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float Target = bWantActive ? 1.f : 0.f;
	if (Alpha == Target && !bWantActive)
	{
		return;
	}
	// Ligando: cresce de baixo para cima em GrowTime; desligando: se parte em pedaços em DissolveTime
	Alpha = FMath::FInterpConstantTo(Alpha, Target, DeltaTime, 1.f / FMath::Max(bWantActive ? GrowTime : DissolveTime, 0.01f));
	Time += DeltaTime;

	const float FeetZ = GetOwner() ? -GetOwner()->GetSimpleCollisionHalfHeight() : -90.f;
	// Sem pulsar (Felipe: chakra FLUI, não pulsa): tamanho, luz e brilho constantes; o movimento é só o do material
	const float Grow = bWantActive ? FMath::SmoothStep(0.f, 1.f, Alpha) : 1.f;
	const float Dissolve = bWantActive ? 0.f : 1.f - Alpha;
	const float Shown = bWantActive ? 1.f : FMath::Min(Alpha * 3.f, 1.f);   // os pedaços somem no fim do Dissolve
	const float Bright = Shown * (0.85f + 0.3f * ChargeLevel);

	for (UMaterialInstanceDynamic* MID : MIDs)
	{
		MID->SetScalarParameterValue(TEXT("Intensity"), Alpha);
	}
	for (UMaterialInstanceDynamic* MID : { FlameMID.Get(), FrontFlameMID.Get() })
	{
		if (MID)
		{
			MID->SetScalarParameterValue(TEXT("Intensity"), Bright);
			MID->SetScalarParameterValue(TEXT("Grow"), Grow);
			MID->SetScalarParameterValue(TEXT("Dissolve"), Dissolve);
			MID->SetScalarParameterValue(TEXT("ChargeLevel"), ChargeLevel);
			MID->SetScalarParameterValue(TEXT("Aspect"), FlameSize.X / FMath::Max(FlameSize.Y, 1.f));   // o desenho não estica com o tamanho
		}
	}

	const FVector2D Size = FlameSize;
	if (Flame) { FaceCamera(Flame, Size, FeetZ + FlameBottomOffset, FlameBackOffset, FlameFrontReach); }
	if (FrontFlame) { FaceCamera(FrontFlame, Size * FrontLayerScale, FeetZ + FlameBottomOffset, -FrontLayerOffset, FlameFrontReach); }

	if (Light)
	{
		Light->SetIntensity(LightIntensity * Alpha * (0.7f + 0.3f * ChargeLevel));
	}

	// Faíscas: sobem em espiral e renascem embaixo
	if (SparkMesh)
	{
		const float Spread = 1.f;
		for (int32 i = 0; i < Sparks.Num(); ++i)
		{
			FSpark& S = Sparks[i];
			S.Height += S.Speed * DeltaTime;
			S.Angle += DeltaTime * 1.5f;
			if (S.Height > 320.f)
			{
				S.Height = 0.f;
				S.Radius = FMath::FRandRange(40.f, 140.f);
			}
			const float Fade = FMath::Clamp(1.f - S.Height / 320.f, 0.f, 1.f) * Alpha;
			const FVector Location(FMath::Cos(S.Angle) * S.Radius * Spread, FMath::Sin(S.Angle) * S.Radius * Spread, FeetZ + S.Height);
			const float Scale = S.Size * FMath::Max(Fade, 0.05f);
			SparkMesh->UpdateInstanceTransform(i, FTransform(FRotator::ZeroRotator, Location, FVector(Scale)), false, i == Sparks.Num() - 1, true);
		}
	}

	if (!bWantActive && Alpha <= 0.f)
	{
		SetPartsVisible(false);
	}
}
