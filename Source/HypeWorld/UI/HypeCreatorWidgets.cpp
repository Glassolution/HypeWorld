// Peças reutilizáveis da tela de criação (ver o .h).

#include "HypeCreatorWidgets.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"

namespace HypeCreator
{
	const FLinearColor White(1.f, 1.f, 1.f, 1.f);
	const FLinearColor Muted(0.62f, 0.62f, 0.60f, 0.85f);

	FSlateBrush Rounded(const FLinearColor& Fill, float Radius, const FLinearColor& Outline = FLinearColor::Transparent, float OutlineWidth = 0.f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(Outline), OutlineWidth);
		return Brush;
	}

	/** Textura RGBA gerada por código (Color recebe a posição 0..1 e devolve a cor sRGB) */
	UTexture2D* MakeTexture(int32 Width, int32 Height, TFunctionRef<FLinearColor(float U, float V)> Color)
	{
		TArray<FColor> Pixels;
		Pixels.SetNumUninitialized(Width * Height);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				Pixels[Y * Width + X] = Color((X + 0.5f) / Width, (Y + 0.5f) / Height).ToFColor(true);
			}
		}
		const TConstArrayView64<uint8> Bytes(reinterpret_cast<const uint8*>(Pixels.GetData()), Pixels.Num() * sizeof(FColor));
		return UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8, NAME_None, Bytes);
	}

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, FVector2D Position, FVector2D Size, FVector2D Alignment = FVector2D::ZeroVector)
	{
		UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Widget);
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAlignment(Alignment);
		PanelSlot->SetPosition(Position);
		if (Size.IsZero()) { PanelSlot->SetAutoSize(true); }
		else { PanelSlot->SetSize(Size); }
		return PanelSlot;
	}

	float Luminance(const FLinearColor& C) { return 0.2126f * C.R + 0.7152f * C.G + 0.0722f * C.B; }
}

// ---------------------------------------------------------------- base

UHypeCreatorPiece::UHypeCreatorPiece(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Font = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/Ninja/UI/Fontes/F_Montserrat.F_Montserrat")));
}

TSharedRef<SWidget> UHypeCreatorPiece::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget && !IsDesignTime())
	{
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Canvas;
		Build();
	}
	return Super::RebuildWidget();
}

void UHypeCreatorPiece::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const float Target = (GetIsEnabled() && IsHovered()) ? 1.f : 0.f;
	const float Blend = 1.f - FMath::Exp(-18.f * FMath::Min(InDeltaTime, 0.1f));
	if (!FMath::IsNearlyEqual(Hover, Target, 0.001f))
	{
		Hover = FMath::Lerp(Hover, Target, Blend);
		SetRenderScale(FVector2D(1.f + (HoverScale - 1.f) * Hover));
		ApplyHover();
	}
}

FSlateFontInfo UHypeCreatorPiece::FontInfo(const TCHAR* Face, int32 Size)
{
	UObject* FontObject = Font.LoadSynchronous();
	return FontObject ? FSlateFontInfo(FontObject, Size, FName(Face)) : FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size);
}

UButton* UHypeCreatorPiece::MakeFlatButton(float Radius)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style;
	Style.SetNormal(HypeCreator::Rounded(FLinearColor::Transparent, Radius));
	Style.SetHovered(HypeCreator::Rounded(FLinearColor(1.f, 1.f, 1.f, 0.08f), Radius));
	Style.SetPressed(HypeCreator::Rounded(FLinearColor(1.f, 1.f, 1.f, 0.14f), Radius));
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	Button->SetStyle(Style);
	return Button;
}

// ---------------------------------------------------------------- card de opção

