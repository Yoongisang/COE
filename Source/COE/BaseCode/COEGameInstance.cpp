// Fill out your copyright notice in the Description page of Project Settings.


#include "COEGameInstance.h"
#include "COECharacter.h"
#include "Turn/TurnPlayer.h"
#include "Turn/TurnEnemy.h"

// 특정 캐릭터를 지정된 팀으로 설정
void UCOEGameInstance::SetTeam(ACOECharacter* Character, ECombatTeam Team)
{
    if (!IsValid(Character)) return; // 유효하지 않은 포인터 방지
    TeamOf.FindOrAdd(Character) = Team; // 이미 있으면 덮어쓰기, 없으면 추가
    UE_LOG(LogTemp, Log, TEXT("[GI] SetTeam: %s -> %s"), *Character->GetName(), *UEnum::GetValueAsString(Team));
}

// 클래스 타입 기반으로 자동 팀 배정
void UCOEGameInstance::AutoAssignTeam(ACOECharacter* Character)
{
    if (!IsValid(Character)) return; // 널 체크

    if (Character->IsA(ATurnPlayer::StaticClass()))
    {
        SetTeam(Character, ECombatTeam::Player);
        UE_LOG(LogTemp, Log, TEXT("[GI] AutoAssignTeam: %s assigned to Player"), *Character->GetName());
    }
    else if (Character->IsA(ATurnEnemy::StaticClass()))
    {
        SetTeam(Character, ECombatTeam::Enemy);
        UE_LOG(LogTemp, Log, TEXT("[GI] AutoAssignTeam: %s assigned to Enemy"), *Character->GetName());
    }
}

// 캐릭터의 팀 반환
ECombatTeam UCOEGameInstance::GetTeam(ACOECharacter* Character) const
{
    if (!IsValid(Character)) return ECombatTeam::Enemy;
    if (const ECombatTeam* Found = TeamOf.Find(Character)) return *Found;
    return ECombatTeam::Enemy;
}

// 캐릭터의 생사 상태 설정
void UCOEGameInstance::SetAlive(ACOECharacter* Character, bool bAlive)
{
    if (!IsValid(Character)) return;

    if (bAlive)
    {
        DeadSet.Remove(Character);
        UE_LOG(LogTemp, Log, TEXT("[GI] SetAlive: %s marked Alive"), *Character->GetName());
    }
    else
    {
        DeadSet.Add(Character);
        UE_LOG(LogTemp, Log, TEXT("[GI] SetAlive: %s marked Dead"), *Character->GetName());
    }
}

// 캐릭터가 살아있는지 여부 반환
bool UCOEGameInstance::IsAlive(ACOECharacter* Character) const
{
    if (!IsValid(Character)) return false;
    bool bResult = !DeadSet.Contains(Character);
    UE_LOG(LogTemp, Log, TEXT("[GI] IsAlive: %s -> %s"), *Character->GetName(), bResult ? TEXT("Alive") : TEXT("Dead"));
    return bResult;
}

// 특정 팀에 살아있는 캐릭터가 있는지 검사
bool UCOEGameInstance::HasTeamAlive(ECombatTeam Team) const
{
    for (const auto& P : TeamOf)
    {
        ACOECharacter* Char = P.Key.Get();
        if (!IsValid(Char)) continue;

        if (P.Value == Team && !DeadSet.Contains(Char))
        {
            UE_LOG(LogTemp, Log, TEXT("[GI] HasTeamAlive: Team %s has living member %s"), *UEnum::GetValueAsString(Team), *Char->GetName());
            return true;
        }
    }
    UE_LOG(LogTemp, Log, TEXT("[GI] HasTeamAlive: Team %s has no living members"), *UEnum::GetValueAsString(Team));
    return false;
}

// 캐릭터의 이니셔티브 값 설정
void UCOEGameInstance::SetInitiative(ACOECharacter* Character, int32 Value)
{
    if (!IsValid(Character)) return;
    InitiativeOf.FindOrAdd(Character) = Value;
    UE_LOG(LogTemp, Log, TEXT("[GI] SetInitiative: %s -> %d"), *Character->GetName(), Value);
}

// 캐릭터의 이니셔티브 값 반환
int32 UCOEGameInstance::GetInitiative(ACOECharacter* Character) const
{
    if (!IsValid(Character)) return 0;
    if (const int32* Found = InitiativeOf.Find(Character))
    {
        UE_LOG(LogTemp, Log, TEXT("[GI] GetInitiative: %s -> %d"), *Character->GetName(), *Found);
        return *Found;
    }
    UE_LOG(LogTemp, Log, TEXT("[GI] GetInitiative: %s -> default 10"), *Character->GetName());
    return 10;
}

// 전투 관련 데이터 초기화
void UCOEGameInstance::ResetCombatData()
{
    CombatState = ECombatState::None;
    DeadSet.Reset();
    InitiativeOf.Reset();
    UE_LOG(LogTemp, Log, TEXT("[GI] ResetCombatData: CombatState reset, DeadSet cleared, InitiativeOf cleared"));
    // TeamOf.Reset(); // 필요 시 주석 해제
}
