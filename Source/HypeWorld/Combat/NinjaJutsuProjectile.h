// Projétil de jutsu (Bola de Fogo e afins), replicado.
// O servidor cria o ator com o ponto de saída, a direção e a hora de saída (hora do servidor); cada máquina calcula a mesma
// trajetória a partir disso (sem replicar o movimento quadro a quadro, então fica liso em todos). Só o servidor testa a
// colisão e o fim do alcance e manda a explosão para todos (posição + dano em área no servidor).
// Visual: corpo de sprites Niagara com flipbooks gravados de fluidos; malhas legadas somente na explosão.
// Distorção de calor, fitas, brasas e luz sem sombra acompanham a trajetória replicada.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NinjaJutsuProjectile.generated.h"

class UNinjaJutsuData;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraSystem;
class UNiagaraComponent;
class UPointLightComponent;
class AHypeWorldCharacter;
class USphereComponent;
class UPostProcessComponent;
class UDecalComponent;

/** Saída do projétil (replicada uma vez) */
USTRUCT()
struct FNinjaJutsuLaunch
{
	GENERATED_BODY()

	UPROPERTY() FVector_NetQuantize10 Start = FVector::ZeroVector;
	UPROPERTY() FVector_NetQuantizeNormal Direction = FVector::ForwardVector;
	UPROPERTY() double StartTime = 0.0;
	UPROPERTY() TObjectPtr<const UNinjaJutsuData> Jutsu = nullptr;
	UPROPERTY() int32 VisualSeed = 1;
};

USTRUCT()
struct FNinjaJutsuImpact
{
	GENERATED_BODY()
	UPROPERTY() bool bValid = false;
	UPROPERTY() FVector_NetQuantize10 Location = FVector::ZeroVector;
	UPROPERTY() double ServerTime = 0.0;
};

UCLASS(Blueprintable)
class HYPEWORLD_API ANinjaJutsuProjectile : public AActor
{
	GENERATED_BODY()

public:

	ANinjaJutsuProjectile();

	/** Servidor, antes de FinishSpawning */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Jutsu")
	void InitLaunch(const UNinjaJutsuData* InJutsu, const FVector& InStart, const FVector& InDirection);

	/** Pré-carregamento (só em máquinas que renderizam, uma vez por mundo e classe): uma cópia local, sem rede e sem dano,
	 *  voa e explode escondida bem abaixo do Context, para o primeiro lançamento de verdade não travar carregando
	 *  materiais, PSOs e sistemas Niagara */
	static void Prewarm(AActor* Context, const UNinjaJutsuData* Jutsu);

	// ---------- Trajetória ----------

