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
#include "Ninja/NinjaDodgeDefinition.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "NinjaAttributeSet.h"
#include "NinjaEffects.h"
#include "NinjaAbilities.h"
#include "NinjaChakraAuraComponent.h"
#include "NinjaChakraFeetComponent.h"
#include "NinjaDamageNumber.h"
#include "Net/UnrealNetwork.h"
#include "Engine/OverlapResult.h"
#include "AnimNotify_DoAttackTrace.h"
#include "TimerManager.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NinjaCombatTypes.h"
#include "HypeWorldPlayerController.h"
#include "NinjaJutsuData.h"
#include "NinjaJutsuAbility.h"
#include "NinjaJutsuProjectile.h"
#include "NinjaVisibleBody.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

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
	GetCharacterMovement()->GravityScale = 1.f;
	GetCharacterMovement()->JumpZVelocity = 980.f;
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
	// Golpes andam pela própria animação, mas dá para corrigir a direção com as teclas no meio do golpe
	GetCharacterMovement()->bAllowPhysicsRotationDuringAnimRootMotion = true;

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
	ChakraFeetFx = CreateDefaultSubobject<UNinjaChakraFeetComponent>(TEXT("ChakraFeetFx"));
	ChakraFeetFx->SetupAttachment(RootComponent);

	ComboAbility = UGA_NinjaMeleeCombo::StaticClass();
	ChargeChakraAbility = UGA_NinjaChargeChakra::StaticClass();

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AHypeWorldCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Atalho direto para abrir/fechar a seleção de estilos e poses de combate.
	PlayerInputComponent->BindKey(EKeys::I, IE_Pressed, this, &AHypeWorldCharacter::DoCombatMenu);

	// Barra de jutsus: tecla do slot = usa o jutsu do slot
	for (int32 SlotIndex = 0; SlotIndex < JutsuSlotKeys.Num(); ++SlotIndex)
	{
		FInputKeyBinding Binding(FInputChord(JutsuSlotKeys[SlotIndex]), IE_Pressed);
		Binding.KeyDelegate.GetDelegateForManualSet().BindWeakLambda(this, [this, SlotIndex]() { TryUseJutsuSlot(SlotIndex); });
		PlayerInputComponent->KeyBindings.Add(Binding);
	}

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AHypeWorldCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHypeWorldCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AHypeWorldCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AHypeWorldCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AHypeWorldCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHypeWorldCharacter::Look);

		// Corrida ninja e dash
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoSprintStart);
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoDash);

		// Combate
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoAttackPressed);
		EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoHeavyAttackPressed);
		EnhancedInputComponent->BindAction(ChargeChakraAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoChargePressed);
		EnhancedInputComponent->BindAction(ChargeChakraAction, ETriggerEvent::Completed, this, &AHypeWorldCharacter::DoChargeReleased);
		EnhancedInputComponent->BindAction(FocusAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoFocusStart);
		EnhancedInputComponent->BindAction(FocusAction, ETriggerEvent::Completed, this, &AHypeWorldCharacter::DoFocusEnd);
		if (CombatMenuAction)
		{
			EnhancedInputComponent->BindAction(CombatMenuAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoCombatMenu);
		}
		if (WalkToggleAction)
		{
			EnhancedInputComponent->BindAction(WalkToggleAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoWalkToggle);
		}
		if (ChakraFeetAction)
		{
			EnhancedInputComponent->BindAction(ChakraFeetAction, ETriggerEvent::Started, this, &AHypeWorldCharacter::DoToggleChakraFeet);
		}
		if (ZoomAction)
		{
			EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AHypeWorldCharacter::Zoom);
		}
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

	CheckForwardDoubleTap(MovementVector.Y);

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

void AHypeWorldCharacter::Zoom(const FInputActionValue& Value)
{
	// Rodinha para cima (+1) aproxima, para baixo (-1) afasta
	CameraZoomGoal = FMath::Clamp(CameraZoomGoal - Value.Get<float>() * CameraZoomStep, CameraZoomMin, CameraZoomMax);
}

void AHypeWorldCharacter::UpdateCameraZoom(float DeltaSeconds)
{
	if (CameraZoomGoal > 0.f)
	{
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, CameraZoomGoal, DeltaSeconds, CameraZoomSpeed);
	}
}

void AHypeWorldCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward — relativo à gravidade do personagem (na parede/teto o "chão" muda)
		const FVector Gravity = GetCharacterMovement()->GetGravityDirection();
		const FQuat ToGravity = FQuat::FindBetweenNormals(Gravity, FVector::DownVector);
		const FRotator GravityRelative = (ToGravity * GetController()->GetControlRotation().Quaternion()).Rotator();
		const FQuat YawRotation = ToGravity.Inverse() * FRotator(0.f, GravityRelative.Yaw, 0.f).Quaternion();

		// get forward vector
		const FVector ForwardDirection = YawRotation.GetAxisX();

		// get right vector 
		const FVector RightDirection = YawRotation.GetAxisY();

		FVector Wish = ForwardDirection * Forward + RightDirection * Right;
		if (!Wish.IsNearlyZero() && !Wish.Equals(RawMoveDirection, 0.01f) && GetNinjaMovement()->CanCancelCombatDodge()) GetNinjaMovement()->TryCancelCombatDodge();
		RawMoveDirection = Wish;
		if (GetNinjaMovement()->HasDashRequest() && DashMoveFrame == GFrameCounter)
		{
			DashMoveDirection = Wish;
		}
		RawMoveTime = GetWorld()->GetTimeSeconds();

		// Carregando chakra / super pulo, fazendo jutsu ou atordoado: não anda. Atacando: só ajusta um pouco a direção (o golpe já empurra para a frente)
		if (bChargingChakra || bChargingSuperJump || IsStunned() || IsCastingJutsu())
		{
			return;
		}
		if (IsAttacking() && !Wish.IsNearlyZero())
		{
			// Só puxa um pouco a direção do golpe (a força fica cheia; o limite de velocidade vem do componente de movimento)
			Wish = (GetActorForwardVector() * (1.f - AttackSteering) + Wish.GetSafeNormal() * AttackSteering).GetSafeNormal2D() * FMath::Min(Wish.Size(), 1.f);
		}

		// Corrida ninja: A/D viram em arco (limita o ângulo em relação à direção atual), para o boneco
		// não sair correndo de lado na frente da câmera. Para trás (S) continua valendo: freia e vira na hora.
		// O trote não entra aqui (corpo de frente para a câmera, passos laterais)
		const FVector VelocityDir = GetVelocity().GetSafeNormal2D();
		if (!GetNinjaMovement()->HasDashRequest() && !GetCharacterMovement()->HasCustomGravity() && GetNinjaMovement()->IsSprinting() && GetVelocity().Size2D() > CameraFollowMinSpeed && !Wish.IsNearlyZero())
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

		// add movement (no plano do chão atual: na parede/teto o "chão" não é horizontal)
		AddMovementInput(GetCharacterMovement()->ProjectToGravityFloor(Wish).GetSafeNormal(), FMath::Min(Wish.Size(), 1.f));
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
	if (IsCastingJutsu())
	{
		return;
	}
	bSpaceHeld = true;
	// Modo de batalha, no chão: S (para trás em relação à câmera) + Espaço = mortal para trás; toque rápido = pulo normal;
	// segurar (LauncherHoldTime) = lançador (joga o adversário para cima e sobe junto), também no meio do combo
	if (IsInBattleMode() && GetCharacterMovement()->IsMovingOnGround())
	{
		if (CanAct() && !IsAttacking() && GetController())
		{
			const FVector Accel = GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal2D();
			const FVector CamFwd = GetControlRotation().Vector().GetSafeNormal2D();
			if (!Accel.IsNearlyZero() && FVector::DotProduct(Accel, CamFwd) < -0.5f)
			{
				DoBackflipJump();
				return;
			}
		}
		const UNinjaFightingStyle* Style = GetFightingStyle();
		if (Style && Style->Launchers.Num() > 0 && CanStartLauncher())
		{
			bLauncherHoldPending = true;   // decide no UpdateLauncherHold (segurou) ou no DoJumpEnd (soltou antes)
			LauncherHoldElapsed = 0.f;
			return;
		}
		if (!IsAttacking()) Jump();
		return;
	}
	// No chão (fora do modo de batalha): segurar carrega o super pulo; um toque rápido ainda é o pulo normal (decidido ao soltar)
	if (CanStartSuperJump())
	{
		bJumpKeyHeld = true;
		JumpHoldTime = 0.f;
		return;
	}
	Jump();
}

void AHypeWorldCharacter::DoJumpEnd()
{
	bSpaceHeld = false;
	if (bLauncherHoldPending)
	{
		// Modo de batalha: soltou antes de virar lançador = pulo normal (golpeando, o pulo não sai — igual a antes)
		bLauncherHoldPending = false;
		if (!IsAttacking())
		{
			TapJump();
		}
		return;
	}
	if (bJumpKeyHeld)
	{
		bJumpKeyHeld = false;
		if (bChargingSuperJump)
		{
			ReleaseSuperJump();
		}
		else
		{
			TapJump();
		}
		return;
	}
	StopJumping();
}

void AHypeWorldCharacter::TapJump()
{
	Jump();
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &ACharacter::StopJumping, 0.1f, false);
}

void AHypeWorldCharacter::UpdateLauncherHold(float DeltaSeconds)
{
	if (!bLauncherHoldPending)
	{
		return;
	}
	// Saiu do chão ou do modo de batalha segurando: desiste (sem pular)
	if (!CanStartLauncher())
	{
		bLauncherHoldPending = false;
		return;
	}
	LauncherHoldElapsed += DeltaSeconds;
	if (LauncherHoldElapsed >= LauncherHoldTime)
	{
		bLauncherHoldPending = false;
		StartAttack(ENinjaAttackInput::Launcher);
	}
}

void AHypeWorldCharacter::DoSprintStart()
{
	// O padrão é trotar (sem segurar nada). Shift troca trote <-> corrida ninja; andando, o Shift já vai para a corrida ninja.
	// Parando, a corrida ninja volta para o trote (UpdateAutoRun).
	if (UNinjaMovementComponent* NinjaMovement = GetNinjaMovement())
	{
		if (NinjaMovement->WantsToSprint())
		{
			DoSprintEnd();
		}
		else
		{
			NinjaMovement->SetRunning(true);
			NinjaMovement->SetSprinting(true);
		}
	}
}

void AHypeWorldCharacter::DoWalkToggle()
{
	if (UNinjaMovementComponent* NinjaMovement = GetNinjaMovement())
	{
		const bool bWalk = NinjaMovement->WantsToRun() || NinjaMovement->WantsToSprint();
		NinjaMovement->SetSprinting(false);
		NinjaMovement->SetRunning(!bWalk);
	}
}

void AHypeWorldCharacter::CheckForwardDoubleTap(float Forward)
{
	if (Forward < 0.5f)
	{
		return;
	}
	// Move chega todo quadro enquanto alguma direção está apertada: se no quadro anterior o W não estava, é um toque novo
	const bool bNewTap = GFrameCounter > LastForwardFrame + 1;
	LastForwardFrame = GFrameCounter;
	if (!bNewTap)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	UNinjaMovementComponent* NinjaMovement = GetNinjaMovement();
	if (NinjaMovement && Now - LastForwardTapTime <= DoubleTapSprintWindow && !NinjaMovement->WantsToSprint())
	{
		// Mesmo estado do Shift na corrida ninja (trote + corrida ninja ligados); parar volta ao trote (UpdateAutoRun)
		NinjaMovement->SetRunning(true);
		NinjaMovement->SetSprinting(true);
		LastForwardTapTime = -100.f;   // um terceiro toque não conta como outro duplo
		return;
	}
	LastForwardTapTime = Now;
}

void AHypeWorldCharacter::UpdateAutoRun(float DeltaSeconds)
{
	UNinjaMovementComponent* NinjaMovement = GetNinjaMovement();
	if (!IsLocallyControlled() || !NinjaMovement || !(NinjaMovement->WantsToRun() || NinjaMovement->WantsToSprint()))
	{
		AutoRunStopTime = 0.f;
		return;
	}
	// Parado no chão sem apertar direção por um instante: a corrida ninja volta para o trote. O trote fica ligado
	// (no PvP o jogador para e muda de lado o tempo todo); só o Shift o desliga
	const bool bStopped = NinjaMovement->IsMovingOnGround() && NinjaMovement->GetCurrentAcceleration().IsNearlyZero() && GetVelocity().Size2D() < 20.f;
	AutoRunStopTime = bStopped && NinjaMovement->WantsToSprint() ? AutoRunStopTime + DeltaSeconds : 0.f;
	if (AutoRunStopTime > 0.3f)
	{
		NinjaMovement->SetSprinting(false);
		NinjaMovement->SetRunning(true);
		AutoRunStopTime = 0.f;
	}
}

void AHypeWorldCharacter::DoSprintEnd()
{
	if (UNinjaMovementComponent* NinjaMovement = GetNinjaMovement())
	{
		NinjaMovement->SetSprinting(false);
		NinjaMovement->SetRunning(true);
	}
}

