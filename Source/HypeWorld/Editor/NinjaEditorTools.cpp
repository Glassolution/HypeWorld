// Ferramentas chamadas pelos scripts do editor.

#include "NinjaEditorTools.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Ninja/NinjaCustomization.h"
#include "Engine/DataTable.h"
#include "Curves/CurveLinearColor.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimMontage.h"
#include "Animation/Skeleton.h"
#include "AnimNotify_DoAttackTrace.h"
#include "AnimNotify_CheckCombo.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/World.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeLayerInfoObject.h"
#include "LandscapeEditTypes.h"
#include "Materials/MaterialInterface.h"
#include "UObject/Package.h"
#include "AssetRegistry/AssetRegistryModule.h"
#if WITH_EDITOR
#include "ActorFactories/ActorFactory.h"
#include "Builders/CubeBuilder.h"
#include "Engine/BlockingVolume.h"
#include "Animation/AnimBlueprint.h"
#include "Engine/SkeletalMesh.h"
#include "AnimationRuntime.h"
#include "Animation/AnimCurveMetadata.h"
#include "ChaosCloth/ChaosClothConfig.h"
#include "ClothingAsset.h"
#include "ClothingAssetFactory.h"
#include "ClothingSystemRuntimeTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Factories/AnimBlueprintFactory.h"
#include "AnimGraphNode_Root.h"
#include "AssetToolsModule.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AnimGraphNode_Slot.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScript.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraDataInterface.h"
#include "NiagaraScriptRuntimeData.h"
#include "NiagaraBakerSettings.h"
#include "NiagaraBakerOutputTexture2D.h"
#include "EdGraph/EdGraphPin.h"
#include "NiagaraNodeOutput.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "Curves/CurveLinearColor.h"
#include "Curves/CurveLinearColorAtlas.h"
#endif

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

