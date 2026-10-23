// Retoques procedurais na pose do ninja (mão na parede, preparar o pouso, agachar ao pousar).

#include "NinjaAnimInstance.h"
#include "NinjaMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/BoneReference.h"
#include "BonePose.h"
#include "TwoBoneIK.h"

// ---------------------------------------------------------------------------------------------------------------------
// Jogo (thread principal): decide o que a pose deve fazer
// ---------------------------------------------------------------------------------------------------------------------

void UNinjaAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	ACharacter* Char = Cast<ACharacter>(TryGetPawnOwner());
	USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	UNinjaMovementComponent* Move = Char ? Cast<UNinjaMovementComponent>(Char->GetCharacterMovement()) : nullptr;
	if (!Char || !Mesh || !Move || DeltaSeconds <= 0.f)
	{
		return;
	}

	const FTransform CompTM = Mesh->GetComponentTransform();
	const FVector Forward = Char->GetActorForwardVector();
	PoseInputs.ForwardCS = CompTM.InverseTransformVectorNoScale(Forward);
	PoseInputs.RightCS = CompTM.InverseTransformVectorNoScale(Char->GetActorRightVector());
	PoseInputs.UpCS = CompTM.InverseTransformVectorNoScale(FVector::UpVector);

	// --- Mão na parede: ponto da parede na altura do ombro, um pouco à frente e abaixo ---
	float WallTarget = 0.f;
	if (Move->IsWallRunning())
	{
		FHitResult Hit;
		float Side = 1.f;
		bool bHit = Move->TraceForWall(Char->GetActorRightVector(), Hit);
		if (!bHit)
		{
			Side = -1.f;
			bHit = Move->TraceForWall(-Char->GetActorRightVector(), Hit);
		}
		if (bHit)
		{
			const FVector Shoulder = Mesh->GetSocketLocation(Side > 0.f ? FName("upperarm_r") : FName("upperarm_l"));
			const FVector Normal = Hit.ImpactNormal.GetSafeNormal2D();
			FVector Target = Shoulder - Normal * FVector::DotProduct(Shoulder - Hit.ImpactPoint, Normal);
			Target += Normal * 6.f + Forward * 25.f - FVector::UpVector * 15.f;
			PoseInputs.WallHandTargetCS = CompTM.InverseTransformPosition(Target);
			PoseInputs.WallSide = Side;
			WallTarget = 1.f;
		}
	}
	PoseInputs.WallHandAlpha = FMath::FInterpTo(PoseInputs.WallHandAlpha, WallTarget, DeltaSeconds, WallHandBlendSpeed);

	// --- Caindo perto do chão: olha para baixo e encolhe as pernas ---
	const FVector Velocity = Char->GetVelocity();
	const bool bInAir = Move->IsFalling() || Move->IsWallRunning();
	float LookTarget = 0.f;
	if (bInAir && Velocity.Z < -150.f)
	{
		const float HalfHeight = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const FVector Start = Char->GetActorLocation();
		const FVector End = Start - FVector::UpVector * (HalfHeight + PrepareLandDistance);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(NinjaGroundTrace), false, Char);
		FHitResult Ground;
		if (GetWorld()->LineTraceSingleByChannel(Ground, Start, End, ECC_Visibility, Params))
		{
			const float DistanceToGround = Ground.Distance - HalfHeight;
			LookTarget = FMath::Clamp(1.f - DistanceToGround / PrepareLandDistance, 0.f, 1.f);
		}
	}
	PoseInputs.LookDownAlpha = FMath::FInterpTo(PoseInputs.LookDownAlpha, LookTarget, DeltaSeconds, 6.f);

	// --- Pouso: agacha proporcional à velocidade da queda ---
	if (bWasInAir && Move->IsMovingOnGround())
	{
		const float FallSpeed = -LastVelocityZ;
		if (FallSpeed > LandMinFallSpeed)
		{
			const float T = FMath::Clamp((FallSpeed - LandMinFallSpeed) / FMath::Max(1.f, LandMaxFallSpeed - LandMinFallSpeed), 0.f, 1.f);
			LandStrength = FMath::Lerp(0.35f, 1.f, T);
			LandTime = 0.f;
			PoseInputs.LookDownAlpha = 0.f;
		}
	}
	bWasInAir = bInAir;
	LastVelocityZ = Velocity.Z;

	float LandAlpha = 0.f;
	if (LandStrength > 0.f)
	{
		LandTime += DeltaSeconds;
		const float InTime = 0.07f; // desce rápido, sobe devagar
		if (LandTime < InTime)
		{
			LandAlpha = LandTime / InTime;
		}
		else if (LandTime < LandDuration)
		{
			LandAlpha = 1.f - FMath::SmoothStep(InTime, LandDuration, LandTime);
		}
		else
		{
			LandStrength = 0.f;
		}
		LandAlpha *= LandStrength;
	}
	PoseInputs.LandAlpha = LandAlpha;
}

