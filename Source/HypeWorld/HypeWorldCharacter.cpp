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
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "NinjaAttributeSet.h"
#include "NinjaEffects.h"
#include "NinjaAbilities.h"
#include "NinjaChakraAuraComponent.h"
#include "NinjaDamageNumber.h"
#include "Net/UnrealNetwork.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"

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
	// Freada suave: atrito próprio para frear (em vez do atrito do chão, que parava no tranco)
	GetCharacterMovement()->BrakingDecelerationWalking = 1200.f;
	GetCharacterMovement()->bUseSeparateBrakingFriction = true;
	GetCharacterMovement()->BrakingFriction = 0.6f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	// Câmera única (parado, andando e correndo): atrás, perto e um pouco acima, olhando levemente para baixo,
	// com o boneco na parte de baixo da tela e o caminho à frente visível
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 70.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true; // segue suave, mas sem deixar o boneco escapar
	CameraBoom->CameraLagSpeed = 20.f;
	CameraBoom->CameraLagMaxDistance = 25.f;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetRelativeRotation(FRotator(-8.f, 0.f, 0.f));

	// Combate: sistema de habilidades no próprio personagem (replicação "Mixed": o dono recebe tudo, os outros o essencial)
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	Attributes = CreateDefaultSubobject<UNinjaAttributeSet>(TEXT("Attributes"));

	ChakraAura = CreateDefaultSubobject<UNinjaChakraAuraComponent>(TEXT("ChakraAura"));
	ChakraAura->SetupAttachment(RootComponent);

	ComboAbility = UGA_NinjaMeleeCombo::StaticClass();
	ChargeChakraAbility = UGA_NinjaChargeChakra::StaticClass();

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

		// Combate
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoAttackPressed);
		EnhancedInputComponent->BindAction(ChargeChakraAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoChargePressed);
		EnhancedInputComponent->BindAction(ChargeChakraAction, ETriggerEvent::Completed, this, &AHypeWorldCharacter::DoChargeReleased);
		EnhancedInputComponent->BindAction(FocusAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoFocusStart);
		EnhancedInputComponent->BindAction(FocusAction, ETriggerEvent::Completed, this, &AHypeWorldCharacter::DoFocusEnd);
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

		FVector Wish = ForwardDirection * Forward + RightDirection * Right;

		// Carregando chakra: não anda. Atacando: só ajusta um pouco a direção (o golpe já empurra para a frente)
		if (bChargingChakra)
		{
			return;
		}
		if (IsAttacking() && !Wish.IsNearlyZero())
		{
			// Só puxa um pouco a direção do golpe (a força fica cheia; o limite de velocidade vem do componente de movimento)
			Wish = (GetActorForwardVector() * (1.f - AttackSteering) + Wish.GetSafeNormal() * AttackSteering).GetSafeNormal2D() * FMath::Min(Wish.Size(), 1.f);
		}

		// Correndo rápido: A/D viram em arco (limita o ângulo em relação à direção atual), para o boneco
		// não sair correndo de lado na frente da câmera. Para trás (S) continua valendo: freia e vira.
		const FVector VelocityDir = GetVelocity().GetSafeNormal2D();
		if (GetVelocity().Size2D() > CameraFollowMinSpeed && GetCharacterMovement()->IsMovingOnGround() && !Wish.IsNearlyZero())
		{
			const float WishSize = FMath::Min(Wish.Size(), 1.f);
			const FVector WishDir = Wish.GetSafeNormal2D();
			const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(VelocityDir, WishDir), -1.f, 1.f)));
			if (Angle > SprintSteerAngle && Angle < 150.f)
			{
				const float Side = FMath::Sign(FVector::CrossProduct(VelocityDir, WishDir).Z);
				Wish = VelocityDir.RotateAngleAxis(Side * SprintSteerAngle, FVector::UpVector) * WishSize;
			}
		}

		// add movement 
		AddMovementInput(Wish.GetSafeNormal2D(), FMath::Min(Wish.Size(), 1.f));
	}
}

void AHypeWorldCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		if (!FMath::IsNearlyZero(Yaw) || !FMath::IsNearlyZero(Pitch))
		{
			LastLookInputTime = GetWorld()->GetTimeSeconds();
		}

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
	DefaultRotationRate = GetCharacterMovement()->RotationRate.Yaw;

	// GAS: dono e avatar são o próprio personagem; habilidades são dadas pelo servidor
	AbilitySystem->InitAbilityActorInfo(this, this);
	if (HasAuthority())
	{
		if (ComboAbility) { AbilitySystem->GiveAbility(FGameplayAbilitySpec(ComboAbility, 1, INDEX_NONE, this)); }
		if (ChargeChakraAbility) { AbilitySystem->GiveAbility(FGameplayAbilitySpec(ChargeChakraAbility, 1, INDEX_NONE, this)); }
	}
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UNinjaAttributeSet::GetHealthAttribute()).AddUObject(this, &AHypeWorldCharacter::OnHealthChanged);
}

void AHypeWorldCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateLean(DeltaSeconds);
	UpdateRunAnimRate();
	UpdateIdleBreak(DeltaSeconds);
	UpdateTurnInPlace(DeltaSeconds);
	UpdateCameraFollow(DeltaSeconds);
}

void AHypeWorldCharacter::UpdateIdleBreak(float DeltaSeconds)
{
	UNinjaMovementComponent* NinjaMovement = GetNinjaMovement();
	if (!NinjaMovement || !IdleBreakMontage)
	{
		return;
	}

	// Parado no chão (pela velocidade, então funciona também nos outros jogadores)
	const bool bStill = NinjaMovement->IsMovingOnGround() && GetVelocity().SizeSquared2D() < 9.f;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!bStill)
	{
		IdleTime = 0.f;
		if (AnimInstance && AnimInstance->Montage_IsPlaying(IdleBreakMontage))
		{
			StopAnimMontage(IdleBreakMontage);   // mexeu: interrompe suavemente
		}
		return;
	}

	IdleTime += DeltaSeconds;
	if (IdleTime >= IdleBreakDelay && AnimInstance && !AnimInstance->IsAnyMontagePlaying())
	{
		PlayAnimMontage(IdleBreakMontage);
		IdleTime = 0.f;   // conta de novo a partir de agora
	}
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
	const float Speed = GetVelocity().Size2D();
	if (NinjaMovement->IsWallRunning() && RunAnimNaturalSpeed > 0.f)
	{
		Rate = FMath::Clamp(Speed / RunAnimNaturalSpeed, 1.f, FMath::Max(1.f, MaxRunAnimRate));
	}
	else if (NinjaMovement->IsMovingOnGround() && !IsAttacking() && AnimSpeedCurve.Num() > 0 && Speed > AnimSpeedCurve[0].X)
	{
		// Velocidade natural da mistura de animações nessa velocidade (interpola a curva)
		float Natural = AnimSpeedCurve.Last().Y;
		for (int32 i = 1; i < AnimSpeedCurve.Num(); ++i)
		{
			if (Speed <= AnimSpeedCurve[i].X)
			{
				const float T = (Speed - AnimSpeedCurve[i - 1].X) / FMath::Max(1.f, AnimSpeedCurve[i].X - AnimSpeedCurve[i - 1].X);
				Natural = FMath::Lerp(AnimSpeedCurve[i - 1].Y, AnimSpeedCurve[i].Y, T);
				break;
			}
		}
		Rate = FMath::Clamp(Speed / FMath::Max(1.f, Natural), 1.f, FMath::Max(1.f, MaxRunAnimRate));
	}
	GetMesh()->GlobalAnimRateScale = FMath::FInterpTo(GetMesh()->GlobalAnimRateScale, Rate, GetWorld()->GetDeltaSeconds(), 10.f);
}

