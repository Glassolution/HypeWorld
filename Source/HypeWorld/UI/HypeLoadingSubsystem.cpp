// Carregamento do mapa seguinte em segundo plano (ver o .h).

#include "HypeLoadingSubsystem.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetCompilingManager.h"
#include "UnrealClient.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/CoreStyle.h"

DEFINE_LOG_CATEGORY_STATIC(LogHypeLoading, Log, All);

void UHypeLoadingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UHypeLoadingSubsystem::OnPostLoadMap);
	// Jogo fora do editor: sem mensagens de debug na tela ("Preparing Shaders", "'DisableAllScreenMessages' to suppress").
	// Igual ao comando DisableAllScreenMessages; no editor (Play) continuam aparecendo.
	if (!GIsEditor)
	{
		GAreScreenMessagesEnabled = false;
	}
}

int32 UHypeLoadingSubsystem::GetPendingCompiles() const
{
	return FAssetCompilingManager::Get().GetNumRemainingAssets();
}

void UHypeLoadingSubsystem::WarmUp(const TArray<FSoftObjectPath>& Paths)
{
	TSet<FString> Packages;
	for (const FSoftObjectPath& Path : Paths)
	{
		if (Path.IsValid()) { Packages.Add(Path.GetLongPackageName()); }
	}
	for (const FString& Package : Packages)
	{
		LoadPackageAsync(Package, FLoadPackageAsyncDelegate::CreateWeakLambda(this, [this](const FName&, UPackage* Loaded, EAsyncLoadingResult::Type Result)
		{
			if (Result == EAsyncLoadingResult::Succeeded && Loaded)
			{
				WarmedAssets.Add(Loaded);
			}
		}));
	}
	UE_LOG(LogHypeLoading, Log, TEXT("Aquecendo %d pacotes (peças da criação de personagem)"), Packages.Num());
}

void UHypeLoadingSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	RemoveFadeOverlay();
	if (ScreenTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ScreenTicker);
		ScreenTicker.Reset();
	}
	Super::Deinitialize();
}

void UHypeLoadingSubsystem::StartPreload(const TSoftObjectPtr<UWorld>& Map)
{
	const FSoftObjectPath Path = Map.ToSoftObjectPath();
	MapPackageName = Path.GetLongPackageName();
	MapAssetName = Path.GetAssetName();
	LoadedWorld = nullptr;
	bLoadFailed = MapPackageName.IsEmpty();
	LastProgress = 0.f;
	LastProgressCheck = -1.0;
	Dependencies.Reset();
	if (bLoadFailed)
	{
		UE_LOG(LogHypeLoading, Error, TEXT("MapaSeguinte vazio: nada para carregar"));
		return;
	}

	// Todos os pacotes de que o mapa depende (de verdade, "hard"), para medir o progresso
	if (IAssetRegistry* Registry = IAssetRegistry::Get())
	{
		TSet<FName> Seen;
		TArray<FName> Pending = { FName(*MapPackageName) };
		const UE::AssetRegistry::FDependencyQuery Hard(UE::AssetRegistry::EDependencyQuery::Hard);
		while (Pending.Num() > 0)
		{
			const FName Current = Pending.Pop(EAllowShrinking::No);
			TArray<FName> Deps;
			Registry->GetDependencies(Current, Deps, UE::AssetRegistry::EDependencyCategory::Package, Hard);
			for (const FName& Dep : Deps)
			{
				if (!Seen.Contains(Dep) && !Dep.ToString().StartsWith(TEXT("/Script/")))
				{
					Seen.Add(Dep);
					Pending.Add(Dep);
				}
			}
		}
		Dependencies = Seen.Array();
	}
	UE_LOG(LogHypeLoading, Log, TEXT("Carregando %s em segundo plano (%d pacotes de dependência)"), *MapPackageName, Dependencies.Num());

	LoadPackageAsync(MapPackageName, FLoadPackageAsyncDelegate::CreateUObject(this, &UHypeLoadingSubsystem::OnPackageLoaded));
}

void UHypeLoadingSubsystem::OnPackageLoaded(const FName& PackageName, UPackage* Package, EAsyncLoadingResult::Type Result)
{
	LoadedWorld = (Result == EAsyncLoadingResult::Succeeded && Package) ? UWorld::FindWorldInPackage(Package) : nullptr;
	bLoadFailed = LoadedWorld == nullptr;
	if (bLoadFailed)
	{
		UE_LOG(LogHypeLoading, Error, TEXT("Falha ao carregar %s"), *PackageName.ToString());
	}
	else
	{
		UE_LOG(LogHypeLoading, Log, TEXT("%s carregado"), *PackageName.ToString());
	}
}

