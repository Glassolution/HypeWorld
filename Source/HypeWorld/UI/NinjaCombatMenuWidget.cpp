// Menu de combate montado inteiro em C++ (ver o .h). Layout pensado em 1920x1080, como o HUD.

#include "NinjaCombatMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "HypeWorldCharacter.h"
#include "NinjaFightingStyle.h"

namespace NinjaMenu
{
	const FLinearColor White(1.f, 1.f, 1.f, 1.f);
	const FLinearColor Soft(0.78f, 0.78f, 0.80f, 1.f);
	const FLinearColor Dim(0.45f, 0.45f, 0.48f, 1.f);
	const FLinearColor Panel(0.022f, 0.022f, 0.027f, 0.93f);
	const FLinearColor Line(1.f, 1.f, 1.f, 0.075f);
	const FLinearColor Accent(0.95f, 0.45f, 0.18f, 1.f);

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

	FSlateFontInfo Font(const TCHAR* Style, float Size)
	{
		return FCoreStyle::GetDefaultFontStyle(Style, Size);
	}

	/** Textura branca gerada por código; Alpha diz a opacidade de cada ponto (0..1), com 4 amostras por pixel. */
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

	float SegmentDistance(const FVector2f& P, const FVector2f& A, const FVector2f& B)
	{
		const FVector2f AB = B - A;
		const float T = FMath::Clamp(FVector2f::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), 1e-6f), 0.f, 1.f);
		return FVector2f::Distance(P, A + AB * T);
	}
}

TSharedRef<SWidget> UNinjaCombatMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget && !IsDesignTime())
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

AHypeWorldCharacter* UNinjaCombatMenuWidget::GetNinja() const
{
	return Cast<AHypeWorldCharacter>(GetOwningPlayerPawn());
}

void UNinjaCombatMenuWidget::Refresh()
{
	if (Canvas)
	{
		Canvas->ClearChildren();
		BuildLayout();
	}
}

