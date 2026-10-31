// Tela de seleção do personagem montada inteira em C++ (ver o .h). Layout em px de 1080p, medidos na imagem de
// referência do Felipe (1329x743 → ×1,45); a escala de DPI da Unreal ajusta para outras telas.

#include "HypeCharacterSelectWidget.h"
#include "HypeProfileSubsystem.h"
#include "Ninja/HypeAppearanceComponent.h"
#include "Combat/NinjaVisibleBody.h"
#include "HypeLoadingSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UnrealClient.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "HypeWorldCharacter.h"
#include "Combat/NinjaJutsuData.h"
#include "HypeWorldPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"

DEFINE_LOG_CATEGORY_STATIC(LogHypeCharacterSelect, Log, All);

namespace HypeSelect
{
    const TArray<FLinearColor> SkinColors = {
        FLinearColor(1.f, 1.f, 1.f), FLinearColor(0.94f, 0.87f, 0.79f),
        FLinearColor(0.84f, 0.69f, 0.54f), FLinearColor(0.69f, 0.50f, 0.35f),
        FLinearColor(0.50f, 0.32f, 0.21f), FLinearColor(0.33f, 0.19f, 0.12f),
        FLinearColor(0.19f, 0.10f, 0.065f), FLinearColor(0.09f, 0.045f, 0.03f)
    };
    const TArray<FLinearColor> HairColors = {
        FLinearColor(0.025f, 0.02f, 0.02f), FLinearColor(0.12f, 0.055f, 0.025f),
        FLinearColor(0.35f, 0.18f, 0.07f), FLinearColor(0.8f, 0.59f, 0.23f),
        FLinearColor(0.7f, 0.72f, 0.76f), FLinearColor(0.4f, 0.06f, 0.025f),
        FLinearColor(0.92f, 0.9f, 0.88f), FLinearColor(0.75f, 0.22f, 0.04f),
        FLinearColor(0.95f, 0.78f, 0.3f), FLinearColor(0.85f, 0.38f, 0.52f),
        FLinearColor(0.03f, 0.05f, 0.16f), FLinearColor(0.18f, 0.2f, 0.24f)
    };
	// Cores medidas na referência (sRGB) convertidas para o espaço linear da interface (sem isso o preto vira cinza)
	FLinearColor SRGB(uint8 R, uint8 G, uint8 B, float A = 1.f)
	{
		FLinearColor C = FLinearColor::FromSRGBColor(FColor(R, G, B));
		C.A = A;
		return C;
	}
	const FLinearColor White(1.f, 1.f, 1.f, 1.f);
	const FLinearColor LabelOff = SRGB(200, 196, 190, 0.6f);
	const FLinearColor BoxOn = SRGB(8, 8, 8, 0.94f);			// caixa escolhida: preta
	const FLinearColor BoxOff = SRGB(22, 19, 16, 0.62f);		// as outras: marrom-escura translúcida
	const FLinearColor BoxLocked = SRGB(22, 19, 16, 0.35f);		// vila trancada: quase some
	const FLinearColor BackFill = SRGB(14, 14, 14, 0.94f);		// Voltar: preto
	const FLinearColor NextFill = SRGB(96, 88, 76, 0.78f);		// Próximo: cinza quente translúcido
	const FLinearColor PillFill = SRGB(16, 16, 16, 0.62f);		// "Olhar fixo"

	float EaseOut(float T) { T = FMath::Clamp(T, 0.f, 1.f); return 1.f - FMath::Pow(1.f - T, 3.f); }

	FSlateBrush Rounded(const FLinearColor& Fill, float Radius, const FLinearColor& Outline = FLinearColor::Transparent, float OutlineWidth = 0.f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(Outline), OutlineWidth);
		return Brush;
	}

	FSlateBrush TextureBrush(UTexture2D* Texture, const FLinearColor& Tint)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}

	/** Textura branca gerada por código; Alpha recebe a posição em px da textura e diz a opacidade (0..1). 4 amostras por pixel. */
	UTexture2D* MakeTexture(int32 Width, int32 Height, TFunctionRef<float(float X, float Y)> Alpha)
	{
		TArray<FColor> Pixels;
		Pixels.SetNumUninitialized(Width * Height);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				float A = 0.f;
				for (int32 S = 0; S < 4; ++S)
				{
					A += Alpha(X + 0.25f + 0.5f * (S % 2), Y + 0.25f + 0.5f * (S / 2));
				}
				Pixels[Y * Width + X] = FColor(255, 255, 255, (uint8)FMath::RoundToInt(FMath::Clamp(A / 4.f, 0.f, 1.f) * 255.f));
			}
		}
		const TConstArrayView64<uint8> Bytes(reinterpret_cast<const uint8*>(Pixels.GetData()), Pixels.Num() * sizeof(FColor));
		return UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8, NAME_None, Bytes);
	}

	/** Ruído suave 1D (0..1), o mesmo em todas as máquinas */
	float Noise(float T, uint32 Seed)
	{
		auto Hash = [Seed](int32 I)
		{
			uint32 H = uint32(I) * 374761393u + Seed * 668265263u;
			H = (H ^ (H >> 13)) * 1274126177u;
			return float(H & 0xFFFF) / 65535.f;
		};
		const int32 I = FMath::FloorToInt(T);
		const float F = T - I;
		return FMath::Lerp(Hash(I), Hash(I + 1), F * F * (3.f - 2.f * F));
	}

	float SegmentDistance(const FVector2f& P, const FVector2f& A, const FVector2f& B)
	{
		const FVector2f AB = B - A;
		const float T = FMath::Clamp(FVector2f::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), 1e-6f), 0.f, 1.f);
		return FVector2f::Distance(P, A + AB * T);
	}
}

UHypeCharacterSelectWidget::UHypeCharacterSelectWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Logo = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/HypeWorld/UI/Textures/T_Logo_HypeWorld.T_Logo_HypeWorld")));
	Font = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/Ninja/UI/Fontes/F_Montserrat.F_Montserrat")));
	Subtitle = NSLOCTEXT("HypeSelect", "Subtitle", "Selecione a vila e o clã da sua candidatura");

	auto Clan = [](const TCHAR* Id, const FText& Name, const FLinearColor& Color)
	{
		FHypeClanOption Option;
		Option.Id = FName(Id);
		Option.Name = Name;
		Option.IconColor = Color;
		return Option;
	};
	FHypeVillageOption Konoha;
	Konoha.Id = TEXT("Konoha");
	Konoha.Name = NSLOCTEXT("HypeSelect", "Konoha", "Konoha");
	Konoha.IconColor = HypeSelect::SRGB(236, 220, 160);
	Konoha.Clans = {
		Clan(TEXT("Akimichi"), NSLOCTEXT("HypeSelect", "Akimichi", "Akimichi"), HypeSelect::SRGB(196, 42, 36)),
		Clan(TEXT("Hyuga"), NSLOCTEXT("HypeSelect", "Hyuga", "Hyuga"), HypeSelect::SRGB(232, 96, 44)),
		Clan(TEXT("Nara"), NSLOCTEXT("HypeSelect", "Nara", "Nara"), HypeSelect::SRGB(150, 176, 128)),
		Clan(TEXT("Senju"), NSLOCTEXT("HypeSelect", "Senju", "Senju"), HypeSelect::SRGB(132, 168, 210)),
		Clan(TEXT("Uchiha"), NSLOCTEXT("HypeSelect", "Uchiha", "Uchiha"), HypeSelect::SRGB(212, 34, 34)),
	};
	FHypeVillageOption Suna;	// sem clãs ainda = trancada (como na referência)
	Suna.Id = TEXT("Suna");
	Suna.Name = NSLOCTEXT("HypeSelect", "Suna", "Suna");
	Suna.IconColor = HypeSelect::SRGB(212, 180, 120);
	Villages = { Konoha, Suna };

	OptionCardClass = TSoftClassPtr<UHypeOptionCardWidget>(FSoftObjectPath(TEXT("/Game/HypeWorld/UI/CharacterSelect/Components/WBP_OptionCard.WBP_OptionCard_C")));
	ColorRowClass = TSoftClassPtr<UHypeColorRowWidget>(FSoftObjectPath(TEXT("/Game/HypeWorld/UI/CharacterSelect/Components/WBP_ColorRow.WBP_ColorRow_C")));
	GradientSliderClass = TSoftClassPtr<UHypeGradientSliderWidget>(FSoftObjectPath(TEXT("/Game/HypeWorld/UI/CharacterSelect/Components/WBP_GradientSlider.WBP_GradientSlider_C")));
	MapaDoJogo = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Maps/Tutorial/L_Tutorial.L_Tutorial")));

	SetIsFocusable(true);
}

TSharedRef<SWidget> UHypeCharacterSelectWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget && !IsDesignTime())
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UHypeCharacterSelectWidget::Refresh(bool bAnimate)
{
    if (Canvas)
    {
        if (bAnimate && PageTime < 0.f)
        {
            PageTime = 0.f;
            bPageRebuildPending = true;
            Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
            return;
        }
        Canvas->ClearChildren();
        BuildLayout();
    }
}

