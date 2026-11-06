#include "SilentStanceAnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimationRuntime.h"
#include "BonePose.h"
#include "HypeWorldCharacter.h"
#include "NinjaMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "TwoBoneIK.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
struct FSilentStanceAnimInstanceProxy final : FAnimInstanceProxy
{
    explicit FSilentStanceAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
    bool bLevelLocomotionGaze = true;
    float RelaxedHandsAlpha = 0.f;
    float SprintNeckAlpha = 0.f;
    FVector SprintLowerNeckDirection = FVector::ZeroVector;
    FVector SprintUpperNeckDirection = FVector::ZeroVector;
    FVector SprintGaze = FVector::ZeroVector;
    float IzunaStanceAlpha = 0.f;
    float IzunaIdleFeetAlpha = 0.f;
    bool bIzunaOutfit = false;

    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        bLevelLocomotionGaze = !Instance->IsAnyMontagePlaying();
        if (const ACharacter* Character = Cast<ACharacter>(Instance->TryGetPawnOwner()))
        {
            bLevelLocomotionGaze &= Character->GetCharacterMovement()->IsMovingOnGround();
            if (const AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(Character))
            {
                bLevelLocomotionGaze &= !Ninja->IsInBattleMode();
            }
        }
        const USilentStanceAnimInstance* Silent = CastChecked<USilentStanceAnimInstance>(Instance);
        const bool bLocomotionOwner = Instance->TryGetPawnOwner() != nullptr || Silent->bPreviewLocomotionHands;
        const float Goal = Silent->bUseRelaxedLocomotionHands && bLevelLocomotionGaze && bLocomotionOwner ? 0.94f : 0.f;
        RelaxedHandsAlpha = FMath::FInterpTo(RelaxedHandsAlpha, Goal, DeltaSeconds, 12.f);

