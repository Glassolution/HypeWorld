// Peças reutilizáveis da tela de criação de personagem (30/10/2026, referência do Felipe: tela do Zenkai).
// Montadas em C++ como o resto da interface; WBP_OptionCard / WBP_ColorRow / WBP_GradientSlider só herdam daqui
// (Tools/editor/criar_tela_criacao.py) para os ajustes ficarem em Class Defaults. Medidas em px de 1080p.
//   - UHypeOptionCardWidget: card escuro arredondado com título, contador, setas ‹ › e a prévia da peça esmaecida.
//   - UHypeColorRowWidget: fileira de bolinhas de cor com check na escolhida e setas para ver mais cores.
//   - UHypeGradientSliderWidget: barra em degradê (tom de pele) com um marcador que se arrasta.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HypeCreatorWidgets.generated.h"

class UButton;
class UCanvasPanel;
class UImage;
class UTextBlock;
class UTexture2D;
class UHypeOptionCardWidget;
class UHypeColorRowWidget;
class UHypeGradientSliderWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHypeOptionStepEvent, UHypeOptionCardWidget*, Card, int32, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHypeColorPickedEvent, UHypeColorRowWidget*, Row, int32, Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHypeSliderChangedEvent, UHypeGradientSliderWidget*, Slider, float, Value);

/** Base: fonte e o "respiro" do mouse em cima (clareia e cresce 2%) */
UCLASS(Abstract)
class HYPEWORLD_API UHypeCreatorPiece : public UUserWidget
{
	GENERATED_BODY()

public:

	UHypeCreatorPiece(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Criacao|Visual", meta=(AllowedClasses="/Script/Engine.Font")) TSoftObjectPtr<UObject> Font;
	/** Escala com o mouse em cima (1 = sem crescer) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Criacao|Visual") float HoverScale = 1.02f;

	/** Identificador livre para quem usa a peça (ex.: o slot da roupa) */
	FName Key;

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void Build() {}
	/** 0..1 suavizado: quanto o mouse está em cima */
	float Hover = 0.f;
	virtual void ApplyHover() {}
	FSlateFontInfo FontInfo(const TCHAR* Face, int32 Size);
	UButton* MakeFlatButton(float Radius);

	UPROPERTY() TObjectPtr<UCanvasPanel> Canvas;
};

UCLASS()
class HYPEWORLD_API UHypeOptionCardWidget : public UHypeCreatorPiece
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Criacao|Layout") FVector2D CardSize = FVector2D(380.f, 76.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Criacao|Layout") float CornerRadius = 8.f;
	/** Fundo preto 60% e quanto a prévia aparece */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Criacao|Visual") FLinearColor FillColor = FLinearColor(0.f, 0.f, 0.f, 0.6f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Criacao|Visual") float PreviewOpacity = 0.42f;

	/** Título, opção atual (0 = primeira), total, nome da opção e prévia (pode ser nula) */
	UFUNCTION(BlueprintCallable, Category="Criacao")
	void Setup(const FText& InTitle, int32 InIndex, int32 InCount, const FText& InValueName, UTexture2D* InPreview);

	UPROPERTY(BlueprintAssignable, Category="Criacao") FHypeOptionStepEvent OnStep;

protected:

	virtual void Build() override;
	virtual void ApplyHover() override;

private:

	UFUNCTION() void HandlePrev();
	UFUNCTION() void HandleNext();
	void Refresh();

	FText Title;
	FText ValueName;
	int32 Index = 0;
	int32 Count = 0;
	UPROPERTY() TObjectPtr<UTexture2D> Preview;
	UPROPERTY() TObjectPtr<UImage> Background;
	UPROPERTY() TObjectPtr<UImage> PreviewImage;
	UPROPERTY() TObjectPtr<UImage> PreviewFade;
	UPROPERTY() TObjectPtr<UTextBlock> TitleText;
	UPROPERTY() TObjectPtr<UTextBlock> CounterText;
	UPROPERTY() TObjectPtr<UTextBlock> ValueText;
	UPROPERTY() TObjectPtr<UImage> TrackFill;
	UPROPERTY() TObjectPtr<UImage> TrackDot;
	UPROPERTY() TObjectPtr<UTexture2D> FadeTexture;
};

UCLASS()
class HYPEWORLD_API UHypeColorRowWidget : public UHypeCreatorPiece
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Criacao|Layout") float SwatchSize = 28.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Criacao|Layout") float SwatchGap = 9.f;
	/** Quantas bolinhas cabem de uma vez (o resto aparece pelas setas) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Criacao|Layout") int32 VisibleCount = 8;

	UFUNCTION(BlueprintCallable, Category="Criacao")
	void Setup(const TArray<FLinearColor>& InColors, int32 InSelected);
	int32 GetSelected() const { return Selected; }

	UPROPERTY(BlueprintAssignable, Category="Criacao") FHypeColorPickedEvent OnColorPicked;

protected:

	virtual void Build() override;

private:

	UFUNCTION() void HandleSwatch();
	UFUNCTION() void HandlePrev();
	UFUNCTION() void HandleNext();
	void Refresh();

	TArray<FLinearColor> Colors;
	int32 Selected = INDEX_NONE;
	int32 First = 0;
	UPROPERTY() TArray<TObjectPtr<UImage>> Rings;
	UPROPERTY() TArray<TObjectPtr<UImage>> Fills;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> Checks;
	UPROPERTY() TArray<TObjectPtr<UButton>> Buttons;
	UPROPERTY() TObjectPtr<UButton> PrevButton;
	UPROPERTY() TObjectPtr<UButton> NextButton;
};

UCLASS()
class HYPEWORLD_API UHypeGradientSliderWidget : public UHypeCreatorPiece
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Criacao|Layout") FVector2D BarSize = FVector2D(380.f, 10.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Criacao|Layout") float ThumbSize = 16.f;

	/** Cores do degradê da esquerda para a direita e o valor inicial (0..1) */
	UFUNCTION(BlueprintCallable, Category="Criacao")
	void Setup(const TArray<FLinearColor>& InStops, float InValue);
	UFUNCTION(BlueprintCallable, Category="Criacao")
	void SetValue(float InValue);
	float GetValue() const { return Value; }

	UPROPERTY(BlueprintAssignable, Category="Criacao") FHypeSliderChangedEvent OnValueChanged;

protected:

	virtual void Build() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:

	void SetFromMouse(const FGeometry& Geometry, const FPointerEvent& Event);
	void PlaceThumb();

	TArray<FLinearColor> Stops;
	float Value = 0.f;
	bool bDragging = false;
	UPROPERTY() TObjectPtr<UImage> Bar;
	UPROPERTY() TObjectPtr<UImage> Thumb;
	UPROPERTY() TObjectPtr<UTexture2D> GradientTexture;
};
