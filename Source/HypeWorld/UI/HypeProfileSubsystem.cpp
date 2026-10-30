#include "HypeProfileSubsystem.h"
#include "Kismet/GameplayStatics.h"

namespace { const TCHAR* ProfileSlot = TEXT("HypeWorld_Character"); }

void UHypeProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (UGameplayStatics::DoesSaveGameExist(ProfileSlot, 0))
    {
        if (UHypeCharacterSave* Save = Cast<UHypeCharacterSave>(UGameplayStatics::LoadGameFromSlot(ProfileSlot, 0)))
        {
            Vila = Save->Vila;
            Cla = Save->Cla;
            Appearance = Save->Appearance;
            Appearance.Sanitize();
            if (HasCharacter())
            {
                // Regra atual: somente Uchiha está disponível, inclusive para perfis antigos.
                Cla=HypeIdentity::ResolveClan(Cla);
                Appearance.Clan=Cla;
                Save->Cla=Cla;
                Save->Appearance=Appearance;
                UGameplayStatics::SaveGameToSlot(Save,ProfileSlot,0);
            }
        }
    }
}

bool UHypeProfileSubsystem::CreateCharacter(FName Village, FName Clan, const FHypeAppearance& NewAppearance)
{
    if (HasCharacter() || !NewAppearance.bCustomized) { return false; }
    UHypeCharacterSave* Save = Cast<UHypeCharacterSave>(UGameplayStatics::CreateSaveGameObject(UHypeCharacterSave::StaticClass()));
    if (!Save) { return false; }
    Save->Vila = Village;
    Save->Cla = HypeIdentity::RollClan();
    Save->Appearance = NewAppearance;
    Save->Appearance.Sanitize();
    Save->Appearance.Clan=Save->Cla;
    if (Save->Appearance.Name.Len() < 2) { return false; }
    if (!UGameplayStatics::SaveGameToSlot(Save, ProfileSlot, 0)) { return false; }
    Vila = Village;
    Cla = Save->Cla;
    Appearance = Save->Appearance;
    return true;
}

bool UHypeProfileSubsystem::SaveAffiliation(FName Village, FName Clan)
{
    if (!HasCharacter() || Village.IsNone() || Clan != Cla) { return false; }
    UHypeCharacterSave* Save=Cast<UHypeCharacterSave>(UGameplayStatics::CreateSaveGameObject(UHypeCharacterSave::StaticClass()));
    if (!Save) { return false; }
    Save->Vila=Village;
    Save->Cla=Clan;
    Save->Appearance=Appearance;
    if (!UGameplayStatics::SaveGameToSlot(Save,ProfileSlot,0)) { return false; }
    Vila=Village;
    Cla=Clan;
    return true;
}
