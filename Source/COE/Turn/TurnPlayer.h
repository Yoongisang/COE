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
	// ���� �� �� ȸ�� ���� �÷��׿� ��
	bool bHasSavedRotation = false;
	FRotator SavedControlRotation;
	TObjectPtr<class APlayerController> PlayerController;
	
public:
	ATurnPlayer();

public:
	virtual void BeginPlay() override;

public:
	//Ŀ�� ǥ�û��� ������Ʈ
	void UpdateCursor();

public:
	/** ��Ŭ�� Aiming */
	//UFUNCTION(BlueprintCallable, Category = "Action")
	virtual void SetAiming(bool bNewAiming) override;
};
