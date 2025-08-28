// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "BaseCode/COEGameInstance.h"
#include "TurnHudWidget.generated.h"

class ATurnPlayer;
class ACOECharacter;

/** HUD 표시 모드 */
UENUM(BlueprintType)
enum class ETurnHudMode : uint8
{
	None,			// 숨김
	PlayerTurn,		// 플레이어 턴 (모든 스킬 버튼)
	EnemyTurn,		// 적 턴 (패링/회피만)
	Targeting,		// 타겟 선택 중
	Aiming			// 조준 중
};

/**
 * 
 */
UCLASS()
class COE_API UTurnHudWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UTurnHudWidget(const FObjectInitializer& ObjectInitializer);

protected:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override; 
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// === UI 컴포넌트들 (블루프린트에서 바인딩) ===

	/** 메인 컨테이너 */
	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* MainContainer = nullptr;
	
	/** AP 표시 텍스트 */
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* AP_Text = nullptr;
	
	/** 스킬 버튼 컨테이너 */
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* SkillButtonContainer_1 = nullptr;
	
	/** Q 스킬 버튼 (기본공격) */
	UPROPERTY(meta = (BindWidget))
	class UButton* Q_Button = nullptr;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Q_Text = nullptr;
	
	/** W 스킬 버튼 (힐) */
	UPROPERTY(meta = (BindWidget))
	class UButton* W_Button = nullptr;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* W_Text = nullptr;
	
	/** E 스킬 버튼 (강공격) */
	UPROPERTY(meta = (BindWidget))
	class UButton* E_Button = nullptr;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* E_Text = nullptr;
	
	/** 스킬 버튼 컨테이너 */
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* SkillButtonContainer_2 = nullptr;
	
	/** A 스킬 버튼 (HP포션) */
	UPROPERTY(meta = (BindWidget))
	class UButton* A_Button = nullptr;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* A_Text = nullptr;
	
	/** S 스킬 버튼 (AP포션) */
	UPROPERTY(meta = (BindWidget))
	class UButton* S_Button = nullptr;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* S_Text = nullptr;
	
	/** D 스킬 버튼 (턴 종료) */
	UPROPERTY(meta = (BindWidget))
	class UButton* D_Button = nullptr;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* D_Text = nullptr;
	
	/** 타겟팅 관련 버튼들 */
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* TargetingButtonContainer = nullptr;
	
	/** 타겟 확정 버튼 */
	UPROPERTY(meta = (BindWidget))
	class UButton* ConfirmTarget_Button = nullptr;
	
	/** 타겟 취소 버튼 */
	UPROPERTY(meta = (BindWidget))
	class UButton* CancelTarget_Button = nullptr;
	
	/** 타겟팅 관련 버튼들 */
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* RangedButtonContainer = nullptr;
	
	/** 원거리 공격 버튼 */
	UPROPERTY(meta = (BindWidget))
	class UButton* RangedAttack_Button = nullptr;

	/** 원거리 조준 버튼 */
	UPROPERTY(meta = (BindWidget))
	class UButton* RangedSetAim_Button = nullptr;

public:
	/** 특정 캐릭터에 HUD 바인딩 */
	UFUNCTION(BlueprintCallable, Category = "Turn HUD")
	void BindToCharacter(ATurnPlayer* Character);

	/** HUD 모드 변경 */
	UFUNCTION(BlueprintCallable, Category = "Turn HUD")
	void SetHudMode(ETurnHudMode NewMode);

	/** AP 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "Turn HUD")
	void UpdateAPDisplay();

	/** 스킬 버튼 활성화/비활성화 */
	UFUNCTION(BlueprintCallable, Category = "Turn HUD")
	void UpdateSkillButtons();

	///** 위젯 표시/숨김 */
	//UFUNCTION(BlueprintCallable, Category = "Turn HUD")
	//void SetHudVisible(bool bVisible);

	protected:
		/** 바인딩된 캐릭터 */
		UPROPERTY()
		TWeakObjectPtr<ATurnPlayer> BoundCharacter;

		/** GameInstance 캐시 */
		UPROPERTY()
		TWeakObjectPtr<UCOEGameInstance> GameInstance;

		/** 현재 HUD 모드 */
		UPROPERTY()
		ETurnHudMode CurrentHudMode = ETurnHudMode::None;

private:
	/** 버튼 이벤트 바인딩 */
	void BindButtonEvents();

	/** 각 스킬별 사용 가능 여부 체크 */
	bool CanUseSkillQ() const;
	bool CanUseSkillW() const;
	bool CanUseSkillE() const;
	bool CanUseSkillA() const;
	bool CanUseSkillS() const;
	bool CanUseSkillD() const;

	/** 버튼 클릭 이벤트들 */
	UFUNCTION()
	void OnQButtonClicked();

	UFUNCTION()
	void OnWButtonClicked();

	UFUNCTION()
	void OnEButtonClicked();

	UFUNCTION()
	void OnAButtonClicked();

	UFUNCTION()
	void OnSButtonClicked();

	UFUNCTION()
	void OnDButtonClicked();

	UFUNCTION()
	void OnConfirmTargetClicked();

	UFUNCTION()
	void OnCancelTargetClicked();

	//UFUNCTION()
	//void OnRangedAttackClicked();

	/** UI 컴포넌트 표시 제어 
	void SetSkillButtonsVisible(bool bVisible);
	void SetTargetingButtonsVisible(bool bVisible);
	void SetRangedButtonVisible(bool bVisible);
	void SetDefenseButtonsVisible(bool bVisible);

	/** 모든 버튼 컨테이너의 표시 여부를 한 번에 관리하는 헬퍼 함수 (신규 추가) */
	void UpdateAllButtonContainerVisibilities(bool bShowSkills, bool bShowTargeting, bool bShowRanged, bool bShowDefense);

	/** 캐릭터 유효성 체크 */
	bool IsCharacterValid() const;

	private:
		bool bButtonsBound = false;
};
