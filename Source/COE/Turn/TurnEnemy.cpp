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
    //�θ𿡼� ���� �������� AcualDamage�� ��ȯ
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    //��: HP ���� ���� (���� ���� ���̶��)
    CharacterStats.CurrentHP -= ActualDamage;
    if (CharacterStats.CurrentHP <= 0.f)
    {
        // ���� ó�� ������ delegate ��ε�ĳ��Ʈ
        OnDead.Broadcast();

        // ���� ���� �� �߰� ó��
        Destroy();
    }

    return ActualDamage;
}

