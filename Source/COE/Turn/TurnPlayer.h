// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCode/COECharacter.h"
#include "TurnPlayer.generated.h"

/**
 * 
 */
UCLASS()
class COE_API ATurnPlayer : public ACOECharacter
{
	GENERATED_BODY()

public:

	TObjectPtr<class UCOEGameInstance> GI;

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

	// CombatManager에 턴 종료 요청(브리지를 통해)
	void RequestEndTurn();

public:
	ATurnPlayer();

public:
	virtual void BeginPlay() override;

public:
	//커서 표시상태 업데이트
	void UpdateCursor();

public:
	/** 우클릭 Aiming */
	//UFUNCTION(BlueprintCallable, Category = "Action")
	virtual void SetAiming(bool bNewAiming) override;

	// ===== 액션/스킬 =====
	/** Q: 기본공격(턴 종료, AP +1) */
	void UseSkill_Q();

	/** Q: 패링(성공 시, AP +2) */
	void Parry();

	/** 좌클릭 Fire (우클릭 조준 상태에서만 AP 1 소모, 0이면 자동 턴 종료) */
	virtual void Fire() override;

	/** 회복 아이템 사용 */
	UFUNCTION(BlueprintCallable, Category = "Consumable|Combat")
	void UseHPPotion();

	/** W: 회피(성공 시, AP +1) */
	void Dodge();

	UFUNCTION(BlueprintCallable, Category = "Consumable|Combat")
	void UseAPPotion();

	/** Enemy Turn 중인지 확인하는 함수 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsEnemyTurnActive() const;

	/** 현재 행동 가능한 상태인지 체크 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool CanPerformAction() const;

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


private:
	/** GI 캐스팅 헬퍼 */
	UCOEGameInstance* GetCOEGameInstance() const;
	/** HP/AP범위 보정 */
	void ClampHPAP();     

	/** 패링 반격 실행 */
	void ExecuteParryCounter(ATurnEnemy* Target);

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
};
