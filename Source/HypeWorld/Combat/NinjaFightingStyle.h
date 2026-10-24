// Estilos de luta ("poses de batalha" escolhidas no menu de combate): base de luta + combos + finalizadores.
// Cada combo é uma sequência de golpes; o jogo sorteia um combo diferente a cada começo de sequência (variedade).
// Estilos com arma (katana, garras, lança...) só podem ser escolhidos segurando a arma.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NinjaCombatTypes.h"
#include "NinjaFightingStyle.generated.h"

class UAnimSequenceBase;
class UAnimMontage;

/** Arma que um estilo exige (Nenhuma = mão livre). */
UENUM(BlueprintType)
enum class ENinjaWeapon : uint8
{
	None		UMETA(DisplayName="Nenhuma"),
	Katana		UMETA(DisplayName="Katana"),
	Claws		UMETA(DisplayName="Garras"),
	Spear		UMETA(DisplayName="Lança"),
	Kunai		UMETA(DisplayName="Kunai"),
	BigSword	UMETA(DisplayName="Espada grande"),
};

/** Uma sequência de golpes do botão de ataque. */
USTRUCT(BlueprintType)
struct FNinjaComboString
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo")
	TArray<FNinjaAttack> Attacks;
};

/** Montagens de dash por direção (cada lista tem variações; o jogo alterna entre elas). */
USTRUCT(BlueprintType)
struct FNinjaDashSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dash") TArray<TObjectPtr<UAnimMontage>> Front;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dash") TArray<TObjectPtr<UAnimMontage>> Back;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dash") TArray<TObjectPtr<UAnimMontage>> Left;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dash") TArray<TObjectPtr<UAnimMontage>> Right;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dash") TArray<TObjectPtr<UAnimMontage>> Air;
};

UCLASS(BlueprintType)
class HYPEWORLD_API UNinjaFightingStyle : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Estilo")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Estilo")
	FText Description;

	/** Cor do ícone no menu */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Estilo")
	FLinearColor IconColor = FLinearColor(0.95f, 0.55f, 0.2f);

	/** Arma exigida (Nenhuma = qualquer um pode usar) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Estilo")
	ENinjaWeapon RequiredWeapon = ENinjaWeapon::None;

	/** Base de luta (parado em modo de batalha) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Estilo")
	TObjectPtr<UAnimSequenceBase> BattleIdle;

	/** Combos do botão de ataque (um é sorteado a cada sequência nova, sem repetir o anterior) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Golpes")
	TArray<FNinjaComboString> Combos;

	/** Golpe forte: sozinho ou como finalizador de um combo (sorteado) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Golpes")
	TArray<FNinjaAttack> Finishers;

	/** Atacar correndo (sorteado) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Golpes")
	TArray<FNinjaAttack> SprintAttacks;

	/** Atacar no ar (sorteado) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Golpes")
	TArray<FNinjaAttack> AirAttacks;
};
