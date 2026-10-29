// Torção distribuída no corpo visível (depois do retarget).

#include "NinjaBodyAnimInstance.h"

#include "Animation/AnimNodeBase.h"
#include "BonePose.h"

UNinjaBodyAnimInstance::UNinjaBodyAnimInstance()
{
	// Antebraço e canela seguem a torção da mão / do pé; braço e coxa desfazem a própria torção perto do ombro / quadril
	for (const TCHAR* Side : { TEXT("l"), TEXT("r") })
	{
		auto Add = [&](const TCHAR* Twist, const TCHAR* Driver, const TCHAR* From, const TCHAR* To, float Fraction)
		{
			TwistBones.Add({ FName(FString::Printf(TEXT("%s_%s"), Twist, Side)), FName(FString::Printf(TEXT("%s_%s"), Driver, Side)),
				FName(FString::Printf(TEXT("%s_%s"), From, Side)), FName(FString::Printf(TEXT("%s_%s"), To, Side)), Fraction });
		};
		Add(TEXT("lowerarm_twist_01"), TEXT("hand"), TEXT("lowerarm"), TEXT("hand"), 0.5f);
		Add(TEXT("lowerarm_twist_02"), TEXT("hand"), TEXT("lowerarm"), TEXT("hand"), 1.f);
		Add(TEXT("upperarm_twist_01"), TEXT("upperarm"), TEXT("upperarm"), TEXT("lowerarm"), -1.f);
		Add(TEXT("upperarm_twist_02"), TEXT("upperarm"), TEXT("upperarm"), TEXT("lowerarm"), -0.5f);
		Add(TEXT("thigh_twist_01"), TEXT("thigh"), TEXT("thigh"), TEXT("calf"), -1.f);
		Add(TEXT("thigh_twist_02"), TEXT("thigh"), TEXT("thigh"), TEXT("calf"), -0.5f);
		Add(TEXT("calf_twist_01"), TEXT("foot"), TEXT("calf"), TEXT("foot"), 0.5f);
		Add(TEXT("calf_twist_02"), TEXT("foot"), TEXT("calf"), TEXT("foot"), 1.f);
	}
}

void FNinjaBodyAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	if (const UNinjaBodyAnimInstance* Body = Cast<UNinjaBodyAnimInstance>(InAnimInstance))
	{
		TwistBones = Body->TwistBones;
	}
}

bool FNinjaBodyAnimInstanceProxy::Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode)
{
	EvaluateAnimationNode_WithRoot(Output, InRootNode);
	if (InRootNode == GetRootNode())
	{
		ApplyTwist(Output);
	}
	return true;
}

void FNinjaBodyAnimInstanceProxy::ApplyTwist(FPoseContext& Output) const
{
	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	const FReferenceSkeleton& RefSkeleton = Bones.GetReferenceSkeleton();
	const TArray<FTransform>& RefLocal = RefSkeleton.GetRefBonePose();
	auto RefComponent = [&](int32 MeshIndex)
	{
		FTransform Result = FTransform::Identity;
		for (int32 Index = MeshIndex; Index != INDEX_NONE; Index = RefSkeleton.GetParentIndex(Index))
		{
			Result = Result * RefLocal[Index];
		}
		return Result;
	};
	for (const FNinjaTwistBone& Twist : TwistBones)
	{
		const int32 TwistMesh = RefSkeleton.FindBoneIndex(Twist.TwistBone);
		const int32 DriverMesh = RefSkeleton.FindBoneIndex(Twist.DriverBone);
		const int32 FromMesh = RefSkeleton.FindBoneIndex(Twist.AxisFrom);
		const int32 ToMesh = RefSkeleton.FindBoneIndex(Twist.AxisTo);
		if (TwistMesh == INDEX_NONE || DriverMesh == INDEX_NONE || FromMesh == INDEX_NONE || ToMesh == INDEX_NONE)
		{
			continue;
		}
		const FCompactPoseBoneIndex TwistIndex = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(TwistMesh));
		const FCompactPoseBoneIndex DriverIndex = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(DriverMesh));
		if (TwistIndex == INDEX_NONE || DriverIndex == INDEX_NONE)
		{
			continue;
		}
		// Eixo do membro na pose de referência, no espaço do próprio osso
		const FVector AxisComponent = (RefComponent(ToMesh).GetLocation() - RefComponent(FromMesh).GetLocation()).GetSafeNormal();
		if (AxisComponent.IsNearlyZero())
		{
			continue;
		}
		const FVector DriverAxis = RefComponent(DriverMesh).GetRotation().UnrotateVector(AxisComponent);
		const FVector TwistAxis = RefComponent(TwistMesh).GetRotation().UnrotateVector(AxisComponent);
		// Torção do Driver em relação à referência (no espaço dele) em volta do eixo
		const FQuat Delta = RefLocal[DriverMesh].GetRotation().Inverse() * Output.Pose[DriverIndex].GetRotation();
		FQuat Swing, TwistQuat;
		Delta.ToSwingTwist(DriverAxis, Swing, TwistQuat);
		float Angle = TwistQuat.GetTwistAngle(DriverAxis);
		Angle = FMath::UnwindRadians(Angle);
		const FQuat Apply(TwistAxis, Angle * Twist.Fraction);
		FTransform& Local = Output.Pose[TwistIndex];
		Local.SetRotation((RefLocal[TwistMesh].GetRotation() * Apply).GetNormalized());
	}
}
