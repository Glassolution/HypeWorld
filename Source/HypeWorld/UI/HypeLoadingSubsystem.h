// Carregamento do mapa seguinte em segundo plano (25/10/2026, tela de carregamento do Hype World).
// Fica no GameInstance, então sobrevive à troca de mapa: carrega o pacote do mapa de forma assíncrona (o jogo continua
// rodando e a tela de carregamento anima a 60 FPS), guarda o mundo carregado (senão a coleta de lixo da troca de mapa
// o descarta) e, depois do OpenLevel, cobre a tela de preto e clareia suave (fade in do preto para o jogo).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "HypeLoadingSubsystem.generated.h"

class SBorder;
class UUserWidget;

UCLASS()
class HYPEWORLD_API UHypeLoadingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Começa a carregar o mapa em segundo plano (não trava o jogo) */
	void StartPreload(const TSoftObjectPtr<UWorld>& Map);

	/** Progresso REAL do carregamento (0..1), lido do carregador assíncrono da Unreal */
	float GetProgress() const;
	bool IsPreloaded() const { return LoadedWorld != nullptr; }
	bool HasLoadFailed() const { return bLoadFailed; }
	/** Materiais/texturas/malhas ainda compilando (o "Preparing Shaders (N)" da Unreal) */
	int32 GetPendingCompiles() const;
	/** Carrega estes assets em segundo plano e os segura (os shaders deles compilam durante a tela de carregamento) */
	void WarmUp(const TArray<FSoftObjectPath>& Paths);

	/** Abre o mapa (já em memória) e faz o fade in do preto quando ele estiver pronto.
	 *  ScreenAfter: tela aberta assim que o mapa tiver o personagem (ex.: seleção do personagem); vazio = direto ao jogo */
	void TravelToPreloadedMap(float FadeInTime = 0.8f, TSubclassOf<UUserWidget> ScreenAfter = nullptr);

private:

	void OnPackageLoaded(const FName& PackageName, UPackage* Package, EAsyncLoadingResult::Type Result);
	void OnPostLoadMap(UWorld* World);
	bool TickFade(float DeltaTime);
	void RemoveFadeOverlay();
	/** Cria a ScreenAfter quando o jogador local já tem personagem (tenta a cada quadro, até 10 s) */
	bool TryShowScreenAfter(float DeltaTime);

	UPROPERTY() TSubclassOf<UUserWidget> PendingScreen;
	UPROPERTY() TArray<TObjectPtr<UObject>> WarmedAssets;
	FTSTicker::FDelegateHandle ScreenTicker;
	float ScreenWait = 0.f;

	/** Mundo carregado, guardado até o OpenLevel terminar */
	UPROPERTY() TObjectPtr<UWorld> LoadedWorld;

	FString MapPackageName;
	FString MapAssetName;
	/** Pacotes de que o mapa depende (registro de assets): progresso = quantos já estão em memória */
	TArray<FName> Dependencies;
	mutable double LastProgressCheck = -1.0;
	mutable float LastProgress = 0.f;
	bool bLoadFailed = false;
	bool bTraveling = false;

	TSharedPtr<SBorder> FadeOverlay;
	FTSTicker::FDelegateHandle FadeTicker;
	FDelegateHandle PostLoadMapHandle;
	float FadeTime = 0.f;
	float FadeDuration = 0.8f;
};
