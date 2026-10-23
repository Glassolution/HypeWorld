// Copyright Epic Games, Inc. All Rights Reserved.

#include "HypeWorldCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "HypeWorld.h"
#include "NinjaMovementComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"

AHypeWorldCharacter::AHypeWorldCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UNinjaMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	// Ninja: pulo alto e rápido (gravidade maior), bom controle no ar, pulo duplo
	GetCharacterMovement()->GravityScale = 1.6f;
	GetCharacterMovement()->JumpZVelocity = 900.f;
	GetCharacterMovement()->AirControl = 0.6f;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MaxAcceleration = 3000.f;
	JumpMaxCount = 2;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true; // câmera "atrasa" um pouco, dá sensação de velocidade
	CameraBoom->CameraLagSpeed = 12.f;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AHypeWorldCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHypeWorldCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AHypeWorldCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHypeWorldCharacter::Look);

		// Corrida ninja e dash
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoSprintStart);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AHypeWorldCharacter::DoSprintEnd);
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoDash);
	}
	else
	{
		UE_LOG(LogHypeWorld, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AHypeWorldCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AHypeWorldCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AHypeWorldCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AHypeWorldCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AHypeWorldCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AHypeWorldCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AHypeWorldCharacter::DoSprintStart()
{
	if (UNinjaMovementComponent* NinjaMovement = GetNinjaMovement())
	{
		NinjaMovement->SetSprinting(true);
	}
}

void AHypeWorldCharacter::DoSprintEnd()
{
	if (UNinjaMovementComponent* NinjaMovement = GetNinjaMovement())
	{
		NinjaMovement->SetSprinting(false);
	}
}

void AHypeWorldCharacter::DoDash()
{
	if (UNinjaMovementComponent* NinjaMovement = GetNinjaMovement())
	{
		NinjaMovement->RequestDash();
	}
}

UNinjaMovementComponent* AHypeWorldCharacter::GetNinjaMovement() const
{
	return Cast<UNinjaMovementComponent>(GetCharacterMovement());
}

void AHypeWorldCharacter::BeginPlay()
{
	Super::BeginPlay();

	BaseMeshRotation = GetMesh()->GetRelativeRotation();
	BaseMeshLocation = GetMesh()->GetRelativeLocation();
	LastYaw = GetActorRotation().Yaw;
}

void AHypeWorldCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateLean(DeltaSeconds);
	UpdateRunAnimRate();
}

void AHypeWorldCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	// Ao refazer movimentos depois de uma correção do servidor, não repetir animações.
	UNinjaMovementComponent* NinjaMovement = GetNinjaMovement();
	if (bClientUpdating || !NinjaMovement)
	{
		return;
	}

	const bool bWasDashing = PrevMovementMode == MOVE_Custom && PreviousCustomMode == NMOVE_Dash;
	const bool bWasWallRunning = PrevMovementMode == MOVE_Custom && PreviousCustomMode == NMOVE_WallRun;

	// Saiu de um modo: para a animação dele
	if (bWasDashing && !NinjaMovement->IsDashing() && DashMontage)
	{
		StopAnimMontage(DashMontage);
	}
	if (bWasWallRunning && !NinjaMovement->IsWallRunning())
	{
		if (WallRunMontage)
		{
			StopAnimMontage(WallRunMontage);
		}

		// Saiu da parede subindo = pulo de parede
		if (NinjaMovement->IsFalling() && GetVelocity().Z > 0.f)
		{
			PlayAnimMontage(WallJumpMontage);
		}
	}

	// Entrou num modo: toca a animação dele
	if (NinjaMovement->IsDashing())
	{
		PlayAnimMontage(DashMontage, NinjaMovement->DashAnimRate);
	}
	else if (NinjaMovement->IsWallRunning() && WallRunMontage)
	{
		PlayAnimMontage(WallRunMontage);
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			// Faz a montagem repetir sozinha enquanto estiver na parede
			AnimInstance->Montage_SetNextSection(FName("Default"), FName("Default"), WallRunMontage);
		}
	}
}

void AHypeWorldCharacter::UpdateLean(float DeltaSeconds)
{
	UNinjaMovementComponent* NinjaMovement = GetNinjaMovement();
	if (!NinjaMovement || DeltaSeconds <= 0.f)
	{
		return;
	}

	const float Yaw = GetActorRotation().Yaw;
	const float YawRate = FMath::FindDeltaAngleDegrees(LastYaw, Yaw) / DeltaSeconds;
	LastYaw = Yaw;

	float TargetLean = 0.f;
	float TargetWallOffset = 0.f;

	if (NinjaMovement->IsWallRunning())
	{
		// Descobre de que lado está a parede (funciona também nos outros jogadores)
		FHitResult Hit;
		float WallSide = 0.f;
		if (NinjaMovement->FindWallBeside(Hit, WallSide))
		{
			TargetLean = WallSide * WallRunLean;              // positivo = topo vai para a esquerda
			TargetWallOffset = WallSide * WallRunMeshOffset;  // pés encostam na parede
		}
	}
	else if (NinjaMovement->IsMovingOnGround())
	{
		// Inclina para dentro da curva; mais forte quanto mais rápido
		const float SpeedFactor = FMath::Clamp(GetVelocity().Size2D() / RunAnimNaturalSpeed, 0.f, 1.f);
		TargetLean = -FMath::Clamp(YawRate * (MaxTurnLean / 360.f), -MaxTurnLean, MaxTurnLean) * SpeedFactor;
	}

	CurrentLean = FMath::FInterpTo(CurrentLean, TargetLean, DeltaSeconds, LeanInterpSpeed);
	CurrentWallOffset = FMath::FInterpTo(CurrentWallOffset, TargetWallOffset, DeltaSeconds, LeanInterpSpeed);

	const FQuat LeanQuat(FVector::ForwardVector, FMath::DegreesToRadians(CurrentLean));
	GetMesh()->SetRelativeRotation(LeanQuat * BaseMeshRotation.Quaternion());
	GetMesh()->SetRelativeLocation(BaseMeshLocation + FVector(0.f, CurrentWallOffset, 0.f));
}

void AHypeWorldCharacter::UpdateRunAnimRate()
{
	UNinjaMovementComponent* NinjaMovement = GetNinjaMovement();
	if (!NinjaMovement)
	{
		return;
	}

	float Rate = 1.f;
	if ((NinjaMovement->IsMovingOnGround() || NinjaMovement->IsWallRunning()) && RunAnimNaturalSpeed > 0.f)
	{
		Rate = FMath::Clamp(GetVelocity().Size2D() / RunAnimNaturalSpeed, 1.f, 2.f);
	}
	GetMesh()->GlobalAnimRateScale = Rate;
}
