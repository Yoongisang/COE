// Fill out your copyright notice in the Description page of Project Settings.


#include "AimUIWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CanvasPanel.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "COECharacter.h"

UAimUIWidget::UAimUIWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bIsVisible = false;

    // 기본적으로 숨김 상태로 시작
    SetVisibility(ESlateVisibility::Hidden);
}

void UAimUIWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 위젯을 화면 중앙에 배치
    CenterWidgetOnScreen();

    // 초기에는 숨김 상태
    SetVisibility(ESlateVisibility::Hidden);
    bIsVisible = false;

    UE_LOG(LogTemp, Log, TEXT("[AimUIWidget] Widget constructed and centered"));
}

void UAimUIWidget::ShowAimUI()
{
    UE_LOG(LogTemp, Log, TEXT("[AimUIWidget] Showing Aim UI"));

    bIsVisible = true;

    // 위젯을 중앙에 다시 배치
    CenterWidgetOnScreen();

    // 즉시 표시
    SetVisibility(ESlateVisibility::Visible);

}

void UAimUIWidget::HideAimUI()
{
    if (!bIsVisible)
    {
        return; // 이미 숨겨져 있거나 애니메이션 중
    }

    UE_LOG(LogTemp, Log, TEXT("[AimUIWidget] Hiding Aim UI"));

    bIsVisible = false;

    SetVisibility(ESlateVisibility::Hidden);
}


void UAimUIWidget::CenterWidgetOnScreen()
{
    // CanvasPanelSlot을 통해 화면 중앙에 배치
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
    {
        // 앵커를 중앙으로 설정 (0.5, 0.5)
        CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));

        // 정렬을 중앙으로 설정
        CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));

        // 위치를 (0, 0)으로 설정 (앵커 기준 중앙)
        CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));

        // 크기 자동 설정 (필요에 따라 조정)
        CanvasSlot->SetSize(FVector2D(100.0f, 100.0f)); // 기본 크기

        UE_LOG(LogTemp, Log, TEXT("[AimUIWidget] Widget centered on screen"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[AimUIWidget] Could not cast to CanvasPanelSlot - widget may not be centered"));
    }
}