void AHypeWorldCharacter::DoDash()
{
	if (IsCastingJutsu())
	{
		return;
	}
	if (IsStunned())
	{
		TrySubstitution();   // apanhando: o botão do dash vira a fuga (substituição)
		return;
	}
	if (UNinjaMovementComponent* NinjaMovement = GetNinjaMovement())
	{
		// Apenas agenda: a direção é lida depois de todos os inputs deste quadro.
		DashMoveDirection = RawMoveDirection;
		DashMoveFrame = GFrameCounter;
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

	SubstitutionCharges = MaxSubstitutions;
	if (HasAuthority())
	{
		for (const FNinjaPassiveInfo& Passive : Passives)
		{
			if (Passive.bLearned && Passive.bEnabledByDefault)
			{
				EnabledPassives |= 1u << uint32(Passive.Id);
			}
		}
	}
	OnRep_EnabledPassives();   // brilho nos pés + colisão com a água conforme a infusão
	BaseMeshRotation = GetMesh()->GetRelativeRotation();
	BaseMeshLocation = GetMesh()->GetRelativeLocation();
	LastYaw = GetActorRotation().Yaw;
	DefaultRotationRate = GetCharacterMovement()->RotationRate.Yaw;
	CameraZoomGoal = FMath::Clamp(CameraBoom->TargetArmLength, CameraZoomMin, CameraZoomMax);

	// GAS: dono e avatar são o próprio personagem; habilidades são dadas pelo servidor
	AbilitySystem->InitAbilityActorInfo(this, this);
	if (HasAuthority())
	{
		if (ComboAbility) { AbilitySystem->GiveAbility(FGameplayAbilitySpec(ComboAbility, 1, INDEX_NONE, this)); }
		if (ChargeChakraAbility) { AbilitySystem->GiveAbility(FGameplayAbilitySpec(ChargeChakraAbility, 1, INDEX_NONE, this)); }
		// Um spec por jutsu da barra (a ficha vai como SourceObject; custo, cooldown e animação saem dela)
		const TSubclassOf<UGameplayAbility> JutsuClass = JutsuAbility ? JutsuAbility : TSubclassOf<UGameplayAbility>(UGA_NinjaJutsu::StaticClass());
		for (UNinjaJutsuData* Jutsu : JutsuLoadout)
		{
			if (Jutsu)
			{
				AbilitySystem->GiveAbility(FGameplayAbilitySpec(JutsuClass, 1, INDEX_NONE, Jutsu));
			}
		}
	}
	JutsuUseCounts.Init(0, FMath::Max(JutsuLoadout.Num(), JutsuSlotKeys.Num()));
	// Cada máquina que renderiza aquece os jutsus da barra escondidos, para o primeiro lançamento não travar
	for (const UNinjaJutsuData* Jutsu : JutsuLoadout)
	{
		ANinjaJutsuProjectile::Prewarm(this, Jutsu);
	}
	CameraBaseRotation = FollowCamera->GetRelativeRotation();
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UNinjaAttributeSet::GetHealthAttribute()).AddUObject(this, &AHypeWorldCharacter::OnHealthChanged);
	GetCharacterMovement()->ProcessRootMotionPreConvertToWorld.BindUObject(this, &AHypeWorldCharacter::ProcessAttackRootMotion);
}

void AHypeWorldCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateLean(DeltaSeconds);
	UpdateRunAnimRate();
	UpdateIdleBreak(DeltaSeconds);
	UpdateDoubleJump();
	// A queda assume a pose antes do pouso; montagens de impulso não prendem o corpo no ar.
	if (GetNinjaMovement()->IsFalling() && FVector::DotProduct(GetVelocity(), GetNinjaMovement()->GetGravityDirection()) > -75.f)
	{
		if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
		{
			for (UAnimMontage* Montage : {SuperJumpLaunchMontage, BackflipMontage, DoubleJumpMontage})
			{
				if (Montage && Anim->Montage_IsPlaying(Montage) && !Anim->Montage_GetIsStopped(Montage)
					&& Anim->Montage_GetPosition(Montage) >= Montage->GetPlayLength() * 0.8f)
				{
					Anim->Montage_Stop(0.25f, Montage);
				}
			}
		}
	}
	UpdateSuperJump(DeltaSeconds);
	UpdateLauncherHold(DeltaSeconds);
	UpdateAutoRun(DeltaSeconds);
	UpdateTurnInPlace(DeltaSeconds);
	UpdateCameraFollow(DeltaSeconds);
	UpdateCameraZoom(DeltaSeconds);
	UpdateGuard();
	UpdateBattleMode();
	UpdateCombatState(DeltaSeconds);
	UpdateJutsuSeals(DeltaSeconds);
	UpdateCameraShake(DeltaSeconds);

	// Aura: nível de carga e "no máximo" pelos atributos replicados, então todos veem igual
	if (ChakraAura && Attributes)
	{
		ChakraAura->SetAuraFull(bChargingChakra && Attributes->GetChakra() >= Attributes->GetMaxChakra() - 0.5f);
		ChakraAura->SetChargeLevel(Attributes->GetChakra() / FMath::Max(Attributes->GetMaxChakra(), 1.f));
	}
	// Chakra nos pés: mais forte grudado na parede/teto, correndo, e no voo do super pulo
	if (ChakraFeetFx && GetNinjaMovement())
	{
		const UNinjaMovementComponent* Move = GetNinjaMovement();
		ChakraFeetFx->SetIntensity(Move->IsOnChakraSurface() || Move->bInSuperJump ? 1.f : (Move->IsSprinting() ? 0.85f : 0.6f));
	}
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

void AHypeWorldCharacter::SetBattleToggle(bool bOn)
{
	bBattleToggle = bOn;
	if (!bOn)
	{
		LastBattleTime = -100.f;   // desligou: sai da batalha já (sem esperar os segundos depois do último golpe)
	}
	if (!HasAuthority())
	{
		ServerSetBattleToggle(bOn);
	}
}

void AHypeWorldCharacter::ServerSetBattleToggle_Implementation(bool bOn)
{
	SetBattleToggle(bOn);
}

void AHypeWorldCharacter::UpdateBattleMode()
{
	// Quem decide: o servidor e o próprio jogador (resposta imediata); os outros recebem pela rede
	if (!HasAuthority() && !IsLocallyControlled())
	{
		return;
	}
	bBattleMode = !bKnockedOut && (bBattleToggle || bGuarding || IsAttacking() || GetWorld()->GetTimeSeconds() - LastBattleTime < BattleModeDuration);
}

void AHypeWorldCharacter::PlayJumpMontage(UAnimMontage* Montage, float UpSpeed)
{
	if (!Montage) return;
	const float Gravity = FMath::Max(1.f, FMath::Abs(GetCharacterMovement()->GetGravityZ()));
	const float ApexTime = FMath::Max(0.45f, UpSpeed / Gravity);
	// Completa a acrobacia perto do ápice, antes de entregar a pose à queda.
	const float Rate = FMath::Clamp(Montage->GetPlayLength() / (ApexTime + 0.15f), 0.65f, 2.2f);
	GetMesh()->GlobalAnimRateScale = 1.f;
	PlayAnimMontage(Montage, Rate);
}

void AHypeWorldCharacter::UpdateDoubleJump()
{
	// No ar nos dois quadros e a velocidade para cima saltou de repente = pulo duplo (o pulo do chão e o da parede
	// começam fora do modo "caindo"; arremesso por golpe vem com atordoamento e não conta)
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	const bool bFalling = Move->IsFalling();
	const float VelocityZ = GetVelocity().Z;
	if (bFalling && bWasFalling && DoubleJumpMontage && !IsStunned() && !IsAttacking()
		&& VelocityZ - DoubleJumpLastVelZ > Move->JumpZVelocity * 0.5f)
	{
		PlayJumpMontage(DoubleJumpMontage, VelocityZ);
	}
	bWasFalling = bFalling;
	DoubleJumpLastVelZ = VelocityZ;
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

	const bool bWasDashing = PrevMovementMode == MOVE_Custom && (PreviousCustomMode == NMOVE_Dash || PreviousCustomMode == NMOVE_CombatDodge);
	const bool bWasWallRunning = PrevMovementMode == MOVE_Custom && PreviousCustomMode == NMOVE_WallRun;

	// Saiu de um modo: para a animação dele (o dash termina pela seção "End" da montagem, se ela tiver: aterrissagem)
	UAnimMontage* EndingDash = ActiveDashMontage ? ActiveDashMontage.Get() : DashMontage;
	if (bWasDashing && !NinjaMovement->IsDashing() && EndingDash)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		static const FName EndSection("End");
		// O impulso termina antes da recuperação visual. Deixe o salto lateral completar
		// o pouso enquanto walking já recebe o input; não cortar nem reiniciar a seção End.
		const bool bCompleteLateralLanding = PreviousCustomMode == NMOVE_Dash && NinjaMovement->IsDashStrafe()
			&& NinjaMovement->IsMovingOnGround() && AnimInstance && AnimInstance->Montage_IsPlaying(EndingDash)
			&& EndingDash->IsValidSectionName(EndSection);
		// Saiu correndo do dash: volta direto à corrida (aterrissagem parada deslizaria)
		const bool bKeepsRunning = NinjaMovement->IsMovingOnGround() && GetVelocity().Size2D() > NinjaMovement->MaxWalkSpeed * 1.5f;
		if (bCompleteLateralLanding)
		{
			const FName Current = AnimInstance->Montage_GetCurrentSection(EndingDash);
			if (Current != EndSection) AnimInstance->Montage_SetNextSection(Current, EndSection, EndingDash);
		}
		else if (NinjaMovement->IsMovingOnGround() && !bKeepsRunning && AnimInstance && AnimInstance->Montage_IsPlaying(EndingDash) && EndingDash->IsValidSectionName(EndSection))
		{
			if (AnimInstance->Montage_GetCurrentSection(EndingDash) != EndSection)
			{
				AnimInstance->Montage_JumpToSection(EndSection, EndingDash);
			}
		}
		else
		{
			BlendOutDashMontage(EndingDash);
		}
	}
	// Pousou: o mortal do pulo duplo (que segura a pose de descida) dá lugar ao pouso
	if (PrevMovementMode == MOVE_Falling && !NinjaMovement->IsFalling() && DoubleJumpMontage)
	{
		AirComboReadyUntil = -100.f;
		StopAnimMontage(DoubleJumpMontage);
	}
	if (PrevMovementMode == MOVE_Falling && !NinjaMovement->IsFalling() && SuperJumpLaunchMontage)
	{
		StopAnimMontage(SuperJumpLaunchMontage);
	}
	if (PrevMovementMode == MOVE_Falling && !NinjaMovement->IsFalling() && BackflipMontage)
	{
		StopAnimMontage(BackflipMontage);
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
		if (NinjaMovement->IsCombatDodging())
		{
			PlayCombatDodge(NinjaMovement->GetCombatDodgeDefinition());
			if (HasAuthority()) MulticastCombatDodge(NinjaMovement->GetCombatDodgeDefinition(), GetControlRotation().Yaw);
			return;
		}
		ActiveDashMontage = PickDashMontage(PrevMovementMode);
		if (ActiveDashMontage && NinjaMovement->IsDashStrafe())
		{
			const int32 EndIndex = ActiveDashMontage->GetSectionIndex(FName("End"));
			if (EndIndex != INDEX_NONE)
			{
				float StartTime, EndTime;
				ActiveDashMontage->GetSectionStartAndEndTime(EndIndex, StartTime, EndTime);
				NinjaMovement->MatchDashDuration(StartTime);
			}
		}
		PlayAnimMontage(ActiveDashMontage, NinjaMovement->DashAnimRate);
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

	UpdateAcrobatics(DeltaSeconds);

	// Malha gira em volta do centro da cápsula (origem): giro do pulo (em pé) * cambalhota do dash (para a frente) * inclinação
	const FQuat AcroQuat = FQuat(FVector::UpVector, FMath::DegreesToRadians(SpinTime >= 0.f
		? JumpSpinDegrees * FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(SpinTime / FMath::Max(JumpSpinDuration, 0.05f), 0.f, 1.f), 2.f) : 0.f))
		* FQuat(FVector::RightVector, FMath::DegreesToRadians(FlipAngle));   // positivo = cabeça vai para a frente e para baixo (medido)
	const FQuat LeanQuat(FVector::ForwardVector, FMath::DegreesToRadians(CurrentLean));
	GetMesh()->SetRelativeRotation(AcroQuat * LeanQuat * BaseMeshRotation.Quaternion());
	// No meio da cambalhota o corpo sobe um pouco (o empurrão das mãos): de ponta-cabeça a cabeça não entra no chão
	const float FlipLift = DashFlipDegrees > 0.f ? 20.f * FMath::Sin(PI * FMath::Clamp(FlipAngle / DashFlipDegrees, 0.f, 1.f)) : 0.f;
	GetMesh()->SetRelativeLocation(AcroQuat.RotateVector(BaseMeshLocation + FVector(0.f, CurrentWallOffset, 0.f)) + FVector(0.f, 0.f, FlipLift));
}

void AHypeWorldCharacter::UpdateAcrobatics(float DeltaSeconds)
{
	UNinjaMovementComponent* NinjaMovement = GetNinjaMovement();
	const float VelZ = GetVelocity().Z;

	// Dash: mergulha (primeiros 15%), gira de ponta-cabeça no meio e pousa no fim — como a estrela/handspring da referência
	if (NinjaMovement->IsDashing() && (NinjaMovement->IsCombatDodging() || NinjaMovement->IsDashStrafe()))
	{
		// O passo lateral já anima o corpo inteiro: não sobrepor a cambalhota frontal.
		DashElapsed = 0.f;
		FlipAngle = 0.f;
	}
	else if (NinjaMovement->IsDashing())
	{
		DashElapsed += DeltaSeconds;
		const float T = FMath::Clamp(DashElapsed / FMath::Max(NinjaMovement->GetDashDuration(), 0.05f), 0.f, 1.f);
		FlipAngle = DashFlipDegrees * FMath::SmoothStep(0.15f, 0.9f, T);
	}
	else
	{
		DashElapsed = 0.f;
		// Dash cortado no meio: termina a volta rápido (sem estalo de volta para a pose em pé)
		if (FlipAngle > 0.f)
		{
			FlipAngle += 900.f * DeltaSeconds;
			if (FlipAngle >= DashFlipDegrees)
			{
				FlipAngle = 0.f;
			}
		}
	}

	// Pulos (detectados pela velocidade: funciona também nos outros jogadores)
	const bool bOnGround = NinjaMovement->IsMovingOnGround();
	if (NinjaMovement->IsFalling() && VelZ > 300.f)
	{
		if (bWasOnGround)
		{
			SpinTime = 0.f;				// saiu do chão pulando: gira no próprio eixo
		}
		else if (VelZ - LastVelocityZ > 400.f)
		{
			bAirArmsSpread = true;		// segundo pulo (ou pulo da parede): braços abertos
		}
	}
	if (SpinTime >= 0.f)
	{
		SpinTime += DeltaSeconds;
		if (SpinTime >= JumpSpinDuration || bOnGround)
		{
			SpinTime = -1.f;
		}
	}
	if (bOnGround)
	{
		bAirArmsSpread = false;
	}
	bWasOnGround = bOnGround;
	LastVelocityZ = VelZ;
}

