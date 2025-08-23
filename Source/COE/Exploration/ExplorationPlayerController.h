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


protected:

	/** 캐릭터 변경 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ChangeCharacter;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** 입력 매핑 컨텍스트 재설정 */
	void SetupInputMappingContext();

public:

	/** BeginPlay */
	virtual void BeginPlay() override;

public:

	/** 마우스 좌클릭 Input  */
	virtual void DoMouseLeftClick() override;

	/** 캐릭터 변경(T) Input  */
	virtual void DoChangeCharacter() ;

	/** OnPossess Override */
	virtual void OnPossess(APawn* InPawn) override;
};
