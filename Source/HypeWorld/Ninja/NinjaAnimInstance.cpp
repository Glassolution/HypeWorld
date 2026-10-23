// Retoques procedurais na pose do ninja (mão na parede, preparar o pouso, agachar ao pousar).

#include "NinjaAnimInstance.h"
#include "NinjaMovementComponent.h"
#include "HypeWorldCharacter.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/BoneReference.h"
#include "BonePose.h"
#include "TwoBoneIK.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationPoseData.h"

namespace
{
	const TCHAR* GFingerBones[] = {
		TEXT("thumb_01"), TEXT("thumb_02"), TEXT("thumb_03"),
		TEXT("index_metacarpal"), TEXT("index_01"), TEXT("index_02"), TEXT("index_03"),
		TEXT("middle_metacarpal"), TEXT("middle_01"), TEXT("middle_02"), TEXT("middle_03"),
		TEXT("ring_metacarpal"), TEXT("ring_01"), TEXT("ring_02"), TEXT("ring_03"),
		TEXT("pinky_metacarpal"), TEXT("pinky_01"), TEXT("pinky_02"), TEXT("pinky_03"),
	};
}

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
		if (Move->FindWallBeside(Hit, Side))
		{
			// Palma na parede, um pouco atrás e abaixo do ombro (a mão "arrasta" na parede)
			const FVector Shoulder = Mesh->GetSocketLocation(Side > 0.f ? FName("upperarm_r") : FName("upperarm_l"));
			const FVector Normal = Hit.ImpactNormal.GetSafeNormal2D();
			FVector Target = Shoulder - Normal * FVector::DotProduct(Shoulder - Hit.ImpactPoint, Normal);
			Target += Normal * 4.f - Forward * 5.f - FVector::UpVector * 20.f;
			PoseInputs.WallHandTargetCS = CompTM.InverseTransformPosition(Target);
			PoseInputs.WallNormalCS = CompTM.InverseTransformVectorNoScale(Normal);
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

	// --- Corrida ninja: braços esticados para trás (pela velocidade, então funciona nos outros jogadores também) ---
	const float SprintTarget = (Move->IsMovingOnGround() && Velocity.Size2D() > SprintPoseMinSpeed) ? 1.f : 0.f;
	PoseInputs.SprintAlpha = FMath::FInterpTo(PoseInputs.SprintAlpha, SprintTarget, DeltaSeconds, 6.f);

	// --- Concentrando chakra: base aberta e mãos juntas no peito ---
	const AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(Char);
	const float ChargeTarget = (Ninja && Ninja->IsChargingChakra()) ? 1.f : 0.f;
	PoseInputs.ChargeAlpha = FMath::FInterpConstantTo(PoseInputs.ChargeAlpha, ChargeTarget, DeltaSeconds, 4.f);   // ~0,25 s para assumir a base
	PoseInputs.Time += DeltaSeconds;

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
		RelaxedHands = Ninja->RelaxedHandsPose;
		LandCrouchDepth = Ninja->LandCrouchDepth;
		LookDownSpineAngle = Ninja->LookDownSpineAngle;
		LookDownHeadAngle = Ninja->LookDownHeadAngle;
		AirLegLift = Ninja->AirLegLift;
		LandSpineAngle = Ninja->LandSpineAngle;
		SprintExtraLean = Ninja->SprintExtraLean;
		SprintArmsDrop = Ninja->SprintArmsDrop;
		SprintCrouchDepth = Ninja->SprintCrouchDepth;
		SprintArmLock = Ninja->SprintArmLock;
		ChargeCrouchDepth = Ninja->ChargeCrouchDepth;
		ChargeStanceWidth = Ninja->ChargeStanceWidth;
		ChargeToeOut = Ninja->ChargeToeOut;
		ChargeKneeOut = Ninja->ChargeKneeOut;
		ChargeStraighten = Ninja->ChargeStraighten;
		ChargeStagger = Ninja->ChargeStagger;
		ChargeLean = Ninja->ChargeLean;
		ChargeHandForward = Ninja->ChargeHandForward;
		ChargeHandDrop = Ninja->ChargeHandDrop;
	}
}

