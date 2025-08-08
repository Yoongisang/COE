// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatManager.generated.h"

/**
 *
 */

class UCOEGameInstance;     // ���� ��Ÿ������ �ҽ�
class ACOECharacter;        // ���� ������ ���� ���̽� (ATurnPlayer/ATurnEnemy)

// �� ���Ŀ� ��Ʈ��: ���� Initiative �켱, ������ TieBreak ������ ����
USTRUCT()
struct FTurnEntry
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<ACOECharacter> Character; // ��Ƽ��� �ൿ�� ���� ĳ����

    UPROPERTY()
    int32 Initiative = 0;                    // �켱��(�������� ����)

    UPROPERTY()
    int32 TieBreak = 0;                      // ���� ����� ����(�������� �켱)

    bool operator<(const FTurnEntry& Other) const
    {
        if (Initiative != Other.Initiative) return Initiative > Other.Initiative; // ��������
        return TieBreak < Other.TieBreak;                                         // ��������
    }
};


// UI/����� �̺�Ʈ(�ǵ����ʹ� GI���� ��ȸ)
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
    FOnCombatStarted OnCombatStarted;   // ���� ����

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnCombatEnded OnCombatEnded;       // ���� ����(����/����)

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnTurnChanged OnTurnStarted;       // �� ����

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnTurnChanged OnTurnEnded;         // �� ����

public: // �����ֱ�
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartCombat();                 // ù �� ���۱��� ó��

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void EndCombat(bool bPlayerWon);    // ���� ����/���� ����

public: // ������ ���� (ACOECharacter ����)
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool RegisterParticipant(ACOECharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool UnregisterParticipant(ACOECharacter* Character);

public: // �� ����
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void BuildTurnOrder();              // GI �̴ϼ�Ƽ��� ����

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void BeginNextTurn();               // ���� ������ ����

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void NotifyTurnActionEnd();         // ��ų/�ִ�/����ü ���� �� ȣ��

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ApplyDamageAndEndTurn(ACOECharacter* InstigatorCharacter, ACOECharacter* TargetCharacter, float Damage); // ����� ����

    UFUNCTION(BlueprintPure, Category = "Combat")
    ACOECharacter* GetActiveCharacter() const;   // ���� �� ��ü

protected:
    virtual void BeginPlay() override;  // GI ĳ��

private:
    void EnterState(uint8 NewState);    // GI�� ���� ����
    void CheckVictory();                // GI �����ͷ� ���� ����

private:
    UPROPERTY()
    UCOEGameInstance* GI = nullptr;     // ���� ��Ÿ������ �ҽ�

    UPROPERTY()
    TArray<TWeakObjectPtr<ACOECharacter>> Participants; // ��ϵ� ������

    UPROPERTY()
    TArray<FTurnEntry> TurnOrder;       // ���ĵ� �� ť

    UPROPERTY()
    int32 CurrentIndex = INDEX_NONE;    // ���� �� �ε���

    UPROPERTY()
    int32 Round = 0;                    // ���� ����(0����)
};