void UHypeOptionCardWidget::Build()
{
	using namespace HypeCreator;
	const float W = CardSize.X, H = CardSize.Y;
	Canvas->SetClipping(EWidgetClipping::ClipToBounds);

	Background = WidgetTree->ConstructWidget<UImage>();
	Background->SetBrush(Rounded(FillColor, CornerRadius));
	Place(Canvas, Background, FVector2D::ZeroVector, CardSize);

	// Prévia da peça na metade direita, apagando para a esquerda
	const float PreviewX = W * 0.46f;
	PreviewImage = WidgetTree->ConstructWidget<UImage>();
	PreviewImage->SetRenderOpacity(PreviewOpacity);
	PreviewImage->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, PreviewImage, FVector2D(PreviewX, 0.f), FVector2D(W - PreviewX, H));
	if (!FadeTexture)
	{
		FadeTexture = MakeTexture(64, 1, [](float U, float) { return FLinearColor(0.f, 0.f, 0.f, 1.f - FMath::SmoothStep(0.f, 1.f, U)); });
	}
	PreviewFade = WidgetTree->ConstructWidget<UImage>();
	FSlateBrush FadeBrush;
	FadeBrush.SetResourceObject(FadeTexture);
	PreviewFade->SetBrush(FadeBrush);
	PreviewFade->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, PreviewFade, FVector2D(PreviewX - 1.f, 0.f), FVector2D(110.f, H));

	TitleText = WidgetTree->ConstructWidget<UTextBlock>();
	TitleText->SetFont(FontInfo(TEXT("SemiBold"), 10));
	TitleText->SetColorAndOpacity(FSlateColor(White));
	Place(Canvas, TitleText, FVector2D(16.f, 20.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));

	// Contador "3/12" numa pílula no canto
	UBorder* Pill = WidgetTree->ConstructWidget<UBorder>();
	Pill->SetBrush(Rounded(FLinearColor(1.f, 1.f, 1.f, 0.10f), 5.f));
	Pill->SetPadding(FMargin(8.f, 2.f));
	CounterText = WidgetTree->ConstructWidget<UTextBlock>();
	CounterText->SetFont(FontInfo(TEXT("SemiBold"), 8));
	CounterText->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.85f, 0.82f)));
	Pill->SetContent(CounterText);
	Place(Canvas, Pill, FVector2D(W - 14.f, 20.f), FVector2D::ZeroVector, FVector2D(1.f, 0.5f));

	ValueText = WidgetTree->ConstructWidget<UTextBlock>();
	ValueText->SetFont(FontInfo(TEXT("SemiBold"), 8));
	ValueText->SetColorAndOpacity(FSlateColor(Muted));
	Place(Canvas, ValueText, FVector2D(46.f, 43.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));

	// Trilho entre as setas: a parte cheia e o ponto mostram onde está a opção na lista
	const float TrackY = 58.f, TrackLeft = 46.f, TrackWidth = W - 92.f;
	UImage* Track = WidgetTree->ConstructWidget<UImage>();
	Track->SetBrush(Rounded(FLinearColor(1.f, 1.f, 1.f, 0.16f), 1.f));
	Place(Canvas, Track, FVector2D(TrackLeft, TrackY), FVector2D(TrackWidth, 2.f), FVector2D(0.f, 0.5f));
	TrackFill = WidgetTree->ConstructWidget<UImage>();
	TrackFill->SetBrush(Rounded(FLinearColor(1.f, 1.f, 1.f, 0.55f), 1.f));
	Place(Canvas, TrackFill, FVector2D(TrackLeft, TrackY), FVector2D(0.f, 2.f), FVector2D(0.f, 0.5f))->SetAutoSize(false);
	TrackDot = WidgetTree->ConstructWidget<UImage>();
	TrackDot->SetBrush(Rounded(White, 4.f));
	Place(Canvas, TrackDot, FVector2D(TrackLeft, TrackY), FVector2D(8.f, 8.f), FVector2D(0.5f, 0.5f));

	auto Arrow = [&](const TCHAR* Glyph, float X, FVector2D Alignment, FName Method)
	{
		UButton* Button = MakeFlatButton(6.f);
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Glyph));
		Text->SetFont(FontInfo(TEXT("SemiBold"), 15));
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.8f)));
		Text->SetJustification(ETextJustify::Center);
		Button->AddChild(Text);
		FScriptDelegate Click;
		Click.BindUFunction(this, Method);
		Button->OnClicked.Add(Click);
		Place(Canvas, Button, FVector2D(X, TrackY - 3.f), FVector2D(28.f, 28.f), Alignment);
	};
	Arrow(TEXT("‹"), 12.f, FVector2D(0.f, 0.5f), GET_FUNCTION_NAME_CHECKED(UHypeOptionCardWidget, HandlePrev));
	Arrow(TEXT("›"), W - 12.f, FVector2D(1.f, 0.5f), GET_FUNCTION_NAME_CHECKED(UHypeOptionCardWidget, HandleNext));

	Refresh();
}

