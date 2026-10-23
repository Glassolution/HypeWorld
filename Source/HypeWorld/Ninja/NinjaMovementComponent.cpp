// Movimento ninja: corrida ninja, dash e corrida na parede, com predição no cliente (pronto para multiplayer).

#include "NinjaMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "HypeWorld.h"
#include "HypeWorldCharacter.h"
#include "Animation/AnimSequenceBase.h"

// ---------------------------------------------------------------------------------------------------------------------
// Regras gerais
// ---------------------------------------------------------------------------------------------------------------------

float UNinjaMovementComponent::GetMaxAcceleration() const
{
	// Já correndo e acelerando para a corrida ninja: aceleração menor (transição suave)
	if (bWantsToSprint && IsMovingOnGround() && Velocity.Size2D() >= MaxWalkSpeed * 0.95f)
	{
		return SprintAcceleration;
	}
	return Super::GetMaxAcceleration();
}

float UNinjaMovementComponent::GetMaxSpeed() const
{
	// Modo foco: anda devagar de frente para a câmera; de costas, mais devagar ainda
	if (IsFocusing())
	{
		const FVector Facing = UpdatedComponent ? UpdatedComponent->GetForwardVector().GetSafeNormal2D() : FVector::ForwardVector;
		const float Back = FMath::Clamp(-FVector::DotProduct(Facing, Acceleration.GetSafeNormal2D()), 0.f, 1.f);
		return FMath::Lerp(FocusSpeed, FocusBackSpeed, Back);
	}

	// Atacando: mantém um pouco do embalo, mas desacelera suave até AttackMaxSpeed
	if (const AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(CharacterOwner); Ninja && Ninja->IsAttacking() && IsMovingOnGround())
	{
		return AttackMaxSpeed;
	}

	// Correndo (ou no pulo que saiu de uma corrida), o limite de velocidade sobe.
	if (bWantsToSprint && (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking || MovementMode == MOVE_Falling))
	{
		return SprintSpeed;
	}
	return Super::GetMaxSpeed();
}

FRotator UNinjaMovementComponent::ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const
{
	// Na parede o corpo olha ao longo da parede, não para onde o controle aponta (senão ele vira de cara para a parede).
	if (IsWallRunning())
	{
		const FVector AlongWall = (Velocity - WallNormal * FVector::DotProduct(Velocity, WallNormal)).GetSafeNormal2D();
		if (!AlongWall.IsNearlyZero())
		{
			return AlongWall.Rotation();
		}
	}
	// Modo foco: o corpo fica de frente para onde a câmera olha (anda de lado/de costas)
	if (IsFocusing() && CharacterOwner)
	{
		return FRotator(0.f, CharacterOwner->GetControlRotation().Yaw, 0.f);
	}
	return Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);
}

bool UNinjaMovementComponent::CanAttemptJump() const
{
	// O padrão só deixa pular no chão ou no ar; aqui também deixamos pular da parede.
	return Super::CanAttemptJump() || (IsJumpAllowed() && IsWallRunning());
}

bool UNinjaMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	if (IsWallRunning() && CharacterOwner && CharacterOwner->CanJump())
	{
		// Pulo de parede: mantém a velocidade ao longo da parede, empurra para fora e para cima.
		FVector AlongWall = Velocity - WallNormal * FVector::DotProduct(Velocity, WallNormal);
		AlongWall.Z = 0.f;
		Velocity = AlongWall + WallNormal * WallJumpOffForce;
		Velocity.Z = JumpZVelocity;
		EndWallRun();
		SetMovementMode(MOVE_Falling);
		return true;
	}
	return Super::DoJump(bReplayingMoves, DeltaTime);
}

void UNinjaMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bWantsToDash = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
	bWantsToFocus = (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;
}

FNetworkPredictionData_Client* UNinjaMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UNinjaMovementComponent* MutableThis = const_cast<UNinjaMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Ninja(*this);
	}
	return ClientPredictionData;
}

void UNinjaMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	DashCooldownLeft = FMath::Max(0.f, DashCooldownLeft - DeltaSeconds);
	WallRunCooldownLeft = FMath::Max(0.f, WallRunCooldownLeft - DeltaSeconds);

	// O pedido de dash vale só para este movimento.
	if (bWantsToDash)
	{
		bWantsToDash = false;
		if (CanDash())
		{
			StartDash();
		}
	}

	if (IsFalling())
	{
		TryStartWallRun();
	}

	// Virada ao sair do lugar (só visual; não roda ao refazer movimentos depois de correção do servidor)
	if (AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(CharacterOwner); Ninja && !Ninja->bClientUpdating)
	{
		Ninja->HandleMoveStart(Acceleration);
	}
}

void UNinjaMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	switch (CustomMovementMode)
	{
	case NMOVE_Dash:	PhysDash(DeltaTime, Iterations); break;
	case NMOVE_WallRun:	PhysWallRun(DeltaTime, Iterations); break;
	default:			Super::PhysCustom(DeltaTime, Iterations); break;
	}
}

void UNinjaMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	// Tocou o chão: libera o dash no ar de novo.
	if (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking)
	{
		bAirDashUsed = false;
	}
}

void UNinjaMovementComponent::ExitToWalkingOrFalling()
{
	FFindFloorResult Floor;
	FindFloor(UpdatedComponent->GetComponentLocation(), Floor, false);
	SetMovementMode(Floor.IsWalkableFloor() ? MOVE_Walking : MOVE_Falling);
}

// ---------------------------------------------------------------------------------------------------------------------
// Dash
// ---------------------------------------------------------------------------------------------------------------------

bool UNinjaMovementComponent::CanDash() const
{
	if (!UpdatedComponent || DashCooldownLeft > 0.f || IsDashing())
	{
		return false;
	}
	return IsMovingOnGround() || IsWallRunning() || (IsFalling() && !bAirDashUsed);
}

void UNinjaMovementComponent::StartDash()
{
	// Vai para onde o jogador está apontando o movimento; sem direção, vai para frente.
	DashDirection = Acceleration.GetSafeNormal2D();
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
	}

	if (!IsMovingOnGround())
	{
		bAirDashUsed = true;
	}
	DashTimeLeft = GetDashDuration();
	DashCooldownLeft = DashCooldown;
	SetMovementMode(MOVE_Custom, NMOVE_Dash);
}

float UNinjaMovementComponent::GetDashDuration() const
{
	return DashRootMotionAnim ? DashAnimMoveEnd / FMath::Max(DashAnimRate, 0.01f) : DashDuration;
}

void UNinjaMovementComponent::EndDash()
{
	DashTimeLeft = 0.f;
	// Seguindo a animação, o dash já termina freando: mantém a velocidade final dela. Sem animação, sai correndo.
	if (!DashRootMotionAnim)
	{
		Velocity = DashDirection * (bWantsToSprint ? SprintSpeed : MaxWalkSpeed);
	}
	ExitToWalkingOrFalling();
}

void UNinjaMovementComponent::PhysDash(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// Sem gravidade enquanto o dash dura. Nunca anda mais que o tempo restante (com FPS baixo um frame pode ser maior que o dash inteiro).
	const float StepTime = FMath::Min(DeltaTime, DashTimeLeft);
	const float Elapsed = GetDashDuration() - DashTimeLeft;
	DashTimeLeft -= StepTime;

	float Speed = DashSpeed;
	if (DashRootMotionAnim && StepTime > 0.f)
	{
		// Quanto a animação anda neste pedaço de tempo = quanto o personagem anda (pés e corpo sincronizados)
		const FTransform Motion = DashRootMotionAnim->ExtractRootMotionFromRange(Elapsed * DashAnimRate, (Elapsed + StepTime) * DashAnimRate, FAnimExtractContext());
		Speed = Motion.GetTranslation().Size2D() * DashDistanceScale / StepTime;
	}
	Velocity = DashDirection * Speed;

	const FVector Delta = Velocity * StepTime;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);
	if (Hit.IsValidBlockingHit())
	{
		HandleImpact(Hit, DeltaTime, Delta);
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
	}

	if (DashTimeLeft <= 0.f)
	{
		EndDash();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Corrida na parede
// ---------------------------------------------------------------------------------------------------------------------

bool UNinjaMovementComponent::TraceForWall(const FVector& Direction, FHitResult& OutHit) const
{
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const float Reach = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius() + WallDetectDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(NinjaWallTrace), false, CharacterOwner);
	FCollisionResponseParams Response;
	InitCollisionParams(Params, Response);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, Start + Direction * Reach,
		UpdatedComponent->GetCollisionObjectType(), Params, Response);

	// Só conta parede quase vertical.
	return bHit && FMath::Abs(OutHit.ImpactNormal.Z) < 0.3f;
}

bool UNinjaMovementComponent::FindWallBeside(FHitResult& OutHit, float& OutSide) const
{
	if (!CharacterOwner)
	{
		return false;
	}
	const FVector Right = CharacterOwner->GetActorRightVector();
	if (!TraceForWall(Right, OutHit) && !TraceForWall(-Right, OutHit))
	{
		return false;
	}
	// O lado vem da direção da parede, não de qual traço acertou.
	OutSide = FVector::DotProduct(Right, -OutHit.ImpactNormal) >= 0.f ? 1.f : -1.f;
	return true;
}

bool UNinjaMovementComponent::TryStartWallRun()
{
	// Precisa estar no ar, segurando a corrida ninja, com velocidade e direção.
	if (!bWantsToSprint || WallRunCooldownLeft > 0.f || Velocity.Size2D() < WallRunMinSpeed || Acceleration.IsNearlyZero())
	{
		return false;
	}

	const FVector MoveDir = Velocity.GetSafeNormal2D();
	const FVector RightDir = FVector::CrossProduct(FVector::UpVector, MoveDir);

	for (const float Side : { 1.f, -1.f })
	{
		FHitResult Hit;
		if (!TraceForWall(RightDir * Side, Hit))
		{
			continue;
		}

		// A parede precisa estar ao lado (até 60° do paralelo), não de frente.
		const FVector Normal = Hit.ImpactNormal.GetSafeNormal2D();
		if (FMath::Abs(FVector::DotProduct(MoveDir, Normal)) > 0.5f)
		{
			continue;
		}

		WallNormal = Normal;
		WallRunTimeLeft = WallRunMaxDuration;
		bAirDashUsed = false;
		Velocity.Z = FMath::Clamp(Velocity.Z, 0.f, WallRunEntryMaxUpSpeed);
		SetMovementMode(MOVE_Custom, NMOVE_WallRun);
		return true;
	}
	return false;
}

