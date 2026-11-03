#include "HypeAppearanceComponent.h"
#include "Combat/NinjaVisibleBody.h"
#include "UI/HypeProfileSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

UHypeAppearanceComponent::UHypeAppearanceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mesh(TEXT("/Game/HypeWorld/Personagens/Indra/Modular/SK_Indra_Head.SK_Indra_Head"));
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> OldBody(TEXT("/Game/HypeWorld/Characters/Base/SK_Base_Body.SK_Base_Body"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/HypeWorld/Personagens/Indra/Modular/M_Indra_Cranio.M_Indra_Cranio"));
    // PartsCatalog fica vazio: as peças avulsas do DT_CustomizationParts foram feitas para o corpo antigo.
    BaseBody = Mesh.Object;
    EffectsBody = OldBody.Object;
    SkinMaterial = Material.Object;
    auto AddModular = [this](const TCHAR* Id,const TCHAR* Label,bool bHair,const TCHAR* Path)
    {
        ConstructorHelpers::FObjectFinder<USkeletalMesh> Found(Path);
        if (!Found.Succeeded()) { return; }
        FHypeCosmeticPart Part;
        Part.Id=FName(Id);Part.Label=FText::FromString(Label);Part.Mesh=Found.Object;
        (bHair ? HairOptions : OutfitOptions).Add(Part);
    };
    AddModular(TEXT("Outfit_ItachiAnbu"),TEXT("Anbu"),false,TEXT("/Game/HypeWorld/Personagens/Indra/Modular/Trajes/ItachiAnbu/SK_Outfit_ItachiAnbu.SK_Outfit_ItachiAnbu"));
    AddModular(TEXT("Outfit_Indra"),TEXT("Indra"),false,TEXT("/Game/HypeWorld/Personagens/Indra/Modular/SK_Outfit_Indra.SK_Outfit_Indra"));
    AddModular(TEXT("Hair_Indra"),TEXT("Indra"),true,TEXT("/Game/HypeWorld/Personagens/Indra/Modular/SK_Hair_Indra.SK_Hair_Indra"));
    // Trajes modulares do catálogo (slot Traje); só estas linhas valem aqui, as outras eram do corpo antigo.
    ConstructorHelpers::FObjectFinder<UDataTable> Parts(TEXT("/Game/HypeWorld/Characters/Parts/DT_CustomizationParts.DT_CustomizationParts"));
    if (Parts.Succeeded())
    {
        for (const TPair<FName,uint8*>& Row : Parts.Object->GetRowMap())
        {
            const FNinjaCustomizationPart* Part = Parts.Object->GetRowStruct()==FNinjaCustomizationPart::StaticStruct() ? reinterpret_cast<const FNinjaCustomizationPart*>(Row.Value) : nullptr;
            if (Part && Part->Slot==ENinjaCustomizationSlot::Outfit && !Part->bOculto)
            {
                AddModular(*Row.Key.ToString(),*Part->DisplayName.ToString(),false,*Part->Mesh.ToSoftObjectPath().ToString());
            }
        }
    }
    auto AddFitted = [this](const TCHAR* Id,const TCHAR* Label,bool bHair,const FNinjaBodyMask& Mask=FNinjaBodyMask())
    {
        // Os trajes antigos contam com o corpo antigo embaixo (mãos e pele); na cabeça modular ficariam sem mãos.
        if (!bHair) { return; }
        const FString Path=FString::Printf(TEXT("/Game/Ninja/Characters/Customization/%s/SK_%s.SK_%s"),Id,Id,Id);
        ConstructorHelpers::FObjectFinder<USkeletalMesh> Found(*Path);
        if (!Found.Succeeded()) { return; }
        FHypeCosmeticPart Part;
        Part.Id=FName(Id);Part.Label=FText::FromString(Label);Part.Mesh=Found.Object;
        Part.RequiredClan=bHair ? NAME_None : FName(TEXT("Uchiha"));Part.BodyMask=Mask;
        (bHair ? HairOptions : OutfitOptions).Add(Part);
    };
#include "HypeFittedCatalog.inl"
}

void UHypeAppearanceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UHypeAppearanceComponent, Appearance);
}

void UHypeAppearanceComponent::ValidateParts(FHypeAppearance& Value) const
{
    Value.Sanitize();
    if (Value.bCustomized && Value.Clan.IsNone()) { Value.Clan=HypeIdentity::ResolveClan(Value.Clan); }
    auto Valid = [this,&Value](FName Id, const TArray<FHypeCosmeticPart>& Parts)
    {
        return Parts.ContainsByPredicate([&](const FHypeCosmeticPart& P)
        {
            return !Id.IsNone() && P.Id == Id && P.AllowsClan(Value.Clan) && P.Mesh && BaseBody && P.Mesh->GetSkeleton() == BaseBody->GetSkeleton();
        });
    };
    if (!Valid(Value.HairId, HairOptions)) { Value.HairId = NAME_None; }
    if (!Valid(Value.OutfitId, OutfitOptions)) { Value.OutfitId = NAME_None; }
    if (!FindCatalogPart(Value.EyesId, ENinjaCustomizationSlot::Eyes)) { Value.EyesId = NAME_None; }
    if (!FindCatalogPart(Value.TopId, ENinjaCustomizationSlot::Top)) { Value.TopId = NAME_None; }
    if (!FindCatalogPart(Value.BottomId, ENinjaCustomizationSlot::Bottom)) { Value.BottomId = NAME_None; }
    if (!FindCatalogPart(Value.ShoesId, ENinjaCustomizationSlot::Shoes)) { Value.ShoesId = NAME_None; }
}

