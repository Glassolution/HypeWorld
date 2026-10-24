// HUD do ninja montado inteiro em C++, limpo, no modelo da referência do Felipe (24/10/2026):
// canto de baixo à esquerda = retrato redondo com anel + nível, nome, barra de vida "750/750", barra de chakra com % e
// moedas; centro de baixo = slots de jutsu, abaixados (só a pontinha aparece) e que sobem no modo de batalha.
// Sem minimapa, sem missão, sem lista de atalhos.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NinjaHUDWidget.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;
class UImage;
class UTextBlock;
class UProgressBar;
class UTexture2D;
class UWidget;

UCLASS()
class HYPEWORLD_API UNinjaHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Retrato do personagem (recortado em círculo). */
	UPROPERTY(EditAnywhere, Category="HUD|Imagens")
	UTexture2D* PortraitTexture = nullptr;

	/** Ícones dos jutsus nos slots (recortados em círculo); slots sem ícone ficam vazios. */
	UPROPERTY(EditAnywhere, Category="HUD|Imagens")
	TArray<UTexture2D*> AbilityIcons;

	/** Experiência do nível atual (0..1) — o arco azul em volta do retrato. Placeholder até existir XP. */
	UPROPERTY(EditAnywhere, Category="HUD")
	float ExperiencePercent = 0.75f;

	/** Fonte do bloco de vida: a Montserrat (fonte composta com as faces "ExtraBold", "Bold" e "SemiBold").
	 *  Vazio = Roboto da engine, só como substituta temporária até a Montserrat ser importada. */
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	UObject* HudFont = nullptr;

	/** O bloco de vida é montado em 548x254 (medidas da referência) e escalado inteiro: posição do canto de cima à esquerda
	 *  (a partir do canto de baixo à esquerda da tela, em px de 1920x1080) e escala */
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	FVector2D LifeBlockPosition = FVector2D(25.f, -215.f);
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	float LifeBlockScale = 0.77f;

	/** Moedas (placeholder até existir dinheiro) */
	UPROPERTY(EditAnywhere, Category="HUD")
	int32 Coins = 0;

	/** Quantos slots de jutsu a barra tem */
	UPROPERTY(EditAnywhere, Category="HUD|Jutsus")
	int32 JutsuSlotCount = 5;

	/** Quanto os slots descem fora do modo de batalha (px; o slot tem 62 — sobra só a pontinha) */
	UPROPERTY(EditAnywhere, Category="HUD|Jutsus")
	float JutsuHiddenOffset = 78.f;

	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetHealth(float Current, float Max);

	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetChakra(float Current, float Max);

	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetPlayerLevel(int32 Level);

	/** Mantido por compatibilidade (o HUD novo não mostra missão) */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetQuest(const FText& Title, const FText& Step) {}

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	void BuildLayout();
	void BuildLifeBlock(UCanvasPanel* Root);
	FSlateFontInfo HudFontInfo(const TCHAR* Face, float SizePx) const;

	UPROPERTY() TObjectPtr<UTextBlock> HealthText;
	UPROPERTY() TObjectPtr<UTextBlock> HealthMaxText;
	UPROPERTY() TObjectPtr<UTextBlock> ChakraText;
	UPROPERTY() TObjectPtr<UTextBlock> LevelText;
	UPROPERTY() TObjectPtr<UTextBlock> NameText;
	UPROPERTY() TObjectPtr<UTextBlock> CoinText;
	UPROPERTY() TObjectPtr<UImage> HealthFill;
	UPROPERTY() TObjectPtr<UImage> ChakraFill;
	UPROPERTY() TObjectPtr<UImage> SwirlIcon;
	UPROPERTY() TObjectPtr<UImage> XpRing;
	UPROPERTY() TObjectPtr<UTexture2D> HealthTexture;
	UPROPERTY() TObjectPtr<UTexture2D> ChakraTexture;
	UPROPERTY() TObjectPtr<UTexture2D> RingTexture;
	UPROPERTY() TObjectPtr<UTexture2D> MicTexture;
	UPROPERTY() TObjectPtr<UTexture2D> HeadsetTexture;
	UPROPERTY() TObjectPtr<UTexture2D> CoinTexture;
	float ShownXp = -1.f;
	int32 ShownCoins = -1;
	static FString FormatThousands(int32 Value);
	/** Barras: a imagem de preenchimento é recortada (não esticada) conforme a porcentagem */
	void SetFill(UImage* Fill, UTexture2D* Texture, float Percent, float Width, const FLinearColor& Tint);
	void UpdateXpRing();
	UPROPERTY() TObjectPtr<UTextBlock> ComboText;
	UPROPERTY() TObjectPtr<UProgressBar> SuperJumpBar;
	UPROPERTY() TObjectPtr<UTextBlock> SuperJumpText;
	UPROPERTY() TArray<TObjectPtr<UImage>> SubstitutionPips;
	UPROPERTY() TObjectPtr<UTexture2D> SwirlTexture;

	/** Slots de jutsu (e as cargas da substituição): sobem e descem juntos com o modo de batalha */
	UPROPERTY() TArray<TObjectPtr<UCanvasPanelSlot>> JutsuSlots;
	TArray<FVector2D> JutsuSlotBase;
	float JutsuOffset = 78.f;

	float Health = -1.f;
	float HealthMax = -1.f;
	float Chakra = -1.f;
	float ChakraMax = -1.f;
	int32 PlayerLevel = 12;
	FString ShownName;
	int32 ShownInfusion = -1;
	int32 ShownSubstitutions = -1;
};
