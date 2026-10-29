#include "NinjaCustomization.h"
#include "NinjaVisibleBody.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

void FNinjaBodyMask::Combine(const FNinjaBodyMask& Other)
{
	if (Other.TorsoMaxZ > Other.TorsoMinZ)
	{
		if (TorsoMaxZ > TorsoMinZ)
		{
			TorsoMinZ = FMath::Min(TorsoMinZ, Other.TorsoMinZ);
			TorsoMaxZ = FMath::Max(TorsoMaxZ, Other.TorsoMaxZ);
		}
		else
		{
			TorsoMinZ = Other.TorsoMinZ;
			TorsoMaxZ = Other.TorsoMaxZ;
		}
	}
	ArmMaxT = FMath::Max(ArmMaxT, Other.ArmMaxT);
	LegMinZ = FMath::Min(LegMinZ, Other.LegMinZ);
	FootMaxZ = FMath::Max(FootMaxZ, Other.FootMaxZ);
}

UNinjaCustomizationComponent::UNinjaCustomizationComponent()
{
	SetIsReplicatedByDefault(true);
}

void UNinjaCustomizationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNinjaCustomizationComponent, Equipped);
}

void UNinjaCustomizationComponent::OnRegister()
{
	Super::OnRegister();
	Apply();
}

void UNinjaCustomizationComponent::OnUnregister()
{
	Clear();
	Super::OnUnregister();
}

#if WITH_EDITOR
void UNinjaCustomizationComponent::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
	Apply();
}
#endif

const FNinjaCustomizationPart* UNinjaCustomizationComponent::FindPart(FName PartId) const
{
	return Catalog ? Catalog->FindRow<FNinjaCustomizationPart>(PartId, TEXT("Personalização"), false) : nullptr;
}

USkeletalMeshComponent* UNinjaCustomizationComponent::ResolveBody() const
{
	if (Body)
	{
		return Body;
	}
	AActor* Owner = GetOwner();
	if (const ACharacter* Character = Cast<ACharacter>(Owner))
	{
		return GetNinjaVisibleBody(Character);
	}
	if (!Owner)
	{
		return nullptr;
	}
	TArray<USkeletalMeshComponent*> Meshes;
	Owner->GetComponents(Meshes);
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		bool bIsPart = false;
		for (const auto& Pair : Parts)
		{
			bIsPart |= Pair.Value == Mesh;
		}
		if (!bIsPart)
		{
			return Mesh;
		}
	}
	return nullptr;
}

void UNinjaCustomizationComponent::Clear()
{
	for (auto& Pair : Parts)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->DestroyComponent();
		}
	}
	Parts.Reset();
	if (BodyMID)
	{
		if (USkeletalMeshComponent* BodyMesh = ResolveBody())
		{
			BodyMesh->SetMaterial(0, OriginalBodyMaterial);
		}
		BodyMID = nullptr;
	}
}

