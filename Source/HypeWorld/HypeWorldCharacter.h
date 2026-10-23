// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "HypeWorldCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UNinjaMovementComponent;
class UAnimMontage;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AHypeWorldCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Corrida ninja (segurar) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SprintAction;

	/** Dash (apertar) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DashAction;

	/** Animação do dash */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* DashMontage;

	/** Animação do pulo de parede */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* WallJumpMontage;

	/** Animação em loop enquanto corre na parede */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* WallRunMontage;

	/** Câmera na corrida ninja: distância até o boneco (cm) */
	UPROPERTY(EditAnywhere, Category="Ninja|Câmera")
	float SprintCameraArmLength = 280.f;

	/** Câmera na corrida ninja: quanto sobe (cm) */
	UPROPERTY(EditAnywhere, Category="Ninja|Câmera")
	float SprintCameraHeight = 90.f;

	/** Câmera na corrida ninja: quanto inclina para baixo (graus) */
	UPROPERTY(EditAnywhere, Category="Ninja|Câmera")
	float SprintCameraPitch = 14.f;

	/** Velocidade a partir da qual a câmera entra no modo corrida (cm/s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Câmera")
	float SprintCameraMinSpeed = 700.f;

	/** Na corrida ninja a câmera acompanha mais firme (menos atraso), senão o boneco escapa para a frente */
	UPROPERTY(EditAnywhere, Category="Ninja|Câmera")
	float SprintCameraLagSpeed = 40.f;

	/** Rapidez da transição da câmera */
	UPROPERTY(EditAnywhere, Category="Ninja|Câmera")
	float CameraBlendSpeed = 4.f;

	/** Animação que toca de vez em quando se ficar parado (olhar em volta) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	UAnimMontage* IdleBreakMontage;

	/** Segundos parado até tocar a animação de olhar em volta */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float IdleBreakDelay = 30.f;

	/** Inclinação máxima ao fazer curva correndo (graus) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float MaxTurnLean = 15.f;

	/** Inclinação correndo na parede, para longe da parede (graus) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float WallRunLean = 30.f;

	/** Quanto o boneco encosta na parede durante a corrida na parede (cm) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float WallRunMeshOffset = 35.f;

	/** Rapidez com que a inclinação acompanha (maior = mais rápido) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float LeanInterpSpeed = 8.f;

	/** Velocidade em que a animação de corrida combina com os pés; acima disso ela é acelerada */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float RunAnimNaturalSpeed = 600.f;

	/** Quanto a animação de corrida pode ser acelerada no máximo (2,7x = corrida de 4,5 m/s chega a ~12 m/s) */
	UPROPERTY(EditAnywhere, Category="Ninja|Animação")
	float MaxRunAnimRate = 2.7f;

public:

	/** Constructor */
	AHypeWorldCharacter(const FObjectInitializer& ObjectInitializer);

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Começa a corrida ninja */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintStart();

	/** Para a corrida ninja */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintEnd();

	/** Pede um dash */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoDash();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Componente de movimento ninja */
	UFUNCTION(BlueprintPure, Category="Ninja")
	UNinjaMovementComponent* GetNinjaMovement() const;

private:

	/** Inclina o boneco nas curvas e na parede (só visual, roda em todas as máquinas) */
	void UpdateLean(float DeltaSeconds);

	/** Acelera a animação de corrida quando o boneco vai mais rápido que ela, para os pés não "patinarem" */
	void UpdateRunAnimRate();

	/** Aproxima, sobe e inclina a câmera durante a corrida ninja (só para quem está jogando nesta máquina) */
	void UpdateSprintCamera(float DeltaSeconds);

	float DefaultArmLength = 400.f;
	FVector DefaultSocketOffset = FVector::ZeroVector;
	FRotator DefaultCameraRotation = FRotator::ZeroRotator;
	float DefaultLagSpeed = 12.f;
	float DefaultLagMaxDistance = 60.f;
	float SprintCameraAlpha = 0.f;

	/** Conta o tempo parado e toca a animação de olhar em volta depois de IdleBreakDelay */
	void UpdateIdleBreak(float DeltaSeconds);

	float IdleTime = 0.f;

	FRotator BaseMeshRotation = FRotator::ZeroRotator;
	FVector BaseMeshLocation = FVector::ZeroVector;
	float CurrentLean = 0.f;
	float CurrentWallOffset = 0.f;
	float LastYaw = 0.f;
};

