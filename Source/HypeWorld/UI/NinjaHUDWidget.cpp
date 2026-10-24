// HUD do ninja montado inteiro em C++ (ver o .h). Layout pensado em 1920x1080; a Unreal escala para outras telas.

#include "NinjaHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Styling/CoreStyle.h"
#include "HypeWorldCharacter.h"
#include "NinjaAttributeSet.h"

namespace NinjaHUD
{
	// Cores medidas na referência (sRGB) e convertidas para o espaço linear que a interface usa
	const FLinearColor White(1.f, 1.f, 1.f, 1.f);
	const FLinearColor Ring = FLinearColor::FromSRGBColor(FColor(140, 200, 238));			// arco azul-claro do retrato (experiência)
	const FLinearColor HealthPink = FLinearColor::FromSRGBColor(FColor(224, 50, 92));		// barra de vida (carmim)
	const FLinearColor ChakraGray = FLinearColor::FromSRGBColor(FColor(172, 184, 178, 225));	// barra de chakra (pincelada cinza-esverdeada)
	const FLinearColor ChakraTrackColor = FLinearColor::FromSRGBColor(FColor(52, 58, 55, 170));
	const FLinearColor ChakraBlue = FLinearColor::FromSRGBColor(FColor(110, 200, 255));
	const FLinearColor Track(0.f, 0.f, 0.f, 0.38f);
	const FLinearColor Badge = FLinearColor::FromSRGBColor(FColor(16, 16, 18));
	const FLinearColor BoxColor = FLinearColor::FromSRGBColor(FColor(20, 21, 24, 240));
	const FLinearColor PortraitBack = FLinearColor::FromSRGBColor(FColor(16, 16, 20));
	const FLinearColor SwirlGray = FLinearColor::FromSRGBColor(FColor(120, 130, 125, 230));
	const FLinearColor Gold(0.98f, 0.76f, 0.22f, 1.f);

	/** Caixa arredondada (ou círculo/pílula quando bRound). */
	FSlateBrush Rounded(const FLinearColor& Fill, float Radius, const FLinearColor& Outline = FLinearColor::Transparent, float OutlineWidth = 0.f, bool bRound = false)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(Outline), OutlineWidth);
		if (bRound)
		{
			Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		}
		return Brush;
	}

	/** Imagem recortada em círculo. */
	FSlateBrush CircleImage(UObject* Texture, const FLinearColor& Tint = White)
	{
		FSlateBrush Brush = Rounded(Tint, 0.f, FLinearColor::Transparent, 0.f, true);
		Brush.SetResourceObject(Texture);
		return Brush;
	}

	FSlateFontInfo Font(const TCHAR* Style, float Size, bool bShadowOutline = true)
	{
		FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(Style, Size);
		if (bShadowOutline)
		{
			Info.OutlineSettings.OutlineSize = 1;
			Info.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.45f);
		}
		return Info;
	}

	/** Textura branca gerada por código; a função diz a opacidade de cada ponto (0..1). */
	UTexture2D* MakeTexture(int32 Size, TFunctionRef<float(float U, float V)> Alpha)
	{
		TArray<FColor> Pixels;
		Pixels.SetNumUninitialized(Size * Size);
		for (int32 Y = 0; Y < Size; ++Y)
		{
			for (int32 X = 0; X < Size; ++X)
			{
				float A = 0.f;
				for (int32 S = 0; S < 4; ++S)
				{
					A += Alpha((X + 0.25f + 0.5f * (S % 2)) / Size, (Y + 0.25f + 0.5f * (S / 2)) / Size);
				}
				Pixels[Y * Size + X] = FColor(255, 255, 255, (uint8)FMath::RoundToInt(FMath::Clamp(A / 4.f, 0.f, 1.f) * 255.f));
			}
		}
		const TConstArrayView64<uint8> Bytes(reinterpret_cast<const uint8*>(Pixels.GetData()), Pixels.Num() * sizeof(FColor));
		return UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8, NAME_None, Bytes);
	}

	/** Textura colorida gerada por código (cor + opacidade por ponto), 4 amostras por pixel. */
	UTexture2D* MakeTextureRGBA(int32 Width, int32 Height, TFunctionRef<FLinearColor(float U, float V)> Color)
	{
		TArray<FColor> Pixels;
		Pixels.SetNumUninitialized(Width * Height);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				FLinearColor Sum(0.f, 0.f, 0.f, 0.f);
				for (int32 S = 0; S < 4; ++S)
				{
					Sum += Color((X + 0.25f + 0.5f * (S % 2)) / Width, (Y + 0.25f + 0.5f * (S / 2)) / Height);
				}
				Sum /= 4.f;
				Pixels[Y * Width + X] = FColor(uint8(FMath::Clamp(Sum.R, 0.f, 1.f) * 255.f), uint8(FMath::Clamp(Sum.G, 0.f, 1.f) * 255.f),
					uint8(FMath::Clamp(Sum.B, 0.f, 1.f) * 255.f), uint8(FMath::Clamp(Sum.A, 0.f, 1.f) * 255.f));
			}
		}
		const TConstArrayView64<uint8> Bytes(reinterpret_cast<const uint8*>(Pixels.GetData()), Pixels.Num() * sizeof(FColor));
		return UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8, NAME_None, Bytes);
	}
}

