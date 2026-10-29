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
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Texture2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Styling/CoreStyle.h"
#include "HypeWorldCharacter.h"
#include "NinjaAttributeSet.h"
#include "NinjaJutsuData.h"
#include "NinjaFlameOrbit.h"

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

namespace NinjaLife
{
	// Medidas em px da imagem normal do Felipe (Tools/hud/bloco_vida_layout.json, gerado por Tools/hud/recortar_bloco_vida.py)
	const FVector2D Origin(92.f, 123.f);	// canto de cima à esquerda do bloco
	const FVector2D Size(1813.f, 582.f);	// até o fim da barra de vida (x 1905) e dos ícones (y 705)
	const FBox2D Base(FVector2D(92.f, 123.f), FVector2D(1876.f, 690.f));
	const FBox2D Badge(FVector2D(253.f, 502.f), FVector2D(445.f, 692.f));
	const FVector2D BadgeCenter(348.5f, 597.5f);
	const FVector2D RingCenter(348.8f, 374.f);
	const float RingInner = 197.5f, RingOuter = 248.5f;
	const float RingStartDeg = 18.f, RingEndDeg = 342.f;	// 0 = embaixo, cresce pela esquerda; o arco começa e termina no selo
	const FColor RingColor(125, 217, 254);
	const FBox2D Health(FVector2D(596.f, 237.f), FVector2D(1905.f, 389.f));
	const FBox2D Chakra(FVector2D(590.f, 403.f), FVector2D(1437.f, 530.f));
	const FBox2D Coin(FVector2D(566.f, 576.f), FVector2D(683.f, 689.f));
	// Textos: ponto de apoio (px da imagem) e tamanho da fonte (px da imagem; altura das maiúsculas ~0,7 disso)
	const FVector2D NamePos(605.f, 172.f);			const float NameSize = 117.f;
	const FVector2D HealthPos(678.f, 312.f);		const float HealthSize = 116.f, HealthMaxSize = 72.f;
	const FVector2D ChakraPos(1385.f, 469.f);		const float ChakraSize = 83.f;		// direita do texto
	const FVector2D ChakraBattlePos(1371.f, 462.f);	const float ChakraBattleSize = 76.f;
	const float LevelSize = 145.f;
	const FVector2D CoinTextPos(712.f, 632.f);		const float CoinSize = 100.f;
	const FColor RaisedShadow(150, 150, 162);		// "relevo" cinza embaixo do nome e das moedas
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

	BuildLifeBlock(Canvas);

