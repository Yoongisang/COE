// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurnCombatBridgeComponent.generated.h"

class UCOEGameInstance;     // 전투 메타 저장소(팀/생사/이니셔티브/상태)
class ACombatManager;       // 턴 진행 오케스트레이터
class ACOECharacter;        // 전투 참가자 베이스(ATurnPlayer/ATurnEnemy)

// 내 턴 알림(BP 확장용)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMyTurnSimple, int32, Round);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COE_API UTurnCombatBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UTurnCombatBridgeComponent();

    // 전투 편의 옵션 -----------------------------
    // BeginPlay 시 자동 등록/세팅을 수행할지 여부(맵 배치만으로 바로 참여 시킬 때 유용)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Bridge")
    bool bAutoRegisterOnBeginPlay = true;

    // 자동 등록 시 기본 이니셔티브(별도 시스템이 있으면 0으로 두고 코드에서 SetInitiative 호출)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Bridge")
    int32 DefaultInitiative = 10;

    // 내 턴/종료를 컴포넌트 이벤트로 브로드캐스트(BP에서 위젯/AI 트리거 연결)
    UPROPERTY(BlueprintAssignable, Category = "Combat|Bridge|Events")
    FOnMyTurnSimple OnMyTurnStarted;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Bridge|Events")
    FOnMyTurnSimple OnMyTurnEnded;

protected:
    virtual void BeginPlay() override; // GI/Manager 캐싱 + 선택적 자동 등록
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override; // 안전한 해제

public:
    // 전투 초기 설정을 수동으로 수행하고 싶을 때 호출(레벨 스크립트/스폰 직후 등)
    UFUNCTION(BlueprintCallable, Category = "Combat|Bridge")
    void InitializeForCombat();

    // 스킬 트리거(이름/타겟 기반). 실제 스킬 코드는 캐릭터/블루프린트에 구현.
    UFUNCTION(BlueprintCallable, Category = "Combat|Bridge")
    void UseSkillByName(FName SkillId, ACOECharacter* Target);

    // 즉시형 기본 공격 예시(ApplyDamage → 턴 종료)
    UFUNCTION(BlueprintCallable, Category = "Combat|Bridge")
    void UseRangedBasic(ACOECharacter* Target, float Damage = 10.f);

    // 지연형 스킬(애님/투사체 등) 종료 시점에서 호출해 턴을 넘김
    UFUNCTION(BlueprintCallable, Category = "Combat|Bridge")
    void NotifySkillFinished();

    // 전투 중 사망 처리(HP 시스템 외부에서 호출 가능. GI Alive=false로 반영 후 필요 시 턴 종료)
    UFUNCTION(BlueprintCallable, Category = "Combat|Bridge")
    void MarkDead(bool bEndTurnIfMine = true);

private:
    // CombatManager 찾기(레벨에 하나 존재한다고 가정. 다수라면 정책 확장 필요)
    void FindCombatManager();

    // 매니저 델리게이트 바인딩/해제
    void BindToManagerDelegates();
    void UnbindFromManagerDelegates();

    // 델리게이트 핸들러(내 캐릭터의 턴만 처리)
    UFUNCTION()
    void HandleTurnStarted(ACOECharacter* ActiveCharacter, int32 Round);

    UFUNCTION()
    void HandleTurnEnded(ACOECharacter* ActiveCharacter, int32 Round);

private:
    UPROPERTY()
    UCOEGameInstance* GI = nullptr;              // GameInstance 캐시

    UPROPERTY()
    ACombatManager* Manager = nullptr;           // CombatManager 캐시

    UPROPERTY()
    ACOECharacter* OwnerCharacter = nullptr;     // 이 컴포넌트를 가진 캐릭터 캐시
};