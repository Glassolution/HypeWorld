// Slot de jutsu do HUD no estilo da referência do Felipe (06/11/2026, barra "Pied de l'Aube"): círculo grande com aro
// escuro de pincel e a arte do jutsu por dentro (M_UI_JutsuSlotRing, Tools/editor/criar_material_slot_anel.py). A arte
// vem do recorte dos slots (o material usa só o círculo da arte; moldura e etiqueta antigas ficam de fora).
// De trás para a frente: brilho azul de seleção (M_UI_JutsuGlow) -> aro + arte -> segundos do cooldown no meio
// ("27.1s", branco, negrito, contorno escuro) -> pílula preta do custo de chakra sobre a borda de baixo do aro.
// Estados: selecionado (maior, escala 1,1 em 0,12 s, brilho azul), hover (1,04), usado (pulso 1 -> 1,12 -> 1 em 0,18 s +
// flash branco), cooldown (arte escura + segundos), sem chakra (avermelhado), vazio (miolo escuro, sem pílula).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NinjaJutsuSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UCanvasPanelSlot;
class UMaterialInstanceDynamic;
class UTexture2D;

/** O que um slot mostra (o jutsu do slot) */
USTRUCT(BlueprintType)
struct FNinjaJutsuSlotData
{
	GENERATED_BODY()

	/** Arte do slot inteiro (recorte do PNG do Felipe: moldura + arte + etiqueta). Vazio = slot vazio */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Jutsu") TObjectPtr<UTexture2D> Icon = nullptr;
	/** Cor temática (brilho de seleção). Vento #7BE07A, raio #8C78FF, fogo #FF8A2B, Rasengan #3FA9FF, Sharingan #FF2A2A */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Jutsu") FLinearColor ThemeColor = FLinearColor(0.05f, 0.4f, 1.f);
	/** Número da etiqueta = custo de chakra (o HUD deixa o slot "sem chakra" quando o chakra atual é menor) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Jutsu") int32 Number = 0;
};

UCLASS()
class HYPEWORLD_API UNinjaJutsuSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Antes de entrar na tela: dados, diâmetro do slot com o aro (px em 1080p) e fonte (custo e segundos do cooldown) */
	void Setup(const FNinjaJutsuSlotData& InData, float InSize, const FSlateFontInfo& InNumberFont, const FSlateFontInfo& InCooldownFont);

	/** Tamanho do widget (caixa com folga para o brilho e a pílula) e onde fica o centro do círculo nela (0..1) */
	static FVector2D BoxSizeFor(float DiscSize);
	static FVector2D DiscAlignment();

	void SetSelected(bool bInSelected) { bSelected = bInSelected; }
	void SetHasChakra(bool bInHasChakra) { bHasChakra = bInHasChakra; }
	void SetCooldown(float Remaining, float Duration);
	void SetNumber(int32 Number);
	/** Troca o jutsu do slot já na tela (arte, cor, custo) */
	void SetData(const FNinjaJutsuSlotData& InData);
	const FNinjaJutsuSlotData& GetData() const { return Data; }
	int32 GetNumber() const { return Data.Number; }
	void PlayUsed() { UsedTime = 0.f; }

	/** Chamado pelo HUD todo quadro (animações dos estados) */
	void TickSlot(float DeltaTime);

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

private:

	void Build();

	UPROPERTY() TObjectPtr<UImage> Glow;
	UPROPERTY() TObjectPtr<UImage> Art;
	UPROPERTY() TObjectPtr<UTextBlock> NumberText;
	UPROPERTY() TObjectPtr<UTextBlock> CooldownText;
	UPROPERTY() TObjectPtr<UWidget> CostPill;
	UPROPERTY() TObjectPtr<UCanvasPanelSlot> GlowSlot;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> GlowMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ArtMID;

	FNinjaJutsuSlotData Data;
	FSlateFontInfo NumberFont;
	FSlateFontInfo CooldownFont;
	float Size = 50.f;
	int32 ShownCooldownTenths = -1;
	bool bSelected = false;
	bool bHasChakra = true;
	float CooldownLeft = 0.f;
	float CooldownDuration = 0.f;
	float SelectAlpha = 0.f;
	float HoverAlpha = 0.f;
	float UsedTime = -1.f;
	float ReadyTime = -1.f;
};
