// Ferramentas chamadas pelos scripts do editor (Tools/editor/*.py) para coisas que o Python não alcança.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NinjaEditorTools.generated.h"

class UBlendSpace;
class UAnimMontage;
class UAnimSequenceBase;
class UFont;
class UFontFace;
class ULandscapeLayerInfoObject;
class UMaterialInterface;
class ALandscape;

UCLASS()
class HYPEWORLD_API UNinjaEditorTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Reconstrói os dados internos (triangulação) de um blend space depois de trocar amostras por script. */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool RebuildBlendSpace(UBlendSpace* BlendSpace);

	/** Recorta um golpe de uma animação: a montagem toca só [StartTime, EndTime] da animação.
	 *  HitTimes = momentos de acerto (AnimNotify DoAttackTrace); ComboTime = abre a janela do próximo golpe (CheckCombo, <0 = sem).
	 *  Tempos no relógio da animação original. */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool BuildAttackMontage(UAnimMontage* Montage, UAnimSequenceBase* Animation, float StartTime, float EndTime,
		const TArray<float>& HitTimes, float ComboTime, float BlendIn, float BlendOut);

	/** Montagem com vários trechos em sequência (Animations[i] de Starts[i] a Ends[i]; End < 0 = até o fim).
	 *  bLoopLast = o último trecho repete sozinho (seção "Loop" que aponta para ela mesma).
	 *  bHoldLastPose = não volta sozinha para a animação de base no fim (fica na última pose até alguém parar). */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool BuildSequenceMontage(UAnimMontage* Montage, const TArray<UAnimSequenceBase*>& Animations, const TArray<float>& Starts,
		const TArray<float>& Ends, float BlendIn, float BlendOut, bool bLoopLast, bool bHoldLastPose);

	/** Igual à BuildSequenceMontage, com seções nomeadas: SectionNames[i] (vazio = continua a seção anterior) começa no
	 *  trecho i. Cada seção segue para a próxima seção nomeada, menos as de LoopSections (repetem sozinhas). Uma seção depois
	 *  de uma que repete só é alcançada por salto no código (ex.: "End" do dash). */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool BuildSectionMontage(UAnimMontage* Montage, const TArray<UAnimSequenceBase*>& Animations, const TArray<float>& Starts,
		const TArray<float>& Ends, const TArray<FName>& SectionNames, const TArray<FName>& LoopSections, float BlendIn, float BlendOut, bool bHoldLastPose);

	/** Monta uma fonte composta (o Python não alcança o Typeface): cada face (ex.: "ExtraBold" -> Montserrat-ExtraBold) vira
	 *  uma entrada da fonte padrão, carregada em tempo de jogo. */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool BuildCompositeFont(UFont* Font, const TArray<FName>& FaceNames, const TArray<UFontFace*>& Faces);

	/** Cria (ou reaproveita) o asset de camada de landscape (LayerInfo) em PackagePath (ex.: /Game/Maps/Tutorial/Camadas/LI_Grama).
	 *  bWeightBlended = camada visual que divide o peso com as outras; false = máscara independente (só para o PCG ler). */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static ULandscapeLayerInfoObject* CreateLandscapeLayerInfo(const FString& PackagePath, FName LayerName, bool bWeightBlended);

	/** Cria um landscape no mundo (o Python não alcança o Import): ComponentsX x ComponentsY componentes de QuadsPerSection
	 *  quads (1 seção cada), canto em Location, escala Scale. Heights = (ComponentsX*Quads+1) x (ComponentsY*Quads+1) valores
	 *  0..65535 (32768 = altura 0), linha a linha em X. Weights = um bloco do mesmo tamanho por camada, na ordem de Layers. */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor", meta=(WorldContext="WorldContextObject"))
	static ALandscape* CreateLandscape(UObject* WorldContextObject, FVector Location, FVector Scale, int32 ComponentsX, int32 ComponentsY,
		int32 QuadsPerSection, const TArray<int32>& Heights, UMaterialInterface* Material, const TArray<ULandscapeLayerInfoObject*>& Layers,
		const TArray<uint8>& Weights);

	/** Blocking Volume em forma de caixa (Size = tamanho total em cm), invisível, bloqueia o jogador */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor", meta=(WorldContext="WorldContextObject"))
	static AActor* SpawnBlockingBox(UObject* WorldContextObject, FVector Location, FRotator Rotation, FVector Size);
};
