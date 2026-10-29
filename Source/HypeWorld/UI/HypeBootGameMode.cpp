// Regras do mapa de boot (ver o .h).

#include "HypeBootGameMode.h"
#include "HypeLoadingScreenWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AHypeBootGameMode::AHypeBootGameMode()
{
	DefaultPawnClass = nullptr;
	LoadingScreenClass = TSoftClassPtr<UHypeLoadingScreenWidget>(FSoftObjectPath(TEXT("/Game/HypeWorld/UI/Loading/WBP_LoadingScreen.WBP_LoadingScreen_C")));
}

void AHypeBootGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}
	PC->SetShowMouseCursor(false);
	PC->SetInputMode(FInputModeGameOnly());
	PC->DisableInput(PC);

	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->bDisableWorldRendering = true;	// o mapa é vazio: não precisa desenhar 3D (volta ao abrir o mapa seguinte)
	}

	TSubclassOf<UHypeLoadingScreenWidget> WidgetClass = LoadingScreenClass.LoadSynchronous();
	if (!WidgetClass)
	{
		WidgetClass = UHypeLoadingScreenWidget::StaticClass();
	}
	if (UHypeLoadingScreenWidget* Screen = CreateWidget<UHypeLoadingScreenWidget>(PC, WidgetClass))
	{
		Screen->AddToViewport(100);
	}
}
