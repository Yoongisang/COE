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
	// ���� �� �� ȸ�� ���� �÷��׿� ��
	bool bHasSavedRotation = false;
	FRotator SavedControlRotation;
	TObjectPtr<class APlayerController> PlayerController;
	
	// ===== AP ����(����) ������ ª�� �Լ��� ����� �ٷ� ���� =====
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

	// CombatManager�� �� ���� ��û(�긮���� ����)
	void RequestEndTurn();

public:
	ATurnPlayer();

public:
	virtual void BeginPlay() override;

public:
	//Ŀ�� ǥ�û��� ������Ʈ
	void UpdateCursor();

public:
	/** ��Ŭ�� Aiming */
	//UFUNCTION(BlueprintCallable, Category = "Action")
	virtual void SetAiming(bool bNewAiming) override;

	// ===== �׼�/��ų =====
	/** Q: �⺻����(�� ����, AP +1) */
	void UseSkill_Q();

	/** �ڽ�Ʈ �Һ��� ��ų(���� �� ��� �� ����) */
	bool UseSkill_WithCost(int32 Cost);

	/** ��Ŭ�� Fire (��Ŭ�� ���� ���¿����� AP 1 �Ҹ�, 0�̸� �ڵ� �� ����) */
	virtual void Fire() override;
};