void UNinjaMovementComponent::EndWallRun()
{
	WallRunTimeLeft = 0.f;
	WallRunCooldownLeft = 0.35f; // evita grudar de novo na mesma parede no frame seguinte
}

void UNinjaMovementComponent::PhysWallRun(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	WallRunTimeLeft -= DeltaTime;

	FHitResult WallHit;
	const bool bStillOnWall = TraceForWall(-WallNormal, WallHit);
	if (!bWantsToSprint || WallRunTimeLeft <= 0.f || !bStillOnWall)
	{
		UE_LOG(LogHypeWorld, Verbose, TEXT("Fim da corrida na parede: correndo=%d tempo_restante=%.2f parede=%d"),
			bWantsToSprint, WallRunTimeLeft, bStillOnWall);
		EndWallRun();
		ExitToWalkingOrFalling();
		return;
	}
	WallNormal = WallHit.ImpactNormal.GetSafeNormal2D();

	// Direção ao longo da parede, no mesmo sentido em que já estamos indo.
	FVector AlongWall = FVector::CrossProduct(WallNormal, FVector::UpVector);
	if (FVector::DotProduct(AlongWall, Velocity) < 0.f)
	{
		AlongWall = -AlongWall;
	}

	// Gravidade reduzida + um leve puxão para a parede para não descolar em curvas.
	const float NewZ = Velocity.Z + GetGravityZ() * WallRunGravityScale * DeltaTime;
	Velocity = AlongWall * WallRunSpeed - WallNormal * 50.f;
	Velocity.Z = NewZ;

	const FVector Delta = Velocity * DeltaTime;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);
	if (Hit.IsValidBlockingHit())
	{
		if (IsWalkable(Hit))
		{
			EndWallRun();
			SetMovementMode(MOVE_Walking);
			return;
		}
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, false);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Movimento salvo (rede)
// ---------------------------------------------------------------------------------------------------------------------

void FSavedMove_Ninja::Clear()
{
	Super::Clear();
	bSavedWantsToSprint = false;
	bSavedWantsToDash = false;
	bSavedWantsToFocus = false;
	bSavedAirDashUsed = false;
	SavedDashTimeLeft = 0.f;
	SavedDashCooldownLeft = 0.f;
	SavedDashDirection = FVector::ForwardVector;
	SavedWallRunTimeLeft = 0.f;
	SavedWallRunCooldownLeft = 0.f;
	SavedWallNormal = FVector::ZeroVector;
}

uint8 FSavedMove_Ninja::GetCompressedFlags() const
{
	uint8 Flags = Super::GetCompressedFlags();
	if (bSavedWantsToSprint)
	{
		Flags |= FLAG_Custom_0;
	}
	if (bSavedWantsToDash)
	{
		Flags |= FLAG_Custom_1;
	}
	if (bSavedWantsToFocus)
	{
		Flags |= FLAG_Custom_2;
	}
	return Flags;
}

void FSavedMove_Ninja::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	if (const UNinjaMovementComponent* Move = Cast<UNinjaMovementComponent>(C->GetCharacterMovement()))
	{
		bSavedWantsToSprint = Move->bWantsToSprint;
		bSavedWantsToDash = Move->bWantsToDash;
		bSavedWantsToFocus = Move->bWantsToFocus;
		bSavedAirDashUsed = Move->bAirDashUsed;
		SavedDashTimeLeft = Move->DashTimeLeft;
		SavedDashCooldownLeft = Move->DashCooldownLeft;
		SavedDashDirection = Move->DashDirection;
		SavedWallRunTimeLeft = Move->WallRunTimeLeft;
		SavedWallRunCooldownLeft = Move->WallRunCooldownLeft;
		SavedWallNormal = Move->WallNormal;
	}
}

void FSavedMove_Ninja::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);

	if (UNinjaMovementComponent* Move = Cast<UNinjaMovementComponent>(C->GetCharacterMovement()))
	{
		Move->bAirDashUsed = bSavedAirDashUsed;
		Move->DashTimeLeft = SavedDashTimeLeft;
		Move->DashCooldownLeft = SavedDashCooldownLeft;
		Move->DashDirection = SavedDashDirection;
		Move->WallRunTimeLeft = SavedWallRunTimeLeft;
		Move->WallRunCooldownLeft = SavedWallRunCooldownLeft;
		Move->WallNormal = SavedWallNormal;
	}
}
