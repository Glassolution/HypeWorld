// HUD do ninja montado inteiro em C++ (ver o .h). Layout pensado em 1920x1080; a Unreal escala para outras telas.

#include "NinjaHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
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
				float A = 0.f;   // 16 amostras por pixel: bordas suaves mesmo em ícones pequenos
				for (int32 S = 0; S < 16; ++S)
				{
					A += Alpha((X + 0.125f + 0.25f * (S % 4)) / Size, (Y + 0.125f + 0.25f * (S / 4)) / Size);
				}
				Pixels[Y * Size + X] = FColor(255, 255, 255, (uint8)FMath::RoundToInt(FMath::Clamp(A / 16.f, 0.f, 1.f) * 255.f));
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
				for (int32 S = 0; S < 16; ++S)
				{
					Sum += Color((X + 0.125f + 0.25f * (S % 4)) / Width, (Y + 0.125f + 0.25f * (S / 4)) / Height);
				}
				Sum /= 16.f;
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
	SwirlTexture = MakeTexture(18, [](float U, float V)
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

	BuildLifeBlock(Canvas);

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
		SetFill(HealthFill, HealthTexture, Max > 0.f ? Current / Max : 0.f, 327.f, FLinearColor::White);   // a cor já está na textura
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
		SetFill(ChakraFill, ChakraTexture, Percent, 205.f, FLinearColor(1.f, 1.f, 1.f, 0.9f));
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

	if (CoinText && Coins != ShownCoins)
	{
		ShownCoins = Coins;
		CoinText->SetText(FText::FromString(FormatThousands(Coins)));
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
	// Anel de XP: 124 px de diâmetro externo, 7 px de espessura, sem contorno. Preenchido #8CC6EE começando embaixo,
	// subindo pela esquerda, topo e descendo pela direita; o resto é trilho #2B2B2B a 80% (abertura embaixo, no selo de nível)
	ShownXp = ExperiencePercent;
	const float P = FMath::Clamp(ExperiencePercent, 0.f, 1.f);
	RingTexture = NinjaHUD::MakeTextureRGBA(124, 124, [P](float U, float V)
	{
		const FVector2f D(U - 0.5f, V - 0.5f);
		const float R = D.Size() * 2.f;
		if (R < 1.f - 14.f / 124.f || R > 1.f)
		{
			return FLinearColor(0.f, 0.f, 0.f, 0.f);
		}
		float Ang = FMath::Atan2(-D.X, D.Y) / (2.f * PI);   // 0 = embaixo, cresce pela esquerda, topo, direita
		Ang = Ang < 0.f ? Ang + 1.f : Ang;
		return Ang <= P ? FLinearColor(140.f / 255.f, 198.f / 255.f, 238.f / 255.f, 1.f) : FLinearColor(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 0.8f);
	});
	if (XpRing)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(RingTexture);
		XpRing->SetBrush(Brush);
	}
}

FSlateFontInfo UNinjaHUDWidget::HudFontInfo(const TCHAR* Face, float SizePx) const
{
	// Medidas da referência em px; a Unreal mede fonte em pontos (96 dpi): pt = px * 0,75
	const float Points = SizePx * 0.75f;
	if (HudFont)
	{
		return FSlateFontInfo(HudFont, Points, FName(Face));
	}
	return FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Points);   // substituta temporária até importar a Montserrat
}

