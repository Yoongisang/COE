// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCode/COECharacter.h"
#include "TargetSelectionComponent.h"
#include "TurnHudWidget.h"
#include "TurnPlayer.generated.h"

/** 공격 시퀀스의 현재 단계 */
UENUM(BlueprintType)
enum class EAttackSequenceState : uint8
{
	None,           // 아무것도 안함
	MovingToTarget, // 적으로 이동 중
	Attacking,      // 공격 애니메이션 실행 중
	Returning       // 원래 위치로 복귀 중
};

/** 턴 상태 확인용 enum */
UENUM(BlueprintType)
enum class ETurnState : uint8
{
	None,           // 전투 외 상태
	MyTurn,         // 내 턴
	AllyTurn,       // 아군 턴
	EnemyTurn       // 적 턴
};

/**
 * 
 */
UCLASS()
class COE_API ATurnPlayer : public ACOECharacter
{
	GENERATED_BODY()

public:

	TObjectPtr<class UCOEGameInstance> GI;

	/** 타겟 선택 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Targeting", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTargetSelectionComponent> TargetSelector;

	/** 현재 대기 중인 스킬 타입 */
	UPROPERTY()
	ESkillTargetType PendingSkillType = ESkillTargetType::Universal;

	/** 대기 중인 스킬 이름 (디버그용) */
	UPROPERTY()
	FString PendingSkillName;

	/** Turn HUD 위젯 컴포넌트 (캐릭터 앞에 표시) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Turn HUD", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWidgetComponent> TurnHudWidgetComponent;

	/** Turn HUD 위젯 클래스 (블루프린트에서 설정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Turn HUD")
	TSubclassOf<class UTurnHudWidget> TurnHudWidgetClass;

	/** Turn HUD 위젯 인스턴스 */
	UPROPERTY(BlueprintReadOnly, Category = "UI|Turn HUD")
	TObjectPtr<UTurnHudWidget> TurnHudWidget;

	/** 현재 HUD 모드 저장 */
	UPROPERTY()
	ETurnHudMode CurrentHudMode = ETurnHudMode::None;

private:

	// 조준 전 뷰 회전 저장 플래그와 값
	bool bHasSavedRotation = false;
	FRotator SavedControlRotation;
	TObjectPtr<class APlayerController> PlayerController;
	
	/** 현재 방어 행동 중인지 (패링/회피 중) */
	UPROPERTY(BlueprintReadOnly, Category = "Defense", meta = (AllowPrivateAccess = "true"))
	bool bIsDefense = false;

	/** 패링 무적 구간 중인지 */
	UPROPERTY(BlueprintReadOnly, Category = "Defense", meta = (AllowPrivateAccess = "true"))
	bool bIsParryInvincible = false;

	/** 회피 무적 구간 중인지 */
	UPROPERTY(BlueprintReadOnly, Category = "Defense", meta = (AllowPrivateAccess = "true"))
	bool bIsDodgeInvincible = false;

	/** 패링 성공 시 반격할 대상 */
	UPROPERTY()
	TWeakObjectPtr<class ATurnEnemy> ParryCounterTarget;

	// ===== AP 헬퍼(전용) 간단한 짧은 함수라 헤더에 바로 구현 =====
	bool CanSpendAP(int32 Amount) const { return CharacterStats.CurrentAP >= Amount; }
	bool SpendAP(int32 Amount)
	{
		if (!CanSpendAP(Amount)) return false;
		CharacterStats.CurrentAP = FMath::Clamp(CharacterStats.CurrentAP - Amount, 0, CharacterStats.MAXAP);
		return true;
	}
	void GainAP(int32 Amount)
	{
		CharacterStats.CurrentAP = FMath::Clamp(CharacterStats.CurrentAP + Amount, 0, CharacterStats.MAXAP);
	}

public:

	ATurnPlayer();

public:
	virtual void BeginPlay() override;

public:
	//커서 표시상태 업데이트
	void UpdateCursor();

	/** 공격 애니메이션이 끝났을 때 AnimInstance에서 호출할 함수 */
	void OnAttackMontageEnded();

public:
	/** 우클릭 Aiming */
	//UFUNCTION(BlueprintCallable, Category = "Action")
	virtual void SetAiming(bool bNewAiming) override;

	// ===== 액션/스킬 =====
	/** Q: 기본공격(턴 종료, AP +1) */
	void UseSkill_Q();

	/** W: 스킬 (턴 종료, AP - 2) */
	void UseSkill_W();

	/** E: 스킬 (턴 종료, AP - 3) */
	void UseSkill_E();

	/** A: HP 물약 사용 */
	void UseSkill_A();

	/** S: AP 물약 사용 */
	void UseSkill_S();

	/** D: 턴 넘기기 */
	void UseSkill_D();

	/** 타겟 선택 완료 후 스킬 실행 */
	UFUNCTION()
	void ExecuteSkillOnTarget(ACOECharacter* TargetCharacter, ESkillTargetType SkillType);

