// Ficha de um jutsu (Data Asset): selos, animação do lançamento, custo, cooldown, dano e o projétil.
// Um jutsu novo = um DA_Jutsu_* novo colocado num slot do personagem (JutsuLoadout); o código é o mesmo para todos.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "NinjaCombatTypes.h"
#include "NinjaJutsuData.generated.h"

class UAnimSequenceBase;
class UAnimMontage;
class UTexture2D;
class ANinjaJutsuProjectile;

/** Um selo da sequência: a pose (no esqueleto do corpo visível, ex. SKEL_Sasuke) e quanto tempo fica nela */
USTRUCT(BlueprintType)
struct FNinjaJutsuSeal
{
	GENERATED_BODY()

	/** Nome do selo (Tigre, Cavalo...), só para leitura */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selo")
	FName Name;

	/** Pose do selo, tocada no corpo visível (slot DefaultSlot do ABP do corpo, depois do retarget) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selo")
	TObjectPtr<UAnimSequenceBase> Pose = nullptr;

	/** Entrada no selo (mistura a partir da pose anterior, s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selo")
	float BlendIn = 0.12f;

	/** Tempo total no selo (s), contando a entrada. Se o clipe for mais curto, congela no último quadro (a pose do selo) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selo")
	float Duration = 0.55f;
};

UCLASS(BlueprintType)
class HYPEWORLD_API UNinjaJutsuData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu")
	FText DisplayName;

	/** Arte do slot do HUD (moldura + arte + etiqueta, mesmo recorte dos slots do Felipe) e cor do brilho */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|HUD")
	TObjectPtr<UTexture2D> Icon = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|HUD")
	FLinearColor ThemeColor = FLinearColor(1.f, 0.54f, 0.17f);

	/** Selos em ordem (cada um entra, segura e passa para o próximo; o último emenda no lançamento) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Animação")
	TArray<FNinjaJutsuSeal> Seals;

	/** Saída do último selo para a animação de lançamento (s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Animação")
	float SealBlendOut = 0.18f;

	/** Lançamento (no esqueleto do manequim, que anima o corpo pelo retarget): sopro e volta à prontidão.
	 *  Precisa do AnimNotify "NinjaJutsuRelease" no instante em que o jutsu sai */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Animação")
	TObjectPtr<UAnimMontage> CastMontage = nullptr;

	/** Sopro/recuperacao no esqueleto do corpo visivel. Quando definido, uma unica montagem
	 *  e montada em runtime com os selos do Data Asset seguidos deste clipe. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Animação")
	TObjectPtr<UAnimSequenceBase> BodyCastAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Animação")
	float CastBlendOut = 0.2f;

	/** Começa o lançamento este tempo antes do fim dos selos (s), para o corpo já inclinar enquanto as mãos saem do selo */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Animação")
	float CastOverlap = 0.1f;

	/** Socket de onde o jutsu sai (no corpo visível; se faltar, usa o osso da cabeça) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Animação")
	FName MuzzleSocket = TEXT("Boca");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Custo")
	float ChakraCost = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Custo")
	float Cooldown = 8.f;

	/** Tag do cooldown (uma por jutsu; está em Config/DefaultGameplayTags.ini, ramo Ninja.Cooldown.Jutsu) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Custo")
	FGameplayTag CooldownTag;

	/** Projétil (o efeito, o movimento e a explosão moram nele) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Projétil")
	TSubclassOf<ANinjaJutsuProjectile> ProjectileClass;

	/** Dano em área na explosão: dano no centro, raio (cm) e quanto sobra na borda (0..1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Dano")
	float Damage = 120.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Dano")
	float DamageRadius = 450.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Dano")
	float EdgeDamageScale = 0.4f;

	/** Impulso em corpos fisicos soltos. Personagens usam Knockback/Launch pelo combate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Dano")
	float PhysicsImpulse = 65000.f;

	/** Reação de quem é pego, empurrão para fora (cm/s), para cima (cm/s) e atordoamento (s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Dano")
	ENinjaHitReaction Reaction = ENinjaHitReaction::Knockback;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Dano")
	float Knockback = 700.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Dano")
	float Launch = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Dano")
	float Stun = 0.8f;

	/** Tremida da câmera no lançamento e na explosão (graus) e até onde a explosão treme (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Câmera")
	float CastShake = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Câmera")
	float ExplosionShake = 1.6f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jutsu|Câmera")
	float ExplosionShakeRadius = 3000.f;

	/** Duração total dos selos */
	float GetSealsDuration() const
	{
		float Total = 0.f;
		for (const FNinjaJutsuSeal& Seal : Seals)
		{
			Total += Seal.Duration;
		}
		return Total;
	}
};
