// Número de dano e flash de impacto.

#include "NinjaDamageNumber.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------------------------------------------------------
// Widget
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> UNinjaDamageNumberWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Number"));
		WidgetTree->RootWidget = Label;
	}
	return Super::RebuildWidget();
}

void UNinjaDamageNumberWidget::SetNumber(const FString& Text, const FLinearColor& Color, float FontSize)
{
	if (!Label)
	{
		RebuildWidget();
	}
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FontSize);
	Font.OutlineSettings.OutlineSize = 3;
	Font.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.85f);
	Label->SetFont(Font);
	Label->SetText(FText::FromString(Text));
	Label->SetColorAndOpacity(FSlateColor(Color));
	Label->SetShadowOffset(FVector2D(2.f, 2.f));
	Label->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.5f));
}

// ---------------------------------------------------------------------------------------------------------------------
// Número
// ---------------------------------------------------------------------------------------------------------------------

ANinjaDamageNumber::ANinjaDamageNumber()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	RootComponent = Widget;
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetDrawAtDesiredSize(true);
	Widget->SetWidgetClass(UNinjaDamageNumberWidget::StaticClass());
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANinjaDamageNumber::Spawn(UWorld* World, const FVector& Location, const FString& Text, const FLinearColor& Color, float FontSize)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Jitter(FMath::FRandRange(-25.f, 25.f), FMath::FRandRange(-25.f, 25.f), FMath::FRandRange(0.f, 20.f));
	ANinjaDamageNumber* Number = World->SpawnActor<ANinjaDamageNumber>(ANinjaDamageNumber::StaticClass(), Location + Jitter, FRotator::ZeroRotator, Params);
	if (!Number)
	{
		return;
	}
	Number->Drift = FVector(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), 150.f);
	Number->Widget->InitWidget();
	if (UNinjaDamageNumberWidget* W = Cast<UNinjaDamageNumberWidget>(Number->Widget->GetUserWidgetObject()))
	{
		W->SetNumber(Text, Color, FontSize);
	}
}

void ANinjaDamageNumber::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;

	// Sobe desacelerando, "salta" no começo e some no fim
	const float T = Age / Lifetime;
	AddActorWorldOffset(Drift * DeltaSeconds * FMath::Max(0.f, 1.f - T * 1.4f));
	if (UUserWidget* W = Widget->GetUserWidgetObject())
	{
		const float Pop = T < 0.12f ? FMath::Lerp(1.6f, 1.f, T / 0.12f) : 1.f;
		W->SetRenderScale(FVector2D(Pop, Pop));
		W->SetRenderOpacity(FMath::Clamp((1.f - T) * 3.f, 0.f, 1.f));
	}
	if (Age >= Lifetime)
	{
		Destroy();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Flash
// ---------------------------------------------------------------------------------------------------------------------

ANinjaHitFlash::ANinjaHitFlash()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		Mesh->SetStaticMesh(Sphere.Object);
	}
}

void ANinjaHitFlash::Spawn(UWorld* World, const FVector& Location, UMaterialInterface* Material, const FLinearColor& Color)
{
	if (!World || !Material || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ANinjaHitFlash* Flash = World->SpawnActor<ANinjaHitFlash>(ANinjaHitFlash::StaticClass(), Location, FRotator::ZeroRotator, Params);
	if (Flash)
	{
		Flash->MID = UMaterialInstanceDynamic::Create(Material, Flash);
		Flash->MID->SetVectorParameterValue(TEXT("Color"), Color);
		Flash->Mesh->SetMaterial(0, Flash->MID);
		Flash->SetActorScale3D(FVector(0.1f));
	}
}

void ANinjaHitFlash::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const float T = FMath::Clamp(Age / 0.18f, 0.f, 1.f);
	SetActorScale3D(FVector(FMath::Lerp(0.1f, 0.6f, FMath::Sqrt(T))));
	if (MID)
	{
		MID->SetScalarParameterValue(TEXT("Intensity"), 1.f - T);
	}
	if (T >= 1.f)
	{
		Destroy();
	}
}
