// Tela de seleção do personagem (25/10/2026, referência do Felipe: menu à esquerda por cima do mapa, personagem no meio).
// Vem depois da tela de carregamento (UHypeLoadingSubsystem a cria quando o mapa abre). Esquerda: logo, frase,
// vilas (rótulo em cima + caixa com pincelada e ícone), clãs da vila escolhida, botões Voltar / Próximo.
// Embaixo à direita: botão "Olhar fixo". Atrás: o mapa atual, com uma câmera parada de frente para o personagem.
// Montada inteira em C++ como o HUD e a tela de carregamento; o WBP_CharacterSelect só herda desta classe para os
// ajustes ficarem em Class Defaults (seção Selecao). Layout em px de 1080p, medidos na imagem de referência.
// Próximo: guarda vila/clã no UHypeProfileSubsystem, a câmera volta para as costas do personagem e o jogo começa.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Ninja/HypeAppearanceTypes.h"
#include "HypeCreatorWidgets.h"
#include "HypeCharacterSelectWidget.generated.h"

class UButton;
class UCanvasPanel;
class UImage;
class UTexture2D;
class ACameraActor;
class UEditableTextBox;
class UTextBlock;
class UHypeOptionCardWidget;
class UHypeColorRowWidget;
class UHypeGradientSliderWidget;
class USkeletalMeshComponent;

USTRUCT(BlueprintType)
struct FHypeClanOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao") FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao") FText Name;
	/** Símbolo (PNG branco ou colorido com transparência). Vazio = letra inicial na cor IconColor */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao") TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao") FLinearColor IconColor = FLinearColor::White;
};

