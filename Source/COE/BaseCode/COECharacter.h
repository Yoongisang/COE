// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "FCharacterStats.h"
#include "COECharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UAimUIWidget;

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

	/** HP 위젯 컴포넌트 (캐릭터 머리 위에 표시) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|HP Widget", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWidgetComponent> HPWidgetComponent;


protected:

	/** 조준 상태 전환 시 보간을 위한 변수 */
	FTimerHandle AimingInterpTimerHandle;
	FVector StartSocketOffset;
	FVector TargetSocketOffset;
	float InterpAlpha = 0.f;
	bool bInterpToAiming = false;

public:

	/** Constructor */
	ACOECharacter();	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimInstance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCOEAnimInstance> AnimInstance;

	/** CombatBridge 블루프린트 바인딩 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Bridge", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UTurnCombatBridgeComponent> TurnBridge;

	/** If true, the character is currently playing an attack animation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")

	bool bIsAttacking = false;
	/** 조준 중인지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsAiming = false;
	/** 발사 했는지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsShooting = false;

	/** SocketLocation */
	UPROPERTY(BlueprintReadOnly, Category = "Socket")
	FVector SocketLocation;

	/** SocketRotation */
	UPROPERTY(BlueprintReadOnly, Category = "Socket")
	FRotator SocketRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	FCharacterStats CharacterStats;

	/** 발사 위치 파티클 (RangedSocket) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	class UParticleSystem* MuzzleFlashParticle;

	/** 충돌 지점 파티클 (ImpactPoint) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	class UParticleSystem* ImpactParticle;

	/** 기본공격 파티클 (DefaultAttackSocket) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	class UParticleSystem* DefaultAttackParticle;

	/** Heal 파티클 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	class UParticleSystem* HealParticle;

	/** Cost스킬 파티클 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	class UParticleSystem* SkillParticle;

	/** HP 위젯 클래스 (블루프린트에서 설정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|HP Widget")
	TSubclassOf<class UCOEUserWidget> HPWidgetClass;

	/** HP 위젯 인스턴스 */
	UPROPERTY(BlueprintReadOnly, Category = "UI|HP Widget")
	TObjectPtr<UCOEUserWidget> HPWidget;

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** 조준 UI 위젯 클래스 (블루프린트에서 설정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Aim")
	TSubclassOf<UAimUIWidget> AimUIWidgetClass;

	/** 조준 UI 위젯 인스턴스 */
	UPROPERTY(BlueprintReadOnly, Category = "UI|Aim")
	TObjectPtr<UAimUIWidget> AimUIWidget;

public:

	virtual void BeginPlay() override;

	/** 기본공격 */
	UFUNCTION(BlueprintCallable, Category = "Action")
	virtual void DefaultAttack();

	/** 기본공격 충돌처리 */
	UFUNCTION(BlueprintCallable, Category = "Action")
	void DoDefaultAttack();

	/** 전투 진입 트랜지션 (슬로우모션 + 지연 레벨 전환) */
	UFUNCTION(BlueprintCallable, Category = "Action")
	void StartCombatTransition(AExplorationEnemy* Enemy, FName BattleMapName, bool bPlayerInitiative = true);

	/** 좌클릭 Fire */
	UFUNCTION(BlueprintCallable, Category = "Action")
	virtual void Fire();

	/** 우클릭 Aiming */
	UFUNCTION(BlueprintCallable, Category = "Action")
	virtual void SetAiming(bool bNewAiming);

	/**  받은 데미지 처리 */
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	void UpdateAimingInterp();

	/** 원거리 공격 파티클 스폰 함수 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SpawnRangedEmitter(FVector TargetLocation);

	/** 기본공격 파티클 스폰 함수 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SpawnDefaultAttackEmitter();

	/** Heal 파티클 스폰 함수 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SpawnHealEmitter(ACOECharacter* Target);

	/** Skill 파티클 스폰 함수 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SpawnSkillEmitter(ACOECharacter* Target);
	protected:

	/** 조준 UI 위젯 초기화 */
	void InitializeAimUIWidget();

	/** 조준 UI 표시 */
	UFUNCTION(BlueprintCallable, Category = "UI|Aim")
	void ShowAimUI();

	/** 조준 UI 숨김 */
	UFUNCTION(BlueprintCallable, Category = "UI|Aim")
	void HideAimUI();

	/** HP 위젯 표시/숨김 */
	UFUNCTION(BlueprintCallable, Category = "UI|HP Widget")
	void SetHPWidgetVisible(bool bVisible);

	/** HP 위젯 강제 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "UI|HP Widget")
	void RefreshHPWidget();

	protected:

	/** HP 위젯 초기화 */
	void InitializeHPWidget();

	/** HP가 변경될 때 호출 (기존 TakeDamage에서 호출) */
	void OnHPChanged();



};

