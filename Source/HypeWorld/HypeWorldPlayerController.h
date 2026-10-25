// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HypeWorldPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UNinjaHUDWidget;
class UNinjaCombatMenuWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AHypeWorldPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** HUD do jogo (WBP_NinjaHUD) */
	UPROPERTY(EditAnywhere, Category="HUD")
	TSubclassOf<UNinjaHUDWidget> HUDWidgetClass;

	/** HUD criado para este jogador */
	UPROPERTY()
	TObjectPtr<UNinjaHUDWidget> HUDWidget;

	/** Menu de combate (Tab), criado na primeira vez que abre */
	UPROPERTY()
	TObjectPtr<UNinjaCombatMenuWidget> CombatMenu;

public:

	/** Abre/fecha o menu de combate: com ele aberto o mouse aparece e a câmera para de girar */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void ToggleCombatMenu();

	UFUNCTION(BlueprintPure, Category="HUD")
	bool IsCombatMenuOpen() const;

	/** Esc: solta o mouse (cursor aparece, câmera para) para clicar fora do jogo; Esc de novo volta a jogar.
	 *  Com o menu de combate aberto, Esc fecha o menu. */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void ToggleFreeMouse();

	/** Esconde/mostra o HUD (a tela de seleção do personagem esconde enquanto está aberta) */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetHUDHidden(bool bHideHUD);

protected:

	bool bMouseFree = false;


	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Câmera relativa à gravidade do personagem: andando na parede/teto o "para cima" da câmera acompanha */
	virtual void UpdateRotation(float DeltaTime) override;

	FVector LastFrameGravity = FVector::ZeroVector;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

};
