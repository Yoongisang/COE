// Fill out your copyright notice in the Description page of Project Settings.


#include "COEUserWidget.h"
#include "COECharacter.h"
#include "Components/ProgressBar.h"
#include "Kismet/KismetTextLibrary.h"

UCOEUserWidget::UCOEUserWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 기본값 설정
    CurrentHPRatio = 1.0f;
    bShouldBeVisible = true;

}

void UCOEUserWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 초기 HP 바 설정
    if (HP_ProgressBar)
    {
        HP_ProgressBar->SetPercent(CurrentHPRatio);
        UpdateHPBarColor();
    }

    UE_LOG(LogTemp, Log, TEXT("[COEUserWidget] Widget constructed"));
}

void UCOEUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 바인딩된 캐릭터가 유효한 경우에만 업데이트
    if (IsCharacterValid())
    {
        UpdateHp();
    }
    else if (BoundCharacter.IsValid() == false)
    {
        // 캐릭터가 파괴되었으면 위젯 숨김
        SetWidgetVisible(false);
    }
}

void UCOEUserWidget::BindToCharacter(ACOECharacter* Character)
{
    if (!IsValid(Character))
    {
        UE_LOG(LogTemp, Warning, TEXT("[COEUserWidget] Trying to bind to invalid character"));
        return;
    }

    BoundCharacter = Character;  // 캐릭터 바인딩 추가

    // 초기 HP 업데이트
    UpdateHp();
    SetWidgetVisible(true);

    UE_LOG(LogTemp, Log, TEXT("[COEUserWidget] Bound to character: %s"), *Character->GetName());
}

void UCOEUserWidget::UpdateHp()
{
    if (!IsCharacterValid() || !HP_ProgressBar)
    {
        return;
    }

    ACOECharacter* Character = BoundCharacter.Get();
    const FCharacterStats& Stats = Character->CharacterStats;

    // HP 비율 계산
    float NewHPRatio = (Stats.MAXHP > 0.0f) ? (Stats.CurrentHP / Stats.MAXHP) : 0.0f;
    NewHPRatio = FMath::Clamp(NewHPRatio, 0.0f, 1.0f);

    // 변경된 경우에만 업데이트
    if (!FMath::IsNearlyEqual(CurrentHPRatio, NewHPRatio, 0.01f))
    {
        CurrentHPRatio = NewHPRatio;
        HP_ProgressBar->SetPercent(CurrentHPRatio);
        UpdateHPBarColor();
    }
}

void UCOEUserWidget::SetWidgetVisible(bool bVisible)
{
    bShouldBeVisible = bVisible;

    if (bVisible)
    {
        SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        SetVisibility(ESlateVisibility::Hidden);
    }
}

void UCOEUserWidget::UpdateHPBarColor()
{
    if (!HP_ProgressBar)
    {
        return;
    }

    // HP 비율에 따른 색상 변경
    FLinearColor BarColor;

    if (CurrentHPRatio > 0.6f)
    {
        // 높은 HP: 녹색
        BarColor = FLinearColor(0.0f, 0.8f, 0.0f, 1.0f); // 밝은 녹색
    }
    else if (CurrentHPRatio > 0.3f)
    {
        // 중간 HP: 노란색  
        BarColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f); // 노란색
    }
    else
    {
        // 낮은 HP: 빨간색
        BarColor = FLinearColor(0.8f, 0.0f, 0.0f, 1.0f); // 빨간색
    }

    // 색상 적용
    HP_ProgressBar->SetFillColorAndOpacity(BarColor);
}

void UCOEUserWidget::SetProgressBarColor(const FLinearColor& Color)
{
    if (!HP_ProgressBar)
    {
        return;
    }
    
    // 프로그레스 바의 색상 스타일 설정
    HP_ProgressBar->SetFillColorAndOpacity(Color);

}

bool UCOEUserWidget::IsCharacterValid() const
{
    return BoundCharacter.IsValid() && IsValid(BoundCharacter.Get());
}