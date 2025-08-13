// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCode/COEPlayerController.h"
#include "ExplorationPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class COE_API AExplorationPlayerController : public ACOEPlayerController
{
	GENERATED_BODY()
	
public:

	/** ExplorationPlayer */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class AExplorationPlayer> ExplorationChar;

public:

	/** BeginPlay */
	virtual void BeginPlay() override;

public:

	/** 마우스 좌클릭 Input  */
	virtual void DoMouseLeftClick() override;
};
