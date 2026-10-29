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
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
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
		// Esc fora do Enhanced Input (tecla fixa, não depende de asset): soltar/prender o mouse
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AHypeWorldPlayerController::ToggleFreeMouse);

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

void AHypeWorldPlayerController::SetHUDHidden(bool bHideHUD)
{
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(bHideHUD ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
}

void AHypeWorldPlayerController::ToggleFreeMouse()
{
	if (IsCombatMenuOpen())
	{
		ToggleCombatMenu();
		return;
	}
	bMouseFree = !bMouseFree;
	SetShowMouseCursor(bMouseFree);
	if (bMouseFree)
	{
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

void AHypeWorldPlayerController::UpdateRotation(float DeltaTime)
{
	// Igual ao padrão, mas trabalhando "no espaço da gravidade" do personagem (receita da Epic para gravidade personalizada):
	// a rotação da câmera é convertida para um mundo onde a gravidade aponta para baixo, recebe o mouse e volta.
	FVector Gravity = FVector::DownVector;
	if (const ACharacter* PlayerCharacter = Cast<ACharacter>(GetPawn()))
	{
		if (const UCharacterMovementComponent* Move = PlayerCharacter->GetCharacterMovement())
		{
			Gravity = Move->GetGravityDirection();
		}
	}

	FRotator ViewRotation = GetControlRotation();
	if (!LastFrameGravity.IsZero() && !LastFrameGravity.Equals(Gravity))
	{
		// A gravidade mudou (grudou numa parede): gira a câmera junto, sem tranco
		ViewRotation = (FQuat::FindBetweenNormals(LastFrameGravity, Gravity) * ViewRotation.Quaternion()).Rotator();
	}
	LastFrameGravity = Gravity;

	const FQuat ToGravity = FQuat::FindBetweenNormals(Gravity, FVector::DownVector);
	FRotator Relative = (ToGravity * ViewRotation.Quaternion()).Rotator();
	FRotator DeltaRot(RotationInput);
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ProcessViewRotation(DeltaTime, Relative, DeltaRot);
		Relative.Roll = 0.f;
		SetControlRotation((ToGravity.Inverse() * Relative.Quaternion()).Rotator());
	}
	if (APawn* const P = GetPawnOrSpectator())
	{
		P->FaceRotation(GetControlRotation(), DeltaTime);
	}
}
