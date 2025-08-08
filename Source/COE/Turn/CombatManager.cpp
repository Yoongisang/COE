// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatManager.h"
#include "Kismet/GameplayStatics.h"
#include "BaseCode/COEGameInstance.h"
#include "BaseCode/COECharacter.h"

ACombatManager::ACombatManager()
{
    PrimaryActorTick.bCanEverTick = false; // �̺�Ʈ �帮�� �����̹Ƿ� ƽ ���ʿ�
}

void ACombatManager::BeginPlay()
{
    Super::BeginPlay();
    // ���� ���� ��Ÿ�����͸� �����ϴ� GameInstance�� ĳ���� �� ������ ��ȸ�� ����
    GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this)); // GI�� null�� �� ����
}

void ACombatManager::EnterState(uint8 NewState)
{
    // CombatState�� ���� �ҽ��� GameInstance. ���� ���̴� �ݵ�� GI�� ���� ���
    if (GI)
    {
        GI->SetCombatState(static_cast<ECombatState>(NewState)); // (uint8��Enum) ���� ĳ����
    }
}

void ACombatManager::StartCombat()
{
    Round = 0;                              // ���� ī���� �ʱ�ȭ(0���� ����)
    if (GI) GI->ResetCombatData();          // ���� ���� �ܿ� ��Ÿ������ �ʱ�ȭ(���� ��å�� ���� ����)

    BuildTurnOrder();                        // �����ڵ��� �̴ϼ�Ƽ�� �������� ����
    EnterState(static_cast<uint8>(ECombatState::Setup)); // ���� �غ� ���� ����
    OnCombatStarted.Broadcast();             // UI/���� �� �ܺ� �ý��ۿ� ���� ���� ��ȣ
    BeginNextTurn();                         // ù �� ����(�ε����� BuildTurnOrder���� -1�� �ʱ�ȭ��)
}

void ACombatManager::EndCombat(bool bPlayerWon)
{
    // ���п� ���� ���� ��ȯ �� �ܺο� ����
    EnterState(static_cast<uint8>(bPlayerWon ? ECombatState::Victory : ECombatState::Defeat));
    OnCombatEnded.Broadcast(bPlayerWon, Round); // ���� �� ���� �� ����
}

bool ACombatManager::RegisterParticipant(ACOECharacter* Character)
{
    // ���� ������ ���(�������� �迭). ��ȿ/�ߺ� ���� ó�� ����
    if (!IsValid(Character)) return false;            // ��/�ı� ��� ���� ����
    if (Participants.Contains(Character)) return false; // �̹� ��ϵ� ��� ����
    Participants.Add(Character);                       // �������ͷ� ���� �� GC/���� ��ȯ ����
    return true;
}

bool ACombatManager::UnregisterParticipant(ACOECharacter* Character)
{
    // ���� ������ ����: Participants�� TurnOrder���� ��� ����
    if (!IsValid(Character)) return false;
    const int32 Removed = Participants.RemoveAll([&](const TWeakObjectPtr<ACOECharacter>& P) { return P.Get() == Character; }); // �������� �񱳷� ����
    TurnOrder.RemoveAll([&](const FTurnEntry& E) { return E.Character.Get() == Character; }); // �� ť������ ����(�� ���� �ı� �� ��ȣ)
    return Removed > 0;                                 // �ϳ� �̻� ���ŵǾ��� ���� true
}

void ACombatManager::BuildTurnOrder()
{
    // ���� ������/���� ���¸� �������� �� �� ť ����
    TurnOrder.Reset();
    for (const auto& P : Participants)                  // �������� �迭 ��ȸ
    {
        ACOECharacter* C = P.Get();                     // �������� �� ���� ������
        if (!IsValid(C)) continue;                      // ��ȿ ������ ��ŵ
        if (GI && !GI->IsAlive(C)) continue;            // ����ڴ� �� ��� ����
        const int32 Ini = GI ? GI->GetInitiative(C) : 10; // �̴ϼ�Ƽ�� ��ȸ(�⺻ 10)
        FTurnEntry E; E.Character = C; E.Initiative = Ini; E.TieBreak = FMath::Rand(); // ���� �� TieBreak�� ���� ����ȭ
        TurnOrder.Add(E);
    }
    TurnOrder.Sort();                                    // ���� Initiative �켱, ������ TieBreak ��������
    CurrentIndex = -1;                                   // BeginNextTurn���� ++�Ǿ� 0���� �����ϵ��� �غ�
}