void UHypeOptionCardWidget::Setup(const FText& InTitle, int32 InIndex, int32 InCount, const FText& InValueName, UTexture2D* InPreview)
{
	Title = InTitle;
	Index = InIndex;
	Count = InCount;
	ValueName = InValueName;
	Preview = InPreview;
	Refresh();
}

void UHypeOptionCardWidget::Refresh()
{
	if (!TitleText)
	{
		return;
	}
	TitleText->SetText(Title);
	CounterText->SetText(FText::FromString(Count > 0 ? FString::Printf(TEXT("%d/%d"), Index + 1, Count) : TEXT("–")));
	ValueText->SetText(ValueName);
	const bool bHasPreview = Preview != nullptr;
	if (bHasPreview)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Preview);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		PreviewImage->SetBrush(Brush);
	}
	PreviewImage->SetVisibility(bHasPreview ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	PreviewFade->SetVisibility(bHasPreview ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	const float TrackWidth = CardSize.X - 92.f;
	const float T = Count > 1 ? float(Index) / float(Count - 1) : 0.f;
	if (UCanvasPanelSlot* FillSlot = Cast<UCanvasPanelSlot>(TrackFill->Slot)) { FillSlot->SetSize(FVector2D(TrackWidth * T, 2.f)); }
	if (UCanvasPanelSlot* DotSlot = Cast<UCanvasPanelSlot>(TrackDot->Slot)) { DotSlot->SetPosition(FVector2D(46.f + TrackWidth * T, 58.f)); }
}

void UHypeOptionCardWidget::ApplyHover()
{
	if (Background)
	{
		FLinearColor Fill = FillColor;
		Fill.R += 0.05f * Hover;
		Fill.G += 0.05f * Hover;
		Fill.B += 0.05f * Hover;
		Fill.A = FMath::Min(Fill.A + 0.12f * Hover, 1.f);
		Background->SetBrushTintColor(FSlateColor(Fill));
	}
	if (PreviewImage) { PreviewImage->SetRenderOpacity(PreviewOpacity + 0.18f * Hover); }
}

void UHypeOptionCardWidget::HandlePrev() { OnStep.Broadcast(this, -1); }
void UHypeOptionCardWidget::HandleNext() { OnStep.Broadcast(this, 1); }

// ---------------------------------------------------------------- fileira de cores

void UHypeColorRowWidget::Build()
{
	using namespace HypeCreator;
	HoverScale = 1.f;	// cada bolinha reage sozinha (anel), a fileira não cresce
	Rings.Reset();
	Fills.Reset();
	Checks.Reset();
	Buttons.Reset();
	const float Pitch = SwatchSize + SwatchGap;
	const float Y = SwatchSize * 0.5f + 4.f;

	auto Arrow = [&](const TCHAR* Glyph, float X, FName Method)
	{
		UButton* Button = MakeFlatButton(6.f);
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Glyph));
		Text->SetFont(FontInfo(TEXT("SemiBold"), 14));
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.75f)));
		Text->SetJustification(ETextJustify::Center);
		Button->AddChild(Text);
		FScriptDelegate Click;
		Click.BindUFunction(this, Method);
		Button->OnClicked.Add(Click);
		Place(Canvas, Button, FVector2D(X, Y), FVector2D(20.f, 28.f), FVector2D(0.5f, 0.5f));
		return Button;
	};
	PrevButton = Arrow(TEXT("‹"), 10.f, GET_FUNCTION_NAME_CHECKED(UHypeColorRowWidget, HandlePrev));
	const float StartX = 24.f + SwatchSize * 0.5f;
	for (int32 I = 0; I < VisibleCount; ++I)
	{
		const FVector2D Center(StartX + I * Pitch, Y);
		UImage* Ring = WidgetTree->ConstructWidget<UImage>();
		Ring->SetBrush(Rounded(FLinearColor::Transparent, (SwatchSize + 6.f) * 0.5f, White, 1.5f));
		Ring->SetRenderOpacity(0.f);
		Ring->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Canvas, Ring, Center, FVector2D(SwatchSize + 6.f), FVector2D(0.5f, 0.5f));
		UImage* Fill = WidgetTree->ConstructWidget<UImage>();
		Fill->SetBrush(Rounded(White, SwatchSize * 0.5f, FLinearColor(1.f, 1.f, 1.f, 0.12f), 1.f));
		Fill->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Canvas, Fill, Center, FVector2D(SwatchSize), FVector2D(0.5f, 0.5f));
		UTextBlock* Check = WidgetTree->ConstructWidget<UTextBlock>();
		Check->SetText(FText::FromString(TEXT("✓")));
		Check->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10));
		Check->SetVisibility(ESlateVisibility::HitTestInvisible);
		Check->SetRenderOpacity(0.f);
		Place(Canvas, Check, Center, FVector2D::ZeroVector, FVector2D(0.5f, 0.5f));
		UButton* Button = MakeFlatButton(SwatchSize * 0.5f);
		Button->OnClicked.AddDynamic(this, &UHypeColorRowWidget::HandleSwatch);
		Place(Canvas, Button, Center, FVector2D(SwatchSize + 4.f), FVector2D(0.5f, 0.5f));
		Rings.Add(Ring);
		Fills.Add(Fill);
		Checks.Add(Check);
		Buttons.Add(Button);
	}
	NextButton = Arrow(TEXT("›"), StartX + (VisibleCount - 1) * Pitch + SwatchSize * 0.5f + 14.f, GET_FUNCTION_NAME_CHECKED(UHypeColorRowWidget, HandleNext));
	Refresh();
}

