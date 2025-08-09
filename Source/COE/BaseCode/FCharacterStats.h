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
	
	/** 생성자에서 기본 스탯 설정 */
	FCharacterStats() :
		MAXHP(100.f), 
		CurrentHP(MAXHP),
		Vitality(0.f),
		AttackPower(0.f),
		Defense(0.f),
		Agility(0.f),
		Luck(0.f),
		MAXAP(10),
		CurrentAP(3)
	{
	}

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

	/** MAX ActionPoint */
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	int32 MAXAP;

	/** Current ActionPoint */
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	int32 CurrentAP;
};