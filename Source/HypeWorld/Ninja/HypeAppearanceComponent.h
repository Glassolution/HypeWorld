#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HypeAppearanceTypes.h"
#include "NinjaCustomization.h"
#include "HypeAppearanceComponent.generated.h"
class USkeletalMesh;
class USkeletalMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UDataTable;

USTRUCT(BlueprintType)
struct FHypeCosmeticPart
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Label;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName RequiredClan;
    /** 0 para roupa geral; 1 a 3 para os trajes exclusivos de um clã. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="3")) int32 ClanOutfitSlot = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FNinjaBodyMask BodyMask;
    bool AllowsClan(FName Clan) const { return RequiredClan.IsNone() || RequiredClan==Clan; }
    /** Malha compatível com o rig e a bind pose do BaseBody. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<USkeletalMesh> Mesh;
};

UCLASS(ClassGroup=(Ninja), meta=(BlueprintSpawnableComponent))
class HYPEWORLD_API UHypeAppearanceComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHypeAppearanceComponent();
    /** Cabeça modular; o resto do corpo vem do traje. */
    UPROPERTY(EditAnywhere, Category="Aparencia") TObjectPtr<USkeletalMesh> BaseBody;
    /** Corpo inteiro para as cascas de efeito (aura, chakra nos pés), que precisam de pés e tronco. */
    UPROPERTY(EditAnywhere, Category="Aparencia") TObjectPtr<USkeletalMesh> EffectsBody;
    UPROPERTY(EditAnywhere, Category="Aparencia") TObjectPtr<UMaterialInterface> SkinMaterial;
    UPROPERTY(EditAnywhere, Category="Aparencia") TArray<FHypeCosmeticPart> HairOptions;
    UPROPERTY(EditAnywhere, Category="Aparencia") TArray<FHypeCosmeticPart> OutfitOptions;
    /** Peças avulsas (olhos, parte de cima, de baixo, calçado): linhas do DT_CustomizationParts */
    UPROPERTY(EditAnywhere, Category="Aparencia") TObjectPtr<UDataTable> PartsCatalog;
    /** Linhas do catálogo de um slot, na ordem da tabela (só as que servem no corpo base) */
    TArray<FName> GetCatalogParts(ENinjaCustomizationSlot Slot) const;
    const FNinjaCustomizationPart* FindCatalogPart(FName Id, ENinjaCustomizationSlot Slot) const;
    UPROPERTY(ReplicatedUsing=OnRep_Appearance, BlueprintReadOnly, Category="Aparencia") FHypeAppearance Appearance;
    /** Mesmo filtro de clã e skeleton utilizado pelo criador. */
    UFUNCTION(BlueprintPure, Category="Aparencia") TArray<FHypeCosmeticPart> GetOutfitsForClan(FName Clan) const;
    void Preview(const FHypeAppearance& Draft);
    void Restore();
    void Commit(const FHypeAppearance& Value);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
protected:
    virtual void TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* Fn) override;
private:
    UFUNCTION(Server, Reliable) void ServerCommit(FHypeAppearance Value);
    UFUNCTION() void OnRep_Appearance();
    bool Apply(const FHypeAppearance& Value);
    void ValidateParts(FHypeAppearance& Value) const;
    void RegisterSilentOutfits(USkeletalMesh* ReferenceBody);
    void ApplyPart(FName Id, const TArray<FHypeCosmeticPart>& Options, USkeletalMeshComponent*& Part, bool bHair, const FHypeAppearance& Value);
    /** Peça avulsa do catálogo num componente próprio (nullptr = esconde) */
    void ApplyCatalogPart(const FNinjaCustomizationPart* Selected, USkeletalMeshComponent*& Part);
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;
    UPROPERTY() TObjectPtr<USkeletalMesh> OriginalMesh;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Hair;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Outfit;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Top;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Bottom;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Shoes;
    FVector OriginalScale = FVector::OneVector;
    bool bLocalProfileLoaded = false;
    bool bPendingApply = true;
};