void UNinjaCustomizationComponent::Apply()
{
	Clear();
	USkeletalMeshComponent* BodyMesh = ResolveBody();
	AActor* Owner = GetOwner();
	if (!BodyMesh || !Owner || !Catalog)
	{
		return;
	}

	// Uma peça por slot: a última da lista vence
	TMap<ENinjaCustomizationSlot, const FNinjaCustomizationPart*> BySlot;
	for (const FName& Id : Equipped)
	{
		if (const FNinjaCustomizationPart* Part = FindPart(Id))
		{
			BySlot.Add(Part->Slot, Part);
		}
	}

	FNinjaBodyMask Mask;
	UTexture2D* EyeTexture = nullptr;
	for (const auto& Pair : BySlot)
	{
		const FNinjaCustomizationPart& Part = *Pair.Value;
		Mask.Combine(Part.BodyMask);
		if (Pair.Key == ENinjaCustomizationSlot::Eyes)
		{
			EyeTexture = Part.EyeTexture.LoadSynchronous();
			continue;
		}
		USkeletalMesh* Mesh = Part.Mesh.LoadSynchronous();
		if (!Mesh)
		{
			continue;
		}
		// Peça presa ao corpo e seguindo a pose dele (Leader Pose: nenhuma animação própria)
		USkeletalMeshComponent* Comp = NewObject<USkeletalMeshComponent>(Owner, NAME_None, RF_Transient | RF_TextExportTransient);
		Comp->SetSkeletalMeshAsset(Mesh);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetupAttachment(BodyMesh);
		Comp->RegisterComponent();
		Comp->SetLeaderPoseComponent(BodyMesh);
		if (UMaterialInterface* Material = Part.Material.LoadSynchronous())
		{
			for (int32 i = 0; i < Comp->GetNumMaterials(); ++i)
			{
				Comp->SetMaterial(i, Material);
			}
		}
		Parts.Add(Pair.Key, Comp);
	}

	// Corpo: máscara do que está coberto e íris trocada (parâmetros do M_Character_Master)
	const FNinjaBodyMask NoMask;
	const bool bMasks = Mask.TorsoMaxZ > Mask.TorsoMinZ || Mask.ArmMaxT > NoMask.ArmMaxT || Mask.LegMinZ < NoMask.LegMinZ || Mask.FootMaxZ > NoMask.FootMaxZ;
	if (bMasks || EyeTexture)
	{
		OriginalBodyMaterial = BodyMesh->GetMaterial(0);
		UMaterialInterface* Base = BodyMaterial ? BodyMaterial.Get() : OriginalBodyMaterial.Get();
		BodyMID = UMaterialInstanceDynamic::Create(Base, this);
		BodyMID->SetScalarParameterValue(TEXT("Mask_TorsoMinZ"), Mask.TorsoMinZ);
		BodyMID->SetScalarParameterValue(TEXT("Mask_TorsoMaxZ"), Mask.TorsoMaxZ);
		BodyMID->SetScalarParameterValue(TEXT("Mask_ArmMaxT"), Mask.ArmMaxT);
		BodyMID->SetScalarParameterValue(TEXT("Mask_LegMinZ"), Mask.LegMinZ);
		BodyMID->SetScalarParameterValue(TEXT("Mask_FootMaxZ"), Mask.FootMaxZ);
		BodyMID->SetScalarParameterValue(TEXT("EyeSwap"), EyeTexture ? 1.f : 0.f);
		if (EyeTexture)
		{
			BodyMID->SetTextureParameterValue(TEXT("EyeTexture"), EyeTexture);
		}
		BodyMesh->SetMaterial(0, BodyMID);
	}
}

void UNinjaCustomizationComponent::SetEquipped(const TArray<FName>& NewEquipped)
{
	Equipped = NewEquipped;
	Apply();   // na hora, também no cliente (o servidor confirma pela replicação)
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerSetEquipped(NewEquipped);
	}
}

void UNinjaCustomizationComponent::ServerSetEquipped_Implementation(const TArray<FName>& NewEquipped)
{
	Equipped = NewEquipped;
	Apply();
}

void UNinjaCustomizationComponent::EquipPart(FName PartId)
{
	const FNinjaCustomizationPart* Part = FindPart(PartId);
	if (!Part)
	{
		return;
	}
	TArray<FName> NewEquipped;
	for (const FName& Id : Equipped)
	{
		const FNinjaCustomizationPart* Other = FindPart(Id);
		if (Other && Other->Slot != Part->Slot)
		{
			NewEquipped.Add(Id);
		}
	}
	NewEquipped.Add(PartId);
	SetEquipped(NewEquipped);
}

void UNinjaCustomizationComponent::UnequipSlot(ENinjaCustomizationSlot Slot)
{
	TArray<FName> NewEquipped;
	for (const FName& Id : Equipped)
	{
		const FNinjaCustomizationPart* Part = FindPart(Id);
		if (Part && Part->Slot != Slot)
		{
			NewEquipped.Add(Id);
		}
	}
	SetEquipped(NewEquipped);
}

USkeletalMeshComponent* UNinjaCustomizationComponent::GetPartComponent(ENinjaCustomizationSlot Slot) const
{
	const TObjectPtr<USkeletalMeshComponent>* Found = Parts.Find(Slot);
	return Found ? Found->Get() : nullptr;
}

UNinjaCustomizationComponent* UNinjaCustomizationLibrary::EquipOnActor(AActor* Actor, UDataTable* Catalog, const TArray<FName>& PartIds, UMaterialInterface* BodyMaterial)
{
	if (!Actor)
	{
		return nullptr;
	}
	UNinjaCustomizationComponent* Comp = Actor->FindComponentByClass<UNinjaCustomizationComponent>();
	if (!Comp)
	{
		Comp = NewObject<UNinjaCustomizationComponent>(Actor, TEXT("Personalizacao"));
		Comp->Catalog = Catalog;
		Comp->BodyMaterial = BodyMaterial;
		Comp->RegisterComponent();
		Actor->AddInstanceComponent(Comp);
	}
	Comp->Catalog = Catalog;
	Comp->BodyMaterial = BodyMaterial;
	Comp->Equipped = PartIds;   // chamado no servidor (ou sem rede); os clientes recebem pela replicação
	Comp->Apply();
	return Comp;
}
