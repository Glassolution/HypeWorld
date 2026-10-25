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
	const FLinearColor White(1.f, 1.f, 1.f, 1.f);
	const FLinearColor LabelOff(1.f, 1.f, 1.f, 0.55f);
	const FLinearColor BoxOn(0.02f, 0.02f, 0.02f, 0.95f);		// caixa escolhida: preta
	const FLinearColor BoxOff(0.10f, 0.09f, 0.08f, 0.55f);		// as outras: escura translúcida
	const FLinearColor BackFill(0.055f, 0.055f, 0.055f, 0.92f);	// Voltar
	const FLinearColor NextFill(0.42f, 0.37f, 0.30f, 0.80f);	// Próximo (bege acinzentado)

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
	Konoha.IconColor = FLinearColor(0.80f, 0.76f, 0.48f, 1.f);
	Konoha.Clans = {
		Clan(TEXT("Akimichi"), NSLOCTEXT("HypeSelect", "Akimichi", "Akimichi"), FLinearColor(0.75f, 0.16f, 0.12f, 1.f)),
		Clan(TEXT("Hyuga"), NSLOCTEXT("HypeSelect", "Hyuga", "Hyuga"), FLinearColor(0.85f, 0.33f, 0.15f, 1.f)),
		Clan(TEXT("Nara"), NSLOCTEXT("HypeSelect", "Nara", "Nara"), FLinearColor(0.45f, 0.60f, 0.38f, 1.f)),
		Clan(TEXT("Senju"), NSLOCTEXT("HypeSelect", "Senju", "Senju"), FLinearColor(0.35f, 0.55f, 0.75f, 1.f)),
		Clan(TEXT("Uchiha"), NSLOCTEXT("HypeSelect", "Uchiha", "Uchiha"), FLinearColor(0.82f, 0.12f, 0.12f, 1.f)),
	};
	FHypeVillageOption Suna;	// sem clãs ainda = trancada (como na referência)
	Suna.Id = TEXT("Suna");
	Suna.Name = NSLOCTEXT("HypeSelect", "Suna", "Suna");
	Suna.IconColor = FLinearColor(0.85f, 0.70f, 0.45f, 1.f);
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
	auto Option = [&](const FText& Name, const TSoftObjectPtr<UTexture2D>& IconAsset, const FLinearColor& IconColor,
		bool bSelected, bool bLocked, float X, float LabelY, float BoxY, const FVector2D& BoxSize, TArray<TObjectPtr<UButton>>& List)
	{
		const float Dim = bLocked ? 0.3f : (bSelected ? 1.f : 0.55f);
		Place(MakeText(Name, FontInfo(bSelected ? TEXT("Bold") : TEXT("SemiBold"), 9), bSelected ? White : LabelOff), TopLeft, FVector2D(X, LabelY), FVector2D::ZeroVector, Center);
		Place(MakeImage(TextureBrush(BoxTexture, bSelected ? BoxOn : BoxOff)), TopLeft, FVector2D(X, BoxY), BoxSize, Center);
		if (UTexture2D* IconTexture = IconAsset.LoadSynchronous())
		{
			const float IconSize = BoxSize.Y * 0.62f;
			Place(MakeImage(TextureBrush(IconTexture, IconColor), Dim), TopLeft, FVector2D(X, BoxY), FVector2D(IconSize, IconSize), Center);
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
		Option(V.Name, V.Icon, V.IconColor, i == SelectedVillage, V.Clans.Num() == 0, RowX(i, Villages.Num(), VillagePitch),
			VillageLabelY, VillageBoxY, VillageBoxSize, VillageButtons);
	}

	// --- Clãs da vila escolhida ---
	if (Village)
	{
		for (int32 i = 0; i < Village->Clans.Num(); ++i)
		{
			const FHypeClanOption& C = Village->Clans[i];
			Option(C.Name, C.Icon, C.IconColor, i == SelectedClan, false, RowX(i, Village->Clans.Num(), ClanPitch),
				ClanLabelY, ClanBoxY, ClanBoxSize, ClanButtons);
		}
	}

	// --- Voltar / Próximo ---
	const float Radius = 7.f;
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
	Place(MakeImage(Rounded(bLookFixed ? FLinearColor(1.f, 1.f, 1.f, 0.18f) : FLinearColor(0.05f, 0.05f, 0.05f, 0.6f), 6.f)), BottomRight, LookPos, LookButtonSize, BottomRight);
	const FLinearColor LookColor = bLookFixed ? White : FLinearColor(1.f, 1.f, 1.f, 0.8f);
	Place(MakeImage(TextureBrush(EyeTexture, LookColor)), BottomRight, LookPos + FVector2D(-LookButtonSize.X + 22.f, -LookButtonSize.Y * 0.5f), FVector2D(18.f, 18.f), Center);
	Place(MakeText(NSLOCTEXT("HypeSelect", "LookFixed", "Olhar fixo"), FontInfo(TEXT("SemiBold"), 9), LookColor), BottomRight,
		LookPos + FVector2D(-LookButtonSize.X * 0.5f - 12.f, -LookButtonSize.Y * 0.5f), FVector2D::ZeroVector, FVector2D(0.5f, 0.5f));
	LookButton = MakeButton(6.f);
	Place(LookButton, BottomRight, LookPos, LookButtonSize, BottomRight);
	Place(MakeText(NSLOCTEXT("HypeSelect", "Version", "hype world · alpha"), FontInfo(TEXT("SemiBold"), 8), FLinearColor(1.f, 1.f, 1.f, 0.35f)), BottomRight,
		FVector2D(-16.f, -8.f), FVector2D::ZeroVector, BottomRight);
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
