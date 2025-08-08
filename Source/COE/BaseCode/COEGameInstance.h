// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "COEGameInstance.generated.h"

/**
 * 
 */

 // ���� ����(���� ���¸� ǥ��: ����/ī�޶� ���� ���� �ý��ۿ��� ó��)
UENUM(BlueprintType)
enum class ECombatState : uint8
{
	None,       // ���� ��Ȱ��
	Setup,      // ���� �غ�(������ ����, ���� ��)
	PreTurn,    // �� ���� ����(UI ����, �Է� ���)
	Acting,     // ��ų/�ִ�/����ü ���� ��
	PostTurn,   // �� ����(����/����� ƽ, DOT)
	Victory,    // �÷��̾� �¸�
	Defeat,     // �÷��̾� �й�
	Paused      // �Ͻ�����(�޴�/����)
};

// �� ����(�ʿ� �� Ally/Summon ������ Ȯ�� ����)
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

	/** �÷��̾ ������ ��� true, ���� ���� ������ ��� false */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bPlayerInitiative = false;

	/** ������ �����Ǿ� ������ �� ��� true */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bPlayerWasDetected = false;

	/** �������� �¸��ߴ��� ���� */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bCombatVictory = false;

	/** ���� �� �÷��̾��� ��ġ */
	UPROPERTY(BlueprintReadOnly, Category = "Return")
	FVector ReturnLocation;

	/** ���� ���� �� ���ƿ� �� �̸� */
	UPROPERTY(BlueprintReadOnly, Category = "Return")
	FName ReturnMapName;

	/** �������� ����� �� �� */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TArray<FName> EnemyToRemoveName;

public:
	// ������������������������������������������������������ Combat Meta API ������������������������������������������������������
	UFUNCTION(BlueprintCallable, Category = "Combat|State")
	void SetCombatState(ECombatState InState) { CombatState = InState; }

	UFUNCTION(BlueprintPure, Category = "Combat|State")
	ECombatState GetCombatState() const { return CombatState; }

	// ---- ��/���� (ACOECharacter ���) ----
	UFUNCTION(BlueprintCallable, Category = "Combat|Teams")
	void SetTeam(ACOECharacter* Character, ECombatTeam Team);

	// TurnPlayer �� Player, TurnEnemy �� Enemy �� �ڵ� �з�(�� �ܴ� ����)
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

	// ---- �̴ϼ�Ƽ�� ----
	UFUNCTION(BlueprintCallable, Category = "Combat|Initiative")
	void SetInitiative(ACOECharacter* Character, int32 Value);

	UFUNCTION(BlueprintPure, Category = "Combat|Initiative")
	int32 GetInitiative(ACOECharacter* Character) const;

	// ---- ���� ----
	UFUNCTION(BlueprintCallable, Category = "Combat|State")
	void ResetCombatData();                           // �� ���� ���� �� ȣ��(��ȯ����/�÷��� ����)

private:
	// ���� ���� ����(���� ƽ�� ����)
	UPROPERTY()
	ECombatState CombatState = ECombatState::None;

	// ĳ���� �� ��
	UPROPERTY()
	TMap<TWeakObjectPtr<ACOECharacter>, ECombatTeam> TeamOf;

	// ����� ĳ����(IsAlive�� DeadSet ������ ���η� �Ǵ�)
	UPROPERTY()
	TSet<TWeakObjectPtr<ACOECharacter>> DeadSet;

	// ĳ���� �� �̴ϼ�Ƽ��(�� ���� ����)
	UPROPERTY()
	TMap<TWeakObjectPtr<ACOECharacter>, int32> InitiativeOf;
};
