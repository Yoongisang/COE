// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FCharacterStats.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FCharacterStats
{
	GENERATED_BODY()

public:
	
	/** MAXHP */
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	float MAXHP;

	/** HP */
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	float CurrentHP;

	/** Vitality */
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	float Vitality;

	/** AttackPower */
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	float AttackPower;

	/** Defense */
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	float Defense;

	/** Agility */
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	float Agility;

	/** Luck */
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	float Luck;
};