// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "COEGameInstance.generated.h"

/**
 * 
 */

 // 전투 상태(로직 상태만 표현: 연출/카메라 등은 별도 시스템에서 처리)
UENUM(BlueprintType)
enum class ECombatState : uint8
{
	None,       // 전투 비활성
	Setup,      // 전투 준비(참가자 수집, 정렬 등)
	PreTurn,    // 턴 시작 직전(UI 열기, 입력 대기)
	Acting,     // 스킬/애님/투사체 실행 중
	PostTurn,   // 턴 정리(버프/디버프 틱, DOT)
	Victory,    // 플레이어 승리
	Defeat,     // 플레이어 패배
	Paused      // 일시정지(메뉴/연출)
};

// 팀 구분(필요 시 Ally/Summon 등으로 확장 가능)
UENUM(BlueprintType)
enum class ECombatTeam : uint8
{
	Player,
	Enemy,
};

UCLASS()
class COE_API UCOEGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:

	/** 생성자 */
	UCOEGameInstance();

public:

	/** TurnLevel 회복 아이템 공용 카운트: HP회복 아이템 최대 갯수*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable|Combat")
	int32 MaxHPPotions = 3;
	
	/** TurnLevel 회복 아이템 공용 카운트: HP회복 아이템 현재 갯수*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable|Combat")
	int32  CurrentHPPotions;

	/** TurnLevel 회복 아이템 공용 카운트: AP회복 아이템 최대 갯수*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable|Combat")
	int32 MaxAPPotions = 2;
	
	/** TurnLevel 회복 아이템 공용 카운트: AP회복 아이템 현재 갯수*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable|Combat")
	int32  CurrentAPPotions;

	/** ExplorationLevel 회복 아이템 공용 카운트: HP회복 아이템 최대 갯수*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable|Exploration")
	int32 MaxExplorationHeal = 3;

	/** ExplorationLevel 회복 아이템 공용 카운트: HP회복 아이템 현재 갯수*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable|Exploration")
	int32  CurrentExplorationHeal;


	/** TrunLevel HP회복아이템 기본 회복 수치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable|Tuning")
	float BaseHPPotionAmount = 50.f;     // HP 포션 기본 회복량(절대값)

	/** TrunLevel AP회복아이템 기본 회복 수치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable|Tuning")
	int32 BaseAPPotionAmount = 2;        // AP 포션 기본 회복량(정수)

public:

	/** 플레이어가 선공한 경우 true, 적이 먼저 감지한 경우 false */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bPlayerInitiative = false;

	/** 적에게 감지되어 전투에 들어간 경우 true */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bPlayerWasDetected = false;

	/** 전투에서 승리했는지 여부 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bCombatVictory = false;

	/** 전투 전 플레이어의 위치 */
	UPROPERTY(BlueprintReadOnly, Category = "Return")
	FVector ReturnLocation;

	/** 전투 종료 후 돌아올 맵 이름 */
	UPROPERTY(BlueprintReadOnly, Category = "Return")
	FName ReturnMapName;

	/** 전투에서 대상이 된 적 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TArray<FName> EnemyToRemoveName;

public:
	// ─────────────────────────── Combat Meta API ───────────────────────────
	UFUNCTION(BlueprintCallable, Category = "Combat|State")
	void SetCombatState(ECombatState InState) { CombatState = InState; }

	UFUNCTION(BlueprintPure, Category = "Combat|State")
	ECombatState GetCombatState() const { return CombatState; }

	// ---- 팀/생사 (ACOECharacter 기반) ----
	UFUNCTION(BlueprintCallable, Category = "Combat|Teams")
	void SetTeam(ACOECharacter* Character, ECombatTeam Team);

	// TurnPlayer ⇒ Player, TurnEnemy ⇒ Enemy 로 자동 분류(그 외는 무시)
	UFUNCTION(BlueprintCallable, Category = "Combat|Teams")
	void AutoAssignTeam(ACOECharacter* Character);

	UFUNCTION(BlueprintPure, Category = "Combat|Teams")
	ECombatTeam GetTeam(ACOECharacter* Character) const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Teams")
	void SetAlive(ACOECharacter* Character, bool bAlive);

	UFUNCTION(BlueprintPure, Category = "Combat|Teams")
	bool IsAlive(ACOECharacter* Character) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Teams")
	bool HasTeamAlive(ECombatTeam Team) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Teams")
	TArray<ACOECharacter*> GetAliveTeamMembers(ECombatTeam Team) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Initiative")
	int32 GetInitiative(ACOECharacter* Character) const;

	// ---- 리셋 ----
	UFUNCTION(BlueprintCallable, Category = "Combat|State")
	void ResetCombatData();                           // 새 전투 시작 전 호출(귀환정보/플래그 유지)

public:

	/** 사용 가능 여부*/
	UFUNCTION(BlueprintCallable, Category = "Consumable|Combat")
	bool HasHPPotion() const { return CurrentHPPotions > 0; }

	UFUNCTION(BlueprintCallable, Category = "Consumable|Combat")
	bool HasAPPotion() const { return CurrentAPPotions > 0; }

	UFUNCTION(BlueprintCallable, Category = "Consumable|Exploration")
	bool HasExplorationHeal() const { return CurrentExplorationHeal > 0; }

	/** 사용(차감) 시도 */
	UFUNCTION(BlueprintCallable, Category = "Consumable|Combat")
	bool TryConsumeHPPotion();

	UFUNCTION(BlueprintCallable, Category = "Consumable|Combat")
	bool TryConsumeAPPotion();

	UFUNCTION(BlueprintCallable, Category = "Consumable|Exploration")
	bool TryConsumeExplorationHeal();

	/** 전부 충전 */
	UFUNCTION(BlueprintCallable, Category = "Consumable")
	void RefillAllConsumables();

	/** 업그레이드용 API */
	UFUNCTION(BlueprintCallable, Category = "Consumable|Upgrade")
	void UpgradeHPPotionCapacity(int32 Delta) { MaxHPPotions = FMath::Max(0, MaxHPPotions + Delta); }

	UFUNCTION(BlueprintCallable, Category = "Consumable|Upgrade")
	void UpgradeAPPotionCapacity(int32 Delta) { MaxAPPotions = FMath::Max(0, MaxAPPotions + Delta); }

	UFUNCTION(BlueprintCallable, Category = "Consumable|Upgrade")
	void UpgradeExplorationHealCapacity(int32 Delta) { MaxExplorationHeal = FMath::Max(0, MaxExplorationHeal + Delta); }

private:
	// 현재 전투 상태(엔진 틱과 무관)
	UPROPERTY()
	ECombatState CombatState = ECombatState::None;

	// 캐릭터 → 팀
	UPROPERTY()
	TMap<TWeakObjectPtr<ACOECharacter>, ECombatTeam> TeamOf;

	// 사망한 캐릭터(IsAlive는 DeadSet 미포함 여부로 판단)
	UPROPERTY()
	TSet<TWeakObjectPtr<ACOECharacter>> DeadSet;

};
