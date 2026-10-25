// Tela de seleção do personagem montada inteira em C++ (ver o .h). Layout em px de 1080p, medidos na imagem de
// referência do Felipe (1329x743 → ×1,45); a escala de DPI da Unreal ajusta para outras telas.

#include "HypeCharacterSelectWidget.h"
#include "HypeProfileSubsystem.h"
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

void UHypeCharacterSelectWidget::Refresh()
{
	if (Canvas)
	{
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
	VillageButtons.Reset();
	ClanButtons.Reset();

	// Escolha válida: vila com clã (as sem clã estão trancadas) e clã dentro da lista
	if (!Villages.IsValidIndex(SelectedVillage) || Villages[SelectedVillage].Clans.Num() == 0)
	{
		SelectedVillage = Villages.IndexOfByPredicate([](const FHypeVillageOption& V) { return V.Clans.Num() > 0; });
	}
	const FHypeVillageOption* Village = Villages.IsValidIndex(SelectedVillage) ? &Villages[SelectedVillage] : nullptr;
	SelectedClan = Village ? FMath::Clamp(SelectedClan, 0, Village->Clans.Num() - 1) : -1;

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
	if (LogoTexture)
	{
		const float Aspect = LogoTexture->GetSizeX() > 64 ? float(LogoTexture->GetSizeY()) / float(LogoTexture->GetSizeX()) : 0.55f;
		Place(MakeImage(TextureBrush(LogoTexture, White)), TopLeft, FVector2D(ColumnCenter, LogoCenterY), FVector2D(LogoWidth, LogoWidth * Aspect), Center);
	}
	Place(MakeText(Subtitle, FontInfo(TEXT("SemiBold"), 12), FLinearColor(1.f, 1.f, 1.f, 0.92f)), TopLeft, FVector2D(ColumnCenter, SubtitleY), FVector2D::ZeroVector, Center);

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
			Option(C.Id, C.Name, C.Icon, C.IconColor, i == SelectedClan, false, RowX(i, Village->Clans.Num(), ClanPitch),
				ClanLabelY, ClanBoxY, ClanBoxSize, ClanButtons);
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
	Place(MakeText(NSLOCTEXT("HypeSelect", "Next", "Próximo"), ButtonFont, White), TopLeft, FVector2D(NextX, ButtonsY), FVector2D::ZeroVector, Center);
	BackButton = MakeButton(Radius);
	NextButton = MakeButton(Radius);
	Place(BackButton, TopLeft, FVector2D(BackX, ButtonsY), ButtonSize, Center);
	Place(NextButton, TopLeft, FVector2D(NextX, ButtonsY), ButtonSize, Center);

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
	SetupPreviewCamera();

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
		if (ClanButtons[i] && ClanButtons[i]->IsHovered())
		{
			SelectedClan = i;
			Refresh();
			return;
		}
	}
	if (NextButton && NextButton->IsHovered())
	{
		Confirm();
	}
	else if (BackButton && BackButton->IsHovered())
	{
		UE_LOG(LogHypeCharacterSelect, Log, TEXT("Voltar: esta é a primeira etapa, nada para voltar ainda"));
	}
	else if (LookButton && LookButton->IsHovered())
	{
		bLookFixed = !bLookFixed;
		Refresh();
	}
}

void UHypeCharacterSelectWidget::Confirm()
{
	const FHypeVillageOption* Village = Villages.IsValidIndex(SelectedVillage) ? &Villages[SelectedVillage] : nullptr;
	const FHypeClanOption* Clan = (Village && Village->Clans.IsValidIndex(SelectedClan)) ? &Village->Clans[SelectedClan] : nullptr;
	UGameInstance* GameInstance = GetGameInstance();
	if (UHypeProfileSubsystem* Profile = GameInstance ? GameInstance->GetSubsystem<UHypeProfileSubsystem>() : nullptr)
	{
		Profile->Vila = Village ? Village->Id : NAME_None;
		Profile->Cla = Clan ? Clan->Id : NAME_None;
	}
	UE_LOG(LogHypeCharacterSelect, Log, TEXT("Escolha: vila %s, clã %s"), Village ? *Village->Id.ToString() : TEXT("-"), Clan ? *Clan->Id.ToString() : TEXT("-"));
	ExitTime = 0.f;	// o menu some (NativeTick) e depois Finish devolve o jogo
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

	// Contorno tracejado com o mouse em cima de Voltar / Próximo
	if (BackDashes && BackButton)
	{
		BackDashes->SetRenderOpacity(BackButton->IsHovered() ? 1.f : 0.f);
	}
	if (NextDashes && NextButton)
	{
		NextDashes->SetRenderOpacity(NextButton->IsHovered() ? 1.f : 0.f);
	}
}
