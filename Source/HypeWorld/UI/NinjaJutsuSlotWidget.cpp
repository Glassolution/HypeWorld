#include "NinjaJutsuSlotWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace NinjaJutsuSlot
{
	const float SelectTime = 0.12f;			// transição do selecionado (ease-out)
	const float UsedPulseTime = 0.18f;		// pulso ao usar
	const float ReadyPulseTime = 0.4f;		// pulso de brilho quando o cooldown termina
	// Medidas do recorte (Tools/hud/slots_jutsu_layout.json): caixa quadrada de 460 px da imagem original, círculo da arte
	// com raio 121 e centro em (230, 200), centro do número na etiqueta em (~230, 378)
	const FVector2D DiscCenter(0.5f, 200.f / 460.f);
	const float DiscRadius = 121.f / 460.f;
	const FVector2D NumberCenter(0.5f, 378.f / 460.f);
	const TCHAR* EmptyArt = TEXT("/Game/Ninja/UI/HUD/Jutsu/Slots/T_JS_Slot1.T_JS_Slot1");	// moldura usada no slot vazio (fica cinza)
}

void UNinjaJutsuSlotWidget::Setup(const FNinjaJutsuSlotData& InData, float InSize, const FSlateFontInfo& InNumberFont, const FSlateFontInfo& InCooldownFont)
{
	Data = InData;
	Size = InSize;
	NumberFont = InNumberFont;
	CooldownFont = InCooldownFont;
}

FVector2D UNinjaJutsuSlotWidget::BoxSizeFor(float DiscSize)
{
	return FVector2D(DiscSize / (2.f * NinjaJutsuSlot::DiscRadius));
}

FVector2D UNinjaJutsuSlotWidget::DiscAlignment()
{
	return NinjaJutsuSlot::DiscCenter;
}

TSharedRef<SWidget> UNinjaJutsuSlotWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget && !IsDesignTime())
	{
		Build();
	}
	return Super::RebuildWidget();
}