	/** 타겟 선택 취소 시 호출 */
	UFUNCTION()
	void OnTargetSelectionCancelled();

	/** 현재 타겟 선택 중인지 확인 */
	UFUNCTION(BlueprintPure, Category = "Combat|Targeting")
	bool IsSelectingTarget() const;

	/** Q: 패링(성공 시, AP +2) */
	void Parry();

	/** 좌클릭 Fire (우클릭 조준 상태에서만 AP 1 소모, 0이면 자동 턴 종료) */
	virtual void Fire() override;

	/** W: 회피(성공 시, AP +1) */
	void Dodge();

	//UFUNCTION(BlueprintCallable, Category = "Consumable|Combat")
	//void UseAPPotion();

	/** Enemy Turn 중인지 확인하는 함수 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsEnemyTurnActive() const;

	/** 현재 행동 가능한 상태인지 체크 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool CanPerformAction() const;

	/** HUD용 - 로그 없이 내 턴인지만 체크 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsMyTurnActive() const;

	/** 현재 턴 상태를 안전하게 확인 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	ETurnState GetCurrentTurnState() const;

	/** 방어 행동 가능한지 체크 (Enemy 턴 중이고 방어 중이 아닐 때) */
	UFUNCTION(BlueprintPure, Category = "Defense")
	bool CanPerformDefense() const;

	/** 무적 상태인지 확인 (패링/회피 무적 구간) */
	UFUNCTION(BlueprintPure, Category = "Defense")
	bool IsInvincible() const { return bIsParryInvincible || bIsDodgeInvincible; }

	/** 패링 구간 시작 (AnimNotify에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Defense")
	void StartParryInvincibility();

	/** 패링 구간 종료 (AnimNotify에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Defense")
	void EndParryInvincibility();

	/** 회피 구간 시작 (AnimNotify에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Defense")
	void StartDodgeInvincibility();

	/** 회피 구간 종료 (AnimNotify에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Defense")
	void EndDodgeInvincibility();

	/** 패링/회피 애니메이션 완전 종료 (AnimNotify에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Defense")
	void EndDefenseAction();

	/** 패링 성공 시 호출 (적이 공격했을 때) */
	UFUNCTION(BlueprintCallable, Category = "Defense")
	void OnParrySuccess(ATurnEnemy* Attacker);

	/** 회피 성공 시 호출 (적이 공격했을 때) */
	UFUNCTION(BlueprintCallable, Category = "Defense")
	void OnDodgeSuccess(ATurnEnemy* Attacker);

	// ===== TakeDamage 오버라이드 (무적 처리) =====
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// CombatManager에 턴 종료 요청(브리지를 통해)
	void RequestEndTurn();

	/** Turn HUD 표시/숨김 */
	UFUNCTION(BlueprintCallable, Category = "UI|Turn HUD")
	void SetTurnHudVisible(bool bVisible);

	/** Turn HUD 모드 변경 */
	UFUNCTION(BlueprintCallable, Category = "UI|Turn HUD")
	void SetTurnHudMode(ETurnHudMode Mode);

	/** Turn HUD 강제 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "UI|Turn HUD")
	void RefreshTurnHud();

private:
	/** GI 캐스팅 헬퍼 */
	UCOEGameInstance* GetCOEGameInstance() const;
	/** HP/AP범위 보정 */
	void ClampHPAP();     

	/** 패링 반격 실행 */
	void ExecuteParryCounter(ATurnEnemy* Target);

	// ===== 공격 시퀀스 관리를 위한 변수들 =====
	/** 현재 공격 시퀀스의 상태 */
	EAttackSequenceState CurrentAttackState = EAttackSequenceState::None;

	/** 공격 시작 전 원래 위치와 회전값 */
	FVector OriginalLocation;
	FRotator OriginalRotation;

	/** 현재 공격 대상 */
	TWeakObjectPtr<ACOECharacter> CurrentAttackTarget;

	/** 이동 보간을 처리할 타이머 핸들 */
	FTimerHandle MovementTimerHandle;

	/** 적과의 공격 거리 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackDistance = 150.0f;

	/** 이동 속도 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float MovementSpeed = 600.0f;

	// ===== 새로운 private 함수들 =====

	/** 이동을 업데이트하는 타이머 함수 */
	void UpdateAttackMovement();

protected:

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	/** 타겟 선택 컴포넌트 초기화 */
	void InitializeTargetSelector();

	/** 스킬별 실제 실행 함수들 */
	void ExecuteBasicAttack(ACOECharacter* Target);
	void ExecuteSkillW(ACOECharacter* Target);
	void ExecuteSkillE(ACOECharacter* Target);
	void ExecuteSkillA(ACOECharacter* Target);
	void ExecuteSkillS(ACOECharacter* Target);

	/** Turn HUD 초기화 */
	void InitializeTurnHud();

public:

	/** 턴 상태 변경 시 HUD 업데이트 */
	void UpdateHudForTurnState();


};