void UHypeCharacterSelectWidget::BuildLayout()
{
	using namespace HypeSelect;

	if (!Canvas)
	{
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Canvas;
		Canvas->SetRenderOpacity(0.f);
	}
    Cards.Reset();
    HairColorRow = nullptr;
    HairColorHex = nullptr;
    bPlacingPageContent = false;
    SkinAmountLabel = nullptr;
    NameInput = nullptr;
    HeightLabel = nullptr;
    HeightSlider = nullptr;
    SkinSlider = nullptr;
    CreationErrorLabel = nullptr;
    PageOpacities.Reset();
    ButtonHover.Reset();
	VillageButtons.Reset();
	ClanButtons.Reset();
    BackButton = nullptr;
    NextButton = nullptr;
    LookButton = nullptr;
    BackDashes = nullptr;
    NextDashes = nullptr;

	// Escolha válida: vila com clã (as sem clã estão trancadas) e clã dentro da lista
	if (!Villages.IsValidIndex(SelectedVillage) || Villages[SelectedVillage].Clans.Num() == 0)
	{
		SelectedVillage = Villages.IndexOfByPredicate([](const FHypeVillageOption& V) { return V.Clans.Num() > 0; });
	}
	const FHypeVillageOption* Village = Villages.IsValidIndex(SelectedVillage) ? &Villages[SelectedVillage] : nullptr;
	SelectedClan = Village ? FMath::Clamp(SelectedClan, 0, Village->Clans.Num() - 1) : -1;
    const UHypeProfileSubsystem* SavedProfile=GetGameInstance() ? GetGameInstance()->GetSubsystem<UHypeProfileSubsystem>() : nullptr;
    if (bChoosingAffiliation && Village && SavedProfile)
    {
        SelectedClan=Village->Clans.IndexOfByPredicate([&](const FHypeClanOption& C) { return C.Id==SavedProfile->Cla; });
    }

	// --- Texturas desenhadas por código (uma vez) ---
	if (!ShadeTexture)
	{
		// Escurecido: forte à esquerda (atrás do menu), some até ~55% da largura
		const float Left = LeftShade, Base = BaseShade;
		ShadeTexture = MakeTexture(256, 4, [Left, Base](float X, float)
		{
			const float U = X / 256.f;
			return Base + Left * (1.f - FMath::SmoothStep(0.f, 0.55f, U));
		});
		// Caixa com pincelada: bordas irregulares e riscos horizontais (2x o tamanho na tela)
		const int32 BW = 156, BH = 134;
		BoxTexture = MakeTexture(BW, BH, [BW, BH](float X, float Y)
		{
			const float L = 6.f + 5.f * Noise(Y * 0.08f, 1) + 3.f * Noise(Y * 0.4f, 2);
			const float R = BW - (6.f + 8.f * Noise(Y * 0.06f, 3) + 3.f * Noise(Y * 0.5f, 4));
			const float T = 6.f + 4.f * Noise(X * 0.05f, 5) + 2.f * Noise(X * 0.4f, 6);
			const float B = BH - (6.f + 5.f * Noise(X * 0.05f, 7) + 2.f * Noise(X * 0.4f, 8));
			const float D = FMath::Min(FMath::Min(X - L, R - X), FMath::Min(Y - T, B - Y));
			float A = FMath::Clamp(D, 0.f, 1.f);
			A *= 0.85f + 0.15f * Noise(Y * 0.35f + 3.f * Noise(X * 0.02f, 9), 10);	// riscos do pincel
			if (R - X < 14.f && Noise(Y * 0.6f, 11) < 0.25f)							// pincel seco na ponta direita
			{
				A *= FMath::Max(R - X, 0.f) / 14.f;
			}
			return A;
		});
		// Contorno tracejado arredondado dos botões (2x o tamanho na tela)
		const float DW = ButtonSize.X * 2.f, DH = ButtonSize.Y * 2.f;
		DashTexture = MakeTexture(FMath::RoundToInt(DW), FMath::RoundToInt(DH), [DW, DH](float X, float Y)
		{
			const FVector2f P(X - DW * 0.5f, Y - DH * 0.5f);
			const FVector2f Half(DW * 0.5f - 2.f, DH * 0.5f - 2.f);
			const float Radius = 14.f, Width = 3.f;
			const FVector2f Q(FMath::Abs(P.X) - Half.X + Radius, FMath::Abs(P.Y) - Half.Y + Radius);
			const float Dist = FVector2f(FMath::Max(Q.X, 0.f), FMath::Max(Q.Y, 0.f)).Size() + FMath::Min(FMath::Max(Q.X, Q.Y), 0.f) - Radius;
			if (FMath::Abs(Dist + Width * 0.5f) > Width * 0.5f)
			{
				return 0.f;
			}
			// Traços de 12 px com 8 de espaço ao longo de cada lado (cantos sempre cheios)
			float Along;
			if (FMath::Abs(P.X) < Half.X - Radius)
			{
				Along = P.X;
			}
			else if (FMath::Abs(P.Y) < Half.Y - Radius)
			{
				Along = P.Y;
			}
			else
			{
				return 1.f;
			}
			return FMath::Fmod(Along + 1000.f, 20.f) < 12.f ? 1.f : 0.f;
		});
		// Olho riscado do botão "Olhar fixo"
		EyeTexture = MakeTexture(64, 64, [](float X, float Y)
		{
			const FVector2f P(X / 64.f - 0.5f, Y / 64.f - 0.5f);
			const float E = FMath::Sqrt(FMath::Square(P.X / 0.40f) + FMath::Square(P.Y / 0.22f));
			const bool Outline = FMath::Abs(E - 1.f) < 0.13f;
			const bool Pupil = P.Size() < 0.09f;
			const bool Slash = SegmentDistance(P, FVector2f(-0.36f, -0.36f), FVector2f(0.36f, 0.36f)) < 0.04f;
			const bool Gap = SegmentDistance(P, FVector2f(-0.36f, -0.36f), FVector2f(0.36f, 0.36f)) < 0.09f;
			return Slash ? 1.f : ((Outline || Pupil) && !Gap ? 1.f : 0.f);
		});
		// Degradê da criação: preto PanelShade na borda esquerda até sumir em PanelShadeReach da largura
		const float Edge = PanelShade, Reach = FMath::Max(PanelShadeReach, 0.05f);
		CreatorShadeTexture = MakeTexture(256, 4, [Edge, Reach](float X, float)
		{
			const float U = X / 256.f;
			return Edge * (1.f - FMath::SmoothStep(0.f, Reach, U));
		});
		LogoTexture = Logo.LoadSynchronous();
		if (!LogoTexture)
		{
			UE_LOG(LogHypeCharacterSelect, Warning, TEXT("Logo não encontrada (%s)"), *Logo.ToString());
		}
	}

	// --- Ajudantes ---
	UObject* FontObject = Font.LoadSynchronous();
	auto FontInfo = [FontObject](const TCHAR* Face, int32 Size)
	{
		return FontObject ? FSlateFontInfo(FontObject, Size, FName(Face)) : FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size);
	};
	const FVector2D TopLeft(0.f, 0.f), Center(0.5f, 0.5f);
	auto Place = [this](UWidget* Widget, FVector2D Anchor, FVector2D Position, FVector2D Size, FVector2D Alignment)
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
        if (bPlacingPageContent || !bCreatingCharacter) { PageOpacities.Add(Widget, Widget->GetRenderOpacity()); }
		return PanelSlot;
	};
	auto MakeImage = [this](const FSlateBrush& Brush, float Opacity = 1.f)
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		Image->SetBrush(Brush);
		Image->SetRenderOpacity(Opacity);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	};
	auto MakeText = [this](const FText& Words, const FSlateFontInfo& Info, const FLinearColor& Color)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetText(Words);
		Text->SetFont(Info);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(ETextJustify::Center);
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Text;
	};
	auto MakeButton = [this](float Radius, TArray<TObjectPtr<UButton>>* List = nullptr)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle Style;
		Style.SetNormal(Rounded(FLinearColor::Transparent, Radius));
		Style.SetHovered(Rounded(FLinearColor(1.f, 1.f, 1.f, 0.06f), Radius));
		Style.SetPressed(Rounded(FLinearColor(1.f, 1.f, 1.f, 0.12f), Radius));
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f));
		Button->SetStyle(Style);
        Button->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
        ButtonHover.Add(Button, 0.f);
		Button->OnClicked.AddDynamic(this, &UHypeCharacterSelectWidget::HandleClick);
		if (List)
		{
			List->Add(Button);
		}
		return Button;
	};
	// Uma opção (vila ou clã): rótulo em cima, caixa de pincel, ícone (ou letra inicial) e o botão por cima
	auto Option = [&](FName Id, const FText& Name, const TSoftObjectPtr<UTexture2D>& IconAsset, const FLinearColor& IconColor,
		bool bSelected, bool bLocked, float X, float LabelY, float BoxY, const FVector2D& BoxSize, TArray<TObjectPtr<UButton>>& List)
	{
		const float Dim = bLocked ? 0.0f : (bSelected ? 1.f : 0.5f);
		Place(MakeText(Name, FontInfo(bSelected ? TEXT("Bold") : TEXT("SemiBold"), 9), bSelected ? White : (bLocked ? LabelOff * FLinearColor(1.f, 1.f, 1.f, 0.6f) : LabelOff)),
			TopLeft, FVector2D(X, LabelY), FVector2D::ZeroVector, Center);
		Place(MakeImage(TextureBrush(BoxTexture, bSelected ? BoxOn : (bLocked ? BoxLocked : BoxOff))), TopLeft, FVector2D(X, BoxY), BoxSize, Center);
		const float IconSize = BoxSize.Y * 0.66f;
		UTexture2D* IconTexture = IconAsset.LoadSynchronous();
		UTexture2D* Generated = IconTexture ? nullptr : GetGeneratedIcon(Id, 0);
		if (IconTexture || Generated)
		{
			// Uchiha: leque com a parte de baixo branca (segunda camada) e a de cima na cor do clã
			if (UTexture2D* Second = IconTexture ? nullptr : GetGeneratedIcon(Id, 1))
			{
				Place(MakeImage(TextureBrush(Second, SRGB(236, 232, 226)), Dim), TopLeft, FVector2D(X, BoxY), FVector2D(IconSize, IconSize), Center);
			}
			Place(MakeImage(TextureBrush(IconTexture ? IconTexture : Generated, IconColor), Dim), TopLeft, FVector2D(X, BoxY), FVector2D(IconSize, IconSize), Center);
		}
		else if (!Name.IsEmpty())
		{
			// Sem arte ainda: letra inicial na cor do clã/vila
			UTextBlock* Initial = MakeText(FText::FromString(Name.ToString().Left(1).ToUpper()), FontInfo(TEXT("ExtraBold"), 20), IconColor);
			Initial->SetRenderOpacity(Dim);
			Place(Initial, TopLeft, FVector2D(X, BoxY), FVector2D::ZeroVector, Center);
		}
		Place(MakeButton(4.f, &List), TopLeft, FVector2D(X, BoxY), BoxSize, Center);
	};
	auto RowX = [this](int32 Index, int32 Count, float Pitch)
	{
		return ColumnCenter + (Index - (Count - 1) * 0.5f) * Pitch;
	};

	// --- Escurecido atrás do menu (tela inteira) ---
	UImage* Shade = MakeImage(TextureBrush(ShadeTexture, FLinearColor::Black));
	UCanvasPanelSlot* ShadeSlot = Canvas->AddChildToCanvas(Shade);
	ShadeSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	ShadeSlot->SetOffsets(FMargin(0.f));
	UImage* DragArea = MakeImage(Rounded(FLinearColor::Transparent, 0.f));
	DragArea->SetVisibility(ESlateVisibility::Visible);
	UCanvasPanelSlot* DragSlot = Canvas->AddChildToCanvas(DragArea);
	DragSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DragSlot->SetOffsets(FMargin(0.f));

	// --- Logo e frase ---
	if (LogoTexture && !bCreatingCharacter)
	{
		const float Aspect = LogoTexture->GetSizeX() > 64 ? float(LogoTexture->GetSizeY()) / float(LogoTexture->GetSizeX()) : 0.55f;
		Place(MakeImage(TextureBrush(LogoTexture, White)), TopLeft, FVector2D(ColumnCenter, LogoCenterY), FVector2D(LogoWidth, LogoWidth * Aspect), Center);
	}
    UHypeProfileSubsystem* Profile = GetGameInstance() ? GetGameInstance()->GetSubsystem<UHypeProfileSubsystem>() : nullptr;
    if (!bCreatingCharacter && !bChoosingAffiliation)
    {
        auto Action = [&](const FText& Label, FVector2D Anchor, FVector2D Position, FVector2D Size, FName Method, int32 TextSize)
        {
            UButton* Button = MakeButton(8.f);
            Button->OnClicked.Clear();
            FScriptDelegate Click;
            Click.BindUFunction(this, Method);
            Button->OnClicked.Add(Click);
            Place(Button, Anchor, Position, Size, Center);
            UTextBlock* Text = MakeText(Label, FontInfo(TEXT("SemiBold"), TextSize), White);
            Button->AddChild(Text);
            return Button;
        };
        if (!Profile || !Profile->HasCharacter())
        {
            Place(MakeText(NSLOCTEXT("HypeSelect", "FirstEntry", "Sua jornada começa aqui"), FontInfo(TEXT("Bold"), 22), White), Center,
                FVector2D(0, -135), FVector2D::ZeroVector, Center);
            Place(MakeImage(Rounded(BackFill, 12.f)), Center, FVector2D::ZeroVector, FVector2D(104, 104), Center);
            UButton* Create = Action(FText::FromString(TEXT("+")), Center, FVector2D::ZeroVector, FVector2D(104, 104), TEXT("StartCreation"), 48);
            Create->SetToolTipText(NSLOCTEXT("HypeSelect", "CreateTooltip", "Criar personagem"));
            Place(MakeText(NSLOCTEXT("HypeSelect", "Create", "Criar personagem"), FontInfo(TEXT("SemiBold"), 14), White), Center,
                FVector2D(0, 95), FVector2D::ZeroVector, Center);
            SetUserFocus(GetOwningPlayer());
        }
        else
        {
            Place(MakeText((Profile->Appearance.Name.IsEmpty() ? NSLOCTEXT("HypeSelect", "YourNinja", "Seu personagem") : FText::FromString(Profile->Appearance.GetFullName())), FontInfo(TEXT("Bold"), 22), White), TopLeft,
                FVector2D(ColumnCenter, 380), FVector2D::ZeroVector, Center);
            const FHypeVillageOption* SavedVillage = Villages.FindByPredicate([&](const FHypeVillageOption& V) { return V.Id == Profile->Vila; });
            const FHypeClanOption* SavedClan = SavedVillage ? SavedVillage->Clans.FindByPredicate([&](const FHypeClanOption& C) { return C.Id == Profile->Cla; }) : nullptr;
            const FText VillageName = SavedVillage ? SavedVillage->Name : (Profile->Vila.IsNone() ? FText::FromString(TEXT("Não escolhida")) : FText::FromName(Profile->Vila));
            const FText ClanName = SavedClan ? SavedClan->Name : (Profile->Cla.IsNone() ? FText::FromString(TEXT("Não escolhido")) : FText::FromName(Profile->Cla));
            Place(MakeText(FText::Format(NSLOCTEXT("HypeSelect", "Summary", "Vila: {0}  ·  Clã: {1}"), VillageName, ClanName), FontInfo(TEXT("SemiBold"), 13), White), TopLeft,
                FVector2D(ColumnCenter, 445), FVector2D::ZeroVector, Center);
            if (SavedVillage && SavedClan)
            {
                TArray<TObjectPtr<UButton>> Unused;
                Option(SavedVillage->Id, SavedVillage->Name, SavedVillage->Icon, SavedVillage->IconColor, true, false, ColumnCenter - 65, 502, 550, VillageBoxSize, Unused);
                Option(SavedClan->Id, SavedClan->Name, SavedClan->Icon, SavedClan->IconColor, true, false, ColumnCenter + 65, 502, 550, ClanBoxSize, Unused);
                for (UButton* B : Unused) { B->SetIsEnabled(false); }
            }
            Place(MakeText(NSLOCTEXT("HypeSelect", "EquippedJutsu", "JUTSUS EQUIPADOS"), FontInfo(TEXT("Bold"), 11), White), TopLeft,
                FVector2D(ColumnCenter, 625), FVector2D::ZeroVector, Center);
            const AHypeWorldCharacter* Ninja = Cast<AHypeWorldCharacter>(GetOwningPlayerPawn());
            FString Jutsus;
            if (Ninja)
            {
                for (int32 JutsuSlot = 0; JutsuSlot < 6; ++JutsuSlot)
                {
                    if (const UNinjaJutsuData* Jutsu = Ninja->GetJutsuInSlot(JutsuSlot))
                    {
                        if (!Jutsus.IsEmpty()) { Jutsus += TEXT("\n"); }
                        Jutsus += Jutsu->DisplayName.ToString();
                    }
                }
            }
            UTextBlock* JutsuText = MakeText(Jutsus.IsEmpty() ? NSLOCTEXT("HypeSelect", "NoJutsu", "Nenhum jutsu equipado") : FText::FromString(Jutsus), FontInfo(TEXT("SemiBold"), 12), White);
            JutsuText->SetWrapTextAt(360.f);
            Place(JutsuText, TopLeft, FVector2D(ColumnCenter, 660), FVector2D::ZeroVector, FVector2D(0.5f, 0));
            Place(MakeImage(Rounded(BackFill, 6.f)), TopLeft, FVector2D(ColumnCenter, 905), FVector2D(240, 52), Center);
            Action(NSLOCTEXT("HypeSelect", "Enter", "Entrar no jogo"), TopLeft, FVector2D(ColumnCenter, 905), FVector2D(240, 52), TEXT("EnterGame"), 13);
        }
        return;
    }
    if (bCreatingCharacter && CreationStep > 0)
    {
        // Layout do Zenkai: painel à esquerda sobre um degradê preto, personagem no centro (câmera própria).
        const float FormLeft=FormLeftX;
        const float FormWidth=FormWidthPx;
        const float FormCenter=FormLeft+FormWidth*0.5f;
        const FLinearColor Muted=SRGB(193,191,185,0.72f);
        const FVector2D LeftMid(0.f,0.5f);
        Shade->SetBrush(TextureBrush(CreatorShadeTexture,FLinearColor::Black));

        if (LogoTexture)
        {
            const float Aspect=LogoTexture->GetSizeX()>64 ? float(LogoTexture->GetSizeY())/float(LogoTexture->GetSizeX()) : 0.55f;
            Place(MakeImage(TextureBrush(LogoTexture,White)),TopLeft,FVector2D(FormCenter,178),FVector2D(150,150*Aspect),Center);
        }
        auto Label = [&](const FString& Text,float X,float Y,int32 Size,const FLinearColor& Color,FVector2D Alignment=FVector2D(0.f,0.5f),const TCHAR* Face=TEXT("SemiBold"))
        {
            UTextBlock* Widget=MakeText(FText::FromString(Text),FontInfo(Face,Size),Color);
            Widget->SetJustification(Alignment.X>0.75f ? ETextJustify::Right : ETextJustify::Left);
            Place(Widget,TopLeft,FVector2D(X,Y),FVector2D::ZeroVector,Alignment);
            return Widget;
        };
        auto Icon = [&](FName Name,float Y)
        {
            if (UTexture2D* Texture=GetCreatorIcon(Name))
            {
                Place(MakeImage(TextureBrush(Texture,SRGB(236,232,224,0.6f))),TopLeft,FVector2D(IconColumnX,Y),FVector2D(20,20),Center);
            }
        };

        // --- Abas: ativa em branco e sublinhada, as outras em cinza ---
        const TCHAR* Tabs[]={TEXT("Identidade"),TEXT("Aparência"),TEXT("Estilo"),TEXT("Confirmar")};
        const FName CategoryActions[]={TEXT("ShowIdentityCategory"),TEXT("ShowAppearanceCategory"),TEXT("ShowStyleCategory"),TEXT("ShowConfirmCategory")};
        UHorizontalBox* CategoryRow=WidgetTree->ConstructWidget<UHorizontalBox>();
        for (int32 Index=0;Index<4;++Index)
        {
            const bool bSelected=CreationStep==Index+1;
            UVerticalBox* Content=WidgetTree->ConstructWidget<UVerticalBox>();
            UTextBlock* Caption=MakeText(FText::FromString(Tabs[Index]),FontInfo(bSelected ? TEXT("Bold") : TEXT("SemiBold"),10),bSelected ? White : SRGB(150,148,143,0.8f));
            Content->AddChildToVerticalBox(Caption)->SetHorizontalAlignment(HAlign_Center);
            USizeBox* Underline=WidgetTree->ConstructWidget<USizeBox>();
            Underline->SetWidthOverride(34.f);
            Underline->SetHeightOverride(2.f);
            Underline->AddChild(MakeImage(Rounded(White,1.f),bSelected ? 1.f : 0.f));
            UVerticalBoxSlot* LineSlot=Content->AddChildToVerticalBox(Underline);
            LineSlot->SetPadding(FMargin(0,6,0,0));
            LineSlot->SetHorizontalAlignment(HAlign_Center);
            UButton* Category=MakeButton(4.f);
            Category->OnClicked.Clear();
            FScriptDelegate Click;
            Click.BindUFunction(this,CategoryActions[Index]);
            Category->OnClicked.Add(Click);
            Category->AddChild(Content);
            USizeBox* HitBox=WidgetTree->ConstructWidget<USizeBox>();
            HitBox->SetHeightOverride(40.f);
            HitBox->AddChild(Category);
            UHorizontalBoxSlot* CategorySlot=CategoryRow->AddChildToHorizontalBox(HitBox);
            CategorySlot->SetPadding(FMargin(0,0,Index<3 ? 24.f : 0.f,0));
            CategorySlot->SetVerticalAlignment(VAlign_Center);
        }
        Place(CategoryRow,TopLeft,FVector2D(FormLeft,290),FVector2D::ZeroVector,LeftMid);

        // --- Peças reutilizáveis ---
        auto Card = [&](FName Key,float Y)
        {
            UClass* Class=OptionCardClass.LoadSynchronous();
            UHypeOptionCardWidget* Widget=CreateWidget<UHypeOptionCardWidget>(GetOwningPlayer(),Class ? Class : UHypeOptionCardWidget::StaticClass());
            Widget->Font=Font;
            Widget->CardSize=FVector2D(FormWidth,72.f);
            Widget->Key=Key;
            Widget->OnStep.AddDynamic(this,&UHypeCharacterSelectWidget::HandleCardStep);
            Place(Widget,TopLeft,FVector2D(FormLeft,Y),Widget->CardSize,FVector2D::ZeroVector);
            Cards.Add(Key,Widget);
            return Widget;
        };
        auto GradientSlider = [&](FName Key,const TArray<FLinearColor>& Stops,float Y,float BarHeight)
        {
            UClass* Class=GradientSliderClass.LoadSynchronous();
            UHypeGradientSliderWidget* Widget=CreateWidget<UHypeGradientSliderWidget>(GetOwningPlayer(),Class ? Class : UHypeGradientSliderWidget::StaticClass());
            Widget->Font=Font;
            Widget->BarSize=FVector2D(FormWidth,BarHeight);
            Widget->Key=Key;
            Widget->Setup(Stops,0.f);
            Widget->OnValueChanged.AddDynamic(this,&UHypeCharacterSelectWidget::HandleSliderChanged);
            Place(Widget,TopLeft,FVector2D(FormLeft,Y),FVector2D(FormWidth,FMath::Max(Widget->ThumbSize,BarHeight)+6.f),LeftMid);
            return Widget;
        };

        bPlacingPageContent=true;
        if (CreationStep == 1)
        {
            Icon(TEXT("Person"),350);
            Label(TEXT("Primeiro nome"),FormLeft,350,10,White);
            NameInput = WidgetTree->ConstructWidget<UEditableTextBox>();
            FEditableTextBoxStyle InputStyle=NameInput->GetWidgetStyle();
            InputStyle.SetFont(FontInfo(TEXT("SemiBold"),11));
            InputStyle.SetForegroundColor(FSlateColor(White));
            InputStyle.SetFocusedForegroundColor(FSlateColor(White));
            InputStyle.SetPadding(FMargin(16,11));
            InputStyle.SetBackgroundImageNormal(Rounded(SRGB(0,0,0,0.6f),8.f));
            InputStyle.SetBackgroundImageHovered(Rounded(SRGB(20,20,20,0.7f),8.f));
            InputStyle.SetBackgroundImageFocused(Rounded(SRGB(26,26,26,0.8f),8.f));
            NameInput->SetWidgetStyle(InputStyle);
            NameInput->SetHintText(FText::FromString(TEXT("Primeiro nome")));
            NameInput->SetText(FText::FromString(DraftAppearance.Name));
            NameInput->OnTextChanged.AddDynamic(this, &UHypeCharacterSelectWidget::HandleNameChanged);
            Place(NameInput,TopLeft,FVector2D(FormLeft,394),FVector2D(FormWidth,44),LeftMid);
            Label(TEXT("De 2 a 24 caracteres, sem sobrenome"),FormLeft,432,8,Muted);

            Icon(TEXT("Drop"),490);
            Label(TEXT("Tom de pele"),FormLeft,490,10,White);
            SkinAmountLabel=Label(TEXT(""),FormLeft+FormWidth,490,9,Muted,FVector2D(1.f,0.5f));
            SkinSlider=GradientSlider(TEXT("Skin"),SkinColors,522,10.f);

            Icon(TEXT("Height"),578);
            Label(TEXT("Altura do personagem"),FormLeft,578,10,White);
            HeightLabel=Label(TEXT(""),FormLeft+FormWidth,578,9,Muted,FVector2D(1.f,0.5f));
            HeightSlider=GradientSlider(TEXT("Height"),{FLinearColor(1,1,1,0.3f),FLinearColor(1,1,1,0.3f)},610,3.f);

            Label(TEXT("O clã é definido automaticamente pelo jogo."),FormLeft,668,8,SRGB(193,191,185,0.5f));
            SyncAppearanceControls();
        }
        else if (CreationStep == 2)
        {
            Icon(TEXT("Eye"),386);
            Card(TEXT("Eyes"),350);
            Label(TEXT("Arraste para girar · role para aproximar"),FormLeft,452,8,SRGB(193,191,185,0.5f));
        }
        else if (CreationStep == 3)
        {
            Icon(TEXT("Hair"),376);
            Card(TEXT("Hair"),340);
            Label(TEXT("Cor do cabelo"),FormLeft,436,10,White);
            HairColorHex=Label(TEXT(""),FormLeft+FormWidth,436,8,Muted,FVector2D(1.f,0.5f));
            UClass* RowClass=ColorRowClass.LoadSynchronous();
            HairColorRow=CreateWidget<UHypeColorRowWidget>(GetOwningPlayer(),RowClass ? RowClass : UHypeColorRowWidget::StaticClass());
            HairColorRow->Font=Font;
            HairColorRow->Key=TEXT("HairColor");
            const int32 HairIndex=HairColors.IndexOfByPredicate([&](const FLinearColor& C) { return C.Equals(DraftAppearance.HairColor,0.001f); });
            HairColorRow->Setup(HairColors,HairIndex);
            HairColorRow->OnColorPicked.AddDynamic(this,&UHypeCharacterSelectWidget::HandleHairColor);
            const float RowWidth=48.f+HairColorRow->VisibleCount*(HairColorRow->SwatchSize+HairColorRow->SwatchGap);
            Place(HairColorRow,TopLeft,FVector2D(FormLeft-6.f,470),FVector2D(RowWidth,HairColorRow->SwatchSize+8.f),LeftMid);
            Icon(TEXT("Shirt"),536);
            Card(TEXT("Outfit"),500);
            Icon(TEXT("Top"),616);
            Card(TEXT("Top"),580);
            Icon(TEXT("Pants"),696);
            Card(TEXT("Bottom"),660);
            Icon(TEXT("Shoe"),776);
            Card(TEXT("Shoes"),740);
        }
        else
        {
            Icon(TEXT("Check"),350);
            Label(TEXT("Revise seu personagem"),FormLeft,350,13,White,LeftMid,TEXT("Bold"));
            auto PartName = [&](FName Key)
            {
                TArray<FName> Ids; TArray<FText> Names;
                GetPartOptions(Key,Ids,Names);
                const int32 Index=Ids.IndexOfByKey(DraftPart(Key));
                return Names.IsValidIndex(Index) ? Names[Index].ToString() : TEXT("—");
            };
            struct FRow { FString Title; FString Value; const FLinearColor* Swatch; };
            const bool bOutfit=!DraftAppearance.OutfitId.IsNone();
            const TArray<FRow> Rows = {
                {TEXT("Nome"),DraftAppearance.Name,nullptr},
                {TEXT("Altura"),FString::Printf(TEXT("%.0f cm"),DraftAppearance.HeightCm),nullptr},
                {TEXT("Tom de pele"),TEXT(""),&DraftAppearance.SkinTint},
                {TEXT("Olhos"),PartName(TEXT("Eyes")),nullptr},
                {TEXT("Cabelo"),PartName(TEXT("Hair")),nullptr},
                {TEXT("Cor do cabelo"),TEXT(""),&DraftAppearance.HairColor},
                {TEXT("Roupa completa"),PartName(TEXT("Outfit")),nullptr},
                {TEXT("Parte de cima"),bOutfit ? TEXT("—") : PartName(TEXT("Top")),nullptr},
                {TEXT("Parte de baixo"),bOutfit ? TEXT("—") : PartName(TEXT("Bottom")),nullptr},
                {TEXT("Calçado"),bOutfit ? TEXT("—") : PartName(TEXT("Shoes")),nullptr},
            };
            const float Top=384.f, RowHeight=34.f;
            Place(MakeImage(Rounded(SRGB(0,0,0,0.6f),8.f)),TopLeft,FVector2D(FormLeft,Top),FVector2D(FormWidth,Rows.Num()*RowHeight+16.f),FVector2D::ZeroVector);
            for (int32 Index=0;Index<Rows.Num();++Index)
            {
                const float Y=Top+8.f+(Index+0.5f)*RowHeight;
                Label(Rows[Index].Title,FormLeft+16.f,Y,9,Muted);
                if (Rows[Index].Swatch)
                {
                    Place(MakeImage(Rounded(*Rows[Index].Swatch,9.f,FLinearColor(1,1,1,0.25f),1.f)),TopLeft,FVector2D(FormLeft+FormWidth-25.f,Y),FVector2D(18,18),Center);
                }
                else
                {
                    Label(Rows[Index].Value.IsEmpty() ? TEXT("—") : Rows[Index].Value,FormLeft+FormWidth-16.f,Y,9,White,FVector2D(1.f,0.5f));
                }
                if (Index>0)
                {
                    Place(MakeImage(Rounded(FLinearColor(1,1,1,0.06f),0.f)),TopLeft,FVector2D(FormLeft+16.f,Y-RowHeight*0.5f),FVector2D(FormWidth-32.f,1.f),FVector2D(0.f,0.5f));
                }
            }
            Label(TEXT("Clã definido automaticamente pelo jogo."),FormLeft,Top+Rows.Num()*RowHeight+40.f,8,SRGB(193,191,185,0.5f));
        }
        bPlacingPageContent=false;
        UpdateCards();

        // --- Voltar (preto) / Próximo (dourado escuro translúcido com borda tracejada) ---
        auto Action = [&](const FString& Text,float X,FName Function)
        {
            const bool bPrimary=Function==TEXT("NextCreationStep");
            const FVector2D Size(158,46);
            const float Y=893.f;
            Place(MakeImage(Rounded(bPrimary ? SRGB(122,98,56,0.55f) : SRGB(6,7,8,0.96f),7.f)),TopLeft,FVector2D(X,Y),Size,Center);
            UImage* Dashes=MakeImage(TextureBrush(DashTexture,SRGB(236,212,160)),bPrimary ? 0.55f : 0.f);
            Place(Dashes,TopLeft,FVector2D(X,Y),Size,Center);
            UButton* Button=MakeButton(7.f);
            Button->OnClicked.Clear();
            FScriptDelegate Click;
            Click.BindUFunction(this,Function);
            Button->OnClicked.Add(Click);
            Button->AddChild(MakeText(FText::FromString(Text),FontInfo(TEXT("SemiBold"),10),White));
            Place(Button,TopLeft,FVector2D(X,Y),Size,Center);
            if (bPrimary) { NextButton=Button; NextDashes=Dashes; }
            else { BackButton=Button; BackDashes=Dashes; }
        };
        Action(TEXT("Voltar"),FormLeft+79.f,TEXT("PreviousCreationStep"));
        Action(CreationStep == 4 ? TEXT("Estou pronto") : TEXT("Próximo"),FormLeft+79.f+174.f,TEXT("NextCreationStep"));
        CreationErrorLabel=Label(ProfileError.ToString(),FormLeft,942,9,SRGB(255,180,160));

        // --- Câmera fixa (canto de baixo à direita): trava o giro do personagem ---
        const FVector2D BottomRight(1.f, 1.f);
        const FVector2D LookPos(-LookButtonMargin.X, -LookButtonMargin.Y);
        const FVector2D LookSize(LookButtonSize.X+14.f,LookButtonSize.Y);
        Place(MakeImage(Rounded(bLookFixed ? SRGB(255,255,255,0.16f) : PillFill,6.f)),BottomRight,LookPos,LookSize,BottomRight);
        const FLinearColor LookColor=bLookFixed ? White : SRGB(230,230,230,0.75f);
        const float LookLeft=LookPos.X-LookSize.X;
        Place(MakeImage(TextureBrush(EyeTexture,LookColor)),BottomRight,FVector2D(LookLeft+22.f,LookPos.Y-LookSize.Y*0.5f),FVector2D(16.f,16.f),Center);
        Place(MakeText(NSLOCTEXT("HypeSelect","FixedCamera","Câmera fixa"),FontInfo(TEXT("SemiBold"),9),LookColor),BottomRight,
            FVector2D(LookLeft+36.f,LookPos.Y-LookSize.Y*0.5f),FVector2D::ZeroVector,FVector2D(0.f,0.5f));
        LookButton=MakeButton(6.f);
        Place(LookButton,BottomRight,LookPos,LookSize,BottomRight);
        Place(MakeText(NSLOCTEXT("HypeSelect","Version","hype world · alpha"),FontInfo(TEXT("SemiBold"),8),FLinearColor(1.f,1.f,1.f,0.35f)),BottomRight,
            FVector2D(-16.f,-8.f),FVector2D::ZeroVector,BottomRight);
        return;
    }
	Place(MakeText(bChoosingAffiliation ? FText::FromString(TEXT("Selecione sua vila")) : Subtitle, FontInfo(TEXT("SemiBold"), 12), FLinearColor(1.f, 1.f, 1.f, 0.92f)), TopLeft, FVector2D(ColumnCenter, SubtitleY), FVector2D::ZeroVector, Center);

	// --- Vilas ---
	for (int32 i = 0; i < Villages.Num(); ++i)
	{
		const FHypeVillageOption& V = Villages[i];
		Option(V.Id, V.Name, V.Icon, V.IconColor, i == SelectedVillage, V.Clans.Num() == 0, RowX(i, Villages.Num(), VillagePitch),
			VillageLabelY, VillageBoxY, VillageBoxSize, VillageButtons);
	}

	// --- Clãs da vila escolhida ---
	if (Village)
	{
		for (int32 i = 0; i < Village->Clans.Num(); ++i)
		{
			const FHypeClanOption& C = Village->Clans[i];
            if (bChoosingAffiliation && Profile && C.Id != Profile->Cla) { continue; }
			Option(C.Id, C.Name, C.Icon, C.IconColor, i == SelectedClan, false, (bChoosingAffiliation ? ColumnCenter : RowX(i, Village->Clans.Num(), ClanPitch)),
				ClanLabelY, ClanBoxY, ClanBoxSize, ClanButtons);
            if (bChoosingAffiliation) { ClanButtons.Last()->SetIsEnabled(false); }
		}
	}

	// --- Voltar / Próximo ---
	const float Radius = 6.f;
	const float BackX = ColumnCenter - (ButtonSize.X + ButtonGap) * 0.5f;
	const float NextX = ColumnCenter + (ButtonSize.X + ButtonGap) * 0.5f;
	const FSlateFontInfo ButtonFont = FontInfo(TEXT("SemiBold"), 11);
	Place(MakeImage(Rounded(BackFill, Radius)), TopLeft, FVector2D(BackX, ButtonsY), ButtonSize, Center);
	Place(MakeImage(Rounded(NextFill, Radius)), TopLeft, FVector2D(NextX, ButtonsY), ButtonSize, Center);
	BackDashes = MakeImage(TextureBrush(DashTexture, FLinearColor(1.f, 1.f, 1.f, 0.55f)), 0.f);
	NextDashes = MakeImage(TextureBrush(DashTexture, FLinearColor(1.f, 1.f, 1.f, 0.55f)), 0.f);
	Place(BackDashes, TopLeft, FVector2D(BackX, ButtonsY), ButtonSize, Center);
	Place(NextDashes, TopLeft, FVector2D(NextX, ButtonsY), ButtonSize, Center);
	Place(MakeText(NSLOCTEXT("HypeSelect", "Back", "Voltar"), ButtonFont, White), TopLeft, FVector2D(BackX, ButtonsY), FVector2D::ZeroVector, Center);
	Place(MakeText(NSLOCTEXT("HypeSelect", "CustomizeNext", "Próximo"), ButtonFont, White), TopLeft, FVector2D(NextX, ButtonsY), FVector2D::ZeroVector, Center);
	BackButton = MakeButton(Radius);
	NextButton = MakeButton(Radius);
	Place(BackButton, TopLeft, FVector2D(BackX, ButtonsY), ButtonSize, Center);
	Place(NextButton, TopLeft, FVector2D(NextX, ButtonsY), ButtonSize, Center);

    if (!ProfileError.IsEmpty())
    {
        UTextBlock* Error = MakeText(ProfileError, FontInfo(TEXT("SemiBold"), 11), SRGB(255, 180, 160));
        Error->SetWrapTextAt(400.f);
        Place(Error, TopLeft, FVector2D(ColumnCenter, ButtonsY + 70), FVector2D::ZeroVector, Center);
    }
	// --- Olhar fixo (canto de baixo à direita) e versão embaixo dele ---
	const FVector2D BottomRight(1.f, 1.f);
	const FVector2D LookPos(-LookButtonMargin.X, -LookButtonMargin.Y);
	Place(MakeImage(Rounded(bLookFixed ? SRGB(255, 255, 255, 0.16f) : PillFill, 6.f)), BottomRight, LookPos, LookButtonSize, BottomRight);
	const FLinearColor LookColor = bLookFixed ? White : SRGB(230, 230, 230, 0.75f);
	// olho à esquerda e o texto logo depois dele (antes o texto ficava em cima do olho)
	const float LookLeft = LookPos.X - LookButtonSize.X;
	Place(MakeImage(TextureBrush(EyeTexture, LookColor)), BottomRight, FVector2D(LookLeft + 22.f, LookPos.Y - LookButtonSize.Y * 0.5f), FVector2D(16.f, 16.f), Center);
	Place(MakeText(NSLOCTEXT("HypeSelect", "FixedCamera", "Câmera fixa"), FontInfo(TEXT("SemiBold"), 9), LookColor), BottomRight,
		FVector2D(LookLeft + 36.f, LookPos.Y - LookButtonSize.Y * 0.5f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
	LookButton = MakeButton(6.f);
	Place(LookButton, BottomRight, LookPos, LookButtonSize, BottomRight);
	Place(MakeText(NSLOCTEXT("HypeSelect", "Version", "hype world · alpha"), FontInfo(TEXT("SemiBold"), 8), FLinearColor(1.f, 1.f, 1.f, 0.35f)), BottomRight,
		FVector2D(-16.f, -8.f), FVector2D::ZeroVector, BottomRight);
}

UTexture2D* UHypeCharacterSelectWidget::GetGeneratedIcon(FName Id, int32 Layer)
{
	using namespace HypeSelect;
	const FName Key = Layer == 0 ? Id : FName(*FString::Printf(TEXT("%s_%d"), *Id.ToString(), Layer));
	if (const TObjectPtr<UTexture2D>* Found = IconCache.Find(Key))
	{
		return *Found;
	}

	// Cada símbolo = distância (em unidades de -1..1, negativa dentro) de um desenho simples; a borda é suavizada em 1 px
	const int32 Size = 96;
	const float PxPerUnit = Size * 0.5f;
	auto Circle = [](const FVector2f& P, const FVector2f& C, float R) { return FVector2f::Distance(P, C) - R; };
	auto Ring = [&](const FVector2f& P, const FVector2f& C, float R, float W) { return FMath::Abs(Circle(P, C, R)) - W * 0.5f; };
	auto Line = [](const FVector2f& P, const FVector2f& A, const FVector2f& B, float W) { return SegmentDistance(P, A, B) - W * 0.5f; };
	auto Box = [](const FVector2f& P, const FVector2f& C, const FVector2f& Half)
	{
		const FVector2f D(FMath::Abs(P.X - C.X) - Half.X, FMath::Abs(P.Y - C.Y) - Half.Y);
		return FVector2f(FMath::Max(D.X, 0.f), FMath::Max(D.Y, 0.f)).Size() + FMath::Min(FMath::Max(D.X, D.Y), 0.f);
	};
	TFunction<float(const FVector2f&)> Shape;
	const FString Name = Id.ToString();
	if (Name == TEXT("Konoha"))
	{
		// Folha: espiral + ponta saindo para a esquerda em cima
		Shape = [&](const FVector2f& P)
		{
			const float R = P.Size();
			const float Theta = FMath::Atan2(P.Y, P.X);
			float Best = 10.f;
			for (int32 K = 0; K < 3; ++K)
			{
				const float T = Theta + PI + 2.f * PI * K;	// 0..6π
				if (T > 4.4f * PI) { continue; }
				const float SpiralR = 0.08f + 0.105f * T / PI;
				Best = FMath::Min(Best, FMath::Abs(R - SpiralR));
			}
			const float Spiral = Best - 0.06f;
			const float Tip = Line(P, FVector2f(-0.25f, -0.52f), FVector2f(-0.82f, -0.78f), 0.10f);
			return FMath::Min(Spiral, Tip);
		};
	}
	else if (Name == TEXT("Suna"))
	{
		// Ampulheta: dois triângulos em contorno
		Shape = [&](const FVector2f& P)
		{
			const float A = Line(P, FVector2f(-0.55f, -0.7f), FVector2f(0.55f, -0.7f), 0.11f);
			const float B = Line(P, FVector2f(-0.55f, 0.7f), FVector2f(0.55f, 0.7f), 0.11f);
			const float C = Line(P, FVector2f(-0.55f, -0.7f), FVector2f(0.55f, 0.7f), 0.11f);
			const float D = Line(P, FVector2f(0.55f, -0.7f), FVector2f(-0.55f, 0.7f), 0.11f);
			return FMath::Min(FMath::Min(A, B), FMath::Min(C, D));
		};
	}
	else if (Name == TEXT("Akimichi"))
	{
		// Anel com um caractere estilizado dentro
		Shape = [&](const FVector2f& P)
		{
			float D = Ring(P, FVector2f(0.f, 0.f), 0.72f, 0.12f);
			D = FMath::Min(D, Line(P, FVector2f(-0.38f, -0.32f), FVector2f(0.38f, -0.32f), 0.10f));
			D = FMath::Min(D, Line(P, FVector2f(-0.30f, 0.02f), FVector2f(0.30f, 0.02f), 0.10f));
			D = FMath::Min(D, Line(P, FVector2f(0.f, -0.48f), FVector2f(0.f, 0.46f), 0.10f));
			D = FMath::Min(D, Line(P, FVector2f(0.f, 0.08f), FVector2f(-0.34f, 0.40f), 0.09f));
			D = FMath::Min(D, Line(P, FVector2f(0.f, 0.08f), FVector2f(0.34f, 0.40f), 0.09f));
			return D;
		};
	}
	else if (Name == TEXT("Hyuga"))
	{
		// Chama/gota: círculo embaixo + ponta para cima
		Shape = [&](const FVector2f& P)
		{
			const float Body = Circle(P, FVector2f(0.f, 0.22f), 0.42f);
			const float Up = FMath::Max(Line(P, FVector2f(0.f, -0.72f), FVector2f(0.f, 0.15f), 0.f) - (0.42f * FMath::Clamp((P.Y + 0.72f) / 0.94f, 0.f, 1.f)), -1.f);
			const float Inner = Circle(P, FVector2f(0.06f, 0.30f), 0.16f);
			return FMath::Max(FMath::Min(Body, Up), -Inner);
		};
	}
	else if (Name == TEXT("Nara"))
	{
		// Lua crescente (sombra)
		Shape = [&](const FVector2f& P)
		{
			return FMath::Max(Circle(P, FVector2f(0.f, 0.f), 0.66f), -Circle(P, FVector2f(0.28f, -0.16f), 0.56f));
		};
	}
	else if (Name == TEXT("Senju"))
	{
		// Losango em contorno com traço no meio
		Shape = [&](const FVector2f& P)
		{
			const float Diamond = FMath::Abs((FMath::Abs(P.X) + FMath::Abs(P.Y)) * 0.7071f - 0.48f) - 0.07f;
			return FMath::Min(Diamond, Line(P, FVector2f(0.f, -0.42f), FVector2f(0.f, 0.42f), 0.10f));
		};
	}
	else if (Name == TEXT("Uchiha"))
	{
		// Leque: camada 0 = metade de cima (cor do clã), camada 1 = metade de baixo + cabo (branco)
		const FVector2f Fan(0.f, -0.14f);
		if (Layer == 0)
		{
			Shape = [&, Fan](const FVector2f& P) { return FMath::Max(Circle(P, Fan, 0.56f), P.Y - (Fan.Y - 0.02f)); };
		}
		else
		{
			Shape = [&, Fan](const FVector2f& P)
			{
				const float Lower = FMath::Max(Circle(P, Fan, 0.56f), (Fan.Y + 0.02f) - P.Y);
				return FMath::Min(Lower, Box(P, FVector2f(0.f, 0.62f), FVector2f(0.07f, 0.26f)));
			};
		}
	}
	if (!Shape || (Layer > 0 && Name != TEXT("Uchiha")))
	{
		IconCache.Add(Key, nullptr);
		return nullptr;
	}
	UTexture2D* Texture = MakeTexture(Size, Size, [&](float X, float Y)
	{
		const FVector2f P(X / PxPerUnit - 1.f, Y / PxPerUnit - 1.f);
		return FMath::Clamp(0.5f - Shape(P) * PxPerUnit, 0.f, 1.f);
	});
	IconCache.Add(Key, Texture);
	return Texture;
}

void UHypeCharacterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Elapsed = 0.f;
	ExitTime = -1.f;

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
    if (APawn* Pawn = GetOwningPlayerPawn()) { bPawnWasHidden = Pawn->IsHidden(); }
    if (UHypeProfileSubsystem* Profile = GetGameInstance()->GetSubsystem<UHypeProfileSubsystem>())
    {
        Profile->bEntryShown = true;
        bChoosingAffiliation = Profile->HasCharacter();
        for (int32 I=0; I<Villages.Num(); ++I)
        {
            if (Villages[I].Id == Profile->Vila)
            {
                SelectedVillage=I;
                const int32 ClanIndex=Villages[I].Clans.IndexOfByPredicate([&](const FHypeClanOption& C) { return C.Id==Profile->Cla; });
                if (ClanIndex!=INDEX_NONE) { SelectedClan=ClanIndex; }
                break;
            }
        }
    }
    Refresh();
	SetupPreviewCamera();
    PreloadParts();
    UpdatePreviewVisibility();
    SetVisibility(ESlateVisibility::Visible);	// recebe arrastar/rolar nos espaços vazios
    bShots = FParse::Param(FCommandLine::Get(), TEXT("CreationShots"));

    // Tela aberta fora do mapa do jogo (L_CharacterCreation): o jogo carrega em segundo plano enquanto o jogador cria
    const FString GamePackage = MapaDoJogo.ToSoftObjectPath().GetLongPackageName();
    const FString ThisPackage = UWorld::RemovePIEPrefix(GetWorld()->GetOutermost()->GetName());
    bTravelAfter = !GamePackage.IsEmpty() && GamePackage != ThisPackage;
    if (bTravelAfter)
    {
        if (UHypeLoadingSubsystem* Loader = GetGameInstance()->GetSubsystem<UHypeLoadingSubsystem>()) { Loader->StartPreload(MapaDoJogo); }
    }

	// Mouse livre só para a tela: o personagem não anda nem gira a câmera, e o HUD some
	PC->SetShowMouseCursor(true);
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(Mode);
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	if (AHypeWorldPlayerController* NinjaPC = Cast<AHypeWorldPlayerController>(PC))
	{
		NinjaPC->SetHUDHidden(true);
	}
}

