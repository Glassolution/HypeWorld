#include "NinjaJutsuSlotWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace NinjaJutsuSlot
{
	const float SelectTime = 0.12f;			// transição do selecionado (ease-out)
	const float UsedPulseTime = 0.18f;		// pulso ao usar
	const float ReadyPulseTime = 0.4f;		// pulso de brilho quando o cooldown termina
	const float SelectedScale = 1.1f;		// selecionado fica um pouco maior (como na referência)
	// Caixa do widget = BoxScale x diâmetro do slot (folga para o brilho e para a pílula do custo); círculo no meio
	const float BoxScale = 1.6f;
	const float GlowScale = 1.45f;			// imagem do brilho: o aro cobre r < 0,69 dela, o brilho é o anel de fora
	// Pílula do custo: centro abaixo do centro do círculo (fração do diâmetro), altura = largura mínima
	const float PillCenterY = 0.46f;
	const float PillHeight = 0.31f;
	const FLinearColor SelectBlue(0.18f, 0.54f, 1.f);	// brilho de seleção azul (o material clareia a cor)
	// Círculo da arte dentro do recorte dos slots (Tools/hud/slots_jutsu_layout.json): caixa de 460 px, raio 121, centro (230, 200)
	const FVector2D ArtCenter(0.5f, 200.f / 460.f);
	const float ArtRadius = 121.f / 460.f;
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
	return FVector2D(DiscSize * NinjaJutsuSlot::BoxScale);
}