float UHypeLoadingSubsystem::GetProgress() const
{
	if (LoadedWorld || bLoadFailed)
	{
		return 1.f;
	}
	// Contar pacotes em memória custa um pouco: 10 vezes por segundo basta (a barra interpola no meio)
	const double Now = FPlatformTime::Seconds();
	if (Now - LastProgressCheck < 0.1)
	{
		return LastProgress;
	}
	LastProgressCheck = Now;

	float Progress;
	if (Dependencies.Num() > 0)
	{
		int32 Loaded = 0;
		for (const FName& Dep : Dependencies)
		{
			const UPackage* Package = FindObjectFast<UPackage>(nullptr, Dep);
			Loaded += (Package && Package->IsFullyLoaded()) ? 1 : 0;
		}
		Progress = 0.98f * float(Loaded) / float(Dependencies.Num());   // os 2% finais = o próprio mapa
	}
	else
	{
		// Sem registro de assets: usa o progresso do pacote do mapa
		Progress = FMath::Max(GetAsyncLoadPercentage(FName(*MapPackageName)), 0.f) / 100.f;
	}
	LastProgress = FMath::Max(LastProgress, Progress);
	return LastProgress;
}

void UHypeLoadingSubsystem::TravelToPreloadedMap(float FadeInTime, TSubclassOf<UUserWidget> ScreenAfter)
{
	if (bTraveling)
	{
		return;
	}
	bTraveling = true;
	PendingScreen = ScreenAfter;
	FadeDuration = FMath::Max(FadeInTime, 0.01f);
	if (UGameViewportClient* Viewport = GetGameInstance()->GetGameViewportClient())
	{
		Viewport->bDisableWorldRendering = false;   // o mapa de boot desliga o 3D (só mostra a tela)
	}
	UE_LOG(LogHypeLoading, Log, TEXT("Abrindo %s"), *MapPackageName);
	UGameplayStatics::OpenLevel(GetGameInstance(), FName(*MapPackageName));
}

void UHypeLoadingSubsystem::OnPostLoadMap(UWorld* World)
{
	if (!bTraveling || !World || World->GetOutermost()->GetName() != MapPackageName)
	{
		return;
	}
	bTraveling = false;
	LoadedWorld = nullptr;   // o mapa agora é o mundo do jogo; não precisa mais segurar

	// Preto por cima de tudo (inclusive do HUD do mapa) e clareia em FadeDuration
	UGameViewportClient* Viewport = GetGameInstance()->GetGameViewportClient();
	if (!Viewport)
	{
		return;
	}
	RemoveFadeOverlay();
	FadeOverlay = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor::Black)
		.Visibility(EVisibility::HitTestInvisible);
	Viewport->AddViewportWidgetContent(FadeOverlay.ToSharedRef(), 10000);
	FadeTime = -0.15f;   // segura o preto um instante: os primeiros quadros do mapa costumam engasgar
	FadeTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UHypeLoadingSubsystem::TickFade));

	// Tela seguinte (seleção do personagem): já embaixo do preto, para o primeiro quadro visível ser ela
	ScreenWait = 0.f;
	if (PendingScreen && TryShowScreenAfter(0.f))
	{
		ScreenTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UHypeLoadingSubsystem::TryShowScreenAfter));
	}
}

bool UHypeLoadingSubsystem::TryShowScreenAfter(float DeltaTime)
{
	ScreenWait += DeltaTime;
	UGameInstance* GameInstance = GetGameInstance();
	APlayerController* PC = GameInstance ? GameInstance->GetFirstLocalPlayerController() : nullptr;
	if (PendingScreen && PC && PC->GetPawn())
	{
		if (UUserWidget* Screen = CreateWidget<UUserWidget>(PC, PendingScreen))
		{
			Screen->AddToViewport(50);
			UE_LOG(LogHypeLoading, Log, TEXT("Tela seguinte aberta: %s"), *PendingScreen->GetName());
		}
	}
	else if (PendingScreen && ScreenWait < 10.f)
	{
		return true;	// o personagem ainda não nasceu: tenta de novo no próximo quadro
	}
	else if (PendingScreen)
	{
		UE_LOG(LogHypeLoading, Warning, TEXT("Tela seguinte não aberta: o jogador não ganhou personagem em 10 s"));
	}
	PendingScreen = nullptr;
	ScreenTicker.Reset();
	return false;
}

bool UHypeLoadingSubsystem::TickFade(float DeltaTime)
{
	FadeTime += FMath::Min(DeltaTime, 1.f / 30.f);   // um engasgo não pula o fade
	const float T = FMath::Clamp(FadeTime / FadeDuration, 0.f, 1.f);
	const float Alpha = 1.f - FMath::InterpEaseInOut(0.f, 1.f, T, 2.f);
	if (FadeOverlay.IsValid())
	{
		FadeOverlay->SetBorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, Alpha));
	}
	if (T >= 1.f)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("CreationShots")))
		{
			FScreenshotRequest::RequestScreenshot(TEXT("Criacao_5_Jogo"), true, false);
		}
		RemoveFadeOverlay();
		return false;
	}
	return true;
}

void UHypeLoadingSubsystem::RemoveFadeOverlay()
{
	if (FadeTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(FadeTicker);
		FadeTicker.Reset();
	}
	if (FadeOverlay.IsValid())
	{
		if (UGameViewportClient* Viewport = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(FadeOverlay.ToSharedRef());
		}
		FadeOverlay.Reset();
	}
}
