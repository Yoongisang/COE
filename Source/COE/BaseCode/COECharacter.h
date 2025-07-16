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

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
//USTRUCT(BlueprintType)
//struct FCharacterStats
//{
//	GENERATED_BODY()
//public:
//	/** HPMAX */
//	UPROPERTY(BlueprintReadOnly, Category = "Status")
//	float HPMAX;
//
//	/** HP */
//	UPROPERTY(BlueprintReadOnly, Category = "Status")
//	float CurrentHP;
//
//	/** Vitality */
//	UPROPERTY(BlueprintReadOnly, Category = "Status")
//	float Vitality;
//
//	/** AttackPower */
//	UPROPERTY(BlueprintReadOnly, Category = "Status")
//	float AttackPower;
//
//	/** Defense */
//	UPROPERTY(BlueprintReadOnly, Category = "Status")
//	float Defense;
//
//	/** Agility */
//	UPROPERTY(BlueprintReadOnly, Category = "Status")
//	float Agility;
//
//	/** Luck */
//	UPROPERTY(BlueprintReadOnly, Category = "Status")
//	float Luck;
//};
UCLASS(abstract)
class ACOECharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimInstance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCOEAnimInstance> AnimInstance;

	/** 조준 상태 전환 시 보간을 위한 변수 */
	FTimerHandle AimingInterpTimerHandle;
	FVector StartSocketOffset;
	FVector TargetSocketOffset;
	float InterpAlpha = 0.f;
	bool bInterpToAiming = false;

public:

	/** Constructor */
	ACOECharacter();	

	/** If true, the character is currently playing an attack animation */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")

	bool bIsAttacking = false;
	/** 조준 중인지 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsAiming = false;
	/** 발사 했는지 */
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

	/** 기본공격*/
	UFUNCTION(BlueprintCallable, Category = "Action")
	void DefaultAttack();

	/** 기본공격 충돌처리 */
	UFUNCTION(BlueprintCallable, Category = "Action")
	void DoDefaultAttack();

	/** 좌클릭 Fire */
	UFUNCTION(BlueprintCallable, Category = "Action")
	void Fire();

	/** 우클릭 Aiming */
	UFUNCTION(BlueprintCallable, Category = "Action")
	void SetAiming(bool bNewAiming);

	/**  받은 데미지 처리 */
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void UpdateAimingInterp();
};

