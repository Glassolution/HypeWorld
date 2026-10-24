// Movimento ninja: corrida ninja, dash e corrida na parede, com predição no cliente (pronto para multiplayer).

#include "NinjaMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "HypeWorld.h"
#include "HypeWorldCharacter.h"
#include "Animation/AnimSequenceBase.h"
#include "NinjaPassives.h"
#include "Components/CapsuleComponent.h"

// ---------------------------------------------------------------------------------------------------------------------
// Regras gerais
// ---------------------------------------------------------------------------------------------------------------------

float UNinjaMovementComponent::GetMaxAcceleration() const
{
	// Já correndo e acelerando para a corrida ninja: aceleração menor (transição suave)
	if (bWantsToSprint && IsMovingOnGround() && Velocity.Size2D() >= RunSpeed * 0.95f)
	{
		return SprintAcceleration;
	}
	return Super::GetMaxAcceleration();
}

float UNinjaMovementComponent::GetMaxSpeed() const
{
	// Atacando: mantém um pouco do embalo, mas desacelera suave até AttackMaxSpeed
	if (const AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(CharacterOwner); Ninja && Ninja->IsAttacking() && IsMovingOnGround())
	{
		return AttackMaxSpeed;
	}

	// Correndo de lado: animações de strafe são mais lentas, a velocidade acompanha
	if (IsStrafeRunning())
	{
		const float Side = GetSidewaysAmount();
		const FVector Facing = UpdatedComponent ? ProjectToGravityFloor(UpdatedComponent->GetForwardVector()).GetSafeNormal() : FVector::ForwardVector;
		const float Back = FMath::Clamp(-FVector::DotProduct(Facing, ProjectToGravityFloor(Acceleration).GetSafeNormal()), 0.f, 1.f);
		return FMath::Lerp(FMath::Lerp(RunSpeed, RunStrafeSpeed, Side), RunBackSpeed, Back);
	}

	// Andando: de costas para a câmera (frente/lado/costas); de costas é mais devagar
	if (IsFocusing())
	{
		const FVector Facing = UpdatedComponent ? UpdatedComponent->GetForwardVector().GetSafeNormal2D() : FVector::ForwardVector;
		const float Back = FMath::Clamp(-FVector::DotProduct(Facing, Acceleration.GetSafeNormal2D()), 0.f, 1.f);
		return FMath::Lerp(FocusSpeed, FocusBackSpeed, Back);
	}

	// Correndo (ou no pulo que saiu de uma corrida), o limite de velocidade sobe.
	if (bWantsToSprint && (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking || MovementMode == MOVE_Falling))
	{
		return SprintSpeed;
	}
	if (bWantsToRun && (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking || MovementMode == MOVE_Falling))
	{
		return RunSpeed;
	}
	return Super::GetMaxSpeed();
}

bool UNinjaMovementComponent::IsStrafeRunning() const
{
	if (!bWantsToRun || bWantsToSprint || !IsMovingOnGround() || !CharacterOwner || Acceleration.SizeSquared2D() < UE_KINDA_SMALL_NUMBER)
	{
		return false;
	}
	const float Angle = FMath::Abs(FMath::FindDeltaAngleDegrees(CharacterOwner->GetControlRotation().Yaw, Acceleration.Rotation().Yaw));
	return Angle <= StrafeMaxAngle;
}

