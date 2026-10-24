// Tipos do combate corpo a corpo: a ficha de cada golpe, as reações de quem apanha e as tags do GAS.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "NinjaCombatTypes.generated.h"

class UAnimMontage;

/** Atordoado: não ataca, não anda, não pula (dado pelo GE_NinjaStun). */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ninja_State_Stunned);
/** Evento que dispara um golpe; EventMagnitude = id do golpe (ver AHypeWorldCharacter::GetAttack). */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ninja_Event_Attack);

/** Como quem apanha reage. */
UENUM(BlueprintType)
enum class ENinjaHitReaction : uint8
{
	Light,			// tranco curto
	Heavy,			// dobra com o golpe, fica atordoado mais tempo
	Knockback,		// é jogado para trás
	Launch,			// sobe no ar
	Block,			// defendeu
	GuardBreak		// a defesa quebrou
};

/** Botão de ataque apertado. */
UENUM(BlueprintType)
enum class ENinjaAttackInput : uint8
{
	None,
	Light,			// botão esquerdo
	Heavy,			// F
	Launcher		// Espaço no meio do combo: golpe que joga o adversário para cima (começa o combo aéreo)
};

/** Ficha de um golpe: animação (montagem recortada) e o que ele faz em quem é atingido. */
USTRUCT(BlueprintType)
struct FNinjaAttack
{
	GENERATED_BODY()

	/** Nome para depuração. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Golpe")
	FName Name;

	/** Montagem do golpe (com os AnimNotifies de acerto e da janela do próximo golpe). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Golpe")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	/** Velocidade da animação (maior = golpe mais rápido). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Golpe")
	float PlayRate = 1.f;

	/** Dano de cada acerto. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Golpe")
	float Damage = 30.f;

	/** Reação no último acerto do golpe (acertos antes dele são sempre leves). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Golpe")
	ENinjaHitReaction Reaction = ENinjaHitReaction::Light;

	/** Empurrão para trás em quem apanha (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Golpe")
	float Knockback = 150.f;

	/** Empurrão para cima em quem apanha (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Golpe")
	float Launch = 0.f;

	/** Tempo atordoado (s): tem que ser maior que a distância até o próximo golpe, senão o combo não "fecha". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Golpe")
	float Stun = 0.55f;

	/** Alcance à frente do corpo e raio do golpe (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Golpe")
	float Reach = 140.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Golpe")
	float Radius = 55.f;

	/** Quebra a defesa. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Golpe")
	bool bBreaksGuard = false;

	/** Escala do avanço da própria animação (1 = como foi gravada). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Golpe")
	float RootMotionScale = 1.f;
};
