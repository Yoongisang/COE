// Fill out your copyright notice in the Description page of Project Settings.


#include "COEGameInstance.h"
#include "COECharacter.h"
#include "Turn/TurnPlayer.h"
#include "Turn/TurnEnemy.h"

// Ư�� ĳ���͸� ������ ������ ����
void UCOEGameInstance::SetTeam(ACOECharacter* Character, ECombatTeam Team)
{
    if (!IsValid(Character)) return; // ��ȿ���� ���� ������ ����
    TeamOf.FindOrAdd(Character) = Team; // �̹� ������ �����, ������ �߰�
    UE_LOG(LogTemp, Log, TEXT("[GI] SetTeam: %s -> %s"), *Character->GetName(), *UEnum::GetValueAsString(Team));
}

// Ŭ���� Ÿ�� ������� �ڵ� �� ����
void UCOEGameInstance::AutoAssignTeam(ACOECharacter* Character)
{
    if (!IsValid(Character)) return; // �� üũ

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

// ĳ������ �� ��ȯ
ECombatTeam UCOEGameInstance::GetTeam(ACOECharacter* Character) const
{
    if (!IsValid(Character)) return ECombatTeam::Enemy;
    if (const ECombatTeam* Found = TeamOf.Find(Character)) return *Found;
    return ECombatTeam::Enemy;
}

// ĳ������ ���� ���� ����
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

// ĳ���Ͱ� ����ִ��� ���� ��ȯ
bool UCOEGameInstance::IsAlive(ACOECharacter* Character) const
{
    if (!IsValid(Character)) return false;
    bool bResult = !DeadSet.Contains(Character);
    UE_LOG(LogTemp, Log, TEXT("[GI] IsAlive: %s -> %s"), *Character->GetName(), bResult ? TEXT("Alive") : TEXT("Dead"));
    return bResult;
}

// Ư�� ���� ����ִ� ĳ���Ͱ� �ִ��� �˻�
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

// ĳ������ �̴ϼ�Ƽ�� �� ����
void UCOEGameInstance::SetInitiative(ACOECharacter* Character, int32 Value)
{
    if (!IsValid(Character)) return;
    InitiativeOf.FindOrAdd(Character) = Value;
    UE_LOG(LogTemp, Log, TEXT("[GI] SetInitiative: %s -> %d"), *Character->GetName(), Value);
}

// ĳ������ �̴ϼ�Ƽ�� �� ��ȯ
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

// ���� ���� ������ �ʱ�ȭ
void UCOEGameInstance::ResetCombatData()
{
    CombatState = ECombatState::None;
    DeadSet.Reset();
    InitiativeOf.Reset();
    UE_LOG(LogTemp, Log, TEXT("[GI] ResetCombatData: CombatState reset, DeadSet cleared, InitiativeOf cleared"));
    // TeamOf.Reset(); // �ʿ� �� �ּ� ����
}