TSharedRef<SWidget> UNinjaHUDWidget::RebuildWidget()
{
	// O layout é criado por código na primeira vez (no editor de widget fica vazio; aparece no Play)
	if (WidgetTree && !WidgetTree->RootWidget && !IsDesignTime())
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UNinjaHUDWidget::BuildLayout()
{
	using namespace NinjaHUD;

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;

	const FVector2D BottomLeft(0.f, 1.f), BottomCenter(0.5f, 1.f), Center(0.5f, 0.5f), LeftMid(0.f, 0.5f);
	auto Place = [Canvas](UWidget* Widget, FVector2D Anchor, FVector2D Position, FVector2D Size, FVector2D Alignment = FVector2D::ZeroVector)
	{
		UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Widget);
		PanelSlot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
		PanelSlot->SetAlignment(Alignment);
		PanelSlot->SetPosition(Position);
		if (Size.IsZero())
		{
			PanelSlot->SetAutoSize(true);
		}
		else
		{
			PanelSlot->SetSize(Size);
		}
		return PanelSlot;
	};
	auto MakeImage = [this](const FSlateBrush& Brush)
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		Image->SetBrush(Brush);
		return Image;
	};
	auto MakeText = [this](const FString& Words, float Size, const FLinearColor& Color = White, const TCHAR* Style = TEXT("Bold"))
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Words));
		Text->SetFont(Font(Style, Size));
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.f, 1.f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.35f));
		return Text;
	};
	auto MakeBar = [this](const FLinearColor& Fill, float Radius)
	{
		UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>();
		FProgressBarStyle Style;
		Style.SetBackgroundImage(Rounded(Track, Radius));
		Style.SetFillImage(Rounded(Fill, Radius));
		Style.SetMarqueeImage(Rounded(Fill, Radius));
		Bar->SetWidgetStyle(Style);
		Bar->SetFillColorAndOpacity(White);
		return Bar;
	};

	// Espiral do chakra (ícone da barra de chakra) e moeda
	SwirlTexture = MakeTexture(64, [](float U, float V)
	{
		const FVector2f P(U - 0.5f, V - 0.5f);
		const float R = P.Size() * 2.f;
		if (R > 0.95f)
		{
			return 0.f;
		}
		const float Ang = FMath::Atan2(P.Y, P.X);
		const float Turns = R * 2.2f - Ang / (2.f * PI);   // espiral de Arquimedes
		const float D = FMath::Abs(Turns - FMath::RoundToFloat(Turns));
		return D < 0.16f ? 1.f : 0.f;
	});

	// =========================== Canto de baixo à esquerda (medido na referência do Felipe) ===========================
	// Texturas desenhadas por código: vida com riscos sutis, chakra em pincelada com ponta diagonal, ícones de microfone/fone/moeda
	auto Hash = [](int32 X, int32 Y) { uint32 H = uint32(X) * 374761393u + uint32(Y) * 668265263u; H = (H ^ (H >> 13)) * 1274126177u; return float(H & 0xFFFF) / 65535.f; };
	auto Noise = [&Hash](float X, float Y)
	{
		const int32 IX = FMath::FloorToInt(X), IY = FMath::FloorToInt(Y);
		const float FX = X - IX, FY = Y - IY, SX = FX * FX * (3.f - 2.f * FX), SY = FY * FY * (3.f - 2.f * FY);
		return FMath::Lerp(FMath::Lerp(Hash(IX, IY), Hash(IX + 1, IY), SX), FMath::Lerp(Hash(IX, IY + 1), Hash(IX + 1, IY + 1), SX), SY);
	};
	HealthTexture = MakeTextureRGBA(512, 48, [&Noise](float U, float V)
	{
		// carmim com riscos escuros finos e ondulados (como a textura da referência) e topo um pouco mais claro
		const float Wave = FMath::Abs(FMath::Sin(U * 26.f + 5.f * FMath::Sin(V * 2.6f + U * 7.f) + Noise(U * 18.f, V * 3.f) * 3.f));
		const float Line = Wave < 0.05f ? 0.86f : 1.f;
		const float Shade = FMath::Lerp(1.04f, 0.94f, V) * Line;
		return FLinearColor(Shade, Shade, Shade, 1.f);
	});
	ChakraTexture = MakeTextureRGBA(512, 48, [&Noise](float U, float V)
	{
		// pincelada: borda esquerda irregular, ponta direita cortada em diagonal "/", estrias horizontais
		const float Left = 0.012f + 0.02f * Noise(V * 9.f, 3.1f);
		const float Right = 0.955f + 0.04f * (1.f - V);
		const float Edge = FMath::Clamp((U - Left) * 160.f, 0.f, 1.f) * FMath::Clamp((Right - U) * 160.f, 0.f, 1.f);
		const float Streak = 0.86f + 0.14f * Noise(U * 6.f, V * 14.f) + 0.08f * (1.f - V);
		const float TopBottom = FMath::Clamp(FMath::Min(V, 1.f - V) * 9.f + 0.35f * Noise(U * 40.f, 7.f), 0.f, 1.f);
		return FLinearColor(Streak, Streak, Streak, Edge * TopBottom);
	});
	MicTexture = MakeTexture(48, [](float U, float V)
	{
		const FVector2f P(U, V);
		const float Capsule = (FMath::Abs(U - 0.5f) < 0.12f && V > 0.18f && V < 0.5f) || FVector2f::Distance(P, FVector2f(0.5f, 0.18f)) < 0.12f || FVector2f::Distance(P, FVector2f(0.5f, 0.5f)) < 0.12f ? 1.f : 0.f;
		const float R = FVector2f::Distance(P, FVector2f(0.5f, 0.48f));
		const float Cup = (V > 0.48f && FMath::Abs(R - 0.22f) < 0.035f) ? 1.f : 0.f;
		const float Stem = (FMath::Abs(U - 0.5f) < 0.035f && V > 0.7f && V < 0.84f) ? 1.f : 0.f;
		const float Base = (FMath::Abs(U - 0.5f) < 0.15f && FMath::Abs(V - 0.85f) < 0.035f) ? 1.f : 0.f;
		return FMath::Max(FMath::Max(Capsule, Cup), FMath::Max(Stem, Base));
	});
	HeadsetTexture = MakeTexture(48, [](float U, float V)
	{
		const FVector2f P(U, V);
		const float R = FVector2f::Distance(P, FVector2f(0.5f, 0.55f));
		const float Band = (V < 0.62f && FMath::Abs(R - 0.34f) < 0.04f) ? 1.f : 0.f;
		auto Cup = [&P](float X) { return FMath::Abs(P.X - X) < 0.08f && P.Y > 0.55f && P.Y < 0.85f; };
		return (Band > 0.f || Cup(0.2f) || Cup(0.8f)) ? 1.f : 0.f;
	});
	CoinTexture = MakeTextureRGBA(48, 48, [](float U, float V)
	{
		// moeda dourada em forma de alvo: aro claro, anel escuro, centro claro
		const float R = FVector2f(U - 0.5f, V - 0.5f).Size() * 2.f;
		if (R > 0.98f) { return FLinearColor(0.f, 0.f, 0.f, 0.f); }
		if (R > 0.62f) { return FLinearColor(0.98f, 0.78f, 0.25f, 1.f); }
		if (R > 0.42f) { return FLinearColor(0.55f, 0.38f, 0.05f, 1.f); }
		return FLinearColor(0.98f, 0.8f, 0.3f, 1.f);
	});

	// Retrato: arco de experiência azul-claro (parcial), fundo escuro, imagem em círculo, nível num círculo preto embaixo
	const FVector2D Portrait(115.f, -115.f);
	XpRing = WidgetTree->ConstructWidget<UImage>();
	Place(XpRing, BottomLeft, Portrait, FVector2D(100.f, 100.f), Center);
	UpdateXpRing();
	Place(MakeImage(Rounded(PortraitBack, 0.f, FLinearColor::Transparent, 0.f, true)), BottomLeft, Portrait, FVector2D(86.f, 86.f), Center);
	if (PortraitTexture)
	{
		Place(MakeImage(CircleImage(PortraitTexture)), BottomLeft, Portrait, FVector2D(82.f, 82.f), Center);
	}
	Place(MakeImage(Rounded(Badge, 0.f, FLinearColor::Transparent, 0.f, true)), BottomLeft, FVector2D(118.f, -73.f), FVector2D(30.f, 30.f), Center);
	LevelText = MakeText(FString::FromInt(PlayerLevel), 12.f);
	Place(LevelText, BottomLeft, FVector2D(118.f, -73.f), FVector2D::ZeroVector, Center);

	NameText = MakeText(TEXT("Ninja"), 15.f);
	Place(NameText, BottomLeft, FVector2D(176.f, -181.f), FVector2D::ZeroVector, LeftMid);

	// Vida: fundo escuro + preenchimento carmim texturizado (recortado pela porcentagem)
	Place(MakeImage(Rounded(FLinearColor(0.f, 0.f, 0.f, 0.35f), 2.f)), BottomLeft, FVector2D(178.f, -157.f), FVector2D(289.f, 27.f));
	HealthFill = WidgetTree->ConstructWidget<UImage>();
	Place(HealthFill, BottomLeft, FVector2D(178.f, -157.f), FVector2D(289.f, 27.f));
	HealthText = MakeText(TEXT("0"), 19.f);
	Place(HealthText, BottomLeft, FVector2D(190.f, -143.f), FVector2D::ZeroVector, LeftMid);
	HealthMaxText = MakeText(TEXT("/0"), 11.f);
	Place(HealthMaxText, BottomLeft, FVector2D(232.f, -140.f), FVector2D::ZeroVector, LeftMid);

	// Chakra: pincelada escura de fundo + pincelada clara (recortada), espiral à esquerda e % à direita
	FSlateBrush ChakraTrack;
	ChakraTrack.SetResourceObject(ChakraTexture);
	ChakraTrack.TintColor = FSlateColor(ChakraTrackColor);
	Place(MakeImage(ChakraTrack), BottomLeft, FVector2D(178.f, -120.f), FVector2D(188.f, 26.f));
	ChakraFill = WidgetTree->ConstructWidget<UImage>();
	Place(ChakraFill, BottomLeft, FVector2D(178.f, -120.f), FVector2D(188.f, 26.f));
	FSlateBrush Swirl;
	Swirl.SetResourceObject(SwirlTexture);
	Swirl.TintColor = FSlateColor(SwirlGray);
	SwirlIcon = MakeImage(Swirl);
	Place(SwirlIcon, BottomLeft, FVector2D(196.f, -107.f), FVector2D(19.f, 19.f), Center);
	ChakraText = MakeText(TEXT("100%"), 14.f);
	Place(ChakraText, BottomLeft, FVector2D(350.f, -107.f), FVector2D::ZeroVector, FVector2D(1.f, 0.5f));

	// Caixa preta com microfone e fone (como na referência)
	Place(MakeImage(Rounded(BoxColor, 4.f)), BottomLeft, FVector2D(372.f, -123.f), FVector2D(95.f, 31.f));
	FSlateBrush Mic;
	Mic.SetResourceObject(MicTexture);
	Place(MakeImage(Mic), BottomLeft, FVector2D(404.f, -107.f), FVector2D(17.f, 17.f), Center);
	FSlateBrush Headset;
	Headset.SetResourceObject(HeadsetTexture);
	Place(MakeImage(Headset), BottomLeft, FVector2D(434.f, -107.f), FVector2D(18.f, 18.f), Center);

	// Moedas
	FSlateBrush Coin;
	Coin.SetResourceObject(CoinTexture);
	Place(MakeImage(Coin), BottomLeft, FVector2D(181.f, -72.f), FVector2D(17.f, 17.f), Center);
	CoinText = MakeText(FText::AsNumber(Coins).ToString(), 14.f);
	Place(CoinText, BottomLeft, FVector2D(196.f, -72.f), FVector2D::ZeroVector, LeftMid);

	// =========================== Centro de baixo: slots de jutsu (descem fora do modo de batalha) ===========================
	JutsuSlots.Reset();
	JutsuSlotBase.Reset();
	const int32 Count = FMath::Max(1, JutsuSlotCount);
	const float Size = 62.f, Gap = 14.f;
	const float StartX = -(Count - 1) * (Size + Gap) * 0.5f;
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector2D Pos(StartX + i * (Size + Gap), -58.f);
		UTexture2D* IconTex = AbilityIcons.IsValidIndex(i) ? AbilityIcons[i] : nullptr;
		UImage* Back = MakeImage(Rounded(FLinearColor(0.05f, 0.05f, 0.06f, 0.82f), 0.f, FLinearColor(1.f, 1.f, 1.f, 0.35f), 2.f, true));
		JutsuSlots.Add(Place(Back, BottomCenter, Pos, FVector2D(Size, Size), Center));
		JutsuSlotBase.Add(Pos);
		if (IconTex)
		{
			JutsuSlots.Add(Place(MakeImage(CircleImage(IconTex)), BottomCenter, Pos, FVector2D(Size - 8.f, Size - 8.f), Center));
			JutsuSlotBase.Add(Pos);
		}
	}
	// Cargas da substituição (Q apanhando): duas pílulas em cima da barra, sobem junto com ela
	SubstitutionPips.Reset();
	for (int32 k = 0; k < 2; ++k)
	{
		const FVector2D Pos(-12.f + k * 24.f, -104.f);
		UImage* Pip = MakeImage(Rounded(White, 0.f, FLinearColor::Transparent, 0.f, true));
		SubstitutionPips.Add(Pip);
		JutsuSlots.Add(Place(Pip, BottomCenter, Pos, FVector2D(18.f, 6.f), Center));
		JutsuSlotBase.Add(Pos);
	}
	JutsuOffset = JutsuHiddenOffset;

	// =========================== Barra do super pulo (só carregando) e contador de golpes ===========================
	SuperJumpText = MakeText(TEXT("PULO DE CHAKRA"), 14.f);
	SuperJumpText->SetVisibility(ESlateVisibility::Collapsed);
	Place(SuperJumpText, BottomCenter, FVector2D(0.f, -168.f), FVector2D::ZeroVector, Center);
	SuperJumpBar = MakeBar(FLinearColor(0.35f, 0.75f, 1.f, 1.f), 4.f);
	SuperJumpBar->SetVisibility(ESlateVisibility::Collapsed);
	Place(SuperJumpBar, BottomCenter, FVector2D(0.f, -146.f), FVector2D(320.f, 12.f), Center);

	ComboText = MakeText(TEXT(""), 40.f, FLinearColor(1.f, 0.62f, 0.15f));
	ComboText->SetVisibility(ESlateVisibility::Collapsed);
	Place(ComboText, FVector2D(1.f, 0.5f), FVector2D(-90.f, -40.f), FVector2D::ZeroVector, FVector2D(1.f, 0.5f));
}

