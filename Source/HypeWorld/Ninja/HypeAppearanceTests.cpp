#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "HypeAppearanceTypes.h"
#include "UI/HypeProfileSubsystem.h"
#include "HypeAppearanceComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/GameInstance.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHypeAppearanceSanitizeTest, "HypeWorld.Appearance.InvalidInput", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHypeAppearanceSanitizeTest::RunTest(const FString& Parameters)
{
    FHypeAppearance Value;
    Value.Name=TEXT("  Felipe\n  ");
    Value.HeightCm=std::numeric_limits<float>::quiet_NaN();
    Value.SkinTint=FLinearColor(-2.f,9.f,std::numeric_limits<float>::infinity(),0.f);
    Value.Sanitize();
    TestEqual(TEXT("Nome limpo"),Value.Name,FString(TEXT("Felipe")));
    TestTrue(TEXT("Altura finita"),FMath::IsFinite(Value.HeightCm));
    TestEqual(TEXT("Cor limitada"),Value.SkinTint,FLinearColor(0,1,1,1));
    Value.HeightCm=900.f;Value.Sanitize();TestEqual(TEXT("Altura maxima"),Value.HeightCm,200.f);
    Value.HeightCm=-900.f;Value.Sanitize();TestEqual(TEXT("Altura minima"),Value.HeightCm,160.f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHypeAppearanceSaveTest, "HypeWorld.Appearance.SaveRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHypeAppearanceSaveTest::RunTest(const FString& Parameters)
{
    UHypeCharacterSave* Original=NewObject<UHypeCharacterSave>();
    Original->Vila=TEXT("Konoha");Original->Cla=TEXT("Hyuga");
    Original->Appearance.bCustomized=true;Original->Appearance.Name=TEXT("Felipe");Original->Appearance.Clan=TEXT("Uchiha");
    Original->Appearance.HeightCm=175.f;Original->Appearance.SkinTint=FLinearColor(.5f,.3f,.2f);
    Original->Appearance.HairId=TEXT("TesteCabelo");Original->Appearance.OutfitId=TEXT("TesteRoupa");
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Serializa perfil"),UGameplayStatics::SaveGameToMemory(Original,Bytes))) { return false; }
    UHypeCharacterSave* Loaded=Cast<UHypeCharacterSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Recupera perfil"),Loaded)) { return false; }
    TestEqual(TEXT("Cla"),Loaded->Cla,Original->Cla);
    TestEqual(TEXT("Nome"),Loaded->Appearance.Name,Original->Appearance.Name);
    TestEqual(TEXT("Clã da identidade"),Loaded->Appearance.Clan,Original->Appearance.Clan);
    TestEqual(TEXT("Nome completo automático"),Loaded->Appearance.GetFullName(),FString(TEXT("Felipe Uchiha")));
    TestEqual(TEXT("Altura"),Loaded->Appearance.HeightCm,Original->Appearance.HeightCm);
    TestEqual(TEXT("Pele"),Loaded->Appearance.SkinTint,Original->Appearance.SkinTint);
    TestEqual(TEXT("Cabelo"),Loaded->Appearance.HairId,Original->Appearance.HairId);
    TestEqual(TEXT("Roupa"),Loaded->Appearance.OutfitId,Original->Appearance.OutfitId);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHypeAppearanceAssetsTest, "HypeWorld.Appearance.CompatibleAssets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHypeAppearanceAssetsTest::RunTest(const FString& Parameters)
{
    UHypeAppearanceComponent* Component=NewObject<UHypeAppearanceComponent>();
    USkeletalMesh* Indra=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/HypeWorld/Personagens/Indra/SK_Indra.SK_Indra"));
    if (!TestNotNull(TEXT("Indra"),Indra) || !TestNotNull(TEXT("Corpo base"),Component->BaseBody.Get())) { return false; }
    TestTrue(TEXT("Mesmo esqueleto"),Indra->GetSkeleton()==Component->BaseBody->GetSkeleton());
    TestNotNull(TEXT("Material de pele"),Component->SkinMaterial.Get());
    TestTrue(TEXT("Componente replicado"),Component->GetIsReplicated());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHypeCharacterIdentityTest, "HypeWorld.Appearance.CharacterWithoutAffiliation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHypeCharacterIdentityTest::RunTest(const FString& Parameters)
{
    UGameInstance* GI=NewObject<UGameInstance>();
    UHypeProfileSubsystem* Profile=NewObject<UHypeProfileSubsystem>(GI);
    TestFalse(TEXT("Perfil vazio não tem personagem"),Profile->HasCharacter());
    Profile->Appearance.bCustomized=true;
    Profile->Appearance.Name=TEXT("Felipe");
    TestTrue(TEXT("Criação independe de vila e clã"),Profile->HasCharacter());
    TestTrue(TEXT("Sem vila atribuída automaticamente"),Profile->Vila.IsNone());
    TestTrue(TEXT("Sem clã atribuído automaticamente"),Profile->Cla.IsNone());
    Profile->Appearance=FHypeAppearance();
    Profile->Vila=TEXT("Konoha");Profile->Cla=TEXT("Uchiha");
    TestTrue(TEXT("Save anterior continua válido"),Profile->HasCharacter());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHypeFirstNameTest, "HypeWorld.Appearance.FirstNameAndAssignedClan", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHypeFirstNameTest::RunTest(const FString& Parameters)
{
    FHypeAppearance Value;
    Value.Name=TEXT("  Álvaro Silva  ");Value.Sanitize();
    TestEqual(TEXT("Preserva acento e remove sobrenome"),Value.Name,FString(TEXT("Álvaro")));
    Value.Name=TEXT("Felipe\tXavier");Value.Sanitize();
    TestEqual(TEXT("Tabulação não junta sobrenome"),Value.Name,FString(TEXT("Felipe")));
    Value.Name=TEXT("Felipe\nXavier");Value.Sanitize();
    TestEqual(TEXT("Quebra de linha não junta sobrenome"),Value.Name,FString(TEXT("Felipe")));
    TestEqual(TEXT("Clã novo recebe único disponível"),HypeIdentity::ResolveClan(NAME_None),FName(TEXT("Uchiha")));
    TestEqual(TEXT("Clã não disponível é rejeitado"),HypeIdentity::ResolveClan(TEXT("Hyuga")),FName(TEXT("Uchiha")));
    TestEqual(TEXT("Clã válido é preservado"),HypeIdentity::ResolveClan(TEXT("Uchiha")),FName(TEXT("Uchiha")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHypeUchihaOutfitTest,"HypeWorld.Appearance.UchihaOutfitRestriction",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHypeUchihaOutfitTest::RunTest(const FString& Parameters)
{
    UHypeAppearanceComponent* C=NewObject<UHypeAppearanceComponent>();
    TestTrue(TEXT("Cabelos importados"),C->HairOptions.Num()>=40);
    TestEqual(TEXT("Quatro roupas"),C->OutfitOptions.Num(),4);
    for (const FHypeCosmeticPart& P:C->OutfitOptions)
    {
        TestTrue(TEXT("Uchiha permitido"),P.AllowsClan(TEXT("Uchiha")));
        TestFalse(TEXT("Hyuga bloqueado"),P.AllowsClan(TEXT("Hyuga")));
        TestFalse(TEXT("Sem clã bloqueado"),P.AllowsClan(NAME_None));
        TestNotNull(TEXT("Malha de roupa"),P.Mesh.Get());
    }
    return true;
}
#endif
