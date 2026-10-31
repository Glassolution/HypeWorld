// Tela de carregamento do Hype World (ver o .h). Layout em px de 1080p; a escala de DPI da Unreal ajusta para 1440p/4K.

#include "HypeLoadingScreenWidget.h"
#include "HypeLoadingSubsystem.h"
#include "HypeCharacterSelectWidget.h"
#include "Ninja/NinjaCustomization.h"
#include "Engine/DataTable.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Styling/CoreStyle.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogHypeLoadingScreen, Log, All);

namespace HypeLoading
{
	float EaseOut(float T) { T = FMath::Clamp(T, 0.f, 1.f); return 1.f - FMath::Pow(1.f - T, 3.f); }
	float EaseInOut(float T) { return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(T, 0.f, 1.f), 2.f); }
	const float PercentAlpha = 0.8f;
	const float StatusAlpha = 0.45f;
	const float TrackAlpha = 0.15f;
}

UHypeLoadingScreenWidget::UHypeLoadingScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MapaSeguinte = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Maps/Tutorial/L_Tutorial_Forest.L_Tutorial_Forest")));
	TelaAoEntrar = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/HypeWorld/UI/CharacterSelect/WBP_CharacterSelect.WBP_CharacterSelect_C")));
	AquecerPecas = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/HypeWorld/Characters/Parts/DT_CustomizationParts.DT_CustomizationParts")));
	Logo = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/HypeWorld/UI/Textures/T_Logo_HypeWorld.T_Logo_HypeWorld")));
	LogoMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/HypeWorld/UI/Loading/M_UI_LoadingLogo.M_UI_LoadingLogo")));
	BackgroundMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/HypeWorld/UI/Loading/M_UI_LoadingBackground.M_UI_LoadingBackground")));
	GlowMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/HypeWorld/UI/Loading/M_UI_LoadingGlow.M_UI_LoadingGlow")));
	Font = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/Ninja/UI/Fontes/F_Montserrat.F_Montserrat")));
	StatusPhrases = {
		NSLOCTEXT("HypeLoading", "S1", "Carregando o mundo..."),
		NSLOCTEXT("HypeLoading", "S2", "Afiando as kunais..."),
		NSLOCTEXT("HypeLoading", "S3", "Concentrando chakra..."),
		NSLOCTEXT("HypeLoading", "S4", "Preparando a vila..."),
	};
	SetIsFocusable(false);
}

TSharedRef<SWidget> UHypeLoadingScreenWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget && !IsDesignTime())
	{
		Build();
	}
	return Super::RebuildWidget();
}