bool FNinjaAnimInstanceProxy::Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode)
{
	// Primeiro a animação normal (gráfico do Animation Blueprint), depois os retoques.
	EvaluateAnimationNode_WithRoot(Output, InRootNode);
	if (InRootNode == GetRootNode())
	{
		ApplyRelaxedHands(Output);
		ApplyNinjaPose(Output);
	}
	return true;
}

void FNinjaAnimInstanceProxy::ApplyNinjaPose(FPoseContext& Output) const
{
	if (Inputs.WallHandAlpha < 0.01f && Inputs.LookDownAlpha < 0.01f && Inputs.LandAlpha < 0.01f && Inputs.SprintAlpha < 0.01f && Inputs.ChargeAlpha < 0.01f)
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

	// Antes de tudo, guarda para que lado aponta a palma de cada mão (os dedos dobrados da mão relaxada indicam a palma)
	const bool bWallRight = Inputs.WallSide > 0.f;
	FVector HandPalmLocal[2] = { FVector::ZeroVector, FVector::ZeroVector };     // [0] esquerda, [1] direita
	FVector HandFingersLocal[2] = { FVector::ZeroVector, FVector::ZeroVector };
	if (Inputs.WallHandAlpha > 0.01f || Inputs.ChargeAlpha > 0.01f)
	{
		FCSPose<FCompactPose> Before;
		Before.InitPose(Output.Pose);
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const FCompactPoseBoneIndex Hand = Bone(Side ? TEXT("hand_r") : TEXT("hand_l"));
			const FCompactPoseBoneIndex Knuckle = Bone(Side ? TEXT("middle_01_r") : TEXT("middle_01_l"));
			const FCompactPoseBoneIndex Tip = Bone(Side ? TEXT("middle_03_r") : TEXT("middle_03_l"));
			if (Hand.IsValid() && Knuckle.IsValid() && Tip.IsValid())
			{
				const FTransform HandT = Before.GetComponentSpaceTransform(Hand);
				const FVector Fingers = (Before.GetComponentSpaceTransform(Knuckle).GetLocation() - HandT.GetLocation()).GetSafeNormal();
				const FVector Curl = Before.GetComponentSpaceTransform(Tip).GetLocation() - Before.GetComponentSpaceTransform(Knuckle).GetLocation();
				const FVector Palm = Curl - Fingers * FVector::DotProduct(Curl, Fingers);
				if (Palm.Size() > 0.5f)
				{
					HandFingersLocal[Side] = HandT.GetRotation().UnrotateVector(Fingers);
					HandPalmLocal[Side] = HandT.GetRotation().UnrotateVector(Palm.GetSafeNormal());
				}
			}
		}
	}
	const FVector PalmLocal = HandPalmLocal[bWallRight ? 1 : 0];
	const FVector FingersLocal = HandFingersLocal[bWallRight ? 1 : 0];
	if (Inputs.WallHandAlpha > 0.01f)
	{
		OpenHand(Output.Pose, bWallRight, Inputs.WallHandAlpha);
		OpenHand(Output.Pose, !bWallRight, Inputs.WallHandAlpha * 0.6f);
	}
	if (Inputs.ChargeAlpha > 0.01f)
	{
		// Selo do tigre: indicador e médio esticados para cima, os outros dedos continuam dobrados
		OpenHand(Output.Pose, true, Inputs.ChargeAlpha, true);
		OpenHand(Output.Pose, false, Inputs.ChargeAlpha, true);
	}
	if (Inputs.SprintAlpha > 0.01f)
	{
		// Corrida ninja: mãos abertas e retas, como nas referências
		OpenHand(Output.Pose, true, Inputs.SprintAlpha * 0.8f);
		OpenHand(Output.Pose, false, Inputs.SprintAlpha * 0.8f);
	}

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

	// Aponta um osso (de Bone até Child) para uma direção, girando pelo menor caminho (sem torcer)
	auto AimBone = [&CS](FCompactPoseBoneIndex Index, FCompactPoseBoneIndex Child, const FVector& Direction, float Alpha)
	{
		if (!Index.IsValid() || !Child.IsValid() || Alpha < 0.01f)
		{
			return;
		}
		FTransform T = CS.GetComponentSpaceTransform(Index);
		const FVector Current = (CS.GetComponentSpaceTransform(Child).GetLocation() - T.GetLocation()).GetSafeNormal();
		const FQuat Turn = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(Current, Direction), Alpha);
		T.SetRotation(Turn * T.GetRotation());
		const FBoneTransform Changed[] = { FBoneTransform(Index, T) };
		CS.LocalBlendCSBoneTransforms(Changed, 1.f);
	};

	// Posição dos pés antes de agachar (no pouso eles ficam onde estavam)
	const FCompactPoseBoneIndex ThighL = Bone(TEXT("thigh_l")), CalfL = Bone(TEXT("calf_l")), FootL = Bone(TEXT("foot_l"));
	const FCompactPoseBoneIndex ThighR = Bone(TEXT("thigh_r")), CalfR = Bone(TEXT("calf_r")), FootR = Bone(TEXT("foot_r"));
	const FVector FootLPos = FootL.IsValid() ? CS.GetComponentSpaceTransform(FootL).GetLocation() : FVector::ZeroVector;
	const FVector FootRPos = FootR.IsValid() ? CS.GetComponentSpaceTransform(FootR).GetLocation() : FVector::ZeroVector;

	// 1) Pouso e corrida ninja: quadril desce
	const float CrouchDepth = LandCrouchDepth * Inputs.LandAlpha + SprintCrouchDepth * Inputs.SprintAlpha + ChargeCrouchDepth * Inputs.ChargeAlpha;
	if (CrouchDepth > 0.1f && Pelvis.IsValid())
	{
		FTransform T = CS.GetComponentSpaceTransform(Pelvis);
		T.AddToTranslation(-Up * CrouchDepth);
		if (Inputs.ChargeAlpha > 0.01f)
		{
			// Peso no meio da base (o quadril sai de cima de um pé só) + respiração lenta
			const FVector Loc = T.GetLocation();
			const FVector Horizontal = Loc - Up * FVector::DotProduct(Loc, Up);
			T.AddToTranslation(-Horizontal * 0.8f * Inputs.ChargeAlpha);
			T.AddToTranslation(Up * FMath::Sin(Inputs.Time * 2.2f) * 0.8f * Inputs.ChargeAlpha);
		}
		const FBoneTransform Changed[] = { FBoneTransform(Pelvis, T) };
		CS.LocalBlendCSBoneTransforms(Changed, 1.f);
	}

	// 1b) Concentrando chakra: quadril e ombros nivelados e de frente (desfaz a torção da animação base)
	if (Inputs.ChargeAlpha > 0.01f)
	{
		auto Square = [&CS, &Bone, Right](const TCHAR* LeftBone, const TCHAR* RightBone, FCompactPoseBoneIndex Index, float Alpha)
		{
			const FCompactPoseBoneIndex L = Bone(LeftBone), R = Bone(RightBone);
			if (!Index.IsValid() || !L.IsValid() || !R.IsValid())
			{
				return;
			}
			const FVector Across = (CS.GetComponentSpaceTransform(R).GetLocation() - CS.GetComponentSpaceTransform(L).GetLocation()).GetSafeNormal();
			FTransform T = CS.GetComponentSpaceTransform(Index);
			T.SetRotation(FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(Across, Right), Alpha) * T.GetRotation());
			const FBoneTransform Changed[] = { FBoneTransform(Index, T) };
			CS.LocalBlendCSBoneTransforms(Changed, 1.f);
		};
		Square(TEXT("thigh_l"), TEXT("thigh_r"), Pelvis, Inputs.ChargeAlpha * 0.6f);        // quadril quase de frente (sobra um pouco de naturalidade)
		Square(TEXT("upperarm_l"), TEXT("upperarm_r"), Spine05, Inputs.ChargeAlpha * 0.5f); // peito acompanha pela metade

		// Coluna ereta: cada trecho aponta mais para cima, sem rigidez total
		const FCompactPoseBoneIndex Spine01 = Bone(TEXT("spine_01"));
		AimBone(Spine01, Spine03, Up, Inputs.ChargeAlpha * ChargeStraighten);
		AimBone(Spine03, Spine05, Up, Inputs.ChargeAlpha * ChargeStraighten);
		AimBone(Spine05, Neck, Up, Inputs.ChargeAlpha * ChargeStraighten * 0.8f);
		AimBone(Neck, Head, Up, Inputs.ChargeAlpha * 0.6f);
	}

	// 2) Tronco e cabeça curvam para frente (olhar o chão / absorver o pouso)
	const float SpineDegrees = LookDownSpineAngle * Inputs.LookDownAlpha + LandSpineAngle * Inputs.LandAlpha + SprintExtraLean * Inputs.SprintAlpha
		+ (ChargeLean + FMath::Sin(Inputs.Time * 2.2f) * 1.0f) * Inputs.ChargeAlpha;   // concentrando: tronco entra no selo, respirando
	RotateBone(Spine03, BendAxis, SpineDegrees * 0.5f);
	RotateBone(Spine05, BendAxis, SpineDegrees * 0.5f);
	RotateBone(Neck, BendAxis, LookDownHeadAngle * Inputs.LookDownAlpha * 0.4f);
	RotateBone(Head, BendAxis, LookDownHeadAngle * Inputs.LookDownAlpha * 0.6f);

	// 3) Pernas: no pouso os pés ficam no lugar (joelhos dobram); no ar encolhem
	const float Lift = AirLegLift * Inputs.LookDownAlpha;
	if (CrouchDepth > 0.1f || Lift > 0.1f)
	{
		const float Ground = FMath::Min(FVector::DotProduct(FootLPos, Up), FVector::DotProduct(FootRPos, Up));
		const float StepArc = FMath::Sin(PI * Inputs.ChargeAlpha) * 7.f;   // o pé sobe no meio da transição: é um passo, não um deslize
		for (const bool bRight : { false, true })
		{
			const FVector Side = bRight ? Right : -Right;
			const FVector AnimFoot = bRight ? FootRPos : FootLPos;
			const FVector StanceFoot = Side * (ChargeStanceWidth * 0.5f) + Forward * (bRight ? -ChargeStagger : ChargeStagger) * 0.5f + Up * Ground;
			const FVector Target = FMath::Lerp(AnimFoot, StanceFoot, Inputs.ChargeAlpha) + Up * (Lift + StepArc);
			// Joelho aponta para a ponta do pé: para frente e para fora na base de concentração
			const FVector Pole = FMath::Lerp(Forward, (Forward + Side * ChargeKneeOut).GetSafeNormal(), Inputs.ChargeAlpha).GetSafeNormal();
			SolveLimb(bRight ? ThighR : ThighL, bRight ? CalfR : CalfL, bRight ? FootR : FootL, Target, Pole);

			// Ponta do pé para fora
			const FCompactPoseBoneIndex Foot = bRight ? FootR : FootL;
			if (Foot.IsValid() && Inputs.ChargeAlpha > 0.01f)
			{
				FTransform T = CS.GetComponentSpaceTransform(Foot);
				const FVector Out = (Forward + Side * FMath::Tan(FMath::DegreesToRadians(ChargeToeOut))).GetSafeNormal();
				const FQuat Turn = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(Forward, Out), Inputs.ChargeAlpha);
				T.SetRotation(Turn * T.GetRotation());
				const FBoneTransform Changed[] = { FBoneTransform(Foot, T) };
				CS.LocalBlendCSBoneTransforms(Changed, 1.f);
			}
		}
	}

	// 4) Corrida ninja: os dois braços esticados para trás, um pouco abertos (como nas referências)
	if (Inputs.SprintAlpha > 0.01f)
	{
		for (const bool bRight : { true, false })
		{
			const FCompactPoseBoneIndex Upper = Bone(bRight ? TEXT("upperarm_r") : TEXT("upperarm_l"));
			const FCompactPoseBoneIndex Lower = Bone(bRight ? TEXT("lowerarm_r") : TEXT("lowerarm_l"));
			const FCompactPoseBoneIndex Hand = Bone(bRight ? TEXT("hand_r") : TEXT("hand_l"));
			const FVector Side = bRight ? Right : -Right;
			AimBone(Upper, Lower, (-Forward - Up * SprintArmsDrop + Side * 0.15f).GetSafeNormal(), Inputs.SprintAlpha * SprintArmLock);   // sobra um pouco do balanço da animação
			if (Upper.IsValid() && Lower.IsValid())
			{
				const FVector UpperDir = (CS.GetComponentSpaceTransform(Lower).GetLocation() - CS.GetComponentSpaceTransform(Upper).GetLocation()).GetSafeNormal();
				AimBone(Lower, Hand, UpperDir, Inputs.SprintAlpha * 0.95f);   // antebraço reto, seguindo o braço
			}
		}
	}

	// 5) Concentrando chakra: ombros um pouco à frente, mãos juntas na frente do peito (selo), cotovelos abertos, queixo baixo
	if (Inputs.ChargeAlpha > 0.01f && Spine05.IsValid())
	{
		RotateBone(Head, BendAxis, 6.f * Inputs.ChargeAlpha);
		const FCompactPoseBoneIndex NeckBase = Neck.IsValid() ? Neck : Spine05;
		const FVector Chest = CS.GetComponentSpaceTransform(NeckBase).GetLocation() + Forward * ChargeHandForward - Up * ChargeHandDrop;   // perto do queixo
		for (const bool bRight : { true, false })
		{
			const FCompactPoseBoneIndex Clavicle = Bone(bRight ? TEXT("clavicle_r") : TEXT("clavicle_l"));
			const FCompactPoseBoneIndex Upper = Bone(bRight ? TEXT("upperarm_r") : TEXT("upperarm_l"));
			const FCompactPoseBoneIndex Lower = Bone(bRight ? TEXT("lowerarm_r") : TEXT("lowerarm_l"));
			const FCompactPoseBoneIndex Hand = Bone(bRight ? TEXT("hand_r") : TEXT("hand_l"));
			const FVector Side = bRight ? Right : -Right;

			// Ombro avança junto com as mãos (a escápula acompanha o braço)
			if (Clavicle.IsValid() && Upper.IsValid())
			{
				const FVector ClavDir = (CS.GetComponentSpaceTransform(Upper).GetLocation() - CS.GetComponentSpaceTransform(Clavicle).GetLocation()).GetSafeNormal();
				AimBone(Clavicle, Upper, (ClavDir + Forward * 0.2f + Up * 0.12f).GetSafeNormal(), Inputs.ChargeAlpha * 0.6f);   // ombros tensos: sobem e avançam
			}
			if (Hand.IsValid())
			{
				const FVector Current = CS.GetComponentSpaceTransform(Hand).GetLocation();
				const FVector Target = Chest + Side * 2.5f;
				SolveLimb(Upper, Lower, Hand, FMath::Lerp(Current, Target, Inputs.ChargeAlpha), (Side * 1.f - Up * 0.55f).GetSafeNormal());   // cotovelos para fora e para baixo

				// Palma virada para a outra mão, dedos para cima e um pouco à frente
				const int32 HandSide = bRight ? 1 : 0;
				if (!HandPalmLocal[HandSide].IsNearlyZero())
				{
					FTransform HandT = CS.GetComponentSpaceTransform(Hand);
					const FQuat HandRot = HandT.GetRotation();
					const FMatrix HandBasis = FRotationMatrix::MakeFromXZ(HandRot.RotateVector(HandFingersLocal[HandSide]), HandRot.RotateVector(HandPalmLocal[HandSide]));
					const FMatrix Wanted = FRotationMatrix::MakeFromXZ((Up + Forward * 0.25f).GetSafeNormal(), -Side);   // dedos para cima, palmas juntas
					HandT.SetRotation(FQuat::Slerp(HandRot, Wanted.ToQuat() * HandBasis.ToQuat().Inverse() * HandRot, Inputs.ChargeAlpha));
					const FBoneTransform Changed[] = { FBoneTransform(Hand, HandT) };
					CS.LocalBlendCSBoneTransforms(Changed, 1.f);
				}
			}
		}
	}

	// 6) Braços na parede: um apoia na parede, o outro abre para trás para equilibrar
	if (Inputs.WallHandAlpha > 0.01f)
	{
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

			// Gira a mão: palma virada para a parede, dedos para frente e um pouco para cima
			if (!PalmLocal.IsNearlyZero() && !Inputs.WallNormalCS.IsNearlyZero())
			{
				FTransform HandT = CS.GetComponentSpaceTransform(WallHand);
				const FQuat HandRot = HandT.GetRotation();
				const FMatrix HandBasis = FRotationMatrix::MakeFromXZ(HandRot.RotateVector(FingersLocal), HandRot.RotateVector(PalmLocal));
				const FVector WantFingers = (Forward * 0.8f + Up * 0.6f).GetSafeNormal();
				const FMatrix Wanted = FRotationMatrix::MakeFromXZ(WantFingers, -Inputs.WallNormalCS.GetSafeNormal());
				const FQuat Fix = Wanted.ToQuat() * HandBasis.ToQuat().Inverse();
				HandT.SetRotation(FQuat::Slerp(HandRot, Fix * HandRot, Inputs.WallHandAlpha));
				const FBoneTransform Changed[] = { FBoneTransform(WallHand, HandT) };
				CS.LocalBlendCSBoneTransforms(Changed, 1.f);
			}
		}
		if (FreeUpper.IsValid() && FreeLower.IsValid() && FreeHand.IsValid())
		{
			// Braço livre: gira a partir do ombro (sem IK, para o punho não torcer) — aberto, para baixo e para trás
			const FVector Wanted = (AwayFromWall - Up * 0.6f - Forward * 0.35f).GetSafeNormal();
			AimBone(FreeUpper, FreeLower, Wanted, Inputs.WallHandAlpha);
			// Antebraço quase reto, seguindo o braço
			const FVector UpperDir = (CS.GetComponentSpaceTransform(FreeLower).GetLocation() - CS.GetComponentSpaceTransform(FreeUpper).GetLocation()).GetSafeNormal();
			AimBone(FreeLower, FreeHand, (UpperDir - Up * 0.25f).GetSafeNormal(), Inputs.WallHandAlpha * 0.6f);
		}
	}

	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(CS), Output.Pose);
}

