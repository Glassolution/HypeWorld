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