void UHypeLoadingScreenWidget::Build()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;

	auto Place = [Canvas](UWidget* Widget, FVector2D Alignment, bool bAutoSize = false)
	{
		UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Widget);
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(Alignment);
		PanelSlot->SetAutoSize(bAutoSize);
		return PanelSlot;
	};
	auto MakeImage = [this](UMaterialInterface* Material, const FLinearColor& Tint)
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		if (Material)
		{
			Image->SetBrushFromMaterial(Material);
		}
		Image->SetColorAndOpacity(Tint);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	};

	FSlateFontInfo PercentFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), PercentFontSize);
	FSlateFontInfo StatusFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), StatusFontSize);
	if (UObject* FontObject = Font.LoadSynchronous())
	{
		PercentFont = FSlateFontInfo(FontObject, PercentFontSize, FontFace);
		StatusFont = FSlateFontInfo(FontObject, StatusFontSize, FontFace);
	}
	else
	{
		UE_LOG(LogHypeLoadingScreen, Warning, TEXT("Fonte não encontrada: usando a padrão da Unreal"));
	}
	PercentFont.LetterSpacing = 60;
	StatusFont.LetterSpacing = 80;
	auto MakeText = [this](const FSlateFontInfo& FontInfo, float Alpha)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetFont(FontInfo);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 1.f, 1.f, Alpha)));
		Text->SetJustification(ETextJustify::Center);
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
		Text->SetRenderOpacity(0.f);
		return Text;
	};

	// Fundo: tela inteira
	UMaterialInterface* BackgroundBase = BackgroundMaterial.LoadSynchronous();
	BackgroundMID = BackgroundBase ? UMaterialInstanceDynamic::Create(BackgroundBase, this) : nullptr;
	Background = MakeImage(nullptr, FLinearColor::Black);
	if (BackgroundMID)
	{
		Background->SetBrushFromMaterial(BackgroundMID);
		Background->SetColorAndOpacity(FLinearColor::White);
	}
	UCanvasPanelSlot* BackgroundSlot = Canvas->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackgroundSlot->SetOffsets(FMargin(0.f));

	// Logo
	LogoTexture = Logo.LoadSynchronous();
	UMaterialInterface* LogoBase = LogoMaterial.LoadSynchronous();
	LogoMID = LogoBase ? UMaterialInstanceDynamic::Create(LogoBase, this) : nullptr;
	if (!LogoTexture)
	{
		UE_LOG(LogHypeLoadingScreen, Warning, TEXT("Logo não encontrada (%s)"), *Logo.ToString());
	}
	if (LogoMID && LogoTexture)
	{
		LogoMID->SetTextureParameterValue(TEXT("Logo"), LogoTexture);
	}
	LogoImage = MakeImage(LogoMID, FLinearColor::White);
	if (!LogoMID && LogoTexture)
	{
		LogoImage->SetBrushFromTexture(LogoTexture);
	}
	LogoImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	LogoImage->SetRenderOpacity(0.f);
	LogoSlot = Place(LogoImage, FVector2D(0.5f, 0.5f));

	// Barra: trilho branco 15%, preenchimento branco e brilho redondo na ponta
	BarTrack = MakeImage(nullptr, FLinearColor(1.f, 1.f, 1.f, HypeLoading::TrackAlpha));
	BarTrackSlot = Place(BarTrack, FVector2D(0.5f, 0.5f));
	BarFill = MakeImage(nullptr, FLinearColor::White);
	BarFillSlot = Place(BarFill, FVector2D(0.f, 0.5f));
	BarTip = MakeImage(GlowMaterial.LoadSynchronous(), FLinearColor::White);
	BarTipSlot = Place(BarTip, FVector2D(0.5f, 0.5f));
	for (UImage* Image : { BarTrack.Get(), BarFill.Get(), BarTip.Get() })
	{
		Image->SetRenderOpacity(0.f);
	}

	PercentText = MakeText(PercentFont, HypeLoading::PercentAlpha);
	PercentText->SetText(FText::FromString(TEXT("0%")));
	PercentSlot = Place(PercentText, FVector2D(0.5f, 0.f), true);
	for (int32 i = 0; i < 2; ++i)
	{
		StatusTexts[i] = MakeText(StatusFont, HypeLoading::StatusAlpha);
		StatusSlots[i] = Place(StatusTexts[i], FVector2D(0.5f, 0.f), true);
	}
	StatusTexts[0]->SetText(StatusPhrases.Num() > 0 ? StatusPhrases[0] : FText::GetEmpty());
}

void UHypeLoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Phase = EPhase::Loading;
	Elapsed = PhaseTime = Shown = PhraseTime = 0.f;
	bShots = FParse::Param(FCommandLine::Get(), TEXT("LoadingShots"));

	UGameInstance* GameInstance = GetGameInstance();
	if (UHypeLoadingSubsystem* Loader = GameInstance ? GameInstance->GetSubsystem<UHypeLoadingSubsystem>() : nullptr)
	{
		Loader->StartPreload(MapaSeguinte);
		if (const UDataTable* Parts = AquecerPecas.LoadSynchronous())
		{
			TArray<FSoftObjectPath> Paths;
			Parts->ForeachRow<FNinjaCustomizationPart>(TEXT("Aquecer"), [&Paths](const FName&, const FNinjaCustomizationPart& Part)
			{
				Paths.Add(Part.Mesh.ToSoftObjectPath());
				Paths.Add(Part.Material.ToSoftObjectPath());
				Paths.Add(Part.EyeTexture.ToSoftObjectPath());
			});
			Loader->WarmUp(Paths);
		}
	}
	ShaderWait = 0.f;
}

