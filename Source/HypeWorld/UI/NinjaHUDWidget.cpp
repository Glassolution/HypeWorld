// HUD do ninja montado inteiro em C++ (ver o .h).

#include "NinjaHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "HypeWorldCharacter.h"
#include "NinjaAttributeSet.h"

// ---------------------------------------------------------------------------------------------------------------------
// Peças de estilo (cores, pincéis, fontes) — o layout foi pensado em 1920x1080; a Unreal escala para outras telas
// ---------------------------------------------------------------------------------------------------------------------

namespace NinjaHUD
{
	const FLinearColor White(1.f, 1.f, 1.f, 1.f);
	const FLinearColor SoftWhite(0.85f, 0.85f, 0.85f, 1.f);
	const FLinearColor Gold(0.80f, 0.66f, 0.45f, 1.f);
	const FLinearColor HealthRed(0.83f, 0.33f, 0.33f, 1.f);
	const FLinearColor ChakraBlue(0.40f, 0.64f, 0.76f, 1.f);
	const FLinearColor DarkFill(0.07f, 0.06f, 0.05f, 0.80f);
	const FLinearColor Track(0.f, 0.f, 0.f, 0.45f);

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

	FSlateFontInfo Font(const TCHAR* Style, float Size)
	{
		FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(Style, Size);
		Info.OutlineSettings.OutlineSize = 1;
		Info.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.55f);
		return Info;
	}

	/** Textura branca gerada por código; a função diz a opacidade de cada pixel (0..1). */
	UTexture2D* MakeTexture(int32 Width, int32 Height, TFunctionRef<float(float U, float V)> Alpha)
	{
		TArray<FColor> Pixels;
		Pixels.SetNumUninitialized(Width * Height);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				// 4 amostras por pixel para a borda ficar suave
				float A = 0.f;
				for (int32 S = 0; S < 4; ++S)
				{
					A += Alpha((X + 0.25f + 0.5f * (S % 2)) / Width, (Y + 0.25f + 0.5f * (S / 2)) / Height);
				}
				Pixels[Y * Width + X] = FColor(255, 255, 255, (uint8)FMath::RoundToInt(FMath::Clamp(A / 4.f, 0.f, 1.f) * 255.f));
			}
		}
		const TConstArrayView64<uint8> Bytes(reinterpret_cast<const uint8*>(Pixels.GetData()), Pixels.Num() * sizeof(FColor));
		return UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8, NAME_None, Bytes);
	}

	bool InsideTriangle(float X, float Y, const FVector2f& A, const FVector2f& B, const FVector2f& C)
	{
		auto Side = [](const FVector2f& P, const FVector2f& Q, float X, float Y) { return (Q.X - P.X) * (Y - P.Y) - (Q.Y - P.Y) * (X - P.X); };
		const float D1 = Side(A, B, X, Y), D2 = Side(B, C, X, Y), D3 = Side(C, A, X, Y);
		return !((D1 < 0 || D2 < 0 || D3 < 0) && (D1 > 0 || D2 > 0 || D3 > 0));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Montagem
// ---------------------------------------------------------------------------------------------------------------------

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
	auto MakeText = [this](const FString& Words, float Size, const FLinearColor& Color = White, const TCHAR* Style = TEXT("Regular"))
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Words));
		Text->SetFont(Font(Style, Size));
		Text->SetColorAndOpacity(FSlateColor(Color));
		return Text;
	};
	/** Quadradinho de tecla com a letra no meio. */
	auto MakeKey = [this, &MakeText](const FString& Letter, float Size)
	{
		UBorder* Box = WidgetTree->ConstructWidget<UBorder>();
		Box->SetBrush(Rounded(FLinearColor(0.05f, 0.05f, 0.05f, 0.70f), 6.f, FLinearColor(1.f, 1.f, 1.f, 0.55f), 1.5f));
		Box->SetPadding(FMargin(0.f));
		Box->SetHorizontalAlignment(HAlign_Center);
		Box->SetVerticalAlignment(VAlign_Center);
		Box->SetContent(MakeText(Letter, Size));
		return Box;
	};
	auto MakeBar = [this](const FLinearColor& Fill)
	{
		UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>();
		FProgressBarStyle Style;
		Style.SetBackgroundImage(Rounded(Track, 0.f, FLinearColor::Transparent, 0.f, true));
		Style.SetFillImage(Rounded(Fill, 0.f, FLinearColor::Transparent, 0.f, true));
		Style.SetMarqueeImage(Rounded(Fill, 0.f, FLinearColor::Transparent, 0.f, true));
		Bar->SetWidgetStyle(Style);
		Bar->SetFillColorAndOpacity(White);
		return Bar;
	};

	FadeTexture = MakeTexture(256, 4, [](float U, float V) { return 1.f - FMath::SmoothStep(0.35f, 1.f, U); });
	ArrowTexture = MakeTexture(64, 64, [](float U, float V)
	{
		// ponta de flecha: triângulo com um entalhe embaixo
		const float X = U * 64.f, Y = V * 64.f;
		const bool bArrow = InsideTriangle(X, Y, FVector2f(32, 4), FVector2f(56, 60), FVector2f(8, 60));
		const bool bNotch = InsideTriangle(X, Y, FVector2f(32, 44), FVector2f(58, 64), FVector2f(6, 64));
		return (bArrow && !bNotch) ? 1.f : 0.f;
	});

	const FVector2D TopLeft(0, 0), TopRight(1, 0), BottomLeft(0, 1), BottomCenter(0.5f, 1);
	const FVector2D Center(0.5f, 0.5f);

	// --- Topo esquerdo: retrato, nível, vida, chakra ---
	FSlateBrush Fade;
	Fade.SetResourceObject(FadeTexture);
	Fade.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Place(MakeImage(Fade), TopLeft, FVector2D(100, 50), FVector2D(520, 116));

	Place(MakeImage(Rounded(DarkFill, 0.f, Gold, 3.f, true)), TopLeft, FVector2D(36, 28), FVector2D(136, 136));
	if (PortraitTexture)
	{
		Place(MakeImage(CircleImage(PortraitTexture)), TopLeft, FVector2D(44, 36), FVector2D(120, 120));
	}

	LevelText = MakeText(FString(), 20.f);
	Place(LevelText, TopLeft, FVector2D(200, 58), FVector2D::ZeroVector);

	HealthBar = MakeBar(HealthRed);
	Place(HealthBar, TopLeft, FVector2D(190, 92), FVector2D(380, 28));
	HealthText = MakeText(FString(), 16.f);
	Place(HealthText, TopLeft, FVector2D(206, 106), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));

	ChakraBar = MakeBar(ChakraBlue);
	Place(ChakraBar, TopLeft, FVector2D(182, 128), FVector2D(350, 28));
	ChakraText = MakeText(FString(), 16.f);
	Place(ChakraText, TopLeft, FVector2D(198, 142), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));

	// --- Missão ---
	FSlateBrush QuestBand = Fade;
	QuestBand.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.60f));
	Place(MakeImage(QuestBand), TopLeft, FVector2D(0, 236), FVector2D(560, 100));

	UImage* Diamond = MakeImage(Rounded(FLinearColor::Transparent, 2.f, White, 2.f));
	Diamond->SetRenderTransformAngle(45.f);
	Place(Diamond, TopLeft, FVector2D(62, 266), FVector2D(18, 18), Center);
	UImage* DiamondDot = MakeImage(Rounded(White, 1.f));
	DiamondDot->SetRenderTransformAngle(45.f);
	Place(DiamondDot, TopLeft, FVector2D(62, 266), FVector2D(7, 7), Center);

	QuestTitleText = MakeText(TEXT("Treine suas habilidades"), 20.f);
	Place(QuestTitleText, TopLeft, FVector2D(90, 266), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
	Place(MakeImage(Rounded(FLinearColor::Transparent, 0.f, SoftWhite, 1.5f, true)), TopLeft, FVector2D(98, 304), FVector2D(13, 13), Center);
	QuestStepText = MakeText(TEXT("Fale com o mestre no dojo"), 15.f, SoftWhite);
	Place(QuestStepText, TopLeft, FVector2D(114, 304), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));

	// --- Topo direito: minimapa (círculo de 224 px, centro em (-140, 142) a partir do canto) ---
	const FVector2D MapCenter(-140, 142);
	Place(MakeImage(Rounded(FLinearColor(0.08f, 0.08f, 0.08f, 0.65f), 0.f, FLinearColor(0.9f, 0.9f, 0.9f, 0.9f), 3.f, true)), TopRight, MapCenter, FVector2D(224, 224), Center);
	MinimapImage = MakeImage(Rounded(FLinearColor(0.35f, 0.30f, 0.24f, 1.f), 0.f, FLinearColor::Transparent, 0.f, true));
	Place(MinimapImage, TopRight, MapCenter, FVector2D(208, 208), Center);
	Place(MakeImage(Rounded(FLinearColor::Transparent, 0.f, FLinearColor(1.f, 1.f, 1.f, 0.25f), 1.f, true)), TopRight, MapCenter, FVector2D(150, 150), Center);

	FSlateBrush Arrow;
	Arrow.SetResourceObject(ArrowTexture);
	MinimapArrow = MakeImage(Arrow);
	Place(MinimapArrow, TopRight, MapCenter, FVector2D(26, 26), Center);

	Place(MakeText(TEXT("N"), 15.f), TopRight, MapCenter + FVector2D(0, -112), FVector2D::ZeroVector, Center);
	Place(MakeText(TEXT("S"), 15.f), TopRight, MapCenter + FVector2D(0, 112), FVector2D::ZeroVector, Center);
	Place(MakeText(TEXT("W"), 15.f), TopRight, MapCenter + FVector2D(-112, 0), FVector2D::ZeroVector, Center);
	Place(MakeText(TEXT("E"), 15.f), TopRight, MapCenter + FVector2D(112, 0), FVector2D::ZeroVector, Center);

	// --- Baixo esquerdo: atalhos ---
	const TCHAR* HintKeys[] = { TEXT("LMB"), TEXT("F"), TEXT("RMB"), TEXT("C"), TEXT("Ctrl"), TEXT("M"), TEXT("I"), TEXT("J") };
	const TCHAR* HintLabels[] = { TEXT("Atacar"), TEXT("Golpe forte"), TEXT("Defender"), TEXT("Carregar chakra"), TEXT("Desviar"), TEXT("Mapa"), TEXT("Inventário"), TEXT("Missões") };
	for (int32 i = 0; i < 8; ++i)
	{
		const float Y = -394.f + i * 46.f;
		const float KeyWidth = FCString::Strlen(HintKeys[i]) > 1 ? 52.f : 32.f;
		Place(MakeKey(HintKeys[i], 13.f), BottomLeft, FVector2D(64, Y), FVector2D(KeyWidth, 32));
		Place(MakeText(HintLabels[i], 17.f, SoftWhite), BottomLeft, FVector2D(76 + KeyWidth, Y + 16.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
	}

	// --- Direita: contador de golpes seguidos ---
	ComboText = MakeText(TEXT(""), 44.f, FLinearColor(1.f, 0.62f, 0.15f), TEXT("Bold"));
	ComboText->SetShadowOffset(FVector2D(2.f, 2.f));
	ComboText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	ComboText->SetVisibility(ESlateVisibility::Collapsed);
	Place(ComboText, FVector2D(1.f, 0.5f), FVector2D(-90.f, -40.f), FVector2D::ZeroVector, FVector2D(1.f, 0.5f));

	// --- Baixo centro: os 4 poderes ---
	const TCHAR* AbilityKeyLetters[] = { TEXT("Q"), TEXT("E"), TEXT("R"), TEXT("T") };
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector2D Circle(-156.f + i * 104.f, -112.f);
		Place(MakeImage(Rounded(DarkFill, 0.f, FLinearColor(0.92f, 0.88f, 0.80f, 0.95f), 2.5f, true)), BottomCenter, Circle, FVector2D(92, 92), Center);
		if (AbilityIcons.IsValidIndex(i) && AbilityIcons[i])
		{
			Place(MakeImage(CircleImage(AbilityIcons[i])), BottomCenter, Circle, FVector2D(84, 84), Center);
		}
		Place(MakeKey(AbilityKeyLetters[i], 14.f), BottomCenter, FVector2D(Circle.X, -44.f), FVector2D(30, 30), Center);
	}

	SetHealth(Health, HealthMax);
	SetChakra(Chakra, ChakraMax);
	SetPlayerLevel(PlayerLevel);
}

