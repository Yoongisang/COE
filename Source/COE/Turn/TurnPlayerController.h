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

	/** Q ��ų �Է� */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UInputAction> QSkillAction;

	/** W ��ų �Է� */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UInputAction> WSkillAction;

	/** E ��ų �Է� */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UInputAction> ESkillAction;

	/** A ��ų �Է� */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UInputAction> ASkillAction;

	/** S ��ų �Է� */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UInputAction> SSkillAction;

	/** D ��ų �Է� */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UInputAction> DSkillAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MouseLookAction;

	/** ���콺 ��Ŭ�� */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MouseLeftClick;

	/** ���콺 ��Ŭ�� */
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

	/** Q ��ų */
	virtual void OnSkillQ();

	/** W ��ų */
	virtual void OnSkillW();

	/** E ��ų */
	virtual void OnSkillE();

	/** A ��ų */
	virtual void OnSkillA();

	/** S ��ų */
	virtual void OnSkillS();

	/** D ��ų */
	virtual void OnSkillD();

	/** ���콺 ��Ŭ�� Input  */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMouseLeftClick();

	/** ���콺 ��Ŭ�� Input  */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMouseRightClickStart();

	/** ���콺 ��Ŭ�� End  */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMouseRightClickEnd();
};