USTRUCT(BlueprintType)
struct FHypeVillageOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao") FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao") FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao") TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao") FLinearColor IconColor = FLinearColor::White;
	/** Clãs desta vila. Vila sem clã fica trancada (aparece apagada e não dá para escolher) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao") TArray<FHypeClanOption> Clans;
};

UCLASS()
class HYPEWORLD_API UHypeCharacterSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UHypeCharacterSelectWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao") TArray<FHypeVillageOption> Villages;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao") int32 SelectedVillage = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao") int32 SelectedClan = 4;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao") FText Subtitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Visual") TSoftObjectPtr<UTexture2D> Logo;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Visual", meta=(AllowedClasses="/Script/Engine.Font")) TSoftObjectPtr<UObject> Font;
	/** Escurecido da esquerda (atrás do menu) e o resto da tela */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Visual") float LeftShade = 0.62f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Visual") float BaseShade = 0.10f;
	/** Fundo desfocado como na referência: abertura da lente (menor = mais desfocado) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Camera") float BackgroundBlurFstop = 1.2f;

	/** Centro da coluna do menu (x) e largura da logo, px de 1080p */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") float ColumnCenter = 384.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") float LogoWidth = 190.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") float LogoCenterY = 254.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") float SubtitleY = 388.f;
	/** Vilas: y do rótulo, y do centro da caixa, tamanho da caixa e distância entre centros */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") float VillageLabelY = 471.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") float VillageBoxY = 522.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") FVector2D VillageBoxSize = FVector2D(75.f, 64.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") float VillagePitch = 98.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") float ClanLabelY = 606.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") float ClanBoxY = 660.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") FVector2D ClanBoxSize = FVector2D(78.f, 67.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") float ClanPitch = 89.f;
	/** Botões Voltar / Próximo: y do centro, tamanho e espaço entre eles */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") float ButtonsY = 831.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") FVector2D ButtonSize = FVector2D(172.f, 49.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") float ButtonGap = 13.f;
	/** Botão "Olhar fixo" (canto de baixo à direita): tamanho e margem da direita/de baixo */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") FVector2D LookButtonSize = FVector2D(130.f, 36.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Layout") FVector2D LookButtonMargin = FVector2D(31.f, 30.f);

	/** Câmera de frente para o personagem: distância à frente, para o lado (+ = direita do personagem) e altura
	 *  (relativas ao centro da cápsula), inclinação e campo de visão */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Camera") float CameraDistance = 440.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Camera") float CameraSide = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Camera") float CameraHeight = 44.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Camera") float CameraPitch = -1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Camera") float CameraFOV = 50.f;
	/** Volta da câmera para as costas do personagem ao apertar Próximo (s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Camera") float CameraBlendTime = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Tempo") float FadeInDelay = 0.2f;
	/** Entrada do painel: desliza da esquerda (PanelSlide px) com fade */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Tempo") float FadeInTime = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Tempo") float PanelSlide = 48.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Tempo") float FadeOutTime = 0.35f;

	/** Botão "Câmera fixa" ligado: arrastar o mouse não gira o personagem */
	UPROPERTY(BlueprintReadOnly, Category="Selecao") bool bLookFixed = false;

	/** Criação (30/10/2026, referência do Zenkai): peças reutilizáveis (WBP_ filhos das classes C++; sem eles usa o C++) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Criacao") TSoftClassPtr<UHypeOptionCardWidget> OptionCardClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Criacao") TSoftClassPtr<UHypeColorRowWidget> ColorRowClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Criacao") TSoftClassPtr<UHypeGradientSliderWidget> GradientSliderClass;
	/** Prévias das peças: <ThumbnailFolder>/T_Thumb_<Id> (geradas por Tools/editor/gerar_thumbnails_pecas.py) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Criacao") FString ThumbnailFolder = TEXT("/Game/HypeWorld/UI/CharacterSelect/Thumbs");
	/** Mapa do jogo: quando a tela abre em outro mapa (L_CharacterCreation), ele é carregado em segundo plano e o
	 *  Próximo / Entrar viaja para ele. Na própria tela do jogo, a câmera só volta para as costas. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Criacao") TSoftObjectPtr<UWorld> MapaDoJogo;
	/** Painel: coluna de ícones, começo e largura dos cards (px de 1080p, medidos na referência ×1,45) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Criacao") float IconColumnX = 134.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Criacao") float FormLeftX = 198.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Criacao") float FormWidthPx = 340.f;
	/** Escurecido da esquerda: opacidade na borda e até onde vai (fração da largura) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Criacao") float PanelShade = 0.85f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Criacao") float PanelShadeReach = 0.55f;
	/** Rosto (aba Aparência): distância da câmera à cabeça e tempo das transições da câmera (s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Camera") float FaceDistance = 95.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Camera") float CameraMoveTime = 0.4f;
	/** Graus de giro do personagem por px arrastado e zoom do scroll (multiplica a distância) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Camera") float DragDegreesPerPixel = 0.45f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selecao|Camera") FVector2D ZoomRange = FVector2D(0.8f, 1.12f);

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:

    // Criação: 1 identidade, 2 aparência, 3 estilo, 4 confirmação. Vila/clã só no retorno.
    bool bChoosingAffiliation = false;
    void ConfirmAffiliation();
    int32 CreationStep = 1;
    FHypeAppearance DraftAppearance;
    UPROPERTY() TObjectPtr<UEditableTextBox> NameInput;
    UPROPERTY() TObjectPtr<UTextBlock> HeightLabel;
    UPROPERTY() TObjectPtr<UTextBlock> SkinAmountLabel;
    void SyncAppearanceControls();
    void GoToCategory(int32 Step);
    UFUNCTION() void ShowIdentityCategory();
    UFUNCTION() void ShowAppearanceCategory();
    UFUNCTION() void ShowStyleCategory();
    UFUNCTION() void ShowConfirmCategory();
    UPROPERTY() TObjectPtr<UHypeGradientSliderWidget> HeightSlider;
    UPROPERTY() TObjectPtr<UHypeGradientSliderWidget> SkinSlider;
    UPROPERTY() TObjectPtr<UHypeColorRowWidget> HairColorRow;
    UPROPERTY() TObjectPtr<UTextBlock> HairColorHex;
    /** Cards das peças pela chave (Hair, Eyes, Outfit, Top, Bottom, Shoes) */
    UPROPERTY() TMap<FName, TObjectPtr<UHypeOptionCardWidget>> Cards;
    /** Opções de uma chave: ids (o primeiro é NAME_None = "Nenhum") e nomes */
    void GetPartOptions(FName Key, TArray<FName>& Ids, TArray<FText>& Names) const;
    FName& DraftPart(FName Key);
    void UpdateCards();
    void PreloadParts();
    UFUNCTION() void HandleCardStep(UHypeOptionCardWidget* Card, int32 Delta);
    UFUNCTION() void HandleHairColor(UHypeColorRowWidget* Row, int32 Index);
    UFUNCTION() void HandleSliderChanged(UHypeGradientSliderWidget* Slider, float Value);
    UPROPERTY() TObjectPtr<UTextBlock> CreationErrorLabel;
    UFUNCTION() void NextCreationStep();
    UFUNCTION() void PreviousCreationStep();
    UFUNCTION() void HandleNameChanged(const FText& Text);
    UFUNCTION() void HandleHeightChanged(float Value);
    UFUNCTION() void HandleSkinChanged(float Value);
    void ApplyDraftPreview();
    void UpdateCreationCamera(float Dt);
	bool bCreatingCharacter = false;
	bool bPawnWasHidden = false;
	FText ProfileError;
	UFUNCTION() void StartCreation();
	UFUNCTION() void CancelCreation();
	UFUNCTION() void EnterGame();
	void UpdatePreviewVisibility();
	void BuildLayout();
	UTexture2D* GetGeneratedIcon(FName Id, int32 Layer);
	void Refresh(bool bAnimate = false);
	void SetupPreviewCamera();
	void Confirm();
	void Finish();

	/** Todos os botões chamam este: descobre qual foi pelo que está sob o mouse (como o menu de combate) */
	UFUNCTION()
	void HandleClick();

	UPROPERTY() TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY() TArray<TObjectPtr<UButton>> VillageButtons;
	UPROPERTY() TArray<TObjectPtr<UButton>> ClanButtons;
	UPROPERTY() TObjectPtr<UButton> BackButton;
	UPROPERTY() TObjectPtr<UButton> NextButton;
	UPROPERTY() TObjectPtr<UButton> LookButton;
	/** Contorno tracejado que aparece com o mouse em cima de Voltar / Próximo */
	UPROPERTY() TObjectPtr<UImage> BackDashes;
	UPROPERTY() TObjectPtr<UImage> NextDashes;

	UPROPERTY() TObjectPtr<UTexture2D> ShadeTexture;
	UPROPERTY() TObjectPtr<UTexture2D> BoxTexture;
	UPROPERTY() TObjectPtr<UTexture2D> DashTexture;
	UPROPERTY() TObjectPtr<UTexture2D> EyeTexture;
	UPROPERTY() TObjectPtr<UTexture2D> LogoTexture;
	/** Símbolos das vilas/clãs desenhados por código (quando não há PNG): chave "Id" ou "Id_1" (segunda camada) */
	UPROPERTY() TMap<FName, TObjectPtr<UTexture2D>> IconCache;

	UPROPERTY() TObjectPtr<ACameraActor> PreviewCamera;
	/** Palco: de onde a câmera olha (fixo; o personagem gira, a câmera não) */
	FVector StageOrigin = FVector::ZeroVector;
	FVector StageForward = FVector::ForwardVector;
	FVector StageRight = FVector::RightVector;
	FRotator PawnStartRotation = FRotator::ZeroRotator;
	float Zoom = 1.f;
	bool bDragging = false;
	float DragLastX = 0.f;
	/** Viagem ao mapa do jogo depois da criação (tela aberta no L_CharacterCreation) */
	bool bTravelAfter = false;
	float TravelWait = -1.f;
	UPROPERTY() TMap<FName, TObjectPtr<UTexture2D>> ThumbCache;
	UTexture2D* GetThumbnail(FName Key, FName Id);
	UPROPERTY() TObjectPtr<UTexture2D> CreatorShadeTexture;
	UTexture2D* GetCreatorIcon(FName Name);
	/** Os widgets do conteúdo da aba (os que somem na troca); logo, abas e fundo ficam */
	bool bPlacingPageContent = false;
	/** Teste: -CreationShots percorre as abas sozinho, tira uma foto de cada (Saved/Screenshots) e cria o personagem */
	bool bShots = false;
	float ShotClock = 0.f;
	int32 ShotStep = 0;
	void TickShots(float Dt);

    // Transições leves em tempo real, sem criar assets ou animar o cenário.
    float PageTime = -1.f;
    bool bPageRebuildPending = false;
    TMap<TWeakObjectPtr<UWidget>, float> PageOpacities;
    TMap<TWeakObjectPtr<UButton>, float> ButtonHover;
	float Elapsed = 0.f;
	float ExitTime = -1.f;	// >= 0: saindo (Próximo apertado)
};
