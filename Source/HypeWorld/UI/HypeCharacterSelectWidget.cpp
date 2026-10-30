// Tela de seleção do personagem montada inteira em C++ (ver o .h). Layout em px de 1080p, medidos na imagem de
// referência do Felipe (1329x743 → ×1,45); a escala de DPI da Unreal ajusta para outras telas.

#include "HypeCharacterSelectWidget.h"
#include "HypeProfileSubsystem.h"
#include "Ninja/HypeAppearanceComponent.h"
#include "Components/EditableTextBox.h"
#include "Components/Slider.h"
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
        FLinearColor(0.7f, 0.72f, 0.76f), FLinearColor(0.4f, 0.06f, 0.025f)
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

	FSlateBrush Rounded(const FLinearColor& Fill, float Radius)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(FLinearColor::Transparent), 0.f);
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
    SkinFrames.Reset();
    SkinChecks.Reset();
    SkinAmountLabel = nullptr;
    NameInput = nullptr;
    HeightLabel = nullptr;
    HeightSlider = nullptr;
    SkinSlider = nullptr;
    CreationErrorLabel = nullptr;
    SkinButtons.Reset();
    HairColorButtons.Reset();
    HairChoice = nullptr;
    OutfitChoice = nullptr;
    VisibleOutfitIds.Reset();
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
        PageOpacities.Add(Widget, Widget->GetRenderOpacity());
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
                for (int32 Slot = 0; Slot < 6; ++Slot)
                {
                    if (const UNinjaJutsuData* Jutsu = Ninja->GetJutsuInSlot(Slot))
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
        const float FormLeft=ColumnCenter-172.f;
        const float FormWidth=344.f;
        const FLinearColor Muted=SRGB(193,191,185,0.72f);
        auto Label = [&](const FString& Text, float Y, int32 Size = 11)
        {
            UTextBlock* Widget=MakeText(FText::FromString(Text),FontInfo(TEXT("SemiBold"),Size),White);
            Widget->SetJustification(ETextJustify::Left);
            Widget->SetWrapTextAt(FormWidth);
            Place(Widget,TopLeft,FVector2D(FormLeft,Y),FVector2D::ZeroVector,FVector2D(0,0.5f));
            return Widget;
        };
        auto Action = [&](const FString& Text,float X,float Y,FName Function)
        {
            const bool bPrimary=Function==TEXT("NextCreationStep");
            const FVector2D Size(166,44);
            Place(MakeImage(Rounded(bPrimary ? SRGB(111,101,84,0.70f) : SRGB(8,10,11,0.94f),7.f)),TopLeft,FVector2D(X,Y),Size,Center);
            UImage* Dashes=MakeImage(TextureBrush(DashTexture,White),bPrimary ? 0.24f : 0.f);
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
            return Button;
        };
        const TCHAR* Tabs[]={TEXT("Identidade"),TEXT("Aparência"),TEXT("Estilo"),TEXT("Confirmar")};
        const FName CategoryActions[]={TEXT("ShowIdentityCategory"),TEXT("ShowAppearanceCategory"),TEXT("ShowStyleCategory"),TEXT("ShowConfirmCategory")};
        // Largura natural dos títulos + espaços explícitos: fontes/DPI não fazem os textos se sobreporem.
        UHorizontalBox* CategoryRow=WidgetTree->ConstructWidget<UHorizontalBox>();
        for (int32 Index=0;Index<4;++Index)
        {
            const bool bSelected=CreationStep==Index+1;
            UVerticalBox* Content=WidgetTree->ConstructWidget<UVerticalBox>();
            UTextBlock* Caption=MakeText(FText::FromString(Tabs[Index]),FontInfo(bSelected ? TEXT("Bold") : TEXT("SemiBold"),9),bSelected ? White : Muted);
            Content->AddChildToVerticalBox(Caption)->SetHorizontalAlignment(HAlign_Center);
            USizeBox* Underline=WidgetTree->ConstructWidget<USizeBox>();
            Underline->SetWidthOverride(48.f);
            Underline->SetHeightOverride(1.5f);
            Underline->AddChild(MakeImage(Rounded(FLinearColor(1,1,1,0.75f),0.5f),bSelected ? 1.f : 0.f));
            UVerticalBoxSlot* LineSlot=Content->AddChildToVerticalBox(Underline);
            LineSlot->SetPadding(FMargin(0,7,0,0));
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
            CategorySlot->SetPadding(FMargin(0,0,Index<3 ? 22.f : 0.f,0));
            CategorySlot->SetVerticalAlignment(VAlign_Center);
        }
        Place(CategoryRow,TopLeft,FVector2D(FormLeft,176),FVector2D::ZeroVector,FVector2D(0,0.5f));
        auto Palette = [&](const TArray<FLinearColor>& Colors,float Y,TArray<TObjectPtr<UButton>>& Buttons,const FLinearColor& Selected)
        {
            const float Pitch=FormWidth/Colors.Num();
            for (int32 Index=0;Index<Colors.Num();++Index)
            {
                const float X=FormLeft+(Index+0.5f)*Pitch;
                const bool bSelected=Colors[Index].Equals(Selected,0.001f);
                UImage* Frame=MakeImage(Rounded(White,13.f),bSelected ? 0.8f : 0.f);
                Place(Frame,TopLeft,FVector2D(X,Y),FVector2D(27,48),Center);
                Place(MakeImage(Rounded(Colors[Index],11.f)),TopLeft,FVector2D(X,Y),FVector2D(23,44),Center);
                UTextBlock* Check=MakeText(FText::FromString(TEXT("✓")),FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),9),Index<3 ? SRGB(30,27,24) : White);
                Check->SetRenderOpacity(bSelected ? 1.f : 0.f);
                Place(Check,TopLeft,FVector2D(X,Y),FVector2D::ZeroVector,Center);
                if (&Buttons==&SkinButtons) { SkinFrames.Add(Frame); SkinChecks.Add(Check); }
                UButton* B=MakeButton(11.f,&Buttons);
                B->OnClicked.Clear();
                B->OnClicked.AddDynamic(this,&UHypeCharacterSelectWidget::HandleCustomizationClick);
                Place(B,TopLeft,FVector2D(X,Y),FVector2D(Pitch-4,52),Center);
            }
        };
        auto SliderCard = [&](const FString& Title,float Y,UTextBlock*& ValueLabel,FName Minus,FName Plus)
        {
            Place(MakeImage(Rounded(SRGB(8,9,11,0.88f),7.f)),TopLeft,FVector2D(ColumnCenter,Y+39),FVector2D(FormWidth,78),Center);
            UTextBlock* Heading=MakeText(FText::FromString(Title),FontInfo(TEXT("SemiBold"),10),White);
            Place(Heading,TopLeft,FVector2D(FormLeft+18,Y+21),FVector2D::ZeroVector,FVector2D(0,0.5f));
            Place(MakeImage(Rounded(FLinearColor(1,1,1,0.065f),5.f)),TopLeft,FVector2D(FormLeft+FormWidth-43,Y+21),FVector2D(58,24),Center);
            ValueLabel=MakeText(FText::GetEmpty(),FontInfo(TEXT("SemiBold"),9),SRGB(219,217,209));
            Place(ValueLabel,TopLeft,FVector2D(FormLeft+FormWidth-43,Y+21),FVector2D::ZeroVector,Center);
            auto Arrow = [&](float X,const TCHAR* Text,FName Method)
            {
                UButton* B=MakeButton(4.f);
                B->OnClicked.Clear();
                FScriptDelegate D; D.BindUFunction(this,Method); B->OnClicked.Add(D);
                B->AddChild(MakeText(FText::FromString(Text),FontInfo(TEXT("SemiBold"),15),Muted));
                Place(B,TopLeft,FVector2D(X,Y+54),FVector2D(28,28),Center);
            };
            Arrow(FormLeft+18,TEXT("‹"),Minus);
            Arrow(FormLeft+FormWidth-18,TEXT("›"),Plus);
        };
        auto StyleSlider = [&](USlider* Slider)
        {
            FSliderStyle Style;
            Style.SetBarThickness(2.f);
            Style.SetNormalBarImage(Rounded(FLinearColor(1,1,1,0.2f),1.5f));
            Style.SetHoveredBarImage(Rounded(FLinearColor(1,1,1,0.35f),1.5f));
            FSlateBrush Thumb=Rounded(White,5.f);
            Thumb.ImageSize=FVector2D(7,7);
            Style.SetNormalThumbImage(Thumb);
            Style.SetHoveredThumbImage(Thumb);
            Style.SetDisabledThumbImage(Thumb);
            Slider->SetWidgetStyle(Style);
        };
        if (CreationStep == 1)
        {
            Label(TEXT("Primeiro nome"), 268, 12);
            NameInput = WidgetTree->ConstructWidget<UEditableTextBox>();
            FEditableTextBoxStyle InputStyle=NameInput->GetWidgetStyle();
            InputStyle.SetFont(FontInfo(TEXT("SemiBold"),11));
            InputStyle.SetForegroundColor(FSlateColor(White));
            InputStyle.SetFocusedForegroundColor(FSlateColor(White));
            InputStyle.SetPadding(FMargin(16,10));
            InputStyle.SetBackgroundImageNormal(Rounded(BackFill,10.f));
            InputStyle.SetBackgroundImageHovered(Rounded(SRGB(28,28,28,0.95f),10.f));
            InputStyle.SetBackgroundImageFocused(Rounded(SRGB(38,38,38,0.95f),10.f));
            NameInput->SetWidgetStyle(InputStyle);
            NameInput->SetHintText(FText::FromString(TEXT("Primeiro nome")));
            NameInput->SetText(FText::FromString(DraftAppearance.Name));
            NameInput->OnTextChanged.AddDynamic(this, &UHypeCharacterSelectWidget::HandleNameChanged);
            Place(NameInput, TopLeft, FVector2D(ColumnCenter, 322), FVector2D(FormWidth,44), Center);
            Label(TEXT("De 2 a 24 caracteres, sem sobrenome"), 367, 9)->SetColorAndOpacity(FSlateColor(Muted));
            Label(TEXT("O clã é definido automaticamente pelo jogo."), 424, 10)->SetColorAndOpacity(FSlateColor(Muted));

        }
        else if (CreationStep == 2)
        {
            Label(TEXT("Tom de pele"),268,11);
            Palette(SkinColors,316,SkinButtons,DraftAppearance.SkinTint);
            UTextBlock* SkinValue=nullptr;
            SliderCard(TEXT("Nuance da pele"),376,SkinValue,TEXT("LighterSkin"),TEXT("DarkerSkin"));
            SkinAmountLabel=SkinValue;
            SkinSlider=WidgetTree->ConstructWidget<USlider>();
            StyleSlider(SkinSlider);
            SkinSlider->OnValueChanged.AddDynamic(this,&UHypeCharacterSelectWidget::HandleSkinChanged);
            Place(SkinSlider,TopLeft,FVector2D(ColumnCenter,430),FVector2D(FormWidth-74,24),Center);
            UTextBlock* HeightValue=nullptr;
            SliderCard(TEXT("Altura"),478,HeightValue,TEXT("ShorterHeight"),TEXT("TallerHeight"));
            HeightLabel=HeightValue;
            HeightSlider=WidgetTree->ConstructWidget<USlider>();
            StyleSlider(HeightSlider);
            HeightSlider->OnValueChanged.AddDynamic(this,&UHypeCharacterSelectWidget::HandleHeightChanged);
            Place(HeightSlider,TopLeft,FVector2D(ColumnCenter,532),FVector2D(FormWidth-74,24),Center);
            SyncAppearanceControls();
        }
        else if (CreationStep == 3)
        {
            const UHypeAppearanceComponent* Appearance = GetOwningPlayerPawn()->FindComponentByClass<UHypeAppearanceComponent>();
            Label(TEXT("Cabelo"), 268, 11);
            auto Parts = [&](const TArray<FHypeCosmeticPart>& Options, FName Selected, float Y, TArray<FName>* VisibleIds=nullptr)
            {
                UComboBoxString* Choice=WidgetTree->ConstructWidget<UComboBoxString>();
                Choice->OnGenerateWidgetEvent.BindDynamic(this,&UHypeCharacterSelectWidget::GenerateCosmeticLabel);
                FComboBoxStyle ComboStyle=Choice->GetWidgetStyle();
                ComboStyle.ComboButtonStyle.ButtonStyle.SetNormal(Rounded(BackFill,6));
                ComboStyle.ComboButtonStyle.ButtonStyle.SetHovered(Rounded(BoxOn,6));
                ComboStyle.ComboButtonStyle.ButtonStyle.SetPressed(Rounded(BoxOn,6));
                ComboStyle.ComboButtonStyle.DownArrowImage.TintColor=FSlateColor(White);
                ComboStyle.ComboButtonStyle.MenuBorderBrush=Rounded(BackFill,6);
                Choice->SetWidgetStyle(ComboStyle);
                FTableRowStyle RowStyle=Choice->GetItemStyle();
                RowStyle.SetActiveBrush(Rounded(NextFill,3));
                RowStyle.SetInactiveBrush(Rounded(NextFill,3));
                Choice->SetItemStyle(RowStyle);
                Choice->SetMaxListHeight(280.f);
                Choice->AddOption(TEXT("Nenhum"));
                if (VisibleIds) { VisibleIds->Reset(); VisibleIds->Add(NAME_None); }
                int32 VisibleIndex=0;
                const FName Clan=DraftAppearance.Clan.IsNone() ? HypeIdentity::ResolveClan(NAME_None) : DraftAppearance.Clan;
                int32 SelectedIndex=0;
                for (int32 Index=0; Index<Options.Num(); ++Index)
                {
                    if (!Options[Index].AllowsClan(Clan)) { continue; }
                    ++VisibleIndex;
                    Choice->AddOption(Options[Index].Label.ToString());
                    if (VisibleIds) { VisibleIds->Add(Options[Index].Id); }
                    if (Options[Index].Id==Selected) { SelectedIndex=VisibleIndex; }
                }
                Choice->SetSelectedIndex(SelectedIndex);
                Place(Choice,TopLeft,FVector2D(ColumnCenter,Y),FVector2D(350,44),Center);
                return Choice;
            };
            // Sem modelos, a interface informa a disponibilidade em vez de oferecer escolhas falsas.
            if (!Appearance || Appearance->HairOptions.IsEmpty()) { Label(TEXT("Sem opções disponíveis"),314,10)->SetColorAndOpacity(FSlateColor(Muted)); }
            else
            {
                HairChoice=Parts(Appearance->HairOptions,DraftAppearance.HairId,318);
                HairChoice->OnSelectionChanged.AddDynamic(this,&UHypeCharacterSelectWidget::HandleHairSelected);
            }
            Label(TEXT("Cor do cabelo"),390,11);
            Palette(HairColors,436,HairColorButtons,DraftAppearance.HairColor);
            for (UButton* B : HairColorButtons) { B->SetIsEnabled(Appearance && !DraftAppearance.HairId.IsNone()); }
            Label(TEXT("Roupa"),524,11);
            if (!Appearance || Appearance->OutfitOptions.IsEmpty()) { Label(TEXT("Sem opções disponíveis"),570,10)->SetColorAndOpacity(FSlateColor(Muted)); }
            else
            {
                OutfitChoice=Parts(Appearance->OutfitOptions,DraftAppearance.OutfitId,570,&VisibleOutfitIds);
                OutfitChoice->OnSelectionChanged.AddDynamic(this,&UHypeCharacterSelectWidget::HandleOutfitSelected);
            }
        }
        else
        {
            Label(TEXT("Revise seu personagem"),268,16);
            Label(DraftAppearance.Name,338,18);

            Label(FString::Printf(TEXT("Altura: %.0f cm"),DraftAppearance.HeightCm),398,11);
            Label(TEXT("Confira as escolhas antes de começar."),466,10)->SetColorAndOpacity(FSlateColor(Muted));
        }
        Action(TEXT("Voltar"),ColumnCenter-89,660,TEXT("PreviousCreationStep"));
        Action(CreationStep == 4 ? TEXT("Criar personagem") : TEXT("Próximo"),ColumnCenter+89,660,TEXT("NextCreationStep"));
        CreationErrorLabel=Label(ProfileError.ToString(),712,9);
        CreationErrorLabel->SetColorAndOpacity(FSlateColor(SRGB(255,180,160)));
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
	Place(MakeText(NSLOCTEXT("HypeSelect", "LookFixed", "Olhar fixo"), FontInfo(TEXT("SemiBold"), 9), LookColor), BottomRight,
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
    UpdatePreviewVisibility();

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
	if (APawn* Pawn = GetOwningPlayerPawn()) { Pawn->SetActorHiddenInGame(bPawnWasHidden); }
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
	// Câmera parada à frente do personagem, olhando para ele (o personagem fica de frente para a tela)
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
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APawn* Pawn = GetOwningPlayerPawn())
		{
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
			Finish();
			return;
		}
	}
	Canvas->SetRenderOpacity(Alpha);
    UpdateCreationCamera(Dt);

    // Saída curta, troca do conteúdo no ponto invisível e entrada com deslocamento de 8 px.
    if (PageTime >= 0.f)
    {
        PageTime += Dt;
        if (bPageRebuildPending && PageTime >= 0.12f)
        {
            Canvas->ClearChildren();
            BuildLayout();
            UpdatePreviewVisibility();
            bPageRebuildPending = false;
            PageTime = 0.f;
        }
        const float Progress = FMath::Clamp(PageTime / (bPageRebuildPending ? 0.12f : 0.22f), 0.f, 1.f);
        const float Visibility = bPageRebuildPending ? 1.f - Progress * Progress : EaseOut(Progress);
        for (const auto& Entry : PageOpacities)
        {
            if (UWidget* Widget = Entry.Key.Get())
            {
                Widget->SetRenderOpacity(Entry.Value * Visibility);
                Widget->SetRenderTranslation(FVector2D(0.f, (1.f - Visibility) * (bPageRebuildPending ? -5.f : 8.f)));
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
		NextDashes->SetRenderOpacity(FMath::FInterpTo(NextDashes->GetRenderOpacity(), NextButton->IsHovered() ? 0.8f : (bCreatingCharacter ? 0.24f : 0.f), Dt, 16.f));
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
void UHypeCharacterSelectWidget::HandleCustomizationClick()
{
    for (int32 I=0; I<SkinButtons.Num(); ++I)
    {
        if (SkinButtons[I]->IsHovered())
        {
            DraftAppearance.SkinTint=HypeSelect::SkinColors[I];
            ApplyDraftPreview();
            Refresh();
            return;
        }
    }
    for (int32 I=0; I<HairColorButtons.Num(); ++I)
    {
        if (HairColorButtons[I]->IsHovered()) { DraftAppearance.HairColor=HypeSelect::HairColors[I]; ApplyDraftPreview(); Refresh(); return; }
    }
}
UWidget* UHypeCharacterSelectWidget::GenerateCosmeticLabel(FString Item)
{
    UTextBlock* Text=WidgetTree->ConstructWidget<UTextBlock>();
    UObject* Face=Font.LoadSynchronous();
    Text->SetFont(Face ? FSlateFontInfo(Face,10,FName(TEXT("Medium"))) : FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),10));
    Text->SetText(FText::FromString(Item));
    Text->SetColorAndOpacity(FSlateColor(HypeSelect::White));
    return Text;
}

void UHypeCharacterSelectWidget::HandleHairSelected(FString Item, ESelectInfo::Type Type)
{
    const UHypeAppearanceComponent* A=GetOwningPlayerPawn()->FindComponentByClass<UHypeAppearanceComponent>();
    const int32 Index=HairChoice ? HairChoice->GetSelectedIndex()-1 : INDEX_NONE;
    DraftAppearance.HairId=A && A->HairOptions.IsValidIndex(Index) ? A->HairOptions[Index].Id : NAME_None;
    ApplyDraftPreview();
    for (UButton* B : HairColorButtons) { B->SetIsEnabled(!DraftAppearance.HairId.IsNone()); }
}
void UHypeCharacterSelectWidget::HandleOutfitSelected(FString Item, ESelectInfo::Type Type)
{
    const UHypeAppearanceComponent* A=GetOwningPlayerPawn()->FindComponentByClass<UHypeAppearanceComponent>();
    const int32 Index=OutfitChoice ? OutfitChoice->GetSelectedIndex() : INDEX_NONE;
    DraftAppearance.OutfitId=A && VisibleOutfitIds.IsValidIndex(Index) ? VisibleOutfitIds[Index] : NAME_None;
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
    const APawn* Pawn=GetOwningPlayerPawn();
    if (!PreviewCamera || !Pawn || ExitTime>=0.f) { return; }
    const bool bClose=bCreatingCharacter && CreationStep==3;
    const float Distance=bClose ? CameraDistance*0.78f : CameraDistance;
    const float Height=bClose ? CameraHeight+24.f : CameraHeight;
    const FVector Target=Pawn->GetActorLocation()+Pawn->GetActorForwardVector()*Distance+Pawn->GetActorRightVector()*CameraSide+FVector::UpVector*Height;
    PreviewCamera->SetActorLocation(FMath::VInterpTo(PreviewCamera->GetActorLocation(),Target,Dt,5.f));
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
    // UE 5.8 também dispara OnValueChanged em SetValue: não reenviar a alteração de sincronização.
    if (SkinSlider)
    {
        SkinSlider->OnValueChanged.RemoveDynamic(this,&UHypeCharacterSelectWidget::HandleSkinChanged);
        SkinSlider->SetValue(Position);
        SkinSlider->OnValueChanged.AddDynamic(this,&UHypeCharacterSelectWidget::HandleSkinChanged);
    }
    if (HeightSlider)
    {
        HeightSlider->OnValueChanged.RemoveDynamic(this,&UHypeCharacterSelectWidget::HandleHeightChanged);
        HeightSlider->SetValue((DraftAppearance.HeightCm-160.f)/40.f);
        HeightSlider->OnValueChanged.AddDynamic(this,&UHypeCharacterSelectWidget::HandleHeightChanged);
    }
    if (SkinAmountLabel) { SkinAmountLabel->SetText(FText::FromString(FString::Printf(TEXT("%.0f%%"),Position*100.f))); }
    if (HeightLabel) { HeightLabel->SetText(FText::FromString(FString::Printf(TEXT("%.0f cm"),DraftAppearance.HeightCm))); }
    const int32 Selected=FMath::RoundToInt(Position*(Colors.Num()-1));
    for (int32 I=0;I<SkinFrames.Num();++I)
    {
        const float Opacity=I==Selected ? 0.8f : 0.f;
        SkinFrames[I]->SetRenderOpacity(Opacity);
        PageOpacities.Add(SkinFrames[I].Get(),Opacity);
        if (SkinChecks.IsValidIndex(I))
        {
            SkinChecks[I]->SetRenderOpacity(I==Selected ? 1.f : 0.f);
            PageOpacities.Add(SkinChecks[I].Get(),I==Selected ? 1.f : 0.f);
        }
    }
}
void UHypeCharacterSelectWidget::LighterSkin() { HandleSkinChanged((SkinSlider ? SkinSlider->GetValue() : 0.f)-0.05f); }
void UHypeCharacterSelectWidget::DarkerSkin() { HandleSkinChanged((SkinSlider ? SkinSlider->GetValue() : 0.f)+0.05f); }
void UHypeCharacterSelectWidget::ShorterHeight() { HandleHeightChanged(FMath::Clamp((DraftAppearance.HeightCm-161.f)/40.f,0.f,1.f)); }
void UHypeCharacterSelectWidget::TallerHeight() { HandleHeightChanged(FMath::Clamp((DraftAppearance.HeightCm-159.f)/40.f,0.f,1.f)); }

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
