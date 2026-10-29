// HUD do ninja montado inteiro em C++, limpo, no modelo da referência do Felipe (24/10/2026):
// canto de baixo à esquerda = bloco de vida feito com as imagens PNG do Felipe recortadas em peças (retrato com anel de XP +
// nível, nome, vida "750/750", chakra cinza com % fora da batalha / azul com "atual / máximo" na batalha, moedas só fora
// da batalha); centro de baixo = slots de jutsu, abaixados (só a pontinha aparece) e que sobem na batalha.
// Sem minimapa, sem missão, sem lista de atalhos.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NinjaJutsuSlotWidget.h"
#include "NinjaHUDWidget.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;
class UImage;
class UTextBlock;
class UProgressBar;
class UTexture2D;
class UWidget;
class UNinjaJutsuData;

UCLASS()
class HYPEWORLD_API UNinjaHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Retrato do personagem (recortado em círculo). */
	UPROPERTY(EditAnywhere, Category="HUD|Imagens")
	UTexture2D* PortraitTexture = nullptr;

	/** Ícones antigos dos jutsus (dos slots de antes da arte do Felipe; não usados pelos slots atuais) */
	UPROPERTY(EditAnywhere, Category="HUD|Imagens")
	TArray<UTexture2D*> AbilityIcons;

	/** Experiência do nível atual (0..1) — o arco azul em volta do retrato. Placeholder até existir XP. */
	UPROPERTY(EditAnywhere, Category="HUD")
	float ExperiencePercent = 0.75f;

	/** Fonte do bloco de vida: a Montserrat (fonte composta com as faces "ExtraBold", "Bold" e "SemiBold").
	 *  Vazio = Roboto da engine, só como substituta temporária até a Montserrat ser importada. */
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	UObject* HudFont = nullptr;

	/** Nome no bloco de vida. Vazio = nome do jogador na rede (no Mac, o nome do computador). Até existir o criador de
	 *  personagem, é o mesmo para todos os jogadores */
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	FString DisplayName;

	/** O bloco de vida usa as medidas da imagem do Felipe (px da imagem normal); LifeBlockScale = px de tela (1920x1080) por px
	 *  da imagem. Posição = canto de baixo à esquerda do bloco, a partir do canto de baixo à esquerda da tela */
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	FVector2D LifeBlockPosition = FVector2D(24.f, -22.f);
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	float LifeBlockScale = 0.18f;

	/** Peças recortadas das imagens do Felipe (Tools/hud/recortar_bloco_vida.py): base (retrato, anel sem o arco, caixa de voz),
	 *  selo de nível, barras cheia/vazia (vida; chakra cinza fora da batalha e azul na batalha), moeda e os 2 ícones da batalha */
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	UTexture2D* LifeBaseTexture = nullptr;
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	UTexture2D* LevelBadgeTexture = nullptr;
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	UTexture2D* HealthFullTexture = nullptr;
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	UTexture2D* HealthEmptyTexture = nullptr;
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	UTexture2D* ChakraFullTexture = nullptr;
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	UTexture2D* ChakraEmptyTexture = nullptr;
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	UTexture2D* ChakraBattleFullTexture = nullptr;
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	UTexture2D* ChakraBattleEmptyTexture = nullptr;
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	UTexture2D* CoinIconTexture = nullptr;
	/** Cards da imagem de batalha, guardados para os efeitos (atordoado, sangramento, tontura...) — ainda não aparecem */
	UPROPERTY(EditAnywhere, Category="HUD|Bloco de vida")
	TArray<UTexture2D*> BattleIconTextures;

	/** Moedas (placeholder até existir dinheiro) */
	UPROPERTY(EditAnywhere, Category="HUD")
	int32 Coins = 0;

	/** Quantos slots de jutsu a barra tem */
	UPROPERTY(EditAnywhere, Category="HUD|Jutsus")
	int32 JutsuSlotCount = 6;

	/** O jutsu de cada slot (arte recortada do PNG do Felipe, cor temática, custo de chakra). Vazio = slots vazios */
	UPROPERTY(EditAnywhere, Category="HUD|Jutsus")
	TArray<FNinjaJutsuSlotData> JutsuSlotData;

	/** Diâmetro do círculo da arte do slot (px em 1080p; a escala de DPI da interface ajusta outras resoluções; a moldura
	 *  e a etiqueta passam dele), espaço a mais entre os centros (centro a centro = Size + Gap) e margem de baixo (fração da
	 *  altura da tela, até a ponta da etiqueta) */
	UPROPERTY(EditAnywhere, Category="HUD|Jutsus")
	float JutsuSlotSize = 50.f;
	UPROPERTY(EditAnywhere, Category="HUD|Jutsus")
	float JutsuSlotGap = 24.f;
	UPROPERTY(EditAnywhere, Category="HUD|Jutsus")
	float JutsuBottomMargin = 0.04f;

	/** Estados dos slots (ligar quando os jutsus existirem) */
	UFUNCTION(BlueprintCallable, Category="HUD|Jutsus") void SetSelectedJutsu(int32 Index);
	UFUNCTION(BlueprintCallable, Category="HUD|Jutsus") void SetJutsuCooldown(int32 Index, float Remaining, float Duration);
	UFUNCTION(BlueprintCallable, Category="HUD|Jutsus") void SetJutsuHasChakra(int32 Index, bool bHasChakra);
	UFUNCTION(BlueprintCallable, Category="HUD|Jutsus") void SetJutsuNumber(int32 Index, int32 Number);
	UFUNCTION(BlueprintCallable, Category="HUD|Jutsus") void PlayJutsuUsed(int32 Index);

	/** Quanto os slots descem fora do modo de batalha (px; sobra só a pontinha) */
	UPROPERTY(EditAnywhere, Category="HUD|Jutsus")
	float JutsuHiddenOffset = 110.f;

	/** Barrinha vertical do super pulo (referência do Felipe): tamanho (px) e posição em relação à cabeça do personagem
	 *  na tela (px; + = direita / baixo) */
	UPROPERTY(EditAnywhere, Category="HUD")
	FVector2D SuperJumpBarSize = FVector2D(6.f, 70.f);
	UPROPERTY(EditAnywhere, Category="HUD")
	FVector2D SuperJumpBarOffset = FVector2D(80.f, 0.f);

	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetHealth(float Current, float Max);

	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetChakra(float Current, float Max);

	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetPlayerLevel(int32 Level);

	/** Mantido por compatibilidade (o HUD novo não mostra missão) */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetQuest(const FText& Title, const FText& Step) {}

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	void BuildLayout();
	void BuildLifeBlock(UCanvasPanel* Root);
	FSlateFontInfo HudFontInfo(const TCHAR* Face, float SizePx) const;

	UPROPERTY() TObjectPtr<UTextBlock> HealthText;
	UPROPERTY() TObjectPtr<UTextBlock> HealthMaxText;
	UPROPERTY() TObjectPtr<UTextBlock> ChakraText;
	UPROPERTY() TObjectPtr<UTextBlock> LevelText;
	UPROPERTY() TObjectPtr<UTextBlock> NameText;
	UPROPERTY() TObjectPtr<UTextBlock> CoinText;
	UPROPERTY() TObjectPtr<UImage> HealthFill;
	UPROPERTY() TObjectPtr<UImage> ChakraFill;
	UPROPERTY() TObjectPtr<UImage> ChakraEmpty;
	UPROPERTY() TObjectPtr<UImage> XpRing;
	UPROPERTY() TObjectPtr<UTexture2D> RingTexture;
	/** Linha de baixo: moedas (só fora da batalha) */
	UPROPERTY() TArray<TObjectPtr<UWidget>> CoinWidgets;
	int32 ShownBattle = -1;
	float ShownXp = -1.f;
	int32 ShownCoins = -1;
	static FString FormatThousands(int32 Value);
	/** Barras: a imagem de preenchimento é recortada (não esticada) conforme a porcentagem */
	void SetFill(UImage* Fill, UTexture2D* Texture, float Percent, float Width, const FLinearColor& Tint);
	void UpdateXpRing();
	/** Troca o visual do modo de batalha: chakra azul com "atual / máximo"; fora dele chakra cinza com % e moedas */
	void ApplyBattleLook(bool bBattle);
	/** px da imagem do Felipe -> px do bloco na tela */
	float K() const { return LifeBlockScale; }
	UPROPERTY() TObjectPtr<UTextBlock> ComboText;
	/** Chamas em órbita: contador "Nome n/N" com a barra do tempo embaixo e a mira circular no centro */
	UPROPERTY() TObjectPtr<UTextBlock> OrbitText;
	UPROPERTY() TObjectPtr<UProgressBar> OrbitBar;
	UPROPERTY() TObjectPtr<UImage> OrbitCrosshair;
	UPROPERTY() TObjectPtr<UImage> OrbitCrosshairDot;
	int32 ShownOrbitFlames = -1;
	UPROPERTY() TObjectPtr<UProgressBar> SuperJumpBar;
	UPROPERTY() TObjectPtr<UCanvasPanelSlot> SuperJumpSlot;
	UPROPERTY() TObjectPtr<UImage> SuperJumpGlow;
	UPROPERTY() TObjectPtr<UCanvasPanelSlot> SuperJumpGlowSlot;
	UPROPERTY() TArray<TObjectPtr<UImage>> SubstitutionPips;

	/** Slots de jutsu (e as cargas da substituição): sobem e descem juntos com o modo de batalha */
	UPROPERTY() TArray<TObjectPtr<UCanvasPanelSlot>> JutsuSlots;
	UPROPERTY() TArray<TObjectPtr<UNinjaJutsuSlotWidget>> JutsuSlotWidgets;
	TArray<FVector2D> JutsuSlotBase;
	/** Jutsu mostrado em cada slot (do JutsuLoadout do personagem) e usos já vistos (pulso ao usar) */
	TArray<TWeakObjectPtr<const UNinjaJutsuData>> ShownSlotJutsu;
	TArray<int32> ShownJutsuUses;
	float JutsuOffset = 78.f;

	float Health = -1.f;
	float HealthMax = -1.f;
	float Chakra = -1.f;
	float ChakraMax = -1.f;
	int32 PlayerLevel = 12;
	FString ShownName;
	int32 ShownInfusion = -1;
	int32 ShownSubstitutions = -1;
};