void AHypeWorldCharacter::UpdateCameraFollow(float DeltaSeconds)
{
	AController* PC = GetController();
	UNinjaMovementComponent* NinjaMovement = GetNinjaMovement();
	if (!IsLocallyControlled() || !PC || !NinjaMovement)
	{
		return;
	}

	const FVector Velocity2D = GetVelocity().GetSafeNormal2D() * GetVelocity().Size2D();
	const bool bFast = Velocity2D.Size() > CameraFollowMinSpeed && !NinjaMovement->IsWallRunning();
	const bool bMouseIdle = GetWorld()->GetTimeSeconds() - LastLookInputTime > CameraFollowDelay;
	if (!bFast || !bMouseIdle)
	{
		return;
	}

	// Gira só o yaw da câmera em direção à direção da corrida, mais forte quanto mais rápido
	FRotator Control = PC->GetControlRotation();
	const float SpeedFactor = FMath::Clamp(Velocity2D.Size() / 1200.f, 0.3f, 1.f);
	Control.Yaw = FMath::FInterpTo(Control.Yaw, Control.Yaw + FMath::FindDeltaAngleDegrees(Control.Yaw, Velocity2D.Rotation().Yaw), DeltaSeconds, CameraFollowSpeed * SpeedFactor);
	PC->SetControlRotation(Control);
}

void AHypeWorldCharacter::UpdateTurnInPlace(float DeltaSeconds)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const float Speed = GetVelocity().Size2D();

	// Rapidez de giro: virada em andamento > corrida (curva aberta) > normal
	if (TurnTimeLeft > 0.f)
	{
		TurnTimeLeft -= DeltaSeconds;
	}
	else
	{
		Move->RotationRate.Yaw = Speed > CameraFollowMinSpeed ? SprintRotationRate : DefaultRotationRate;
	}

	const bool bStill = Move->IsMovingOnGround() && Speed < 10.f;

	if (!bStill)
	{
		StillTime = 0.f;
		return;
	}
	StillTime += DeltaSeconds;
}

void AHypeWorldCharacter::HandleMoveStart(const FVector& MoveAcceleration)
{
	// Chamado pelo componente de movimento antes de girar o corpo, com a direção da tecla deste quadro
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (StillTime < 0.2f || MoveAcceleration.IsNearlyZero() || GetVelocity().Size2D() > 10.f || !Move->IsMovingOnGround())
	{
		return;
	}
	StillTime = 0.f;

	const float Angle = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, MoveAcceleration.Rotation().Yaw);
	UAnimMontage* Montage = nullptr;
	if (FMath::Abs(Angle) >= 135.f)
	{
		Montage = Turn180Montage;
	}
	else if (FMath::Abs(Angle) >= 60.f)
	{
		Montage = TurnRightMontage;   // a animação só dá os passos (o giro é do código), serve para os dois lados
	}
	if (Montage && TurnDuration > 0.f)
	{
		PlayAnimMontage(Montage, Montage->GetPlayLength() / TurnDuration);
		Move->RotationRate.Yaw = FMath::Abs(Angle) / TurnDuration;   // corpo gira no ritmo da animação
		TurnTimeLeft = TurnDuration;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Combate
// ---------------------------------------------------------------------------------------------------------------------

UAbilitySystemComponent* AHypeWorldCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void AHypeWorldCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AbilitySystem->InitAbilityActorInfo(this, this);   // atualiza o controle dono (servidor)
}

void AHypeWorldCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHypeWorldCharacter, bChargingChakra);
}

bool AHypeWorldCharacter::IsAttacking() const
{
	const FGameplayAbilitySpec* Spec = ComboAbility ? AbilitySystem->FindAbilitySpecFromClass(ComboAbility) : nullptr;
	return Spec && Spec->IsActive();
}

void AHypeWorldCharacter::DoAttackPressed()
{
	if (bKnockedOut || bChargingChakra)
	{
		return;
	}
	if (IsAttacking())
	{
		// Durante o combo: guarda o clique para emendar o próximo golpe (aqui e no servidor)
		CachedAttackInputTime = GetWorld()->GetTimeSeconds();
		if (!HasAuthority())
		{
			ServerCacheAttackInput();
		}
		return;
	}
	AbilitySystem->TryActivateAbilityByClass(ComboAbility);
}

void AHypeWorldCharacter::ServerCacheAttackInput_Implementation()
{
	CachedAttackInputTime = GetWorld()->GetTimeSeconds();
}

