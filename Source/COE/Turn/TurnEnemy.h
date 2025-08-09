// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCode/COECharacter.h"
#include "TurnEnemy.generated.h"

// Delegate Ÿ�� ����
DECLARE_MULTICAST_DELEGATE(FOnTurnEnemyDead)
/**
 * 
 */
UCLASS()
class COE_API ATurnEnemy : public ACOECharacter
{
	GENERATED_BODY()

public:
    ATurnEnemy();

public:
    virtual void BeginPlay() override;

public:

    /** �׾��� �� ��ε�ĳ��Ʈ�Ǵ� �̺�Ʈ */
    FOnTurnEnemyDead OnDead;

public:
    /** �������� �޴� �Լ� (����) */
    virtual float TakeDamage(float DamageAmount,struct FDamageEvent const& DamageEvent,AController* EventInstigator, AActor* DamageCauser) override;

};
