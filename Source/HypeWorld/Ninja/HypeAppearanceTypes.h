#pragma once
#include "CoreMinimal.h"
#include "HypeAppearanceTypes.generated.h"

namespace HypeIdentity
{
    // Catálogo do jogo, nunca uma escolha enviada pelo menu do jogador.
    inline const TArray<FName>& AvailableClans()
    {
        static const TArray<FName> Available = { FName(TEXT("Uchiha")) };
        return Available;
    }
    inline FName RollClan()
    {
        const TArray<FName>& Available=AvailableClans();
        return Available[FMath::RandHelper(Available.Num())];
    }
    inline FName ResolveClan(FName Saved)
    {
        return AvailableClans().Contains(Saved) ? Saved : RollClan();
    }
}

/** Dados cosméticos; nunca alteram velocidade, cápsula, dano ou esqueleto. */
USTRUCT(BlueprintType)
struct FHypeAppearance
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bCustomized = false;
    /** Apenas primeiro nome. Clã é separado e atribuído pelo jogo. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FString Name;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame) FName Clan;

    FString GetFullName() const { return Clan.IsNone() ? Name : Name + TEXT(" ") + Clan.ToString(); }
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float HeightCm = 190.782f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FLinearColor SkinTint = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FLinearColor HairColor = FLinearColor(0.025f, 0.02f, 0.02f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName HairId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName OutfitId;

    void Sanitize()
    {
        HeightCm = FMath::IsFinite(HeightCm) ? FMath::Clamp(HeightCm, 160.f, 200.f) : 190.782f;
        Name = Name.TrimStartAndEnd();
        int32 FirstNameLength=0;
        while (FirstNameLength<Name.Len() && !FChar::IsWhitespace(Name[FirstNameLength])) { ++FirstNameLength; }
        Name = Name.Left(FMath::Min(FirstNameLength,24));
        auto SafeColor = [](FLinearColor& C)
        {
            C.R = FMath::IsFinite(C.R) ? FMath::Clamp(C.R, 0.f, 1.f) : 1.f;
            C.G = FMath::IsFinite(C.G) ? FMath::Clamp(C.G, 0.f, 1.f) : 1.f;
            C.B = FMath::IsFinite(C.B) ? FMath::Clamp(C.B, 0.f, 1.f) : 1.f;
            C.A = 1.f;
        };
        SafeColor(SkinTint);
        SafeColor(HairColor);
    }
};