UAnimBlueprint* UNinjaEditorTools::CreateRetargetAnimBlueprint(USkeletalMesh* TargetMesh, UObject* IKRetargeter, const FString& PackagePath, const FString& AssetName)
{
#if WITH_EDITOR
	UClass* NodeClass = FindObject<UClass>(nullptr, TEXT("/Script/IKRigDeveloper.AnimGraphNode_RetargetPoseFromMesh"));
	if (!TargetMesh || !IKRetargeter || !NodeClass)
	{
		return nullptr;
	}

	UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
	Factory->TargetSkeleton = TargetMesh->GetSkeleton();
	Factory->PreviewSkeletalMesh = TargetMesh;
	Factory->ParentClass = UAnimInstance::StaticClass();
	Factory->BlueprintType = BPTYPE_Normal;
	Factory->bTemplate = false;   // senão o Anim Blueprint sai sem esqueleto e não roda no mesh
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(AssetTools.CreateAsset(AssetName, PackagePath, UAnimBlueprint::StaticClass(), Factory));
	if (!AnimBP)
	{
		return nullptr;
	}

	UEdGraph* AnimGraph = nullptr;
	for (UEdGraph* Graph : AnimBP->FunctionGraphs)
	{
		if (Graph->GetFName() == UEdGraphSchema_K2::GN_AnimGraph)
		{
			AnimGraph = Graph;
		}
	}
	UAnimGraphNode_Root* Root = nullptr;
	for (UEdGraphNode* GraphNode : AnimGraph ? AnimGraph->Nodes : TArray<TObjectPtr<UEdGraphNode>>())
	{
		Root = Root ? Root : Cast<UAnimGraphNode_Root>(GraphNode);
	}
	if (!Root)
	{
		return nullptr;
	}

	// Nó "Retarget Pose From Mesh": o retargeter fica no struct Node (FAnimNode_RetargetPoseFromMesh::IKRetargeterAsset)
	UEdGraphNode* Retarget = NewObject<UEdGraphNode>(AnimGraph, NodeClass, NAME_None, RF_Transactional);
	if (FStructProperty* NodeProp = FindFProperty<FStructProperty>(NodeClass, TEXT("Node")))
	{
		void* NodeData = NodeProp->ContainerPtrToValuePtr<void>(Retarget);
		if (FObjectPropertyBase* AssetProp = FindFProperty<FObjectPropertyBase>(NodeProp->Struct, TEXT("IKRetargeterAsset")))
		{
			AssetProp->SetObjectPropertyValue_InContainer(NodeData, IKRetargeter);
		}
	}
	AnimGraph->AddNode(Retarget, false, false);
	Retarget->CreateNewGuid();
	Retarget->PostPlacedNewNode();
	Retarget->AllocateDefaultPins();
	Retarget->NodePosX = Root->NodePosX - 400;
	Retarget->NodePosY = Root->NodePosY;

	UEdGraphPin* PoseOut = nullptr;
	UEdGraphPin* ResultIn = nullptr;
	for (UEdGraphPin* Pin : Retarget->Pins)
	{
		PoseOut = (!PoseOut && Pin->Direction == EGPD_Output) ? Pin : PoseOut;
	}
	for (UEdGraphPin* Pin : Root->Pins)
	{
		ResultIn = (!ResultIn && Pin->Direction == EGPD_Input) ? Pin : ResultIn;
	}
	if (!PoseOut || !ResultIn || !AnimGraph->GetSchema()->TryCreateConnection(PoseOut, ResultIn))
	{
		return nullptr;
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);
	FKismetEditorUtilities::CompileBlueprint(AnimBP);
	AnimBP->MarkPackageDirty();
	return AnimBP;
#else
	return nullptr;
#endif
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

bool UNinjaEditorTools::BuildCompositeFont(UFont* Font, const TArray<FName>& FaceNames, const TArray<UFontFace*>& Faces)
{
#if WITH_EDITOR
	if (!Font || FaceNames.Num() != Faces.Num() || Faces.Num() == 0)
	{
		return false;
	}
	Font->Modify();
	Font->FontCacheType = EFontCacheType::Runtime;
	FCompositeFont& Composite = Font->GetMutableInternalCompositeFont();
	Composite.DefaultTypeface.Fonts.Reset();
	for (int32 i = 0; i < Faces.Num(); ++i)
	{
		if (!Faces[i])
		{
			return false;
		}
		FTypefaceEntry& Entry = Composite.DefaultTypeface.Fonts.AddDefaulted_GetRef();
		Entry.Name = FaceNames[i];
		Entry.Font = FFontData(Faces[i]);
	}
	Font->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

ULandscapeLayerInfoObject* UNinjaEditorTools::CreateLandscapeLayerInfo(const FString& PackagePath, FName LayerName, bool bWeightBlended)
{
#if WITH_EDITOR
	const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
	if (ULandscapeLayerInfoObject* Existing = LoadObject<ULandscapeLayerInfoObject>(nullptr, *(PackagePath + TEXT(".") + AssetName), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		return Existing;
	}
	UPackage* Package = CreatePackage(*PackagePath);
	ULandscapeLayerInfoObject* LayerInfo = NewObject<ULandscapeLayerInfoObject>(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);
	LayerInfo->SetLayerName(LayerName, false);
	LayerInfo->SetBlendMethod(bWeightBlended ? ELandscapeTargetLayerBlendMethod::FinalWeightBlending : ELandscapeTargetLayerBlendMethod::None, false);
	FAssetRegistryModule::AssetCreated(LayerInfo);
	Package->MarkPackageDirty();
	return LayerInfo;
#else
	return nullptr;
#endif
}

ALandscape* UNinjaEditorTools::CreateLandscape(UObject* WorldContextObject, FVector Location, FVector Scale, int32 ComponentsX, int32 ComponentsY,
	int32 QuadsPerSection, const TArray<int32>& Heights, UMaterialInterface* Material, const TArray<ULandscapeLayerInfoObject*>& Layers,
	const TArray<uint8>& Weights)
{
#if WITH_EDITOR
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	const int32 SizeX = ComponentsX * QuadsPerSection + 1;
	const int32 SizeY = ComponentsY * QuadsPerSection + 1;
	const int32 Count = SizeX * SizeY;
	if (!World || Heights.Num() != Count || Weights.Num() != Count * Layers.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("CreateLandscape: dados com tamanho errado (alturas %d, esperado %d; pesos %d, esperado %d)"),
			Heights.Num(), Count, Weights.Num(), Count * Layers.Num());
		return nullptr;
	}

	TArray<uint16> HeightData;
	HeightData.SetNumUninitialized(Count);
	for (int32 i = 0; i < Count; ++i)
	{
		HeightData[i] = uint16(FMath::Clamp(Heights[i], 0, 65535));
	}
	TArray<FLandscapeImportLayerInfo> LayerInfos;
	for (int32 l = 0; l < Layers.Num(); ++l)
	{
		if (!Layers[l])
		{
			return nullptr;
		}
		FLandscapeImportLayerInfo& Info = LayerInfos.Emplace_GetRef(Layers[l]->GetLayerName());
		Info.LayerInfo = Layers[l];
		Info.LayerData.Append(Weights.GetData() + l * Count, Count);
	}
	TMap<FGuid, TArray<uint16>> HeightDataPerLayers;
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> LayerDataPerLayers;
	HeightDataPerLayers.Add(FGuid(), MoveTemp(HeightData));
	LayerDataPerLayers.Add(FGuid(), LayerInfos);

	ALandscape* Landscape = World->SpawnActor<ALandscape>(Location, FRotator::ZeroRotator);
	Landscape->LandscapeMaterial = Material;
	Landscape->SetActorRelativeScale3D(Scale);
	Landscape->Import(FGuid::NewGuid(), 0, 0, SizeX - 1, SizeY - 1, 1, QuadsPerSection, HeightDataPerLayers, TEXT(""), LayerDataPerLayers,
		ELandscapeImportAlphamapType::Additive, TArrayView<const FLandscapeLayer>());

	ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
	if (LandscapeInfo)
	{
		LandscapeInfo->UpdateLayerInfoMap(Landscape);
		for (ULandscapeLayerInfoObject* LayerInfo : Layers)
		{
			Landscape->AddTargetLayer(LayerInfo->GetLayerName(), FLandscapeTargetLayerSettings(LayerInfo));
			const int32 Index = LandscapeInfo->GetLayerInfoIndex(LayerInfo->GetLayerName());
			if (Index != INDEX_NONE)
			{
				LandscapeInfo->Layers[Index].LayerInfoObj = LayerInfo;
			}
		}
	}
	Landscape->PostEditChange();
	return Landscape;
#else
	return nullptr;
#endif
}

AActor* UNinjaEditorTools::SpawnBlockingBox(UObject* WorldContextObject, FVector Location, FRotator Rotation, FVector Size)
{
#if WITH_EDITOR
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}
	ABlockingVolume* Volume = World->SpawnActor<ABlockingVolume>(Location, Rotation);
	if (!Volume)
	{
		return nullptr;
	}
	UCubeBuilder* Builder = NewObject<UCubeBuilder>();
	Builder->X = Size.X;
	Builder->Y = Size.Y;
	Builder->Z = Size.Z;
	UActorFactory::CreateBrushForVolumeActor(Volume, Builder);
	return Volume;
#else
	return nullptr;
#endif
}

bool UNinjaEditorTools::InsertSlotBeforeOutput(UAnimBlueprint* AnimBP, FName SlotName)
{
#if WITH_EDITOR
	UEdGraph* AnimGraph = nullptr;
	for (UEdGraph* Graph : AnimBP ? AnimBP->FunctionGraphs : TArray<TObjectPtr<UEdGraph>>())
	{
		if (Graph->GetFName() == UEdGraphSchema_K2::GN_AnimGraph)
		{
			AnimGraph = Graph;
		}
	}
	UAnimGraphNode_Root* Root = nullptr;
	for (UEdGraphNode* GraphNode : AnimGraph ? AnimGraph->Nodes : TArray<TObjectPtr<UEdGraphNode>>())
	{
		Root = Root ? Root : Cast<UAnimGraphNode_Root>(GraphNode);
		if (UAnimGraphNode_Slot* Existing = Cast<UAnimGraphNode_Slot>(GraphNode))
		{
			if (Existing->Node.SlotName == SlotName)
			{
				return true;
			}
		}
	}
	UEdGraphPin* ResultIn = nullptr;
	for (UEdGraphPin* Pin : Root ? Root->Pins : TArray<UEdGraphPin*>())
	{
		ResultIn = (!ResultIn && Pin->Direction == EGPD_Input) ? Pin : ResultIn;
	}
	if (!ResultIn || ResultIn->LinkedTo.Num() != 1)
	{
		return false;
	}
	UEdGraphPin* PrevOut = ResultIn->LinkedTo[0];

	UAnimGraphNode_Slot* SlotNode = NewObject<UAnimGraphNode_Slot>(AnimGraph, NAME_None, RF_Transactional);
	SlotNode->Node.SlotName = SlotName;
	AnimGraph->AddNode(SlotNode, false, false);
	SlotNode->CreateNewGuid();
	SlotNode->PostPlacedNewNode();
	SlotNode->AllocateDefaultPins();
	SlotNode->NodePosX = Root->NodePosX - 220;
	SlotNode->NodePosY = Root->NodePosY;
	UEdGraphPin* SlotIn = nullptr;
	UEdGraphPin* SlotOut = nullptr;
	for (UEdGraphPin* Pin : SlotNode->Pins)
	{
		if (Pin->Direction == EGPD_Input && !SlotIn) { SlotIn = Pin; }
		if (Pin->Direction == EGPD_Output && !SlotOut) { SlotOut = Pin; }
	}
	const UEdGraphSchema* Schema = AnimGraph->GetSchema();
	ResultIn->BreakAllPinLinks();
	if (!SlotIn || !SlotOut || !Schema->TryCreateConnection(PrevOut, SlotIn) || !Schema->TryCreateConnection(SlotOut, ResultIn))
	{
		return false;
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);
	FKismetEditorUtilities::CompileBlueprint(AnimBP);
	AnimBP->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

#if WITH_EDITOR
namespace
{
	FNiagaraEmitterHandle* FindEmitterHandle(UNiagaraSystem* System, const FString& EmitterName)
	{
		if (!System)
		{
			return nullptr;
		}
		for (FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
		{
			if (Handle.GetName().ToString() == EmitterName)
			{
				return &Handle;
			}
		}
		return nullptr;
	}

	/** Scripts que guardam constantes deste emissor: os dele e os do sistema (Emitter Spawn/Update vão para os do sistema) */
	TArray<UNiagaraScript*> ScriptsOf(UNiagaraSystem* System, FNiagaraEmitterHandle& Handle)
	{
		TArray<UNiagaraScript*> Scripts;
		if (FVersionedNiagaraEmitterData* Data = Handle.GetEmitterData())
		{
			Data->GetScripts(Scripts, false);
		}
		Scripts.Add(System->GetSystemSpawnScript());
		Scripts.Add(System->GetSystemUpdateScript());
		return Scripts;
	}

	FString ValueToString(const FNiagaraVariableBase& Var, const uint8* Data)
	{
		const FNiagaraTypeDefinition& T = Var.GetType();
		if (!Data) { return TEXT("?"); }
		if (T == FNiagaraTypeDefinition::GetFloatDef()) { return FString::SanitizeFloat(*(const float*)Data); }
		if (T == FNiagaraTypeDefinition::GetIntDef() || T.IsEnum()) { return FString::FromInt(*(const int32*)Data); }
		if (T == FNiagaraTypeDefinition::GetBoolDef()) { return (*(const int32*)Data) ? TEXT("true") : TEXT("false"); }
		const int32 N = T.GetSize() / 4;
		if (T.GetSize() % 4 == 0 && N <= 4)
		{
			TArray<FString> Parts;
			for (int32 i = 0; i < N; ++i) { Parts.Add(FString::SanitizeFloat(((const float*)Data)[i])); }
			return FString::Join(Parts, TEXT(","));
		}
		return FString::Printf(TEXT("<%d bytes>"), T.GetSize());
	}
}
#endif

UNiagaraSystem* UNinjaEditorTools::CreateNiagaraSystem(const FString& PackagePath, const FString& AssetName, const TArray<UNiagaraEmitter*>& Emitters, const TArray<FString>& EmitterNames)
{
#if WITH_EDITOR
	const FString PackageName = PackagePath / AssetName;
	UPackage* Package = CreatePackage(*PackageName);
	UNiagaraSystem* System = NewObject<UNiagaraSystem>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	UNiagaraSystemFactoryNew::InitializeSystem(System, true);
	for (int32 i = 0; i < Emitters.Num(); ++i)
	{
		if (!Emitters[i])
		{
			continue;
		}
		const FGuid Id = FNiagaraEditorUtilities::AddEmitterToSystem(*System, *Emitters[i], Emitters[i]->GetExposedVersion().VersionGuid, true);
		for (FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
		{
			if (Handle.GetId() == Id && EmitterNames.IsValidIndex(i))
			{
				Handle.SetName(*EmitterNames[i], *System);
			}
		}
	}
	FAssetRegistryModule::AssetCreated(System);
	System->MarkPackageDirty();
	System->RequestCompile(false);
	System->WaitForCompilationComplete();
	return System;
#else
	return nullptr;
#endif
}

FString UNinjaEditorTools::DescribeNiagaraSystem(UNiagaraSystem* System, bool bRendererProperties)
{
	FString Out;
#if WITH_EDITOR
	if (!System)
	{
		return Out;
	}
	for (FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		Out += FString::Printf(TEXT("== Emissor %s\n"), *Handle.GetName().ToString());
		TSet<FString> Seen;
		for (UNiagaraScript* Script : ScriptsOf(System, Handle))
		{
			if (!Script) { continue; }
			for (const FNiagaraVariableWithOffset& Var : Script->RapidIterationParameters.ReadParameterVariables())
			{
				const FString Name = Var.GetName().ToString();
				if (Seen.Contains(Name)) { continue; }
				Seen.Add(Name);
				Out += FString::Printf(TEXT("  %s (%s) = %s\n"), *Name, *Var.GetType().GetName(),
					*ValueToString(Var, Script->RapidIterationParameters.GetParameterData(Var)));
			}
		}
		if (FVersionedNiagaraEmitterData* Data = Handle.GetEmitterData())
		{
			int32 Index = 0;
			for (UNiagaraRendererProperties* Renderer : Data->GetRenderers())
			{
				Out += FString::Printf(TEXT("  Renderer %d: %s\n"), Index++, *GetNameSafe(Renderer ? Renderer->GetClass() : nullptr));
				if (Renderer && bRendererProperties)
				{
					for (TFieldIterator<FProperty> It(Renderer->GetClass()); It; ++It)
					{
						if (It->HasAnyPropertyFlags(CPF_Edit))
						{
							FString Value;
							It->ExportTextItem_Direct(Value, It->ContainerPtrToValuePtr<void>(Renderer), nullptr, Renderer, PPF_None);
							Out += FString::Printf(TEXT("     %s = %s\n"), *It->GetName(), *Value.Left(120));
						}
					}
				}
			}
		}
	}
#endif
	return Out;
}

int32 UNinjaEditorTools::SetNiagaraConstant(UNiagaraSystem* System, const FString& EmitterName, const FString& ParamName, const FString& Value)
{
	int32 Count = 0;
#if WITH_EDITOR
	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return -1;
	}
	TArray<FString> Parts;
	Value.ParseIntoArray(Parts, TEXT(","));
	for (UNiagaraScript* Script : ScriptsOf(System, *Handle))
	{
		if (!Script) { continue; }
		FNiagaraParameterStore& Store = Script->RapidIterationParameters;
		TArray<FNiagaraVariableWithOffset> Vars(Store.ReadParameterVariables());
		for (const FNiagaraVariableWithOffset& Var : Vars)
		{
			const FString Name = Var.GetName().ToString();
			const bool bMatch = Name == ParamName
				|| (Name.EndsWith(TEXT(".") + ParamName) && Name.StartsWith(TEXT("Constants.") + EmitterName + TEXT(".")));
			if (!bMatch) { continue; }
			const FNiagaraTypeDefinition& T = Var.GetType();
			TArray<uint8> Bytes;
			Bytes.SetNumZeroed(T.GetSize());
			if (T == FNiagaraTypeDefinition::GetBoolDef())
			{
				*(int32*)Bytes.GetData() = (Value.ToBool() || Value == TEXT("1")) ? FNiagaraBool::True : FNiagaraBool::False;
			}
			else if (T == FNiagaraTypeDefinition::GetIntDef() || T.IsEnum())
			{
				*(int32*)Bytes.GetData() = FCString::Atoi(*Value);
			}
			else if (T.GetSize() % 4 == 0 && T.GetSize() / 4 <= 4)
			{
				for (int32 i = 0; i < T.GetSize() / 4; ++i)
				{
					((float*)Bytes.GetData())[i] = Parts.IsValidIndex(i) ? FCString::Atof(*Parts[i]) : (Parts.Num() == 1 ? FCString::Atof(*Parts[0]) : 0.f);
				}
			}
			else
			{
				continue;
			}
			Script->Modify();
			Store.SetParameterData(Bytes.GetData(), FNiagaraVariable(Var), false);
			Count++;
		}
	}
	System->MarkPackageDirty();
#endif
	return Count;
}

bool UNinjaEditorTools::SetNiagaraRendererProperty(UNiagaraSystem* System, const FString& EmitterName, int32 RendererIndex, const FString& Property, const FString& Value)
{
#if WITH_EDITOR
	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	FVersionedNiagaraEmitterData* Data = Handle ? Handle->GetEmitterData() : nullptr;
	if (!Data || !Data->GetRenderers().IsValidIndex(RendererIndex))
	{
		return false;
	}
	UNiagaraRendererProperties* Renderer = Data->GetRenderers()[RendererIndex];
	FProperty* Prop = Renderer ? Renderer->GetClass()->FindPropertyByName(*Property) : nullptr;
	if (!Prop)
	{
		return false;
	}
	Renderer->Modify();
	Renderer->PreEditChange(Prop);
	const TCHAR* Result = Prop->ImportText_Direct(*Value, Prop->ContainerPtrToValuePtr<void>(Renderer), Renderer, PPF_None);
	FPropertyChangedEvent Event(Prop);
	Renderer->PostEditChangeProperty(Event);
	System->MarkPackageDirty();
	return Result != nullptr;
#else
	return false;
#endif
}

bool UNinjaEditorTools::SetNiagaraEmitterProperty(UNiagaraSystem* System, const FString& EmitterName, const FString& Property, const FString& Value)
{
#if WITH_EDITOR
	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	FVersionedNiagaraEmitterData* Data = Handle ? Handle->GetEmitterData() : nullptr;
	FProperty* Prop = Data ? FVersionedNiagaraEmitterData::StaticStruct()->FindPropertyByName(*Property) : nullptr;
	if (!Prop)
	{
		return false;
	}
	const TCHAR* Result = Prop->ImportText_Direct(*Value, Prop->ContainerPtrToValuePtr<void>(Data), nullptr, PPF_None);
	if (UNiagaraEmitter* Emitter = Handle->GetInstance().Emitter)
	{
		Emitter->Modify();
		FPropertyChangedEvent Event(Prop);
		Emitter->PostEditChangeProperty(Event);
	}
	System->MarkPackageDirty();
	return Result != nullptr;
#else
	return false;
#endif
}

bool UNinjaEditorTools::CompileNiagaraSystem(UNiagaraSystem* System, bool bFixedBounds, FVector BoundsExtent)
{
#if WITH_EDITOR
	if (!System)
	{
		return false;
	}
	System->bFixedBounds = bFixedBounds;
	if (bFixedBounds)
	{
		System->SetFixedBounds(FBox(-BoundsExtent, BoundsExtent));
	}
	System->RequestCompile(false);
	System->WaitForCompilationComplete();
	System->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

FString UNinjaEditorTools::DescribeNiagaraModulePins(UNiagaraSystem* System, const FString& EmitterName)
{
	FString Out;
#if WITH_EDITOR
	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	FVersionedNiagaraEmitterData* Data = Handle ? Handle->GetEmitterData() : nullptr;
	UNiagaraScriptSource* Source = Data ? Cast<UNiagaraScriptSource>(Data->GraphSource) : nullptr;
	if (!Source || !Source->NodeGraph) { return Out; }
	for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
	{
		UNiagaraNodeFunctionCall* Function = Cast<UNiagaraNodeFunctionCall>(Node);
		if (!Function) { continue; }
		for (UEdGraphPin* Pin : Function->Pins)
		{
			if (Pin->Direction != EGPD_Input) { continue; }
			Out += Function->GetFunctionName() + TEXT(" | ") + Pin->PinName.ToString() + TEXT(" = ") + Pin->DefaultValue;
			if (const UEnum* Enum = Cast<UEnum>(Pin->PinType.PinSubCategoryObject.Get()))
			{
				for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
				{
					Out += FString::Printf(TEXT(" [%lld:%s]"), Enum->GetValueByIndex(Index), *Enum->GetNameStringByIndex(Index));
				}
			}
			Out += TEXT("\n");
		}
	}
#endif
	return Out;
}

bool UNinjaEditorTools::SetNiagaraModulePin(UNiagaraSystem* System, const FString& EmitterName,
	const FString& ModuleName, const FString& PinName, const FString& Value)
{
#if WITH_EDITOR
	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	FVersionedNiagaraEmitterData* Data = Handle ? Handle->GetEmitterData() : nullptr;
	UNiagaraScriptSource* Source = Data ? Cast<UNiagaraScriptSource>(Data->GraphSource) : nullptr;
	if (!Source || !Source->NodeGraph) { return false; }
	for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
	{
		UNiagaraNodeFunctionCall* Function = Cast<UNiagaraNodeFunctionCall>(Node);
		if (!Function || Function->GetFunctionName() != ModuleName) { continue; }
		for (UEdGraphPin* Pin : Function->Pins)
		{
			if (Pin->Direction == EGPD_Input && Pin->PinName.ToString() == PinName)
			{
				Function->Modify();
				Pin->DefaultValue = Value;
				Function->PinDefaultValueChanged(Pin);
				Source->NodeGraph->NotifyGraphChanged();
				System->MarkPackageDirty();
				return true;
			}
		}
	}
#endif
	return false;
}

bool UNinjaEditorTools::AddNiagaraUpdateModule(UNiagaraSystem* System, const FString& EmitterName, const FString& ScriptPath, int32 Index)
{
#if WITH_EDITOR
	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	FVersionedNiagaraEmitterData* Data = Handle ? Handle->GetEmitterData() : nullptr;
	UNiagaraScriptSource* Source = Data ? Cast<UNiagaraScriptSource>(Data->GraphSource) : nullptr;
	UNiagaraScript* Module = LoadObject<UNiagaraScript>(nullptr, *ScriptPath);
	if (!Source || !Source->NodeGraph || !Module) { return false; }
	for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
	{
		if (UNiagaraNodeFunctionCall* Function = Cast<UNiagaraNodeFunctionCall>(Node))
		{
			if (Function->FunctionScript == Module) { return true; }
		}
	}
	for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
	{
		UNiagaraNodeOutput* Output = Cast<UNiagaraNodeOutput>(Node);
		if (Output && Output->GetUsage() == ENiagaraScriptUsage::ParticleUpdateScript)
		{
			const bool bAdded = FNiagaraStackGraphUtilities::AddScriptModuleToStack(Module, *Output, Index) != nullptr;
			Source->NodeGraph->NotifyGraphChanged();
			System->MarkPackageDirty();
			return bAdded;
		}
	}
#endif
	return false;
}

bool UNinjaEditorTools::AddNiagaraModule(UNiagaraSystem* System, const FString& EmitterName, const FString& ScriptPath, const FString& Stage, int32 Index)
{
#if WITH_EDITOR
	ENiagaraScriptUsage Usage = ENiagaraScriptUsage::ParticleUpdateScript;
	if (Stage == TEXT("EmitterSpawn")) { Usage = ENiagaraScriptUsage::EmitterSpawnScript; }
	else if (Stage == TEXT("EmitterUpdate")) { Usage = ENiagaraScriptUsage::EmitterUpdateScript; }
	else if (Stage == TEXT("ParticleSpawn")) { Usage = ENiagaraScriptUsage::ParticleSpawnScript; }
	else if (Stage != TEXT("ParticleUpdate")) { return false; }
	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	FVersionedNiagaraEmitterData* Data = Handle ? Handle->GetEmitterData() : nullptr;
	UNiagaraScriptSource* Source = Data ? Cast<UNiagaraScriptSource>(Data->GraphSource) : nullptr;
	UNiagaraScript* Module = LoadObject<UNiagaraScript>(nullptr, *ScriptPath);
	if (!Source || !Source->NodeGraph || !Module) { return false; }
	for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
	{
		UNiagaraNodeFunctionCall* Function = Cast<UNiagaraNodeFunctionCall>(Node);
		if (Function && Function->FunctionScript == Module && FNiagaraStackGraphUtilities::GetOutputNodeUsage(*Function) == Usage)
		{
			return true;
		}
	}
	for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
	{
		UNiagaraNodeOutput* Output = Cast<UNiagaraNodeOutput>(Node);
		if (!Output || Output->GetUsage() != Usage) { continue; }
		const bool bAdded = FNiagaraStackGraphUtilities::AddScriptModuleToStack(Module, *Output, Index) != nullptr;
		Source->NodeGraph->NotifyGraphChanged();
		System->MarkPackageDirty();
		return bAdded;
	}
#endif
	return false;
}

UTexture2D* UNinjaEditorTools::CreateFireColorAtlas(const FString& PackagePath)
{
#if WITH_EDITOR
	const FString CurvePath = PackagePath + TEXT("/C_Katon_Temperatura");
	UCurveLinearColor* Curve = LoadObject<UCurveLinearColor>(nullptr, *(CurvePath + TEXT(".C_Katon_Temperatura")));
	if (!Curve) { Curve = NewObject<UCurveLinearColor>(CreatePackage(*CurvePath), TEXT("C_Katon_Temperatura"), RF_Public | RF_Standalone); FAssetRegistryModule::AssetCreated(Curve); }
	const float Times[] = {0.f, .14f, .32f, .57f, .81f, 1.f};
	const FLinearColor Colors[] = {{.022f,.012f,.009f,1.f},{.12f,.008f,.002f,1.f},{.8f,.025f,.002f,1.f},{2.8f,.23f,.008f,1.f},{4.5f,1.15f,.06f,1.f},{6.f,3.5f,.65f,1.f}};
	for (int32 Channel = 0; Channel < 4; ++Channel)
	{
		Curve->FloatCurves[Channel].Reset();
		for (int32 Key = 0; Key < UE_ARRAY_COUNT(Times); ++Key)
		{
			const FKeyHandle Handle = Curve->FloatCurves[Channel].AddKey(Times[Key], Colors[Key].Component(Channel));
			Curve->FloatCurves[Channel].SetKeyInterpMode(Handle, RCIM_Linear);
		}
	}
	Curve->MarkPackageDirty();
	const FString AtlasPath = PackagePath + TEXT("/CA_Katon_Temperatura");
	UCurveLinearColorAtlas* Atlas = LoadObject<UCurveLinearColorAtlas>(nullptr, *(AtlasPath + TEXT(".CA_Katon_Temperatura")));
	if (!Atlas) { Atlas = NewObject<UCurveLinearColorAtlas>(CreatePackage(*AtlasPath), TEXT("CA_Katon_Temperatura"), RF_Public | RF_Standalone); FAssetRegistryModule::AssetCreated(Atlas); }
	Atlas->TextureSize = 256;
	Atlas->GradientCurves = {Curve};
	Atlas->bDisableAllAdjustments = true;
	Atlas->UpdateTextures();
	Atlas->PostEditChange();
	Atlas->MarkPackageDirty();
	return Atlas;
#else
	return nullptr;
#endif
}

bool UNinjaEditorTools::LinkNiagaraFloatInput(UNiagaraSystem* System, const FString& EmitterName, const FString& ModuleName, const FString& InputName, const FString& ParameterName)
{
#if WITH_EDITOR
	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	FVersionedNiagaraEmitterData* Data = Handle ? Handle->GetEmitterData() : nullptr;
	UNiagaraScriptSource* Source = Data ? Cast<UNiagaraScriptSource>(Data->GraphSource) : nullptr;
	if (!Source || !Source->NodeGraph) { return false; }
	const FNiagaraVariable Variable(FNiagaraTypeDefinition::GetFloatDef(), FName(ParameterName));
	if (ParameterName.StartsWith(TEXT("User."))) { System->GetExposedParameters().AddParameter(Variable); System->GetExposedParameters().SetParameterValue<float>(1.f, Variable); }
	for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
	{
		UNiagaraNodeFunctionCall* Function = Cast<UNiagaraNodeFunctionCall>(Node);
		if (!Function || Function->GetFunctionName() != ModuleName) { continue; }
		UEdGraphPin& Pin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*Function, FNiagaraParameterHandle(FName(ModuleName + TEXT(".") + InputName)), FNiagaraTypeDefinition::GetFloatDef(), FGuid(), FGuid());
		FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(Pin, Variable, {});
		Source->NodeGraph->NotifyGraphChanged(); System->MarkPackageDirty(); return true;
	}
#endif
	return false;
}

FString UNinjaEditorTools::PrepareFluidBaker(UNiagaraSystem* System, float StartSeconds, float DurationSeconds)
{
	FString Inventory;
#if WITH_EDITOR
	if (!System) { return TEXT("ERROR: no system"); }
	TMap<FString, FString> Bindings;
	for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		FVersionedNiagaraEmitterData* Data = Handle.GetInstance().GetEmitterData();
		if (!Handle.GetIsEnabled() || !Data) { continue; }
		Data->ForEachScript([&](UNiagaraScript* Script)
		{
			const FNiagaraScriptRuntimeData* Runtime = System->GetScriptRuntimeData(&Handle, *Script).Get();
			const FNiagaraScriptExecutionParameterStore* Store = Runtime ? Runtime->GetExecutionReadyParameterStore() : nullptr;
			if (!Store) { return; }
			for (const FNiagaraVariableWithOffset& Variable : Store->ReadParameterVariables())
			{
				if (!Variable.IsDataInterface()) { continue; }
				UNiagaraDataInterface* DI = Store->GetDataInterface(Variable.Offset);
				if (!DI || !DI->CanRenderVariablesToCanvas()) { continue; }
				TArray<FNiagaraVariableBase> Attributes;
				DI->GetCanvasVariables(Attributes);
				for (const FNiagaraVariableBase& Attribute : Attributes)
				{
					const FString Name = Attribute.GetName().ToString();
					const FString Binding = TEXT("EmitterDI.") + Variable.GetName().ToString() + TEXT(".") + Name;
					Inventory += Binding + TEXT("\n");
					if (Name == TEXT("Density") || Name == TEXT("Temperature") || Name == TEXT("Velocity"))
					{
						FString* Existing = Bindings.Find(Name);
						if (!Existing || Binding.Len() < Existing->Len()) { Bindings.Add(Name, Binding); }
					}
				}
			}
		});
	}
	if (Bindings.Num() != 3) { return TEXT("ERROR: missing channels\n") + Inventory; }
	UNiagaraBakerSettings* Settings = System->GetBakerSettings();
	Settings->StartSeconds = StartSeconds;
	Settings->DurationSeconds = DurationSeconds;
	Settings->FramesPerSecond = 60;
	Settings->FramesPerDimension = FIntPoint(8, 8);
	Settings->bLockToSimulationFrameRate = true;
	Settings->Outputs.Reset();
	for (const TCHAR* Name : {TEXT("Density"), TEXT("Temperature"), TEXT("Velocity")})
	{
		UNiagaraBakerOutputTexture2D* Output = NewObject<UNiagaraBakerOutputTexture2D>(Settings);
		Output->OutputName = Name;
		Output->SourceBinding.SourceName = FName(Bindings[Name]);
		Output->SourceBinding.DisplayString = Bindings[Name];
		Output->FrameSize = FIntPoint(256, 256);
		Output->AtlasTextureSize = FIntPoint(2048, 2048);
		Output->bGenerateAtlas = true;
		Output->bExportFrames = true;
		Output->FramesExportPathFormat = TEXT("{SavedDir}/NiagaraBaker/{AssetName}/") + FString(Name) + TEXT("_{FrameIndex}.exr");
		Output->AtlasAssetPathFormat = TEXT("{AssetFolder}/{AssetName}_{OutputName}_8x8");
		Settings->Outputs.Add(Output);
	}
	System->MarkPackageDirty();
	return TEXT("READY\n") + Inventory;
#else
	return TEXT("ERROR: editor only");
#endif
}


bool UNinjaEditorTools::SetSteppedColorCurve(UCurveLinearColor* Curve, const TArray<FLinearColor>& Colors)
{
	if (!Curve || Colors.Num() == 0)
	{
		return false;
	}
	Curve->Modify();
	for (int32 Channel = 0; Channel < 4; ++Channel)
	{
		FRichCurve& Rich = Curve->FloatCurves[Channel];
		Rich.Reset();
		for (int32 i = 0; i < Colors.Num(); ++i)
		{
			const FLinearColor& C = Colors[i];
			const float Value = Channel == 0 ? C.R : Channel == 1 ? C.G : Channel == 2 ? C.B : C.A;
			const FKeyHandle Key = Rich.AddKey(float(i) / Colors.Num(), Value);
			Rich.SetKeyInterpMode(Key, RCIM_Constant);
		}
	}
	Curve->OnCurveChanged(TArray<FRichCurveEditInfo>());
	Curve->MarkPackageDirty();
	return true;
}

AActor* UNinjaEditorTools::SpawnLocalActor(UObject* WorldContext, TSubclassOf<AActor> ActorClass, FVector Location, FRotator Rotation)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || !ActorClass)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AActor>(ActorClass, Location, Rotation, Params);
}