void UNinjaHUDWidget::BuildLifeBlock(UCanvasPanel* Root)
{
	using namespace NinjaHUD;

	// Painel 548x254 com as coordenadas exatas da referência (origem no canto de cima à esquerda), escalado inteiro
	UCanvasPanel* Block = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BlocoDeVida"));
	UCanvasPanelSlot* BlockSlot = Root->AddChildToCanvas(Block);
	BlockSlot->SetAnchors(FAnchors(0.f, 1.f));
	BlockSlot->SetAlignment(FVector2D::ZeroVector);
	BlockSlot->SetPosition(LifeBlockPosition);
	BlockSlot->SetSize(FVector2D(548.f, 254.f));
	Block->SetRenderTransformPivot(FVector2D::ZeroVector);
	Block->SetRenderScale(FVector2D(LifeBlockScale, LifeBlockScale));

	auto Put = [Block](UWidget* Widget, float X, float Y, float W, float H, FVector2D Alignment = FVector2D::ZeroVector)
	{
		UCanvasPanelSlot* PanelSlot = Block->AddChildToCanvas(Widget);
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAlignment(Alignment);
		PanelSlot->SetPosition(FVector2D(X, Y));
		if (W <= 0.f)
		{
			PanelSlot->SetAutoSize(true);
		}
		else
		{
			PanelSlot->SetSize(FVector2D(W, H));
		}
		return PanelSlot;
	};
	auto Solid = [this](const FColor& SRGB, float Opacity, bool bCircle = false)
	{
		FLinearColor Color = FLinearColor::FromSRGBColor(SRGB);
		Color.A = Opacity;
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		FSlateBrush Brush;
		if (bCircle)
		{
			Brush = Rounded(Color, 0.f, FLinearColor::Transparent, 0.f, true);
		}
		else
		{
			Brush.TintColor = FSlateColor(Color);   // retângulo de cantos retos
		}
		Image->SetBrush(Brush);
		return Image;
	};
	auto Textured = [this](UTexture2D* Texture, const FLinearColor& Tint)
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.TintColor = FSlateColor(Tint);
		Image->SetBrush(Brush);
		return Image;
	};
	auto Text = [this](const FString& Words, const TCHAR* Face, float SizePx, bool bShadow)
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(FText::FromString(Words));
		T->SetFont(HudFontInfo(Face, SizePx));
		T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		if (bShadow)
		{
			T->SetShadowOffset(FVector2D(1.f, 1.f));
			T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
		}
		return T;
	};
	auto S = [](float R, float G, float B, float A = 1.f) { return FLinearColor(R / 255.f, G / 255.f, B / 255.f, A); };   // cor sRGB para a textura (textura é sRGB)

	// ---- Texturas (desenhadas por código) ----
	auto Hash = [](int32 X, int32 Y) { uint32 H = uint32(X) * 374761393u + uint32(Y) * 668265263u; H = (H ^ (H >> 13)) * 1274126177u; return float(H & 0xFFFF) / 65535.f; };
	auto Noise = [&Hash](float X, float Y)
	{
		const int32 IX = FMath::FloorToInt(X), IY = FMath::FloorToInt(Y);
		const float FX = X - IX, FY = Y - IY, SX = FX * FX * (3.f - 2.f * FX), SY = FY * FY * (3.f - 2.f * FY);
		return FMath::Lerp(FMath::Lerp(Hash(IX, IY), Hash(IX + 1, IY), SX), FMath::Lerp(Hash(IX, IY + 1), Hash(IX + 1, IY + 1), SX), SY);
	};
	// Vida: #E8335C com linhas finas onduladas "mármore/rachadura" #B8213F a ~40%, espalhadas na barra toda
	HealthTexture = MakeTextureRGBA(327, 30, [&Noise, &S](float U, float V)
	{
		const float X = U * 10.9f, Y = V;   // proporção 327x30
		const float Warp = Noise(X * 1.3f, Y * 2.f) * 2.2f + Noise(X * 3.1f + 7.f, Y * 4.f) * 0.7f;
		const float Field = FMath::Abs(FMath::Sin(X * 2.1f + Y * 3.f + Warp * 3.f));
		const float Line = FMath::Clamp(1.f - Field / 0.07f, 0.f, 1.f) * (0.55f + 0.45f * Noise(X * 2.f, 3.f));
		return FMath::Lerp(S(232, 51, 92), S(184, 33, 63), Line * 0.4f);
	});
	// Chakra: cinza #6E6E6E com pincelada/pedra (variações #555 a #888), retangular
	ChakraTexture = MakeTextureRGBA(205, 26, [&Noise, &S](float U, float V)
	{
		const float X = U * 7.9f, Y = V;
		const float Grain = 0.6f * Noise(X * 1.5f, Y * 9.f) + 0.25f * Noise(X * 6.f, Y * 3.f) + 0.15f * Noise(X * 20.f, Y * 20.f);
		const float Gray = FMath::Lerp(85.f, 136.f, Grain);   // #555..#888
		return S(Gray, Gray, Gray);
	});
	// Ícones de traço fino: microfone e headphone; moeda (anel dourado, centro escuro)
	MicTexture = MakeTexture(14, [](float U, float V)
	{
		const FVector2f P(U, V);
		const float W = 0.075f;   // espessura do traço (~1 px)
		const bool Capsule = (FMath::Abs(U - 0.5f) <= 0.15f && V >= 0.2f && V <= 0.45f) || FVector2f::Distance(P, FVector2f(0.5f, 0.2f)) <= 0.15f || FVector2f::Distance(P, FVector2f(0.5f, 0.45f)) <= 0.15f;
		const float R = FVector2f::Distance(P, FVector2f(0.5f, 0.46f));
		const bool Cup = V >= 0.46f && FMath::Abs(R - 0.27f) <= W * 0.5f;
		const bool Stem = FMath::Abs(U - 0.5f) <= W * 0.5f && V >= 0.73f && V <= 0.88f;
		const bool Base = FMath::Abs(U - 0.5f) <= 0.17f && FMath::Abs(V - 0.88f) <= W * 0.5f;
		return (Capsule || Cup || Stem || Base) ? 1.f : 0.f;
	});
	HeadsetTexture = MakeTexture(14, [](float U, float V)
	{
		const FVector2f P(U, V);
		const float R = FVector2f::Distance(P, FVector2f(0.5f, 0.55f));
		const bool Band = V <= 0.58f && FMath::Abs(R - 0.36f) <= 0.04f;
		auto Cup = [&P](float X) { return FMath::Abs(P.X - X) <= 0.09f && P.Y >= 0.55f && P.Y <= 0.86f; };
		return (Band || Cup(0.17f) || Cup(0.83f)) ? 1.f : 0.f;
	});
	CoinTexture = MakeTextureRGBA(18, 18, [&S](float U, float V)
	{
		const float R = FVector2f(U - 0.5f, V - 0.5f).Size() * 2.f;
		if (R > 1.f)
		{
			return FLinearColor(0.f, 0.f, 0.f, 0.f);
		}
		return R > 0.52f ? S(226, 179, 60) : S(58, 42, 10);
	});

	// ---- Camadas (de trás para frente): vida -> chakra -> caixa de voz -> avatar -> selo -> textos ----
	// 4) Barra de vida 327x30 em (183,88): vazio #3A1A22 85%, preenchimento texturizado (recortado pela porcentagem)
	Put(Solid(FColor(58, 26, 34), 0.85f), 183.f, 88.f, 327.f, 30.f);
	HealthFill = WidgetTree->ConstructWidget<UImage>();
	Put(HealthFill, 183.f, 88.f, 327.f, 30.f);

	// 5) Barra de chakra 205x26 em (183,124): fundo #1E1E1E 75%, preenchimento pedra cinza 90%, espiral apagada
	Put(Solid(FColor(30, 30, 30), 0.75f), 183.f, 124.f, 205.f, 26.f);
	ChakraFill = WidgetTree->ConstructWidget<UImage>();
	Put(ChakraFill, 183.f, 124.f, 205.f, 26.f);
	FLinearColor SwirlTint = FLinearColor::FromSRGBColor(FColor(154, 154, 154));
	SwirlTint.A = 0.4f;
	SwirlIcon = Textured(SwirlTexture, SwirlTint);
	Put(SwirlIcon, 205.f, 137.f, 18.f, 18.f, FVector2D(0.5f, 0.5f));

	// 6) Caixa de voz 108x26 em (395,124): #141414 85%, microfone (433) e headphone (467) #E6E6E6 14 px
	Put(Solid(FColor(20, 20, 20), 0.85f), 395.f, 124.f, 108.f, 26.f);
	const FLinearColor IconTint = FLinearColor::FromSRGBColor(FColor(230, 230, 230));
	Put(Textured(MicTexture, IconTint), 433.f, 137.f, 14.f, 14.f, FVector2D(0.5f, 0.5f));
	Put(Textured(HeadsetTexture, IconTint), 467.f, 137.f, 14.f, 14.f, FVector2D(0.5f, 0.5f));

	// 1) Avatar centrado em (117,130): anel de XP 124 px, fundo #1A1A1A e retrato em círculo de 100 px
	XpRing = WidgetTree->ConstructWidget<UImage>();
	Put(XpRing, 117.f, 130.f, 124.f, 124.f, FVector2D(0.5f, 0.5f));
	UpdateXpRing();
	Put(Solid(FColor(26, 26, 26), 1.f, true), 117.f, 130.f, 100.f, 100.f, FVector2D(0.5f, 0.5f));
	if (PortraitTexture)
	{
		UImage* Portrait = WidgetTree->ConstructWidget<UImage>();
		Portrait->SetBrush(CircleImage(PortraitTexture));
		Put(Portrait, 117.f, 130.f, 100.f, 100.f, FVector2D(0.5f, 0.5f));
	}

	// 2) Selo de nível: círculo #111111 de 32 px em (118,186), texto ExtraBold 15
	Put(Solid(FColor(17, 17, 17), 1.f, true), 118.f, 186.f, 32.f, 32.f, FVector2D(0.5f, 0.5f));
	LevelText = Text(FString::FromInt(PlayerLevel), TEXT("ExtraBold"), 15.f, false);
	Put(LevelText, 118.f, 186.f, 0.f, 0.f, FVector2D(0.5f, 0.5f));

	// 3) Nome: x 181, topo em y 50, Bold 17, sombra 1 px
	NameText = Text(TEXT("Ninja"), TEXT("Bold"), 17.f, true);
	Put(NameText, 181.f, 50.f, 0.f, 0.f);

	// Texto da vida: "750" ExtraBold 22 + "/750" Bold 13 na mesma linha de base, x 200, centrado na barra
	HealthText = Text(TEXT("0"), TEXT("ExtraBold"), 22.f, true);
	HealthMaxText = Text(TEXT("/0"), TEXT("Bold"), 13.f, true);
	UHorizontalBox* HpRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	HpRow->AddChildToHorizontalBox(HealthText)->SetVerticalAlignment(VAlign_Bottom);
	UHorizontalBoxSlot* MaxSlot = HpRow->AddChildToHorizontalBox(HealthMaxText);
	MaxSlot->SetVerticalAlignment(VAlign_Bottom);
	MaxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));   // aproxima a linha de base (fontes de tamanhos diferentes)
	Put(HpRow, 200.f, 103.f, 0.f, 0.f, FVector2D(0.f, 0.5f));

	// Texto do chakra: ExtraBold 14, alinhado à direita com 10 px de margem (x 378)
	ChakraText = Text(TEXT("100%"), TEXT("ExtraBold"), 14.f, false);
	Put(ChakraText, 378.f, 137.f, 0.f, 0.f, FVector2D(1.f, 0.5f));

	// 7) Moedas: ícone 18 px em (181,187), texto Bold 15 em x 199
	Put(Textured(CoinTexture, FLinearColor::White), 181.f, 187.f, 18.f, 18.f, FVector2D(0.5f, 0.5f));
	CoinText = Text(FormatThousands(Coins), TEXT("Bold"), 15.f, false);
	Put(CoinText, 199.f, 187.f, 0.f, 0.f, FVector2D(0.f, 0.5f));
	ShownCoins = Coins;
}

FString UNinjaHUDWidget::FormatThousands(int32 Value)
{
	// 40300 -> "40,300" (vírgula como na referência, independente do idioma do sistema)
	FString Digits = FString::FromInt(FMath::Abs(Value));
	for (int32 i = Digits.Len() - 3; i > 0; i -= 3)
	{
		Digits.InsertAt(i, TEXT(','));
	}
	return Value < 0 ? TEXT("-") + Digits : Digits;
}
