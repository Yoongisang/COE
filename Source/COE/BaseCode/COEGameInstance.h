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

	/** �÷��̾ ������ ��� true, ���� ���� ������ ��� false */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bPlayerInitiative = false;

	/** ������ �����Ǿ� ������ �� ��� true */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bPlayerWasDetected = false;

	/** �������� �¸��ߴ��� ���� */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bCombatVictory = false;

	/** ���� �� �÷��̾��� ��ġ */
	UPROPERTY(BlueprintReadOnly, Category = "Return")
	FVector ReturnLocation;

	/** ���� ���� �� ���ƿ� �� �̸� */
	UPROPERTY(BlueprintReadOnly, Category = "Return")
	FName ReturnMapName;

	/** �������� ����� �� �� */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TWeakObjectPtr<AActor> EnemyToRemove;
};
