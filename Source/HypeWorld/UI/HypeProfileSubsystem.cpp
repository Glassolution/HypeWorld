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

bool UHypeProfileSubsystem::SaveCharacterAppearance(const FHypeAppearance& NewAppearance)
{
    FHypeAppearance Validated = NewAppearance;
    Validated.Sanitize();
    if (!Validated.bCustomized || Validated.Name.Len() < 2) { return false; }

    const bool bExisting = HasCharacter();
    const FName Clan = bExisting ? Cla : HypeIdentity::RollClan();
    Validated.Clan = Clan;
    return WriteProfile(bExisting ? Vila : NAME_None, Clan, Validated);
}

bool UHypeProfileSubsystem::SaveAffiliation(FName Village, FName Clan)
{
    if (!HasCharacter() || Village.IsNone() || Clan != Cla) { return false; }
    return WriteProfile(Village, Clan, Appearance);
}

bool UHypeProfileSubsystem::WriteProfile(FName Village, FName Clan, const FHypeAppearance& NewAppearance)
{
    UHypeCharacterSave* Save = Cast<UHypeCharacterSave>(UGameplayStatics::CreateSaveGameObject(UHypeCharacterSave::StaticClass()));
    if (!Save) { return false; }
    Save->Vila = Village;
    Save->Cla = Clan;
    Save->Appearance = NewAppearance;
    if (!UGameplayStatics::SaveGameToSlot(Save, ProfileSlot, 0))
    {
        UE_LOG(LogTemp, Warning, TEXT("Falha de gravação do perfil local no slot %s"), ProfileSlot);
        return false;
    }

    Vila = Save->Vila;
    Cla = Save->Cla;
    Appearance = Save->Appearance;
    return true;
}
