// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "COEGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class COE_API UCOEGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	/** 플레이어가 선공한 경우 true, 적이 먼저 감지한 경우 false */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bPlayerInitiative = false;

	/** 적에게 감지되어 전투에 들어간 경우 true */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bPlayerWasDetected = false;

	/** 전투에서 승리했는지 여부 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bCombatVictory = false;

	/** 전투 전 플레이어의 위치 */
	UPROPERTY(BlueprintReadOnly, Category = "Return")
	FVector ReturnLocation;

	/** 전투 종료 후 돌아올 맵 이름 */
	UPROPERTY(BlueprintReadOnly, Category = "Return")
	FName ReturnMapName;

	/** 전투에서 대상이 된 적 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TWeakObjectPtr<AActor> EnemyToRemove;
};
