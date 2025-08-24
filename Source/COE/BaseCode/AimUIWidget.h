// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AimUIWidget.generated.h"

/**
 * 
 */
UCLASS()
class COE_API UAimUIWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UAimUIWidget(const FObjectInitializer& ObjectInitializer);

protected:

	virtual void NativeConstruct() override;

public:

	/** 조준 UI 표시 */
	UFUNCTION(BlueprintCallable, Category = "Aim UI")
	void ShowAimUI();

	/** 조준 UI 숨김 */
	UFUNCTION(BlueprintCallable, Category = "Aim UI")
	void HideAimUI();

protected:

	/** 조준 이미지 (블루프린트에서 바인딩) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Aim UI")
	class UImage* Aim;

private:

	/** 현재 표시 상태 */
	UPROPERTY()
	bool bIsVisible = false;

private:

	/** 위젯을 화면 중앙으로 이동 */
	void CenterWidgetOnScreen();
};
