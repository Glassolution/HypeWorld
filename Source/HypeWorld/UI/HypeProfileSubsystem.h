// Escolhas do jogador feitas nas telas de menu (25/10/2026): por enquanto só vila e clã da tela de seleção
// (UHypeCharacterSelectWidget). Fica no GameInstance (sobrevive à troca de mapa). Ainda NÃO é salvo em disco nem
// replicado: isso entra com o criador de personagem/progressão (Fase 5).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HypeProfileSubsystem.generated.h"

UCLASS()
class HYPEWORLD_API UHypeProfileSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, Category="Perfil") FName Vila;
	UPROPERTY(BlueprintReadOnly, Category="Perfil") FName Cla;
};
