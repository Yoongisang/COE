// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnHudWidget.h"
#include "TurnPlayer.h"
#include "BaseCode/COEGameInstance.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"

static void SetVis(UWidget* W, bool bShow) {
    if (W) W->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

UTurnHudWidget::UTurnHudWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    CurrentHudMode = ETurnHudMode::None;
}

void UTurnHudWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // ───────── 누락 위젯 로깅 ─────────
#define REQ(Ptr,Name) if(!(Ptr)) UE_LOG(LogTemp, Warning, TEXT("[TurnHUD] Missing bind: %s (%s)"), TEXT(Name), *GetName());
    REQ(Q_Button, "Q_Button");    REQ(Q_Text, "Q_Text");
    REQ(W_Button, "W_Button");    REQ(W_Text, "W_Text");
    REQ(E_Button, "E_Button");    REQ(E_Text, "E_Text");
    REQ(A_Button, "A_Button");    REQ(A_Text, "A_Text");
    REQ(S_Button, "S_Button");    REQ(S_Text, "S_Text");
    REQ(D_Button, "D_Button");    REQ(D_Text, "D_Text");
    REQ(ConfirmTarget_Button, "ConfirmTarget_Button");
    REQ(CancelTarget_Button, "CancelTarget_Button");
#undef REQ

    // GameInstance 캐시
    GameInstance = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this));

    // 이미 바인딩돼 있으면 건너뜀
    if (!bButtonsBound)
    {
        BindButtonEvents(); // 여기 안에서 AddDynamic 하기 전에 RemoveDynamic(this)를 먼저 해도 OK
        bButtonsBound = true;
    }

    UE_LOG(LogTemp, Log, TEXT("[TurnHudWidget] Widget constructed"));
}

void UTurnHudWidget::NativeDestruct()
{
    if (Q_Button) Q_Button->OnClicked.RemoveDynamic(this, &UTurnHudWidget::OnQButtonClicked);
    if (W_Button) W_Button->OnClicked.RemoveDynamic(this, &UTurnHudWidget::OnWButtonClicked);
    if (E_Button) E_Button->OnClicked.RemoveDynamic(this, &UTurnHudWidget::OnEButtonClicked);
    if (A_Button) A_Button->OnClicked.RemoveDynamic(this, &UTurnHudWidget::OnAButtonClicked);
    if (S_Button) S_Button->OnClicked.RemoveDynamic(this, &UTurnHudWidget::OnSButtonClicked);
    if (D_Button) D_Button->OnClicked.RemoveDynamic(this, &UTurnHudWidget::OnDButtonClicked);
    if (ConfirmTarget_Button) ConfirmTarget_Button->OnClicked.RemoveDynamic(this, &UTurnHudWidget::OnConfirmTargetClicked);
    if (CancelTarget_Button)  CancelTarget_Button->OnClicked.RemoveDynamic(this, &UTurnHudWidget::OnCancelTargetClicked);

    bButtonsBound = false;
    Super::NativeDestruct();
}

void UTurnHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (GetVisibility() != ESlateVisibility::Visible) return;
    if (!IsCharacterValid()) return;

    UpdateAPDisplay();

    if (CurrentHudMode == ETurnHudMode::EnemyTurn)
    {
        // 방어 모드일 땐 스킬 UI 갱신 금지
        // (원한다면 여기서 Q/W Enabled를 따로 관리)
        if (Q_Button) Q_Button->SetIsEnabled(true);
        if (W_Button) W_Button->SetIsEnabled(true);
    }
    else
    {
        UpdateSkillButtons();
    }

    //// 위젯이 숨겨져 있으면 업데이트하지 않음
    //if (GetVisibility() != ESlateVisibility::Visible)
    //{
    //    return;
    //}
    //
    //// 바인딩된 캐릭터가 유효한 경우에만 업데이트
    //if (IsCharacterValid())
    //{
    //    UpdateAPDisplay();
    //    UpdateSkillButtons();
    //}
}

