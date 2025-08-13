// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnEnemy.h"

ATurnEnemy::ATurnEnemy()
{
}

void ATurnEnemy::BeginPlay()
{
    Super::BeginPlay();

    CharacterStats.Agility = 8.f;
}

float ATurnEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    //부모에서 받은 데미지를 AcualDamage로 반환
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    //예: HP 감소 로직 (직접 관리 중이라면)
    CharacterStats.CurrentHP -= ActualDamage;
    if (CharacterStats.CurrentHP <= 0.f)
    {
        // 죽음 처리 직전에 delegate 브로드캐스트
        OnDead.Broadcast();

        // 액터 제거 등 추가 처리
        Destroy();
    }

    return ActualDamage;
}

