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
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FNinjaBodyMask BodyMask;
    bool AllowsClan(FName Clan) const { return RequiredClan.IsNone() || RequiredClan==Clan; }
    /** Malha preparada no rig Indra, com bind pose original. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<USkeletalMesh> Mesh;
};

UCLASS(ClassGroup=(Ninja), meta=(BlueprintSpawnableComponent))
class HYPEWORLD_API UHypeAppearanceComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHypeAppearanceComponent();
    UPROPERTY(EditAnywhere, Category="Aparencia") TObjectPtr<USkeletalMesh> BaseBody;
    UPROPERTY(EditAnywhere, Category="Aparencia") TObjectPtr<UMaterialInterface> SkinMaterial;
    UPROPERTY(EditAnywhere, Category="Aparencia") TArray<FHypeCosmeticPart> HairOptions;
    UPROPERTY(EditAnywhere, Category="Aparencia") TArray<FHypeCosmeticPart> OutfitOptions;
    /** Peças avulsas (olhos, parte de cima, de baixo, calçado): linhas do DT_CustomizationParts */
    UPROPERTY(EditAnywhere, Category="Aparencia") TObjectPtr<UDataTable> PartsCatalog;
    /** Linhas do catálogo de um slot, na ordem da tabela (só as que servem no corpo base) */
    TArray<FName> GetCatalogParts(ENinjaCustomizationSlot Slot) const;
    const FNinjaCustomizationPart* FindCatalogPart(FName Id, ENinjaCustomizationSlot Slot) const;
    /** Quanto o topo da cabeça (máscara do couro cabeludo) pega a cor do cabelo quando há cabelo. */
    UPROPERTY(EditAnywhere, Category="Aparencia", meta=(ClampMin="0", ClampMax="1")) float ScalpStrength = 0.85f;
    UPROPERTY(ReplicatedUsing=OnRep_Appearance, BlueprintReadOnly, Category="Aparencia") FHypeAppearance Appearance;
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
    void ApplyPart(FName Id, const TArray<FHypeCosmeticPart>& Options, USkeletalMeshComponent*& Part, bool bHair, const FHypeAppearance& Value);
    /** Peça avulsa do catálogo num componente próprio (nullptr = esconde) */
    void ApplyCatalogPart(const FNinjaCustomizationPart* Selected, USkeletalMeshComponent*& Part);
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;
    UPROPERTY() TObjectPtr<USkeletalMesh> OriginalMesh;
    UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SkinMID;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Hair;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Outfit;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Top;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Bottom;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Shoes;
    FVector OriginalScale = FVector::OneVector;
    bool bLocalProfileLoaded = false;
    bool bPendingApply = true;
};