	// =========================== Centro de baixo: slots de jutsu (descem fora do modo de batalha) ===========================
	JutsuSlots.Reset();
	JutsuSlotBase.Reset();
	JutsuSlotWidgets.Reset();
	// Arte de cada slot: recortes do PNG do Felipe (Tools/editor/montar_slots_jutsu.py liga no WBP). Sem dados = slots vazios
	const TArray<FNinjaJutsuSlotData>& SlotData = JutsuSlotData;
	// Linha centrada embaixo, como na imagem: Size = diâmetro do círculo da arte; a moldura e a etiqueta passam dele
	// (etiqueta até ~0,93 x Size abaixo do centro do círculo; moldura até ~0,72 x Size em volta)
	const int32 Count = FMath::Max(1, JutsuSlotCount);
	const float Size = JutsuSlotSize;
	const float StartX = -(Count - 1) * (Size + JutsuSlotGap) * 0.5f;
	const float CenterY = -(JutsuBottomMargin * 1080.f + Size * 0.93f);
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector2D Pos(StartX + i * (Size + JutsuSlotGap), CenterY);
		UNinjaJutsuSlotWidget* SlotWidget = CreateWidget<UNinjaJutsuSlotWidget>(this, UNinjaJutsuSlotWidget::StaticClass());
		SlotWidget->Setup(SlotData.IsValidIndex(i) ? SlotData[i] : FNinjaJutsuSlotData(), Size,
			HudFontInfo(TEXT("ExtraBold"), Size * 0.38f), HudFontInfo(TEXT("ExtraBold"), Size * 0.46f));
		SlotWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		JutsuSlotWidgets.Add(SlotWidget);
		JutsuSlots.Add(Place(SlotWidget, BottomCenter, Pos, UNinjaJutsuSlotWidget::BoxSizeFor(Size), UNinjaJutsuSlotWidget::DiscAlignment()));
		JutsuSlotBase.Add(Pos);
	}
	// Cargas da substituição (Q apanhando): duas pílulas em cima da barra, sobem junto com ela
	SubstitutionPips.Reset();
	for (int32 k = 0; k < 2; ++k)
	{
		const FVector2D Pos(-12.f + k * 24.f, CenterY - Size * 0.72f - 12.f);   // acima da moldura
		UImage* Pip = MakeImage(Rounded(White, 0.f, FLinearColor::Transparent, 0.f, true));
		SubstitutionPips.Add(Pip);
		JutsuSlots.Add(Place(Pip, BottomCenter, Pos, FVector2D(18.f, 6.f), Center));
		JutsuSlotBase.Add(Pos);
	}
	JutsuOffset = JutsuHiddenOffset;

	// =========================== Barrinha do super pulo (só carregando) e contador de golpes ===========================
	// Vertical e fina, ao lado da cabeça do personagem (segue ele na tela): vidro escuro translúcido com contorno claro,
	// energia azul em degradê (mais clara em cima) e um brilho suave atrás que cresce com a carga e pulsa quando enche
	SuperJumpGlow = MakeImage(FSlateBrush());
	{
		FSlateBrush Glow;
		Glow.SetResourceObject(MakeTexture(64, [](float U, float V)
		{
			const float DX = (U - 0.5f) / 0.5f, DY = (V - 0.5f) / 0.5f;   // elipse que ocupa a imagem inteira
			return FMath::Pow(FMath::Clamp(1.f - FMath::Sqrt(DX * DX + DY * DY), 0.f, 1.f), 2.f);
		}));
		Glow.TintColor = FSlateColor(FLinearColor(0.15f, 0.55f, 1.f, 1.f));
		SuperJumpGlow->SetBrush(Glow);
	}
	SuperJumpGlow->SetVisibility(ESlateVisibility::Collapsed);
	SuperJumpGlowSlot = Place(SuperJumpGlow, FVector2D::ZeroVector, FVector2D::ZeroVector, SuperJumpBarSize * FVector2D(6.f, 1.5f), FVector2D(0.5f, 0.5f));

	SuperJumpBar = WidgetTree->ConstructWidget<UProgressBar>();
	{
		FProgressBarStyle Style;
		Style.SetBackgroundImage(Rounded(FLinearColor(0.01f, 0.03f, 0.08f, 0.35f), SuperJumpBarSize.X * 0.5f, FLinearColor(0.75f, 0.9f, 1.f, 0.55f), 1.f));
		FSlateBrush Fill = Rounded(White, SuperJumpBarSize.X * 0.5f);   // cantos = metade da LARGURA (o "redondo" da Slate usa a altura e vira losango)
		Fill.SetResourceObject(MakeTextureRGBA(8, 128, [](float U, float V)
		{
			const float H = 1.f - V;   // 0 embaixo, 1 em cima
			const FLinearColor Deep(0.02f, 0.18f, 1.f), Mid(0.1f, 0.6f, 1.f), Top(0.75f, 0.95f, 1.f);
			FLinearColor C = H < 0.6f ? FMath::Lerp(Deep, Mid, H / 0.6f) : FMath::Lerp(Mid, Top, (H - 0.6f) / 0.4f);
			const float Core = 1.f - FMath::Abs(U - 0.5f) * 1.2f;   // miolo mais claro que as beiradas
			C = FMath::Lerp(C, FLinearColor::White, 0.25f * Core * Core);
			C.A = 0.55f + 0.3f * H;
			return C;
		}));
		Style.SetFillImage(Fill);
		Style.SetMarqueeImage(Fill);
		SuperJumpBar->SetWidgetStyle(Style);
		SuperJumpBar->SetFillColorAndOpacity(White);
		SuperJumpBar->SetBarFillType(EProgressBarFillType::BottomToTop);
	}
	SuperJumpBar->SetVisibility(ESlateVisibility::Collapsed);
	SuperJumpSlot = Place(SuperJumpBar, FVector2D::ZeroVector, FVector2D::ZeroVector, SuperJumpBarSize, FVector2D(0.5f, 1.f));

	// =========================== Chamas em órbita: contador + barra do tempo (centro de baixo) e mira ===========================
	OrbitText = MakeText(TEXT(""), 17.f, White, TEXT("Bold"));
	OrbitText->SetJustification(ETextJustify::Center);
	OrbitText->SetVisibility(ESlateVisibility::Collapsed);
	Place(OrbitText, BottomCenter, FVector2D(0.f, CenterY - Size * 0.72f - 58.f), FVector2D::ZeroVector, FVector2D(0.5f, 1.f));
	OrbitBar = MakeBar(FLinearColor(1.f, 0.38f, 0.12f), 2.f);
	OrbitBar->SetVisibility(ESlateVisibility::Collapsed);
	Place(OrbitBar, BottomCenter, FVector2D(0.f, CenterY - Size * 0.72f - 50.f), FVector2D(220.f, 4.f), FVector2D(0.5f, 0.f));
	OrbitCrosshair = MakeImage(Rounded(FLinearColor::Transparent, 15.f, FLinearColor(1.f, 1.f, 1.f, 0.85f), 2.f, true));
	OrbitCrosshair->SetVisibility(ESlateVisibility::Collapsed);
	Place(OrbitCrosshair, Center, FVector2D::ZeroVector, FVector2D(30.f, 30.f), FVector2D(0.5f, 0.5f));
	OrbitCrosshairDot = MakeImage(Rounded(FLinearColor(1.f, 1.f, 1.f, 0.9f), 2.f, FLinearColor::Transparent, 0.f, true));
	OrbitCrosshairDot->SetVisibility(ESlateVisibility::Collapsed);
	Place(OrbitCrosshairDot, Center, FVector2D::ZeroVector, FVector2D(4.f, 4.f), FVector2D(0.5f, 0.5f));

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
		SetFill(HealthFill, HealthFullTexture, Max > 0.f ? Current / Max : 0.f, NinjaLife::Health.GetSize().X * K(), FLinearColor::White);
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
		// Fora da batalha: barra cinza com "%"; na batalha: barra azul com "atual / máximo"
		const bool bBattle = ShownBattle == 1;
		const float Percent = Max > 0.f ? Current / Max : 0.f;
		SetFill(ChakraFill, bBattle ? ChakraBattleFullTexture : ChakraFullTexture, Percent, NinjaLife::Chakra.GetSize().X * K(), FLinearColor::White);
		ChakraText->SetText(FText::FromString(bBattle ? FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(Current), FMath::RoundToInt(Max))
			: FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Percent * 100.f))));
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
		const FString Name = !DisplayName.IsEmpty() ? DisplayName : PS->GetPlayerName().IsEmpty() ? TEXT("Ninja") : PS->GetPlayerName();
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

	// Bloco de vida: visual de batalha (chakra azul) ou normal (chakra cinza, moedas)
	const bool bBattle = Ninja->IsInBattleMode();
	if (int32(bBattle) != ShownBattle)
	{
		ApplyBattleLook(bBattle);
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
	// Jutsus da barra: arte/custo do Data Asset do slot (se houver), cooldown pelo GAS e pulso ao usar
	ShownSlotJutsu.SetNum(JutsuSlotWidgets.Num());
	ShownJutsuUses.SetNum(JutsuSlotWidgets.Num());
	for (int32 i = 0; i < JutsuSlotWidgets.Num(); ++i)
	{
		const UNinjaJutsuData* Jutsu = Ninja->GetJutsuInSlot(i);
		if (Jutsu && ShownSlotJutsu[i].Get() != Jutsu)
		{
			ShownSlotJutsu[i] = Jutsu;
			FNinjaJutsuSlotData Data;
			Data.Icon = Jutsu->Icon;
			Data.ThemeColor = Jutsu->ThemeColor;
			Data.Number = FMath::RoundToInt(Jutsu->ChakraCost);
			JutsuSlotWidgets[i]->SetData(Data);
		}
		if (Jutsu)
		{
			float Duration = 0.f;
			const float Remaining = Ninja->GetJutsuCooldownRemaining(i, Duration);
			if (Remaining > 0.f)
			{
				JutsuSlotWidgets[i]->SetCooldown(Remaining, Duration);
			}
		}
		const int32 Uses = Ninja->GetJutsuUseCount(i);
		if (Uses != ShownJutsuUses[i])
		{
			if (Uses > ShownJutsuUses[i])
			{
				JutsuSlotWidgets[i]->PlayUsed();
			}
			ShownJutsuUses[i] = Uses;
		}
	}
	for (UNinjaJutsuSlotWidget* SlotWidget : JutsuSlotWidgets)
	{
		SlotWidget->SetHasChakra(Chakra >= SlotWidget->GetNumber());   // número da etiqueta = custo de chakra
		SlotWidget->TickSlot(InDeltaTime);
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

	// Super pulo carregando: barrinha ao lado da cabeça do personagem na tela
	if (SuperJumpBar && SuperJumpSlot)
	{
		FVector2D HeadOnScreen;
		// Osso da cabeça (agachado carregando, o topo da cápsula fica bem acima dela)
		const FVector Head = Ninja->GetMesh()->GetSocketLocation(TEXT("head")) + Ninja->GetActorUpVector() * 20.f;
		const bool bShow = Ninja->IsChargingSuperJump()
			&& UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(GetOwningPlayer(), Head, HeadOnScreen, false);
		SuperJumpBar->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		SuperJumpGlow->SetVisibility(SuperJumpBar->GetVisibility());
		if (bShow)
		{
			const float Charge = Ninja->GetSuperJumpCharge();
			const float Time = GetWorld()->GetRealTimeSeconds();
			const float Pulse = Charge >= 1.f ? 0.5f + 0.5f * FMath::Sin(Time * 12.f) : 0.f;   // cheia: pisca rápido
			SuperJumpSlot->SetPosition(HeadOnScreen + SuperJumpBarOffset);
			SuperJumpBar->SetPercent(Charge);
			SuperJumpBar->SetFillColorAndOpacity(FMath::Lerp(FLinearColor::White, FLinearColor(1.6f, 1.8f, 2.f, 1.f), Pulse));
			// O brilho fica atrás da parte cheia e cresce com a carga
			SuperJumpGlowSlot->SetPosition(HeadOnScreen + SuperJumpBarOffset - FVector2D(0.f, SuperJumpBarSize.Y * Charge * 0.5f));
			SuperJumpGlowSlot->SetSize(FVector2D(SuperJumpBarSize.X * (5.f + 2.f * Pulse), SuperJumpBarSize.Y * (0.35f + 1.f * Charge)));
			SuperJumpGlow->SetRenderOpacity(0.35f + 0.5f * Charge + 0.15f * Pulse);
		}
	}

	// Chamas em órbita: "Nome n/N", barra do tempo que falta e mira no centro
	if (OrbitText && OrbitBar && OrbitCrosshair)
	{
		const ANinjaFlameOrbit* Orbit = Ninja->GetActiveFlameOrbit();
		const bool bShow = Orbit && Orbit->IsActive();
		const ESlateVisibility Vis = bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		OrbitText->SetVisibility(Vis);
		OrbitBar->SetVisibility(Vis);
		OrbitCrosshair->SetVisibility(Vis);
		OrbitCrosshairDot->SetVisibility(Vis);
		if (bShow)
		{
			if (Orbit->GetFlamesLeft() != ShownOrbitFlames)
			{
				ShownOrbitFlames = Orbit->GetFlamesLeft();
				const UNinjaJutsuData* OrbitJutsu = Orbit->GetJutsu();
				const FString Label = OrbitJutsu && !OrbitJutsu->OrbitLabel.IsEmpty() ? OrbitJutsu->OrbitLabel.ToString() : FString(TEXT("Chamas"));
				OrbitText->SetText(FText::FromString(FString::Printf(TEXT("%s %d/%d"), *Label, ShownOrbitFlames, Orbit->GetFlameCount())));
			}
			float Duration = 1.f;
			const float Left = Orbit->GetTimeLeft(Duration);
			OrbitBar->SetPercent(Left / FMath::Max(Duration, 0.01f));
		}
		else
		{
			ShownOrbitFlames = -1;
		}
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
	// Arco de XP no anel do retrato (o trilho escuro já está na imagem base): mesmo raio e cor do arco da imagem do Felipe,
	// começando logo depois do selo, subindo pela esquerda, topo e descendo pela direita até o outro lado do selo
	using namespace NinjaLife;
	ShownXp = ExperiencePercent;
	const float P = FMath::Clamp(ExperiencePercent, 0.f, 1.f);
	const int32 Px = FMath::Clamp(FMath::CeilToInt(RingOuter * 2.f * K() * 1.5f), 32, 1024);   // 1,5x o tamanho na tela
	const float Inner = RingInner / RingOuter;
	const float EndDeg = RingStartDeg + (RingEndDeg - RingStartDeg) * P;
	const FLinearColor Arc(RingColor.R / 255.f, RingColor.G / 255.f, RingColor.B / 255.f, 1.f);
	RingTexture = NinjaHUD::MakeTextureRGBA(Px, Px, [Inner, EndDeg, Arc, P](float U, float V)
	{
		const FVector2f D(U - 0.5f, V - 0.5f);
		const float R = D.Size() * 2.f;
		float Ang = FMath::RadiansToDegrees(FMath::Atan2(-D.X, D.Y));
		Ang = Ang < 0.f ? Ang + 360.f : Ang;
		const bool bOn = P > 0.f && R >= Inner && R <= 1.f && Ang >= RingStartDeg && Ang <= EndDeg;
		return bOn ? Arc : FLinearColor(0.f, 0.f, 0.f, 0.f);
	});
	if (XpRing)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(RingTexture);
		XpRing->SetBrush(Brush);
	}
}

void UNinjaHUDWidget::ApplyBattleLook(bool bBattle)
{
	using namespace NinjaLife;
	ShownBattle = bBattle ? 1 : 0;
	if (ChakraEmpty)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(bBattle ? ChakraBattleEmptyTexture : ChakraEmptyTexture);
		ChakraEmpty->SetBrush(Brush);
	}
	if (ChakraText)
	{
		ChakraText->SetFont(HudFontInfo(TEXT("ExtraBold"), (bBattle ? ChakraBattleSize : ChakraSize) * K()));
		if (UCanvasPanelSlot* TextSlot = Cast<UCanvasPanelSlot>(ChakraText->Slot))
		{
			TextSlot->SetPosition(((bBattle ? ChakraBattlePos : ChakraPos) - Origin) * K());
		}
	}
	for (UWidget* W : CoinWidgets)
	{
		W->SetVisibility(bBattle ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (ChakraMax >= 0.f)
	{
		SetChakra(Chakra, ChakraMax);
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
	using namespace NinjaLife;

	// Painel com as medidas da imagem do Felipe (px da imagem x LifeBlockScale = px na tela), preso no canto de baixo à esquerda
	UCanvasPanel* Block = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BlocoDeVida"));
	UCanvasPanelSlot* BlockSlot = Root->AddChildToCanvas(Block);
	BlockSlot->SetAnchors(FAnchors(0.f, 1.f));
	BlockSlot->SetAlignment(FVector2D(0.f, 1.f));
	BlockSlot->SetPosition(LifeBlockPosition);
	BlockSlot->SetSize(Size * K());

	auto PutRect = [this, Block](UWidget* Widget, const FBox2D& Rect)
	{
		UCanvasPanelSlot* PanelSlot = Block->AddChildToCanvas(Widget);
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetPosition((Rect.Min - Origin) * K());
		PanelSlot->SetSize(Rect.GetSize() * K());
		return PanelSlot;
	};
	auto PutAt = [this, Block](UWidget* Widget, const FVector2D& Point, const FVector2D& Alignment)
	{
		UCanvasPanelSlot* PanelSlot = Block->AddChildToCanvas(Widget);
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAlignment(Alignment);
		PanelSlot->SetPosition((Point - Origin) * K());
		PanelSlot->SetAutoSize(true);
		return PanelSlot;
	};
	auto Picture = [this](UTexture2D* Texture)
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Image->SetBrush(Brush);
		Image->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		return Image;
	};
	auto Text = [this](const FString& Words, const TCHAR* Face, float SizeImagePx, const FLinearColor& Color, const FVector2D& ShadowOffset, const FLinearColor& Shadow)
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetText(FText::FromString(Words));
		T->SetFont(HudFontInfo(Face, SizeImagePx * K()));
		T->SetColorAndOpacity(FSlateColor(Color));
		T->SetShadowOffset(ShadowOffset);
		T->SetShadowColorAndOpacity(Shadow);
		return T;
	};
	const FLinearColor Relief = FLinearColor::FromSRGBColor(RaisedShadow);
	const FVector2D ReliefOffset(0.f, FMath::Max(1.f, 6.f * K()));
	const FLinearColor SoftShadow(0.f, 0.f, 0.f, 0.35f);

	// Camadas, de trás para frente: base (retrato, trilho do anel, caixa de voz) -> arco de XP -> selo + nível -> barras -> textos -> moedas
	PutRect(Picture(LifeBaseTexture), Base);
	XpRing = WidgetTree->ConstructWidget<UImage>();
	PutRect(XpRing.Get(), FBox2D(RingCenter - FVector2D(RingOuter), RingCenter + FVector2D(RingOuter)));
	UpdateXpRing();
	PutRect(Picture(LevelBadgeTexture), Badge);
	LevelText = Text(FString::FromInt(PlayerLevel), TEXT("ExtraBold"), LevelSize, FLinearColor::White, FVector2D::ZeroVector, FLinearColor::Transparent);
	PutAt(LevelText.Get(), BadgeCenter, FVector2D(0.5f, 0.5f));

	// Vida: barra vazia (escurecida) inteira e a cheia recortada pela porcentagem
	PutRect(Picture(HealthEmptyTexture), NinjaLife::Health);
	HealthFill = Picture(HealthFullTexture);
	PutRect(HealthFill.Get(), NinjaLife::Health);

	// Chakra: idem; as texturas trocam com o modo de batalha (ApplyBattleLook)
	ChakraEmpty = Picture(ChakraEmptyTexture);
	PutRect(ChakraEmpty.Get(), NinjaLife::Chakra);
	ChakraFill = Picture(ChakraFullTexture);
	PutRect(ChakraFill.Get(), NinjaLife::Chakra);

	// Nome (com relevo cinza embaixo, como na imagem)
	NameText = Text(TEXT("Ninja"), TEXT("ExtraBold"), NameSize, FLinearColor::White, ReliefOffset, Relief);
	PutAt(NameText.Get(), NamePos, FVector2D(0.f, 0.5f));

	// "750" grande + "/750" menor na mesma linha de base
	HealthText = Text(TEXT("0"), TEXT("ExtraBold"), HealthSize, FLinearColor::White, FVector2D(0.f, 1.f), SoftShadow);
	HealthMaxText = Text(TEXT("/0"), TEXT("Bold"), HealthMaxSize, FLinearColor::White, FVector2D(0.f, 1.f), SoftShadow);
	UHorizontalBox* HpRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	HpRow->AddChildToHorizontalBox(HealthText)->SetVerticalAlignment(VAlign_Bottom);
	UHorizontalBoxSlot* MaxSlot = HpRow->AddChildToHorizontalBox(HealthMaxText);
	MaxSlot->SetVerticalAlignment(VAlign_Bottom);
	MaxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, (HealthSize - HealthMaxSize) * 0.25f * K()));   // linhas de base juntas (descendentes diferentes)
	PutAt(HpRow, HealthPos, FVector2D(0.f, 0.5f));

	// Texto do chakra, branco nos dois modos (alinhado à direita; posição e tamanho trocam com o modo de batalha)
	ChakraText = Text(TEXT("100%"), TEXT("ExtraBold"), ChakraSize, FLinearColor::White, FVector2D(0.f, 1.f), SoftShadow);
	PutAt(ChakraText.Get(), ChakraPos, FVector2D(1.f, 0.5f));

	// Linha de baixo fora da batalha: moeda + quantidade
	CoinWidgets.Reset();
	UImage* CoinImage = Picture(CoinIconTexture);
	PutRect(CoinImage, Coin);
	CoinText = Text(FormatThousands(Coins), TEXT("ExtraBold"), CoinSize, FLinearColor::White, ReliefOffset, Relief);
	PutAt(CoinText.Get(), CoinTextPos, FVector2D(0.f, 0.5f));
	ShownCoins = Coins;
	CoinWidgets = { CoinImage, CoinText };

	ShownBattle = -1;
	ApplyBattleLook(false);
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