void UHypeCharacterSelectWidget::NativeDestruct()
{
	// Tela fechada sem Próximo (troca de mapa etc.): não deixar a câmera de prévia sobrando
	if (ExitTime < 0.f && PreviewCamera)
	{
		PreviewCamera->Destroy();
		PreviewCamera = nullptr;
	}
	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		Pawn->SetActorHiddenInGame(bPawnWasHidden);
		if (PreviewCamera) { Pawn->SetActorRotation(PawnStartRotation); }
	}
    if (bCreatingCharacter)
    {
        if (APawn* Pawn = GetOwningPlayerPawn())
        {
            if (UHypeAppearanceComponent* Appearance = Pawn->FindComponentByClass<UHypeAppearanceComponent>()) { Appearance->Restore(); }
        }
    }
	Super::NativeDestruct();
}

void UHypeCharacterSelectWidget::SetupPreviewCamera()
{
	APlayerController* PC = GetOwningPlayer();
	APawn* Pawn = GetOwningPlayerPawn();
	UWorld* World = GetWorld();
	if (!PC || !Pawn || !World)
	{
		UE_LOG(LogHypeCharacterSelect, Warning, TEXT("Sem personagem: a câmera de prévia não foi criada"));
		return;
	}
	// Câmera parada à frente do personagem, olhando para ele (o personagem fica de frente para a tela).
	// O palco fica guardado: arrastar gira o personagem, a câmera não acompanha.
	StageOrigin = Pawn->GetActorLocation();
	StageForward = Pawn->GetActorForwardVector();
	StageRight = Pawn->GetActorRightVector();
	PawnStartRotation = Pawn->GetActorRotation();
	Zoom = 1.f;
	const FVector Forward = Pawn->GetActorForwardVector();
	const FVector Location = Pawn->GetActorLocation() + Forward * CameraDistance + Pawn->GetActorRightVector() * CameraSide + FVector::UpVector * CameraHeight;
	FRotator Rotation = (-Forward).Rotation();
	Rotation.Pitch = CameraPitch;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	PreviewCamera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Location, Rotation, Params);
	if (PreviewCamera)
	{
		PreviewCamera->GetCameraComponent()->SetFieldOfView(CameraFOV);
		PreviewCamera->GetCameraComponent()->SetConstraintAspectRatio(false);
		// Fundo desfocado como na referência: foco no personagem, lente bem aberta
		FPostProcessSettings& Post = PreviewCamera->GetCameraComponent()->PostProcessSettings;
		Post.bOverride_DepthOfFieldFocalDistance = true;
		Post.DepthOfFieldFocalDistance = CameraDistance;
		Post.bOverride_DepthOfFieldFstop = true;
		Post.DepthOfFieldFstop = BackgroundBlurFstop;
		Post.bOverride_DepthOfFieldMinFstop = true;
		Post.DepthOfFieldMinFstop = BackgroundBlurFstop;
		PreviewCamera->GetCameraComponent()->PostProcessBlendWeight = 1.f;
		PC->SetViewTargetWithBlend(PreviewCamera, 0.f);
	}
}

