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
	// 적이 죽을 때마다 Enemy 클래스에서 호출
	void OnEnemyDied();  

private:
	//남아있는 적 수
	int32 RemainingEnemies = 0;
	//남아있는 적 수 확인 및 모두 처치시 Level이동
	void CheckEndOfCombat();
};
