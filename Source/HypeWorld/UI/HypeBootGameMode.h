// Regras do mapa de boot (L_Boot, 25/10/2026): sem personagem, sem HUD, sem input e sem cursor; só mostra a tela de
// carregamento (WBP_LoadingScreen), que carrega e abre o mapa seguinte. O 3D é desligado (só a tela é desenhada).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HypeBootGameMode.generated.h"

class UHypeLoadingScreenWidget;

UCLASS()
class HYPEWORLD_API AHypeBootGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AHypeBootGameMode();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Carregamento") TSoftClassPtr<UHypeLoadingScreenWidget> LoadingScreenClass;

protected:

	virtual void BeginPlay() override;
};