bool AHypeWorldCharacter::IsSpreadingArms() const
{
	return bAirArmsSpread;   // no dash os braços são da animação da estrela
}

void AHypeWorldCharacter::UpdateRunAnimRate()
{
	UNinjaMovementComponent* NinjaMovement = GetNinjaMovement();
	if (!NinjaMovement)
	{
		return;
	}

	// Dash: a montagem toca no tempo certo (a taxa da corrida/andar acelerava o voo, que passava do ponto e estalava)
	if (NinjaMovement->IsDashing() || IsPlayingDashMontage())
	{
		GetMesh()->GlobalAnimRateScale = 1.f;
		return;
	}

	float Rate = 1.f;
	const float Speed = GetSurfaceSpeed();   // na parede/teto também (velocidade ao longo do "chão" atual)
	if (NinjaMovement->IsWallRunning() && RunAnimNaturalSpeed > 0.f)
	{
		Rate = FMath::Clamp(Speed / RunAnimNaturalSpeed, 1.f, FMath::Max(1.f, MaxRunAnimRate));
	}
	else if (NinjaMovement->IsMovingOnGround() && !IsAttacking() && Speed > RunAnimMinSpeed && AnimSpeedCurve.Num() > 0)
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
		// De lado/de costas (corpo de frente para a câmera): cada direção tem a sua animação, com passo de tamanho diferente.
		// A velocidade natural é a mistura das direções, com o mesmo peso que o blend space dá a cada uma.
		const FVector VelDir = GetCharacterMovement()->ProjectToGravityFloor(GetVelocity()).GetSafeNormal();
		const FVector Facing = GetCharacterMovement()->ProjectToGravityFloor(GetActorForwardVector()).GetSafeNormal();
		const float Fwd = FVector::DotProduct(Facing, VelDir);
		const float Side = FVector::DotProduct(FVector::CrossProduct(Facing, VelDir), -GetCharacterMovement()->GetGravityDirection());   // + direita, - esquerda
		const bool bRunningSide = Speed >= StrafeRunMinSpeed;
		const float SideNatural = bRunningSide ? (Side >= 0.f ? RunStrafeRightNatural : RunStrafeLeftNatural) : WalkStrafeNatural;
		float WFwd = FMath::Max(0.f, Fwd), WBack = FMath::Max(0.f, -Fwd), WSide = FMath::Abs(Side);
		// Correndo, pesos como no BS_ANAP_Locomocao: até 45° só a corrida de frente (diagonais), 45°–90° frente→lado, 90°–180° lado→costas
		const float Angle = FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(Side), Fwd));
		if (bRunningSide && NinjaMovement->WantsToRun() && !NinjaMovement->WantsToSprint() && !NinjaMovement->IsFocusing())
		{
			// Mesmos pesos lineares da faixa de 700 cm/s: frente/lado/costas a cada 90 graus.
			WSide = Angle <= 90.f ? Angle / 90.f : (180.f - Angle) / 90.f;
			WFwd = FMath::Max(0.f, 1.f - Angle / 90.f);
			WBack = FMath::Max(0.f, (Angle - 90.f) / 90.f);
		}
		else if (bRunningSide && Angle <= 45.f)
		{
			WFwd = 1.f;
			WSide = 0.f;
			WBack = 0.f;
		}
		else if (bRunningSide && Angle > 90.f)
		{
			WBack = (Angle - 90.f) / 90.f;
			WFwd = 0.f;
			WSide = 1.f - WBack;
		}
		else if (bRunningSide)
		{
			WSide = (Angle - 45.f) / 45.f;
			WFwd = 1.f - WSide;
			WBack = 0.f;
		}
		const float BackNatural = bRunningSide ? RunBackNatural : WalkBackNatural;
		Natural = (WFwd * Natural + WSide * SideNatural + WBack * BackNatural) / FMath::Max(WFwd + WSide + WBack, 0.01f);
		Rate = FMath::Clamp(Speed / FMath::Max(1.f, Natural), 0.8f, FMath::Max(1.f, MaxRunAnimRate));   // um pouco mais lenta também vale (trote)
	}
	// O trote usa taxas próprias por amostra; não multiplicar novamente pela razão 700/velocidade natural.
	if (NinjaMovement->IsMovingOnGround() && NinjaMovement->WantsToRun() && !NinjaMovement->WantsToSprint()
		&& !NinjaMovement->IsFocusing() && !NinjaMovement->IsDashing() && !IsAttacking() && !NinjaMovement->HasCustomGravity())
	{
		Rate = FMath::Lerp(0.85f, 1.f, FMath::Clamp(Speed / FMath::Max(1.f, NinjaMovement->RunSpeed), 0.f, 1.f));
	}
	GetMesh()->GlobalAnimRateScale = FMath::FInterpTo(GetMesh()->GlobalAnimRateScale, Rate, GetWorld()->GetDeltaSeconds(), 10.f);
}

void AHypeWorldCharacter::UpdateCameraFollow(float DeltaSeconds)
{
	AController* PC = GetController();
	UNinjaMovementComponent* NinjaMovement = GetNinjaMovement();
	if (NinjaMovement && (NinjaMovement->IsDashing() || IsPlayingDashMontage()))
	{
		return; // O acompanhamento automático não muda a referência do desvio lateral.
	}
	if (!IsLocallyControlled() || !PC || !NinjaMovement || NinjaMovement->HasCustomGravity())
	{
		return;
	}

	const FVector Velocity2D = GetVelocity().GetSafeNormal2D() * GetVelocity().Size2D();
	// Correndo de lado ou na direção da câmera (de frente para ela) a câmera não persegue — senão o strafe vira círculo.
	// Só A/D (sem W) também não: o ninja vira e corre de lado na tela, em linha reta.
	const FVector Input2D = NinjaMovement->GetCurrentAcceleration().GetSafeNormal2D();
	const bool bSidewaysInput = !Input2D.IsNearlyZero() && FMath::Abs(FMath::FindDeltaAngleDegrees(PC->GetControlRotation().Yaw, Input2D.Rotation().Yaw)) > 60.f;
	const bool bTowardCamera = FMath::Abs(FMath::FindDeltaAngleDegrees(PC->GetControlRotation().Yaw, Velocity2D.Rotation().Yaw)) > 100.f;
	// Corrida ninja: o corpo vira para onde corre e a câmera volta para as costas (A/D fazem curva). Com S ele vira na hora e
	// corre na direção da câmera — aí a câmera não persegue (senão o "para trás" gira junto com ela e ele fica dando voltas)
	const bool bTowardCameraSprint = FMath::Abs(FMath::FindDeltaAngleDegrees(PC->GetControlRotation().Yaw, GetActorRotation().Yaw)) > 120.f;
	const bool bSprintFollow = NinjaMovement->IsSprinting() && Velocity2D.Size() > CameraFollowMinSpeed && !NinjaMovement->IsWallRunning() && !bTowardCameraSprint;
	const bool bFast = bSprintFollow || (Velocity2D.Size() > CameraFollowMinSpeed && !NinjaMovement->IsWallRunning() && !NinjaMovement->IsStrafeRunning() && !NinjaMovement->IsFocusing() && !bTowardCamera && !bSidewaysInput);
	const bool bMouseIdle = GetWorld()->GetTimeSeconds() - LastLookInputTime > CameraFollowDelay;
	// Golpeando (e logo depois): a câmera é só do jogador — o avanço do golpe não pode girá-la (pareceria mira travada)
	const bool bFighting = IsAttacking() || GetWorld()->GetTimeSeconds() - LastAttackEndTime < 0.6f;
	if (!bFast || !bMouseIdle || bFighting)
	{
		return;
	}

	// Gira só o yaw da câmera em direção à direção da corrida, mais forte quanto mais rápido
	FRotator Control = PC->GetControlRotation();
	const float SpeedFactor = FMath::Clamp(Velocity2D.Size() / 1200.f, 0.3f, 1.f);
	const float TargetYaw = bSprintFollow ? GetActorRotation().Yaw : Velocity2D.Rotation().Yaw;   // corrida ninja: atrás do corpo
	const float FollowSpeed = bSprintFollow ? SprintCameraFollowSpeed : CameraFollowSpeed * SpeedFactor;   // corrida ninja: acompanha as curvas rápidas
	Control.Yaw = FMath::FInterpTo(Control.Yaw, Control.Yaw + FMath::FindDeltaAngleDegrees(Control.Yaw, TargetYaw), DeltaSeconds, FollowSpeed);
	PC->SetControlRotation(Control);
}

void AHypeWorldCharacter::UpdateTurnInPlace(float DeltaSeconds)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const float Speed = GetVelocity().Size2D();

	// Rapidez de giro: virada em andamento > corrida ninja (curva aberta) / trote de combate (vira quase na hora) > normal
	if (TurnTimeLeft > 0.f)
	{
		TurnTimeLeft -= DeltaSeconds;
	}
	else if (Speed > CameraFollowMinSpeed)
	{
		const UNinjaMovementComponent* NinjaMovement = GetNinjaMovement();
		Move->RotationRate.Yaw = NinjaMovement && NinjaMovement->WantsToSprint() ? SprintRotationRate : CombatRunRotationRate;
	}
	else
	{
		Move->RotationRate.Yaw = DefaultRotationRate;
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
	DOREPLIFETIME_CONDITION(AHypeWorldCharacter, bGuarding, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AHypeWorldCharacter, bBattleMode, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AHypeWorldCharacter, SubstitutionCharges, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AHypeWorldCharacter, FightingStyleIndex, COND_SkipOwner);
	DOREPLIFETIME(AHypeWorldCharacter, EquippedWeapon);
	DOREPLIFETIME(AHypeWorldCharacter, EnabledPassives);
	DOREPLIFETIME_CONDITION(AHypeWorldCharacter, bChargingSuperJump, COND_SkipOwner);
	DOREPLIFETIME(AHypeWorldCharacter, bInvulnerable);
}

bool AHypeWorldCharacter::IsAttacking() const
{
	const FGameplayAbilitySpec* Spec = ComboAbility ? AbilitySystem->FindAbilitySpecFromClass(ComboAbility) : nullptr;
	return Spec && Spec->IsActive();
}

bool AHypeWorldCharacter::IsStunned() const
{
	return AbilitySystem && AbilitySystem->HasMatchingGameplayTag(TAG_Ninja_State_Stunned);
}

bool AHypeWorldCharacter::IsGuarding() const
{
	return bGuarding && !bKnockedOut && !IsStunned();
}

int32 AHypeWorldCharacter::GetComboHitCount() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() - LastComboHitTime <= ComboCounterTime ? ComboHits : 0;
}

// Ids dos golpes: 0..99 combo leve, 100+ fortes, 200 correndo, 300 no ar
namespace NinjaAttackId
{
	constexpr int32 Heavy = 100;
	constexpr int32 Sprint = 200;
	constexpr int32 Air = 300;
	constexpr int32 Launcher = 400;
}

const FNinjaAttack* AHypeWorldCharacter::GetAttack(int32 AttackId) const
{
	// Estilo de luta: leve = combo*16 + posição; forte/correndo/ar = base + índice da variação
	if (const UNinjaFightingStyle* Style = GetFightingStyle())
	{
		auto At = [](const TArray<FNinjaAttack>& List, int32 Index) { return List.IsValidIndex(Index) && List[Index].Montage ? &List[Index] : nullptr; };
		if (AttackId >= 0 && AttackId < NinjaAttackId::Heavy)
		{
			return Style->Combos.IsValidIndex(AttackId / 16) ? At(Style->Combos[AttackId / 16].Attacks, AttackId % 16) : nullptr;
		}
		if (AttackId >= NinjaAttackId::Heavy && AttackId < NinjaAttackId::Sprint)
		{
			return At(Style->Finishers, AttackId - NinjaAttackId::Heavy);
		}
		if (AttackId >= NinjaAttackId::Sprint && AttackId < NinjaAttackId::Air)
		{
			return At(Style->SprintAttacks, AttackId - NinjaAttackId::Sprint);
		}
		if (AttackId >= NinjaAttackId::Air && AttackId < NinjaAttackId::Launcher)
		{
			return At(Style->AirAttacks, AttackId - NinjaAttackId::Air);
		}
		if (AttackId >= NinjaAttackId::Launcher && AttackId < NinjaAttackId::Launcher + 100)
		{
			return At(Style->Launchers, AttackId - NinjaAttackId::Launcher);
		}
		return nullptr;
	}
	if (AttackId >= 0 && AttackId < NinjaAttackId::Heavy)
	{
		return LightCombo.IsValidIndex(AttackId) ? &LightCombo[AttackId] : nullptr;
	}
	if (AttackId >= NinjaAttackId::Heavy && AttackId < NinjaAttackId::Sprint)
	{
		return HeavyAttacks.IsValidIndex(AttackId - NinjaAttackId::Heavy) ? &HeavyAttacks[AttackId - NinjaAttackId::Heavy] : nullptr;
	}
	if (AttackId == NinjaAttackId::Sprint)
	{
		return SprintAttack.Montage ? &SprintAttack : nullptr;
	}
	if (AttackId == NinjaAttackId::Air)
	{
		return AirAttack.Montage ? &AirAttack : nullptr;
	}
	return nullptr;
}

