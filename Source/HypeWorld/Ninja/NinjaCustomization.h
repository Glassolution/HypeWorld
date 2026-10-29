// Personalização modular (29/10/2026): o jogador monta o personagem com peças de personagens diferentes sobre o
// SK_Base_Body (esqueleto do Indra). Cada peça é uma Skeletal Mesh no mesmo esqueleto, presa ao corpo por Leader Pose;
// olhos são só uma textura de íris projetada no rosto do corpo base pelo M_Character_Master.
// A máscara de corpo esconde (Opacity Mask) a pele que ficaria por baixo de cada roupa, em regiões da bind pose.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NinjaCustomization.generated.h"

class USkeletalMesh;
class USkeletalMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture2D;

UENUM(BlueprintType)
enum class ENinjaCustomizationSlot : uint8
{
	Hair,
	Top,
	Bottom,
	Shoes,
	Headband,
	Gloves,
	Pouch,
	Wraps,
	Eyes
};

/** O que uma roupa esconde do corpo base (cm, bind pose do Indra). Valores padrão = não esconde nada. */
USTRUCT(BlueprintType)
struct FNinjaBodyMask
{
	GENERATED_BODY()

	/** Tronco escondido entre essas alturas (Min >= Max desliga) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float TorsoMinZ = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float TorsoMaxZ = 0.f;
	/** Braço escondido do ombro até essa distância ao longo do braço (cm; o punho fica em ~50) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ArmMaxT = -100.f;
	/** Perna escondida da virilha até essa altura */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float LegMinZ = 999.f;
	/** Pé escondido do chão até essa altura */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float FootMaxZ = 0.f;

	/** Junta duas máscaras: esconde o que qualquer uma esconde */
	void Combine(const FNinjaBodyMask& Other);
};

/** Linha do DT_CustomizationParts (o nome da linha é o ID, ex.: Top_Minato) */
USTRUCT(BlueprintType)
struct FNinjaCustomizationPart : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ENinjaCustomizationSlot Slot = ENinjaCustomizationSlot::Top;
	/** Peça (todas as fatias de corpo são Skeletal Meshes no SKEL_Indra); vazio no slot de olhos */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<USkeletalMesh> Mesh;
	/** Material instance do M_Character_Master; vazio = o da malha */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UMaterialInterface> Material;
	/** Olhos: íris (T_Eyes_<Nome>) projetada no rosto do corpo base */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UTexture2D> EyeTexture;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FNinjaBodyMask BodyMask;
};

/** Veste peças do catálogo num corpo base. Funciona no editor (preview) e no jogo; em rede o servidor decide as peças
 *  (Equipped replicado) e cada máquina monta as malhas localmente. */
UCLASS(ClassGroup=(Ninja), meta=(BlueprintSpawnableComponent))
class HYPEWORLD_API UNinjaCustomizationComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UNinjaCustomizationComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Personalização") TObjectPtr<UDataTable> Catalog;
	/** IDs das peças vestidas (no máximo uma por slot; a última vence) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_Equipped, Category="Personalização") TArray<FName> Equipped;
	/** Material do corpo com Opacity Mask ligado (MI_Base_Body_Custom); vazio = o atual do corpo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Personalização") TObjectPtr<UMaterialInterface> BodyMaterial;
	/** Corpo onde as peças entram; vazio = corpo visível do personagem ou a primeira Skeletal Mesh do ator */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Personalização") TObjectPtr<USkeletalMeshComponent> Body;

	/** Recria as peças e o material do corpo a partir de Equipped */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Personalização") void Apply();
	/** Veste uma peça (troca a do mesmo slot) */
	UFUNCTION(BlueprintCallable, Category="Personalização") void EquipPart(FName PartId);
	UFUNCTION(BlueprintCallable, Category="Personalização") void UnequipSlot(ENinjaCustomizationSlot Slot);

	UFUNCTION(BlueprintPure, Category="Personalização") USkeletalMeshComponent* GetPartComponent(ENinjaCustomizationSlot Slot) const;

protected:

	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif

private:

	UFUNCTION() void OnRep_Equipped() { Apply(); }
	UFUNCTION(Server, Reliable) void ServerSetEquipped(const TArray<FName>& NewEquipped);
	void SetEquipped(const TArray<FName>& NewEquipped);
	const FNinjaCustomizationPart* FindPart(FName PartId) const;
	USkeletalMeshComponent* ResolveBody() const;
	void Clear();

	UPROPERTY(Transient) TMap<ENinjaCustomizationSlot, TObjectPtr<USkeletalMeshComponent>> Parts;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BodyMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> OriginalBodyMaterial;
};

UCLASS()
class HYPEWORLD_API UNinjaCustomizationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Cria (ou reaproveita) o componente de personalização no ator e veste as peças. Para testes e para o jogo. */
	UFUNCTION(BlueprintCallable, Category="Personalização")
	static UNinjaCustomizationComponent* EquipOnActor(AActor* Actor, UDataTable* Catalog, const TArray<FName>& PartIds, UMaterialInterface* BodyMaterial);
};
