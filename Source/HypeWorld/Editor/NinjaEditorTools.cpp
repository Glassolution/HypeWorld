// Ferramentas chamadas pelos scripts do editor.

#include "NinjaEditorTools.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimMontage.h"
#include "Animation/Skeleton.h"
#include "AnimNotify_DoAttackTrace.h"
#include "AnimNotify_CheckCombo.h"

bool UNinjaEditorTools::RebuildBlendSpace(UBlendSpace* BlendSpace)
{
#if WITH_EDITOR
	if (BlendSpace)
	{
		BlendSpace->Modify();
		BlendSpace->ValidateSampleData();
		BlendSpace->ResampleData();
		BlendSpace->MarkPackageDirty();
		return true;
	}
#endif
	return false;
}

bool UNinjaEditorTools::BuildAttackMontage(UAnimMontage* Montage, UAnimSequenceBase* Animation, float StartTime, float EndTime,
	const TArray<float>& HitTimes, float ComboTime, float BlendIn, float BlendOut)
{
#if WITH_EDITOR
	if (!Montage || !Animation || EndTime <= StartTime)
	{
		return false;
	}
	Montage->Modify();

	// Um trecho só, no slot padrão
	if (Montage->SlotAnimTracks.Num() == 0)
	{
		Montage->SlotAnimTracks.AddDefaulted();
		Montage->SlotAnimTracks[0].SlotName = FAnimSlotGroup::DefaultSlotName;
	}
	FAnimTrack& Track = Montage->SlotAnimTracks[0].AnimTrack;
	Track.AnimSegments.Reset();
	FAnimSegment Segment;
	Segment.SetAnimReference(Animation, true);
	Segment.AnimStartTime = FMath::Max(0.f, StartTime);
	Segment.AnimEndTime = FMath::Min(EndTime, Animation->GetPlayLength());
	Segment.AnimPlayRate = 1.f;
	Segment.LoopingCount = 1;
	Segment.StartPos = 0.f;
	Track.AnimSegments.Add(Segment);
	const float Length = Segment.AnimEndTime - Segment.AnimStartTime;
	Montage->SetCompositeLength(Length);

	Montage->CompositeSections.Reset();
	Montage->AddAnimCompositeSection(FName("Default"), 0.f);

	// AnimNotifies: acertos e janela de combo (no relógio da montagem)
	Montage->Notifies.Reset();
	if (Montage->AnimNotifyTracks.Num() == 0)
	{
		Montage->AnimNotifyTracks.Add(FAnimNotifyTrack(TEXT("1"), FLinearColor::White));
	}
	auto AddNotify = [Montage, StartTime, Length](UAnimNotify* Notify, float SourceTime)
	{
		const float Time = FMath::Clamp(SourceTime - StartTime, 0.f, Length - 0.01f);
		FAnimNotifyEvent& Event = Montage->Notifies.AddDefaulted_GetRef();
		Event.Notify = Notify;
		Event.NotifyName = FName(*Notify->GetNotifyName());
		Event.Link(Montage, Time);
		Event.TriggerTimeOffset = GetTriggerTimeOffsetForType(Montage->CalculateOffsetForNotify(Time));
		Event.TrackIndex = 0;
	};
	for (const float Hit : HitTimes)
	{
		AddNotify(NewObject<UAnimNotify_DoAttackTrace>(Montage, NAME_None, RF_Transactional), Hit);
	}
	if (ComboTime >= 0.f)
	{
		AddNotify(NewObject<UAnimNotify_CheckCombo>(Montage, NAME_None, RF_Transactional), ComboTime);
	}

	Montage->BlendIn.SetBlendTime(BlendIn);
	Montage->BlendOut.SetBlendTime(BlendOut);
	Montage->bEnableAutoBlendOut = true;
	Montage->RefreshCacheData();
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UNinjaEditorTools::BuildSequenceMontage(UAnimMontage* Montage, const TArray<UAnimSequenceBase*>& Animations, const TArray<float>& Starts,
	const TArray<float>& Ends, float BlendIn, float BlendOut, bool bLoopLast, bool bHoldLastPose)
{
#if WITH_EDITOR
	if (!Montage || Animations.Num() == 0 || Starts.Num() != Animations.Num() || Ends.Num() != Animations.Num())
	{
		return false;
	}
	Montage->Modify();
	if (Montage->SlotAnimTracks.Num() == 0)
	{
		Montage->SlotAnimTracks.AddDefaulted();
		Montage->SlotAnimTracks[0].SlotName = FAnimSlotGroup::DefaultSlotName;
	}
	FAnimTrack& Track = Montage->SlotAnimTracks[0].AnimTrack;
	Track.AnimSegments.Reset();
	float Position = 0.f;
	float LastStart = 0.f;
	for (int32 i = 0; i < Animations.Num(); ++i)
	{
		UAnimSequenceBase* Animation = Animations[i];
		if (!Animation)
		{
			return false;
		}
		FAnimSegment Segment;
		Segment.SetAnimReference(Animation, true);
		Segment.AnimStartTime = FMath::Clamp(Starts[i], 0.f, Animation->GetPlayLength());
		Segment.AnimEndTime = Ends[i] < 0.f ? Animation->GetPlayLength() : FMath::Clamp(Ends[i], Segment.AnimStartTime + 0.01f, Animation->GetPlayLength());
		Segment.AnimPlayRate = 1.f;
		Segment.LoopingCount = 1;
		Segment.StartPos = Position;
		Track.AnimSegments.Add(Segment);
		LastStart = Position;
		Position += Segment.AnimEndTime - Segment.AnimStartTime;
	}
	Montage->SetCompositeLength(Position);

	Montage->CompositeSections.Reset();
	Montage->AddAnimCompositeSection(FName("Default"), 0.f);
	if (bLoopLast)
	{
		const FName Loop("Loop");
		if (LastStart > 0.f)
		{
			Montage->AddAnimCompositeSection(Loop, LastStart);
			Montage->GetAnimCompositeSection(Montage->GetSectionIndex(FName("Default"))).NextSectionName = Loop;
			Montage->GetAnimCompositeSection(Montage->GetSectionIndex(Loop)).NextSectionName = Loop;
		}
		else
		{
			Montage->GetAnimCompositeSection(0).NextSectionName = FName("Default");
		}
	}
	Montage->Notifies.Reset();
	Montage->BlendIn.SetBlendTime(BlendIn);
	Montage->BlendOut.SetBlendTime(BlendOut);
	Montage->bEnableAutoBlendOut = !bHoldLastPose;
	Montage->RefreshCacheData();
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UNinjaEditorTools::BuildSectionMontage(UAnimMontage* Montage, const TArray<UAnimSequenceBase*>& Animations, const TArray<float>& Starts,
	const TArray<float>& Ends, const TArray<FName>& SectionNames, const TArray<FName>& LoopSections, float BlendIn, float BlendOut, bool bHoldLastPose)
{
#if WITH_EDITOR
	// Monta os trechos como a BuildSequenceMontage (sem loop) e depois refaz as seções
	if (SectionNames.Num() != Animations.Num() || !BuildSequenceMontage(Montage, Animations, Starts, Ends, BlendIn, BlendOut, false, bHoldLastPose))
	{
		return false;
	}
	const FAnimTrack& Track = Montage->SlotAnimTracks[0].AnimTrack;
	Montage->CompositeSections.Reset();
	TArray<FName> Order;
	for (int32 i = 0; i < SectionNames.Num(); ++i)
	{
		const FName Name = (i == 0 && SectionNames[i].IsNone()) ? FName("Default") : SectionNames[i];
		if (!Name.IsNone())
		{
			Montage->AddAnimCompositeSection(Name, Track.AnimSegments[i].StartPos);
			Order.Add(Name);
		}
	}
	for (int32 i = 0; i < Order.Num(); ++i)
	{
		FCompositeSection& Section = Montage->GetAnimCompositeSection(Montage->GetSectionIndex(Order[i]));
		Section.NextSectionName = LoopSections.Contains(Order[i]) ? Order[i] : (Order.IsValidIndex(i + 1) ? Order[i + 1] : NAME_None);
	}
	Montage->RefreshCacheData();
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}
