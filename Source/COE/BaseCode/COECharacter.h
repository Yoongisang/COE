// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "FCharacterStats.h"
#include "COECharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

//DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(abstract)
class ACOECharacter : public ACharacter
{
	GENERATED_BODY()

protected:

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimInstance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCOEAnimInstance> AnimInstance;

	/** ���� ���� ��ȯ �� ������ ���� ���� */
	FTimerHandle AimingInterpTimerHandle;
	FVector StartSocketOffset;
	FVector TargetSocketOffset;
	float InterpAlpha = 0.f;
	bool bInterpToAiming = false;

public:

	/** Constructor */
	ACOECharacter();	

	/** CombatBridge �������Ʈ ���ε� */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Bridge", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UTurnCombatBridgeComponent> TurnBridge;

	/** If true, the character is currently playing an attack animation */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")

	bool bIsAttacking = false;
	/** ���� ������ */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsAiming = false;
	/** �߻� �ߴ��� */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsShooting = false;

	/** SocketLocation */
	UPROPERTY(BlueprintReadOnly, Category = "Socket")
	FVector SocketLocation;

	/** SocketRotation */
	UPROPERTY(BlueprintReadOnly, Category = "Socket")
	FRotator SocketRotation;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FCharacterStats CharacterStats;

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

public:

	virtual void BeginPlay() override;

	/** �⺻���� */
	UFUNCTION(BlueprintCallable, Category = "Action")
	void DefaultAttack();

	/** �⺻���� �浹ó�� */
	UFUNCTION(BlueprintCallable, Category = "Action")
	void DoDefaultAttack();

	/** ��Ŭ�� Fire */
	UFUNCTION(BlueprintCallable, Category = "Action")
	void Fire();

	/** ��Ŭ�� Aiming */
	UFUNCTION(BlueprintCallable, Category = "Action")
	virtual void SetAiming(bool bNewAiming);

	/**  ���� ������ ó�� */
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	void UpdateAimingInterp();

};

