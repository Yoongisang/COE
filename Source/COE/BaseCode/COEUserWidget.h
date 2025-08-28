// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "COEUserWidget.generated.h"

class ACOECharacter;

/**
 * 
 */
UCLASS()
class COE_API UCOEUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UCOEUserWidget(const FObjectInitializer& ObjectInitializer);

protected:

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:

	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HP_ProgressBar;

public:
	/** 특정 캐릭터의 HP와 바인딩 */
	UFUNCTION(BlueprintCallable, Category = "HP Widget")
	void BindToCharacter(ACOECharacter* Character);

	/** HP 값 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "HP Widget")
	void UpdateHp();

	/** 위젯 표시/숨김 제어 */
	UFUNCTION(BlueprintCallable, Category = "HP Widget")
	void SetWidgetVisible(bool bVisible);

	/** HP 바 색상 변경 (HP 비율에 따라) */
	UFUNCTION(BlueprintCallable, Category = "HP Widget")
	void UpdateHPBarColor();

protected:

	/** 바인딩된 캐릭터 참조 */
	UPROPERTY()
	TWeakObjectPtr<ACOECharacter> BoundCharacter;

	/** 현재 HP 비율 캐시 */
	UPROPERTY()
	float CurrentHPRatio = 1.0f;

	/** 위젯이 표시될지 여부 */
	UPROPERTY()
	bool bShouldBeVisible = true;

private:
	/** HP 바 색상 설정 */
	void SetProgressBarColor(const FLinearColor& Color);

	/** 캐릭터 유효성 검사 */
	bool IsCharacterValid() const;

};