void UHypeLoadingScreenWidget::UpdateLayout(const FVector2D& ScreenSize)
{
	// Proporção da logo (lida aqui: sem o jogo cozinhado a textura pode ainda estar sendo preparada no começo e ter 32x32)
	if (LogoTexture && LogoTexture->GetSizeX() > 64)
	{
		LogoAspect = float(LogoTexture->GetSizeY()) / float(LogoTexture->GetSizeX());
	}
	// 35% da largura; em ultrawide a logo é medida como se a tela fosse 16:9 (não fica gigante)
	const float LogoWidth = LogoWidthFraction * FMath::Min(ScreenSize.X, ScreenSize.Y * 16.f / 9.f);
	const float LogoHeight = LogoWidth * LogoAspect;
	const float LogoCenter = -LogoRaise;
	LogoSlot->SetPosition(FVector2D(0.f, LogoCenter));
	LogoSlot->SetSize(FVector2D(LogoWidth, LogoHeight));

	BarTop = LogoCenter + LogoHeight * 0.5f + BarGap;
	BarTrackSlot->SetPosition(FVector2D(0.f, BarTop));
	BarTrackSlot->SetSize(BarSize);
	BarFillSlot->SetPosition(FVector2D(-BarSize.X * 0.5f, BarTop));
	BarFillSlot->SetSize(FVector2D(BarSize.X * Shown, BarSize.Y));
	BarTipSlot->SetPosition(FVector2D(-BarSize.X * 0.5f + BarSize.X * Shown, BarTop));
	BarTipSlot->SetSize(FVector2D(26.f, 26.f));

	const float PercentTop = BarTop + BarSize.Y * 0.5f + PercentGap;
	PercentSlot->SetPosition(FVector2D(0.f, PercentTop));
	const float StatusTop = PercentTop + PercentFontSize * 1.4f + StatusGap;
	StatusSlots[0]->SetPosition(FVector2D(0.f, StatusTop));
	StatusSlots[1]->SetPosition(FVector2D(0.f, StatusTop));
}

void UHypeLoadingScreenWidget::SetStatus(const FText& Text)
{
	// Crossfade: o texto novo entra no bloco que estava apagado
	ActiveStatus = 1 - ActiveStatus;
	StatusTexts[ActiveStatus]->SetText(Text);
	StatusFade = 0.f;
}

void UHypeLoadingScreenWidget::TickStatus(float DeltaTime, float UiAlpha)
{
	StatusFade = FMath::Min(StatusFade + DeltaTime / FMath::Max(CrossfadeTime, 0.01f), 1.f);
	const float In = HypeLoading::EaseInOut(StatusFade);
	StatusTexts[ActiveStatus]->SetRenderOpacity(UiAlpha * In);
	StatusTexts[1 - ActiveStatus]->SetRenderOpacity(UiAlpha * (1.f - In));
}

void UHypeLoadingScreenWidget::RequestShot(const TCHAR* Name)
{
	if (bShots)
	{
		FScreenshotRequest::RequestScreenshot(FString(Name), true, false);
		UE_LOG(LogHypeLoadingScreen, Log, TEXT("Foto %s (t=%.2f s, barra %.0f%%)"), Name, Elapsed, Shown * 100.f);
	}
}

void UHypeLoadingScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	using namespace HypeLoading;

	if (Phase == EPhase::Done || !LogoSlot)
	{
		return;
	}
	const float Dt = FMath::Min(InDeltaTime, 0.1f);	// um engasgo não faz a animação pular
	Elapsed += Dt;
	PhaseTime += Dt;
	Frames++;
	FrameTimeSum += InDeltaTime;
	if (Elapsed > 0.5f)	// o primeiro quadro (abrindo o jogo) não conta
	{
		WorstFrame = FMath::Max(WorstFrame, InDeltaTime);
	}

	UGameInstance* GameInstance = GetGameInstance();
	UHypeLoadingSubsystem* Loader = GameInstance ? GameInstance->GetSubsystem<UHypeLoadingSubsystem>() : nullptr;

	// --- Entrada ---
	const float LogoT = (Elapsed - StartDelay) / FMath::Max(LogoEntryTime, 0.01f);
	float LogoAlpha = EaseOut(LogoT);
	float LogoScale = FMath::Lerp(0.92f, 1.f, EaseOut(LogoT));
	const float Reveal = EaseInOut(LogoT * 0.9f);
	const float UiStart = StartDelay + UiDelay;
	float UiAlpha = EaseOut((Elapsed - UiStart) / FMath::Max(UiFadeTime, 0.01f));
	float SceneAlpha = 1.f;	// fundo (papel e partículas)

	// --- Progresso: persegue o real sem saltos e leva pelo menos MinFillTime para encher ---
	const float Real = Loader ? Loader->GetProgress() : 1.f;
	const float Target = FMath::Min(Real, FMath::Max(Elapsed - UiStart, 0.f) / FMath::Max(MinFillTime, 0.01f));
	const float Step = FMath::Max((Target - Shown) * (1.f - FMath::Exp(-5.f * Dt)), 0.f);
	Shown = FMath::Min(Shown + FMath::Min(Step, 0.9f * Dt), 1.f);
	if (Target >= 1.f && 1.f - Shown < 0.004f)
	{
		Shown = 1.f;
	}

	switch (Phase)
	{
	case EPhase::Loading:
		PhraseTime += Dt;
		if (StatusPhrases.Num() > 1 && PhraseTime >= PhraseInterval)
		{
			PhraseTime = 0.f;
			PhraseIndex = (PhraseIndex + 1) % StatusPhrases.Num();
			SetStatus(StatusPhrases[PhraseIndex]);
		}
		if (Loader && Loader->HasLoadFailed())
		{
			if (CountdownShown != -2)
			{
				SetStatus(NSLOCTEXT("HypeLoading", "Fail", "Não foi possível carregar o mapa"));
				CountdownShown = -2;
			}
			PhraseTime = -1000.f;
		}
		else if (Shown >= 1.f && Loader && Loader->IsPreloaded() && Elapsed >= MinDisplayTime
			&& Loader->GetPendingCompiles() > 0 && ShaderWait < MaxShaderWait)
		{
			// Mapa pronto, shaders ainda compilando: espera aqui (sem "Preparing Shaders" e sem material cinza no jogo)
			ShaderWait += Dt;
			if (CountdownShown != -3)
			{
				SetStatus(NSLOCTEXT("HypeLoading", "Shaders", "Preparando os efeitos visuais..."));
				CountdownShown = -3;
			}
			PhraseTime = -1000.f;
		}
		else if (Shown >= 1.f && Loader && Loader->IsPreloaded() && Elapsed >= MinDisplayTime)
		{
			Phase = EPhase::Countdown;
			PhaseTime = 0.f;
			CountdownShown = -1;
		}
		break;

	case EPhase::Countdown:
	{
		const int32 Remaining = CountdownSeconds - FMath::FloorToInt(PhaseTime);
		if (Remaining <= 0)
		{
			Phase = EPhase::Exit;
			PhaseTime = 0.f;
		}
		else if (Remaining != CountdownShown)
		{
			CountdownShown = Remaining;
			SetStatus(FText::Format(NSLOCTEXT("HypeLoading", "Countdown", "Entrando no jogo em {0}..."), FText::AsNumber(Remaining)));
		}
		if (Remaining == 1 && PhaseTime >= CountdownSeconds - 1 + 0.4f && ShotsTaken == 2)
		{
			RequestShot(TEXT("Loading_3_Contagem"));
			ShotsTaken++;
		}
		break;
	}

	case EPhase::Exit:
	{
		// Logo e textos somem crescendo até 1,04; o fundo apaga junto e a tela fica preta
		const float E = EaseInOut(PhaseTime / FMath::Max(ExitTime, 0.01f));
		LogoAlpha *= 1.f - E;
		LogoScale *= FMath::Lerp(1.f, 1.04f, E);
		UiAlpha *= 1.f - E;
		SceneAlpha = 1.f - E;
		if (PhaseTime >= ExitTime + 0.1f)	// um instante de preto puro antes de trocar de mapa
		{
			Phase = EPhase::Done;
			UE_LOG(LogHypeLoadingScreen, Log, TEXT("Tela de carregamento: %d quadros em %.2f s = %.1f FPS médio; pior quadro %.1f ms"),
				Frames, FrameTimeSum, Frames / FMath::Max(FrameTimeSum, 0.001f), WorstFrame * 1000.f);
			if (Loader)
			{
				// Sem o WBP (script ainda não rodado) usa a classe C++, que já tem todos os valores
				TSubclassOf<UUserWidget> NextScreen = TelaAoEntrar.IsNull() ? nullptr : TelaAoEntrar.LoadSynchronous();
				if (!NextScreen && !TelaAoEntrar.IsNull())
				{
					NextScreen = UHypeCharacterSelectWidget::StaticClass();
				}
				Loader->TravelToPreloadedMap(FadeInTime, NextScreen);
			}
		}
		break;
	}

	default:
		break;
	}

	// --- Aplicar ---
	UpdateLayout(MyGeometry.GetLocalSize());

	const float Breath = 0.5f - 0.5f * FMath::Cos(2.f * PI * FMath::Max(Elapsed - StartDelay - LogoEntryTime, 0.f) / FMath::Max(GlowCycle, 0.1f));
	if (LogoMID)
	{
		LogoMID->SetScalarParameterValue(TEXT("Reveal"), Reveal);
		LogoMID->SetScalarParameterValue(TEXT("Glow"), GlowMax * Breath * LogoAlpha);
	}
	LogoImage->SetRenderOpacity(LogoAlpha);
	LogoImage->SetRenderScale(FVector2D(LogoScale));

	if (BackgroundMID)
	{
		const FVector2D Size = MyGeometry.GetLocalSize();
		BackgroundMID->SetScalarParameterValue(TEXT("Time"), Elapsed);
		BackgroundMID->SetScalarParameterValue(TEXT("Aspect"), Size.X / FMath::Max(Size.Y, 1.f));
		BackgroundMID->SetScalarParameterValue(TEXT("Fade"), SceneAlpha * EaseOut(Elapsed / 1.2f));
	}

	BarTrack->SetRenderOpacity(UiAlpha);
	BarFill->SetRenderOpacity(UiAlpha);
	BarTip->SetRenderOpacity(UiAlpha * FMath::Clamp(Shown * 20.f, 0.f, 1.f) * 0.7f);
	PercentText->SetRenderOpacity(UiAlpha);
	PercentText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::FloorToInt(Shown * 100.f + 0.001f))));
	TickStatus(Dt, UiAlpha);

	if (ShotsTaken == 0 && LogoT >= 0.55f)
	{
		RequestShot(TEXT("Loading_1_Entrada"));
		ShotsTaken++;
	}
	else if (ShotsTaken == 1 && Shown >= 0.5f)
	{
		RequestShot(TEXT("Loading_2_Meio"));
		ShotsTaken++;
	}
}