void UHypeCharacterSelectWidget::HandleClick()
{
	if (ExitTime >= 0.f)
	{
		return;
	}
	for (int32 i = 0; i < VillageButtons.Num(); ++i)
	{
		if (VillageButtons[i] && VillageButtons[i]->IsHovered())
		{
			if (Villages.IsValidIndex(i) && Villages[i].Clans.Num() > 0 && i != SelectedVillage)
			{
				SelectedVillage = i;
				SelectedClan = 0;
				Refresh();
			}
			return;
		}
	}
	for (int32 i = 0; i < ClanButtons.Num(); ++i)
	{
		if (!bChoosingAffiliation && ClanButtons[i] && ClanButtons[i]->IsHovered())
		{
			SelectedClan = i;
			Refresh();
			return;
		}
	}
	if (NextButton && NextButton->IsHovered())
	{
		if (bChoosingAffiliation) { ConfirmAffiliation(); }
        else { NextCreationStep(); }
	}
	else if (BackButton && BackButton->IsHovered())
	{
        if (bChoosingAffiliation) { bChoosingAffiliation=false; Refresh(true); }
        else { CancelCreation(); }
	}
	else if (LookButton && LookButton->IsHovered())
	{
		bLookFixed = !bLookFixed;
		bDragging = false;
		Refresh();
	}
}