        static const FName IzunaMeshName(TEXT("SK_Uchiha_Robe"));
        const USkeletalMeshComponent* Component = Instance->GetSkelMeshComponent();
        const USkeletalMesh* Mesh = Component ? Component->GetSkeletalMeshAsset() : nullptr;
        const ACharacter* Character = Cast<ACharacter>(Instance->TryGetPawnOwner());
        const USkeletalMeshComponent* Source = Character ? Character->GetMesh() : nullptr;
        const UAnimInstance* SourceAnim = Source ? Source->GetAnimInstance() : nullptr;
        UpdateSprintNeck(Instance, Cast<AHypeWorldCharacter>(Character), Component, Source, SourceAnim, DeltaSeconds);
        bIzunaOutfit = Mesh && Mesh->GetFName() == IzunaMeshName;
        const bool bCanNarrowStance = bIzunaOutfit && bLevelLocomotionGaze
            && (Character || Silent->bPreviewIzunaStance)
            && (!SourceAnim || !SourceAnim->IsAnyMontagePlaying());
        if (!bCanNarrowStance)
        {
            IzunaStanceAlpha = 0.f;
            IzunaIdleFeetAlpha = 0.f;
            return;
        }
        const float Speed = Character ? Character->GetVelocity().Size2D() : 0.f;
        const float StanceGoal = FMath::GetMappedRangeValueClamped(FVector2D(205., 470.), FVector2D(1., 0.), Speed);
        IzunaStanceAlpha = FMath::FInterpTo(IzunaStanceAlpha, StanceGoal, DeltaSeconds, 9.f);
        const float IdleFeetGoal = Character
            ? FMath::GetMappedRangeValueClamped(FVector2D(6., 75.), FVector2D(1., 0.), Speed)
            : (Silent->bPreviewIzunaIdleFeet ? 1.f : 0.f);
        IzunaIdleFeetAlpha = FMath::FInterpTo(IzunaIdleFeetAlpha, IdleFeetGoal, DeltaSeconds, 9.f);
    }

    void UpdateSprintNeck(const UAnimInstance* Instance, const AHypeWorldCharacter* Character,
        const USkeletalMeshComponent* Target, const USkeletalMeshComponent* Source,
        const UAnimInstance* SourceAnim, float DeltaSeconds)
    {
        const UNinjaMovementComponent* Move = Character ? Character->GetNinjaMovement() : nullptr;
        if (!Move || !Move->IsMovingOnGround() || Move->IsDashing() || Move->IsDashStrafe()
            || Instance->IsAnyMontagePlaying() || !SourceAnim || SourceAnim->IsAnyMontagePlaying()
            || !Target || !Source || Source == Target || !Source->GetSkeletalMeshAsset())
        {
            SprintNeckAlpha = 0.f;
            return;
        }

        // Follow the same transition speed as the gameplay sprint pose.
        SprintNeckAlpha = FMath::FInterpTo(SprintNeckAlpha, Move->IsSprinting() ? 1.f : 0.f, DeltaSeconds, 6.f);
        if (SprintNeckAlpha < KINDA_SMALL_NUMBER) { return; }

        const int32 Lower = Source->GetBoneIndex(TEXT("neck_01"));
        const int32 Upper = Source->GetBoneIndex(TEXT("neck_02"));
        const int32 Head = Source->GetBoneIndex(TEXT("head"));
        const TArray<FTransform>& SourcePose = Source->GetComponentSpaceTransforms();
        if (!SourcePose.IsValidIndex(Lower) || !SourcePose.IsValidIndex(Upper) || !SourcePose.IsValidIndex(Head))
        {
            SprintNeckAlpha = 0.f;
            return;
        }

        const FQuat SourceToTarget = Target->GetComponentQuat().Inverse() * Source->GetComponentQuat();
        SprintLowerNeckDirection = SourceToTarget.RotateVector(
            SourcePose[Upper].GetLocation() - SourcePose[Lower].GetLocation()).GetSafeNormal();
        SprintUpperNeckDirection = SourceToTarget.RotateVector(
            SourcePose[Head].GetLocation() - SourcePose[Upper].GetLocation()).GetSafeNormal();
        const FReferenceSkeleton& SourceSkeleton = Source->GetSkeletalMeshAsset()->GetRefSkeleton();
        const FQuat SourceHeadBind = FAnimationRuntime::GetComponentSpaceTransformRefPose(SourceSkeleton, Head).GetRotation();
        const FVector SourceGazeLocal = SourceHeadBind.UnrotateVector(SourceToTarget.UnrotateVector(FVector::YAxisVector));
        SprintGaze = SourceToTarget.RotateVector(SourcePose[Head].GetRotation().RotateVector(SourceGazeLocal)).GetSafeNormal();

        if (SprintLowerNeckDirection.IsNearlyZero() || SprintUpperNeckDirection.IsNearlyZero() || SprintGaze.IsNearlyZero())
        {
            SprintNeckAlpha = 0.f;
        }
    }

    void ApplySprintNeck(FPoseContext& Output, FCompactPoseBoneIndex Head, const FVector& GazeLocal) const
    {
        const FCompactPoseBoneIndex Upper = Output.Pose.GetParentBoneIndex(Head);
        if (!Upper.IsValid()) { return; }
        const FCompactPoseBoneIndex Lower = Output.Pose.GetParentBoneIndex(Upper);
        if (!Lower.IsValid()) { return; }

        const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
        FBoneReference LowerNeck(TEXT("neck_01"));
        FBoneReference UpperNeck(TEXT("neck_02"));
        LowerNeck.Initialize(Bones);
        UpperNeck.Initialize(Bones);
        if (LowerNeck.GetCompactPoseIndex(Bones) != Lower || UpperNeck.GetCompactPoseIndex(Bones) != Upper) { return; }

        FCSPose<FCompactPose> CS;
        CS.InitPose(Output.Pose);
        const float Alpha = FMath::SmoothStep(0.f, 1.f, SprintNeckAlpha);
        const FQuat HeadRotation = CS.GetComponentSpaceTransform(Head).GetRotation();
        const FQuat GazeDelta = FQuat::FindBetweenNormals(HeadRotation.RotateVector(GazeLocal), SprintGaze);
        const FQuat DesiredHead = FQuat::Slerp(FQuat::Identity, GazeDelta, Alpha) * HeadRotation;

        const FCompactPoseBoneIndex Chest = Output.Pose.GetParentBoneIndex(Lower);
        if (!Chest.IsValid()) { return; }
        const FTransform& LowerPose = CS.GetComponentSpaceTransform(Lower);
        const FTransform& UpperPose = CS.GetComponentSpaceTransform(Upper);
        const FTransform& HeadPose = CS.GetComponentSpaceTransform(Head);
        const FVector LowerDirection = (UpperPose.GetLocation() - LowerPose.GetLocation()).GetSafeNormal();
        const FVector UpperDirection = (HeadPose.GetLocation() - UpperPose.GetLocation()).GetSafeNormal();
        if (LowerDirection.IsNearlyZero() || UpperDirection.IsNearlyZero()) { return; }

        // Aim the two hinges in sequence while keeping every local translation
        // and the target's original neck lengths.
        const FQuat LowerSwing = FQuat::Slerp(FQuat::Identity,
            FQuat::FindBetweenNormals(LowerDirection, SprintLowerNeckDirection), Alpha);
        const FQuat UpperSwing = FQuat::Slerp(FQuat::Identity,
            FQuat::FindBetweenNormals(LowerSwing.RotateVector(UpperDirection), SprintUpperNeckDirection), Alpha);
        const FQuat LowerRotation = (LowerSwing * LowerPose.GetRotation()).GetNormalized();
        const FQuat UpperRotation = (UpperSwing * LowerSwing * UpperPose.GetRotation()).GetNormalized();
        const FQuat ChestRotation = CS.GetComponentSpaceTransform(Chest).GetRotation();
        Output.Pose[Lower].SetRotation((ChestRotation.Inverse() * LowerRotation).GetNormalized());
        Output.Pose[Upper].SetRotation((LowerRotation.Inverse() * UpperRotation).GetNormalized());
        Output.Pose[Head].SetRotation((UpperRotation.Inverse() * DesiredHead).GetNormalized());
    }

    void RelaxHands(FPoseContext& Output)
    {
        if (RelaxedHandsAlpha < KINDA_SMALL_NUMBER) { return; }
        const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
        FCompactPose Ref;
        Ref.SetBoneContainer(&Bones);
        Ref.ResetToRefPose();
        FCSPose<FCompactPose> RefCS;
        RefCS.InitPose(Ref);
        const auto Index = [&Bones](const FString& Name)
        {
            FBoneReference Bone{FName(*Name)};
            Bone.Initialize(Bones);
            return Bone.GetCompactPoseIndex(Bones);
        };
        for (const TCHAR* Side : {TEXT("l"), TEXT("r")})
        {
            const FString Suffix = FString(TEXT("_")) + Side;
            const FCompactPoseBoneIndex Hand = Index(TEXT("hand") + Suffix);
            if (Hand.IsValid())
            {
                Output.Pose[Hand].SetRotation(FQuat::Slerp(Output.Pose[Hand].GetRotation(), Ref[Hand].GetRotation(), RelaxedHandsAlpha).GetNormalized());
            }
            for (const TCHAR* Digit : {TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky")})
            {
                const bool bThumb = FCString::Strcmp(Digit, TEXT("thumb")) == 0;
                const FCompactPoseBoneIndex Meta = Index(FString(Digit) + TEXT("_metacarpal") + Suffix);
                if (Meta.IsValid())
                {
                    Output.Pose[Meta].SetRotation(FQuat::Slerp(Output.Pose[Meta].GetRotation(), Ref[Meta].GetRotation(), RelaxedHandsAlpha).GetNormalized());
                }
                for (int32 Joint = 1; Joint <= 3; ++Joint)
                {
                    const FString Name = FString::Printf(TEXT("%s_%02d%s"), Digit, Joint, *Suffix);
                    const FCompactPoseBoneIndex Bone = Index(Name);
                    if (!Bone.IsValid()) { continue; }
                    const FCompactPoseBoneIndex Next = Joint < 3 ? Index(FString::Printf(TEXT("%s_%02d%s"), Digit, Joint + 1, *Suffix)) : FCompactPoseBoneIndex(INDEX_NONE);
                    const FCompactPoseBoneIndex Parent = Ref.GetParentBoneIndex(Bone);
                    const FTransform& Transform = RefCS.GetComponentSpaceTransform(Bone);
                    const FVector Direction = Next.IsValid()
                        ? RefCS.GetComponentSpaceTransform(Next).GetTranslation() - Transform.GetTranslation()
                        : Transform.GetTranslation() - RefCS.GetComponentSpaceTransform(Parent).GetTranslation();
                    const FVector Axis = Transform.GetRotation().UnrotateVector(FVector::CrossProduct(Direction.GetSafeNormal(), -FVector::ZAxisVector).GetSafeNormal());
                    // Small anatomical curls, measured from this mesh's own neutral bind pose.
                    const float Degrees = bThumb ? 2.f : (Joint == 1 ? 8.f : Joint == 2 ? 12.f : 6.f);
                    const FQuat Relaxed = (Ref[Bone].GetRotation() * FQuat(Axis, FMath::DegreesToRadians(Degrees))).GetNormalized();
                    Output.Pose[Bone].SetRotation(FQuat::Slerp(Output.Pose[Bone].GetRotation(), Relaxed, RelaxedHandsAlpha).GetNormalized());
                }
            }
        }
    }

    void UpdateHandCorrectives(FPoseContext& Output)
    {
        const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
        FCompactPose Ref;
        Ref.SetBoneContainer(&Bones);
        Ref.ResetToRefPose();
        FCSPose<FCompactPose> RefCS;
        RefCS.InitPose(Ref);
        const auto Index = [&Bones](const FString& Name)
        {
            FBoneReference Bone{FName(*Name)};
            Bone.Initialize(Bones);
            return Bone.GetCompactPoseIndex(Bones);
        };
        for (const TCHAR* Side : {TEXT("l"), TEXT("r")})
        {
            for (const TCHAR* Digit : {TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky")})
            {
                const bool bThumb = FCString::Strcmp(Digit, TEXT("thumb")) == 0;
                float Ratio = 0.f;
                for (int32 Joint = 1; Joint <= 3; ++Joint)
                {
                    const FCompactPoseBoneIndex Bone = Index(FString::Printf(TEXT("%s_%02d_%s"), Digit, Joint, Side));
                    if (!Bone.IsValid()) { continue; }
                    const FCompactPoseBoneIndex Next = Joint < 3 ? Index(FString::Printf(TEXT("%s_%02d_%s"), Digit, Joint + 1, Side)) : FCompactPoseBoneIndex(INDEX_NONE);
                    const FCompactPoseBoneIndex Parent = Ref.GetParentBoneIndex(Bone);
                    const FTransform& Transform = RefCS.GetComponentSpaceTransform(Bone);
                    const FVector Direction = Next.IsValid()
                        ? RefCS.GetComponentSpaceTransform(Next).GetTranslation() - Transform.GetTranslation()
                        : Transform.GetTranslation() - RefCS.GetComponentSpaceTransform(Parent).GetTranslation();
                    const FVector Axis = Transform.GetRotation().UnrotateVector(FVector::CrossProduct(Direction.GetSafeNormal(), -FVector::ZAxisVector).GetSafeNormal());
                    FQuat Delta = (Ref[Bone].GetRotation().Inverse() * Output.Pose[Bone].GetRotation()).GetNormalized();
                    if (Delta.W < 0.f) { Delta = Delta * -1.f; }
                    const float Angle = FMath::RadiansToDegrees(2.f * FMath::Atan2(FVector::DotProduct(FVector(Delta.X, Delta.Y, Delta.Z), Axis), Delta.W));
                    const float Full = bThumb ? (Joint == 1 ? 15.f : Joint == 2 ? 45.f : 65.f) : (Joint == 1 ? 65.f : Joint == 2 ? 90.f : 60.f);
                    Ratio += FMath::Clamp(Angle / Full, 0.f, 1.f) / 3.f;
                }
                const float Corrective = FMath::Clamp((Ratio - 0.45f) / 0.55f, 0.f, 1.f);
                Output.Curve.Set(FName(*FString::Printf(TEXT("Corr_%s_%s"), Digit, Side)), Corrective);
            }
        }
    }

    void UpdateSleeveCorrectives(FPoseContext& Output) const
    {
        if (!bIzunaOutfit) { return; }

        const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
        FCSPose<FCompactPose> CS;
        CS.InitPose(Output.Pose);
        static const FName Arms[] = {TEXT("upperarm_l"), TEXT("upperarm_r")};
        static const FName Elbows[] = {TEXT("lowerarm_l"), TEXT("lowerarm_r")};
        static const FName Curves[] = {TEXT("Corr_SleeveRaise_l"), TEXT("Corr_SleeveRaise_r")};
        for (int32 Side = 0; Side < UE_ARRAY_COUNT(Arms); ++Side)
        {
            FBoneReference Arm(Arms[Side]);
            FBoneReference Elbow(Elbows[Side]);
            Arm.Initialize(Bones);
            Elbow.Initialize(Bones);
            const FCompactPoseBoneIndex ArmIndex = Arm.GetCompactPoseIndex(Bones);
            const FCompactPoseBoneIndex ElbowIndex = Elbow.GetCompactPoseIndex(Bones);
            if (!ArmIndex.IsValid() || !ElbowIndex.IsValid()) { continue; }

            const FVector Direction = (CS.GetComponentSpaceTransform(ElbowIndex).GetLocation()
                - CS.GetComponentSpaceTransform(ArmIndex).GetLocation()).GetSafeNormal();
            Output.Curve.Set(Curves[Side], FMath::SmoothStep(.50, .96, Direction.Z));
        }
    }

    void NarrowIzunaStance(FPoseContext& Output) const
    {
        if (IzunaStanceAlpha < KINDA_SMALL_NUMBER) { return; }
        const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
        FCSPose<FCompactPose> CS;
        CS.InitPose(Output.Pose);
        const auto Index = [&Bones](FName Name)
        {
            FBoneReference Bone(Name);
            Bone.Initialize(Bones);
            return Bone.GetCompactPoseIndex(Bones);
        };
        const auto RelativeRotation = [](const FTransform& Bone, const FTransform& Parent)
        {
            return (Parent.GetRotation().Inverse() * Bone.GetRotation()).GetNormalized();
        };
        const FCompactPoseBoneIndex Pelvis = Index(TEXT("pelvis"));
        if (!Pelvis.IsValid()) { return; }
        const double CenterX = CS.GetComponentSpaceTransform(Pelvis).GetTranslation().X;

        for (const TCHAR* Side : {TEXT("l"), TEXT("r")})
        {
            const double Sign = Side[0] == TEXT('l') ? 1. : -1.;
            const FString Suffix = FString(TEXT("_")) + Side;
            const FCompactPoseBoneIndex Arm = Index(FName(*(TEXT("upperarm") + Suffix)));
            const FCompactPoseBoneIndex Elbow = Index(FName(*(TEXT("lowerarm") + Suffix)));
            if (Arm.IsValid() && Elbow.IsValid())
            {
                FTransform Upper = CS.GetComponentSpaceTransform(Arm);
                const FVector Segment = CS.GetComponentSpaceTransform(Elbow).GetTranslation() - Upper.GetTranslation();
                // Only close an arm that already hangs down and away from the torso.
                const double OpenAngle = FMath::RadiansToDegrees(FMath::Atan2(Sign * Segment.X, FMath::Max(-Segment.Z, 1.)));
                const double Degrees = FMath::Clamp(OpenAngle - 3., 0., 3.0) * IzunaStanceAlpha
                    * FMath::Clamp(-Segment.Z / 15., 0., 1.);
                Upper.SetRotation((FQuat(FVector::YAxisVector, Sign * FMath::DegreesToRadians(Degrees))
                    * Upper.GetRotation()).GetNormalized());
                const FCompactPoseBoneIndex Parent = Output.Pose.GetParentBoneIndex(Arm);
                if (Parent.IsValid())
                {
                    Output.Pose[Arm].SetRotation(RelativeRotation(Upper, CS.GetComponentSpaceTransform(Parent)));
                }
            }

            const FCompactPoseBoneIndex Thigh = Index(FName(*(TEXT("thigh") + Suffix)));
            const FCompactPoseBoneIndex Calf = Index(FName(*(TEXT("calf") + Suffix)));
            const FCompactPoseBoneIndex Foot = Index(FName(*(TEXT("foot") + Suffix)));
            if (!Thigh.IsValid() || !Calf.IsValid() || !Foot.IsValid()
                || Output.Pose.GetParentBoneIndex(Calf) != Thigh || Output.Pose.GetParentBoneIndex(Foot) != Calf)
            {
                continue;
            }
            const FCompactPoseBoneIndex Hip = Output.Pose.GetParentBoneIndex(Thigh);
            if (!Hip.IsValid()) { continue; }
            FTransform UpperLeg = CS.GetComponentSpaceTransform(Thigh);
            FTransform LowerLeg = CS.GetComponentSpaceTransform(Calf);
            FTransform Ankle = CS.GetComponentSpaceTransform(Foot);
            const FVector Original = Ankle.GetTranslation();
            // Idle uses the original retargeted leg pose shared with the young Sasuke outfit.
            // Fade out the walking inset as the character comes to a stop.
            const double Inset = FMath::Clamp(Sign * (Original.X - CenterX) - 12.5, 0., 2.5)
                * IzunaStanceAlpha * (1.f - IzunaIdleFeetAlpha);
            if (Inset < KINDA_SMALL_NUMBER) { continue; }
            const FVector Target(Original.X - Sign * Inset, Original.Y, Original.Z);
            const FQuat AnkleRotation = Ankle.GetRotation();
            AnimationCore::SolveTwoBoneIK(UpperLeg, LowerLeg, Ankle, LowerLeg.GetTranslation(),
                Target, false, 1., 1.);
            if (!Ankle.GetTranslation().Equals(Target, .02)) { continue; }
            Ankle.SetRotation(AnkleRotation);
            Output.Pose[Thigh].SetRotation(RelativeRotation(UpperLeg, CS.GetComponentSpaceTransform(Hip)));
            Output.Pose[Calf].SetRotation(RelativeRotation(LowerLeg, UpperLeg));
            Output.Pose[Foot].SetRotation(RelativeRotation(Ankle, LowerLeg));
        }
    }

    virtual bool Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode) override
    {
        EvaluateAnimationNode_WithRoot(Output, InRootNode);
        if (InRootNode != GetRootNode()) { return true; }
        NarrowIzunaStance(Output);
        RelaxHands(Output);
        UpdateHandCorrectives(Output);
        UpdateSleeveCorrectives(Output);
        if (!bLevelLocomotionGaze && SprintNeckAlpha < KINDA_SMALL_NUMBER) { return true; }

        const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
        FBoneReference HeadRef(TEXT("head"));
        HeadRef.Initialize(Bones);
        const FCompactPoseBoneIndex Head = HeadRef.GetCompactPoseIndex(Bones);
        if (!Head.IsValid()) { return true; }

        FCompactPose Ref;
        Ref.SetBoneContainer(&Bones);
        Ref.ResetToRefPose();
        FCSPose<FCompactPose> RefCS;
        RefCS.InitPose(Ref);
        FCSPose<FCompactPose> CS;
        CS.InitPose(Output.Pose);

        // FBX model forward is +Y. Derive the local gaze from this mesh's bind pose.
        const FVector GazeLocal = RefCS.GetComponentSpaceTransform(Head).GetRotation().UnrotateVector(FVector::YAxisVector);
        const FQuat HeadRotation = CS.GetComponentSpaceTransform(Head).GetRotation();
        const FVector Gaze = HeadRotation.RotateVector(GazeLocal).GetSafeNormal();
        FVector Horizontal(Gaze.X, Gaze.Y, 0.f);
        if (bLevelLocomotionGaze && Horizontal.Normalize())
        {
            const FQuat Correction = FQuat::FindBetweenNormals(Gaze, Horizontal);
            const FQuat NewHeadRotation = (Correction * HeadRotation).GetNormalized();

            const FCompactPoseBoneIndex Parent = Output.Pose.GetParentBoneIndex(Head);
            FQuat ParentRotation = Parent.IsValid() ? CS.GetComponentSpaceTransform(Parent).GetRotation() : FQuat::Identity;
            // Share the pitch correction with the upper neck; yaw and animation timing remain intact.
            FBoneReference NeckRef(TEXT("neck_02"));
            NeckRef.Initialize(Bones);
            const FCompactPoseBoneIndex Neck = NeckRef.GetCompactPoseIndex(Bones);
            if (Neck.IsValid() && Parent == Neck)
            {
                const FCompactPoseBoneIndex GrandParent = Output.Pose.GetParentBoneIndex(Neck);
                const FQuat GrandParentRotation = GrandParent.IsValid() ? CS.GetComponentSpaceTransform(GrandParent).GetRotation() : FQuat::Identity;
                ParentRotation = (FQuat::Slerp(FQuat::Identity, Correction, 0.35f) * ParentRotation).GetNormalized();
                Output.Pose[Neck].SetRotation((GrandParentRotation.Inverse() * ParentRotation).GetNormalized());
            }
            Output.Pose[Head].SetRotation((ParentRotation.Inverse() * NewHeadRotation).GetNormalized());
        }
        if (SprintNeckAlpha >= KINDA_SMALL_NUMBER)
        {
            ApplySprintNeck(Output, Head, GazeLocal);
        }
        return true;
    }
};
}

FAnimInstanceProxy* USilentStanceAnimInstance::CreateAnimInstanceProxy()
{
    return new FSilentStanceAnimInstanceProxy(this);
}

void USilentStanceAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
    delete InProxy;
}