int32 AHypeWorldCharacter::ChooseAttack(ENinjaAttackInput Input) const
{
	// Emenda no golpe atual, ou no que acabou de terminar (se a janela de combo dele chegou a abrir)
	const int32 ChainId = (IsAttacking() && CurrentAttackId != INDEX_NONE) ? CurrentAttackId
		: (GetWorld()->GetTimeSeconds() <= ChainExpireTime ? LastChainAttackId : INDEX_NONE);
	const bool bChaining = ChainId != INDEX_NONE;
	const bool bLightChain = bChaining && ChainId < NinjaAttackId::Heavy;

	// Estilo de luta: combos sorteados (nunca o mesmo duas vezes seguidas), finalizador/corrida/ar sorteados
	if (const UNinjaFightingStyle* Style = GetFightingStyle())
	{
		auto NewCombo = [this, Style]()
		{
			if (Style->Combos.Num() == 0)
			{
				return int32(INDEX_NONE);
			}
			LastComboString = PickVariant(Style->Combos.Num(), LastComboString);
			return LastComboString * 16;
		};
		if (Input == ENinjaAttackInput::Launcher)
		{
			if (Style->Launchers.Num() == 0 || !CanStartLauncher())
			{
				return INDEX_NONE;
			}
			LastLauncher = PickVariant(Style->Launchers.Num(), LastLauncher);
			return NinjaAttackId::Launcher + LastLauncher;
		}
		// No ar: o combo aéreo segue a sequência AirAttacks (depois do lançador ou de um golpe aéreo, o próximo da lista)
		if (Input == ENinjaAttackInput::Light && GetCharacterMovement()->IsFalling() && Style->AirAttacks.Num() > 0)
		{
			const bool bAirChain = bChaining && ChainId >= NinjaAttackId::Air && ChainId < NinjaAttackId::Launcher;
			const int32 Next = bAirChain ? ChainId - NinjaAttackId::Air + 1 : 0;
			return Style->AirAttacks.IsValidIndex(Next) ? NinjaAttackId::Air + Next : INDEX_NONE;
		}
		if (Input == ENinjaAttackInput::Heavy)
		{
			if (Style->Finishers.Num() == 0 || (bChaining && !bLightChain))
			{
				return INDEX_NONE;
			}
			LastFinisher = PickVariant(Style->Finishers.Num(), LastFinisher);
			return NinjaAttackId::Heavy + LastFinisher;
		}
		if (bLightChain)
		{
			const int32 S = ChainId / 16, P = ChainId % 16;
			return (Style->Combos.IsValidIndex(S) && P + 1 < Style->Combos[S].Attacks.Num()) ? ChainId + 1 : NewCombo();
		}
		if (bChaining)
		{
			return NewCombo();
		}
		if (GetCharacterMovement()->IsMovingOnGround() && GetVelocity().Size2D() >= SprintAttackMinSpeed && Style->SprintAttacks.Num() > 0)
		{
			LastSprintAttack = PickVariant(Style->SprintAttacks.Num(), LastSprintAttack);
			return NinjaAttackId::Sprint + LastSprintAttack;
		}
		return NewCombo();
	}

	if (Input == ENinjaAttackInput::Heavy)
	{
		// Forte sozinho, ou finalizador que depende de em que ponto do combo leve ele entra; forte não emenda em forte
		if (HeavyAttacks.Num() == 0 || (bChaining && !bLightChain))
		{
			return INDEX_NONE;
		}
		return NinjaAttackId::Heavy + (bLightChain ? FMath::Min(ChainId + 1, HeavyAttacks.Num() - 1) : 0);
	}

	if (LightCombo.Num() == 0)
	{
		return INDEX_NONE;
	}
	if (bLightChain)
	{
		return (ChainId + 1) % LightCombo.Num();
	}
	if (bChaining)
	{
		return 0;   // depois de um forte / de corrida / aéreo: recomeça o combo leve
	}
	if (GetCharacterMovement()->IsFalling() && AirAttack.Montage)
	{
		return NinjaAttackId::Air;
	}
	if (GetCharacterMovement()->IsMovingOnGround() && GetVelocity().Size2D() >= SprintAttackMinSpeed && SprintAttack.Montage)
	{
		return NinjaAttackId::Sprint;
	}
	return 0;
}

void AHypeWorldCharacter::DoAttackPressed()
{
	if (!IsCastingJutsu())
	{
		StartAttack(ENinjaAttackInput::Light);
	}
}

void AHypeWorldCharacter::DoHeavyAttackPressed()
{
	if (!IsCastingJutsu())
	{
		StartAttack(ENinjaAttackInput::Heavy);
	}
}

void AHypeWorldCharacter::StartAttack(ENinjaAttackInput Input)
{
	if (!TryCancelDodgeForAction()) return;
	if (bKnockedOut || bChargingChakra || IsStunned() || !ComboAbility)
	{
		return;
	}
	const AHypeWorldPlayerController* PC = Cast<AHypeWorldPlayerController>(GetController());
	if (PC && PC->IsCombatMenuOpen())
	{
		return;   // menu de combate aberto: o clique é do menu
	}
	// Modo de batalha, no chão, segurando Espaço + ataque = lançador (começa o combo aéreo)
	if (Input == ENinjaAttackInput::Light && bSpaceHeld && CanStartLauncher())
	{
		const UNinjaFightingStyle* Style = GetFightingStyle();
		if (Style && Style->Launchers.Num() > 0)
		{
			Input = ENinjaAttackInput::Launcher;
			bLauncherHoldPending = false;   // já virou lançador: soltar o Espaço depois não pula
		}
	}
	if (Input == ENinjaAttackInput::Launcher && !CanStartLauncher()) return;
	if (IsAttacking() && !bComboWindowOpen)
	{
		// No meio do golpe: guarda o clique; ele vale quando a janela do próximo golpe abrir (CheckCombo).
		// Lançador guardado tem prioridade: cliques depois dele não o apagam
		if (BufferedInput == ENinjaAttackInput::Launcher && Input != ENinjaAttackInput::Launcher)
		{
			return;
		}
		BufferedInput = Input;
		CachedAttackInputTime = GetWorld()->GetTimeSeconds();
		return;
	}
	const int32 AttackId = ChooseAttack(Input);
	FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromClass(ComboAbility);
	if (AttackId == INDEX_NONE || !Spec)
	{
		return;
	}
	BufferedInput = ENinjaAttackInput::None;
	bComboWindowOpen = false;

	// O id vai no evento: o servidor recebe junto com a ativação e toca o mesmo golpe
	FGameplayEventData Payload;
	Payload.EventTag = TAG_Ninja_Event_Attack;
	Payload.Instigator = this;
	Payload.EventMagnitude = AttackId;
	AbilitySystem->TriggerAbilityFromGameplayEvent(Spec->Handle, AbilitySystem->AbilityActorInfo.Get(), TAG_Ninja_Event_Attack, &Payload, *AbilitySystem);
}

float AHypeWorldCharacter::GetAirComboCooldownRemaining() const
{
	return FMath::Max(0.f, NextLauncherTime - GetWorld()->GetTimeSeconds());
}

float AHypeWorldCharacter::GetSuperJumpCooldownRemaining() const
{
	return FMath::Max(0.f, NextSuperJumpTime - GetWorld()->GetTimeSeconds());
}

bool AHypeWorldCharacter::CanStartLauncher() const
{
	const float Now = GetWorld()->GetTimeSeconds();
	return CanAct() && IsInBattleMode() && GetCharacterMovement()->IsMovingOnGround()
		&& Now >= NextLauncherTime && Now <= GroundComboReadyUntil;
}

bool AHypeWorldCharacter::CanActivateAttack(int32 AttackId) const
{
	if (AttackId >= NinjaAttackId::Launcher && AttackId < NinjaAttackId::Launcher + 100) return CanStartLauncher();
	if (AttackId >= NinjaAttackId::Air && AttackId < NinjaAttackId::Launcher)
		return GetCharacterMovement()->IsFalling() && GetWorld()->GetTimeSeconds() <= AirComboReadyUntil;
	return true;
}

void AHypeWorldCharacter::BeginAttack(int32 AttackId)
{
	if (!CanActivateAttack(AttackId) || !TryCancelDodgeForAction()) return;
	const FNinjaAttack* Attack = GetAttack(AttackId);
	if (!Attack || !Attack->Montage)
	{
		return;
	}
	CurrentAttackId = AttackId;
	LastChainAttackId = INDEX_NONE;
	LastBattleTime = GetWorld()->GetTimeSeconds();
	AttackStartTime = LastBattleTime;
	if (AttackId >= NinjaAttackId::Launcher && AttackId < NinjaAttackId::Launcher + 100)
	{
		NextLauncherTime = AttackStartTime + AirComboCooldown;
		GroundComboReadyUntil = -100.f;
		AirComboReadyUntil = AttackStartTime + 6.f;
	}
	else if (AttackId >= NinjaAttackId::Air && AttackId < NinjaAttackId::Launcher)
	{
		AirComboReadyUntil = AttackStartTime + 2.f;
	}
	AttackHitIndex = 0;
	bComboWindowOpen = false;

	// Quantos acertos o golpe tem
	AttackHitCount = 0;
	for (const FAnimNotifyEvent& Event : Attack->Montage->Notifies)
	{
		AttackHitCount += Cast<UAnimNotify_DoAttackTrace>(Event.Notify) ? 1 : 0;
	}
	AttackHitCount = FMath::Max(1, AttackHitCount);

	// Próximo golpe do combo aéreo: continua suspenso enquanto prepara o golpe
	if (AttackId >= NinjaAttackId::Air && AttackId < NinjaAttackId::Launcher && GetNinjaMovement() && GetNinjaMovement()->IsAirSuspended())
	{
		GetNinjaMovement()->SuspendInAir(AirSuspendTime);
	}

	// Sem mira automática: o golpe vai para onde a câmera olha (o corpo vira até lá rápido, sem estalo — ver o movimento)
	AttackFacingYaw = GetController() ? GetController()->GetControlRotation().Yaw : GetActorRotation().Yaw;

	// Durante golpes o avanço da animação move o personagem (ver ProcessAttackRootMotion)
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	}
}

void AHypeWorldCharacter::EndAttack(bool bWasCancelled)
{
	// Golpe que terminou sem ser interrompido: o próximo clique ainda emenda por um instante depois do fim
	LastChainAttackId = bWasCancelled ? INDEX_NONE : CurrentAttackId;
	ChainExpireTime = GetWorld()->GetTimeSeconds() + ComboChainGrace;
	if (!bWasCancelled && CurrentAttackId >= 0 && CurrentAttackId < NinjaAttackId::Heavy
		&& GroundComboReadyUntil >= GetWorld()->GetTimeSeconds()) GroundComboReadyUntil = ChainExpireTime;
	if (bWasCancelled)
	{
		BufferedInput = ENinjaAttackInput::None;
	}
	else if (IsLocallyControlled() && BufferedInput != ENinjaAttackInput::None)
	{
		// Clique guardado e a janela não chegou a abrir (o golpe acabou no mesmo quadro): vale agora
		GetWorldTimerManager().SetTimerForNextTick(this, &AHypeWorldCharacter::ConsumeBufferedAttack);
	}
	CurrentAttackId = INDEX_NONE;
	bComboWindowOpen = false;
	LastAttackEndTime = GetWorld()->GetTimeSeconds();
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}
}

FTransform AHypeWorldCharacter::ProcessAttackRootMotion(const FTransform& InRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds)
{
	const FNinjaAttack* Attack = GetAttack(CurrentAttackId);
	if (!Attack)
	{
		return InRootMotion;
	}
	// Só o avanço para a frente (as animações do Mixamo deslizam de lado), escalado
	const FVector CompForward = GetMesh()->GetComponentTransform().InverseTransformVectorNoScale(GetActorForwardVector()).GetSafeNormal();
	float Advance = FVector::DotProduct(InRootMotion.GetTranslation(), CompForward) * Attack->RootMotionScale;

	// Corpo a corpo: com alguém logo à frente o golpe para de avançar (só física — não mira nem puxa ninguém)
	if (Advance > 0.f && HasBodyAhead())
	{
		Advance = 0.f;
	}
	FTransform Result = InRootMotion;
	Result.SetTranslation(CompForward * Advance);
	return Result;
}

bool AHypeWorldCharacter::HasBodyAhead() const
{
	TArray<FOverlapResult> Overlaps;
	const float Range = GetSimpleCollisionRadius() * 2.f + AttackBodySpacing;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NinjaAttackSpacing), false, this);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Range), Params);
	for (const FOverlapResult& O : Overlaps)
	{
		const AHypeWorldCharacter* Other = Cast<AHypeWorldCharacter>(O.GetActor());
		if (!Other || Other == this)
		{
			continue;
		}
		const FVector To = Other->GetActorLocation() - GetActorLocation();
		const float Gap = To.Size2D() - GetSimpleCollisionRadius() - Other->GetSimpleCollisionRadius();
		if (Gap < AttackBodySpacing && FVector::DotProduct(To.GetSafeNormal2D(), GetActorForwardVector()) > 0.5f)
		{
			return true;
		}
	}
	return false;
}

void AHypeWorldCharacter::CheckCombo()
{
	// AnimNotify perto do fim de cada golpe: abre a janela do próximo. Quem decide é o jogador (o servidor recebe pela habilidade)
	if (!IsLocallyControlled())
	{
		return;
	}
	bComboWindowOpen = IsAttacking();
	if (BufferedInput != ENinjaAttackInput::None)
	{
		// No próximo quadro: trocar de montagem dentro de um AnimNotify não é seguro
		GetWorldTimerManager().SetTimerForNextTick(this, &AHypeWorldCharacter::ConsumeBufferedAttack);
	}
}

void AHypeWorldCharacter::ConsumeBufferedAttack()
{
	const ENinjaAttackInput Input = BufferedInput;
	BufferedInput = ENinjaAttackInput::None;
	if (Input != ENinjaAttackInput::None && GetWorld()->GetTimeSeconds() - CachedAttackInputTime <= ComboInputWindow)
	{
		bComboWindowOpen = true;   // o clique foi dado durante o golpe: emenda mesmo que a janela já tenha passado
		StartAttack(Input);
	}
}