void UHypeCharacterSelectWidget::Confirm()
{
    UHypeProfileSubsystem* Profile = GetGameInstance() ? GetGameInstance()->GetSubsystem<UHypeProfileSubsystem>() : nullptr;
    if (!Profile || !Profile->CreateCharacter(NAME_None, NAME_None, DraftAppearance))
    {
        ProfileError = NSLOCTEXT("HypeSelect", "SaveFailed", "Não foi possível salvar. Tente novamente.");
        Refresh();
        return;
    }
    UE_LOG(LogHypeCharacterSelect, Log, TEXT("Personagem salvo: vila %s, clã %s"), *Profile->Vila.ToString(), *Profile->Cla.ToString());
    if (UHypeAppearanceComponent* Appearance = GetOwningPlayerPawn()->FindComponentByClass<UHypeAppearanceComponent>()) { Appearance->Commit(Profile->Appearance); }
    bCreatingCharacter = false;
    ExitTime = 0.f; // criação concluída: entra direto, menu de retorno só na próxima abertura.
}

void UHypeCharacterSelectWidget::UpdatePreviewVisibility()
{
    ApplyDraftPreview();
    const UHypeProfileSubsystem* Profile = GetGameInstance() ? GetGameInstance()->GetSubsystem<UHypeProfileSubsystem>() : nullptr;
    if (APawn* Pawn = GetOwningPlayerPawn())
    {
        Pawn->SetActorHiddenInGame(bPawnWasHidden || (!bCreatingCharacter && (!Profile || !Profile->HasCharacter())));
    }
}

