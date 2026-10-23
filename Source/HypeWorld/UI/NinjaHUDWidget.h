// HUD do ninja montado inteiro em C++, seguindo o mockup (Desktop/Hype World/Referencias/HUD/mockup_hud.png):
// retrato + nível + vida + chakra (topo esquerdo), missão, minimapa (topo direito), atalhos (baixo esquerdo)
// e os 4 poderes no centro de baixo.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NinjaHUDWidget.generated.h"

class UCanvasPanel;
class UImage;
class UTextBlock;
class UProgressBar;
class UTexture2D;
class UTextureRenderTarget2D;
class USceneCaptureComponent2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class HYPEWORLD_API UNinjaHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Retrato do personagem (recortado em círculo). */
	UPROPERTY(EditAnywhere, Category="HUD|Imagens")
	UTexture2D* PortraitTexture = nullptr;

	/** Ícones dos 4 poderes, na ordem das teclas Q, E, R, T (recortados em círculo). */
	UPROPERTY(EditAnywhere, Category="HUD|Imagens")
	TArray<UTexture2D*> AbilityIcons;

	/** Material que mostra a câmera do minimapa em círculo (M_UI_Minimapa, parâmetro de textura "Map"). */
	UPROPERTY(EditAnywhere, Category="HUD|Minimapa")
	UMaterialInterface* MinimapMaterial = nullptr;

	/** Quantos metros do mundo cabem no minimapa (em cm). */
	UPROPERTY(EditAnywhere, Category="HUD|Minimapa")
	float MinimapWorldSize = 6000.f;

	/** De quanto em quanto tempo o minimapa é redesenhado (s). Maior = mais leve. */
	UPROPERTY(EditAnywhere, Category="HUD|Minimapa")
	float MinimapUpdateInterval = 0.1f;

	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetHealth(float Current, float Max);

	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetChakra(float Current, float Max);

	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetPlayerLevel(int32 Level);

	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetQuest(const FText& Title, const FText& Step);

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	void BuildLayout();
	void UpdateMinimap(float DeltaTime);

	UPROPERTY() TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY() TObjectPtr<UProgressBar> ChakraBar;
	UPROPERTY() TObjectPtr<UTextBlock> HealthText;
	UPROPERTY() TObjectPtr<UTextBlock> ChakraText;
	UPROPERTY() TObjectPtr<UTextBlock> LevelText;
	UPROPERTY() TObjectPtr<UTextBlock> QuestTitleText;
	UPROPERTY() TObjectPtr<UTextBlock> QuestStepText;
	UPROPERTY() TObjectPtr<UImage> MinimapImage;
	UPROPERTY() TObjectPtr<UImage> MinimapArrow;
	UPROPERTY() TObjectPtr<UTexture2D> FadeTexture;
	UPROPERTY() TObjectPtr<UTexture2D> ArrowTexture;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> MinimapTarget;
	UPROPERTY() TObjectPtr<USceneCaptureComponent2D> MinimapCapture;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MinimapMID;

	float MinimapTimer = 0.f;

	// Valores de exemplo até a Fase 2 ligar vida/chakra/nível ao sistema de atributos
	float Health = 500.f;
	float HealthMax = 500.f;
	float Chakra = 180.f;
	float ChakraMax = 200.f;
	int32 PlayerLevel = 12;
};
