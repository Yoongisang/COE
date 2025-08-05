// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCode/COECharacter.h"
#include "ExplorationPlayer.generated.h"

/**
 * 
 */
UCLASS()
class COE_API AExplorationPlayer : public ACOECharacter
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;

};