bool UNinjaEditorTools::UpdateSkeletonRefPoseFromMesh(USkeletalMesh* Mesh)
{
#if WITH_EDITOR
	USkeleton* Skeleton = Mesh ? Mesh->GetSkeleton() : nullptr;
	if (!Skeleton)
	{
		return false;
	}
	Skeleton->Modify();
	Skeleton->UpdateReferencePoseFromMesh(Mesh);
	Skeleton->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UNinjaEditorTools::SetOutfitCatalogRow(UDataTable* Catalog, FName RowName, const FText& DisplayName, USkeletalMesh* Mesh, bool bOculto)
{
#if WITH_EDITOR
	if (!Catalog || !Mesh || Catalog->GetRowStruct() != FNinjaCustomizationPart::StaticStruct()) { return false; }
	FNinjaCustomizationPart Row;
	Row.DisplayName = DisplayName;
	Row.Slot = ENinjaCustomizationSlot::Outfit;
	Row.Mesh = Mesh;
	Row.bOculto = bOculto;
	Catalog->AddRow(RowName, Row);
	Catalog->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

AActor* UNinjaEditorTools::SpawnActorInPlayWorld(AActor* AnyActorInWorld, TSubclassOf<AActor> ActorClass)
{
	UWorld* World = AnyActorInWorld ? AnyActorInWorld->GetWorld() : nullptr;
	return World && *ActorClass ? World->SpawnActor<AActor>(ActorClass, AnyActorInWorld->GetActorTransform()) : nullptr;
}


bool UNinjaEditorTools::SetMeshSocket(USkeletalMesh* Mesh, FName SocketName, FName BoneName, FTransform LocalTransform)
{
#if WITH_EDITOR
    if (!Mesh || SocketName.IsNone() || Mesh->GetRefSkeleton().FindBoneIndex(BoneName) == INDEX_NONE)
    {
        return false;
    }
    Mesh->Modify();
    USkeletalMeshSocket* Socket = Mesh->FindSocket(SocketName);
    if (!Socket)
    {
        Socket = NewObject<USkeletalMeshSocket>(Mesh);
        Socket->SocketName = SocketName;
        Mesh->AddSocket(Socket, false);
    }
    Socket->BoneName = BoneName;
    Socket->SetSocketLocalTransform(LocalTransform);
    Mesh->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}


bool UNinjaEditorTools::BindRobeCloth(USkeletalMesh* Mesh, FName SectionMaterial)
{
#if WITH_EDITOR
	if (!Mesh || !Mesh->GetImportedModel()
		|| !Mesh->GetImportedModel()->LODModels.IsValidIndex(0))
	{
		return false;
	}
	FSkeletalMeshLODModel& Lod = Mesh->GetImportedModel()->LODModels[0];
	const TArray<FSkeletalMaterial>& Materials = Mesh->GetMaterials();
	const int32 Section = Lod.Sections.IndexOfByPredicate([&](const FSkelMeshSection& Candidate)
	{
		const int32 Slot = Candidate.MaterialIndex;
		return Materials.IsValidIndex(Slot)
			&& (Materials[Slot].ImportedMaterialSlotName == SectionMaterial
			|| Materials[Slot].MaterialSlotName == SectionMaterial);
	});
	if (Section == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("Missing cloth material section %s"), *SectionMaterial.ToString());
		return false;
	}

	const bool bNinjaShirt = SectionMaterial == TEXT("Ninja_Fitted_Shirt");
	const bool bShadowSleeves = SectionMaterial == TEXT("Shadow_Shinobi_Sleeves");
	const bool bUpperCloth = bNinjaShirt || bShadowSleeves;
	const bool bShortTunic = SectionMaterial == TEXT("Shadow_Shinobi_Hem");
	const bool bLongRobe = SectionMaterial == TEXT("Izuna_Skirt");
	if (!bUpperCloth && !bShortTunic && !bLongRobe) { return false; }

	const TCHAR* AssetName = bUpperCloth ? TEXT("UchihaUpperCloth")
		: bShortTunic ? TEXT("ShadowShinobiTunic") : TEXT("IzunaPainelInferior");
	const TCHAR* CollisionName = bUpperCloth ? TEXT("UchihaArmClothCollisions")
		: bShortTunic ? TEXT("ShadowShinobiCollisions") : TEXT("IzunaClothCollisions");
	const FReferenceSkeleton& Skeleton = Mesh->GetRefSkeleton();
	// Cloth uses its own capsules without changing the character's physics asset.
	UClothingAssetCommon* Cloth = Cast<UClothingAssetCommon>(Mesh->GetClothingAsset(Lod.Sections[Section].ClothingData.AssetGuid));
	const bool bReconfigure = Cloth != nullptr;
	const bool bLightFabric = bShortTunic || bShadowSleeves;
	UPhysicsAsset* Collision = bReconfigure ? Cloth->PhysicsAsset.Get() : nullptr;
	if (!Collision)
	{
		Collision = NewObject<UPhysicsAsset>(Mesh, CollisionName, RF_Transactional);
	}
	Collision->Modify();
	Collision->SkeletalBodySetups.Reset();
	auto AddCapsule = [&](const FName StartName, const FName EndName, const float Radius, const float EndPadding, const float HipRadius = 0.f, const float HipInset = 0.f)
	{
		const int32 StartIndex = Skeleton.FindBoneIndex(StartName);
		const int32 EndIndex = Skeleton.FindBoneIndex(EndName);
		if (StartIndex == INDEX_NONE || EndIndex == INDEX_NONE) { return false; }

		const FTransform Start = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, StartIndex);
		const FVector End = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, EndIndex).GetLocation();
		const FVector LocalEnd = Start.InverseTransformPosition(End);
		const float SegmentLength = FMath::Max(1.f, static_cast<float>(LocalEnd.Size()) - 2.f * Radius + EndPadding);
		FKSphylElem Capsule(Radius, SegmentLength);
		Capsule.Center = LocalEnd * .5;
		Capsule.Rotation = FQuat::FindBetweenNormals(FVector::ZAxisVector, LocalEnd.GetSafeNormal()).Rotator();
		USkeletalBodySetup* Body = NewObject<USkeletalBodySetup>(Collision);
		Body->BoneName = StartName;
		if (HipRadius > 0.f)
		{
			// Inset only the proximal end, keeping the knee-side collision in place.
			const float KneeRadius = Radius;
			FKTaperedCapsuleElem Tapered(KneeRadius, HipRadius, FMath::Max(1.f, SegmentLength - HipInset));
			Tapered.Center = Capsule.Center + LocalEnd.GetSafeNormal() * (HipInset * .5f);
			Tapered.Rotation = Capsule.Rotation;
			Body->AggGeom.TaperedCapsuleElems.Add(Tapered);
		}
		else
		{
			Body->AggGeom.SphylElems.Add(Capsule);
		}
		Collision->SkeletalBodySetups.Add(Body);
		return true;
	};
	for (const TCHAR* Side : {TEXT("_l"), TEXT("_r")})
	{
		if (bUpperCloth)
		{
			if (!AddCapsule(FName(*(FString(TEXT("upperarm")) + Side)), FName(*(FString(TEXT("lowerarm")) + Side)), 3.35f, 1.f))
			{
				return false;
			}
		}
		else if (!AddCapsule(FName(*(FString(TEXT("thigh")) + Side)), FName(*(FString(TEXT("calf")) + Side)),
			bLongRobe ? 6.2f : 8.45f, bLongRobe ? 2.f : 5.f,
			bLongRobe ? 7.25f : 7.85f, bLongRobe ? 1.25f : 1.65f)
			|| !AddCapsule(FName(*(FString(TEXT("calf")) + Side)), FName(*(FString(TEXT("foot")) + Side)),
				bLongRobe ? 5.15f : 6.55f, bLongRobe ? -3.f : 5.f))
		{
			return false;
		}
	}
	Collision->UpdateBodySetupIndexMap();
	Collision->UpdateBoundsBodiesArray();

	FSkeletalMeshClothBuildParams Params;
	Params.AssetName = AssetName;
	Params.LodIndex = 0;
	Params.SourceSection = Section;
	Params.bRemoveFromMesh = false;
	if (!bReconfigure)
	{
		UClothingAssetFactory* Factory = NewObject<UClothingAssetFactory>();
		Cloth = Cast<UClothingAssetCommon>(Factory->CreateFromSkeletalMesh(Mesh, Params));
	}
	if (!Cloth || Cloth->LodData.Num() != 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not extract the cloth section"));
		return false;
	}
	if (!bReconfigure)
	{
		Mesh->AddClothingAsset(Cloth); // Initializes the simulation configs.
	}
	UChaosClothConfig* Config = Cloth->GetClothConfig<UChaosClothConfig>();
	if (!Config)
	{
		UE_LOG(LogTemp, Error, TEXT("Chaos cloth config is unavailable"));
		return false;
	}
	Cloth->PhysicsAsset = Collision;
	Config->CollisionThickness = bUpperCloth ? .35f : bShortTunic ? .65f : .35f;
	if (bShortTunic)
	{
		// A sliding lining avoids bunching as the thigh moves under the panel.
		Config->FrictionCoefficient = .24f;
	}
	else if (bLongRobe)
	{
		Config->FrictionCoefficient = .12f;
		Config->GravityScale = .62f;
		Config->LinearVelocityScale = FVector(.72);
		Config->AngularVelocityScale = .76f;
	}
	Config->bUseCCD = true;
	Config->DampingCoefficient = bLightFabric ? .04f : .055f;
	Config->LocalDampingCoefficient = bLightFabric ? .12f : bUpperCloth ? .18f : .24f;
	const float EdgeStiffness = bLightFabric ? .77f : .86f;
	Config->EdgeStiffnessWeighted = { EdgeStiffness, EdgeStiffness };
	Config->bUseBendingElements = true;
	const float Bending = bLightFabric ? (bShortTunic ? .43f : .34f) : bUpperCloth ? .43f : .62f;
	Config->BendingStiffnessWeighted = { Bending, Bending };
	Config->BucklingRatio = .45f;
	const float Buckling = bShortTunic ? .39f : bUpperCloth ? .34f : .52f;
	Config->BucklingStiffnessWeighted = { Buckling, Buckling };
	const float AnimDrive = bLightFabric ? (bShortTunic ? .18f : .39f) : bUpperCloth ? .65f : .66f;
	Config->AnimDriveStiffness = { AnimDrive, AnimDrive };
	const float DriveDamping = bLightFabric ? .52f : .75f;
	Config->AnimDriveDamping = { DriveDamping, DriveDamping };

	FClothLODDataCommon& ClothLod = Cloth->LodData[0];
	const FClothPhysicalMeshData& PhysicalMesh = ClothLod.PhysicalMeshData;
	TSet<int32> AnchoredBoundary;
	if (bUpperCloth)
	{
		TSet<uint64> OpenEdges;
		for (int32 Index = 0; Index < PhysicalMesh.Indices.Num(); Index += 3)
		{
			for (int32 Edge = 0; Edge < 3; ++Edge)
			{
				const uint32 A = PhysicalMesh.Indices[Index + Edge];
				const uint32 B = PhysicalMesh.Indices[Index + (Edge + 1) % 3];
				const uint64 Key = (uint64(FMath::Min(A, B)) << 32) | FMath::Max(A, B);
				if (!OpenEdges.Remove(Key)) { OpenEdges.Add(Key); }
			}
		}
		for (const uint64 Key : OpenEdges)
		{
			AnchoredBoundary.Add(static_cast<int32>(Key >> 32));
			AnchoredBoundary.Add(static_cast<int32>(Key & 0xffffffff));
		}
	}
	FPointWeightMap& Travel = ClothLod.PointWeightMaps[0];
	Travel.Name = bUpperCloth ? TEXT("Ombros fixos e tecido solto") : TEXT("Cintura fixa e barra solta");
	int32 Pinned = 0;
	for (int32 Vertex = 0; Vertex < ClothLod.PhysicalMeshData.Vertices.Num(); ++Vertex)
	{
		const FVector3f& Position = ClothLod.PhysicalMeshData.Vertices[Vertex];
		const float Height = Position.Z;
		// The long panel follows the stride through skinning. Use Chaos only
		// for a small secondary swing below the hips.
		const float HemSwing = FMath::SmoothStep(0.f, 1.f, (87.5f - Height) / 41.f);
		float Distance = bShortTunic
			? 8.1f * FMath::SmoothStep(0.f, 1.f, (103.6f - Height) / 19.8f)
			: 2.45f * HemSwing;
		if (bShortTunic)
		{
			// Stabilize the high sides below the belt. Leave the front, back
			// and lower edge free to respond to the stride.
			const float Side = FMath::SmoothStep(11.f, 19.f, FMath::Abs(Position.X));
			Distance *= 1.f - .78f * Side;
		}
		if (bUpperCloth)
		{
			const float Lateral = FMath::Abs(Position.X);
			// The Sasuke shoulder follows the approved skinning. Physics blends
			// in below the shoulder, on the loose part of the sleeve.
			const float Sleeve = bNinjaShirt ? .68f * FMath::SmoothStep(27.7f, 34.5f, Lateral)
				: FMath::SmoothStep(23.f, 33.f, Lateral);
			const float UpperArmHeight = FMath::SmoothStep(132.f, 138.f, Height);
			const float Cuff = bShadowSleeves ? 1.f - FMath::SmoothStep(34.f, 36.f, Lateral) : 1.f;
			const float ShirtHem = bNinjaShirt ? .85f * (1.f - FMath::SmoothStep(101.f, 112.f, Height)) : 0.f;
			const float SleeveTravel = bShadowSleeves ? 1.95f : 1.25f;
			Distance = FMath::Max(SleeveTravel * Sleeve * UpperArmHeight * Cuff, ShirtHem);
			const float SeamFalloff = bShadowSleeves ? 1.55f : 2.5f;
			float BoundaryDistanceSq = FMath::Square(SeamFalloff);
			if (Distance > 0.f)
			{
				for (const int32 Anchor : AnchoredBoundary)
				{
					BoundaryDistanceSq = FMath::Min(BoundaryDistanceSq,
						(PhysicalMesh.Vertices[Anchor] - Position).SizeSquared());
				}
			}
			Distance *= FMath::SmoothStep(0.f, SeamFalloff, FMath::Sqrt(BoundaryDistanceSq));
		}
		Travel.SetValue(Vertex, Distance);
		Pinned += Distance <= UE_SMALL_NUMBER ? 1 : 0;
	}
	if (Pinned == 0 || Pinned == Travel.Num()) { return false; }
	const int32 ParticleCount = Travel.Num();
	if (bUpperCloth && !bReconfigure)
	{
		// Keep the fabric outside its skinned surface while allowing outward and
		// tangential motion. This protects nearby layered clothing and seams.
		for (const EWeightMapTargetCommon Target : { EWeightMapTargetCommon::BackstopRadius, EWeightMapTargetCommon::BackstopDistance })
		{
			const bool bRadius = Target == EWeightMapTargetCommon::BackstopRadius;
			FPointWeightMap& Mask = ClothLod.PointWeightMaps.AddDefaulted_GetRef();
			Mask.Initialize(FPointWeightMap(ParticleCount, bRadius ? 2.5f : .05f), Target);
			Mask.Name = bRadius ? TEXT("Raio de contenção") : TEXT("Folga para dentro");
		}
	}
	Cloth->ApplyParameterMasks();
	Cloth->RefreshBoneMapping(Mesh);
	Cloth->InvalidateAllCachedData();
	if (!bReconfigure)
	{
		FScopedSkeletalMeshPostEditChange ScopedPostEditChange(Mesh);
		FSkelMeshSourceSectionUserData& SourceSection = Lod.UserSectionsData.FindOrAdd(
			Lod.Sections[Section].OriginalDataSectionIndex);
		if (!Cloth->BindToSkeletalMesh(Mesh, 0, Section, 0)) { return false; }
		SourceSection.CorrespondClothAssetIndex = Mesh->GetMeshClothingAssets().Find(Cloth);
		SourceSection.ClothingData.AssetGuid = Cloth->GetAssetGuid();
		SourceSection.ClothingData.AssetLodIndex = 0;
	}
	TArray<UClothingAssetBase*> AssetsInUse;
	Mesh->GetClothingAssetsInUse(AssetsInUse);
	if (!AssetsInUse.Contains(Cloth))
	{
		UE_LOG(LogTemp, Error, TEXT("Cloth assignment was not copied to render data"));
		return false;
	}
	// Classify the sleeve curves on this mesh so they can drive morphs
	// without writing curve metadata into the shared character skeleton.
	for (const FName Curve : {FName(TEXT("Corr_SleeveRaise_l")), FName(TEXT("Corr_SleeveRaise_r"))})
	{
		if (!Mesh->FindMorphTarget(Curve)) { continue; }
		UAnimCurveMetaData* MetaData = Mesh->GetAssetUserData<UAnimCurveMetaData>();
		if (!MetaData)
		{
			MetaData = NewObject<UAnimCurveMetaData>(Mesh);
			Mesh->AddAssetUserData(MetaData);
		}
		MetaData->AddCurveMetaData(Curve, false);
		MetaData->SetCurveMetaDataMorphTarget(Curve, true);
	}
	Mesh->MarkPackageDirty();
	UE_LOG(LogTemp, Display, TEXT("UCHIHA_CHAOS_BOUND asset=%s section=%d particles=%d pinned=%d capsules=%d"),
		AssetName, Section, ParticleCount, Pinned, Collision->SkeletalBodySetups.Num());
	return true;