void ACombatManager::BeginNextTurn()
{
    // �� ť�� ��������� �����(ó��/���� �ѿ���/��� ��� ��)
    if (TurnOrder.Num() == 0)
    {
        BuildTurnOrder();
        if (TurnOrder.Num() == 0) { EndCombat(true); return; } // ��ȣ: ������ ���� �� ������ ��Ȳ ó��
    }

    CurrentIndex++;                                      // ���� ������ �̵�(-1��0, 0��1 ...)
    if (CurrentIndex >= TurnOrder.Num())                 // ������ �����̸� ���� �ѿ���
    {
        Round++;                                         // ���� ����
        BuildTurnOrder();                                // ���/��Ȱ/���� ���� �ݿ��� ������
        CurrentIndex = 0;                                // �� ���� ù �����ں��� ����
    }

    ACOECharacter* Active = GetActiveCharacter();        // ���� �� ��ü
    if (!IsValid(Active)) { BeginNextTurn(); return; }   // �ı�/��ȿ�� ��ŵ�ϰ� ���� ������(���� ��ġ)

    EnterState(static_cast<uint8>(ECombatState::PreTurn)); // �Է� ���/���� �غ� �ܰ�
    OnTurnStarted.Broadcast(Active, Round);               // UI/����/�α� �� �ܺ� �ý��� ����
}

void ACombatManager::NotifyTurnActionEnd()
{
    // ĳ���� �� �׼�(�ִ�/����ü ��)�� �Ϸ�� �������� ȣ��Ǵ� ��������Ʈ
    if (ACOECharacter* Active = GetActiveCharacter())
    {
        OnTurnEnded.Broadcast(Active, Round);            // UI/����/�α� �� ���� ó�� ��ȣ
    }

    CheckVictory();                                      // �׼� ����� ���а� ���ȴ��� Ȯ��
    if (GI && (GI->GetCombatState() == ECombatState::Victory || GI->GetCombatState() == ECombatState::Defeat))
    {
        return;                                          // ������ ���� ���¸� �� �������� ����
    }

    EnterState(static_cast<uint8>(ECombatState::PostTurn)); // ��ó�� �ܰ� ǥ��
    BeginNextTurn();                                      // ���� ������ ����
}

void ACombatManager::ApplyDamageAndEndTurn(ACOECharacter* InstigatorCharacter, ACOECharacter* TargetCharacter, float Damage)
{
    // ����� ��ų�� ���� �Լ�: �������� �����ϰ� ����/���п� ������� �� ����
    if (!IsValid(TargetCharacter)) { NotifyTurnActionEnd(); return; } // Ÿ�� ��ȿ�� �ٷ� �� ����

    // UGameplayStatics::ApplyDamage�� ����� TakeDamage�� ȣ���Ͽ� ���� ü��/��� ó���� ����
    AController* Ctrl = InstigatorCharacter ? InstigatorCharacter->GetController() : nullptr; // ������ ��Ʈ�ѷ�(���� �� ����)
    UGameplayStatics::ApplyDamage(TargetCharacter, Damage, Ctrl, InstigatorCharacter, nullptr); // DamageType�� ���� Ȯ�� ����

    // ��� �ݿ��� ����� TakeDamage ���ο��� GI->SetAlive(this,false)�� �����ϴ� ���� ����
    NotifyTurnActionEnd();                                   // ��� �� ����� �÷ο� ����
}

ACOECharacter* ACombatManager::GetActiveCharacter() const
{
    // ���� �ε����� ��ȿ���� ���� �� �ش� ��Ʈ���� ĳ���� ��ȯ
    if (!TurnOrder.IsValidIndex(CurrentIndex)) return nullptr; // �ʱ�/���� ����
    return TurnOrder[CurrentIndex].Character.Get();            // �������� �� ���� ������
}

void ACombatManager::CheckVictory()
{
    // GameInstance�� ��ϵ� ��/���� ������ �������� ������ ���� ����
    if (!GI) return;                                        // GI ������ ���� �Ұ�
    const bool bPlayersAlive = GI->HasTeamAlive(ECombatTeam::Player); // �÷��̾� �� ���� ����
    const bool bEnemiesAlive = GI->HasTeamAlive(ECombatTeam::Enemy);   // �� �� ���� ����

    if (!bPlayersAlive && !bEnemiesAlive) { EndCombat(false); return; } // ���� ����: �ӽ÷� �й� ó��(��Ģ�� �°� ���� ����)
    if (!bEnemiesAlive) { EndCombat(true); return; }                    // �� ����: �¸�
    if (!bPlayersAlive) { EndCombat(false); return; }                   // �÷��̾� ����: �й�
}