// ---------------------------------------------------------------------------------------------------------------------
// Pose (thread de animação): aplica os retoques depois do gráfico
// ---------------------------------------------------------------------------------------------------------------------

void FNinjaAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);

	if (const UNinjaAnimInstance* Ninja = Cast<UNinjaAnimInstance>(InAnimInstance))
	{
		Inputs = Ninja->PoseInputs;
		LandCrouchDepth = Ninja->LandCrouchDepth;
		LookDownSpineAngle = Ninja->LookDownSpineAngle;
		LookDownHeadAngle = Ninja->LookDownHeadAngle;
		AirLegLift = Ninja->AirLegLift;
		LandSpineAngle = Ninja->LandSpineAngle;
	}
}

bool FNinjaAnimInstanceProxy::Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode)
{
	// Primeiro a animação normal (gráfico do Animation Blueprint), depois os retoques.
	EvaluateAnimationNode_WithRoot(Output, InRootNode);
	if (InRootNode == GetRootNode())
	{
		ApplyNinjaPose(Output);
	}
	return true;
}

void FNinjaAnimInstanceProxy::ApplyNinjaPose(FPoseContext& Output) const
{
	if (Inputs.WallHandAlpha < 0.01f && Inputs.LookDownAlpha < 0.01f && Inputs.LandAlpha < 0.01f)
	{
		return;
	}

	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	auto Bone = [&Bones](const TCHAR* Name)
	{
		FBoneReference Ref(Name);
		Ref.Initialize(Bones);
		return Ref.GetCompactPoseIndex(Bones);
	};

	const FCompactPoseBoneIndex Pelvis = Bone(TEXT("pelvis"));
	const FCompactPoseBoneIndex Spine03 = Bone(TEXT("spine_03"));
	const FCompactPoseBoneIndex Spine05 = Bone(TEXT("spine_05"));
	const FCompactPoseBoneIndex Neck = Bone(TEXT("neck_01"));
	const FCompactPoseBoneIndex Head = Bone(TEXT("head"));

	FCSPose<FCompactPose> CS;
	CS.InitPose(Output.Pose);

	const FVector Up = Inputs.UpCS.GetSafeNormal();
	const FVector Forward = Inputs.ForwardCS.GetSafeNormal();
	const FVector Right = Inputs.RightCS.GetSafeNormal();
	const FVector BendAxis = FVector::CrossProduct(Up, Forward).GetSafeNormal(); // ângulo positivo = curva para frente

	auto RotateBone = [&CS](FCompactPoseBoneIndex Index, const FVector& Axis, float Degrees)
	{
		if (!Index.IsValid() || FMath::IsNearlyZero(Degrees))
		{
			return;
		}
		FTransform T = CS.GetComponentSpaceTransform(Index);
		T.SetRotation(FQuat(Axis, FMath::DegreesToRadians(Degrees)) * T.GetRotation());
		const FBoneTransform Changed[] = { FBoneTransform(Index, T) };
		CS.LocalBlendCSBoneTransforms(Changed, 1.f);
	};

	auto SolveLimb = [&CS](FCompactPoseBoneIndex Root, FCompactPoseBoneIndex Mid, FCompactPoseBoneIndex End, const FVector& Effector, const FVector& PoleDirection)
	{
		if (!Root.IsValid() || !Mid.IsValid() || !End.IsValid())
		{
			return;
		}
		FTransform RootT = CS.GetComponentSpaceTransform(Root);
		FTransform MidT = CS.GetComponentSpaceTransform(Mid);
		FTransform EndT = CS.GetComponentSpaceTransform(End);
		const FVector JointTarget = MidT.GetLocation() + PoleDirection * 50.f;
		AnimationCore::SolveTwoBoneIK(RootT, MidT, EndT, JointTarget, Effector, true, 1.0, 1.15);
		const FBoneTransform Changed[] = { FBoneTransform(Root, RootT), FBoneTransform(Mid, MidT), FBoneTransform(End, EndT) };
		CS.LocalBlendCSBoneTransforms(Changed, 1.f);
	};

	// Posição dos pés antes de agachar (no pouso eles ficam onde estavam)
	const FCompactPoseBoneIndex ThighL = Bone(TEXT("thigh_l")), CalfL = Bone(TEXT("calf_l")), FootL = Bone(TEXT("foot_l"));
	const FCompactPoseBoneIndex ThighR = Bone(TEXT("thigh_r")), CalfR = Bone(TEXT("calf_r")), FootR = Bone(TEXT("foot_r"));
	const FVector FootLPos = FootL.IsValid() ? CS.GetComponentSpaceTransform(FootL).GetLocation() : FVector::ZeroVector;
	const FVector FootRPos = FootR.IsValid() ? CS.GetComponentSpaceTransform(FootR).GetLocation() : FVector::ZeroVector;

	// 1) Pouso: quadril desce
	if (Inputs.LandAlpha > 0.01f && Pelvis.IsValid())
	{
		FTransform T = CS.GetComponentSpaceTransform(Pelvis);
		T.AddToTranslation(-Up * LandCrouchDepth * Inputs.LandAlpha);
		const FBoneTransform Changed[] = { FBoneTransform(Pelvis, T) };
		CS.LocalBlendCSBoneTransforms(Changed, 1.f);
	}

	// 2) Tronco e cabeça curvam para frente (olhar o chão / absorver o pouso)
	const float SpineDegrees = LookDownSpineAngle * Inputs.LookDownAlpha + LandSpineAngle * Inputs.LandAlpha;
	RotateBone(Spine03, BendAxis, SpineDegrees * 0.5f);
	RotateBone(Spine05, BendAxis, SpineDegrees * 0.5f);
	RotateBone(Neck, BendAxis, LookDownHeadAngle * Inputs.LookDownAlpha * 0.4f);
	RotateBone(Head, BendAxis, LookDownHeadAngle * Inputs.LookDownAlpha * 0.6f);

	// 3) Pernas: no pouso os pés ficam no lugar (joelhos dobram); no ar encolhem
	const float Lift = AirLegLift * Inputs.LookDownAlpha;
	if (Inputs.LandAlpha > 0.01f || Lift > 0.1f)
	{
		SolveLimb(ThighL, CalfL, FootL, FootLPos + Up * Lift, Forward);
		SolveLimb(ThighR, CalfR, FootR, FootRPos + Up * Lift, Forward);
	}

	// 4) Braços na parede: um apoia na parede, o outro abre para equilibrar
	if (Inputs.WallHandAlpha > 0.01f)
	{
		const bool bWallRight = Inputs.WallSide > 0.f;
		const FCompactPoseBoneIndex WallUpper = Bone(bWallRight ? TEXT("upperarm_r") : TEXT("upperarm_l"));
		const FCompactPoseBoneIndex WallLower = Bone(bWallRight ? TEXT("lowerarm_r") : TEXT("lowerarm_l"));
		const FCompactPoseBoneIndex WallHand = Bone(bWallRight ? TEXT("hand_r") : TEXT("hand_l"));
		const FCompactPoseBoneIndex FreeUpper = Bone(bWallRight ? TEXT("upperarm_l") : TEXT("upperarm_r"));
		const FCompactPoseBoneIndex FreeLower = Bone(bWallRight ? TEXT("lowerarm_l") : TEXT("lowerarm_r"));
		const FCompactPoseBoneIndex FreeHand = Bone(bWallRight ? TEXT("hand_l") : TEXT("hand_r"));
		const FVector AwayFromWall = bWallRight ? -Right : Right;

		if (WallHand.IsValid())
		{
			const FVector Current = CS.GetComponentSpaceTransform(WallHand).GetLocation();
			const FVector Effector = FMath::Lerp(Current, Inputs.WallHandTargetCS, Inputs.WallHandAlpha);
			SolveLimb(WallUpper, WallLower, WallHand, Effector, (-Up - Forward).GetSafeNormal());
		}
		if (FreeUpper.IsValid() && FreeHand.IsValid())
		{
			const FVector Shoulder = CS.GetComponentSpaceTransform(FreeUpper).GetLocation();
			const FVector Current = CS.GetComponentSpaceTransform(FreeHand).GetLocation();
			const FVector Balance = Shoulder + AwayFromWall * 40.f - Up * 35.f + Forward * 10.f;
			const FVector Effector = FMath::Lerp(Current, Balance, Inputs.WallHandAlpha * 0.7f);
			SolveLimb(FreeUpper, FreeLower, FreeHand, Effector, (-Up + AwayFromWall).GetSafeNormal());
		}
	}

	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(CS), Output.Pose);
}