void AHypeWorldCharacter::BeginCombo()
{
	ComboIndex = 0;
	CachedAttackInputTime = -100.f;
	FaceNearestEnemy();
	Lunge();
}

void AHypeWorldCharacter::CheckCombo()
{
	// Chamado pelo AnimNotify no fim de cada golpe: se o jogador clicou a tempo, emenda o próximo
	if (!IsAttacking() || GetWorld()->GetTimeSeconds() - CachedAttackInputTime > ComboInputWindow)
	{
		return;
	}
	CachedAttackInputTime = -100.f;
	if (ComboIndex + 1 < ComboSectionNames.Num())
	{
		++ComboIndex;
		FaceNearestEnemy();
		Lunge();
		AbilitySystem->CurrentMontageJumpToSection(ComboSectionNames[ComboIndex]);
	}
}

void AHypeWorldCharacter::DoAttackTrace(FName DamageSourceBone)
{
	// Só o servidor decide o acerto e o dano (todos veem o resultado pela vida replicada)
	if (!HasAuthority())
	{
		return;
	}

	const FVector Start = GetActorLocation() + GetActorForwardVector() * 30.f;
	const FVector End = Start + GetActorForwardVector() * MeleeReach;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NinjaMelee), false, this);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, Objects, FCollisionShape::MakeSphere(MeleeRadius), Params);

	TSet<AActor*> Done;
	const float Damage = ComboDamage.IsValidIndex(ComboIndex) ? ComboDamage[ComboIndex] : 25.f;
	bool bHitSomething = false;
	for (const FHitResult& Hit : Hits)
	{
		AHypeWorldCharacter* Target = Cast<AHypeWorldCharacter>(Hit.GetActor());
		if (!Target || Done.Contains(Target) || Target->bKnockedOut)
		{
			continue;
		}
		Done.Add(Target);

		// Desvio: quem está no dash fica invulnerável
		UNinjaMovementComponent* TargetMove = Target->GetNinjaMovement();
		if (TargetMove && TargetMove->IsDashing())
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
		FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
		Context.AddHitResult(Hit);
		FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(UGE_NinjaDamage::StaticClass(), 1.f, Context);
		Spec.Data->SetSetByCallerMagnitude(NinjaSetByCaller::Damage, Damage);
		AbilitySystem->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

		// Empurrãozinho para trás no último golpe
		if (ComboIndex == ComboSectionNames.Num() - 1)
		{
			Target->LaunchCharacter(GetActorForwardVector() * 700.f + FVector(0.f, 0.f, 250.f), true, true);
		}
		bHitSomething = true;
	}

	if (bHitSomething)
	{
		Attributes->SetChakra(FMath::Min(Attributes->GetChakra() + ChakraPerHit, Attributes->GetMaxChakra()));
	}
}

void AHypeWorldCharacter::FaceNearestEnemy()
{
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NinjaSoftLock), false, this);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(SoftLockRange), Params);

	// Prefere o alvo mais perto da direção que o jogador está olhando (câmera)
	const FVector Look = GetController() ? GetController()->GetControlRotation().Vector().GetSafeNormal2D() : GetActorForwardVector();
	AActor* Best = nullptr;
	float BestScore = -1.f;
	for (const FOverlapResult& O : Overlaps)
	{
		AHypeWorldCharacter* Other = Cast<AHypeWorldCharacter>(O.GetActor());
		if (!Other || Other == this || Other->bKnockedOut)
		{
			continue;
		}
		const FVector To = (Other->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		const float Facing = FVector::DotProduct(Look, To);
		if (Facing < 0.25f)
		{
			continue;
		}
		const float Score = Facing - (Other->GetActorLocation() - GetActorLocation()).Size2D() / SoftLockRange * 0.5f;
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Other;
		}
	}
	if (Best)
	{
		SetActorRotation(FRotator(0.f, (Best->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0.f));
	}
	else if (GetController())
	{
		SetActorRotation(FRotator(0.f, Look.Rotation().Yaw, 0.f));
	}
}

