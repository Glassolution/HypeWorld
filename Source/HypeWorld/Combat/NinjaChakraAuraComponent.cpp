// Aura de chakra (estilo Shinobi Striker).

#include "NinjaChakraAuraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Character.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

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
	bWantActive = bActive;
	if (bActive)
	{
		SetPartsVisible(true);
	}
}

void UNinjaChakraAuraComponent::BeginPlay()
{
	Super::BeginPlay();
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

	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	auto MakeStatic = [this, Owner](UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& Location, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner);
		Comp->SetStaticMesh(Mesh);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(false);
		Comp->SetupAttachment(this);
		Comp->SetRelativeLocation(Location);
		Comp->SetRelativeScale3D(Scale);
		Comp->RegisterComponent();
		if (UMaterialInstanceDynamic* MID = MakeMID(Material))
		{
			Comp->SetMaterial(0, MID);
		}
		return Comp;
	};

	// Pés do personagem ficam em -meia altura da cápsula (o componente fica no centro da cápsula)
	const float FeetZ = -Owner->GetSimpleCollisionHalfHeight();

	// 1) Brilho no corpo: cópia da malha seguindo a pose do personagem, inflada pelo material
	USkeletalMeshComponent* Body = Owner->GetMesh();
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

	// 2) Chamas: dois cilindros (100 x 100 cm na escala 1, centro no meio) sem tampa visível
	FlameInner = MakeStatic(Cylinder, FlameMaterial, FVector(0.f, 0.f, FeetZ + 110.f), FVector(1.1f, 1.1f, 2.3f));
	FlameOuter = MakeStatic(Cylinder, FlameMaterial, FVector(0.f, 0.f, FeetZ + 130.f), FVector(1.6f, 1.6f, 2.7f));

	// 3) Anéis no chão
	GroundRing = MakeStatic(Plane, RingMaterial, FVector(0.f, 0.f, FeetZ + 1.5f), FVector(4.f, 4.f, 1.f));

	// 4) Faíscas subindo
	if (Sphere)
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
			Spark.Radius = FMath::FRandRange(25.f, 75.f);
			Spark.Height = FMath::FRandRange(0.f, 220.f);
			Spark.Speed = FMath::FRandRange(90.f, 220.f);
			Spark.Size = FMath::FRandRange(0.015f, 0.04f);
			Sparks.Add(Spark);
			SparkMesh->AddInstance(FTransform::Identity);
		}
	}

	// 5) Luz azul pulsando
	Light = NewObject<UPointLightComponent>(Owner);
	Light->SetupAttachment(this);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	Light->SetLightColor(ChakraColor);
	Light->SetIntensity(0.f);
	Light->SetAttenuationRadius(450.f);
	Light->SetCastShadows(false);
	Light->RegisterComponent();
}

void UNinjaChakraAuraComponent::SetPartsVisible(bool bVisible)
{
	for (USceneComponent* Part : TArray<USceneComponent*>{ Shell, FlameInner, FlameOuter, GroundRing, SparkMesh, Light })
	{
		if (Part)
		{
			Part->SetVisibility(bVisible);
		}
	}
}

void UNinjaChakraAuraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float Target = bWantActive ? 1.f : 0.f;
	if (Alpha == Target && !bWantActive)
	{
		return;
	}
	Alpha = FMath::FInterpConstantTo(Alpha, Target, DeltaTime, FadeSpeed);
	Time += DeltaTime;

	// Respira um pouco, como chakra pulsando
	const float Pulse = 0.85f + 0.15f * FMath::Sin(Time * 9.f);
	for (UMaterialInstanceDynamic* MID : MIDs)
	{
		MID->SetScalarParameterValue(TEXT("Intensity"), Alpha * Pulse);
	}
	if (Light)
	{
		Light->SetIntensity(LightIntensity * Alpha * (0.75f + 0.25f * FMath::Sin(Time * 13.f)));
	}

	// Chamas giram devagar em sentidos opostos (dá profundidade)
	if (FlameInner) { FlameInner->AddRelativeRotation(FRotator(0.f, 40.f * DeltaTime, 0.f)); }
	if (FlameOuter) { FlameOuter->AddRelativeRotation(FRotator(0.f, -25.f * DeltaTime, 0.f)); }

	// Faíscas: sobem em espiral e renascem embaixo
	if (SparkMesh)
	{
		const float FeetZ = GetOwner() ? -GetOwner()->GetSimpleCollisionHalfHeight() : -90.f;
		for (int32 i = 0; i < Sparks.Num(); ++i)
		{
			FSpark& S = Sparks[i];
			S.Height += S.Speed * DeltaTime;
			S.Angle += DeltaTime * 1.5f;
			S.Radius = FMath::Max(10.f, S.Radius - DeltaTime * 12.f);
			if (S.Height > 240.f)
			{
				S.Height = 0.f;
				S.Radius = FMath::FRandRange(30.f, 80.f);
			}
			const float Fade = FMath::Clamp(1.f - S.Height / 240.f, 0.f, 1.f) * Alpha;
			const FVector Location(FMath::Cos(S.Angle) * S.Radius, FMath::Sin(S.Angle) * S.Radius, FeetZ + S.Height);
			const float Scale = S.Size * FMath::Max(Fade, 0.05f);
			SparkMesh->UpdateInstanceTransform(i, FTransform(FRotator::ZeroRotator, Location, FVector(Scale, Scale, Scale * 2.5f)), false, i == Sparks.Num() - 1, true);
		}
	}

	if (!bWantActive && Alpha <= 0.f)
	{
		SetPartsVisible(false);
	}
}