	/** A chama cresce de StartRadius até FullRadius em GrowTime (subindo GrowLift) enquanto já avança */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Trajetória") float GrowTime = 0.4f;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Trajetória") float GrowLift = 35.f;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Trajetória") float StartRadius = 12.f;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Trajetória") float FullRadius = 150.f;
	/** Avança até Range (cm, contado da boca): sai a LaunchSpeedScale x Speed e acelera (ease-in) até Speed (cm/s) em AccelTime */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Trajetória") float Speed = 1700.f;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Trajetória") float LaunchSpeedScale = 0.35f;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Trajetória") float AccelTime = 0.3f;
	/** No cliente: atraso da replicação que a bola recupera ao aparecer (s); o resto ela recupera aos poucos, sem pulo */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Trajetória") float MaxCatchUp = 0.05f;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Trajetória") float Range = 2300.f;
	/** Raio da colisão em relação ao raio visível (a casca de chamas pode encostar no chão sem explodir) */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Trajetória") float CollisionRadiusScale = 0.5f;
	/** Giro das cascas de chamas (graus/s) */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Trajetória") float SpinSpeed = 160.f;

	// ---------- Visual ----------

	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UStaticMesh> BallMesh;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UNiagaraSystem> SpiralSystem;
	/** Flipbooks de fluidos gravados: nenhum solver de fluidos em runtime. */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UNiagaraSystem> BodySystem;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UStaticMesh> IgnitionMesh;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UMaterialInterface> IgnitionMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UMaterialInterface> CoreMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UMaterialInterface> FlameMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UMaterialInterface> OuterFlameMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UMaterialInterface> HeatMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UMaterialInterface> HaloMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UMaterialInterface> GroundGlowMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UMaterialInterface> ShockwaveMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UNiagaraSystem> GroundFireSystem;
	/** Escala de cada casca em relação ao raio */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") FVector ShellScales = FVector(1.f, 1.18f, 1.42f);
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") float HeatScale = 1.7f;

	/** Partículas em volta da bola (brasas, faíscas, fumaça), presas a ela */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UNiagaraSystem> TrailSystem;
	/** Saída na boca (faíscas + chama pequena + jato até a bola), preso ao socket da boca pelo personagem */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UNiagaraSystem> MuzzleSystem;
	/** Quanto tempo a boca solta fogo (s); depois o efeito para de emitir e some sozinho */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") float MuzzleDuration = 0.45f;
	/** Explosão (clarão, labaredas, brasas, fumaça subindo) */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UNiagaraSystem> ExplosionSystem;
	/** Explosão estilizada (formas 3D + material mestre). Preenchida = substitui ExplosionSystem, GroundFireSystem e as
	 *  camadas antigas da explosão (onda das cascas, halo, distorção de tela, onda no chão) */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TSubclassOf<class ANinjaFireExplosion> StylizedExplosionClass;
	/** Marca de queimado no chão: tamanho (cm), tempo visível e tempo sumindo (s) */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") TObjectPtr<UMaterialInterface> ScorchDecal;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") float ScorchSize = 380.f;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") float ScorchHold = 3.f;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") float ScorchFade = 2.5f;

	/** Luz: cor, intensidade com a bola cheia, raio, e o clarão da explosão (x intensidade, s) */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") FLinearColor LightColor = FLinearColor(1.f, 0.45f, 0.12f);
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") float LightIntensity = 60.f;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") float LightRadius = 1100.f;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") float FlashMultiplier = 3.f;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") float FlashTime = 0.6f;

	/** Onda de fogo da explosão: escala final (x raio cheio) e duração (s) */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") float WaveScale = 2.6f;
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") float WaveTime = 0.7f;

	/** Depois da explosão o ator ainda vive este tempo (onda e luz) (s) */
	UPROPERTY(EditDefaultsOnly, Category="Jutsu|Visual") float LingerTime = 1.5f;

	/** Instante da trajetória (s desde a saída, no relógio do servidor) — para medições e capturas */
	UFUNCTION(BlueprintPure, Category="Jutsu") float GetFlightTime() const;
	UFUNCTION(BlueprintPure, Category="Jutsu") bool HasExploded() const { return bExploded; }

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, Category="Jutsu") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category="Jutsu") TObjectPtr<UStaticMeshComponent> Core;
	UPROPERTY(VisibleAnywhere, Category="Jutsu") TObjectPtr<UStaticMeshComponent> Flames;
	UPROPERTY(VisibleAnywhere, Category="Jutsu") TObjectPtr<UStaticMeshComponent> OuterFlames;
	UPROPERTY(VisibleAnywhere, Category="Jutsu") TObjectPtr<UStaticMeshComponent> Heat;
	UPROPERTY(VisibleAnywhere, Category="Jutsu") TObjectPtr<UStaticMeshComponent> Halo;
	UPROPERTY(VisibleAnywhere, Category="Jutsu") TObjectPtr<UStaticMeshComponent> GroundWave;
	UPROPERTY(VisibleAnywhere, Category="Jutsu") TObjectPtr<UDecalComponent> GroundGlow;
	UPROPERTY(VisibleAnywhere, Category="Jutsu") TObjectPtr<UNiagaraComponent> Trail;
	UPROPERTY(VisibleAnywhere, Category="Jutsu") TObjectPtr<UNiagaraComponent> BodyFlames;
	UPROPERTY(VisibleAnywhere, Category="Jutsu") TArray<TObjectPtr<UNiagaraComponent>> Spirals;
	UPROPERTY(VisibleAnywhere, Category="Jutsu") TObjectPtr<UStaticMeshComponent> Ignition;
	UPROPERTY(VisibleAnywhere, Category="Jutsu") TObjectPtr<UPointLightComponent> Light;
	UPROPERTY(VisibleAnywhere, Category="Jutsu") TObjectPtr<USphereComponent> BloomBounds;
	UPROPERTY(VisibleAnywhere, Category="Jutsu") TObjectPtr<UPostProcessComponent> FirePostProcess;

	UPROPERTY(ReplicatedUsing=OnRep_Launch) FNinjaJutsuLaunch Launch;
	UFUNCTION() void OnRep_Launch();
	UPROPERTY(ReplicatedUsing=OnRep_Impact) FNinjaJutsuImpact Impact;
	UFUNCTION() void OnRep_Impact();

	UFUNCTION(NetMulticast, Reliable) void MulticastExplode(FVector_NetQuantize10 Location);

private:

	void SetupVisuals();
	/** Centro e raio da bola no instante T */
	void Evaluate(float T, FVector& OutCenter, float& OutRadius, bool& bOutReachedRange) const;
	void ApplyBallVisual(const FVector& Center, float Radius, float T);
	void UpdateSceneIntegration(const FVector& Center, float Radius, float T, bool bImpact);
	void ServerCheckHit(const FVector& From, const FVector& To, float Radius, float T);
	void Explode(const FVector& Location);
	void PlayExplosion(const FVector& Location);
	void ApplyAreaDamage(const FVector& Location);
	AHypeWorldCharacter* GetCaster() const;
	double Now() const;

	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> MIDs;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> HaloMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> WaveMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> GlowMID;
	FVector GroundPoint = FVector::ZeroVector;
	FVector GroundNormal = FVector::UpVector;
	double LastGroundTrace = -100.0;
	bool bHasGround = false;
	bool bLaunched = false;
	bool bPrewarm = false;
	/** Relógio local do voo (o do servidor dá saltos no cliente): início local e atraso ainda a recuperar */
	double LocalLaunchTime = 0.0;
	float CatchUpDebt = 0.f;
	bool bExploded = false;
	double ExplodeTime = 0.0;
	FVector LastCenter = FVector::ZeroVector;
	float LastRadius = 0.f;
	float SpinAngle = 0.f;
};
