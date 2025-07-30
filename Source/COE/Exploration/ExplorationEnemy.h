// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCode/COECharacter.h"
#include "ExplorationEnemy.generated.h"

/**
 * 
 */
UCLASS()
class COE_API AExplorationEnemy : public ACOECharacter
{
	GENERATED_BODY()
public:
	//TrunLevel �Ҵ�
	UPROPERTY(EditAnywhere, Category = "Combat")
	TArray<FName> PossibleBattleLevels;
};