void UNinjaJutsuSlotWidget::Build()
{
	using namespace NinjaJutsuSlot;
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;
	auto Put = [Root](UWidget* Widget, const FVector2D& CenterAt, const FVector2D& WidgetSize)
	{
		UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Widget);
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetPosition(CenterAt);
		if (WidgetSize.IsZero())
		{
			PanelSlot->SetAutoSize(true);
		}
		else
		{
			PanelSlot->SetAutoSize(false);
			PanelSlot->SetSize(WidgetSize);
		}
		return PanelSlot;
	};
	const FVector2D Box = BoxSizeFor(Size);
	const FVector2D Disc = Box * DiscCenter;
	const bool bEmpty = Data.Icon == nullptr;

	// 1) Brilho na cor do jutsu, atrás da moldura (o material mostra o anel de fora, sumindo suave)
	Glow = WidgetTree->ConstructWidget<UImage>();
	if (UMaterialInterface* GlowMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ninja/UI/HUD/Jutsu/M_UI_JutsuGlow.M_UI_JutsuGlow")))
	{
		GlowMID = UMaterialInstanceDynamic::Create(GlowMat, this);
		GlowMID->SetVectorParameterValue(TEXT("Color"), Data.ThemeColor);
		Glow->SetBrushFromMaterial(GlowMID);
	}
	GlowSlot = Put(Glow, Disc, FVector2D(Size * 2.f));

	// 2) Arte do slot (moldura + arte + etiqueta), estados só dentro do círculo
	Art = WidgetTree->ConstructWidget<UImage>();
	if (UMaterialInterface* ArtMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ninja/UI/HUD/Jutsu/M_UI_JutsuSlotArt.M_UI_JutsuSlotArt")))
	{
		ArtMID = UMaterialInstanceDynamic::Create(ArtMat, this);
		UTexture2D* Texture = bEmpty ? LoadObject<UTexture2D>(nullptr, EmptyArt) : Data.Icon.Get();
		if (Texture)
		{
			ArtMID->SetTextureParameterValue(TEXT("Slot"), Texture);
		}
		ArtMID->SetVectorParameterValue(TEXT("DiscCenter"), FLinearColor(DiscCenter.X, DiscCenter.Y, 0.f, 0.f));
		ArtMID->SetScalarParameterValue(TEXT("DiscRadius"), DiscRadius);
		ArtMID->SetScalarParameterValue(TEXT("Empty"), bEmpty ? 1.f : 0.f);
		Art->SetBrushFromMaterial(ArtMID);
	}
	Put(Art, Box * 0.5f, Box);

	// 3) Número na etiqueta preta embaixo
	NumberText = WidgetTree->ConstructWidget<UTextBlock>();
	NumberText->SetFont(NumberFont);
	NumberText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	NumberText->SetJustification(ETextJustify::Center);
	NumberText->SetText(FText::AsNumber(Data.Number));
	Put(NumberText, Box * NumberCenter, FVector2D::ZeroVector);
	NumberText->SetVisibility(bEmpty ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	// 4) Segundos do cooldown no meio do círculo (só durante o cooldown)
	CooldownText = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo CdFont = CooldownFont;
	CdFont.OutlineSettings.OutlineSize = 2;
	CdFont.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.85f);
	CooldownText->SetFont(CdFont);
	CooldownText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CooldownText->SetJustification(ETextJustify::Center);
	Put(CooldownText, Disc, FVector2D::ZeroVector);
	CooldownText->SetVisibility(ESlateVisibility::Collapsed);

	SetRenderTransformPivot(DiscCenter);
}

void UNinjaJutsuSlotWidget::SetCooldown(float Remaining, float Duration)
{
	CooldownLeft = FMath::Max(Remaining, 0.f);
	CooldownDuration = FMath::Max(Duration, CooldownLeft);
}

void UNinjaJutsuSlotWidget::SetNumber(int32 Number)
{
	Data.Number = Number;
	if (NumberText)
	{
		NumberText->SetText(FText::AsNumber(Number));
	}
}

void UNinjaJutsuSlotWidget::TickSlot(float DeltaTime)
{
	using namespace NinjaJutsuSlot;
	SelectAlpha = FMath::FInterpConstantTo(SelectAlpha, bSelected ? 1.f : 0.f, DeltaTime, 1.f / SelectTime);
	HoverAlpha = FMath::FInterpConstantTo(HoverAlpha, IsHovered() ? 1.f : 0.f, DeltaTime, 1.f / SelectTime);
	const float Sel = 1.f - FMath::Square(1.f - SelectAlpha);   // ease-out
	const float Hov = 1.f - FMath::Square(1.f - HoverAlpha);
	float Scale = (1.f + 0.08f * Sel) * (1.f + 0.05f * Hov * (1.f - Sel));

	// Usou: pulso 1 -> 1,15 -> 1 e flash branco
	float Flash = 0.f;
	if (UsedTime >= 0.f)
	{
		UsedTime += DeltaTime;
		const float P = UsedTime / UsedPulseTime;
		if (P >= 1.f)
		{
			UsedTime = -1.f;
		}
		else
		{
			Scale *= 1.f + 0.15f * FMath::Sin(PI * P);
			Flash = 1.f - P;
		}
	}

	// Cooldown: ao terminar, pulso de brilho na cor do jutsu
	if (CooldownLeft > 0.f)
	{
		CooldownLeft -= DeltaTime;
		if (CooldownLeft <= 0.f)
		{
			CooldownLeft = 0.f;
			ReadyTime = 0.f;
		}
	}
	const float CooldownFrac = CooldownDuration > 0.f ? CooldownLeft / CooldownDuration : 0.f;
	float Ready = 0.f;
	if (ReadyTime >= 0.f)
	{
		ReadyTime += DeltaTime;
		const float Q = ReadyTime / ReadyPulseTime;
		if (Q >= 1.f)
		{
			ReadyTime = -1.f;
		}
		else
		{
			Ready = FMath::Sin(PI * Q);
			Scale *= 1.f + 0.05f * Ready;
		}
	}

	const bool bEmpty = Data.Icon == nullptr;
	const bool bDimmed = bEmpty || CooldownFrac > 0.f || !bHasChakra;
	const float GlowIntensity = bDimmed ? 0.f : FMath::Lerp(0.45f, 0.85f, Sel) + 0.1f * Hov + 0.6f * Ready;
	if (GlowMID)
	{
		GlowMID->SetScalarParameterValue(TEXT("Intensity"), GlowIntensity);
	}
	if (Glow)
	{
		Glow->SetVisibility(GlowIntensity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (GlowSlot)
	{
		GlowSlot->SetSize(FVector2D(Size * (FMath::Lerp(2.f, 2.2f, Sel) + 0.2f * Ready)));
	}
	if (ArtMID)
	{
		ArtMID->SetScalarParameterValue(TEXT("Selected"), Sel);
		ArtMID->SetScalarParameterValue(TEXT("Cooldown"), CooldownFrac);
		ArtMID->SetScalarParameterValue(TEXT("NoChakra"), bHasChakra || bEmpty ? 0.f : 1.f);
		ArtMID->SetScalarParameterValue(TEXT("Flash"), Flash);
	}
	const int32 Seconds = CooldownLeft > 0.f ? FMath::CeilToInt(CooldownLeft) : 0;
	if (CooldownText && Seconds != ShownCooldownSeconds)
	{
		ShownCooldownSeconds = Seconds;
		CooldownText->SetText(FText::AsNumber(Seconds));
		CooldownText->SetVisibility(Seconds > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	SetRenderScale(FVector2D(Scale, Scale));
}