#else
	return false;
#endif
}

bool UNinjaEditorTools::ResetHairCloth(USkeletalMesh* Mesh)
{
#if WITH_EDITOR
	if (!Mesh || !Mesh->GetImportedModel() || !Mesh->GetImportedModel()->LODModels.IsValidIndex(0))
	{
		return false;
	}
	const int32 SectionCount = Mesh->GetImportedModel()->LODModels[0].Sections.Num();
	for (int32 Section = SectionCount - 1; Section >= 0; --Section)
	{
		Mesh->RemoveClothingAsset(0, Section);
	}
	return Mesh->GetMeshClothingAssets().IsEmpty();
#else
	return false;
#endif
}

bool UNinjaEditorTools::BindHairCloth(USkeletalMesh* Mesh, FName MaterialSlot)
{
#if WITH_EDITOR
	if (!Mesh || !Mesh->GetImportedModel() || !Mesh->GetImportedModel()->LODModels.IsValidIndex(0))
	{
		return false;
	}
	FSkeletalMeshLODModel& Lod = Mesh->GetImportedModel()->LODModels[0];
	const TArray<FSkeletalMaterial>& Materials = Mesh->GetMaterials();
	const int32 Section = Lod.Sections.IndexOfByPredicate([&](const FSkelMeshSection& Candidate)
	{
		return !Candidate.HasClothingData() && Materials.IsValidIndex(Candidate.MaterialIndex)
			&& Materials[Candidate.MaterialIndex].MaterialSlotName == MaterialSlot;
	});
	if (Section == INDEX_NONE) { return false; }

	const FName CollisionName(TEXT("HairHeadAndShoulders"));
	UPhysicsAsset* Collision = FindObject<UPhysicsAsset>(Mesh, *CollisionName.ToString());
	if (!Collision)
	{
		Collision = NewObject<UPhysicsAsset>(Mesh, CollisionName, RF_Transactional);
		const FReferenceSkeleton& Skeleton = Mesh->GetRefSkeleton();
		auto AddSphere = [&](FName Bone, FVector Center, float Radius)
		{
			const int32 Index = Skeleton.FindBoneIndex(Bone);
			if (Index == INDEX_NONE) { return false; }
			USkeletalBodySetup* Setup = NewObject<USkeletalBodySetup>(Collision);
			Setup->BoneName = Bone;
			FKSphereElem Sphere(Radius);
			Sphere.Center = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Index).InverseTransformPosition(Center);
			Setup->AggGeom.SphereElems.Add(Sphere);
			Collision->SkeletalBodySetups.Add(Setup);
			return true;
		};
		if (!AddSphere(TEXT("head"), FVector(0., -2.5, 169.1), 7.7f)
			|| !AddSphere(TEXT("neck_01"), FVector(0., -6., 154.5), 5.4f)
			|| !AddSphere(TEXT("spine_05"), FVector(0., -8.5, 141.), 12.1f)
			|| !AddSphere(TEXT("clavicle_l"), FVector(11.5, -7.5, 143.), 6.1f)
			|| !AddSphere(TEXT("clavicle_r"), FVector(-11.5, -7.5, 143.), 6.1f))
		{
			return false;
		}
		Collision->UpdateBodySetupIndexMap();
		Collision->UpdateBoundsBodiesArray();
	}

	FSkeletalMeshClothBuildParams Params;
	Params.AssetName = FString::Printf(TEXT("HairTips_%d"), Section);
	Params.LodIndex = 0;
	Params.SourceSection = Section;
	UClothingAssetFactory* Factory = NewObject<UClothingAssetFactory>();
	UClothingAssetCommon* Cloth = Cast<UClothingAssetCommon>(Factory->CreateFromSkeletalMesh(Mesh, Params));
	if (!Cloth)
	{
		// Repair zero-area triangles on a transient simulation source only.
		// Keep the approved render mesh and its UV/material sections untouched.
		USkeletalMesh* Proxy = DuplicateObject<USkeletalMesh>(Mesh, GetTransientPackage(),
			MakeUniqueObjectName(GetTransientPackage(), USkeletalMesh::StaticClass(), TEXT("HairSimSource")));
		FSkeletalMeshLODModel& ProxyLod = Proxy->GetImportedModel()->LODModels[0];
		FSkelMeshSection& Source = ProxyLod.Sections[Section];
		TArray<uint32> ValidIndices;
		for (uint32 Face = 0; Face < Source.NumTriangles; ++Face)
		{
			const uint32 Index = Source.BaseIndex + 3 * Face;
			const uint32 A = ProxyLod.IndexBuffer[Index];
			const uint32 B = ProxyLod.IndexBuffer[Index + 1];
			const uint32 C = ProxyLod.IndexBuffer[Index + 2];
			const auto Position = [&](uint32 Vertex)
			{
				return Source.SoftVertices[Vertex - Source.BaseVertexIndex].Position;
			};
			if (FVector3f::CrossProduct(Position(B) - Position(A), Position(C) - Position(A)).SizeSquared() > 1.e-7f)
			{
				ValidIndices.Append({A, B, C});
			}
		}
		if (ValidIndices.IsEmpty()) { return false; }
		const int32 Removed = 3 * Source.NumTriangles - ValidIndices.Num();
		FMemory::Memcpy(ProxyLod.IndexBuffer.GetData() + Source.BaseIndex, ValidIndices.GetData(),
			ValidIndices.Num() * sizeof(uint32));
		Source.NumTriangles = ValidIndices.Num() / 3;
		UClothingAssetCommon* ProxyCloth = Cast<UClothingAssetCommon>(Factory->CreateFromSkeletalMesh(Proxy, Params));
		if (!ProxyCloth) { return false; }
		Cloth = DuplicateObject<UClothingAssetCommon>(ProxyCloth, Mesh,
			MakeUniqueObjectName(Mesh, UClothingAssetCommon::StaticClass(), FName(*Params.AssetName)));
		UE_LOG(LogTemp, Display, TEXT("HAIR_PROXY_REPAIR %s discarded=%d"), *Mesh->GetName(), Removed / 3);
	}
	if (Cloth->LodData.Num() != 1) { return false; }
	FClothLODDataCommon& ClothLod = Cloth->LodData[0];
	const TArray<FVector3f>& Vertices = ClothLod.PhysicalMeshData.Vertices;
	if (Vertices.Num() < 8) { return false; }

	TArray<float> Reach;
	Reach.Reserve(Vertices.Num());
	float Longest = 0.f;
	for (const FVector3f& Point : Vertices)
	{
		const FVector3f FromScalp((Point.X / 8.4f), (Point.Y + 3.4f) / 10.4f, (Point.Z - 170.f) / 9.6f);
		const float Distance = FromScalp.Size();
		Reach.Add(Distance);
		Longest = FMath::Max(Longest, Distance);
	}
	TArray<float> SortedReach = Reach;
	SortedReach.Sort();
	const float Root = FMath::Min(SortedReach[Vertices.Num() * 3 / 4],
		FMath::Max(1.12f, SortedReach[Vertices.Num() / 6]));
	const float Length = FMath::SmoothStep(2.f, 5.3f, Longest);
	const float TipTravel = FMath::Lerp(1.5f, 9.f, Length);
	FPointWeightMap& Travel = ClothLod.PointWeightMaps[0];
	Travel.Name = TEXT("Raiz fixa e pontas soltas");
	int32 Pinned = 0;
	for (int32 Index = 0; Index < Reach.Num(); ++Index)
	{
		const float Weight = FMath::SmoothStep(Root, FMath::Max(Root + .025f, Longest), Reach[Index]);
		Travel.SetValue(Index, TipTravel * Weight);
		Pinned += Weight <= UE_SMALL_NUMBER ? 1 : 0;
	}
	if (!Pinned || Pinned == Reach.Num()) { return false; }

	Mesh->AddClothingAsset(Cloth);
	UChaosClothConfig* Config = Cloth->GetClothConfig<UChaosClothConfig>();
	if (!Config) { return false; }
	Cloth->PhysicsAsset = Collision;
	Config->CollisionThickness = .28f;
	Config->FrictionCoefficient = .22f;
	Config->bUseCCD = true;
	Config->DampingCoefficient = .065f;
	Config->LocalDampingCoefficient = .24f;
	Config->EdgeStiffnessWeighted = { .92f, .92f };
	Config->bUseBendingElements = true;
	const float Bending = FMath::Lerp(.68f, .46f, Length);
	Config->BendingStiffnessWeighted = { Bending, Bending };
	Config->BucklingStiffnessWeighted = { .44f, .44f };
	const float Drive = FMath::Lerp(.75f, .43f, Length);
	Config->AnimDriveStiffness = { Drive, Drive };
	Config->AnimDriveDamping = { .68f, .68f };
	Config->GravityScale = FMath::Lerp(.45f, .72f, Length);
	Config->LinearVelocityScale = FVector(.88);
	Config->AngularVelocityScale = .9f;

	Cloth->ApplyParameterMasks();
	Cloth->RefreshBoneMapping(Mesh);
	Cloth->InvalidateAllCachedData();
	{
		FScopedSkeletalMeshPostEditChange ScopedPostEditChange(Mesh);
		FSkelMeshSourceSectionUserData& Data = Lod.UserSectionsData.FindOrAdd(Lod.Sections[Section].OriginalDataSectionIndex);
		if (!Cloth->BindToSkeletalMesh(Mesh, 0, Section, 0)) { return false; }
		Data.CorrespondClothAssetIndex = Mesh->GetMeshClothingAssets().Find(Cloth);
		Data.ClothingData.AssetGuid = Cloth->GetAssetGuid();
		Data.ClothingData.AssetLodIndex = 0;
	}
	TArray<UClothingAssetBase*> InUse;
	Mesh->GetClothingAssetsInUse(InUse);
	if (!InUse.Contains(Cloth)) { return false; }
	FitHairClothToOutfits(Mesh);
	Mesh->MarkPackageDirty();
	UE_LOG(LogTemp, Display, TEXT("HAIR_CHAOS_BOUND %s section=%d particles=%d pinned=%d travel=%.2f"),
		*Mesh->GetName(), Section, Vertices.Num(), Pinned, TipTravel);
	return true;
