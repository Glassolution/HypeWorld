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
	const FLinearColor White(1.f, 1.f, 1.f, 1.f);
	const FLinearColor Ring(0.55f, 0.84f, 0.98f, 1.f);			// anel azul-claro do retrato
	const FLinearColor HealthPink(0.91f, 0.21f, 0.37f, 1.f);	// barra de vida
	const FLinearColor ChakraGray(0.52f, 0.58f, 0.56f, 0.92f);	// barra de chakra (cinza esverdeado, como na referência)
	const FLinearColor ChakraBlue(0.45f, 0.8f, 1.f, 1.f);
	const FLinearColor Track(0.f, 0.f, 0.f, 0.38f);
	const FLinearColor Badge(0.07f, 0.07f, 0.08f, 0.95f);
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

	// =========================== Canto de baixo à esquerda: retrato, nome, vida, chakra, moedas ===========================
	const FVector2D Portrait(115.f, -115.f);
	Place(MakeImage(Rounded(Ring, 0.f, FLinearColor::Transparent, 0.f, true)), BottomLeft, Portrait, FVector2D(100.f, 100.f), Center);
	Place(MakeImage(Rounded(FLinearColor(0.08f, 0.08f, 0.1f, 1.f), 0.f, FLinearColor::Transparent, 0.f, true)), BottomLeft, Portrait, FVector2D(88.f, 88.f), Center);
	if (PortraitTexture)
	{
		Place(MakeImage(CircleImage(PortraitTexture)), BottomLeft, Portrait, FVector2D(86.f, 86.f), Center);
	}
	Place(MakeImage(Rounded(Badge, 0.f, FLinearColor(1.f, 1.f, 1.f, 0.15f), 1.f, true)), BottomLeft, FVector2D(115.f, -68.f), FVector2D(30.f, 30.f), Center);
	LevelText = MakeText(FString::FromInt(PlayerLevel), 12.f);
	Place(LevelText, BottomLeft, FVector2D(115.f, -68.f), FVector2D::ZeroVector, Center);

	NameText = MakeText(TEXT("Ninja"), 17.f);
	Place(NameText, BottomLeft, FVector2D(164.f, -162.f), FVector2D::ZeroVector, LeftMid);

	HealthBar = MakeBar(HealthPink, 4.f);
	Place(HealthBar, BottomLeft, FVector2D(164.f, -140.f), FVector2D(246.f, 24.f), FVector2D(0.f, 0.f));
	HealthText = MakeText(TEXT("0"), 18.f);
	Place(HealthText, BottomLeft, FVector2D(174.f, -128.f), FVector2D::ZeroVector, LeftMid);
	HealthMaxText = MakeText(TEXT("/0"), 11.f);
	Place(HealthMaxText, BottomLeft, FVector2D(214.f, -125.f), FVector2D::ZeroVector, LeftMid);

	ChakraBar = MakeBar(ChakraGray, 4.f);
	Place(ChakraBar, BottomLeft, FVector2D(164.f, -110.f), FVector2D(152.f, 22.f), FVector2D(0.f, 0.f));
	FSlateBrush Swirl;
	Swirl.SetResourceObject(SwirlTexture);
	Swirl.TintColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.6f));
	Place(MakeImage(Swirl), BottomLeft, FVector2D(184.f, -99.f), FVector2D(16.f, 16.f), Center);
	ChakraText = MakeText(TEXT("100%"), 12.f);
	Place(ChakraText, BottomLeft, FVector2D(308.f, -99.f), FVector2D::ZeroVector, FVector2D(1.f, 0.5f));

	// Caixa preta à direita do chakra (na referência é o microfone): aqui, a infusão de chakra nos pés (V)
	UImage* InfusionBg = MakeImage(Rounded(FLinearColor(0.02f, 0.02f, 0.03f, 0.88f), 4.f));
	InfusionBox = InfusionBg;
	Place(InfusionBg, BottomLeft, FVector2D(324.f, -110.f), FVector2D(86.f, 22.f));
	InfusionText = MakeText(TEXT("INFUSÃO"), 10.f, ChakraBlue);
	Place(InfusionText, BottomLeft, FVector2D(367.f, -99.f), FVector2D::ZeroVector, Center);

	Place(MakeImage(Rounded(Gold, 0.f, FLinearColor(0.6f, 0.42f, 0.05f, 1.f), 2.f, true)), BottomLeft, FVector2D(172.f, -68.f), FVector2D(16.f, 16.f), Center);
	CoinText = MakeText(TEXT("0"), 14.f);
	Place(CoinText, BottomLeft, FVector2D(188.f, -68.f), FVector2D::ZeroVector, LeftMid);

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
	if (HealthBar)
	{
		HealthBar->SetPercent(Max > 0.f ? Current / Max : 0.f);
		HealthText->SetText(FText::AsNumber(FMath::RoundToInt(Current)));
		HealthMaxText->SetText(FText::FromString(FString::Printf(TEXT("/%d"), FMath::RoundToInt(Max))));
	}
}

void UNinjaHUDWidget::SetChakra(float Current, float Max)
{
	Chakra = Current;
	ChakraMax = Max;
	if (ChakraBar)
	{
		const float Percent = Max > 0.f ? Current / Max : 0.f;
		ChakraBar->SetPercent(Percent);
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

	// Infusão de chakra: etiqueta acesa/apagada (some se a passiva não foi aprendida)
	const int32 Infusion = !Ninja->HasLearnedPassive(ENinjaPassive::ChakraFeet) ? -2 : (Ninja->IsPassiveEnabled(ENinjaPassive::ChakraFeet) ? 1 : 0);
	if (Infusion != ShownInfusion && InfusionText)
	{
		ShownInfusion = Infusion;
		const ESlateVisibility Show = Infusion == -2 ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible;
		InfusionBox->SetVisibility(Show);
		InfusionText->SetVisibility(Show);
		InfusionText->SetColorAndOpacity(FSlateColor(Infusion == 1 ? NinjaHUD::ChakraBlue : FLinearColor(0.42f, 0.42f, 0.45f, 1.f)));
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
