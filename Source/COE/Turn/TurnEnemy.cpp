// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnEnemy.h"
#include "BaseCode/COEAnimInstance.h"
#include "TurnCombatBridgeComponent.h"

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

void ATurnEnemy::ExecuteEnemyTurn()
{
    // 자신의 턴이 아니면 동작하지 않음
    if (!IsMyTurnActive())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s ExecuteEnemyTurn called but not my turn!"), *GetName());
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Enemy Turn Started - Thinking..."), *GetName());

    // 5초 후 공격 실행
    FTimerHandle AttackDelayHandle;
    GetWorld()->GetTimerManager().SetTimer(
        AttackDelayHandle,
        this,
        &ATurnEnemy::PerformEnemyAttack,
        5.0f,
        false
    );
}

bool ATurnEnemy::IsMyTurnActive() const
{
    if (!TurnBridge || !TurnBridge->GetManager())
        return false;

    // CombatManager를 통해 현재 활성 캐릭터가 자신인지 확인
    ACOECharacter* ActiveChar = TurnBridge->GetManager()->GetActiveCharacter();
    bool bIsMyTurn = (ActiveChar == this);

    if (bIsMyTurn)
    {
        UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s - My Turn Active"), *GetName());
    }

    return bIsMyTurn;
}

void ATurnEnemy::DefaultAttack()
{
    // 자신의 턴이 아니면 동작하지 않음
    if (!IsMyTurnActive())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s DefaultAttack called but not my turn!"), *GetName());
        return;
    }

    // Enemy 전용 기본공격 구현 (Super 호출하지 않음)
    UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Enemy DefaultAttack executed!"), *GetName());

    // TODO: Enemy 기본공격 로직 구현
    // - 애니메이션 재생
    // - 타겟 선정 (플레이어)
    // - 데미지 적용
    // - 이펙트/사운드

    // 임시로 애니메이션 재생만 구현
    if (IsValid(AnimInstance))
    {
        bIsAttacking = true; // 공격 상태로 설정
        AnimInstance->DefaultAttackAnim();
        UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] Enemy DefaultAttack Animation started"));
    }
    else
    {
        // AnimInstance가 없는 경우 즉시 턴 종료
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] No AnimInstance, ending turn immediately"));
        FinishEnemyTurn();
    }
}

void ATurnEnemy::PerformEnemyAttack()
{
    // 자신의 턴이 아니면 동작하지 않음
    if (!IsMyTurnActive())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s PerformEnemyAttack called but not my turn!"), *GetName());
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Attack!"), *GetName());

    // 랜덤하게 기본공격 또는 스킬공격 선택 (50% 확률)
    int32 AttackType = FMath::RandRange(1, 2);

    if (AttackType == 1)
    {
        // 기본공격 함수 호출 (오버라이드된 DefaultAttack 사용)
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s 기본공격(DefaultAttack) 함수 호출!"), *GetName());
        DefaultAttack();
    }
    else
    {
        // 스킬공격 함수 호출 (추후 구현)
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s 스킬공격(PerformSkill) 함수 호출!"), *GetName());
        PerformSkill(); // 추후 구현할 함수

        // 2초 후 턴 종료 -> 추후 스킬도 에니메이션이 생기면 이부분 제거
        FTimerHandle EndTurnDelayHandle;
        GetWorld()->GetTimerManager().SetTimer
        (
            EndTurnDelayHandle,
            this,
            &ATurnEnemy::FinishEnemyTurn,
            2.0f,
            false
        );
    }
   
}

void ATurnEnemy::PerformSkill()
{
    // 자신의 턴이 아니면 동작하지 않음
    if (!IsMyTurnActive())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s DefaultAttack called but not my turn!"), *GetName());
        return;
    }

    // Enemy 전용 스킬공격 구현
    UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Enemy Skill executed!"), *GetName());

    // TODO: Enemy 스킬공격 로직 구현
    // - 애니메이션 재생
    // - 타겟 선정 (플레이어)
    // - 데미지 적용
    // - 이펙트/사운드

    // 추후 스킬 애니메이션 재생 구현
    //if (IsValid(AnimInstance))
    //{
    //    AnimInstance->DefaultAttackAnim();
    //    UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] Enemy DefaultAttack Animation started"));
    //}
}

void ATurnEnemy::FinishEnemyTurn()
{
    // 자신의 턴이 아니면 동작하지 않음
    if (!IsMyTurnActive())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s DefaultAttack called but not my turn!"), *GetName());
        return;

    }
    UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Turn Finished"), *GetName());

    // 브리지를 통해 턴 종료 알림
    if (TurnBridge)
    {
        TurnBridge->NotifySkillFinished();
    }
}