float UNinjaMovementComponent::GetSidewaysAmount() const
{
	if (!UpdatedComponent || Acceleration.SizeSquared2D() < UE_KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	return FMath::Abs(FVector::CrossProduct(UpdatedComponent->GetForwardVector().GetSafeNormal2D(), Acceleration.GetSafeNormal2D()).Z);
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
	// Na parede/teto: o giro normal da engine (relativo à gravidade)
	if (HasCustomGravity())
	{
		return Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);
	}
	// Carregando o super pulo: o corpo vira para onde a câmera olha (é para lá que ele salta)
	if (const AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(CharacterOwner); Ninja && Ninja->IsChargingSuperJump())
	{
		return FRotator(0.f, CharacterOwner->GetControlRotation().Yaw, 0.f);
	}
	// Dash de lado/de costas: o corpo continua de frente para a câmera
	if (IsDashing() && bDashStrafe)
	{
		return FRotator(0.f, DashFacingYaw, 0.f);
	}
	// Golpeando: o corpo vira (rápido, limitado pelo RotationRate — sem estalo) para onde a câmera olhava no golpe
	if (const AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(CharacterOwner); Ninja && Ninja->IsAttacking())
	{
		return FRotator(0.f, Ninja->GetAttackFacingYaw(), 0.f);
	}
	// Andando (ou correndo para frente/lado): o corpo olha para onde a câmera olha. Parado: não gira — a câmera orbita livre.
	if ((IsFocusing() || IsStrafeRunning()) && CharacterOwner)
	{
		if (Acceleration.SizeSquared2D() < UE_KINDA_SMALL_NUMBER)
		{
			return CurrentRotation;
		}
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
	// Pulo saindo da parede/teto: empurra para longe dela (e um pouco para cima) e volta a gravidade normal
	if (HasCustomGravity() && IsMovingOnGround() && CharacterOwner && CharacterOwner->CanJump())
	{
		const FVector Away = -GetGravityDirection();
		Velocity = ProjectToGravityFloor(Velocity) + Away * SurfaceJumpOffSpeed;
		ResetGravity();
		Velocity.Z = FMath::Max(Velocity.Z, SurfaceJumpOffSpeed * 0.6f);
		SurfaceAttachBlock = 0.35f;
		SetMovementMode(MOVE_Falling);
		return true;
	}
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
	bWantsToRun = (Flags & FSavedMove_Character::FLAG_Custom_3) != 0;
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

	if (IsFalling() && !IsChakraFeetActive())
	{
		TryStartWallRun();   // com a infusão o personagem anda na parede de verdade (gravidade), não corre de lado
	}

	// Parede/teto: no ar com a gravidade da superfície por um instante = saiu pela borda -> gravidade normal
	SurfaceAttachBlock = FMath::Max(0.f, SurfaceAttachBlock - DeltaSeconds);
	if (HasCustomGravity())
	{
		SurfaceAirTime = IsFalling() ? SurfaceAirTime + DeltaSeconds : 0.f;
		if (SurfaceAirTime > SurfaceLostTime || !IsChakraFeetActive())
		{
			ResetGravity();
		}
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

	// Tocou o chão: libera o dash no ar de novo e acaba o voo do super pulo.
	if (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking)
	{
		bAirDashUsed = false;
		bInSuperJump = false;
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
	// Vai para onde o jogador está apontando o movimento; sem direção, vai para frente (no plano do chão atual: parede/teto também)
	DashDirection = ProjectToGravityFloor(Acceleration).GetSafeNormal();
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = ProjectToGravityFloor(UpdatedComponent->GetForwardVector()).GetSafeNormal();
	}

	if (!IsMovingOnGround())
	{
		bAirDashUsed = true;
	}

	// Tecla para o lado/para trás em relação à câmera (no chão): o corpo fica de frente para a câmera e o dash vai de lado/de costas
	bDashStrafe = false;
	if (IsMovingOnGround() && !HasCustomGravity() && CharacterOwner && !Acceleration.IsNearlyZero())
	{
		const float ControlYaw = CharacterOwner->GetControlRotation().Yaw;
		if (FMath::Abs(FMath::FindDeltaAngleDegrees(ControlYaw, DashDirection.Rotation().Yaw)) > DashStrafeAngle)
		{
			bDashStrafe = true;
			DashFacingYaw = ControlYaw;
			MoveUpdatedComponent(FVector::ZeroVector, FRotator(0.f, ControlYaw, 0.f).Quaternion(), false);
		}
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
	// Correndo: sai do dash já na velocidade da corrida (a estrela emenda na corrida sem parar).
	// Sem correr: com animação mantém a velocidade final dela (freia junto); sem animação, sai andando.
	if (bWantsToSprint || bWantsToRun || !DashRootMotionAnim)
	{
		Velocity = DashDirection * (bWantsToSprint ? SprintSpeed : (bWantsToRun ? RunSpeed : MaxWalkSpeed));
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
		const double From = DashAnimStart + Elapsed * DashAnimRate;
		const FTransform Motion = DashRootMotionAnim->ExtractRootMotionFromRange(From, From + StepTime * DashAnimRate, FAnimExtractContext());
		Speed = Motion.GetTranslation().Size2D() * DashDistanceScale / StepTime;
		const float EntrySpeed = bWantsToSprint ? SprintSpeed : (bWantsToRun ? RunSpeed : MaxWalkSpeed);
		Speed = FMath::Max(Speed, EntrySpeed * DashMinSpeedFraction);
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
	if (!(bWantsToSprint || bWantsToRun) || WallRunCooldownLeft > 0.f || Velocity.Size2D() < WallRunMinSpeed || Acceleration.IsNearlyZero())
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
	if (!(bWantsToSprint || bWantsToRun) || WallRunTimeLeft <= 0.f || !bStillOnWall)
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
	bSavedWantsToRun = false;
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
	if (bSavedWantsToRun)
	{
		Flags |= FLAG_Custom_3;
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
		bSavedWantsToRun = Move->bWantsToRun;
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

// ---------------------------------------------------------------------------------------------------------------------
// Infusão de chakra nos pés: parede, teto e água
// ---------------------------------------------------------------------------------------------------------------------

bool UNinjaMovementComponent::IsChakraFeetActive() const
{
	const AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(CharacterOwner);
	return Ninja && Ninja->IsPassiveEnabled(ENinjaPassive::ChakraFeet);
}

void UNinjaMovementComponent::OnChakraFeetChanged(bool bEnabled)
{
	// Água: com a infusão a superfície bloqueia (pisa em cima); sem ela, passa e nada
	if (CharacterOwner && CharacterOwner->GetCapsuleComponent())
	{
		CharacterOwner->GetCapsuleComponent()->SetCollisionResponseToChannel(WaterChannel, bEnabled ? ECR_Block : ECR_Overlap);
	}
	if (!bEnabled && HasCustomGravity())
	{
		ResetGravity();   // desligou na parede/teto: cai
	}
}

void UNinjaMovementComponent::ResetGravity()
{
	SetGravityDirection(DefaultGravityDirection);
	SurfaceAirTime = 0.f;
	if (IsMovingOnGround())
	{
		SetMovementMode(MOVE_Falling);   // o chão de antes (parede/teto) não serve mais
	}
}

void UNinjaMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	Super::HandleImpact(Hit, TimeSlice, MoveDelta);
	TryAttachToSurface(Hit);
}

bool UNinjaMovementComponent::TryAttachToSurface(const FHitResult& Hit)
{
	// Com a infusão, indo contra uma parede/teto (não um chão), ela vira o chão: a gravidade passa a apontar para ela
	if (!IsChakraFeetActive() || SurfaceAttachBlock > 0.f || !Hit.bBlockingHit || !(IsMovingOnGround() || IsFalling() || IsDashing()))
	{
		return false;
	}
	if (Cast<APawn>(Hit.GetActor()))
	{
		return false;
	}
	const FVector Normal = Hit.ImpactNormal.GetSafeNormal();
	const FVector Up = -GetGravityDirection();
	if (FVector::DotProduct(Normal, Up) > 0.7f)
	{
		return false;   // já é chão para a gravidade atual
	}
	const FVector Wish = Acceleration.IsNearlyZero() ? Velocity : Acceleration;
	if (FVector::DotProduct(Wish.GetSafeNormal(), -Normal) < 0.35f)
	{
		return false;   // só quando está indo de encontro à superfície (tecla apertada para ela / pulando nela)
	}
	SetGravityDirection(-Normal);
	Velocity = FVector::VectorPlaneProject(Velocity, Normal);
	SurfaceAirTime = 0.f;
	if (IsDashing())
	{
		DashDirection = ProjectToGravityFloor(DashDirection).GetSafeNormal();
	}
	return true;
}

void UNinjaMovementComponent::ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration)
{
	if (bInSuperJump && IsFalling())
	{
		return;   // o personagem tem atrito de frenagem separado (BrakingFriction) que valeria também no ar
	}
	Super::ApplyVelocityBraking(DeltaTime, Friction, BrakingDeceleration);
}