const FNinjaCustomizationPart* UHypeAppearanceComponent::FindCatalogPart(FName Id, ENinjaCustomizationSlot Slot) const
{
    if (Id.IsNone() || !PartsCatalog) { return nullptr; }
    const FNinjaCustomizationPart* Part = PartsCatalog->FindRow<FNinjaCustomizationPart>(Id, TEXT("Aparencia"), false);
    if (!Part || Part->Slot != Slot) { return nullptr; }
    if (Slot == ENinjaCustomizationSlot::Eyes) { return Part->EyeTexture.IsNull() ? nullptr : Part; }
    // Só peças no esqueleto do corpo base (Leader Pose exige o mesmo esqueleto)
    const USkeletalMesh* Mesh = Part->Mesh.LoadSynchronous();
    return Mesh && BaseBody && Mesh->GetSkeleton() == BaseBody->GetSkeleton() ? Part : nullptr;
}

TArray<FName> UHypeAppearanceComponent::GetCatalogParts(ENinjaCustomizationSlot Slot) const
{
    TArray<FName> Ids;
    if (PartsCatalog)
    {
        for (const FName& Row : PartsCatalog->GetRowNames())
        {
            if (FindCatalogPart(Row, Slot)) { Ids.Add(Row); }
        }
    }
    return Ids;
}

void UHypeAppearanceComponent::ApplyCatalogPart(const FNinjaCustomizationPart* Selected, USkeletalMeshComponent*& Part)
{
    USkeletalMesh* Mesh = Selected ? Selected->Mesh.LoadSynchronous() : nullptr;
    if (!Mesh)
    {
        if (Part) { Part->SetVisibility(false); }
        return;
    }
    if (!Part)
    {
        Part = NewObject<USkeletalMeshComponent>(GetOwner());
        Part->SetupAttachment(Body);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetOwner()->AddInstanceComponent(Part);
        Part->RegisterComponent();
    }
    if (Part->GetSkeletalMeshAsset() != Mesh)
    {
        Part->EmptyOverrideMaterials();
        Part->SetSkeletalMesh(Mesh);
        if (UMaterialInterface* Material = Selected->Material.LoadSynchronous())
        {
            for (int32 Index=0; Index<Part->GetNumMaterials(); ++Index) { Part->SetMaterial(Index, Material); }
        }
    }
    Part->SetLeaderPoseComponent(Body);
    Part->SetVisibility(true);
}

void UHypeAppearanceComponent::Commit(const FHypeAppearance& Value)
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    if (!Pawn || (!Pawn->IsLocallyControlled() && !GetOwner()->HasAuthority())) { return; }
    if (GetOwner()->HasAuthority()) { ServerCommit_Implementation(Value); }
    else { ServerCommit(Value); }
}

void UHypeAppearanceComponent::ServerCommit_Implementation(FHypeAppearance Value)
{
    // O servidor atribui o clã; o cliente não pode escolher outro pela ficha enviada.
    Value.Clan=Value.bCustomized ? HypeIdentity::ResolveClan(Appearance.Clan.IsNone() ? Value.Clan : Appearance.Clan) : NAME_None;
    ValidateParts(Value);
    Appearance = Value;
    UE_LOG(LogTemp, Log, TEXT("Aparência confirmada: %s | cabelo %s | roupa %s | olhos %s | cima %s | baixo %s | calçado %s"), *Value.GetFullName(),
        *Value.HairId.ToString(), *Value.OutfitId.ToString(), *Value.EyesId.ToString(), *Value.TopId.ToString(), *Value.BottomId.ToString(), *Value.ShoesId.ToString());
    OnRep_Appearance();
    GetOwner()->ForceNetUpdate();
}
void UHypeAppearanceComponent::OnRep_Appearance() { bPendingApply = !Apply(Appearance); }
void UHypeAppearanceComponent::Preview(const FHypeAppearance& Draft) { Apply(Draft); }
void UHypeAppearanceComponent::Restore() { Apply(Appearance); }