#else
	return false;
#endif
}

int32 UNinjaEditorTools::FitHairClothToOutfits(USkeletalMesh* Mesh)
{
#if WITH_EDITOR
	if (!Mesh) { return INDEX_NONE; }

	TArray<UClothingAssetCommon*> HairSections;
	float LowestHair = TNumericLimits<float>::Max();
	for (UClothingAssetBase* Asset : Mesh->GetMeshClothingAssets())
	{
		if (UClothingAssetCommon* Cloth = Cast<UClothingAssetCommon>(Asset))
		{
			HairSections.Add(Cloth);
			for (const FClothLODDataCommon& Lod : Cloth->LodData)
			{
				for (const FVector3f& Vertex : Lod.PhysicalMeshData.Vertices)
				{
					LowestHair = FMath::Min(LowestHair, Vertex.Z);
				}
			}
		}
	}
	if (HairSections.IsEmpty()) { return INDEX_NONE; }
	if (LowestHair >= 152.f) { return 0; }

	UPhysicsAsset* Collision = Cast<UPhysicsAsset>(HairSections[0]->PhysicsAsset);
	if (!Collision || Collision->GetFName() != TEXT("HairHeadAndShoulders")) { return INDEX_NONE; }

	// Dimensions in reference mesh centimetres, measured against the outer
	// silhouette of the Manto, Ninja shirt and Shinobi tunic.
	struct FCapsule
	{
		FName Bone;
		FVector Start;
		FVector End;
		float Radius;
	};
	TArray<FCapsule> Capsules = {
		{ TEXT("neck_01"), {-2.5, -5.6, 152.5}, {2.5, -5.6, 152.5}, 7.5f },
		{ TEXT("spine_05"), {-5.4, -4.7, 140.4}, {5.4, -4.7, 140.4}, 13.15f },
		{ TEXT("spine_04"), {-4.8, -4.5, 131.4}, {4.8, -4.5, 131.4}, 12.7f },
		{ TEXT("spine_03"), {-4.0, -3.0, 121.4}, {4.0, -3.0, 121.4}, 12.1f },
		{ TEXT("spine_02"), {-4.4, -1.8, 111.0}, {4.4, -1.8, 111.0}, 11.9f },
		{ TEXT("pelvis"), {-4.6, -2.4, 102.5}, {4.6, -2.4, 102.5}, 15.1f },
		{ TEXT("clavicle_l"), {7.5, -7.8, 144.0}, {16.2, -7.4, 145.2}, 7.65f },
		{ TEXT("clavicle_r"), {-7.5, -7.8, 144.0}, {-16.2, -7.4, 145.2}, 7.65f },
		{ TEXT("upperarm_l"), {18.3, -6.7, 145.9}, {28.0, -7.2, 144.2}, 7.3f },
		{ TEXT("upperarm_r"), {-18.3, -6.7, 145.9}, {-28.0, -7.2, 144.2}, 7.3f },
	};
	if (LowestHair < 109.f)
	{
		Capsules.Append({
			{ TEXT("thigh_l"), {10.0, -3.8, 92.6}, {12.0, -3.5, 72.0}, 9.2f },
			{ TEXT("thigh_r"), {-10.0, -3.8, 92.6}, {-12.0, -3.5, 72.0}, 9.2f },
		});
	}
	const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
	for (const FCapsule& Shape : Capsules)
	{
		if (Ref.FindBoneIndex(Shape.Bone) == INDEX_NONE) { return INDEX_NONE; }
	}
	for (const FCapsule& Shape : Capsules)
	{
		USkeletalBodySetup* Setup = nullptr;
		const int32 SetupIndex = Collision->FindBodyIndex(Shape.Bone);
		if (Collision->SkeletalBodySetups.IsValidIndex(SetupIndex))
		{
			Setup = Collision->SkeletalBodySetups[SetupIndex];
		}
		else
		{
			Setup = NewObject<USkeletalBodySetup>(Collision, NAME_None, RF_Transactional);
			Setup->BoneName = Shape.Bone;
			Collision->SkeletalBodySetups.Add(Setup);
		}
		const FTransform Bone = FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, Ref.FindBoneIndex(Shape.Bone));
		const FVector Axis = Bone.InverseTransformVectorNoScale(Shape.End - Shape.Start);
		FKSphylElem Capsule(Shape.Radius, Axis.Length());
		Capsule.Center = Bone.InverseTransformPosition((Shape.Start + Shape.End) * .5);
		Capsule.Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Axis.GetSafeNormal()).Rotator();
		Setup->AggGeom.SphereElems.Reset();
		Setup->AggGeom.SphylElems = { Capsule };
	}
	Collision->UpdateBodySetupIndexMap();
	Collision->UpdateBoundsBodiesArray();
	for (UClothingAssetCommon* Cloth : HairSections)
	{
		if (UChaosClothConfig* Config = Cloth->GetClothConfig<UChaosClothConfig>())
		{
			Config->CollisionThickness = .52f;
			Config->FrictionCoefficient = .14f;
			Config->bUseCCD = true;
		}
		Cloth->RefreshBoneMapping(Mesh);
	}
	Mesh->MarkPackageDirty();
	return Capsules.Num();
#else
	return INDEX_NONE;
#endif
}

TArray<FVector> UNinjaEditorTools::ReadClothParticles(USkeletalMeshComponent* Component, int32 ClothAssetIndex)
{
	TArray<FVector> Positions;
#if WITH_EDITOR
	if (Component)
	{
		for (const auto& Entry : Component->GetCurrentClothingData_GameThread())
		{
			if (ClothAssetIndex != INDEX_NONE && Entry.Key != ClothAssetIndex) { continue; }
			for (const FVector3f& Point : Entry.Value.Positions)
			{
				Positions.Add(Entry.Value.Transform.TransformPosition(FVector(Point)));
			}
		}
	}
#endif
	return Positions;
}