void AHypeWorldCharacter::Lunge()
{
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		// Vindo correndo, o golpe aproveita parte do embalo (mas nunca menos que o avanço normal)
		const float Carry = FMath::Max(AttackLunge, GetVelocity().Size2D() * 0.6f);
		LaunchCharacter(GetActorForwardVector() * Carry, true, false);
	}
}

void AHypeWorldCharacter::HitStop()
{
	CustomTimeDilation = 0.05f;
	GetWorldTimerManager().SetTimer(HitStopTimer, [this]() { CustomTimeDilation = 1.f; }, HitStopDuration, false);
}

void AHypeWorldCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	const float Delta = Data.NewValue - Data.OldValue;
	if (Delta < -0.5f)
	{
		const FVector Head = GetActorLocation() + FVector(0.f, 0.f, GetSimpleCollisionHalfHeight() + 10.f);
		ANinjaDamageNumber::Spawn(GetWorld(), Head, FString::FromInt(FMath::RoundToInt(-Delta)),
			-Delta >= 50.f ? FLinearColor(1.f, 0.55f, 0.1f) : FLinearColor(1.f, 0.95f, 0.75f), -Delta >= 50.f ? 46.f : 36.f);
		ANinjaHitFlash::Spawn(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 25.f), HitFlashMaterial, FLinearColor(1.f, 0.85f, 0.5f));
		HitStop();

		if (Data.NewValue <= 0.f)
		{
			OnKnockedOut();
		}
		else if (HitReactMontages.Num() > 0)
		{
			if (bChargingChakra && HasAuthority())
			{
				AbilitySystem->CancelAbilities();   // levar golpe interrompe a carga de chakra
			}
			PlayAnimMontage(HitReactMontages[FMath::RandRange(0, HitReactMontages.Num() - 1)], 1.3f);
		}
	}
	else if (Delta > 0.5f && bKnockedOut && Data.NewValue > 0.f)
	{
		// Voltou (respawn do boneco de treino)
		bKnockedOut = false;
		StopAnimMontage(KnockoutMontage);
	}
}

void AHypeWorldCharacter::OnKnockedOut()
{
	bKnockedOut = true;
	if (HasAuthority())
	{
		AbilitySystem->CancelAbilities();
	}
	if (KnockoutMontage)
	{
		PlayAnimMontage(KnockoutMontage);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Carregar chakra
// ---------------------------------------------------------------------------------------------------------------------

bool AHypeWorldCharacter::CanChargeChakra() const
{
	return !bKnockedOut && !IsAttacking() && GetCharacterMovement()->IsMovingOnGround();
}

void AHypeWorldCharacter::DoChargePressed()
{
	AbilitySystem->TryActivateAbilityByClass(ChargeChakraAbility);
}

void AHypeWorldCharacter::DoChargeReleased()
{
	if (FGameplayAbilitySpec* Spec = ChargeChakraAbility ? AbilitySystem->FindAbilitySpecFromClass(ChargeChakraAbility) : nullptr)
	{
		if (Spec->IsActive())
		{
			AbilitySystem->CancelAbilityHandle(Spec->Handle);
		}
	}
}

void AHypeWorldCharacter::SetChargingChakra(bool bCharging)
{
	bChargingChakra = bCharging;
	OnRep_ChargingChakra();
}

void AHypeWorldCharacter::OnRep_ChargingChakra()
{
	if (ChakraAura)
	{
		ChakraAura->SetAuraActive(bChargingChakra);
	}
}

float AHypeWorldCharacter::GetHealth() const
{
	return Attributes ? Attributes->GetHealth() : 0.f;
}

float AHypeWorldCharacter::GetChakra() const
{
	return Attributes ? Attributes->GetChakra() : 0.f;
}

void AHypeWorldCharacter::DoFocusStart()
{
	if (UNinjaMovementComponent* NinjaMovement = GetNinjaMovement())
	{
		NinjaMovement->SetFocusing(true);
	}
}

void AHypeWorldCharacter::DoFocusEnd()
{
	if (UNinjaMovementComponent* NinjaMovement = GetNinjaMovement())
	{
		NinjaMovement->SetFocusing(false);
	}
}
