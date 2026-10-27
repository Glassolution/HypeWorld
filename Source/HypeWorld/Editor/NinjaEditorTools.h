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
class UAnimBlueprint;
class USkeletalMesh;

class UNiagaraSystem;
class UCurveLinearColor;
class UNiagaraEmitter;
class UTexture2D;

UCLASS()
class HYPEWORLD_API UNinjaEditorTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool AddNiagaraUpdateModule(UNiagaraSystem* System, const FString& EmitterName, const FString& ScriptPath, int32 Index);
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool LinkNiagaraFloatInput(UNiagaraSystem* System, const FString& EmitterName, const FString& ModuleName, const FString& InputName, const FString& ParameterName);
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static UTexture2D* CreateFireColorAtlas(const FString& PackagePath);

	/** Reconstrói os dados internos (triangulação) de um blend space depois de trocar amostras por script. */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool RebuildBlendSpace(UBlendSpace* BlendSpace);

	/** Cria um Anim Blueprint para TargetMesh cujo AnimGraph é só um "Retarget Pose From Mesh" (pose do componente pai,
	 *  pelo IKRetargeter) ligado ao Output Pose. O nó da engine não é exportado, então é criado por reflexão. */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static UAnimBlueprint* CreateRetargetAnimBlueprint(USkeletalMesh* TargetMesh, UObject* IKRetargeter, const FString& PackagePath, const FString& AssetName);

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
	/** Põe um nó Slot (SlotName) entre o último nó e o Output Pose do AnimGraph (ex.: ABP do corpo visível depois do
	 *  Retarget Pose From Mesh, para montagens tocadas no próprio corpo, como os selos de jutsu). Não duplica se já existir. */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool InsertSlotBeforeOutput(UAnimBlueprint* AnimBP, FName SlotName);

	// ---------- Niagara (o Python não alcança as pilhas dos emissores) ----------

	/** Cria um sistema Niagara com cópias dos emissores dados (ex.: modelos vazios do plugin), com os nomes dados */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static UNiagaraSystem* CreateNiagaraSystem(const FString& PackagePath, const FString& AssetName, const TArray<UNiagaraEmitter*>& Emitters, const TArray<FString>& EmitterNames);

	/** Lista emissores, parâmetros constantes (rapid iteration) de cada script com valores e as propriedades dos renderers */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static FString DescribeNiagaraSystem(UNiagaraSystem* System, bool bRendererProperties);

	/** Troca um parâmetro constante de módulo (nome completo ou final, ex. "SpawnRate.SpawnRate") em todos os scripts do
	 *  emissor. Valor em texto: "1.5", "1,2,3", "1,0.5,0,1", "true", ou inteiro (enums). Devolve quantos achou */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static int32 SetNiagaraConstant(UNiagaraSystem* System, const FString& EmitterName, const FString& ParamName, const FString& Value);

	/** Propriedade de um renderer do emissor por reflexão (ImportText: "Material", "Alignment", "SubImageSize"...) */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool SetNiagaraRendererProperty(UNiagaraSystem* System, const FString& EmitterName, int32 RendererIndex, const FString& Property, const FString& Value);

	/** Propriedade do emissor (FVersionedNiagaraEmitterData: "bLocalSpace", "SimTarget", "bDeterminism"...) */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool SetNiagaraEmitterProperty(UNiagaraSystem* System, const FString& EmitterName, const FString& Property, const FString& Value);

	/** Compila o sistema (espera terminar) e marca para salvar */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool CompileNiagaraSystem(UNiagaraSystem* System, bool bFixedBounds, FVector BoundsExtent);

	/** Entradas de compilacao dos modulos (inclusive modos de velocidade e de loop). */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static FString DescribeNiagaraModulePins(UNiagaraSystem* System, const FString& EmitterName);

	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool SetNiagaraModulePin(UNiagaraSystem* System, const FString& EmitterName,
		const FString& ModuleName, const FString& PinName, const FString& Value);

	/** Configura o Baker com os canais reais do Grid2D, sem captura de cor tonemapeada. */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static FString PrepareFluidBaker(UNiagaraSystem* System, float StartSeconds, float DurationSeconds);

	/** Curva de cor em degraus: Colors[i] vale a partir de i/N (interpolação constante), para Curve Atlas de faixas fixas */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor")
	static bool SetSteppedColorCurve(UCurveLinearColor* Curve, const TArray<FLinearColor>& Colors);

	/** Cria um ator localmente no mundo do jogo (inclusive num cliente de rede, onde summon não funciona), para capturas de teste */
	UFUNCTION(BlueprintCallable, Category="Ninja|Editor", meta=(WorldContext="WorldContext"))
	static AActor* SpawnLocalActor(UObject* WorldContext, TSubclassOf<AActor> ActorClass, FVector Location, FRotator Rotation);
};
