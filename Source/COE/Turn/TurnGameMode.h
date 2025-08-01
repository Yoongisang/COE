// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCode/COEGameMode.h"
#include "TurnGameMode.generated.h"

/**
 * 
 */
UCLASS()
class COE_API ATurnGameMode : public ACOEGameMode
{
	GENERATED_BODY()


public:
	virtual void BeginPlay() override;
	// ���� ���� ������ Enemy Ŭ�������� ȣ��
	void OnEnemyDied();  

private:
	//�����ִ� �� ��
	int32 RemainingEnemies = 0;
	//�����ִ� �� �� Ȯ�� �� ��� óġ�� Level�̵�
	void CheckEndOfCombat();
};