void UNinjaHUDWidget::SetHealth(float Current, float Max)
{
	Health = Current;
	HealthMax = Max;
	if (HealthFill)
	{
		SetFill(HealthFill, HealthTexture, Max > 0.f ? Current / Max : 0.f, 289.f, NinjaHUD::HealthPink);
		HealthText->SetText(FText::AsNumber(FMath::RoundToInt(Current)));
		HealthMaxText->SetText(FText::FromString(FString::Printf(TEXT("/%d"), FMath::RoundToInt(Max))));
	}
}

void UNinjaHUDWidget::SetChakra(float Current, float Max)
{
	Chakra = Current;
	ChakraMax = Max;
	if (ChakraFill)
	{
		const float Percent = Max > 0.f ? Current / Max : 0.f;
		SetFill(ChakraFill, ChakraTexture, Percent, 188.f, NinjaHUD::ChakraGray);
		ChakraText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Percent * 100.f))));
	}
}

void UNinjaHUDWidget::SetPlayerLevel(int32 Level)
{
	PlayerLevel = Level;
	if (LevelText)
	{
		LevelText->SetText(FText::AsNumber(Level));
	}
}

void UNinjaHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(GetOwningPlayerPawn());
	if (!Ninja)
	{
		return;
	}

	// Vida e chakra de verdade (atributos do GAS, replicados)
	if (const UNinjaAttributeSet* A = Ninja->GetAttributes())
	{
		if (A->GetHealth() != Health || A->GetMaxHealth() != HealthMax)
		{
			SetHealth(A->GetHealth(), A->GetMaxHealth());
		}
		if (A->GetChakra() != Chakra || A->GetMaxChakra() != ChakraMax)
		{
			SetChakra(A->GetChakra(), A->GetMaxChakra());
		}
	}

	// Nome do jogador (o da partida)
	if (const APlayerState* PS = Ninja->GetPlayerState())
	{
		const FString Name = PS->GetPlayerName().IsEmpty() ? TEXT("Ninja") : PS->GetPlayerName();
		if (Name != ShownName && NameText)
		{
			ShownName = Name;
			NameText->SetText(FText::FromString(Name));
		}
	}

	// Infusão de chakra ligada: a espiral da barra de chakra fica azul (discreto, sem mudar o desenho da referência)
	const int32 Infusion = Ninja->IsPassiveEnabled(ENinjaPassive::ChakraFeet) ? 1 : 0;
	if (Infusion != ShownInfusion && SwirlIcon)
	{
		ShownInfusion = Infusion;
		SwirlIcon->SetColorAndOpacity(Infusion ? NinjaHUD::ChakraBlue : NinjaHUD::SwirlGray);
	}
	if (!FMath::IsNearlyEqual(ExperiencePercent, ShownXp, 0.002f))
	{
		UpdateXpRing();
	}

	// Slots de jutsu: sobem no modo de batalha, descem (só a pontinha) fora dele
	const float Target = Ninja->IsInBattleMode() ? 0.f : JutsuHiddenOffset;
	JutsuOffset = FMath::FInterpTo(JutsuOffset, Target, InDeltaTime, 9.f);
	for (int32 i = 0; i < JutsuSlots.Num(); ++i)
	{
		if (JutsuSlots[i] && JutsuSlotBase.IsValidIndex(i))
		{
			JutsuSlots[i]->SetPosition(JutsuSlotBase[i] + FVector2D(0.f, JutsuOffset));
		}
	}
	const float Raised = 1.f - FMath::Clamp(JutsuOffset / FMath::Max(JutsuHiddenOffset, 1.f), 0.f, 1.f);
	for (UImage* Pip : SubstitutionPips)
	{
		Pip->SetRenderOpacity(Raised);   // as cargas só aparecem com a barra levantada (batalha)
	}
	if (Ninja->GetSubstitutionCharges() != ShownSubstitutions)
	{
		ShownSubstitutions = Ninja->GetSubstitutionCharges();
		for (int32 k = 0; k < SubstitutionPips.Num(); ++k)
		{
			SubstitutionPips[k]->SetColorAndOpacity(k < ShownSubstitutions ? FLinearColor(1.f, 1.f, 1.f, 0.95f) : FLinearColor(1.f, 1.f, 1.f, 0.18f));
		}
	}

	// Super pulo carregando
	if (SuperJumpBar)
	{
		const ESlateVisibility Show = Ninja->IsChargingSuperJump() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		SuperJumpBar->SetVisibility(Show);
		SuperJumpText->SetVisibility(Show);
		SuperJumpBar->SetPercent(Ninja->GetSuperJumpCharge());
	}

	// Contador de golpes seguidos
	if (ComboText)
	{
		const int32 Hits = Ninja->GetComboHitCount();
		ComboText->SetVisibility(Hits >= 2 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Hits >= 2)
		{
			ComboText->SetText(FText::FromString(FString::Printf(TEXT("%d GOLPES"), Hits)));
		}
	}
}