void UHypeCharacterSelectWidget::StartCreation()
{
    if (ExitTime >= 0.f) { return; }
    CreationStep = 1;
    bChoosingAffiliation = false;
    DraftAppearance = FHypeAppearance();
    DraftAppearance.bCustomized = true;
    // Começa vestido como na referência: primeiro cabelo e primeiro traje do clã
    for (const FName Key : { FName(TEXT("Hair")), FName(TEXT("Outfit")) })
    {
        TArray<FName> Ids; TArray<FText> Names;
        GetPartOptions(Key, Ids, Names);
        if (Ids.Num() > 1) { DraftPart(Key) = Ids[1]; }
    }
    bCreatingCharacter = true;
    ProfileError = FText::GetEmpty();
    Refresh(true);
}

void UHypeCharacterSelectWidget::CancelCreation()
{
    if (UHypeAppearanceComponent* Appearance = GetOwningPlayerPawn()->FindComponentByClass<UHypeAppearanceComponent>()) { Appearance->Restore(); }
    bCreatingCharacter = false;
    ProfileError = FText::GetEmpty();
    Refresh(true);
}

void UHypeCharacterSelectWidget::EnterGame()
{
    const UHypeProfileSubsystem* Profile = GetGameInstance() ? GetGameInstance()->GetSubsystem<UHypeProfileSubsystem>() : nullptr;
    if (ExitTime < 0.f && Profile && Profile->HasCharacter()) { ExitTime = 0.f; }
}

void UHypeCharacterSelectWidget::Finish()
{
	if (bTravelAfter)
	{
		// Escurece o 3D e viaja para o mapa do jogo quando ele terminar de carregar (NativeTick)
		if (TravelWait < 0.f)
		{
			TravelWait = 0.f;
			if (APlayerController* PC = GetOwningPlayer())
			{
				if (PC->PlayerCameraManager) { PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, 0.3f, FLinearColor::Black, false, true); }
			}
		}
		return;
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APawn* Pawn = GetOwningPlayerPawn())
		{
			Pawn->SetActorRotation(PawnStartRotation);
			PC->SetViewTargetWithBlend(Pawn, CameraBlendTime, VTBlend_Cubic);
		}
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		PC->ResetIgnoreMoveInput();
		PC->ResetIgnoreLookInput();
		if (AHypeWorldPlayerController* NinjaPC = Cast<AHypeWorldPlayerController>(PC))
		{
			NinjaPC->SetHUDHidden(false);
		}
	}
	if (PreviewCamera)
	{
		PreviewCamera->SetLifeSpan(CameraBlendTime + 0.5f);	// precisa existir até a câmera terminar de voltar
		PreviewCamera = nullptr;
	}
	RemoveFromParent();
}

void UHypeCharacterSelectWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	using namespace HypeSelect;

	if (!Canvas)
	{
		return;
	}
	const float Dt = FMath::Min(InDeltaTime, 0.1f);
	Elapsed += Dt;
	float Alpha = EaseOut((Elapsed - FadeInDelay) / FMath::Max(FadeInTime, 0.01f));
	if (ExitTime >= 0.f)
	{
		ExitTime += Dt;
		Alpha *= 1.f - EaseOut(ExitTime / FMath::Max(FadeOutTime, 0.01f));
		if (ExitTime >= FadeOutTime)
		{
			Canvas->SetRenderOpacity(0.f);
			Finish();
			if (TravelWait >= 0.f && TravelWait < 1000.f)
			{
				TravelWait += Dt;
				UHypeLoadingSubsystem* Loader = GetGameInstance() ? GetGameInstance()->GetSubsystem<UHypeLoadingSubsystem>() : nullptr;
				if (TravelWait >= 0.35f && (!Loader || Loader->IsPreloaded() || Loader->HasLoadFailed()))
				{
					TravelWait = 1001.f;
					if (APlayerController* PC = GetOwningPlayer())
					{
						PC->SetShowMouseCursor(false);
						PC->SetInputMode(FInputModeGameOnly());
						PC->ResetIgnoreMoveInput();
						PC->ResetIgnoreLookInput();
					}
					if (Loader && Loader->IsPreloaded()) { Loader->TravelToPreloadedMap(0.8f, nullptr); }
					else { UGameplayStatics::OpenLevelBySoftObjectPtr(this, MapaDoJogo); }
				}
			}
			return;
		}
	}
	Canvas->SetRenderOpacity(Alpha);
	if (bShots) { TickShots(Dt); }
	// Entrada: o painel desliza da esquerda junto com o fade
	Canvas->SetRenderTranslation(FVector2D(ExitTime >= 0.f ? 0.f : -PanelSlide * (1.f - Alpha), 0.f));
    UpdateCreationCamera(Dt);

    // Saída curta, troca do conteúdo no ponto invisível e entrada com deslocamento de 8 px.
    if (PageTime >= 0.f)
    {
        PageTime += Dt;
        if (bPageRebuildPending && PageTime >= 0.09f)
        {
            Canvas->ClearChildren();
            BuildLayout();
            UpdatePreviewVisibility();
            bPageRebuildPending = false;
            PageTime = 0.f;
        }
        const float Progress = FMath::Clamp(PageTime / (bPageRebuildPending ? 0.09f : 0.13f), 0.f, 1.f);
        const float RevealAmount = bPageRebuildPending ? 1.f - Progress * Progress : EaseOut(Progress);
        for (const auto& Entry : PageOpacities)
        {
            if (UWidget* Widget = Entry.Key.Get())
            {
                Widget->SetRenderOpacity(Entry.Value * RevealAmount);
                Widget->SetRenderTranslation(FVector2D(0.f, (1.f - RevealAmount) * (bPageRebuildPending ? -5.f : 8.f)));
            }
        }
        if (!bPageRebuildPending && Progress >= 1.f)
        {
            PageTime = -1.f;
            Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
    }
    // Exponencial independente do FPS: realce suave, foco por teclado e pressão discreta.
    const float HoverBlend = 1.f - FMath::Exp(-18.f * Dt);
    for (auto& Entry : ButtonHover)
    {
        if (UButton* Button = Entry.Key.Get())
        {
            const bool bActive = Button->GetIsEnabled() && (Button->IsHovered() || Button->HasKeyboardFocus());
            Entry.Value = FMath::Lerp(Entry.Value, bActive ? 1.f : 0.f, HoverBlend);
            const float TargetScale = Button->IsPressed() ? 0.97f : 1.f + 0.02f * Entry.Value;
            const FVector2D Scale = Button->GetRenderTransform().Scale;
            Button->SetRenderScale(FMath::Lerp(Scale, FVector2D(TargetScale), HoverBlend));
            FButtonStyle Style = Button->GetStyle();
            const FSlateBrush Glow = Rounded(FLinearColor(1.f, 1.f, 1.f, 0.07f * Entry.Value), 6.f);
            Style.SetNormal(Glow);
            Style.SetHovered(Glow);
            Button->SetStyle(Style);
        }
    }

	// Contorno tracejado com o mouse em cima de Voltar / Próximo
	if (BackDashes && BackButton)
	{
		BackDashes->SetRenderOpacity(FMath::FInterpTo(BackDashes->GetRenderOpacity(), BackButton->IsHovered() ? 1.f : 0.f, Dt, 16.f));
	}
	if (NextDashes && NextButton)
	{
		NextDashes->SetRenderOpacity(FMath::FInterpTo(NextDashes->GetRenderOpacity(), NextButton->IsHovered() ? 0.95f : (bCreatingCharacter ? 0.55f : 0.f), Dt, 16.f));
	}
}

void UHypeCharacterSelectWidget::ApplyDraftPreview()
{
    if (APawn* Pawn = GetOwningPlayerPawn())
    {
        if (UHypeAppearanceComponent* Appearance = Pawn->FindComponentByClass<UHypeAppearanceComponent>())
        {
            if (bCreatingCharacter) { Appearance->Preview(DraftAppearance); }
            else { Appearance->Restore(); }
        }
    }
}

void UHypeCharacterSelectWidget::HandleNameChanged(const FText& Text)
{
    DraftAppearance.Name = Text.ToString();
    DraftAppearance.Sanitize();
    if (NameInput && NameInput->GetText().ToString() != DraftAppearance.Name) { NameInput->SetText(FText::FromString(DraftAppearance.Name)); }
    ProfileError = FText::GetEmpty();
    if (CreationErrorLabel) { CreationErrorLabel->SetText(ProfileError); }
}