void AHypeWorldCharacter::DoAttackTrace(FName DamageSourceBone)
{
	// Servidor: acerto de verdade (dano). Jogador: só o efeito (pausa no impacto e contador de combo).
	const FNinjaAttack* Attack = GetAttack(CurrentAttackId);
	if (!Attack || !(HasAuthority() || IsLocallyControlled()))
	{
		return;
	}
	const bool bFinalHit = ++AttackHitIndex >= AttackHitCount;
	// Habilita o lançador depois de executar um soco, mesmo sem acertar um alvo.
	if (CurrentAttackId >= 0 && CurrentAttackId < NinjaAttackId::Heavy && GetCharacterMovement()->IsMovingOnGround())
	{
		GroundComboReadyUntil = GetWorld()->GetTimeSeconds()
			+ Attack->Montage->GetPlayLength() / FMath::Max(0.1f, Attack->PlayRate) + ComboChainGrace;
	}

	const FVector Start = GetActorLocation() + GetActorForwardVector() * 30.f;
	const FVector End = Start + GetActorForwardVector() * Attack->Reach;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NinjaMelee), false, this);
	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Attack->Radius), Params);

	TSet<AActor*> Done;
	bool bHitSomething = false;
	for (const FHitResult& Hit : Hits)
	{
		AHypeWorldCharacter* Target = Cast<AHypeWorldCharacter>(Hit.GetActor());
		if (!Target || Target == this || Done.Contains(Target) || Target->bKnockedOut)
		{
			continue;
		}
		Done.Add(Target);

		// Desvio: quem está no dash fica invulnerável
		const UNinjaMovementComponent* TargetMove = Target->GetNinjaMovement();
		if ((TargetMove && TargetMove->IsDashing()) || Target->IsInvulnerable())
		{
			continue;
		}
		bHitSomething = true;
		if (HasAuthority())
		{
			ApplyHit(Target, *Attack, bFinalHit, Hit);
		}
	}

	if (bHitSomething)
	{
		// Lançador: o atacante sobe atrás de quem foi lançado. Golpe no ar: fica suspenso no ar (o combo aéreo continua);
		// o último golpe aéreo solta: o atacante cai normal
		if (IsLocallyControlled() || HasAuthority())
		{
			if (CurrentAttackId >= NinjaAttackId::Launcher && CurrentAttackId < NinjaAttackId::Launcher + 100 && bFinalHit)
			{
				LaunchCharacter(FVector(0.f, 0.f, LauncherFollowUpSpeed), false, true);
			}
			else if (bFinalHit && IsAirFinisher(*Attack, CurrentAttackId))
			{
				GetNinjaMovement()->EndAirSuspend();
			}
			else if (GetCharacterMovement()->IsFalling())
			{
				LaunchCharacter(FVector(0.f, 0.f, AirHoverSpeed), false, true);
				GetNinjaMovement()->SuspendInAir(AirSuspendTime);
			}
		}
		HitStop();
		if (IsLocallyControlled())
		{
			const float Now = GetWorld()->GetTimeSeconds();
			ComboHits = Now - LastComboHitTime <= ComboCounterTime ? ComboHits + 1 : 1;
			LastComboHitTime = Now;
		}
		if (HasAuthority())
		{
			Attributes->SetChakra(FMath::Min(Attributes->GetChakra() + ChakraPerHit, Attributes->GetMaxChakra()));
		}
	}
}

void AHypeWorldCharacter::ApplyHit(AHypeWorldCharacter* Target, const FNinjaAttack& Attack, bool bFinalHit, const FHitResult& Hit)
{
	const FVector Push = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	ENinjaHitReaction Reaction = bFinalHit ? Attack.Reaction : ENinjaHitReaction::Light;
	float Damage = Attack.Damage;
	float Stun = Attack.Stun;
	float Knockback = bFinalHit ? Attack.Knockback : FMath::Min(Attack.Knockback, 120.f);
	float Launch = bFinalHit ? Attack.Launch : 0.f;

	// Defesa: só segura golpes de frente e no chão (no ar não dá para defender); golpe forte quebra.
	// Lançador quebra a defesa e lança do mesmo jeito (o combo aéreo sempre começa)
	const bool bFromFront = FVector::DotProduct(Target->GetActorForwardVector().GetSafeNormal2D(), -Push) > 0.2f;
	const bool bLauncherBreak = Attack.bBreaksGuard && Attack.Reaction == ENinjaHitReaction::Launch && bFinalHit;
	if (Target->IsGuarding() && bFromFront && Target->GetCharacterMovement()->IsMovingOnGround() && !bLauncherBreak)
	{
		if (Attack.bBreaksGuard)
		{
			Reaction = ENinjaHitReaction::GuardBreak;
			Damage *= 0.5f;
			Stun = GuardBreakStun;
			Knockback = 250.f;
		}
		else
		{
			Reaction = ENinjaHitReaction::Block;
			Damage *= GuardDamageScale;
			Stun = BlockStun;
			Knockback = 120.f;
		}
		Launch = 0.f;
	}

	// Anti-combo infinito: golpes seguidos no mesmo alvo atordoam e tiram menos; no limite ele é arremessado e fica invulnerável
	bool bComboBreak = false;
	if (Reaction != ENinjaHitReaction::Block)
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (Now < Target->InvulnerableUntil)
		{
			return;
		}
		const bool bInCombo = Target->IsStunned() || Now - Target->LastTakenHitTime < 0.35f;
		if (!bInCombo)
		{
			Target->TakenComboHits = 0;
			Target->TakenComboStart = Now;
		}
		Target->TakenComboHits++;
		Target->LastTakenHitTime = Now;
		Target->LastAttacker = this;
		const int32 Previous = Target->TakenComboHits - 1;
		Stun *= FMath::Max(MinHitstunScale, FMath::Pow(HitstunDecay, float(Previous)));
		Damage *= FMath::Max(MinComboDamageScale, 1.f - ComboDamageDecay * Previous);
		if (Target->TakenComboHits >= MaxComboHits || Now - Target->TakenComboStart >= MaxComboTime)
		{
			bComboBreak = true;
			Reaction = ENinjaHitReaction::Knockback;
			Knockback = FMath::Max(Knockback, 750.f);
			Launch = FMath::Max(Launch, 350.f);
			Stun = FMath::Max(Stun, 0.6f);
			Target->InvulnerableUntil = Now + ComboBreakInvulnerability;
			Target->TakenComboHits = 0;
		}
	}

	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();

	// Levar golpe (sem defender) interrompe o que ele estava fazendo: combo, carga de chakra
	if (Reaction != ENinjaHitReaction::Block)
	{
		TargetASC->CancelAbilities();
	}

	FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
	Context.AddHitResult(Hit);
	FGameplayEffectSpecHandle DamageSpec = AbilitySystem->MakeOutgoingSpec(UGE_NinjaDamage::StaticClass(), 1.f, Context);
	DamageSpec.Data->SetSetByCallerMagnitude(NinjaSetByCaller::Damage, Damage);
	AbilitySystem->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);

	if (Stun > 0.f && !Target->bKnockedOut)
	{
		FGameplayEffectSpecHandle StunSpec = AbilitySystem->MakeOutgoingSpec(UGE_NinjaStun::StaticClass(), 1.f, Context);
		StunSpec.Data->SetSetByCallerMagnitude(NinjaSetByCaller::Duration, Stun);
		AbilitySystem->ApplyGameplayEffectSpecToTarget(*StunSpec.Data.Get(), TargetASC);
	}

	// Combo aéreo: apanhando no ar fica suspenso junto com o atacante, com pouco empurrão. O último golpe aéreo arremessa
	// para longe e deixa invulnerável até depois de cair (sem golpes infinitos no chão)
	bool bAirJuggle = false;
	bool bThrownAway = bComboBreak;
	if (Reaction != ENinjaHitReaction::Block && !bComboBreak)
	{
		if (bFinalHit && IsAirFinisher(Attack, CurrentAttackId))
		{
			bThrownAway = true;
			Target->InvulnerableUntil = GetWorld()->GetTimeSeconds() + AirFinisherInvulnerability;
			Target->TakenComboHits = 0;
		}
		else if (Target->GetCharacterMovement()->IsFalling())
		{
			bAirJuggle = true;
			Knockback = FMath::Min(Knockback, AirJuggleKnockback);
			Launch = FMath::Max(Launch, AirJuggleSpeed);
		}
	}
	// A reação antes do impulso: a suspensão (ou o voo sem frear) já vale no próximo movimento
	Target->MulticastHitReaction(Reaction, -Push, bComboBreak, bAirJuggle, bThrownAway);
	if (Knockback > 0.f || !FMath::IsNearlyZero(Launch))
	{
		Target->LaunchCharacter(Push * Knockback + FVector(0.f, 0.f, Launch), true, !FMath::IsNearlyZero(Launch));
	}
}

bool AHypeWorldCharacter::IsAirFinisher(const FNinjaAttack& Attack, int32 AttackId) const
{
	return AttackId >= NinjaAttackId::Air && AttackId < NinjaAttackId::Launcher && Attack.Reaction == ENinjaHitReaction::Knockback;
}

void AHypeWorldCharacter::MulticastHitReaction_Implementation(ENinjaHitReaction Reaction, FVector_NetQuantizeNormal FromDirection, bool bComboBreak, bool bAirJuggle, bool bThrownAway)
{
	if (bKnockedOut)
	{
		return;
	}
	if (UNinjaMovementComponent* Move = GetNinjaMovement())
	{
		if (bAirJuggle)
		{
			Move->SuspendInAir(AirSuspendTime);
		}
		else if (bThrownAway)
		{
			Move->EndAirSuspend();
			Move->bInSuperJump = true;   // voa sem frear no ar até cair (acaba ao pousar)
		}
	}
	LastBattleTime = GetWorld()->GetTimeSeconds();   // apanhou: entra em modo de batalha
	const FVector Head = GetActorLocation() + FVector(0.f, 0.f, GetSimpleCollisionHalfHeight() + 30.f);
	if (Reaction == ENinjaHitReaction::Block)
	{
		ANinjaHitFlash::Spawn(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 40.f) + FVector(FromDirection) * 35.f, HitFlashMaterial, FLinearColor(0.35f, 0.7f, 1.f));
		ANinjaDamageNumber::Spawn(GetWorld(), Head, TEXT("DEFESA"), FLinearColor(0.45f, 0.8f, 1.f), 28.f);
		return;
	}

	// Vira para quem bateu (a animação de reação é de frente)
	if (HasAuthority() || IsLocallyControlled())
	{
		SetActorRotation(FRotator(0.f, FVector(FromDirection).Rotation().Yaw, 0.f));
	}
	if (Reaction == ENinjaHitReaction::GuardBreak)
	{
		ANinjaDamageNumber::Spawn(GetWorld(), Head, TEXT("DEFESA QUEBRADA"), FLinearColor(1.f, 0.45f, 0.15f), 32.f);
	}
	if (bComboBreak)
	{
		ANinjaDamageNumber::Spawn(GetWorld(), Head + FVector(0.f, 0.f, 25.f), TEXT("COMBO QUEBRADO"), FLinearColor(0.55f, 0.85f, 1.f), 30.f);
	}
	UAnimMontage* Montage = nullptr;
	if (Reaction == ENinjaHitReaction::Launch && LaunchReactMontage)
	{
		Montage = LaunchReactMontage;
	}
	else if (GetCharacterMovement()->IsFalling() && AirHitReactMontages.Num() > 0)
	{
		Montage = AirHitReactMontages[FMath::RandRange(0, AirHitReactMontages.Num() - 1)];
	}
	else if (Reaction != ENinjaHitReaction::Light && HeavyHitReactMontage)
	{
		Montage = HeavyHitReactMontage;
	}
	else if (HitReactMontages.Num() > 0)
	{
		Montage = HitReactMontages[FMath::RandRange(0, HitReactMontages.Num() - 1)];
	}
	if (Montage)
	{
		PlayAnimMontage(Montage, Reaction == ENinjaHitReaction::Light ? 1.3f : 1.f);
	}
}

bool AHypeWorldCharacter::IsPlayingAttackMontage() const
{
	const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	const UAnimMontage* Montage = AnimInstance ? AnimInstance->GetCurrentActiveMontage() : nullptr;
	if (!Montage)
	{
		return false;
	}
	auto Uses = [Montage](const FNinjaAttack& Attack) { return Attack.Montage == Montage; };
	if (const UNinjaFightingStyle* Style = GetFightingStyle())
	{
		for (const FNinjaComboString& Combo : Style->Combos)
		{
			if (Combo.Attacks.ContainsByPredicate(Uses))
			{
				return true;
			}
		}
		if (Style->Finishers.ContainsByPredicate(Uses) || Style->SprintAttacks.ContainsByPredicate(Uses) || Style->AirAttacks.ContainsByPredicate(Uses) || Style->Launchers.ContainsByPredicate(Uses))
		{
			return true;
		}
	}
	return LightCombo.ContainsByPredicate(Uses) || HeavyAttacks.ContainsByPredicate(Uses) || Uses(SprintAttack) || Uses(AirAttack);
}

void AHypeWorldCharacter::HitStop()
{
	CustomTimeDilation = 0.05f;
	GetWorldTimerManager().SetTimer(HitStopTimer, [this]() { CustomTimeDilation = 1.f; }, HitStopDuration, false);
}

// ---------------------------------------------------------------------------------------------------------------------
// Defesa
// ---------------------------------------------------------------------------------------------------------------------

