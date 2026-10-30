#include "HypeAppearanceComponent.h"
#include "Combat/NinjaVisibleBody.h"
#include "UI/HypeProfileSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

UHypeAppearanceComponent::UHypeAppearanceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mesh(TEXT("/Game/HypeWorld/Characters/Base/SK_Base_Body.SK_Base_Body"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Ninja/Characters/Customization/Materials/MI_CreatorBody.MI_CreatorBody"));
    BaseBody = Mesh.Object;
    SkinMaterial = Material.Object;
    auto AddFitted = [this](const TCHAR* Id,const TCHAR* Label,bool bHair,const FNinjaBodyMask& Mask=FNinjaBodyMask())
    {
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
        for (int32 Index=0; Index<Body->GetNumMaterials(); ++Index) { OriginalMaterials.Add(Body->GetMaterial(Index)); }
    }
    if (!Input.bCustomized)
    {
        if (Body->GetSkeletalMeshAsset() != OriginalMesh) { Body->SetSkeletalMesh(OriginalMesh, false); Body->EmptyOverrideMaterials(); }
        Body->SetRelativeScale3D(OriginalScale);
        for (int32 Index=0; Index<OriginalMaterials.Num(); ++Index) { Body->SetMaterial(Index, OriginalMaterials[Index]); }
        SkinMID = nullptr;
        if (Hair) { Hair->SetVisibility(false); }
        if (Outfit) { Outfit->SetVisibility(false); }
        return true;
    }
    if (!BaseBody || !SkinMaterial || BaseBody->GetSkeleton() != OriginalMesh->GetSkeleton()) { return false; }
    FHypeAppearance Value = Input;
    ValidateParts(Value);
    if (Body->GetSkeletalMeshAsset() != BaseBody)
    {
        Body->SetSkeletalMesh(BaseBody, false);
        // Remove overrides do modelo anterior só nesta instância; assets permanecem intactos.
        Body->EmptyOverrideMaterials();
        SkinMID = nullptr;
    }
    // Escala uniforme visual: não deforma ossos nem muda cápsula, alcance ou velocidades.
    Body->SetRelativeScale3D(OriginalScale * (Value.HeightCm / 190.782f));
    if (!SkinMID)
    {
        Body->SetMaterial(0, SkinMaterial);
        SkinMID = Body->CreateDynamicMaterialInstance(0, SkinMaterial);
    }
    if (SkinMID)
    {
        SkinMID->SetVectorParameterValue(TEXT("BaseColor Tint"),Value.SkinTint);
        const FHypeCosmeticPart* Selected=OutfitOptions.FindByPredicate([&](const FHypeCosmeticPart& P) { return P.Id==Value.OutfitId && P.AllowsClan(Value.Clan); });
        const FNinjaBodyMask Mask=Selected ? Selected->BodyMask : FNinjaBodyMask();
        SkinMID->SetScalarParameterValue(TEXT("Mask_TorsoMinZ"),Mask.TorsoMinZ);
        SkinMID->SetScalarParameterValue(TEXT("Mask_TorsoMaxZ"),Mask.TorsoMaxZ);
        SkinMID->SetScalarParameterValue(TEXT("Mask_ArmMaxT"),Mask.ArmMaxT);
        SkinMID->SetScalarParameterValue(TEXT("Mask_LegMinZ"),Mask.LegMinZ);
        SkinMID->SetScalarParameterValue(TEXT("Mask_FootMaxZ"),Mask.FootMaxZ);
    }
    USkeletalMeshComponent* HairPtr = Hair;
    ApplyPart(Value.HairId, HairOptions, HairPtr, true, Value);
    Hair = HairPtr;
    if (SkinMID)
    {
        // Couro cabeludo com a cor do cabelo (máscara T_Base_Body_ScalpMask): sem pele clara entre as mechas.
        const bool bHasHair = Hair && Hair->IsVisible();
        SkinMID->SetVectorParameterValue(TEXT("Scalp Color"), Value.HairColor);
        SkinMID->SetScalarParameterValue(TEXT("Scalp Strength"), bHasHair ? ScalpStrength : 0.0f);
    }
    USkeletalMeshComponent* OutfitPtr = Outfit;
    ApplyPart(Value.OutfitId, OutfitOptions, OutfitPtr, false, Value);
    Outfit = OutfitPtr;
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