void UHypeCharacterSelectWidget::HandleHeightChanged(float Value)
{
    DraftAppearance.HeightCm = FMath::RoundToFloat(160.f + Value*40.f);
    SyncAppearanceControls();
    ApplyDraftPreview();
}
void UHypeCharacterSelectWidget::HandleSkinChanged(float Value)
{
    const float Position=FMath::Clamp(Value,0.f,1.f)*(HypeSelect::SkinColors.Num()-1);
    const int32 Low=FMath::FloorToInt(Position), High=FMath::Min(Low+1,HypeSelect::SkinColors.Num()-1);
    DraftAppearance.SkinTint=FMath::Lerp(HypeSelect::SkinColors[Low],HypeSelect::SkinColors[High],Position-Low);
    SyncAppearanceControls();
    ApplyDraftPreview();
}
void UHypeCharacterSelectWidget::NextCreationStep()
{
    if (PageTime>=0.f || ExitTime>=0.f) { return; }
    DraftAppearance.Sanitize();
    if (CreationStep == 1 && DraftAppearance.Name.Len()<2)
    {
        ProfileError=FText::FromString(TEXT("Escolha um primeiro nome com pelo menos 2 caracteres."));
        if (CreationErrorLabel) { CreationErrorLabel->SetText(ProfileError); }
        return;
    }
    ProfileError=FText::GetEmpty();
    if (CreationStep == 4) { Confirm(); return; }
    ++CreationStep;
    Refresh(true);
}
void UHypeCharacterSelectWidget::PreviousCreationStep()
{
    if (PageTime>=0.f || ExitTime>=0.f) { return; }
    ProfileError=FText::GetEmpty();
    if (CreationStep>1) { --CreationStep; Refresh(true); }
    else { CancelCreation(); }
}
void UHypeCharacterSelectWidget::UpdateCreationCamera(float Dt)
{
    APawn* Pawn=GetOwningPlayerPawn();
    if (!PreviewCamera || !Pawn || ExitTime>=0.f) { return; }
    // Corpo inteiro (Identidade, Estilo, Confirmar) ou rosto (Aparência)
    const bool bFace=bCreatingCharacter && CreationStep==2;
    FVector LookAt, Location;
    FRotator Rotation;
    if (bFace)
    {
        const USkeletalMeshComponent* Body=GetNinjaVisibleBody(Cast<ACharacter>(Pawn));
        const FName HeadBone(TEXT("head"));
        LookAt=(Body && Body->DoesSocketExist(HeadBone)) ? Body->GetSocketLocation(HeadBone)+FVector(0,0,3) : StageOrigin+FVector(0,0,75);
        Location=LookAt+StageForward*FaceDistance*Zoom+StageRight*CameraSide*0.25f;
        Rotation=(LookAt-Location).Rotation();
    }
    else
    {
        Location=StageOrigin+StageForward*CameraDistance*Zoom+StageRight*CameraSide+FVector::UpVector*CameraHeight;
        LookAt=StageOrigin+FVector::UpVector*CameraHeight;
        Rotation=(-StageForward).Rotation();
        Rotation.Pitch=CameraPitch;
    }
    // ~99% do caminho em CameraMoveTime, independente do FPS
    const float Alpha=1.f-FMath::Exp(-4.6f*Dt/FMath::Max(CameraMoveTime,0.05f));
    PreviewCamera->SetActorLocationAndRotation(FMath::Lerp(PreviewCamera->GetActorLocation(),Location,Alpha),
        FQuat::Slerp(PreviewCamera->GetActorRotation().Quaternion(),Rotation.Quaternion(),Alpha));
    FPostProcessSettings& Post=PreviewCamera->GetCameraComponent()->PostProcessSettings;
    Post.DepthOfFieldFocalDistance=FMath::Lerp(Post.DepthOfFieldFocalDistance,float(FVector::Dist(PreviewCamera->GetActorLocation(),LookAt)),Alpha);
}

void UHypeCharacterSelectWidget::ConfirmAffiliation()
{
    if (PageTime>=0.f || ExitTime>=0.f || !bChoosingAffiliation) { return; }
    const FHypeVillageOption* Village=Villages.IsValidIndex(SelectedVillage) ? &Villages[SelectedVillage] : nullptr;
    const FHypeClanOption* Clan=Village && Village->Clans.IsValidIndex(SelectedClan) ? &Village->Clans[SelectedClan] : nullptr;
    UHypeProfileSubsystem* Profile=GetGameInstance()->GetSubsystem<UHypeProfileSubsystem>();
    if (!Profile || !Village || !Clan || !Profile->SaveAffiliation(Village->Id,Clan->Id))
    {
        ProfileError=FText::FromString(TEXT("Não foi possível salvar a escolha. Tente novamente."));
        Refresh();
        return;
    }
    ProfileError=FText::GetEmpty();
    bChoosingAffiliation=false;
    Refresh(true);
}

void UHypeCharacterSelectWidget::SyncAppearanceControls()
{
    // Posição da cor de pele atual no degradê (ponto mais próximo da linha entre as cores de referência)
    float Position=0.f,Best=MAX_flt;
    const auto& Colors=HypeSelect::SkinColors;
    for (int32 I=0;I<Colors.Num()-1;++I)
    {
        const FLinearColor AB=Colors[I+1]-Colors[I], AP=DraftAppearance.SkinTint-Colors[I];
        const float Length=AB.R*AB.R+AB.G*AB.G+AB.B*AB.B;
        const float T=FMath::Clamp((AP.R*AB.R+AP.G*AB.G+AP.B*AB.B)/FMath::Max(Length,0.000001f),0.f,1.f);
        const FLinearColor D=DraftAppearance.SkinTint-FMath::Lerp(Colors[I],Colors[I+1],T);
        const float Distance=D.R*D.R+D.G*D.G+D.B*D.B;
        if (Distance<Best) { Best=Distance; Position=(I+T)/(Colors.Num()-1); }
    }
    if (SkinSlider) { SkinSlider->SetValue(Position); }
    if (HeightSlider) { HeightSlider->SetValue((DraftAppearance.HeightCm-160.f)/40.f); }
    if (SkinAmountLabel) { SkinAmountLabel->SetText(FText::FromString(FString::Printf(TEXT("%.0f%%"),Position*100.f))); }
    if (HeightLabel) { HeightLabel->SetText(FText::FromString(FString::Printf(TEXT("%.0f cm"),DraftAppearance.HeightCm))); }
    if (HairColorHex) { HairColorHex->SetText(FText::FromString(TEXT("#")+DraftAppearance.HairColor.ToFColor(true).ToHex().Left(6))); }
    if (HairColorRow) { HairColorRow->SetIsEnabled(!DraftAppearance.HairId.IsNone()); }
}

void UHypeCharacterSelectWidget::GoToCategory(int32 Step)
{
    if (!bCreatingCharacter || PageTime>=0.f || ExitTime>=0.f || Step<1 || Step>4 || Step==CreationStep) { return; }
    DraftAppearance.Sanitize();
    if (Step>1 && DraftAppearance.Name.Len()<2)
    {
        ProfileError=FText::FromString(TEXT("Informe seu primeiro nome antes de continuar."));
        if (CreationErrorLabel) { CreationErrorLabel->SetText(ProfileError); }
        return;
    }
    ProfileError=FText::GetEmpty();
    CreationStep=Step;
    Refresh(true);
}
void UHypeCharacterSelectWidget::ShowIdentityCategory() { GoToCategory(1); }
void UHypeCharacterSelectWidget::ShowAppearanceCategory() { GoToCategory(2); }
void UHypeCharacterSelectWidget::ShowStyleCategory() { GoToCategory(3); }
void UHypeCharacterSelectWidget::ShowConfirmCategory() { GoToCategory(4); }

// ---------------------------------------------------------------- criação: peças, prévias e mouse

namespace HypeSelect
{
    ENinjaCustomizationSlot CatalogSlot(FName Key)
    {
        if (Key == TEXT("Eyes")) { return ENinjaCustomizationSlot::Eyes; }
        if (Key == TEXT("Bottom")) { return ENinjaCustomizationSlot::Bottom; }
        if (Key == TEXT("Shoes")) { return ENinjaCustomizationSlot::Shoes; }
        return ENinjaCustomizationSlot::Top;
    }
    FText CardTitle(FName Key)
    {
        if (Key == TEXT("Eyes")) { return FText::FromString(TEXT("Olhos")); }
        if (Key == TEXT("Hair")) { return FText::FromString(TEXT("Cabelo")); }
        if (Key == TEXT("Outfit")) { return FText::FromString(TEXT("Roupa completa")); }
        if (Key == TEXT("Top")) { return FText::FromString(TEXT("Parte de cima")); }
        if (Key == TEXT("Bottom")) { return FText::FromString(TEXT("Parte de baixo")); }
        return FText::FromString(TEXT("Calçado"));
    }
}

void UHypeCharacterSelectWidget::GetPartOptions(FName Key, TArray<FName>& Ids, TArray<FText>& Names) const
{
    Ids = { NAME_None };
    Names = { FText::FromString(Key == TEXT("Eyes") ? TEXT("Padrão") : TEXT("Nenhum")) };
    const APawn* Pawn = GetOwningPlayerPawn();
    const UHypeAppearanceComponent* Appearance = Pawn ? Pawn->FindComponentByClass<UHypeAppearanceComponent>() : nullptr;
    if (!Appearance)
    {
        return;
    }
    if (Key == TEXT("Hair") || Key == TEXT("Outfit"))
    {
        const FName Clan = DraftAppearance.Clan.IsNone() ? HypeIdentity::ResolveClan(NAME_None) : DraftAppearance.Clan;
        for (const FHypeCosmeticPart& Part : Key == TEXT("Hair") ? Appearance->HairOptions : Appearance->OutfitOptions)
        {
            if (Part.AllowsClan(Clan)) { Ids.Add(Part.Id); Names.Add(Part.Label); }
        }
        return;
    }
    const ENinjaCustomizationSlot Slot = HypeSelect::CatalogSlot(Key);
    for (const FName& Id : Appearance->GetCatalogParts(Slot))
    {
        const FNinjaCustomizationPart* Part = Appearance->FindCatalogPart(Id, Slot);
        Ids.Add(Id);
        Names.Add(Part && !Part->DisplayName.IsEmpty() ? Part->DisplayName : FText::FromName(Id));
    }
}

FName& UHypeCharacterSelectWidget::DraftPart(FName Key)
{
    if (Key == TEXT("Hair")) { return DraftAppearance.HairId; }
    if (Key == TEXT("Outfit")) { return DraftAppearance.OutfitId; }
    if (Key == TEXT("Eyes")) { return DraftAppearance.EyesId; }
    if (Key == TEXT("Top")) { return DraftAppearance.TopId; }
    if (Key == TEXT("Bottom")) { return DraftAppearance.BottomId; }
    return DraftAppearance.ShoesId;
}

UTexture2D* UHypeCharacterSelectWidget::GetThumbnail(FName Key, FName Id)
{
    if (Id.IsNone())
    {
        return nullptr;
    }
    if (Key == TEXT("Eyes"))
    {
        const APawn* Pawn = GetOwningPlayerPawn();
        const UHypeAppearanceComponent* Appearance = Pawn ? Pawn->FindComponentByClass<UHypeAppearanceComponent>() : nullptr;
        const FNinjaCustomizationPart* Part = Appearance ? Appearance->FindCatalogPart(Id, ENinjaCustomizationSlot::Eyes) : nullptr;
        return Part ? Part->EyeTexture.LoadSynchronous() : nullptr;
    }
    if (const TObjectPtr<UTexture2D>* Cached = ThumbCache.Find(Id))
    {
        return Cached->Get();
    }
    // Prévia gerada pela câmera de captura (gerar_thumbnails_pecas.py); sem ela o card fica só com o texto
    const FString Package = FString::Printf(TEXT("%s/T_Thumb_%s"), *ThumbnailFolder, *Id.ToString());
    UTexture2D* Texture = FPackageName::DoesPackageExist(Package)
        ? LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("%s.T_Thumb_%s"), *Package, *Id.ToString()), nullptr, LOAD_NoWarn | LOAD_Quiet)
        : nullptr;
    ThumbCache.Add(Id, Texture);
    return Texture;
}