void FNinjaAnimInstanceProxy::OpenHand(FCompactPose& Pose, bool bRight, float Alpha, bool bOnlyIndexAndMiddle) const
{
	const FBoneContainer& Bones = Pose.GetBoneContainer();
	for (const TCHAR* Finger : GFingerBones)
	{
		if (bOnlyIndexAndMiddle && !FCString::Strstr(Finger, TEXT("index")) && !FCString::Strstr(Finger, TEXT("middle")))
		{
			continue;
		}
		FBoneReference Ref(FName(*FString::Printf(TEXT("%s_%s"), Finger, bRight ? TEXT("r") : TEXT("l"))));
		Ref.Initialize(Bones);
		const FCompactPoseBoneIndex Index = Ref.GetCompactPoseIndex(Bones);
		if (Index.IsValid())
		{
			FTransform& Local = Pose[Index];
			Local.SetRotation(FQuat::Slerp(Local.GetRotation(), Pose.GetRefPose(Index).GetRotation(), Alpha));
		}
	}
}

void FNinjaAnimInstanceProxy::ApplyRelaxedHands(FPoseContext& Output) const
{
	if (!RelaxedHands)
	{
		return;
	}

	// Calcula a pose da animação de mão relaxada e copia só a rotação dos dedos
	FPoseContext Relaxed(Output);
	FAnimationPoseData RelaxedData(Relaxed);
	RelaxedHands->GetAnimationPose(RelaxedData, FAnimExtractContext(0.0));

	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	for (const TCHAR* Side : { TEXT("l"), TEXT("r") })
	{
		for (const TCHAR* Finger : GFingerBones)
		{
			FBoneReference Ref(FName(*FString::Printf(TEXT("%s_%s"), Finger, Side)));
			Ref.Initialize(Bones);
			const FCompactPoseBoneIndex Index = Ref.GetCompactPoseIndex(Bones);
			if (Index.IsValid())
			{
				Output.Pose[Index].SetRotation(Relaxed.Pose[Index].GetRotation());
			}
		}
	}
}