void UTurnHudWidget::BindToCharacter(ATurnPlayer* Character)
{
    if (!IsValid(Character))
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnHudWidget] Trying to bind to invalid character"));
        return;
    }

    BoundCharacter = Character;
    UE_LOG(LogTemp, Log, TEXT("[TurnHudWidget] Bound to character: %s"), *Character->GetName());

    // 초기 상태 업데이트
    UpdateAPDisplay();
    UpdateSkillButtons();
}

void UTurnHudWidget::SetHudMode(ETurnHudMode NewMode)
{
    if (CurrentHudMode == NewMode)
        return;

    CurrentHudMode = NewMode;
    UE_LOG(LogTemp, Log, TEXT("[TurnHudWidget] HUD Mode changed to: %s"), *UEnum::GetValueAsString(NewMode));

    // 모드에 따라 어떤 컨테이너를 보여줄지 결정하여 헬퍼 함수를 호출합니다.
    switch (NewMode)
    {
    case ETurnHudMode::None:
        UpdateAllButtonContainerVisibilities(false, false, false, false);
        break;

    case ETurnHudMode::PlayerTurn:
        // 플레이어 턴 기본 상태에서는 스킬 버튼만 표시합니다.
        UpdateAllButtonContainerVisibilities(true, true, true, false);
        break;

    case ETurnHudMode::EnemyTurn:
        UpdateAllButtonContainerVisibilities(false, false, false, true);
        break;

    case ETurnHudMode::Targeting:
        // 타겟팅 중에는 스킬 버튼 대신 타겟팅 버튼을 표시합니다.
        UpdateAllButtonContainerVisibilities(false, true, false, false);
        break;

    case ETurnHudMode::Aiming:
        // 조준 중에는 원거리 공격 버튼만 표시합니다.
        UpdateAllButtonContainerVisibilities(false, false, true, false);
        break;
    }

    // HUD 자체의 전체적인 표시 여부도 설정합니다.
    SetVisibility(NewMode == ETurnHudMode::None ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
}

void UTurnHudWidget::UpdateAPDisplay()
{
    if (!IsCharacterValid() || !AP_Text)
        return;

    ATurnPlayer* Player = BoundCharacter.Get();
    const FCharacterStats& Stats = Player->CharacterStats;

    FString APString = FString::Printf(TEXT("AP: %d/%d"), Stats.CurrentAP, Stats.MAXAP);
    AP_Text->SetText(FText::FromString(APString));

    // AP에 따른 색상 변경
    FLinearColor APColor = FLinearColor::White;
    if (Stats.CurrentAP <= 1)
    {
        APColor = FLinearColor::Red; // AP 부족
    }
    else if (Stats.CurrentAP <= 2)
    {
        APColor = FLinearColor::Yellow; // AP 주의
    }
    else
    {
        APColor = FLinearColor::Green; // AP 충분
    }

    AP_Text->SetColorAndOpacity(FSlateColor(APColor));
}

void UTurnHudWidget::UpdateSkillButtons()
{
    if (!IsCharacterValid())
        return;

    // 방어 모드에선 스킬 UI 갱신 금지 (라벨 덮어쓰기 방지)
    if (CurrentHudMode == ETurnHudMode::EnemyTurn)
    {
        // 여기서 방어 라벨을 한번 더 보증해도 좋음
        if (Q_Text) Q_Text->SetText(FText::FromString(TEXT("Q: 패링")));
        if (W_Text) W_Text->SetText(FText::FromString(TEXT("W: 회피")));
        if (Q_Button) Q_Button->SetIsEnabled(true);
        if (W_Button) W_Button->SetIsEnabled(true);
        return;
    }

    // 각 스킬 버튼의 활성화 상태 업데이트
    if (Q_Button) Q_Button->SetIsEnabled(CanUseSkillQ());
    if (W_Button) W_Button->SetIsEnabled(CanUseSkillW());
    if (E_Button) E_Button->SetIsEnabled(CanUseSkillE());
    if (A_Button) A_Button->SetIsEnabled(CanUseSkillA());
    if (S_Button) S_Button->SetIsEnabled(CanUseSkillS());
    if (D_Button) D_Button->SetIsEnabled(CanUseSkillD());

    // 스킬 설명 업데이트
    if (Q_Text) Q_Text->SetText(FText::FromString(TEXT("Q:기본공격(-1AP)")));
    if (W_Text) W_Text->SetText(FText::FromString(TEXT("W:힐(-2AP)")));
    if (E_Text) E_Text->SetText(FText::FromString(TEXT("E:강공격(-3AP)")));

    // 포션 개수 표시
    if (GameInstance.IsValid())
    {
        if (A_Text)
        {
            FString HPPotionText = FString::Printf(TEXT("A:HP포션(%d개)"),
                GameInstance->CurrentHPPotions);
            A_Text->SetText(FText::FromString(HPPotionText));
        }

        if (S_Text)
        {
            FString APPotionText = FString::Printf(TEXT("S:AP포션(%d개)"),
                GameInstance->CurrentAPPotions);
            S_Text->SetText(FText::FromString(APPotionText));
        }
    }

    if (D_Text) D_Text->SetText(FText::FromString(TEXT("D:턴 종료")));
}

//void UTurnHudWidget::SetHudVisible(bool bVisible)
//{
//    if (bVisible)
//    {
//        SetVisibility(ESlateVisibility::Visible);
//    }
//    else
//    {
//        SetVisibility(ESlateVisibility::Hidden);
//    }
//}

void UTurnHudWidget::BindButtonEvents()
{
    // 스킬 버튼들
    if (Q_Button)
    {
        Q_Button->OnClicked.RemoveAll(this);         
        Q_Button->OnClicked.AddUniqueDynamic(this, &UTurnHudWidget::OnQButtonClicked);
    }

    if (W_Button)
    {
        W_Button->OnClicked.RemoveAll(this);         
        W_Button->OnClicked.AddUniqueDynamic(this, &UTurnHudWidget::OnWButtonClicked);
    }

    if (E_Button)
    {
        E_Button->OnClicked.RemoveAll(this);         
        E_Button->OnClicked.AddUniqueDynamic(this, &UTurnHudWidget::OnEButtonClicked);
    }

    if (A_Button)
    {
        A_Button->OnClicked.RemoveAll(this);          
        A_Button->OnClicked.AddUniqueDynamic(this, &UTurnHudWidget::OnAButtonClicked);
    }

    if (S_Button)
    {
        S_Button->OnClicked.RemoveAll(this);          
        S_Button->OnClicked.AddUniqueDynamic(this, &UTurnHudWidget::OnSButtonClicked);
    }

    if (D_Button)
    {
        D_Button->OnClicked.RemoveAll(this);          
        D_Button->OnClicked.AddUniqueDynamic(this, &UTurnHudWidget::OnDButtonClicked);
    }
    

    // 타겟팅 버튼들
    if (ConfirmTarget_Button)
    {
        ConfirmTarget_Button->OnClicked.RemoveAll(this);           
        ConfirmTarget_Button->OnClicked.AddUniqueDynamic(this, &UTurnHudWidget::OnConfirmTargetClicked);
    }

    if (CancelTarget_Button)
    {
        CancelTarget_Button->OnClicked.RemoveAll(this);           
        CancelTarget_Button->OnClicked.AddUniqueDynamic(this, &UTurnHudWidget::OnCancelTargetClicked);
    }
  
}

bool UTurnHudWidget::CanUseSkillQ() const
{
    return IsCharacterValid() && BoundCharacter->IsMyTurnActive();
}

bool UTurnHudWidget::CanUseSkillW() const
{
    if (!IsCharacterValid()) return false;
    return BoundCharacter->IsMyTurnActive() && BoundCharacter->CharacterStats.CurrentAP >= 2;
}

bool UTurnHudWidget::CanUseSkillE() const
{
    if (!IsCharacterValid()) return false;
    return BoundCharacter->IsMyTurnActive() && BoundCharacter->CharacterStats.CurrentAP >= 3;
}

bool UTurnHudWidget::CanUseSkillA() const
{
    if (!IsCharacterValid() || !GameInstance.IsValid()) return false;
    return BoundCharacter->IsMyTurnActive() && GameInstance->HasHPPotion();

}

bool UTurnHudWidget::CanUseSkillS() const
{
    if (!IsCharacterValid() || !GameInstance.IsValid()) return false;
    return BoundCharacter->IsMyTurnActive() && GameInstance->HasAPPotion();
}

bool UTurnHudWidget::CanUseSkillD() const
{
    return IsCharacterValid() && BoundCharacter->IsMyTurnActive();
}

void UTurnHudWidget::OnQButtonClicked()
{
    if (IsCharacterValid())
    {
        BoundCharacter->UseSkill_Q();
    }
}

void UTurnHudWidget::OnWButtonClicked()
{
    if (IsCharacterValid())
    {
        BoundCharacter->UseSkill_W();
    }
}

void UTurnHudWidget::OnEButtonClicked()
{
    if (IsCharacterValid())
    {
        BoundCharacter->UseSkill_E();
    }
}

void UTurnHudWidget::OnAButtonClicked()
{
    if (IsCharacterValid())
    {
        BoundCharacter->UseSkill_A();
    }
}

void UTurnHudWidget::OnSButtonClicked()
{
    if (IsCharacterValid())
    {
        BoundCharacter->UseSkill_S();
    }
}

void UTurnHudWidget::OnDButtonClicked()
{
    if (IsCharacterValid())
    {
        BoundCharacter->UseSkill_D();
    }
}

void UTurnHudWidget::OnConfirmTargetClicked()
{
    if (IsCharacterValid() && BoundCharacter->TargetSelector)
    {
        BoundCharacter->TargetSelector->ConfirmTarget();
    }
}

void UTurnHudWidget::OnCancelTargetClicked()
{
    if (IsCharacterValid() && BoundCharacter->TargetSelector)
    {
        BoundCharacter->TargetSelector->CancelTargetSelection();
    }
}

//void UTurnHudWidget::OnRangedAttackClicked()
//{
//    if (IsCharacterValid())
//    {
//        BoundCharacter->Fire();
//    }
//}

//void UTurnHudWidget::SetSkillButtonsVisible(bool bVisible)
//{
//    if (SkillButtonContainer_1)
//    {
//        SkillButtonContainer_1->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
//    }
//
//    if (SkillButtonContainer_2)
//    {
//        SkillButtonContainer_1->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
//    }
//}
//
//void UTurnHudWidget::SetTargetingButtonsVisible(bool bVisible)
//{
//    if (TargetingButtonContainer)
//    {
//        TargetingButtonContainer->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
//    }
//}
//
//void UTurnHudWidget::SetRangedButtonVisible(bool bVisible)
//{
//    if (RangedButtonContainer)
//    {
//        RangedButtonContainer->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
//    }
//}
//
//void UTurnHudWidget::SetDefenseButtonsVisible(bool bVisible)
//{
//    // 적 턴일 때 패링/회피 버튼만 표시
//    if (bVisible)
//    {
//        if (Q_Button && Q_Text)
//        {
//            Q_Button->SetVisibility(ESlateVisibility::Visible);
//            Q_Text->SetText(FText::FromString(TEXT("Q: 패링")));
//        }
//        if (W_Button && W_Text)
//        {
//            W_Button->SetVisibility(ESlateVisibility::Visible);
//            W_Text->SetText(FText::FromString(TEXT("W: 회피")));
//        }
//
//        // 나머지 버튼들은 숨김
//        if (E_Button) E_Button->SetVisibility(ESlateVisibility::Hidden);
//        if (A_Button) A_Button->SetVisibility(ESlateVisibility::Hidden);
//        if (S_Button) S_Button->SetVisibility(ESlateVisibility::Hidden);
//        if (D_Button) D_Button->SetVisibility(ESlateVisibility::Hidden);
//    }
//    else
//    {
//        // 모든 스킬 버튼 숨김
//        if (Q_Button) Q_Button->SetVisibility(ESlateVisibility::Hidden);
//        if (W_Button) W_Button->SetVisibility(ESlateVisibility::Hidden);
//        if (E_Button) E_Button->SetVisibility(ESlateVisibility::Hidden);
//        if (A_Button) A_Button->SetVisibility(ESlateVisibility::Hidden);
//        if (S_Button) S_Button->SetVisibility(ESlateVisibility::Hidden);
//        if (D_Button) D_Button->SetVisibility(ESlateVisibility::Hidden);
//    }
//}

void UTurnHudWidget::UpdateAllButtonContainerVisibilities(bool bShowSkills, bool bShowTargeting, bool bShowRanged, bool bShowDefense)
{
    const bool bShowSkillContainers = bShowSkills || bShowDefense; // ← 핵심

    // 스킬 컨테이너 (1, 2)
    if (SkillButtonContainer_1)
    {
        SkillButtonContainer_1->SetVisibility(bShowSkillContainers ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    if (SkillButtonContainer_2)
    {
        SkillButtonContainer_2->SetVisibility(bShowSkills ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    // 타겟팅 컨테이너
    if (TargetingButtonContainer)
    {
        TargetingButtonContainer->SetVisibility(bShowTargeting ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    // 원거리 공격 컨테이너
    if (RangedButtonContainer)
    {
        RangedButtonContainer->SetVisibility(bShowRanged ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    // 방어 모드: Q/W만 키고 라벨 변경
    if (bShowDefense)
    {
        SetVis(Q_Button, true);
        SetVis(W_Button, true);
        if (Q_Text) Q_Text->SetText(FText::FromString(TEXT("Q: 패링")));
        if (W_Text) W_Text->SetText(FText::FromString(TEXT("W: 회피")));
    }
    else if (bShowSkills)
    {
        // 평상시 스킬 모드: Q/W 복구
        SetVis(Q_Button, true);
        SetVis(W_Button, true);
        if (Q_Text) Q_Text->SetText(FText::FromString(TEXT("Q:기본공격(+1AP)")));
        if (W_Text) W_Text->SetText(FText::FromString(TEXT("W:힐(-2AP)")));
    }
    else
    {
        // 둘 다 아니면 Q/W 숨김
        SetVis(Q_Button, false);
        SetVis(W_Button, false);
    }

    // 일반 스킬 모드가 아닐 때, 다른 스킬 버튼(E,A,S,D)은 항상 숨깁니다.
    if (!bShowSkills)
    {
        if (E_Button) E_Button->SetVisibility(ESlateVisibility::Collapsed);
        if (A_Button) A_Button->SetVisibility(ESlateVisibility::Collapsed);
        if (S_Button) S_Button->SetVisibility(ESlateVisibility::Collapsed);
        if (D_Button) D_Button->SetVisibility(ESlateVisibility::Collapsed);
    }
    else
    {
        // 스킬 모드일 때는 모든 스킬 버튼을 다시 보이게 합니다.

        if (E_Button) E_Button->SetVisibility(ESlateVisibility::Visible);
        if (A_Button) A_Button->SetVisibility(ESlateVisibility::Visible);
        if (S_Button) S_Button->SetVisibility(ESlateVisibility::Visible);
        if (D_Button) D_Button->SetVisibility(ESlateVisibility::Visible);
        UpdateSkillButtons(); // 텍스트도 원래대로 돌려놓습니다.
    }
}

bool UTurnHudWidget::IsCharacterValid() const
{
    return BoundCharacter.IsValid() && IsValid(BoundCharacter.Get());
}