void UHypeCharacterSelectWidget::UpdateCards()
{
    for (const auto& Entry : Cards)
    {
        UHypeOptionCardWidget* Card = Entry.Value.Get();
        if (!Card)
        {
            continue;
        }
        TArray<FName> Ids; TArray<FText> Names;
        GetPartOptions(Entry.Key, Ids, Names);
        const int32 Index = FMath::Max(Ids.IndexOfByKey(DraftPart(Entry.Key)), 0);
        Card->Setup(HypeSelect::CardTitle(Entry.Key), Index, Ids.Num(), Names[Index], GetThumbnail(Entry.Key, Ids[Index]));
        // Com traje inteiro as peças avulsas de corpo não aparecem: cards apagados
        const bool bCovered = !DraftAppearance.OutfitId.IsNone() && (Entry.Key == TEXT("Top") || Entry.Key == TEXT("Bottom") || Entry.Key == TEXT("Shoes"));
        Card->SetRenderOpacity(bCovered ? 0.55f : 1.f);
        PageOpacities.Add(Card, Card->GetRenderOpacity());
    }
}

void UHypeCharacterSelectWidget::PreloadParts()
{
    // Tudo em memória antes de abrir: trocar de peça não espera disco (sem "piscar")
    const APawn* Pawn = GetOwningPlayerPawn();
    const UHypeAppearanceComponent* Appearance = Pawn ? Pawn->FindComponentByClass<UHypeAppearanceComponent>() : nullptr;
    if (!Appearance || !Appearance->PartsCatalog)
    {
        return;
    }
    for (const ENinjaCustomizationSlot Slot : { ENinjaCustomizationSlot::Eyes, ENinjaCustomizationSlot::Top, ENinjaCustomizationSlot::Bottom, ENinjaCustomizationSlot::Shoes })
    {
        for (const FName& Id : Appearance->GetCatalogParts(Slot))
        {
            if (const FNinjaCustomizationPart* Part = Appearance->FindCatalogPart(Id, Slot))
            {
                Part->Material.LoadSynchronous();
                Part->EyeTexture.LoadSynchronous();
            }
        }
    }
}

void UHypeCharacterSelectWidget::HandleCardStep(UHypeOptionCardWidget* Card, int32 Delta)
{
    if (!Card || ExitTime >= 0.f)
    {
        return;
    }
    const FName Key = Card->Key;
    TArray<FName> Ids; TArray<FText> Names;
    GetPartOptions(Key, Ids, Names);
    const int32 Current = FMath::Max(Ids.IndexOfByKey(DraftPart(Key)), 0);
    const FName Chosen = Ids[(Current + Delta + Ids.Num() * 4) % Ids.Num()];
    DraftPart(Key) = Chosen;
    // Traje inteiro e peças avulsas de corpo não se misturam
    if (!Chosen.IsNone() && Key == TEXT("Outfit"))
    {
        DraftAppearance.TopId = DraftAppearance.BottomId = DraftAppearance.ShoesId = NAME_None;
    }
    else if (!Chosen.IsNone() && (Key == TEXT("Top") || Key == TEXT("Bottom") || Key == TEXT("Shoes")))
    {
        DraftAppearance.OutfitId = NAME_None;
    }
    ApplyDraftPreview();
    UpdateCards();
    SyncAppearanceControls();
}

void UHypeCharacterSelectWidget::HandleHairColor(UHypeColorRowWidget* Row, int32 Index)
{
    if (HypeSelect::HairColors.IsValidIndex(Index))
    {
        DraftAppearance.HairColor = HypeSelect::HairColors[Index];
        ApplyDraftPreview();
        SyncAppearanceControls();
    }
}

void UHypeCharacterSelectWidget::HandleSliderChanged(UHypeGradientSliderWidget* Slider, float Value)
{
    if (Slider && Slider->Key == TEXT("Skin")) { HandleSkinChanged(Value); }
    else if (Slider && Slider->Key == TEXT("Height")) { HandleHeightChanged(Value); }
}

FReply UHypeCharacterSelectWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // Só chega aqui o clique em espaço vazio (botões e cards ficam com o deles): arrastar gira o personagem
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && PreviewCamera && !bLookFixed && ExitTime < 0.f)
    {
        bDragging = true;
        DragLastX = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()).X;
        return FReply::Handled().CaptureMouse(TakeWidget());
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UHypeCharacterSelectWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bDragging)
    {
        const float X = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()).X;
        if (APawn* Pawn = GetOwningPlayerPawn())
        {
            Pawn->AddActorWorldRotation(FRotator(0.f, -(X - DragLastX) * DragDegreesPerPixel, 0.f));
        }
        DragLastX = X;
        return FReply::Handled();
    }
    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UHypeCharacterSelectWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bDragging)
    {
        bDragging = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UHypeCharacterSelectWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (PreviewCamera && ExitTime < 0.f)
    {
        // Zoom leve: rolar para cima aproxima
        Zoom = FMath::Clamp(Zoom - InMouseEvent.GetWheelDelta() * 0.05f, ZoomRange.X, ZoomRange.Y);
        return FReply::Handled();
    }
    return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

UTexture2D* UHypeCharacterSelectWidget::GetCreatorIcon(FName Name)
{
    using namespace HypeSelect;
    const FName CacheKey(*(TEXT("Creator_") + Name.ToString()));
    if (const TObjectPtr<UTexture2D>* Found = IconCache.Find(CacheKey))
    {
        return Found->Get();
    }
    // Ícones da coluna fina, desenhados por código (formas simples em 64 px, brancos)
    auto Box = [](const FVector2f& P, float X0, float Y0, float X1, float Y1) { return P.X >= X0 && P.X <= X1 && P.Y >= Y0 && P.Y <= Y1; };
    auto Disc = [](const FVector2f& P, const FVector2f& C, float R) { return FVector2f::Distance(P, C) <= R; };
    TFunction<bool(const FVector2f&)> Shape;
    if (Name == TEXT("Person"))
    {
        Shape = [&](const FVector2f& P) { return Disc(P, FVector2f(0.f, -0.2f), 0.13f) || (P.Y > 0.08f && P.Y < 0.38f && FMath::Square(P.X / 0.3f) + FMath::Square((P.Y - 0.38f) / 0.3f) <= 1.f); };
    }
    else if (Name == TEXT("Drop"))
    {
        Shape = [&](const FVector2f& P) { return Disc(P, FVector2f(0.f, 0.12f), 0.22f) || (P.Y > -0.38f && P.Y <= 0.12f && FMath::Abs(P.X) <= 0.22f * (P.Y + 0.38f) / 0.5f); };
    }
    else if (Name == TEXT("Height"))
    {
        Shape = [&](const FVector2f& P)
        {
            const bool Stem = FMath::Abs(P.X) < 0.035f && FMath::Abs(P.Y) < 0.36f;
            const bool Up = P.Y < -0.18f && P.Y > -0.4f && FMath::Abs(P.X) <= (P.Y + 0.4f) * 0.9f;
            const bool Down = P.Y > 0.18f && P.Y < 0.4f && FMath::Abs(P.X) <= (0.4f - P.Y) * 0.9f;
            return Stem || Up || Down;
        };
    }
    else if (Name == TEXT("Eye"))
    {
        Shape = [&](const FVector2f& P)
        {
            const float E = FMath::Sqrt(FMath::Square(P.X / 0.42f) + FMath::Square(P.Y / 0.24f));
            return FMath::Abs(E - 1.f) < 0.12f || P.Size() < 0.11f;
        };
    }
    else if (Name == TEXT("Hair"))
    {
        Shape = [&](const FVector2f& P)
        {
            const bool Dome = P.Y <= 0.05f && Disc(P, FVector2f(0.f, 0.05f), 0.36f);
            const bool Locks = P.Y > 0.05f && P.Y < 0.3f && FMath::Abs(P.X) > 0.2f && FMath::Abs(P.X) < 0.36f;
            return (Dome && !(P.Y > -0.08f && FMath::Abs(P.X) < 0.2f)) || Locks;
        };
    }
    else if (Name == TEXT("Shirt") || Name == TEXT("Top"))
    {
        Shape = [&](const FVector2f& P)
        {
            const bool Body = Box(P, -0.21f, -0.28f, 0.21f, 0.38f);
            const bool Sleeves = Box(P, -0.4f, -0.28f, 0.4f, -0.06f);
            return (Body || Sleeves) && !Disc(P, FVector2f(0.f, -0.32f), 0.1f);
        };
    }
    else if (Name == TEXT("Pants"))
    {
        Shape = [&](const FVector2f& P) { return Box(P, -0.26f, -0.38f, 0.26f, -0.2f) || Box(P, -0.26f, -0.2f, -0.04f, 0.4f) || Box(P, 0.04f, -0.2f, 0.26f, 0.4f); };
    }
    else if (Name == TEXT("Shoe"))
    {
        Shape = [&](const FVector2f& P) { return Box(P, -0.34f, 0.08f, 0.36f, 0.26f) || Box(P, -0.34f, -0.24f, -0.02f, 0.1f) || Disc(P, FVector2f(0.22f, 0.12f), 0.14f); };
    }
    else
    {
        Shape = [&](const FVector2f& P)
        {
            return SegmentDistance(P, FVector2f(-0.3f, 0.02f), FVector2f(-0.08f, 0.24f)) < 0.07f || SegmentDistance(P, FVector2f(-0.08f, 0.24f), FVector2f(0.32f, -0.22f)) < 0.07f;
        };
    }
    UTexture2D* Texture = MakeTexture(64, 64, [&Shape](float X, float Y) { return Shape(FVector2f(X / 64.f - 0.5f, Y / 64.f - 0.5f)) ? 1.f : 0.f; });
    IconCache.Add(CacheKey, Texture);
    return Texture;
}

void UHypeCharacterSelectWidget::TickShots(float Dt)
{
    // Roteiro do teste: cada passo espera o anterior assentar (fade, câmera 0,4 s, troca de aba)
    ShotClock += Dt;
    struct FStep { float At; TFunction<void()> Run; };
    auto Shot = [](const TCHAR* Name)
    {
        FScreenshotRequest::RequestScreenshot(FString(Name), true, false);
        UE_LOG(LogHypeCharacterSelect, Log, TEXT("Foto %s"), Name);
    };
    auto Step = [this](FName Key, int32 Delta) { if (const TObjectPtr<UHypeOptionCardWidget>* Card = Cards.Find(Key)) { HandleCardStep(Card->Get(), Delta); } };
    const TArray<FStep> Steps = {
        { 2.0f, [this]() { if (!bCreatingCharacter) { StartCreation(); } } },
        { 3.0f, [this]() { HandleNameChanged(FText::FromString(TEXT("Felipe"))); if (NameInput) { NameInput->SetText(FText::FromString(DraftAppearance.Name)); } HandleSkinChanged(0.3f); } },
        { 4.0f, [&]() { Shot(TEXT("Criacao_1_Identidade")); } },
        { 4.5f, [this]() { ShowAppearanceCategory(); } },
        { 5.5f, [&]() { Step(TEXT("Eyes"), 1); } },
        { 6.5f, [&]() { Shot(TEXT("Criacao_2_Aparencia")); } },
        { 7.0f, [this]() { ShowStyleCategory(); } },
        { 8.0f, [&]() { Step(TEXT("Hair"), 15); if (HairColorRow) { HairColorRow->Setup(HypeSelect::HairColors, 2); } HandleHairColor(HairColorRow, 2); } },
        { 9.0f, [&]() { Shot(TEXT("Criacao_3_Estilo")); } },
        { 9.5f, [this]() { ShowConfirmCategory(); } },
        { 10.5f, [&]() { Shot(TEXT("Criacao_4_Confirmar")); } },
        { 11.5f, [this]()
            {
                UE_LOG(LogHypeCharacterSelect, Log, TEXT("Rascunho: nome %s, pele %s, altura %.0f, cabelo %s, cor %s, roupa %s, olhos %s, cima %s, baixo %s, calçado %s"),
                    *DraftAppearance.Name, *DraftAppearance.SkinTint.ToString(), DraftAppearance.HeightCm, *DraftAppearance.HairId.ToString(),
                    *DraftAppearance.HairColor.ToString(), *DraftAppearance.OutfitId.ToString(), *DraftAppearance.EyesId.ToString(),
                    *DraftAppearance.TopId.ToString(), *DraftAppearance.BottomId.ToString(), *DraftAppearance.ShoesId.ToString());
                NextCreationStep();
            } },
    };
    if (Steps.IsValidIndex(ShotStep) && ShotClock >= Steps[ShotStep].At)
    {
        Steps[ShotStep].Run();
        ++ShotStep;
    }
}