void UNinjaHUDWidget::SetSelectedJutsu(int32 Index)
{
	for (int32 i = 0; i < JutsuSlotWidgets.Num(); ++i)
	{
		JutsuSlotWidgets[i]->SetSelected(i == Index);
	}
}

void UNinjaHUDWidget::SetJutsuCooldown(int32 Index, float Remaining, float Duration)
{
	if (JutsuSlotWidgets.IsValidIndex(Index))
	{
		JutsuSlotWidgets[Index]->SetCooldown(Remaining, Duration);
	}
}

void UNinjaHUDWidget::SetJutsuHasChakra(int32 Index, bool bHasChakra)
{
	if (JutsuSlotWidgets.IsValidIndex(Index))
	{
		JutsuSlotWidgets[Index]->SetHasChakra(bHasChakra);
	}
}

void UNinjaHUDWidget::SetJutsuNumber(int32 Index, int32 Number)
{
	if (JutsuSlotWidgets.IsValidIndex(Index))
	{
		JutsuSlotWidgets[Index]->SetNumber(Number);
	}
}

void UNinjaHUDWidget::PlayJutsuUsed(int32 Index)
{
	if (JutsuSlotWidgets.IsValidIndex(Index))
	{
		JutsuSlotWidgets[Index]->PlayUsed();
	}
}