void AHypeWorldCharacter::UpdateGuard()
{
	if (!IsLocallyControlled())
	{
		return;
	}
	const UNinjaMovementComponent* NinjaMovement = GetNinjaMovement();
	const bool bNewGuarding = bWantsGuard && NinjaMovement && NinjaMovement->IsMovingOnGround() && !NinjaMovement->IsRunning()
		&& !NinjaMovement->IsSprinting() && !IsAttacking() && !bChargingChakra && !bKnockedOut;
	if (bNewGuarding != bGuarding)
	{
		bGuarding = bNewGuarding;
		if (!HasAuthority())
		{
			ServerSetGuarding(bNewGuarding);
		}
	}
}

void AHypeWorldCharacter::ServerSetGuarding_Implementation(bool bNewGuarding)
{
	bGuarding = bNewGuarding;
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

		// A reação (animação, virar para quem bateu) vem do MulticastHitReaction, que sabe o tipo do golpe
		if (Data.NewValue <= 0.f)
		{
			OnKnockedOut();
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
	bBattleToggle = false;   // nocauteado: o modo de batalha ligado pelo clique acaba
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

bool AHypeWorldCharacter::CanJumpInternal_Implementation() const
{
	return !IsStunned() && Super::CanJumpInternal_Implementation();
}

bool AHypeWorldCharacter::CanChargeChakra() const
{
	return !bKnockedOut && !IsAttacking() && !IsCastingJutsu() && GetCharacterMovement()->IsMovingOnGround();
}

// ---------------------------------------------------------------------------------------------------------------------
// Jutsus
// ---------------------------------------------------------------------------------------------------------------------

bool AHypeWorldCharacter::TryUseJutsuSlot(int32 Slot)
{
	const UNinjaJutsuData* Jutsu = GetJutsuInSlot(Slot);
	if (!Jutsu || !AbilitySystem || !CanStartJutsu())
	{
		return false;
	}
	for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
	{
		if (Spec.SourceObject.Get() == Jutsu)
		{
			if (AbilitySystem->TryActivateAbility(Spec.Handle))
			{
				if (JutsuUseCounts.IsValidIndex(Slot))
				{
					JutsuUseCounts[Slot]++;
				}
				return true;
			}
			return false;
		}
	}
	return false;
}

float AHypeWorldCharacter::GetJutsuCooldownRemaining(int32 Slot, float& OutDuration) const
{
	OutDuration = 0.f;
	const UNinjaJutsuData* Jutsu = GetJutsuInSlot(Slot);
	if (!Jutsu || !AbilitySystem || !Jutsu->CooldownTag.IsValid())
	{
		return 0.f;
	}
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Jutsu->CooldownTag));
	float Remaining = 0.f;
	for (const TPair<float, float>& Entry : AbilitySystem->GetActiveEffectsTimeRemainingAndDuration(Query))
	{
		if (Entry.Key > Remaining)
		{
			Remaining = Entry.Key;
			OutDuration = Entry.Value;
		}
	}
	return Remaining;
}

bool AHypeWorldCharacter::CanStartJutsu() const
{
	return CanAct() && !IsCastingJutsu() && !IsAttacking() && !bChargingChakra && !bChargingSuperJump
		&& GetCharacterMovement() && !GetCharacterMovement()->IsFalling();
}

USkeletalMeshComponent* AHypeWorldCharacter::GetJutsuBody() const
{
	// Aura/feet create visible mesh copies too, but these follow a leader pose and have no anim instance.
	TInlineComponentArray<USkeletalMeshComponent*> Meshes(this);
	for (USkeletalMeshComponent* JutsuMesh : Meshes)
	{
		if (JutsuMesh != GetMesh() && JutsuMesh->IsVisible() && JutsuMesh->GetAnimInstance() && JutsuMesh->GetSkeletalMeshAsset())
		{
			return JutsuMesh;
		}
	}
	return GetMesh();
}

void AHypeWorldCharacter::BeginJutsu(const UNinjaJutsuData* Jutsu)
{
	CurrentJutsu = Jutsu;
	GetCharacterMovement()->StopMovementImmediately();
	// Vira para onde a câmera olha (o jutsu sai nessa direção)
	if (Controller)
	{
		SetActorRotation(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
	}
	LastBattleTime = GetWorld()->GetTimeSeconds();   // jutsu conta como ação de batalha
	PlayJutsuSeals(Jutsu);
	if (HasAuthority())
	{
		MulticastJutsuSeals(Jutsu);
	}
}

void AHypeWorldCharacter::MulticastJutsuSeals_Implementation(const UNinjaJutsuData* Jutsu)
{
	// Quem já tocou (o próprio jogador, que prevê, e o servidor) não toca de novo
	if (!HasAuthority() && !IsLocallyControlled())
	{
		PlayJutsuSeals(Jutsu);
	}
}

void AHypeWorldCharacter::PlayJutsuSeals(const UNinjaJutsuData* Jutsu)
{
	PresentedJutsu = Jutsu;
	bJutsuReleased = false;
	SealIndex = INDEX_NONE;
	if (Jutsu && Jutsu->BodyCastAnimation)
	{
		USkeletalMeshComponent* Body = GetJutsuBody();
		UAnimInstance* Anim = Body ? Body->GetAnimInstance() : nullptr;
		if (!Anim) { return; }
		UAnimMontage* Montage = NewObject<UAnimMontage>(Anim);
		Montage->SetSkeleton(Jutsu->BodyCastAnimation->GetSkeleton());
		Montage->SlotAnimTracks.Reset();
		FSlotAnimationTrack& Slot = Montage->SlotAnimTracks.AddDefaulted_GetRef();
		Slot.SlotName = FAnimSlotGroup::DefaultSlotName;
		float Position = 0.f;
		auto AddSegment = [&](UAnimSequenceBase* Sequence, float Duration)
		{
			if (!Sequence || Duration <= 0.f) { return; }
			FAnimSegment& Segment = Slot.AnimTrack.AnimSegments.AddDefaulted_GetRef();
			Segment.SetAnimReference(Sequence, true);
			Segment.AnimStartTime = 0.f;
			Segment.AnimEndTime = Sequence->GetPlayLength();
			Segment.AnimPlayRate = Sequence->GetPlayLength() / FMath::Max(Duration, 0.01f);
			Segment.LoopingCount = 1;
			Segment.StartPos = Position;
			Position += Duration;
		};
		for (const FNinjaJutsuSeal& Seal : Jutsu->Seals) { AddSegment(Seal.Pose, Seal.Duration); }
		AddSegment(Jutsu->BodyCastAnimation, Jutsu->BodyCastAnimation->GetPlayLength());
		Montage->SetCompositeLength(Position);
		Montage->AddAnimCompositeSection(TEXT("Default"), 0.f);
		Montage->BlendIn.SetBlendTime(Jutsu->Seals.Num() ? Jutsu->Seals[0].BlendIn : 0.1f);
		Montage->BlendOut.SetBlendTime(Jutsu->CastBlendOut);
		Montage->bEnableAutoBlendOut = true;
		SealMontage = Montage;
		const float Played = Anim->Montage_Play(Montage);
		if (Played <= 0.f)
		{
			UE_LOG(LogHypeWorld, Error, TEXT("Jutsu montage rejected: body=%s skeleton=%s montage=%s length=%.3f slots=%d segments=%d"),
				*GetNameSafe(Body->GetSkeletalMeshAsset()), *GetNameSafe(Montage->GetSkeleton()), *GetNameSafe(Montage),
				Montage->GetPlayLength(), Montage->SlotAnimTracks.Num(), Slot.AnimTrack.AnimSegments.Num());
		}
		return;
	}
	if (Jutsu && Jutsu->Seals.Num() > 0)
	{
		StartSeal(0);
	}
}

void AHypeWorldCharacter::StartSeal(int32 Index)
{
	SealIndex = Index;
	SealElapsed = 0.f;
	const FNinjaJutsuSeal& Seal = PresentedJutsu->Seals[Index];
	USkeletalMeshComponent* Body = GetJutsuBody();
	UAnimInstance* Anim = Body ? Body->GetAnimInstance() : nullptr;
	if (Anim && Seal.Pose)
	{
		// Em loop para não sair sozinho no fim do clipe: UpdateJutsuSeals congela no último quadro e para no fim dos selos
		SealMontage = Anim->PlaySlotAnimationAsDynamicMontage(Seal.Pose, TEXT("DefaultSlot"), Seal.BlendIn, PresentedJutsu->SealBlendOut, 1.f, 100);
	}
}

void AHypeWorldCharacter::UpdateJutsuSeals(float DeltaSeconds)
{
	if (SealIndex == INDEX_NONE || !PresentedJutsu || !PresentedJutsu->Seals.IsValidIndex(SealIndex))
	{
		return;
	}
	SealElapsed += DeltaSeconds;
	const FNinjaJutsuSeal& Seal = PresentedJutsu->Seals[SealIndex];
	USkeletalMeshComponent* Body = GetJutsuBody();
	UAnimInstance* Anim = Body ? Body->GetAnimInstance() : nullptr;
	if (Anim && SealMontage && Seal.Pose && Anim->Montage_IsPlaying(SealMontage)
		&& Anim->Montage_GetPosition(SealMontage) >= Seal.Pose->GetPlayLength() - 0.04f)
	{
		Anim->Montage_Pause(SealMontage);   // segura a pose do selo
	}
	if (SealElapsed >= Seal.Duration)
	{
		if (PresentedJutsu->Seals.IsValidIndex(SealIndex + 1))
		{
			StartSeal(SealIndex + 1);
		}
		else if (!IsLocallyControlled() && !HasAuthority())
		{
			StopJutsuSeals(PresentedJutsu->SealBlendOut);   // nos outros jogadores a habilidade não roda: para sozinho
		}
		else
		{
			SealIndex = INDEX_NONE;   // a habilidade chama EndJutsuSeals quando o lançamento começa
		}
	}
}

void AHypeWorldCharacter::StopJutsuSeals(float BlendOut)
{
	SealIndex = INDEX_NONE;
	USkeletalMeshComponent* Body = GetJutsuBody();
	UAnimInstance* Anim = Body ? Body->GetAnimInstance() : nullptr;
	if (Anim && SealMontage)
	{
		Anim->Montage_Stop(BlendOut, SealMontage);
	}
	SealMontage = nullptr;
}

void AHypeWorldCharacter::EndJutsuSeals()
{
	StopJutsuSeals(CurrentJutsu ? CurrentJutsu->SealBlendOut : 0.18f);
}

void AHypeWorldCharacter::EndJutsu(bool bWasCancelled)
{
	StopJutsuSeals(bWasCancelled ? 0.12f : (CurrentJutsu ? CurrentJutsu->CastBlendOut : 0.2f));
	CurrentJutsu = nullptr;
	PresentedJutsu = nullptr;
	if (HasAuthority()) { MulticastJutsuEnd(bWasCancelled); }
}

void AHypeWorldCharacter::MulticastJutsuEnd_Implementation(bool bCancelled)
{
	if (!HasAuthority() && !IsLocallyControlled())
	{
		StopJutsuSeals(bCancelled ? 0.12f : (PresentedJutsu ? PresentedJutsu->CastBlendOut : 0.2f));
		PresentedJutsu = nullptr;
	}
}

void AHypeWorldCharacter::OnJutsuRelease()
{
	const UNinjaJutsuData* Jutsu = CurrentJutsu ? CurrentJutsu.Get() : PresentedJutsu.Get();
	if (!Jutsu || !Jutsu->ProjectileClass || bJutsuReleased)
	{
		return;
	}
	bJutsuReleased = true;
	USkeletalMeshComponent* Body = GetJutsuBody();
	FVector Mouth = GetActorLocation() + FVector(0.f, 0.f, 70.f);
	if (Body && Body->DoesSocketExist(Jutsu->MuzzleSocket))
	{
		Mouth = Body->GetSocketLocation(Jutsu->MuzzleSocket);
	}
	// Mira: para onde a câmera olha (com limite de inclinação); sem controle, para a frente do corpo
	FRotator Aim = GetActorRotation();
	if (Controller)
	{
		Aim = Controller->GetControlRotation();
	}
	Aim.Roll = 0.f;
	const ANinjaJutsuProjectile* Defaults = Jutsu->ProjectileClass->GetDefaultObject<ANinjaJutsuProjectile>();
	// Convergencia da boca para o ponto sob a mira, inclusive quando a camera esta acima do personagem.
	FVector ViewLocation = Mouth;
	if (APlayerController* PC = Cast<APlayerController>(Controller)) { PC->GetPlayerViewPoint(ViewLocation, Aim); }
	FHitResult AimHit;
	FCollisionQueryParams AimParams(SCENE_QUERY_STAT(NinjaJutsuAim), false, this);
	const FVector AimEnd = ViewLocation + Aim.Vector() * Defaults->Range;
	const bool bAimHit = GetWorld()->LineTraceSingleByChannel(AimHit, ViewLocation, AimEnd, ECC_Visibility, AimParams);
	const FVector Dir = ((bAimHit ? AimHit.ImpactPoint : AimEnd) - Mouth).GetSafeNormal();
	Aim = Dir.Rotation();

	if (GetNetMode() != NM_DedicatedServer && Defaults->MuzzleSystem && Body)
	{
		// Faíscas e chama pequena saindo da boca, presas ao socket e apontadas para onde a bola vai
		UNiagaraComponent* Muzzle = UNiagaraFunctionLibrary::SpawnSystemAttached(Defaults->MuzzleSystem, Body,
			Body->DoesSocketExist(Jutsu->MuzzleSocket) ? Jutsu->MuzzleSocket : NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget, true, true, ENCPoolMethod::AutoRelease);
		if (Muzzle)
		{
			Muzzle->SetUsingAbsoluteRotation(true);
			Muzzle->SetWorldRotation(Aim);
			FTimerHandle StopMuzzle;
			GetWorldTimerManager().SetTimer(StopMuzzle, FTimerDelegate::CreateWeakLambda(Muzzle, [Muzzle]() { Muzzle->Deactivate(); }),
				FMath::Max(Defaults->MuzzleDuration, 0.05f), false);
		}
	}
	if (IsLocallyControlled())
	{
		AddCameraShake(Jutsu->CastShake, 0.45f);
	}
	if (HasAuthority())
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FTransform SpawnAt(Dir.Rotation(), Mouth);
		if (ANinjaJutsuProjectile* Projectile = GetWorld()->SpawnActorDeferred<ANinjaJutsuProjectile>(Jutsu->ProjectileClass, SpawnAt, this, this,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Projectile->InitLaunch(Jutsu, Mouth + Dir * 20.f, Dir);
			Projectile->FinishSpawning(SpawnAt);
		}
	}
}

void AHypeWorldCharacter::ApplyJutsuHit(AHypeWorldCharacter* Target, const UNinjaJutsuData* Jutsu, float Scale, const FHitResult& Hit)
{
	if (!Target || !Jutsu || Target->bKnockedOut)
	{
		return;
	}
	FNinjaAttack Attack;
	Attack.Name = Jutsu->GetFName();
	Attack.Damage = Jutsu->Damage * Scale;
	Attack.Reaction = Jutsu->Reaction;
	Attack.Knockback = Jutsu->Knockback * Scale;
	Attack.Launch = Jutsu->Launch * Scale;
	Attack.Stun = Jutsu->Stun;
	Attack.bBreaksGuard = true;
	LastBattleTime = GetWorld()->GetTimeSeconds();
	ApplyHit(Target, Attack, true, Hit);
}

void AHypeWorldCharacter::AddCameraShake(float Degrees, float Duration)
{
	if (!IsLocallyControlled() || Degrees <= 0.f)
	{
		return;
	}
	// Uma tremida maior por cima de uma menor ganha; nunca soma até ficar exagerada
	const float Current = ShakeDuration > 0.f ? ShakeAmplitude * FMath::Square(1.f - FMath::Clamp(ShakeElapsed / ShakeDuration, 0.f, 1.f)) : 0.f;
	if (Degrees >= Current)
	{
		ShakeAmplitude = Degrees;
		ShakeDuration = FMath::Max(Duration, 0.05f);
		ShakeElapsed = 0.f;
	}
}

void AHypeWorldCharacter::UpdateCameraShake(float DeltaSeconds)
{
	if (ShakeDuration <= 0.f)
	{
		return;
	}
	ShakeElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(ShakeElapsed / ShakeDuration, 0.f, 1.f);
	if (Alpha >= 1.f)
	{
		ShakeDuration = 0.f;
		FollowCamera->SetRelativeRotation(CameraBaseRotation);
		return;
	}
	const float Amp = ShakeAmplitude * FMath::Square(1.f - Alpha);
	const float T = ShakeElapsed;
	const FRotator Offset(Amp * (FMath::Sin(T * 61.f) * 0.7f + FMath::Sin(T * 37.f) * 0.3f),
		Amp * (FMath::Sin(T * 47.f + 1.3f) * 0.6f + FMath::Sin(T * 29.f) * 0.4f),
		Amp * 0.4f * FMath::Sin(T * 53.f + 0.7f));
	FollowCamera->SetRelativeRotation(CameraBaseRotation + Offset);
}

void AHypeWorldCharacter::DoChargePressed()
{
	if (!TryCancelDodgeForAction()) return;
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
	if (ChargeChakraMontage)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (bChargingChakra && AnimInstance && !AnimInstance->Montage_IsPlaying(ChargeChakraMontage))
		{
			PlayAnimMontage(ChargeChakraMontage);
		}
		else if (!bChargingChakra)
		{
			StopAnimMontage(ChargeChakraMontage);
		}
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
	// Um clique liga o modo de batalha (fica até outro clique ou até ser nocauteado); segurando, também defende
	SetBattleToggle(!bBattleToggle);
	bWantsGuard = true;   // parado ou andando, o botão direito também defende
	if (UNinjaMovementComponent* NinjaMovement = GetNinjaMovement())
	{
		NinjaMovement->SetFocusing(true);
	}
}

void AHypeWorldCharacter::DoFocusEnd()
{
	bWantsGuard = false;
	if (UNinjaMovementComponent* NinjaMovement = GetNinjaMovement())
	{
		NinjaMovement->SetFocusing(false);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// PvP justo: fuga do combo (substituição), invulnerabilidade, passo do golpe, fim da animação de apanhar
// ---------------------------------------------------------------------------------------------------------------------

void AHypeWorldCharacter::UpdateCombatState(float DeltaSeconds)
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (HasAuthority())
	{
		bInvulnerable = Now < InvulnerableUntil;
		// Recarga da substituição: uma carga por vez
		if (SubstitutionCharges < MaxSubstitutions)
		{
			SubstitutionRechargeTime += DeltaSeconds;
			if (SubstitutionRechargeTime >= SubstitutionRecharge)
			{
				SubstitutionRechargeTime = 0.f;
				SubstitutionCharges++;
			}
		}
	}

	// Golpeando: passo para a frente no começo de cada golpe (para quem apanha não sair do alcance), menos com alguém colado
	if (IsLocallyControlled() && IsAttacking() && Now - AttackStartTime < AttackLungeTime && GetCharacterMovement()->IsMovingOnGround() && !HasBodyAhead())
	{
		AddMovementInput(GetActorForwardVector(), 1.f);
	}

	// Aterrissagem do dash (seção "End", vai até ele se levantar): se o jogador se mexer, sai suave na hora (não desliza agachado)
	if (ActiveDashMontage && !GetNinjaMovement()->IsDashing() && !GetNinjaMovement()->IsDashStrafe()
		&& GetCharacterMovement()->IsMovingOnGround() && !GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && AnimInstance->Montage_IsPlaying(ActiveDashMontage) && AnimInstance->Montage_GetCurrentSection(ActiveDashMontage) == FName("End"))
		{
			BlendOutDashMontage(ActiveDashMontage);
		}
	}

	// Acabou o atordoamento: a animação de apanhar sai assim que ele se mexe (não fica "preso" na reação)
	if (!IsStunned() && GetVelocity().SizeSquared2D() > 40.f * 40.f && GetCharacterMovement()->IsMovingOnGround())
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			const UAnimMontage* Current = AnimInstance->GetCurrentActiveMontage();
			if (Current && (Current == HeavyHitReactMontage || HitReactMontages.Contains(Current)))
			{
				AnimInstance->Montage_Stop(0.2f, Current);
			}
		}
	}
}

