// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaseCode/COEGameInstance.h"
#include "Engine/Engine.h"
#include "TargetSelectionComponent.generated.h"

class ACOECharacter;
class APlayerController;
class UCameraComponent;

// 타겟 선택 상태
UENUM(BlueprintType)
enum class ETargetSelectionState : uint8
{
	None,           // 타겟 선택 비활성
	Selecting,      // 타겟 선택 중
	Confirmed       // 타겟 확정됨
};

// 스킬 타입 (타겟 제한용)
UENUM(BlueprintType)
enum class ESkillTargetType : uint8
{
	Attack,         // 공격 스킬 (적에게만)
	Heal,           // 회복 스킬 (아군에게만)
	Buff,           // 버프 스킬 (아군에게만)
	Universal       // 범용 (모든 대상에게)
};

// 타겟 선택 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTargetSelected, ACOECharacter*, TargetCharacter, ESkillTargetType, SkillType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTargetSelectionCancelled);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COE_API UTargetSelectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

    UTargetSelectionComponent();

    // 이벤트 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "Target Selection")
    FOnTargetSelected OnTargetSelected;

    UPROPERTY(BlueprintAssignable, Category = "Target Selection")
    FOnTargetSelectionCancelled OnTargetSelectionCancelled;

protected:

    virtual void BeginPlay() override;

public:

    /** 타겟 선택 시작 */
    UFUNCTION(BlueprintCallable, Category = "Target Selection")
    void StartTargetSelection(ESkillTargetType SkillType);

    /** 타겟 선택 취소 */
    UFUNCTION(BlueprintCallable, Category = "Target Selection")
    void CancelTargetSelection();

    /** 현재 타겟 확정 */
    UFUNCTION(BlueprintCallable, Category = "Target Selection")
    void ConfirmTarget();

    /** 다음 타겟으로 이동 */
    UFUNCTION(BlueprintCallable, Category = "Target Selection")
    void SelectNextTarget();

    /** 이전 타겟으로 이동 */
    UFUNCTION(BlueprintCallable, Category = "Target Selection")
    void SelectPreviousTarget();

    /** 현재 상태 확인 */
    UFUNCTION(BlueprintPure, Category = "Target Selection")
    ETargetSelectionState GetSelectionState() const { return SelectionState; }

    /** 현재 선택된 타겟 반환 */
    UFUNCTION(BlueprintPure, Category = "Target Selection")
    ACOECharacter* GetCurrentTarget() const { return CurrentTarget.Get(); }

    /** 타겟 선택이 유효한지 확인 */
    UFUNCTION(BlueprintPure, Category = "Target Selection")
    bool IsTargetValid() const;

private:

    /** 타겟 목록 업데이트 */
    void UpdateTargetList();

    /** 타겟 인덱스 변경 */
    void ChangeTargetIndex(int32 Delta);

    /** 타겟에 따른 카메라 설정 */
    void SetupCameraForTarget(ACOECharacter* Target);

    /** 플레이어를 타겟 방향으로 회전 */
    void RotatePlayerToTarget(ACOECharacter* Target);

    /** 아군 캐릭터 정면에서 바라보는 카메라 설정 */
    void SetupPlayerFrontCamera(ACOECharacter* PlayerTarget);

    /** 임시 카메라 액터 생성 */
    void CreateTemporaryCameraActor();

    /** 임시 카메라 정리 */
    void CleanupTemporaryCamera();

    /** 원래 카메라로 복원 */
    void RestoreOriginalCamera();

private:

    /** 현재 선택 상태 */
    UPROPERTY()
    ETargetSelectionState SelectionState = ETargetSelectionState::None;

    /** 현재 스킬 타입 */
    UPROPERTY()
    ESkillTargetType CurrentSkillType = ESkillTargetType::Universal;

    /** 선택 가능한 타겟 목록 */
    UPROPERTY()
    TArray<TWeakObjectPtr<ACOECharacter>> ValidTargets;

    /** 현재 선택된 타겟 인덱스 */
    UPROPERTY()
    int32 CurrentTargetIndex = 0;

    /** 현재 선택된 타겟 */
    UPROPERTY()
    TWeakObjectPtr<ACOECharacter> CurrentTarget;

    /** 컨트롤러 캐시 */
    UPROPERTY()
    TWeakObjectPtr<APlayerController> PlayerController;

    /** GameInstance 캐시 */
    UPROPERTY()
    TWeakObjectPtr<UCOEGameInstance> GameInstance;

    /** 임시 카메라 (아군 타겟팅용) */
    UPROPERTY()
    TObjectPtr<UCameraComponent> TempCamera;

    /** 임시 카메라 액터 */
    UPROPERTY()
    TObjectPtr<AActor> TempCameraActor;

    /** 원래 카메라 복원용 */
    UPROPERTY()
    TWeakObjectPtr<AActor> OriginalViewTarget;

    /** 플레이어 캐릭터 캐시 */
    UPROPERTY()
    TWeakObjectPtr<ACOECharacter> OwnerCharacter;

public:

    /** 원래 플레이어 회전값 */
    UPROPERTY()
    FRotator OriginalPlayerRotation;
		
};
