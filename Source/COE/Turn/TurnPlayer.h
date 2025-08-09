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

private:
	// 조준 전 뷰 회전 저장 플래그와 값
	bool bHasSavedRotation = false;
	FRotator SavedControlRotation;
	TObjectPtr<class APlayerController> PlayerController;
	
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
	/** Q: 기본공격(턴 유지, AP +1) */
	void UseSkill_Q();

	/** 코스트 소비형 스킬(성공 시 즉시 턴 종료) */
	bool UseSkill_WithCost(int32 Cost);

	/** 좌클릭 Fire (우클릭 조준 상태에서만 AP 1 소모, 0이면 자동 턴 종료) */
	virtual void Fire() override;
};
