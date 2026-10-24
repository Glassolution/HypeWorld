// Copyright Epic Games, Inc. All Rights Reserved.


#include "HypeWorldPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "HypeWorld.h"
#include "NinjaHUDWidget.h"
#include "NinjaCombatMenuWidget.h"
#include "Components/Widget.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AHypeWorldPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// HUD só para quem está jogando nesta máquina
	if (IsLocalPlayerController() && HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UNinjaHUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToPlayerScreen(1);
		}
	}

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogHypeWorld, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AHypeWorldPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AHypeWorldPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

bool AHypeWorldPlayerController::IsCombatMenuOpen() const
{
	return CombatMenu && CombatMenu->IsInViewport() && CombatMenu->GetVisibility() == ESlateVisibility::Visible;
}

void AHypeWorldPlayerController::ToggleCombatMenu()
{
	if (!IsLocalPlayerController())
	{
		return;
	}
	if (!CombatMenu)
	{
		CombatMenu = CreateWidget<UNinjaCombatMenuWidget>(this, UNinjaCombatMenuWidget::StaticClass());
		if (!CombatMenu)
		{
			return;
		}
		CombatMenu->AddToPlayerScreen(10);
		CombatMenu->SetVisibility(ESlateVisibility::Collapsed);
		CombatMenu->OnCloseRequested.AddUObject(this, &AHypeWorldPlayerController::ToggleCombatMenu);
	}
	const bool bOpen = !IsCombatMenuOpen();
	CombatMenu->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(bOpen ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);   // o HUD ficaria por baixo do menu
	}
	SetShowMouseCursor(bOpen);
	if (bOpen)
	{
		CombatMenu->Refresh();
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
		SetIgnoreLookInput(true);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
		ResetIgnoreLookInput();
	}
}
