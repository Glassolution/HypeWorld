// Menu de combate (Tab): réplica do menu de referência do Felipe.
// Esquerda: categorias que abrem e fecham (Naturezas, Kekkei Genkai, Clã, Taijutsu, Jutsus Especiais, Especializações).
//   Taijutsu = poses de batalha (estilos de luta): clicar escolhe; estilos com arma ficam trancados sem a arma.
// Direita: Favoritos (barra com "+", Edição, Modo Normal) e categorias com espaços para jutsus.
// Montado inteiro em C++ (como o HUD); refeito a cada clique para refletir o estado.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NinjaCombatMenuWidget.generated.h"

class UButton;
class UCanvasPanel;
class UTexture2D;
class AHypeWorldCharacter;

UCLASS()
class HYPEWORLD_API UNinjaCombatMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Pede para fechar (o controle do jogador esconde o menu e devolve o mouse ao jogo) */
	DECLARE_MULTICAST_DELEGATE(FOnCloseRequested);
	FOnCloseRequested OnCloseRequested;

	/** Refaz o menu (estilo escolhido, categorias abertas) */
	void Refresh();

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

private:

	void BuildLayout();

	/** Todos os botões chamam este: descobre qual foi pelo que está sob o mouse */
	UFUNCTION()
	void HandleClick();

	AHypeWorldCharacter* GetNinja() const;

	UPROPERTY() TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY() TObjectPtr<UButton> CloseButton;
	UPROPERTY() TArray<TObjectPtr<UButton>> CategoryButtons;
	UPROPERTY() TArray<TObjectPtr<UButton>> StyleButtons;
	UPROPERTY() TArray<TObjectPtr<UButton>> ToggleButtons;
	UPROPERTY() TObjectPtr<UTexture2D> StarTexture;
	UPROPERTY() TObjectPtr<UTexture2D> ChevronTexture;
	UPROPERTY() TObjectPtr<UTexture2D> LockTexture;

	/** Categoria aberta à esquerda (-1 = nenhuma); começa em Taijutsu */
	int32 OpenCategory = 3;
	bool bEditMode = false;
	bool bNormalMode = false;
	FText Feedback;
};