void UHypeAppearanceComponent::ApplyPart(FName Id, const TArray<FHypeCosmeticPart>& Options, USkeletalMeshComponent*& Part, bool bHair, const FHypeAppearance& Value)
{
    const FHypeCosmeticPart* Selected = Options.FindByPredicate([&](const FHypeCosmeticPart& P) { return !Id.IsNone() && P.Id == Id && P.AllowsClan(Value.Clan); });
    if (!Selected || !Selected->Mesh || Selected->Mesh->GetSkeleton() != BaseBody->GetSkeleton())
    {
        if (Part) { Part->SetVisibility(false); }
        return;
    }
    if (!Part)
    {
        Part = NewObject<USkeletalMeshComponent>(GetOwner());
        Part->SetupAttachment(Body);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        // A aura usa o Custom Depth para ficar atrás do personagem; o corpo agora está no traje
        Part->SetRenderCustomDepth(true);
        GetOwner()->AddInstanceComponent(Part);
        Part->RegisterComponent();
    }
    if (Part->GetSkeletalMeshAsset()!=Selected->Mesh)
    {
        // Cada cabelo usa sua própria textura; um MID da opção anterior não pode ficar no componente.
        Part->EmptyOverrideMaterials();
        Part->SetSkeletalMesh(Selected->Mesh);
    }
    Part->SetLeaderPoseComponent(Body);
    Part->SetVisibility(true);
    if (bHair)
    {
        for (int32 Index=0; Index<Part->GetNumMaterials(); ++Index)
        {
            if (UMaterialInstanceDynamic* MID = Part->CreateDynamicMaterialInstance(Index))
            {
                MID->SetVectorParameterValue(TEXT("BaseColor Tint"), Value.HairColor);
            }
        }
    }
}

bool UHypeAppearanceComponent::Apply(const FHypeAppearance& Input)
{
    const ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) { return false; }
    if (!Body)
    {
        Body = GetNinjaVisibleBody(Character);
        if (!Body || !Body->GetSkeletalMeshAsset()) { Body = nullptr; return false; }
        OriginalMesh = Body->GetSkeletalMeshAsset();
        OriginalScale = Body->GetRelativeScale3D();
    }
    if (!BaseBody || BaseBody->GetSkeleton() != OriginalMesh->GetSkeleton()) { return false; }
    FHypeAppearance Value = Input;
    ValidateParts(Value);
    // A malha base é só a cabeça: o resto do corpo vem do traje. Sem escolha (ou perfil antigo) usa o traje padrão.
    if (Value.OutfitId.IsNone() && OutfitOptions.Num()>0) { Value.OutfitId = OutfitOptions[0].Id; }
    if (Body->GetSkeletalMeshAsset() != BaseBody)
    {
        Body->SetSkeletalMesh(BaseBody, false);
        // Remove overrides do modelo anterior só nesta instância; assets permanecem intactos.
        Body->EmptyOverrideMaterials();
    }
    // Escala uniforme visual: não deforma ossos nem muda cápsula, alcance ou velocidades.
    Body->SetRelativeScale3D(OriginalScale * (Value.HeightCm / 190.782f));
    USkeletalMeshComponent* HairPtr = Hair;
    ApplyPart(Value.HairId, HairOptions, HairPtr, true, Value);
    Hair = HairPtr;
    USkeletalMeshComponent* OutfitPtr = Outfit;
    ApplyPart(Value.OutfitId, OutfitOptions, OutfitPtr, false, Value);
    Outfit = OutfitPtr;
    const bool bFullOutfit = Outfit && Outfit->IsVisible();
    // Mesmo tom no rosto e nos braços; SkinTint é multiplicado na cor do rosto (branco = pele original).
    const FLinearColor SkinTone = FLinearColor(FColor(238, 206, 189)) * Value.SkinTint;
    for (USkeletalMeshComponent* Skin : { Body.Get(), Outfit.Get() })
    {
        for (int32 Index=0; Skin && Index<Skin->GetNumMaterials(); ++Index)
        {
            if (UMaterialInstanceDynamic* MID = Skin->CreateDynamicMaterialInstance(Index))
            {
                MID->SetVectorParameterValue(TEXT("SkinTone"), SkinTone);
            }
        }
    }
    USkeletalMeshComponent* TopPtr = Top;
    USkeletalMeshComponent* BottomPtr = Bottom;
    USkeletalMeshComponent* ShoesPtr = Shoes;
    ApplyCatalogPart(bFullOutfit ? nullptr : FindCatalogPart(Value.TopId, ENinjaCustomizationSlot::Top), TopPtr);
    ApplyCatalogPart(bFullOutfit ? nullptr : FindCatalogPart(Value.BottomId, ENinjaCustomizationSlot::Bottom), BottomPtr);
    ApplyCatalogPart(bFullOutfit ? nullptr : FindCatalogPart(Value.ShoesId, ENinjaCustomizationSlot::Shoes), ShoesPtr);
    Top = TopPtr;
    Bottom = BottomPtr;
    Shoes = ShoesPtr;
    return true;
}

void UHypeAppearanceComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt, TickType, Fn);
    APawn* Pawn = Cast<APawn>(GetOwner());
    if (!bLocalProfileLoaded && Pawn && Pawn->IsLocallyControlled())
    {
        bLocalProfileLoaded = true;
        if (const UHypeProfileSubsystem* Profile = GetWorld()->GetGameInstance()->GetSubsystem<UHypeProfileSubsystem>())
        {
            if (Profile->HasCharacter()) { Commit(Profile->Appearance); }
        }
    }
    if (bPendingApply) { bPendingApply = !Apply(Appearance); }
}