void AHypeWorldCharacter::TrySubstitution()
{
	if (!IsStunned() || bKnockedOut || SubstitutionCharges <= 0)
	{
		return;
	}
	if (HasAuthority())
	{
		DoSubstitution();
	}
	else
	{
		ServerSubstitution();
	}
}

void AHypeWorldCharacter::ServerSubstitution_Implementation()
{
	DoSubstitution();
}

void AHypeWorldCharacter::DoSubstitution()
{
	// Servidor confere de novo: só apanhando, com carga
	if (!IsStunned() || bKnockedOut || SubstitutionCharges <= 0)
	{
		return;
	}
	SubstitutionCharges--;
	AbilitySystem->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TAG_Ninja_State_Stunned));
	AbilitySystem->CancelAbilities();
	InvulnerableUntil = GetWorld()->GetTimeSeconds() + SubstitutionInvulnerability;
	bInvulnerable = true;
	TakenComboHits = 0;

	// Reaparece longe de quem bateu (sem ninguém: para trás), de frente para ele
	const FVector From = GetActorLocation();
	FVector Away = -GetActorForwardVector();
	if (const AActor* Attacker = LastAttacker.Get())
	{
		Away = (From - Attacker->GetActorLocation()).GetSafeNormal2D();
	}
	Away = Away.IsNearlyZero() ? -GetActorForwardVector() : Away;
	GetCharacterMovement()->StopMovementImmediately();
	TeleportTo(From + Away * SubstitutionDistance, FRotator(0.f, (-Away).Rotation().Yaw, 0.f), false, false);
	MulticastSubstitution(From, GetActorLocation());
}

void AHypeWorldCharacter::MulticastSubstitution_Implementation(FVector_NetQuantize From, FVector_NetQuantize To)
{
	StopAnimMontage();   // sai da animação de apanhar na hora
	const FLinearColor Smoke(0.85f, 0.85f, 0.8f);
	ANinjaHitFlash::Spawn(GetWorld(), FVector(From) + FVector(0.f, 0.f, 30.f), HitFlashMaterial, Smoke);
	ANinjaHitFlash::Spawn(GetWorld(), FVector(To) + FVector(0.f, 0.f, 30.f), HitFlashMaterial, Smoke);
	ANinjaDamageNumber::Spawn(GetWorld(), FVector(From) + FVector(0.f, 0.f, 110.f), TEXT("SUBSTITUIÇÃO"), FLinearColor(0.9f, 0.9f, 0.8f), 30.f);

	// O tronco que fica no lugar (só visual, cai e some)
	if (UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AStaticMeshActor* Log = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(From) + FVector(0.f, 0.f, 10.f), FRotator(0.f, GetActorRotation().Yaw, 75.f), Params))
		{
			Log->SetMobility(EComponentMobility::Movable);
			UStaticMeshComponent* LogMesh = Log->GetStaticMeshComponent();
			LogMesh->SetStaticMesh(Cylinder);
			LogMesh->SetWorldScale3D(FVector(0.35f, 0.35f, 1.3f));
			LogMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
			LogMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
			UMaterialInterface* ShapeMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
			if (UMaterialInstanceDynamic* Wood = LogMesh->CreateDynamicMaterialInstance(0, ShapeMaterial))
			{
				Wood->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.32f, 0.18f, 0.08f));
			}
			LogMesh->SetSimulatePhysics(true);
			Log->SetLifeSpan(2.f);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Estilos de luta, variedade de golpes e de dash, menu de combate
// ---------------------------------------------------------------------------------------------------------------------

int32 AHypeWorldCharacter::PickVariant(int32 Num, int32 Last)
{
	// Sorteia um índice diferente do último (com uma só opção, repete)
	if (Num <= 1)
	{
		return 0;
	}
	int32 Index = FMath::RandRange(0, Num - 2);
	return (Last != INDEX_NONE && Index >= Last) ? Index + 1 : Index;
}

UNinjaFightingStyle* AHypeWorldCharacter::GetFightingStyle() const
{
	return FightingStyles.IsValidIndex(FightingStyleIndex) ? FightingStyles[FightingStyleIndex].Get() : nullptr;
}

bool AHypeWorldCharacter::CanUseFightingStyle(const UNinjaFightingStyle* Style) const
{
	return Style && (Style->RequiredWeapon == ENinjaWeapon::None || Style->RequiredWeapon == EquippedWeapon) && Style->Combos.Num() > 0;
}

bool AHypeWorldCharacter::SelectFightingStyle(int32 Index)
{
	if (!FightingStyles.IsValidIndex(Index) || !CanUseFightingStyle(FightingStyles[Index]))
	{
		return false;
	}
	FightingStyleIndex = Index;
	LastComboString = LastFinisher = LastSprintAttack = LastAirAttack = INDEX_NONE;
	if (!HasAuthority())
	{
		ServerSelectFightingStyle(Index);
	}
	return true;
}

void AHypeWorldCharacter::ServerSelectFightingStyle_Implementation(int32 Index)
{
	SelectFightingStyle(Index);
}

UAnimSequenceBase* AHypeWorldCharacter::GetBattleIdlePose() const
{
	const UNinjaFightingStyle* Style = GetFightingStyle();
	return Style ? Style->BattleIdle.Get() : nullptr;
}

void AHypeWorldCharacter::BlendOutDashMontage(UAnimMontage* Montage)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!Montage || !AnimInstance || !AnimInstance->Montage_IsPlaying(Montage) || AnimInstance->Montage_GetIsStopped(Montage))
	{
		return;
	}
	// Enquanto some, a montagem continua tocando: o voo tem que seguir para a aterrissagem (mesma animação, emenda sem salto).
	// Se seguisse para o Loop (outra animação), a pose trocava de uma vez no meio da transição = o "pulo".
	static const FName EndSection("End");
	const FName Current = AnimInstance->Montage_GetCurrentSection(Montage);
	if (Current != EndSection && Montage->IsValidSectionName(EndSection))
	{
		AnimInstance->Montage_SetNextSection(Current, EndSection, Montage);
	}
	// O tempo da mistura corre na velocidade da animação (andando ela vai a 2x): compensa para durar DashExitBlendTime de verdade
	const float ExitTime = GetNinjaMovement()->IsDashStrafe() ? 0.14f : DashExitBlendTime;
	AnimInstance->Montage_Stop(ExitTime * FMath::Max(GetMesh()->GlobalAnimRateScale, 0.1f), Montage);
}

bool AHypeWorldCharacter::IsPlayingDashMontage() const
{
	const UAnimInstance* Anim = GetMesh()->GetAnimInstance();
	return ActiveDashMontage && Anim && Anim->Montage_IsActive(ActiveDashMontage);
}

UNinjaDodgeDefinition* AHypeWorldCharacter::GetCombatDodgeForDirection(int32 Index) const
{
	return CombatDodges.IsValidIndex(Index) ? CombatDodges[Index].Get() : nullptr;
}

bool AHypeWorldCharacter::TryCancelDodgeForAction()
{
	return !GetNinjaMovement() || GetNinjaMovement()->TryCancelCombatDodge();
}

void AHypeWorldCharacter::PlayCombatDodge(UNinjaDodgeDefinition* Definition)
{
	if (!Definition || !Definition->Montage) return;
	ActiveDashMontage = Definition->Montage;
	GetMesh()->GlobalAnimRateScale = 1.f;
	SetAnimRootMotionTranslationScale(Definition->bUseRootMotion ? Definition->Distance / FMath::Max(1.f, Definition->AuthoredRootDistance) : 1.f);
	if (UAnimInstance* Anim = GetMesh()->GetAnimInstance()) Anim->SetRootMotionMode(Definition->bUseRootMotion ? ERootMotionMode::RootMotionFromMontagesOnly : ERootMotionMode::IgnoreRootMotion);
	PlayAnimMontage(Definition->Montage, Definition->Montage->GetPlayLength() / Definition->Duration);
}

