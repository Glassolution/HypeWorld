// Tela de carregamento do Hype World (25/10/2026, referência do Felipe: fundo preto, logo branca no centro, barra fina,
// porcentagem, linha de status e contagem "Entrando no jogo em 3... 2... 1...").
// Montada inteira em C++ como o HUD (no editor de widget fica vazia; aparece no jogo). O WBP_LoadingScreen só herda
// desta classe para os ajustes ficarem em Class Defaults (seção Carregamento).
// Fundo (M_UI_LoadingBackground: papel escuro quase invisível + vinheta + pontinhos de chakra subindo) -> logo
// (M_UI_LoadingLogo: revelar da esquerda para a direita com borda de pincel + brilho que "respira") -> barra 320x2 com
// brilho na ponta -> porcentagem -> status (frases trocando com crossfade).
// O mapa seguinte carrega em segundo plano (UHypeLoadingSubsystem) e a barra mostra o progresso real, interpolado.
// Todas as animações usam o tempo real do quadro: a mesma duração a 30 ou 120 FPS.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HypeLoadingScreenWidget.generated.h"

class UImage;
class UTextBlock;
class UCanvasPanelSlot;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTexture2D;

UCLASS()
class HYPEWORLD_API UHypeLoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UHypeLoadingScreenWidget(const FObjectInitializer& ObjectInitializer);

	/** Mapa aberto depois da tela (por enquanto a floresta do tutorial; depois, a criação de personagem) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento") TSoftObjectPtr<UWorld> MapaSeguinte;

	/** Tela aberta quando o mapa seguinte entra (seleção do personagem). Vazio = direto ao jogo */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento") TSoftClassPtr<UUserWidget> TelaAoEntrar;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Visual") TSoftObjectPtr<UTexture2D> Logo;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Visual") TSoftObjectPtr<UMaterialInterface> LogoMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Visual") TSoftObjectPtr<UMaterialInterface> BackgroundMaterial;
	/** Brilho redondo da ponta da barra */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Visual") TSoftObjectPtr<UMaterialInterface> GlowMaterial;
	/** Fonte composta (Montserrat) e a face usada nos textos */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Visual", meta=(AllowedClasses="/Script/Engine.Font")) TSoftObjectPtr<UObject> Font;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Visual") FName FontFace = TEXT("SemiBold");

	/** Largura da logo em fração da largura da tela (em telas ultrawide, conta como se fosse 16:9) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Layout") float LogoWidthFraction = 0.35f;
	/** Centro da logo acima do centro da tela (px de 1080p) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Layout") float LogoRaise = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Layout") FVector2D BarSize = FVector2D(320.f, 2.f);
	/** Espaço entre a logo e a barra, a barra e a porcentagem, a porcentagem e o status */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Layout") float BarGap = 36.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Layout") float PercentGap = 14.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Layout") float StatusGap = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Layout") int32 PercentFontSize = 16;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Layout") int32 StatusFontSize = 12;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Status") TArray<FText> StatusPhrases;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Status") float PhraseInterval = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Status") float CrossfadeTime = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Status") int32 CountdownSeconds = 3;

	/** Tela preta antes da logo, entrada da logo, atraso da barra/textos depois da logo e o fade deles */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Tempo") float StartDelay = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Tempo") float LogoEntryTime = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Tempo") float UiDelay = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Tempo") float UiFadeTime = 0.5f;
	/** A tela fica pelo menos isto (s), mesmo se o mapa carregar antes; a barra leva pelo menos MinFillTime para encher */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Tempo") float MinDisplayTime = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Tempo") float MinFillTime = 1.6f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Tempo") float ExitTime = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Tempo") float FadeInTime = 0.8f;

	/** Brilho da logo "respirando": de 0 até GlowMax, ciclo de GlowCycle s */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Visual") float GlowMax = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento|Visual") float GlowCycle = 2.5f;

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	enum class EPhase : uint8 { Loading, Countdown, Exit, Done };

	void Build();
	void UpdateLayout(const FVector2D& ScreenSize);
	void SetStatus(const FText& Text);
	void TickStatus(float DeltaTime, float UiAlpha);
	void RequestShot(const TCHAR* Name);

	UPROPERTY() TObjectPtr<UImage> Background;
	UPROPERTY() TObjectPtr<UImage> LogoImage;
	UPROPERTY() TObjectPtr<UTexture2D> LogoTexture;
	UPROPERTY() TObjectPtr<UImage> BarTrack;
	UPROPERTY() TObjectPtr<UImage> BarFill;
	UPROPERTY() TObjectPtr<UImage> BarTip;
	UPROPERTY() TObjectPtr<UTextBlock> PercentText;
	UPROPERTY() TObjectPtr<UTextBlock> StatusTexts[2];
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> LogoMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BackgroundMID;
	UPROPERTY() TObjectPtr<UCanvasPanelSlot> LogoSlot;
	UPROPERTY() TObjectPtr<UCanvasPanelSlot> BarTrackSlot;
	UPROPERTY() TObjectPtr<UCanvasPanelSlot> BarFillSlot;
	UPROPERTY() TObjectPtr<UCanvasPanelSlot> BarTipSlot;
	UPROPERTY() TObjectPtr<UCanvasPanelSlot> PercentSlot;
	UPROPERTY() TObjectPtr<UCanvasPanelSlot> StatusSlots[2];

	EPhase Phase = EPhase::Loading;
	float Elapsed = 0.f;
	float PhaseTime = 0.f;
	float Shown = 0.f;			// progresso mostrado (persegue o real suavemente)
	float LogoAspect = 0.55f;	// altura / largura da textura
	float BarTop = 0.f;			// y da barra, relativo ao centro da tela
	int32 PhraseIndex = 0;
	float PhraseTime = 0.f;
	int32 ActiveStatus = 0;
	float StatusFade = 1.f;
	int32 CountdownShown = -1;

	// Medição de FPS durante a tela (vai para o log ao sair)
	int32 Frames = 0;
	float FrameTimeSum = 0.f;
	float WorstFrame = 0.f;

	// -LoadingShots na linha de comando: fotos nos 3 momentos (entrada, ~50%, contagem "1")
	bool bShots = false;
	int32 ShotsTaken = 0;
};
