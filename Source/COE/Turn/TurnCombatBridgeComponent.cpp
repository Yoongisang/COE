// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnCombatBridgeComponent.h"
#include "CombatManager.h"              // ACombatManager
#include "BaseCode/COEGameInstance.h"            // UCOEGameInstance
#include "BaseCode/COECharacter.h"               // ACOECharacter
#include "Kismet/GameplayStatics.h"     // GetGameInstance, GetAllActorsOfClass
#include "GameFramework/Actor.h"

UTurnCombatBridgeComponent::UTurnCombatBridgeComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // �̺�Ʈ �帮��
}

void UTurnCombatBridgeComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<ACOECharacter>(GetOwner());      // ������ ĳ���� ĳ��
    GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this)); // GI ĳ��

    FindCombatManager();                                    // �Ŵ��� Ž��
    BindToManagerDelegates();                               // ��������Ʈ ����(������)

    if (bAutoRegisterOnBeginPlay)
    {
        InitializeForCombat();                              // �ڵ� ���/����
    }
}

void UTurnCombatBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindFromManagerDelegates();                           // ������ ����
    Super::EndPlay(EndPlayReason);
}

void UTurnCombatBridgeComponent::InitializeForCombat()
{
    if (!OwnerCharacter) return;                            // ĳ���Ͱ� �ƴ� ��� ���� ����

    if (!GI)
    {
        GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this));
    }
    if (GI)
    {
        GI->AutoAssignTeam(OwnerCharacter);                // TurnPlayer��Player, TurnEnemy��Enemy
        GI->SetAlive(OwnerCharacter, true);                // ���� ���� �� ���� ó��
        if (DefaultInitiative != INT32_MIN)
        {
            GI->SetInitiative(OwnerCharacter, DefaultInitiative); // �⺻ �̴ϼ�Ƽ�� ����
        }
    }

    if (!Manager) FindCombatManager();
    if (Manager && OwnerCharacter)
    {
        Manager->RegisterParticipant(OwnerCharacter);      // ���� ���� ���
    }
}

void UTurnCombatBridgeComponent::UseSkillByName(FName SkillId, ACOECharacter* Target)
{
    if (!Manager) FindCombatManager();
    if (!Manager || !OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Bridge] Manager/Owner missing. UseSkillByName aborted: %s"), *SkillId.ToString());
        return;
    }

    // TODO: ĳ������ ���� ��ų �Լ� ���ε� ����
    // ��) if (SkillId=="Q") OwnerCharacter->CastQ(Target);

    if (SkillId == TEXT("RightClickRanged"))
    {
        UseRangedBasic(Target, 10.f);                      // ����� ���÷� ó��
        return;
    }

    // ������ ��ų�� �ִϸ��̼�/����ü ���� �������� NotifySkillFinished() ȣ�� �ʿ�
}

void UTurnCombatBridgeComponent::UseRangedBasic(ACOECharacter* Target, float Damage)
{
    if (!Manager || !OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Bridge] Manager/Owner missing. UseRangedBasic aborted"));
        return;
    }

    if (!IsValid(Target))
    {
        // Ÿ���� ������ �׳� �ϸ� ������ �÷ο찡 ������ �ʰ� ��
        UE_LOG(LogTemp, Warning, TEXT("[Bridge] Target invalid. Ending turn without damage."));
        Manager->NotifyTurnActionEnd();
        return;
    }

    // �����: ������ ���� �� �� ����
    AController* Ctrl = OwnerCharacter->GetController();
    UGameplayStatics::ApplyDamage(Target, Damage, Ctrl, OwnerCharacter, nullptr);

    Manager->NotifyTurnActionEnd();
}

void UTurnCombatBridgeComponent::NotifySkillFinished()
{
    if (!Manager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Bridge] Manager missing. NotifySkillFinished ignored."));
        return;
    }
    // �ִ�/����ü ���� �� �׼� �Ϸ� �������� ȣ�� �� ���� ������ ����
    Manager->NotifyTurnActionEnd();
}

void UTurnCombatBridgeComponent::MarkDead(bool bEndTurnIfMine)
{
    if (!OwnerCharacter || !GI)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Bridge] MarkDead: Missing Owner/GI"));
        return;
    }
    GI->SetAlive(OwnerCharacter, false);                   // GI�� ��� �ݿ�(���� ���� �ٰ�)

    if (bEndTurnIfMine && Manager && Manager->GetActiveCharacter() == OwnerCharacter)
    {
        // �� �� ���� ��� ����ߴٸ� ���� �о� ���� ��ü ����
        Manager->NotifyTurnActionEnd();
    }
}

void UTurnCombatBridgeComponent::FindCombatManager()
{
    if (Manager) return;                                   // �̹� ã������ ��ŵ

    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(this, ACombatManager::StaticClass(), Found);
    Manager = Found.Num() > 0 ? Cast<ACombatManager>(Found[0]) : nullptr; // ���� �ʿ� �ϳ���� ����

    if (!Manager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Bridge] CombatManager not found in level"));
    }
}

void UTurnCombatBridgeComponent::BindToManagerDelegates()
{
    if (!Manager) return;

    // ��������Ʈ�� ���� ���ε� �ߺ��� ���ϱ� ���� ���ε� ���� ����ε� ����
    UnbindFromManagerDelegates();

    Manager->OnTurnStarted.AddDynamic(this, &UTurnCombatBridgeComponent::HandleTurnStarted);
    Manager->OnTurnEnded.AddDynamic(this, &UTurnCombatBridgeComponent::HandleTurnEnded);
}

void UTurnCombatBridgeComponent::UnbindFromManagerDelegates()
{
    if (!Manager) return;

    Manager->OnTurnStarted.RemoveDynamic(this, &UTurnCombatBridgeComponent::HandleTurnStarted);
    Manager->OnTurnEnded.RemoveDynamic(this, &UTurnCombatBridgeComponent::HandleTurnEnded);
}

void UTurnCombatBridgeComponent::HandleTurnStarted(ACOECharacter* ActiveCharacter, int32 Round)
{
    if (ActiveCharacter == OwnerCharacter)
    {
        // �� �� ����: UI ����, �Է� ���, AI Ʈ���� ���� BP���� ���ε��Ͽ� ó��
        OnMyTurnStarted.Broadcast(Round);
        UE_LOG(LogTemp, Log, TEXT("[Bridge] My Turn START (Round %d) : %s"), Round, *OwnerCharacter->GetName());
    }
}

void UTurnCombatBridgeComponent::HandleTurnEnded(ACOECharacter* ActiveCharacter, int32 Round)
{
    if (ActiveCharacter == OwnerCharacter)
    {
        // �� �� ����: UI �ݱ�, �Է� ��� ���� BP���� ó��
        OnMyTurnEnded.Broadcast(Round);
        UE_LOG(LogTemp, Log, TEXT("[Bridge] My Turn END   (Round %d) : %s"), Round, *OwnerCharacter->GetName());
    }
}


