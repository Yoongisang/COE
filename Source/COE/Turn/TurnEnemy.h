// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCode/COECharacter.h"
#include "TurnEnemy.generated.h"

// Delegate 타입 선언
DECLARE_MULTICAST_DELEGATE(FOnTurnEnemyDead)

/** Enemy 공격 시퀀스 상태 */
UENUM(BlueprintType)
enum class EEnemyAttackState : uint8
{
    None,           // 아무것도 안함
    RotatingToTarget,  // 타겟 방향으로 회전 중
    MovingToTarget,    // 플레이어에게 이동 중
    Attacking,         // 공격 애니메이션 실행 중
    RotatingToOrigin,  // 원래 위치 방향으로 회전 중
    ReturningToOrigin, // 원래 위치로 복귀 중
    RotatingToOriginalDirection, // 원래 바라보던 방향으로 회전 중
    PerformingSkill    // 스킬 실행 중 (제자리에서 레이저빔)
};

/**
 * 
 */
UCLASS()
class COE_API ATurnEnemy : public ACOECharacter
{
	GENERATED_BODY()

public:
    ATurnEnemy();

public:
    virtual void BeginPlay() override;

public:

    /** 죽었을 때 브로드캐스트되는 이벤트 */
    FOnTurnEnemyDead OnDead;

    /** 지정된 타겟 (BridgeComponent에서 설정) */
    UPROPERTY()
    TWeakObjectPtr<ACOECharacter> AssignedTarget;

    /** 스킬 사용 중 저장할 타겟 위치 */
    UPROPERTY()
    FVector SkillTargetLocation;

public:
    /** 데미지를 받는 함수 (예시) */
    virtual float TakeDamage(float DamageAmount,struct FDamageEvent const& DamageEvent,AController* EventInstigator, AActor* DamageCauser) override;

public:
    /** Enemy 턴에서 자동으로 행동을 수행하는 함수 */
    UFUNCTION(BlueprintCallable, Category = "Combat|AI")
    void ExecuteEnemyTurn();

    /** 지정된 타겟으로 턴 실행 */
    UFUNCTION(BlueprintCallable, Category = "Combat|AI")
    void ExecuteEnemyTurnWithTarget(ACOECharacter* SpecificTarget);

    /** 현재 자신의 턴인지 확인하는 함수 */
    UFUNCTION(BlueprintPure, Category = "Combat")
    bool IsMyTurnActive() const;

    /** 현재 공격 상태 반환 (AnimInstance에서 사용) */
    UFUNCTION(BlueprintPure, Category = "Combat")
    EEnemyAttackState GetCurrentAttackState() const { return CurrentAttackState; }

    /** 기본공격 오버라이드 (Enemy 전용) */
    virtual void DefaultAttack() override;

    /** 지정된 타겟으로 기본공격 */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void DefaultAttackToTarget(ACOECharacter* SpecificTarget);

    /** Enemy 턴 종료 처리 */
    void FinishEnemyTurn();

    /** 공격 애니메이션이 끝났을 때 AnimInstance에서 호출할 함수 */
    void OnAttackMontageEnded();

    /** 스킬 애니메이션이 끝났을 때 AnimInstance에서 호출할 함수 */
    void OnSkillMontageEnded();

    /** 타겟 설정 함수 (Bridge에서 호출) */
    UFUNCTION(BlueprintCallable, Category = "Combat|Targeting")
    void SetAssignedTarget(ACOECharacter* Target);

    // === 스킬 AnimNotify 콜백 함수들 ===

    /** Fire 단계: 발사 위치에 FX 스폰 */
    UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
    void OnSkillFire();

    /** Fire_Spawn 단계: 플레이어 위치에 Impact 스폰 + 데미지 적용 */
    UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
    void OnSkillFireSpawn();

    /** Fire_End 단계: 원래 방향으로 회전 시작 */
    UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
    void OnSkillFireEnd();

private:
    /** Enemy 공격 로직 */
    void PerformEnemyAttack();

    /** 스킬 공격 함수 */
    void PerformSkill();

    /** 타겟 플레이어 찾기 */
    ACOECharacter* FindTargetPlayer();

    /** 공격 시퀀스 시작 */
    void StartAttackSequence(ACOECharacter* Target);

    /** 이동 시퀀스 업데이트 */
    void UpdateAttackMovement();

    /** 스킬 시퀀스 시작 (제자리에서 수행) */
    void StartSkillSequence(ACOECharacter* Target);

    /** 스킬 회전 업데이트 (타겟 방향으로) */
    void UpdateSkillRotation();

    /** 스킬 종료 회전 업데이트 (원래 방향으로) */
    void UpdateSkillEndRotation();

private:
    /** 현재 공격 시퀀스 상태 */
    UPROPERTY()
    EEnemyAttackState CurrentAttackState = EEnemyAttackState::None;

    /** 공격 대상 */
    UPROPERTY()
    TWeakObjectPtr<ACOECharacter> CurrentAttackTarget;

    /** 원래 위치 */
    UPROPERTY()
    FVector OriginalLocation;

    /** 원래 회전 */
    UPROPERTY()
    FRotator OriginalRotation;

    /** 타겟 회전 */
    UPROPERTY()
    FRotator TargetRotation;

    /** 이동 업데이트용 타이머 */
    UPROPERTY()
    FTimerHandle MovementTimerHandle;

    /** 회전 속도 */
    UPROPERTY(EditAnywhere, Category = "Combat|Movement")
    float RotationSpeed = 3.0f;

    /** 공격 거리 */
    UPROPERTY(EditAnywhere, Category = "Combat|Movement")
    float AttackDistance = 200.0f;

    /** 이동 속도 */
    UPROPERTY(EditAnywhere, Category = "Combat|Movement")
    float MovementSpeed = 400.0f;

    /** 스킬 회전 속도 (레이저빔용) */
    UPROPERTY(EditAnywhere, Category = "Combat|Skill")
    float SkillRotationSpeed = 5.0f;

    /** 레이저빔 데미지 */
    UPROPERTY(EditAnywhere, Category = "Combat|Skill")
    float LaserBeamDamage = 30.0f;

};