void AHypeWorldCharacter::MulticastCombatDodge_Implementation(UNinjaDodgeDefinition* Definition, float FacingYaw)
{
	if (HasAuthority() || IsLocallyControlled()) return;
	GetNinjaMovement()->SetSimulatedCombatDodge(Definition, FacingYaw);
	PlayCombatDodge(Definition);
}

UAnimMontage* AHypeWorldCharacter::PickDashMontage(EMovementMode PrevMovementMode)
{
	// Direção do dash em relação a para onde o corpo olha (no dash de lado/de costas o corpo fica de frente para a câmera)
	const UNinjaMovementComponent* NinjaMovement = GetNinjaMovement();
	FVector Dir;
	float FacingYaw;
	if (NinjaMovement && (IsLocallyControlled() || HasAuthority()))
	{
		Dir = NinjaMovement->GetDashDirection();
		FacingYaw = NinjaMovement->IsDashStrafe() ? NinjaMovement->GetDashFacingYaw() : Dir.Rotation().Yaw;
	}
	else
	{
		Dir = GetVelocity().GetSafeNormal2D();   // outros jogadores: pela velocidade e rotação recebidas
		FacingYaw = GetActorRotation().Yaw;
	}
	const float Rel = NinjaMovement && (IsLocallyControlled() || HasAuthority()) ? NinjaMovement->GetDashInputAngle()
		: (Dir.IsNearlyZero() ? 0.f : FMath::FindDeltaAngleDegrees(FacingYaw, Dir.Rotation().Yaw));

	const TArray<TObjectPtr<UAnimMontage>>* List = &DashVariants.Front;
	if (PrevMovementMode == MOVE_Falling && !(NinjaMovement && NinjaMovement->IsDashStrafe()) && DashVariants.Air.Num() > 0)
	{
		List = &DashVariants.Air;
	}
	else if (FMath::Abs(Rel) >= 135.f && DashVariants.Back.Num() > 0)
	{
		List = &DashVariants.Back;
	}
	else if (Rel >= 40.f && DashVariants.Right.Num() > 0)
	{
		List = &DashVariants.Right;
	}
	else if (Rel <= -40.f && DashVariants.Left.Num() > 0)
	{
		List = &DashVariants.Left;
	}
	if (List->Num() == 0)
	{
		return DashMontage;
	}
	// Variação: contador de dashes embaralhado (igual em todas as máquinas, sem precisar mandar nada pela rede)
	uint32 Hash = (uint32(DashCount++) + 1u) * 0x9E3779B1u;
	Hash = ((Hash ^ (Hash >> 15)) * 0x2C1B3C6Du) >> 16;
	return (*List)[Hash % uint32(List->Num())];
}

void AHypeWorldCharacter::DoCombatMenu()
{
	if (AHypeWorldPlayerController* PC = Cast<AHypeWorldPlayerController>(GetController()))
	{
		PC->ToggleCombatMenu();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Passivas e infusão de chakra nos pés (super pulo; andar na parede/teto/água fica no componente de movimento)
// ---------------------------------------------------------------------------------------------------------------------

bool AHypeWorldCharacter::HasLearnedPassive(ENinjaPassive Passive) const
{
	return Passives.ContainsByPredicate([Passive](const FNinjaPassiveInfo& Info) { return Info.Id == Passive && Info.bLearned; });
}

void AHypeWorldCharacter::SetPassiveEnabled(ENinjaPassive Passive, bool bEnabled)
{
	if (!HasLearnedPassive(Passive) || IsPassiveEnabled(Passive) == bEnabled)
	{
		return;
	}
	const uint32 Bit = 1u << uint32(Passive);
	EnabledPassives = bEnabled ? (EnabledPassives | Bit) : (EnabledPassives & ~Bit);
	OnRep_EnabledPassives();
	if (!HasAuthority())
	{
		ServerSetPassiveEnabled(Passive, bEnabled);
	}
	// Sem texto na tela ao ligar/desligar (pedido do Felipe): o efeito nos pés já mostra
}

void AHypeWorldCharacter::ServerSetPassiveEnabled_Implementation(ENinjaPassive Passive, bool bEnabled)
{
	SetPassiveEnabled(Passive, bEnabled);
}

void AHypeWorldCharacter::OnRep_EnabledPassives()
{
	UpdateChakraFeetVisual();
	if (UNinjaMovementComponent* NinjaMovement = GetNinjaMovement())
	{
		NinjaMovement->OnChakraFeetChanged(IsPassiveEnabled(ENinjaPassive::ChakraFeet));
	}
}

void AHypeWorldCharacter::DoToggleChakraFeet()
{
	TogglePassive(ENinjaPassive::ChakraFeet);
}

void AHypeWorldCharacter::UpdateChakraFeetVisual()
{
	// Chamas de chakra nos pés com a infusão ligada (todo mundo vê: vem da passiva replicada)
	if (ChakraFeetFx)
	{
		ChakraFeetFx->SetFeetActive(IsPassiveEnabled(ENinjaPassive::ChakraFeet));
	}
}

float AHypeWorldCharacter::GetSurfaceSpeed() const
{
	return GetCharacterMovement()->ProjectToGravityFloor(GetVelocity()).Size();
}

bool AHypeWorldCharacter::CanStartSuperJump() const
{
	// Pulo carregado vale sempre (fora do modo de batalha); não depende mais da infusão nos pés (pedido do Felipe)
	return GetCharacterMovement()->IsMovingOnGround() && CanAct() && !IsAttacking() && !bChargingChakra
		&& !IsInBattleMode() && GetWorld()->GetTimeSeconds() >= NextSuperJumpTime;
}

void AHypeWorldCharacter::UpdateSuperJump(float DeltaSeconds)
{
	if (!IsLocallyControlled() || !bJumpKeyHeld)
	{
		return;
	}
	// Saiu do chão ou apanhou: cancela (sem pular)
	if (!GetCharacterMovement()->IsMovingOnGround() || !CanAct())
	{
		CancelSuperJumpCharge();
		return;
	}
	JumpHoldTime += DeltaSeconds;
	if (!bChargingSuperJump && JumpHoldTime >= SuperJumpHoldThreshold)
	{
		bChargingSuperJump = true;
		OnRep_ChargingSuperJump();
		if (!HasAuthority())
		{
			ServerSetChargingSuperJump(true);
		}
	}
	if (bChargingSuperJump)
	{
		SuperJumpCharge = FMath::Clamp((JumpHoldTime - SuperJumpHoldThreshold) / FMath::Max(SuperJumpChargeTime, 0.05f), 0.f, 1.f);
	}
}

void AHypeWorldCharacter::ReleaseSuperJump()
{
	if (!CanStartSuperJump()) { CancelSuperJumpCharge(); return; }
	// Para a frente (para onde a câmera olha, no plano do chão atual) e para cima: mais carga = mais longe e mais alto
	const float A = SuperJumpCharge;
	const FVector Up = -GetCharacterMovement()->GetGravityDirection();
	FVector Forward = GetCharacterMovement()->ProjectToGravityFloor(GetControlRotation().Vector()).GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		Forward = GetActorForwardVector();
	}
	// Mira: câmera olhando para o céu = salto reto para cima; olhando para a frente/lados = arco para a frente
	const float Pitch = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(FVector::DotProduct(GetControlRotation().Vector(), Up), -1.f, 1.f)));
	const float Vertical = FMath::SmoothStep((float)SuperJumpVerticalPitch.X, (float)SuperJumpVerticalPitch.Y, Pitch);
	const float ForwardSpeed = FMath::Lerp(SuperJumpForwardSpeed.X, SuperJumpForwardSpeed.Y, A) * (1.f - Vertical);
	const float UpSpeed = FMath::Lerp(FMath::Lerp(SuperJumpUpSpeed.X, SuperJumpUpSpeed.Y, A), FMath::Lerp(SuperJumpVerticalSpeed.X, SuperJumpVerticalSpeed.Y, A), Vertical);
	const FVector LaunchVelocity = Forward * ForwardSpeed + Up * UpSpeed;
	NextSuperJumpTime = GetWorld()->GetTimeSeconds() + SuperJumpCooldown;

	bChargingSuperJump = false;
	SuperJumpCharge = 0.f;
	OnRep_ChargingSuperJump();
	SetActorRotation(FRotationMatrix::MakeFromXZ(Forward, Up).Rotator());
	if (SuperJumpLaunchMontage)
	{
		PlayJumpMontage(SuperJumpLaunchMontage, UpSpeed);
	}
	GetNinjaMovement()->bInSuperJump = true;
	GetNinjaMovement()->StartFloatJump(SuperJumpGravity.X, SuperJumpGravity.Y, SuperJumpGravity.Z);
	LaunchCharacter(LaunchVelocity, true, true);
	if (!HasAuthority())
	{
		ServerSuperJump(LaunchVelocity);
	}
	else
	{
		MulticastSuperJumpLaunch(UpSpeed);
	}
}

void AHypeWorldCharacter::ServerSetChargingSuperJump_Implementation(bool bCharging)
{
	bChargingSuperJump = bCharging && CanStartSuperJump();
	OnRep_ChargingSuperJump();
}

void AHypeWorldCharacter::ServerSuperJump_Implementation(FVector_NetQuantize10 LaunchVelocity)
{
	// Servidor confere: nunca mais forte que a barra cheia
	if (!CanAct() || GetWorld()->GetTimeSeconds() < NextSuperJumpTime || !GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}
	const float MaxSpeed = FMath::Max(FVector2D(SuperJumpForwardSpeed.Y, SuperJumpUpSpeed.Y).Size(), SuperJumpVerticalSpeed.Y) * 1.05f;
	NextSuperJumpTime = GetWorld()->GetTimeSeconds() + SuperJumpCooldown;
	bChargingSuperJump = false;
	OnRep_ChargingSuperJump();
	GetNinjaMovement()->bInSuperJump = true;
	GetNinjaMovement()->StartFloatJump(SuperJumpGravity.X, SuperJumpGravity.Y, SuperJumpGravity.Z);
	LaunchCharacter(LaunchVelocity.GetClampedToMaxSize(MaxSpeed), true, true);
	MulticastSuperJumpLaunch(FVector::DotProduct(LaunchVelocity.GetClampedToMaxSize(MaxSpeed), -GetCharacterMovement()->GetGravityDirection()));
}

void AHypeWorldCharacter::MulticastSuperJumpLaunch_Implementation(float UpSpeed)
{
	// Os outros jogadores veem o salto (quem pulou já tocou a animação)
	if (!IsLocallyControlled() && SuperJumpLaunchMontage)
	{
		PlayJumpMontage(SuperJumpLaunchMontage, UpSpeed);
	}
}

void AHypeWorldCharacter::OnRep_ChargingSuperJump()
{
	if (!SuperJumpChargeMontage)
	{
		return;
	}
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (bChargingSuperJump && AnimInstance && !AnimInstance->Montage_IsPlaying(SuperJumpChargeMontage))
	{
		PlayAnimMontage(SuperJumpChargeMontage);
	}
	else if (!bChargingSuperJump)
	{
		StopAnimMontage(SuperJumpChargeMontage);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Combo aéreo (lançador) e mortal para trás
// ---------------------------------------------------------------------------------------------------------------------

void AHypeWorldCharacter::CancelSuperJumpCharge()
{
	bJumpKeyHeld = false;
	if (bChargingSuperJump)
	{
		bChargingSuperJump = false;
		SuperJumpCharge = 0.f;
		OnRep_ChargingSuperJump();
		if (!HasAuthority())
		{
			ServerSetChargingSuperJump(false);
		}
	}
}

bool AHypeWorldCharacter::IsInComboContext() const
{
	return IsAttacking() || (LastChainAttackId != INDEX_NONE && GetWorld()->GetTimeSeconds() <= ChainExpireTime + 0.25f);
}

void AHypeWorldCharacter::DoBackflipJump()
{
	// Para cima e um pouco para trás (em relação à câmera), com mortal para trás; o corpo continua de frente
	const FVector Back = -GetCharacterMovement()->ProjectToGravityFloor(GetControlRotation().Vector()).GetSafeNormal();
	const FVector LaunchVelocity = Back * BackflipBackSpeed + FVector(0.f, 0.f, BackflipUpSpeed);
	if (BackflipMontage)
	{
		PlayJumpMontage(BackflipMontage, BackflipUpSpeed);
	}
	GetNinjaMovement()->bInSuperJump = true;   // sem frenagem no ar (senão o embalo para trás some)
	GetNinjaMovement()->bInBackflip = true;
	GetNinjaMovement()->StartFloatJump(BackflipGravity.X, BackflipGravity.Y, BackflipGravity.Z);
	LaunchCharacter(LaunchVelocity, true, true);
	if (!HasAuthority())
	{
		ServerBackflipJump(LaunchVelocity);
	}
	else
	{
		MulticastBackflip();
	}
}

void AHypeWorldCharacter::ServerBackflipJump_Implementation(FVector_NetQuantize10 LaunchVelocity)
{
	if (!CanAct() || !GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}
	GetNinjaMovement()->bInSuperJump = true;
	GetNinjaMovement()->bInBackflip = true;
	GetNinjaMovement()->StartFloatJump(BackflipGravity.X, BackflipGravity.Y, BackflipGravity.Z);
	LaunchCharacter(LaunchVelocity.GetClampedToMaxSize(FVector2D(BackflipBackSpeed, BackflipUpSpeed).Size() * 1.05f), true, true);
	MulticastBackflip();
}

void AHypeWorldCharacter::MulticastBackflip_Implementation()
{
	if (!IsLocallyControlled() && BackflipMontage)
	{
		PlayJumpMontage(BackflipMontage, BackflipUpSpeed);
	}
}
