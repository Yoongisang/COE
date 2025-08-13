// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatManager.generated.h"

/**
 *
 */

class UCOEGameInstance;     // 전투 메타데이터 소스
class ACOECharacter;        // 전투 참가자 공통 베이스 (ATurnPlayer/ATurnEnemy)

// 턴 정렬용 엔트리: 높은 Initiative 우선, 동률은 TieBreak 난수로 결정
USTRUCT()
struct FTurnEntry
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<ACOECharacter> Character; // 액티브로 행동할 전투 캐릭터

    UPROPERTY()
    int32 Initiative = 0;                    // 우선도(높을수록 먼저)

    UPROPERTY()
    int32 TieBreak = 0;                      // 동률 깨기용 난수(작을수록 우선)

    bool operator<(const FTurnEntry& Other) const
    {
        if (Initiative != Other.Initiative) return Initiative > Other.Initiative; // 내림차순
        return TieBreak < Other.TieBreak;                                         // 오름차순
    }
};


// UI/사운드용 이벤트(실데이터는 GI에서 조회)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatEnded, bool, bPlayerWon, int32, Rounds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTurnChanged, ACOECharacter*, ActiveCharacter, int32, Round);

UCLASS()
class COE_API ACombatManager : public AActor
{
	GENERATED_BODY()
	
public:
    ACombatManager();

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnCombatStarted OnCombatStarted;   // 전투 시작

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnCombatEnded OnCombatEnded;       // 전투 종료(승패/라운드)

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnTurnChanged OnTurnStarted;       // 턴 시작

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnTurnChanged OnTurnEnded;         // 턴 종료

public: // 수명주기
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartCombat();                 // 첫 턴 시작까지 처리

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void EndCombat(bool bPlayerWon);    // 강제 종료/승패 통지

public: // 참가자 관리 (ACOECharacter 한정)
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool RegisterParticipant(ACOECharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool UnregisterParticipant(ACOECharacter* Character);

public: // 턴 로직
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void BuildTurnOrder();              // GI 이니셔티브로 정렬

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void BeginNextTurn();               // 다음 턴으로 진행

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void NotifyTurnActionEnd();         // 스킬/애님/투사체 종료 시 호출

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ApplyDamageAndEndTurn(ACOECharacter* InstigatorCharacter, ACOECharacter* TargetCharacter, float Damage); // 즉시형 편의

    UFUNCTION(BlueprintPure, Category = "Combat")
    ACOECharacter* GetActiveCharacter() const;   // 현재 턴 주체

protected:
    virtual void BeginPlay() override;  // GI 캐싱

private:
    void EnterState(uint8 NewState);    // GI로 상태 전파
    void CheckVictory();                // GI 데이터로 승패 판정

private:
    UPROPERTY()
    UCOEGameInstance* GI = nullptr;     // 전투 메타데이터 소스

    UPROPERTY()
    TArray<TWeakObjectPtr<ACOECharacter>> Participants; // 등록된 참가자

    UPROPERTY()
    TArray<FTurnEntry> TurnOrder;       // 정렬된 턴 큐

    UPROPERTY()
    int32 CurrentIndex = INDEX_NONE;    // 현재 턴 인덱스

    UPROPERTY()
    int32 Round = 0;                    // 누적 라운드(0부터)
};