void UHypeColorRowWidget::Setup(const TArray<FLinearColor>& InColors, int32 InSelected)
{
	Colors = InColors;
	Selected = InSelected;
	// A escolhida sempre visível
	if (Selected != INDEX_NONE && (Selected < First || Selected >= First + VisibleCount))
	{
		First = FMath::Clamp(Selected - VisibleCount / 2, 0, FMath::Max(Colors.Num() - VisibleCount, 0));
	}
	Refresh();
}

void UHypeColorRowWidget::Refresh()
{
	for (int32 I = 0; I < Fills.Num(); ++I)
	{
		const int32 C = First + I;
		const bool bValid = Colors.IsValidIndex(C);
		const ESlateVisibility Show = bValid ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		Fills[I]->SetVisibility(Show);
		Rings[I]->SetVisibility(Show);
		Checks[I]->SetVisibility(Show);
		Buttons[I]->SetVisibility(bValid ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (!bValid)
		{
			continue;
		}
		Fills[I]->SetColorAndOpacity(Colors[C]);
		const bool bSelected = C == Selected;
		Rings[I]->SetRenderOpacity(bSelected ? 0.9f : 0.f);
		Checks[I]->SetRenderOpacity(bSelected ? 1.f : 0.f);
		Checks[I]->SetColorAndOpacity(FSlateColor(HypeCreator::Luminance(Colors[C]) > 0.45f ? FLinearColor(0.05f, 0.05f, 0.05f) : HypeCreator::White));
	}
	const bool bScroll = Colors.Num() > VisibleCount;
	if (PrevButton) { PrevButton->SetRenderOpacity(bScroll && First > 0 ? 1.f : 0.25f); }
	if (NextButton) { NextButton->SetRenderOpacity(bScroll && First + VisibleCount < Colors.Num() ? 1.f : 0.25f); }
}

void UHypeColorRowWidget::HandleSwatch()
{
	for (int32 I = 0; I < Buttons.Num(); ++I)
	{
		if (Buttons[I] && Buttons[I]->IsHovered() && Colors.IsValidIndex(First + I))
		{
			Selected = First + I;
			Refresh();
			OnColorPicked.Broadcast(this, Selected);
			return;
		}
	}
}

void UHypeColorRowWidget::HandlePrev()
{
	First = FMath::Max(First - 1, 0);
	Refresh();
}

void UHypeColorRowWidget::HandleNext()
{
	First = FMath::Clamp(First + 1, 0, FMath::Max(Colors.Num() - VisibleCount, 0));
	Refresh();
}

// ---------------------------------------------------------------- slider em degradê

void UHypeGradientSliderWidget::Build()
{
	using namespace HypeCreator;
	SetVisibility(ESlateVisibility::Visible);	// recebe o clique em qualquer ponto da barra
	const float H = FMath::Max(ThumbSize, BarSize.Y) + 6.f;
	Bar = WidgetTree->ConstructWidget<UImage>();
	Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Canvas, Bar, FVector2D(0.f, H * 0.5f), BarSize, FVector2D(0.f, 0.5f));
	Thumb = WidgetTree->ConstructWidget<UImage>();
	Thumb->SetBrush(Rounded(White, ThumbSize * 0.5f, FLinearColor(0.f, 0.f, 0.f, 0.55f), 2.f));
	Thumb->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Canvas, Thumb, FVector2D(0.f, H * 0.5f), FVector2D(ThumbSize), FVector2D(0.5f, 0.5f));
	if (GradientTexture)
	{
		FSlateBrush Brush = Rounded(White, BarSize.Y * 0.5f);
		Brush.SetResourceObject(GradientTexture);
		Bar->SetBrush(Brush);
	}
	PlaceThumb();
}

