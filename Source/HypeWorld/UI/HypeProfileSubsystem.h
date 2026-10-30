// Perfil local: escolhas de criação persistidas no computador do jogador.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Ninja/HypeAppearanceTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HypeProfileSubsystem.generated.h"

UCLASS()
class HYPEWORLD_API UHypeCharacterSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) FName Vila;
    UPROPERTY(SaveGame) FName Cla;
    UPROPERTY(SaveGame) FHypeAppearance Appearance;
};

UCLASS()
class HYPEWORLD_API UHypeProfileSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    UPROPERTY(BlueprintReadOnly, Category="Perfil") FName Vila;
    UPROPERTY(BlueprintReadOnly, Category="Perfil") FName Cla;
    UFUNCTION(BlueprintPure, Category="Perfil") bool HasCharacter() const { return (Appearance.bCustomized && Appearance.Name.Len() >= 2) || (!Vila.IsNone() && !Cla.IsNone()); }
    UPROPERTY(BlueprintReadOnly, Category="Perfil") FHypeAppearance Appearance;
    bool CreateCharacter(FName Village, FName Clan, const FHypeAppearance& NewAppearance);
    bool SaveAffiliation(FName Village, FName Clan);
    // Só controla o fluxo desta sessão; não é estado de combate ou progressão em rede.
    bool bEntryShown = false;
};
