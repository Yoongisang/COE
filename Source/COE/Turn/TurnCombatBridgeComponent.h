// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurnCombatBridgeComponent.generated.h"

class UCOEGameInstance;     // ���� ��Ÿ �����(��/����/�̴ϼ�Ƽ��/����)
class ACombatManager;       // �� ���� ���ɽ�Ʈ������
class ACOECharacter;        // ���� ������ ���̽�(ATurnPlayer/ATurnEnemy)

// �� �� �˸�(BP Ȯ���)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMyTurnSimple, int32, Round);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COE_API UTurnCombatBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UTurnCombatBridgeComponent();

    // ���� ���� �ɼ� -----------------------------
    // BeginPlay �� �ڵ� ���/������ �������� ����(�� ��ġ������ �ٷ� ���� ��ų �� ����)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Bridge")
    bool bAutoRegisterOnBeginPlay = true;

    // �ڵ� ��� �� �⺻ �̴ϼ�Ƽ��(���� �ý����� ������ 0���� �ΰ� �ڵ忡�� SetInitiative ȣ��)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Bridge")
    int32 DefaultInitiative = 10;

    // �� ��/���Ḧ ������Ʈ �̺�Ʈ�� ��ε�ĳ��Ʈ(BP���� ����/AI Ʈ���� ����)
    UPROPERTY(BlueprintAssignable, Category = "Combat|Bridge|Events")
    FOnMyTurnSimple OnMyTurnStarted;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Bridge|Events")
    FOnMyTurnSimple OnMyTurnEnded;

protected:
    virtual void BeginPlay() override; // GI/Manager ĳ�� + ������ �ڵ� ���
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override; // ������ ����

public:
    // ���� �ʱ� ������ �������� �����ϰ� ���� �� ȣ��(���� ��ũ��Ʈ/���� ���� ��)
    UFUNCTION(BlueprintCallable, Category = "Combat|Bridge")
    void InitializeForCombat();

    // ��ų Ʈ����(�̸�/Ÿ�� ���). ���� ��ų �ڵ�� ĳ����/�������Ʈ�� ����.
    UFUNCTION(BlueprintCallable, Category = "Combat|Bridge")
    void UseSkillByName(FName SkillId, ACOECharacter* Target);

    // ����� �⺻ ���� ����(ApplyDamage �� �� ����)
    UFUNCTION(BlueprintCallable, Category = "Combat|Bridge")
    void UseRangedBasic(ACOECharacter* Target, float Damage = 10.f);

    // ������ ��ų(�ִ�/����ü ��) ���� �������� ȣ���� ���� �ѱ�
    UFUNCTION(BlueprintCallable, Category = "Combat|Bridge")
    void NotifySkillFinished();

    // ���� �� ��� ó��(HP �ý��� �ܺο��� ȣ�� ����. GI Alive=false�� �ݿ� �� �ʿ� �� �� ����)
    UFUNCTION(BlueprintCallable, Category = "Combat|Bridge")
    void MarkDead(bool bEndTurnIfMine = true);

private:
    // CombatManager ã��(������ �ϳ� �����Ѵٰ� ����. �ټ���� ��å Ȯ�� �ʿ�)
    void FindCombatManager();

    // �Ŵ��� ��������Ʈ ���ε�/����
    void BindToManagerDelegates();
    void UnbindFromManagerDelegates();

    // ��������Ʈ �ڵ鷯(�� ĳ������ �ϸ� ó��)
    UFUNCTION()
    void HandleTurnStarted(ACOECharacter* ActiveCharacter, int32 Round);

    UFUNCTION()
    void HandleTurnEnded(ACOECharacter* ActiveCharacter, int32 Round);

private:
    UPROPERTY()
    UCOEGameInstance* GI = nullptr;              // GameInstance ĳ��

    UPROPERTY()
    ACombatManager* Manager = nullptr;           // CombatManager ĳ��

    UPROPERTY()
    ACOECharacter* OwnerCharacter = nullptr;     // �� ������Ʈ�� ���� ĳ���� ĳ��
};