// ---------------------------------------------------------------------------------------------------------------------
// Valores
// ---------------------------------------------------------------------------------------------------------------------

void UNinjaHUDWidget::SetHealth(float Current, float Max)
{
	Health = Current;
	HealthMax = Max;
	if (HealthBar)
	{
		HealthBar->SetPercent(Max > 0.f ? Current / Max : 0.f);
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(Current), FMath::RoundToInt(Max))));
	}
}

void UNinjaHUDWidget::SetChakra(float Current, float Max)
{
	Chakra = Current;
	ChakraMax = Max;
	if (ChakraBar)
	{
		ChakraBar->SetPercent(Max > 0.f ? Current / Max : 0.f);
		ChakraText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(Current), FMath::RoundToInt(Max))));
	}
}

void UNinjaHUDWidget::SetPlayerLevel(int32 Level)
{
	PlayerLevel = Level;
	if (LevelText)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv. %d"), Level)));
	}
}

void UNinjaHUDWidget::SetQuest(const FText& Title, const FText& Step)
{
	if (QuestTitleText)
	{
		QuestTitleText->SetText(Title);
		QuestStepText->SetText(Step);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Minimapa: uma câmera ortográfica olhando de cima, seguindo o jogador (norte = +X do mundo, no topo)
// ---------------------------------------------------------------------------------------------------------------------

void UNinjaHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	APlayerController* PC = GetOwningPlayer();
	if (!PC || !MinimapImage)
	{
		return;
	}

	MinimapTarget = NewObject<UTextureRenderTarget2D>(this);
	MinimapTarget->ClearColor = FLinearColor(0.35f, 0.30f, 0.24f, 1.f);
	MinimapTarget->InitAutoFormat(256, 256);

	MinimapCapture = NewObject<USceneCaptureComponent2D>(PC);
	MinimapCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	MinimapCapture->OrthoWidth = MinimapWorldSize;
	MinimapCapture->CaptureSource = SCS_BaseColor;   // cores sem luz: mais leve e com cara de mapa
	MinimapCapture->bCaptureEveryFrame = false;
	MinimapCapture->bCaptureOnMovement = false;
	MinimapCapture->TextureTarget = MinimapTarget;
	MinimapCapture->ShowFlags.SetAtmosphere(false);
	MinimapCapture->ShowFlags.SetFog(false);
	MinimapCapture->RegisterComponent();

	// A câmera grava a transparência zerada; o material usa só as cores e recorta o círculo
	if (MinimapMaterial)
	{
		MinimapMID = UMaterialInstanceDynamic::Create(MinimapMaterial, this);
		MinimapMID->SetTextureParameterValue(TEXT("Map"), MinimapTarget);
		FSlateBrush MapBrush;
		MapBrush.SetResourceObject(MinimapMID);
		MinimapImage->SetBrush(MapBrush);
	}
	else
	{
		MinimapImage->SetBrush(NinjaHUD::CircleImage(MinimapTarget));
	}
	MinimapTimer = MinimapUpdateInterval; // desenha já no primeiro quadro
}

void UNinjaHUDWidget::NativeDestruct()
{
	if (MinimapCapture)
	{
		MinimapCapture->DestroyComponent();
		MinimapCapture = nullptr;
	}
	Super::NativeDestruct();
}

void UNinjaHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateMinimap(InDeltaTime);

	// Vida e chakra de verdade (atributos do GAS, replicados)
	if (const AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(GetOwningPlayerPawn()))
	{
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
}

void UNinjaHUDWidget::UpdateMinimap(float DeltaTime)
{
	APawn* Pawn = GetOwningPlayerPawn();
	if (!Pawn || !MinimapCapture)
	{
		return;
	}

	// A flecha gira com o jogador; o mapa fica parado com o norte para cima
	if (MinimapArrow)
	{
		MinimapArrow->SetRenderTransformAngle(Pawn->GetActorRotation().Yaw);
	}

	MinimapTimer += DeltaTime;
	if (MinimapTimer < MinimapUpdateInterval)
	{
		return;
	}
	MinimapTimer = 0.f;

	MinimapCapture->SetWorldLocationAndRotation(Pawn->GetActorLocation() + FVector(0.f, 0.f, 5000.f), FRotator(-90.f, 0.f, 0.f));
	MinimapCapture->CaptureScene();
}
