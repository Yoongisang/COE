// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCode/COECharacter.h"
#include "TurnEnemy.generated.h"

// Delegate 타입 선언
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

    /** 죽었을 때 브로드캐스트되는 이벤트 */
    FOnTurnEnemyDead OnDead;

public:
    /** 데미지를 받는 함수 (예시) */
    virtual float TakeDamage(float DamageAmount,struct FDamageEvent const& DamageEvent,AController* EventInstigator, AActor* DamageCauser) override;

};