FVector2D UNinjaJutsuSlotWidget::DiscAlignment()
{
	return FVector2D(0.5f, 0.5f);
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
	const FVector2D Disc = Box * 0.5f;
	const bool bEmpty = Data.Icon == nullptr;

	// 1) Brilho azul de seleção, atrás do aro (só aparece selecionado / ao ficar pronto)
	Glow = WidgetTree->ConstructWidget<UImage>();
	if (UMaterialInterface* GlowMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ninja/UI/HUD/Jutsu/M_UI_JutsuGlow.M_UI_JutsuGlow")))
	{
		GlowMID = UMaterialInstanceDynamic::Create(GlowMat, this);
		GlowMID->SetVectorParameterValue(TEXT("Color"), SelectBlue);
		Glow->SetBrushFromMaterial(GlowMID);
	}
	GlowSlot = Put(Glow, Disc, FVector2D(Size * GlowScale));
	Glow->SetVisibility(ESlateVisibility::Collapsed);

	// 2) Aro escuro de pincel + arte do jutsu (estados aplicados na arte)
	Art = WidgetTree->ConstructWidget<UImage>();
	if (UMaterialInterface* ArtMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ninja/UI/HUD/Jutsu/M_UI_JutsuSlotRing.M_UI_JutsuSlotRing")))
	{
		ArtMID = UMaterialInstanceDynamic::Create(ArtMat, this);
		ApplyArt();
		Art->SetBrushFromMaterial(ArtMID);
	}
	Put(Art, Disc, FVector2D(Size));

	// 3) Segundos do cooldown no meio do círculo ("27.1s"), branco e negrito com contorno escuro
	CooldownText = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo CdFont = CooldownFont;
	CdFont.OutlineSettings.OutlineSize = 1;
	CdFont.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.7f);
	CooldownText->SetFont(CdFont);
	CooldownText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CooldownText->SetShadowOffset(FVector2D(0.f, 1.5f));
	CooldownText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	CooldownText->SetJustification(ETextJustify::Center);
	Put(CooldownText, Disc, FVector2D::ZeroVector);
	CooldownText->SetVisibility(ESlateVisibility::Collapsed);

	// 4) Custo de chakra numa pílula preta por cima da borda de baixo do aro (1 dígito = círculo, mais dígitos = pílula)
	const float PillH = Size * PillHeight;
	UBorder* Pill = WidgetTree->ConstructWidget<UBorder>();
	FSlateBrush PillBrush;
	PillBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	PillBrush.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.96f));
	PillBrush.OutlineSettings = FSlateBrushOutlineSettings(0.f, FSlateColor(FLinearColor(0.011f, 0.021f, 0.020f, 1.f)), 1.f);
	PillBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
	Pill->SetBrush(PillBrush);
	Pill->SetPadding(FMargin(PillH * 0.24f, 0.f));
	Pill->SetHorizontalAlignment(HAlign_Center);
	Pill->SetVerticalAlignment(VAlign_Center);
	NumberText = WidgetTree->ConstructWidget<UTextBlock>();
	NumberText->SetFont(NumberFont);
	NumberText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	NumberText->SetJustification(ETextJustify::Center);
	NumberText->SetText(FText::AsNumber(Data.Number));
	Pill->SetContent(NumberText);
	USizeBox* PillBox = WidgetTree->ConstructWidget<USizeBox>();
	PillBox->SetHeightOverride(PillH);
	PillBox->SetMinDesiredWidth(PillH);
	PillBox->SetContent(Pill);
	CostPill = PillBox;
	Put(PillBox, Disc + FVector2D(0.f, Size * PillCenterY), FVector2D::ZeroVector);
	CostPill->SetVisibility(bEmpty ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
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

void UNinjaJutsuSlotWidget::ApplyArt()
{
	if (!ArtMID)
	{
		return;
	}
	if (Data.Icon)
	{
		ArtMID->SetTextureParameterValue(TEXT("Slot"), Data.Icon.Get());
	}
	const FVector2D Center = Data.bStandaloneArt ? FVector2D(0.5f, 0.5f) : NinjaJutsuSlot::ArtCenter;
	ArtMID->SetVectorParameterValue(TEXT("ArtCenter"), FLinearColor(Center.X, Center.Y, 0.f, 0.f));
	ArtMID->SetScalarParameterValue(TEXT("ArtRadius"), Data.bStandaloneArt ? 0.5f : NinjaJutsuSlot::ArtRadius);
	ArtMID->SetScalarParameterValue(TEXT("Empty"), Data.Icon ? 0.f : 1.f);
}

void UNinjaJutsuSlotWidget::SetData(const FNinjaJutsuSlotData& InData)
{
	Data = InData;
	const bool bEmpty = Data.Icon == nullptr;
	ApplyArt();
	if (NumberText)
	{
		NumberText->SetText(FText::AsNumber(Data.Number));
	}
	if (CostPill)
	{
		CostPill->SetVisibility(bEmpty ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UNinjaJutsuSlotWidget::TickSlot(float DeltaTime)
{
	using namespace NinjaJutsuSlot;
	SelectAlpha = FMath::FInterpConstantTo(SelectAlpha, bSelected ? 1.f : 0.f, DeltaTime, 1.f / SelectTime);
	HoverAlpha = FMath::FInterpConstantTo(HoverAlpha, IsHovered() ? 1.f : 0.f, DeltaTime, 1.f / SelectTime);
	const float Sel = 1.f - FMath::Square(1.f - SelectAlpha);   // ease-out
	const float Hov = 1.f - FMath::Square(1.f - HoverAlpha);
	float Scale = FMath::Lerp(1.f, SelectedScale, Sel) * (1.f + 0.04f * Hov * (1.f - Sel));

	// Usou: pulso 1 -> 1,12 -> 1 e flash branco
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
			Scale *= 1.f + 0.12f * FMath::Sin(PI * P);
			Flash = 1.f - P;
		}
	}

	// Cooldown: ao terminar, pulso de brilho azul
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
			Scale *= 1.f + 0.04f * Ready;
		}
	}

	// Brilho azul só no selecionado (e no pulso de "pronto"), como na referência
	const bool bEmpty = Data.Icon == nullptr;
	const float GlowIntensity = bEmpty ? 0.f : 0.75f * Sel + 0.4f * Ready;
	if (GlowMID)
	{
		GlowMID->SetScalarParameterValue(TEXT("Intensity"), GlowIntensity);
	}
	if (Glow)
	{
		Glow->SetVisibility(GlowIntensity > 0.01f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (GlowSlot)
	{
		GlowSlot->SetSize(FVector2D(Size * (GlowScale + 0.1f * Ready)));
	}
	if (ArtMID)
	{
		ArtMID->SetScalarParameterValue(TEXT("Selected"), Sel);
		ArtMID->SetScalarParameterValue(TEXT("Cooldown"), CooldownFrac);
		ArtMID->SetScalarParameterValue(TEXT("NoChakra"), bHasChakra || bEmpty ? 0.f : 1.f);
		ArtMID->SetScalarParameterValue(TEXT("Flash"), Flash);
	}
	// Segundos com uma casa decimal ("27.1s"); atualiza o texto só quando o décimo muda
	const int32 Tenths = CooldownLeft > 0.f ? FMath::CeilToInt(CooldownLeft * 10.f) : 0;
	if (CooldownText && Tenths != ShownCooldownTenths)
	{
		ShownCooldownTenths = Tenths;
		CooldownText->SetText(FText::FromString(FString::Printf(TEXT("%d.%ds"), Tenths / 10, Tenths % 10)));
		CooldownText->SetVisibility(Tenths > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	SetRenderScale(FVector2D(Scale, Scale));
}
