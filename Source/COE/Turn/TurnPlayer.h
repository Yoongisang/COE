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
	// 조준 전 뷰 회전 저장 플래그와 값
	bool bHasSavedRotation = false;
	FRotator SavedControlRotation;
	TObjectPtr<class APlayerController> PlayerController;
	
public:
	ATurnPlayer();

public:
	virtual void BeginPlay() override;

public:
	//커서 표시상태 업데이트
	void UpdateCursor();

public:
	/** 우클릭 Aiming */
	//UFUNCTION(BlueprintCallable, Category = "Action")
	virtual void SetAiming(bool bNewAiming) override;
};