void UNinjaHUDWidget::SetFill(UImage* Fill, UTexture2D* Texture, float Percent, float Width, const FLinearColor& Tint)
{
	// Mostra só o pedaço [0, Percent] da textura, no tamanho certo (sem esticar a ponta/pincelada)
	Percent = FMath::Clamp(Percent, 0.f, 1.f);
	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.TintColor = FSlateColor(Tint);
	Brush.SetUVRegion(FBox2f(FVector2f(0.f, 0.f), FVector2f(FMath::Max(Percent, 0.001f), 1.f)));
	Fill->SetBrush(Brush);
	Fill->SetVisibility(Percent > 0.001f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Fill->Slot))
	{
		PanelSlot->SetSize(FVector2D(Width * Percent, PanelSlot->GetSize().Y));
	}
}

void UNinjaHUDWidget::UpdateXpRing()
{
	// Arco azul-claro em volta do retrato: começa embaixo (atrás do nível) e sobe pela esquerda, no sentido horário
	ShownXp = ExperiencePercent;
	const float P = FMath::Clamp(ExperiencePercent, 0.f, 1.f);
	RingTexture = NinjaHUD::MakeTexture(128, [P](float U, float V)
	{
		const FVector2f D(U - 0.5f, V - 0.5f);
		const float R = D.Size() * 2.f;
		if (R < 0.87f || R > 0.995f)
		{
			return 0.f;
		}
		float Ang = FMath::Atan2(-D.X, D.Y) / (2.f * PI);   // 0 = embaixo, cresce indo pela esquerda e por cima
		Ang = Ang < 0.f ? Ang + 1.f : Ang;
		return Ang <= P ? 1.f : 0.f;
	});
	if (XpRing)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(RingTexture);
		Brush.TintColor = FSlateColor(NinjaHUD::Ring);
		XpRing->SetBrush(Brush);
	}
}
