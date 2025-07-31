// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TurnPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * 
 */
UCLASS(abstract)
class COE_API ATurnPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** TunrPlayer */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class ATurnPlayer> TurnChar;

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TArray<UInputMappingContext*> DefaultMappingContexts;

protected:

	/** Q 스킬 입력 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UInputAction> QSkillAction;

	/** W 스킬 입력 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UInputAction> WSkillAction;

	/** E 스킬 입력 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UInputAction> ESkillAction;

	/** A 스킬 입력 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UInputAction> ASkillAction;

	/** S 스킬 입력 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UInputAction> SSkillAction;

	/** D 스킬 입력 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UInputAction> DSkillAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MouseLookAction;

	/** 마우스 좌클릭 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MouseLeftClick;

	/** 마우스 우클릭 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MouseRightClick;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

protected:
	/** Called for looking input */
	void Look(const FInputActionValue& Value);
public:

	/** BeginPlay */
	virtual void BeginPlay() override;

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Q 스킬 */
	virtual void OnSkillQ();

	/** W 스킬 */
	virtual void OnSkillW();

	/** E 스킬 */
	virtual void OnSkillE();

	/** A 스킬 */
	virtual void OnSkillA();

	/** S 스킬 */
	virtual void OnSkillS();

	/** D 스킬 */
	virtual void OnSkillD();

	/** 마우스 좌클릭 Input  */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMouseLeftClick();

	/** 마우스 우클릭 Input  */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMouseRightClickStart();

	/** 마우스 우클릭 End  */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMouseRightClickEnd();
};