void UHypeGradientSliderWidget::Setup(const TArray<FLinearColor>& InStops, float InValue)
{
	Stops = InStops;
	if (Stops.Num() >= 2)
	{
		const TArray<FLinearColor> Local = Stops;
		GradientTexture = HypeCreator::MakeTexture(256, 1, [Local](float U, float)
		{
			const float P = U * (Local.Num() - 1);
			const int32 Low = FMath::Clamp(FMath::FloorToInt(P), 0, Local.Num() - 2);
			return FMath::Lerp(Local[Low], Local[Low + 1], P - Low);
		});
		if (Bar)
		{
			FSlateBrush Brush = HypeCreator::Rounded(HypeCreator::White, BarSize.Y * 0.5f);
			Brush.SetResourceObject(GradientTexture);
			Bar->SetBrush(Brush);
		}
	}
	SetValue(InValue);
}

void UHypeGradientSliderWidget::SetValue(float InValue)
{
	Value = FMath::Clamp(InValue, 0.f, 1.f);
	PlaceThumb();
}

void UHypeGradientSliderWidget::PlaceThumb()
{
	if (Thumb)
	{
		if (UCanvasPanelSlot* ThumbSlot = Cast<UCanvasPanelSlot>(Thumb->Slot))
		{
			const float H = FMath::Max(ThumbSize, BarSize.Y) + 6.f;
			ThumbSlot->SetPosition(FVector2D(FMath::Lerp(ThumbSize * 0.5f, BarSize.X - ThumbSize * 0.5f, Value), H * 0.5f));
		}
	}
}

void UHypeGradientSliderWidget::SetFromMouse(const FGeometry& Geometry, const FPointerEvent& Event)
{
	const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	const float Usable = FMath::Max(BarSize.X - ThumbSize, 1.f);
	const float NewValue = FMath::Clamp((Local.X - ThumbSize * 0.5f) / Usable, 0.f, 1.f);
	if (!FMath::IsNearlyEqual(NewValue, Value, 0.0005f))
	{
		Value = NewValue;
		PlaceThumb();
		OnValueChanged.Broadcast(this, Value);
	}
}

FReply UHypeGradientSliderWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	bDragging = true;
	SetFromMouse(InGeometry, InMouseEvent);
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UHypeGradientSliderWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging)
	{
		SetFromMouse(InGeometry, InMouseEvent);
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UHypeGradientSliderWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging)
	{
		bDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}