void UNinjaCombatMenuWidget::BuildLayout()
{
	using namespace NinjaMenu;

	if (!Canvas)
	{
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Canvas;
	}
	CategoryButtons.Reset();
	StyleButtons.Reset();
	ToggleButtons.Reset();
	PassiveButtons.Reset();
	PassiveButtonIds.Reset();

	// Ícones desenhados por código: estrela de 4 pontas (título), seta para baixo (categorias), cadeado (estilo com arma)
	if (!StarTexture)
	{
		StarTexture = MakeTexture(96, [](float U, float V)
		{
			const FVector2f P(U - 0.5f, V - 0.5f);
			const float R = P.Size() * 2.f, Ang = FMath::Atan2(P.Y, P.X);
			const float Edge = 0.30f + 0.70f * FMath::Pow(FMath::Abs(FMath::Cos(2.f * Ang)), 6.f);   // pontas em 0/90/180/270
			return (R < Edge * 0.96f && R > 0.17f) ? 1.f : 0.f;
		});
		ChevronTexture = MakeTexture(48, [](float U, float V)
		{
			const FVector2f P(U, V);
			const float D = FMath::Min(SegmentDistance(P, FVector2f(0.22f, 0.38f), FVector2f(0.5f, 0.64f)), SegmentDistance(P, FVector2f(0.5f, 0.64f), FVector2f(0.78f, 0.38f)));
			return D < 0.055f ? 1.f : 0.f;
		});
		LockTexture = MakeTexture(48, [](float U, float V)
		{
			const bool Body = U > 0.2f && U < 0.8f && V > 0.45f && V < 0.9f;
			const float R = FVector2f(U - 0.5f, V - 0.45f).Size();
			const bool Shackle = V < 0.45f && R > 0.15f && R < 0.24f;
			return (Body || Shackle) ? 1.f : 0.f;
		});
	}

	const FVector2D TopLeft(0.f, 0.f), TopRight(1.f, 0.f), Center(0.5f, 0.5f);
	auto Place = [this](UWidget* Widget, FVector2D Anchor, FVector2D Position, FVector2D Size, FVector2D Alignment = FVector2D::ZeroVector)
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
	auto Icon = [&MakeImage](UTexture2D* Texture, const FLinearColor& Tint)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.TintColor = FSlateColor(Tint);
		return MakeImage(Brush);
	};
	auto MakeText = [this](const FString& Words, float Size, const FLinearColor& Color = White, const TCHAR* Style = TEXT("Regular"))
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Words));
		Text->SetFont(Font(Style, Size));
		Text->SetColorAndOpacity(FSlateColor(Color));
		return Text;
	};
	auto MakeButton = [this](TArray<TObjectPtr<UButton>>* List)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle Style;
		Style.SetNormal(Rounded(FLinearColor::Transparent, 8.f));
		Style.SetHovered(Rounded(FLinearColor(1.f, 1.f, 1.f, 0.05f), 8.f));
		Style.SetPressed(Rounded(FLinearColor(1.f, 1.f, 1.f, 0.10f), 8.f));
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f));
		Button->SetStyle(Style);
		Button->OnClicked.AddDynamic(this, &UNinjaCombatMenuWidget::HandleClick);
		if (List)
		{
			List->Add(Button);
		}
		return Button;
	};

	AHypeWorldCharacter* Ninja = GetNinja();

	// =================================================== Esquerda: menu de combate ===================================================
	const FVector2D L(125.f, 78.f);
	const float LW = 470.f, LH = 910.f;
	Place(MakeImage(Rounded(Panel, 14.f)), TopLeft, L, FVector2D(LW, LH));

	Place(Icon(StarTexture, White), TopLeft, L + FVector2D(88.f, 58.f), FVector2D(38.f, 38.f), Center);
	UHorizontalBox* Title = WidgetTree->ConstructWidget<UHorizontalBox>();
	Title->AddChildToHorizontalBox(MakeText(TEXT("MENU DE "), 22.f, White, TEXT("Regular")));
	Title->AddChildToHorizontalBox(MakeText(TEXT("COMBATE"), 22.f, White, TEXT("Bold")));
	Place(Title, TopLeft, L + FVector2D(122.f, 58.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));

	Place(MakeText(TEXT("×"), 30.f, Soft), TopLeft, L + FVector2D(LW - 38.f, 36.f), FVector2D::ZeroVector, Center);
	CloseButton = MakeButton(nullptr);
	Place(CloseButton, TopLeft, L + FVector2D(LW - 38.f, 36.f), FVector2D(40.f, 40.f), Center);

	static const TCHAR* Categories[] = { TEXT("Naturezas"), TEXT("Kekkei Genkai"), TEXT("Clã"), TEXT("Taijutsu"), TEXT("Passivas"), TEXT("Jutsus Especiais"), TEXT("Especializações") };
	const float RowH = 58.f, StyleH = 46.f;   // compacto: Taijutsu aberto com 7 estilos cabe no painel
	float Y = L.Y + 104.f;
	for (int32 i = 0; i < UE_ARRAY_COUNT(Categories); ++i)
	{
		const bool bOpen = OpenCategory == i;
		Place(MakeText(Categories[i], 18.f, White), TopLeft, FVector2D(L.X + 50.f, Y + RowH * 0.5f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
		UImage* Chevron = Icon(ChevronTexture, Soft);
		Chevron->SetRenderTransformAngle(bOpen ? 180.f : 0.f);
		Place(Chevron, TopLeft, FVector2D(L.X + LW - 58.f, Y + RowH * 0.5f), FVector2D(20.f, 20.f), Center);
		Place(MakeButton(&CategoryButtons), TopLeft, FVector2D(L.X + 36.f, Y + 6.f), FVector2D(LW - 72.f, RowH - 12.f));
		Y += RowH;

		if (bOpen)
		{
			if (i == 3)
			{
				// Taijutsu: poses de batalha (estilos de luta)
				const TArray<TObjectPtr<UNinjaFightingStyle>> Empty;
				const TArray<TObjectPtr<UNinjaFightingStyle>>& Styles = Ninja ? Ninja->FightingStyles : Empty;
				Place(MakeText(TEXT("POSES DE BATALHA"), 12.f, Dim, TEXT("Bold")), TopLeft, FVector2D(L.X + 58.f, Y + 2.f), FVector2D::ZeroVector);
				Y += 22.f;
				for (int32 s = 0; s < Styles.Num(); ++s)
				{
					const UNinjaFightingStyle* Style = Styles[s];
					if (!Style)
					{
						continue;
					}
					const bool bSelected = Ninja && Ninja->GetFightingStyleIndex() == s;
					const bool bUsable = Ninja && Ninja->CanUseFightingStyle(Style);
					if (bSelected)
					{
						Place(MakeImage(Rounded(FLinearColor(1.f, 1.f, 1.f, 0.07f), 8.f)), TopLeft, FVector2D(L.X + 44.f, Y + 3.f), FVector2D(LW - 88.f, StyleH - 6.f));
						Place(MakeImage(Rounded(Accent, 2.f)), TopLeft, FVector2D(L.X + 44.f, Y + 12.f), FVector2D(3.f, StyleH - 24.f));
					}
					const FLinearColor IconTint = bUsable ? Style->IconColor : Style->IconColor * FLinearColor(0.35f, 0.35f, 0.35f, 1.f);
					Place(MakeImage(Rounded(IconTint, 0.f, FLinearColor(1.f, 1.f, 1.f, 0.35f), 1.5f, true)), TopLeft, FVector2D(L.X + 78.f, Y + StyleH * 0.5f), FVector2D(30.f, 30.f), Center);
					const FString Initial = Style->DisplayName.ToString().Left(1).ToUpper();
					Place(MakeText(Initial, 15.f, White, TEXT("Bold")), TopLeft, FVector2D(L.X + 78.f, Y + StyleH * 0.5f), FVector2D::ZeroVector, Center);
					Place(MakeText(Style->DisplayName.ToString(), 15.f, bUsable ? White : Dim, TEXT("Bold")), TopLeft, FVector2D(L.X + 106.f, Y + 15.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
					FString Sub = Style->Description.ToString();
					if (!bUsable && Style->RequiredWeapon != ENinjaWeapon::None)
					{
						Sub = FString::Printf(TEXT("Requer arma: %s"), *UEnum::GetDisplayValueAsText(Style->RequiredWeapon).ToString());
						Place(Icon(LockTexture, Dim), TopLeft, FVector2D(L.X + LW - 70.f, Y + StyleH * 0.5f), FVector2D(18.f, 18.f), Center);
					}
					else if (bSelected)
					{
						Place(MakeText(TEXT("EM USO"), 11.f, Accent, TEXT("Bold")), TopLeft, FVector2D(L.X + LW - 60.f, Y + StyleH * 0.5f), FVector2D::ZeroVector, FVector2D(1.f, 0.5f));
					}
					Place(MakeText(Sub, 11.f, Dim, TEXT("Italic")), TopLeft, FVector2D(L.X + 106.f, Y + 32.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
					UButton* StyleButton = MakeButton(&StyleButtons);
					StyleButton->SetToolTipText(Style->DisplayName);
					Place(StyleButton, TopLeft, FVector2D(L.X + 44.f, Y + 3.f), FVector2D(LW - 88.f, StyleH - 6.f));
					Y += StyleH;
				}
				Y += 8.f;
			}
			else if (i == 4)
			{
				// Passivas aprendidas (infusão de chakra, do clã...): interruptor liga/desliga
				int32 Shown = 0;
				for (const FNinjaPassiveInfo& Passive : Ninja ? Ninja->Passives : TArray<FNinjaPassiveInfo>())
				{
					if (!Passive.bLearned)
					{
						continue;
					}
					const bool bOn = Ninja->IsPassiveEnabled(Passive.Id);
					Place(MakeText(Passive.Name.ToString(), 15.f, White, TEXT("Bold")), TopLeft, FVector2D(L.X + 62.f, Y + 15.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
					Place(MakeText(FString::Printf(TEXT("%s • %s"), *Passive.Origin.ToString(), *Passive.Description.ToString()), 11.f, Dim, TEXT("Italic")),
						TopLeft, FVector2D(L.X + 62.f, Y + 33.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
					const FVector2D Switch(L.X + LW - 86.f, Y + 23.f);
					Place(MakeImage(Rounded(bOn ? FLinearColor(0.25f, 0.6f, 0.95f, 1.f) : FLinearColor(0.16f, 0.16f, 0.18f, 1.f), 0.f, FLinearColor(1.f, 1.f, 1.f, 0.15f), 1.f, true)),
						TopLeft, Switch, FVector2D(44.f, 22.f), FVector2D(0.f, 0.5f));
					Place(MakeImage(Rounded(White, 0.f, FLinearColor::Transparent, 0.f, true)), TopLeft, Switch + FVector2D(bOn ? 33.f : 11.f, 0.f), FVector2D(16.f, 16.f), Center);
					UButton* Toggle = MakeButton(&PassiveButtons);
					PassiveButtonIds.Add(uint8(Passive.Id));
					Place(Toggle, TopLeft, FVector2D(L.X + 44.f, Y + 3.f), FVector2D(LW - 88.f, StyleH - 6.f));
					Y += StyleH;
					++Shown;
				}
				if (Shown == 0)
				{
					Place(MakeText(TEXT("Nenhuma passiva aprendida ainda"), 14.f, Dim, TEXT("Italic")), TopLeft, FVector2D(L.X + 58.f, Y + 14.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
					Y += 36.f;
				}
				Y += 6.f;
			}
			else
			{
				Place(MakeText(TEXT("Nenhum jutsu aprendido ainda"), 14.f, Dim, TEXT("Italic")), TopLeft, FVector2D(L.X + 58.f, Y + 14.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
				Y += 36.f;
			}
		}
		Place(MakeImage(Rounded(Line, 0.f)), TopLeft, FVector2D(L.X + 48.f, Y), FVector2D(LW - 96.f, 1.f));
	}
	if (!Feedback.IsEmpty())
	{
		Place(MakeText(Feedback.ToString(), 14.f, Accent, TEXT("Bold")), TopLeft, FVector2D(L.X + LW * 0.5f, L.Y + LH - 34.f), FVector2D::ZeroVector, Center);
	}

	// =================================================== Direita: favoritos ===================================================
	const FVector2D TopBar(-575.f, 48.f);
	const float RW = 480.f;
	Place(MakeImage(Rounded(Panel, 12.f)), TopRight, TopBar, FVector2D(RW, 82.f));
	Place(MakeText(TEXT("Favoritos"), 21.f, White, TEXT("Bold")), TopRight, TopBar + FVector2D(52.f, 41.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
	Place(MakeText(TEXT("+"), 18.f, Soft), TopRight, TopBar + FVector2D(160.f, 41.f), FVector2D::ZeroVector, Center);
	const TCHAR* Toggles[] = { TEXT("Edição"), TEXT("Modo Normal") };
	const bool ToggleOn[] = { bEditMode, bNormalMode };
	float TX = 228.f;
	for (int32 t = 0; t < 2; ++t)
	{
		Place(MakeImage(Rounded(ToggleOn[t] ? Accent : FLinearColor(0.12f, 0.12f, 0.13f, 1.f), 5.f, FLinearColor(1.f, 1.f, 1.f, 0.12f), 1.f)), TopRight, TopBar + FVector2D(TX, 41.f), FVector2D(20.f, 20.f), FVector2D(0.f, 0.5f));
		Place(MakeText(Toggles[t], 13.f, White), TopRight, TopBar + FVector2D(TX + 28.f, 41.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
		Place(MakeButton(&ToggleButtons), TopRight, TopBar + FVector2D(TX - 4.f, 25.f), FVector2D(t == 0 ? 86.f : 124.f, 32.f));
		TX += t == 0 ? 100.f : 0.f;
	}

	const FVector2D Main(-575.f, 140.f);
	Place(MakeImage(Rounded(Panel, 12.f)), TopRight, Main, FVector2D(RW, 640.f));
	float RY = 52.f;
	auto Header = [&](const TCHAR* Name)
	{
		Place(MakeText(Name, 15.f, White, TEXT("Bold")), TopRight, Main + FVector2D(46.f, RY), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
		RY += 30.f;
	};
	auto Hint = [&]()
	{
		Place(MakeText(TEXT("Arraste um jutsu aqui"), 12.f, Dim, TEXT("Italic")), TopRight, Main + FVector2D(50.f, RY), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
		RY += 66.f;
	};
	auto Slots = [&](int32 Count)
	{
		for (int32 k = 0; k < Count; ++k)
		{
			Place(MakeImage(Rounded(FLinearColor(0.10f, 0.10f, 0.11f, 1.f), 0.f, FLinearColor(1.f, 1.f, 1.f, 0.10f), 1.5f, true)), TopRight, Main + FVector2D(70.f + k * 66.f, RY + 12.f), FVector2D(46.f, 46.f), Center);
		}
		RY += 72.f;
	};

	// Pose de batalha em uso (vem do Taijutsu)
	Header(TEXT("POSE DE BATALHA"));
	if (const UNinjaFightingStyle* Current = Ninja ? Ninja->GetFightingStyle() : nullptr)
	{
		Place(MakeImage(Rounded(Current->IconColor, 0.f, FLinearColor(1.f, 1.f, 1.f, 0.45f), 2.f, true)), TopRight, Main + FVector2D(70.f, RY + 12.f), FVector2D(46.f, 46.f), Center);
		Place(MakeText(Current->DisplayName.ToString().Left(1).ToUpper(), 18.f, White, TEXT("Bold")), TopRight, Main + FVector2D(70.f, RY + 12.f), FVector2D::ZeroVector, Center);
		Place(MakeText(Current->DisplayName.ToString(), 15.f, White, TEXT("Bold")), TopRight, Main + FVector2D(104.f, RY + 4.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
		Place(MakeText(FString::Printf(TEXT("%d combos • %d finalizadores"), Current->Combos.Num(), Current->Finishers.Num()), 12.f, Dim, TEXT("Italic")), TopRight, Main + FVector2D(104.f, RY + 24.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
	}
	RY += 72.f;
	Header(TEXT("NOVA CATEGORIA"));
	Hint();
	Header(TEXT("NOVA CATEGORIA"));
	Hint();
	Header(TEXT("KG"));
	Slots(3);
	Header(TEXT("KATON"));
	Slots(4);
	Header(TEXT("NOVA CATEGORIA"));
}

void UNinjaCombatMenuWidget::HandleClick()
{
	AHypeWorldCharacter* Ninja = GetNinja();
	if (CloseButton && CloseButton->IsHovered())
	{
		OnCloseRequested.Broadcast();
		return;
	}
	for (int32 i = 0; i < CategoryButtons.Num(); ++i)
	{
		if (CategoryButtons[i] && CategoryButtons[i]->IsHovered())
		{
			OpenCategory = OpenCategory == i ? INDEX_NONE : i;
			Feedback = FText::GetEmpty();
			Refresh();
			return;
		}
	}
	for (int32 s = 0; s < StyleButtons.Num(); ++s)
	{
		if (StyleButtons[s] && StyleButtons[s]->IsHovered() && Ninja)
		{
			// Os botões seguem a ordem da lista (estilos vazios são pulados ao montar)
			int32 Index = INDEX_NONE;
			for (int32 k = 0, Seen = 0; k < Ninja->FightingStyles.Num(); ++k)
			{
				if (Ninja->FightingStyles[k] && Seen++ == s)
				{
					Index = k;
					break;
				}
			}
			const UNinjaFightingStyle* Style = Ninja->FightingStyles.IsValidIndex(Index) ? Ninja->FightingStyles[Index].Get() : nullptr;
			if (Style && Ninja->SelectFightingStyle(Index))
			{
				Feedback = FText::FromString(FString::Printf(TEXT("Pose de batalha: %s"), *Style->DisplayName.ToString()));
			}
			else if (Style && Style->RequiredWeapon != ENinjaWeapon::None)
			{
				Feedback = FText::FromString(FString::Printf(TEXT("Segure uma %s para usar este estilo"), *UEnum::GetDisplayValueAsText(Style->RequiredWeapon).ToString()));
			}
			Refresh();
			return;
		}
	}
	for (int32 p = 0; p < PassiveButtons.Num(); ++p)
	{
		if (PassiveButtons[p] && PassiveButtons[p]->IsHovered() && Ninja && PassiveButtonIds.IsValidIndex(p))
		{
			Ninja->TogglePassive(ENinjaPassive(PassiveButtonIds[p]));
			Refresh();
			return;
		}
	}
	for (int32 t = 0; t < ToggleButtons.Num(); ++t)
	{
		if (ToggleButtons[t] && ToggleButtons[t]->IsHovered())
		{
			(t == 0 ? bEditMode : bNormalMode) = !(t == 0 ? bEditMode : bNormalMode);
			Refresh();
			return;
		}
	}
}
