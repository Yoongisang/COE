// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnEnemy.h"
#include "TurnPlayer.h"
#include "BaseCode/COEAnimInstance.h"
#include "TurnCombatBridgeComponent.h"
#include "CombatManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"

ATurnEnemy::ATurnEnemy()
{
    // Enemy는 움직임에 따라 회전하지 않도록 설정
    GetCharacterMovement()->bOrientRotationToMovement = false;
    bUseControllerRotationYaw = false; // Enemy는 수동으로 회전 제어
}

void ATurnEnemy::BeginPlay()
{
    Super::BeginPlay();

    // 초기 위치와 회전 저장 (중요!)
    OriginalLocation = GetActorLocation();
    OriginalRotation = GetActorRotation();
}

float ATurnEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // 이미 사망한 경우 데미지 처리 중단
    if (CharacterStats.CurrentHP <= 0.f)
    {
        return 0.f;
    }

    // 부모 클래스의 TakeDamage를 호출하여 공통 로직(HP 감소, 피격 애니메이션 등) 실행
    const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    // Super::TakeDamage에서 HP가 0 이하로 변경되었는지 재확인
    if (CharacterStats.CurrentHP <= 0.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s has died."), *GetName());

        // 1. 전투 시스템에 사망 알림 (가장 중요)
        // 만약 자신의 턴에 죽었다면, 이 함수가 CombatManager에게 턴을 넘기라고 알립니다.
        if (TurnBridge)
        {
            TurnBridge->MarkDead(true);
        }

        // 2. TurnGameMode에 사망 알림 (적 카운팅용)
        OnDead.Broadcast();

        // 3. 콜리전 및 이동 비활성화
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetCharacterMovement()->DisableMovement();

        // 4. 액터를 즉시 파괴하는 대신, 일정 시간 후 안전하게 제거되도록 설정
        // SetLifeSpan()은 이미 부모 클래스에서 호출되므로 여기서는 생략해도 됩니다.
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

void ATurnEnemy::ExecuteEnemyTurnWithTarget(ACOECharacter* SpecificTarget)
{
    // 자신의 턴이 아니면 동작하지 않음
    if (!IsMyTurnActive())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s ExecuteEnemyTurnWithTarget called but not my turn!"), *GetName());
        return;
    }

    if (!IsValid(SpecificTarget))
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s ExecuteEnemyTurnWithTarget called with invalid target"), *GetName());
        // 타겟이 무효하면 일반 로직으로 대체
        ExecuteEnemyTurn();
        return;
    }

    // 지정된 타겟 설정
    SetAssignedTarget(SpecificTarget);

    UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Enemy Turn Started with assigned target: %s"),
        *GetName(), *SpecificTarget->GetName());

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
    // 지정된 타겟이 있으면 그 타겟으로 공격
    if (AssignedTarget.IsValid())
    {
        DefaultAttackToTarget(AssignedTarget.Get());
        return;
    }

    // 지정된 타겟이 없으면 기존 로직 사용
    DefaultAttackToTarget(nullptr);

}

void ATurnEnemy::DefaultAttackToTarget(ACOECharacter* SpecificTarget)
{
    // 자신의 턴이 아니면 동작하지 않음
    if (!IsMyTurnActive())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s DefaultAttackToTarget called but not my turn!"), *GetName());
        return;
    }

    // 이미 공격 시퀀스가 진행 중이면 중복 실행 방지
    if (CurrentAttackState != EEnemyAttackState::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Attack sequence already in progress"), *GetName());
        return;
    }

    ACOECharacter* Target = nullptr;

    // 지정된 타겟이 있고 유효하면 사용
    if (IsValid(SpecificTarget))
    {
        Target = SpecificTarget;
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Using assigned target: %s"), *GetName(), *Target->GetName());
    }
    // AssignedTarget 멤버 변수도 확인합니다.
    else if(AssignedTarget.IsValid())
    {
        Target = AssignedTarget.Get();
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Using member target: %s"), *GetName(), *Target->GetName());
    }

    if (!Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s No target found, ending turn"), *GetName());
        FinishEnemyTurn();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Starting attack sequence on %s"), *GetName(), *Target->GetName());

    // 공격 시퀀스 시작
    StartAttackSequence(Target);

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

	// SkillMontage가 없으면 기본공격만 수행
    if (!AnimInstance->SkillMontage)
    {
        DefaultAttack();
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s 기본공격(DefaultAttack) 함수 호출!"), *GetName());
		return;
    }

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

    // 이미 다른 시퀀스가 진행 중이면 중복 실행 방지
    if (CurrentAttackState != EEnemyAttackState::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Skill sequence already in progress"), *GetName());
        return;
    }

    // 타겟 찾기 (지정된 타겟 우선)
    ACOECharacter* Target = nullptr;
    if (AssignedTarget.IsValid())
    {
        Target = AssignedTarget.Get();
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Using assigned target for skill: %s"), *GetName(), *Target->GetName());
    }


    if (!Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s No target found for skill, ending turn"), *GetName());
        FinishEnemyTurn();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Enemy Skill executed! Target: %s"), *GetName(), *Target->GetName());

    // 스킬 시퀀스 시작 (제자리에서 수행)
    StartSkillSequence(Target);
}

ACOECharacter* ATurnEnemy::FindTargetPlayer()
{
    // GameInstance에서 살아있는 플레이어 찾기 (백업 로직)
    UCOEGameInstance* GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this));
    if (!GI) return nullptr;

    TArray<ACOECharacter*> AlivePlayers = GI->GetAliveTeamMembers(ECombatTeam::Player);
    if (AlivePlayers.Num() == 0) return nullptr;

    // 가장 가까운 플레이어 선택
    ACOECharacter* ClosestPlayer = nullptr;
    float MinDistance = FLT_MAX;

    for (ACOECharacter* Player : AlivePlayers)
    {
        if (!IsValid(Player)) continue;

        float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            ClosestPlayer = Player;
        }
    }

    return ClosestPlayer;
}

void ATurnEnemy::StartAttackSequence(ACOECharacter* Target)
{
    if (!IsValid(Target))
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Invalid target for attack sequence"), *GetName());
        FinishEnemyTurn();
        return;
    }

    // 공격 상태로 설정
    bIsAttacking = true;
    CurrentAttackTarget = Target;
    CurrentAttackState = EEnemyAttackState::RotatingToTarget;

    // 타겟 방향 계산
    FVector DirectionToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    TargetRotation = DirectionToTarget.Rotation();
    TargetRotation.Pitch = 0.0f;
    TargetRotation.Roll = 0.0f;

    UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Starting attack sequence - Phase 1: Rotating to target"), *GetName());

    // 이동 업데이트 타이머 시작
    GetWorld()->GetTimerManager().SetTimer(
        MovementTimerHandle,
        this,
        &ATurnEnemy::UpdateAttackMovement,
        0.016f, // 약 60fps
        true
    );
}

void ATurnEnemy::UpdateAttackMovement()
{
    if (CurrentAttackState == EEnemyAttackState::None)
    {
        GetWorld()->GetTimerManager().ClearTimer(MovementTimerHandle);
        return;
    }

    const float DeltaTime = GetWorld()->GetTimerManager().GetTimerRate(MovementTimerHandle);

    switch (CurrentAttackState)
    {
    case EEnemyAttackState::RotatingToTarget:
    {
        // 타겟 방향으로 회전
        FRotator CurrentRotation = GetActorRotation();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSpeed);
        SetActorRotation(NewRotation);

        // 회전 완료 체크
        if (CurrentRotation.Equals(TargetRotation, 3.0f))
        {
            SetActorRotation(TargetRotation);
            CurrentAttackState = EEnemyAttackState::MovingToTarget;
            UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Phase 2: Moving to target"), *GetName());
        }
        break;
    }

    case EEnemyAttackState::MovingToTarget:
    {
        if (!CurrentAttackTarget.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Target lost during movement"), *GetName());
            FinishEnemyTurn();
            return;
        }

        // 목표 지점 계산 (플레이어 앞 AttackDistance 만큼 떨어진 곳)
        const FVector TargetLocation = CurrentAttackTarget->GetActorLocation();
        const FVector DirectionToTarget = (GetActorLocation() - TargetLocation).GetSafeNormal();
        const FVector Destination = TargetLocation + (DirectionToTarget * AttackDistance);

        // 목표 지점까지의 거리 확인
        const float DistanceToDestination = FVector::Dist(GetActorLocation(), Destination);

        if (DistanceToDestination < 10.0f)
        {
            // 도착했으므로 공격 단계로 전환
            GetCharacterMovement()->StopMovementImmediately();
            CurrentAttackState = EEnemyAttackState::Attacking;

            UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Phase 3: Attacking"), *GetName());

            // 공격 애니메이션 실행
            if (IsValid(AnimInstance))
            {
                AnimInstance->DefaultAttackAnim();
            }
            else
            {
                // 애니메이션이 없으면 바로 복귀 단계로
                OnAttackMontageEnded();
            }
        }
        else
        {
            // 목표 방향으로 이동 입력
            const FVector MoveDirection = (Destination - GetActorLocation()).GetSafeNormal();
            AddMovementInput(MoveDirection, 1.0f);
        }
        break;
    }

    case EEnemyAttackState::RotatingToOrigin:
    {
        // 원래 위치 방향으로 회전
        FRotator CurrentRotation = GetActorRotation();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSpeed);
        SetActorRotation(NewRotation);

        // 회전 완료 체크
        if (CurrentRotation.Equals(TargetRotation, 3.0f))
        {
            SetActorRotation(TargetRotation);
            CurrentAttackState = EEnemyAttackState::ReturningToOrigin;
            UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Phase 4: Returning to origin"), *GetName());
        }
        break;
    }

    case EEnemyAttackState::ReturningToOrigin:
    {
        // 원래 위치로 복귀
        const float DistanceToOrigin = FVector::Dist(GetActorLocation(), OriginalLocation);

        if (DistanceToOrigin < 10.0f)
        {
            // 원래 위치에 도착
            GetCharacterMovement()->StopMovementImmediately();
            SetActorLocation(OriginalLocation);
            CurrentAttackState = EEnemyAttackState::RotatingToOriginalDirection;
            TargetRotation = OriginalRotation;
            UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Phase 5: Rotating to original direction"), *GetName());
        }
        else
        {
            // 원래 위치 방향으로 이동
            const FVector MoveDirection = (OriginalLocation - GetActorLocation()).GetSafeNormal();
            AddMovementInput(MoveDirection, 1.0f);
        }
        break;
    }

    case EEnemyAttackState::RotatingToOriginalDirection:
    {
        // 원래 바라보던 방향으로 회전
        FRotator CurrentRotation = GetActorRotation();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSpeed);
        SetActorRotation(NewRotation);

        // 회전 완료 체크
        if (CurrentRotation.Equals(TargetRotation, 3.0f))
        {
            SetActorRotation(TargetRotation);
            UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Phase 6: Attack sequence complete!"), *GetName());

            // 공격 시퀀스 완료 - 턴 종료
            FinishEnemyTurn();
        }
        break;
    }

    default:
        break;
    }
}

void ATurnEnemy::StartSkillSequence(ACOECharacter* Target)
{
    if (!IsValid(Target))
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Invalid target for skill sequence"), *GetName());
        FinishEnemyTurn();
        return;
    }

    // 스킬 상태로 설정
    bIsAttacking = true;
    CurrentAttackTarget = Target;
    CurrentAttackState = EEnemyAttackState::PerformingSkill;

    // 스킬 시전 시점의 타겟 위치 저장 (미리 저장해서 움직여도 해당 위치에 레이저 발사)
    SkillTargetLocation = Target->GetActorLocation();

    // 타겟 방향 계산
    FVector DirectionToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    TargetRotation = DirectionToTarget.Rotation();
    TargetRotation.Pitch = 0.0f;
    TargetRotation.Roll = 0.0f;

    UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Starting skill sequence - Rotating to target for laser beam"), *GetName());

    // 스킬용 회전 타이머 시작 (빠른 회전)
    GetWorld()->GetTimerManager().SetTimer(
        MovementTimerHandle,
        this,
        &ATurnEnemy::UpdateSkillRotation,
        0.016f, // 약 60fps
        true
    );
}
void ATurnEnemy::UpdateSkillRotation()
{
    if (CurrentAttackState != EEnemyAttackState::PerformingSkill)
    {
        GetWorld()->GetTimerManager().ClearTimer(MovementTimerHandle);
        return;
    }

    const float DeltaTime = GetWorld()->GetTimerManager().GetTimerRate(MovementTimerHandle);

    // 타겟 방향으로 회전
    FRotator CurrentRotation = GetActorRotation();
    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, SkillRotationSpeed);
    SetActorRotation(NewRotation);

    // 회전 완료 체크
    if (CurrentRotation.Equals(TargetRotation, 2.0f))
    {
        SetActorRotation(TargetRotation);
        GetWorld()->GetTimerManager().ClearTimer(MovementTimerHandle);

        UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Skill rotation complete - Starting laser beam animation"), *GetName());

        // 스킬 애니메이션 실행
        if (IsValid(AnimInstance))
        {
            AnimInstance->SkillAnim(); // SkillMontage 실행
        }
        else
        {
            // 애니메이션이 없으면 바로 종료
            UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s No AnimInstance for skill, ending turn"), *GetName());
            OnSkillMontageEnded();
        }
    }

}
void ATurnEnemy::UpdateSkillEndRotation()
{
    const float DeltaTime = GetWorld()->GetTimerManager().GetTimerRate(MovementTimerHandle);

    // 원래 방향으로 회전
    FRotator CurrentRotation = GetActorRotation();
    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, SkillRotationSpeed);
    SetActorRotation(NewRotation);

    // 회전 완료 체크
    if (CurrentRotation.Equals(TargetRotation, 2.0f))
    {
        SetActorRotation(TargetRotation);
        GetWorld()->GetTimerManager().ClearTimer(MovementTimerHandle);

        UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Skill end rotation complete - Ready for montage end"), *GetName());
        
        // 스킬 시퀀스 완전 완료 - OnSkillMontageEnded 직접 호출
        OnSkillMontageEnded();

    }

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

    // 진행 중인 시퀀스 정리
    GetWorld()->GetTimerManager().ClearTimer(MovementTimerHandle);
    CurrentAttackState = EEnemyAttackState::None;
    CurrentAttackTarget.Reset();

    // 지정된 타겟 정리 (턴이 끝나면 초기화)
    AssignedTarget.Reset();

    bIsAttacking = false;

    // 브리지를 통해 턴 종료 알림
    if (TurnBridge)
    {
        TurnBridge->NotifySkillFinished();
    }

}

void ATurnEnemy::OnAttackMontageEnded()
{
    if (CurrentAttackState == EEnemyAttackState::Attacking)
    {
        UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Attack montage finished. Starting return sequence."), *GetName());

        // 원래 위치로 돌아가는 단계로 전환
        CurrentAttackState = EEnemyAttackState::RotatingToOrigin;

        // 원래 위치 방향으로 회전할 벡터 계산
        FVector DirectionToOrigin = (OriginalLocation - GetActorLocation()).GetSafeNormal();
        TargetRotation = DirectionToOrigin.Rotation();
        TargetRotation.Pitch = 0.0f;
        TargetRotation.Roll = 0.0f;

        UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Starting rotation to origin direction"), *GetName());
    }
}

void ATurnEnemy::OnSkillMontageEnded()
{
    if (CurrentAttackState == EEnemyAttackState::PerformingSkill)
    {
        UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Skill montage finished - Ending turn"), *GetName());

        // 스킬 시퀀스 완료 - 바로 턴 종료 (이동 없이)
        FinishEnemyTurn();
    }
}

void ATurnEnemy::SetAssignedTarget(ACOECharacter* Target)
{
    AssignedTarget = Target;
    if (IsValid(Target))
    {
        UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Assigned target set to: %s"), *GetName(), *Target->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Assigned target cleared"), *GetName());
    }

}

void ATurnEnemy::OnSkillFire()
{
    UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s OnSkillFire - Spawning laser FX at fire socket"), *GetName());

    // Fire 단계: 발사 위치(RangedSocket)에 레이저 시작 FX 스폰
    if (SkillParticle)
    {
        UGameplayStatics::SpawnEmitterAttached(
            SkillParticle,
            GetMesh(),
            FName("RangedSocket"),  // 발사 소켓
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true  // Auto Destroy
        );
        UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Laser fire FX spawned at RangedSocket"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s SkillParticle is null - no laser fire FX"), *GetName());
    }
}

void ATurnEnemy::OnSkillFireSpawn()
{
    UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s OnSkillFireSpawn - Spawning impact at target location"), *GetName());

    // Fire_Spawn 단계: 저장된 타겟 위치에 Impact 스폰 + 데미지 적용
    if (ImpactParticle)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            ImpactParticle,
            SkillTargetLocation,  // 미리 저장된 타겟 위치
            FRotator::ZeroRotator,
            FVector(1.5f), // 스킬이므로 임팩트를 조금 크게
            true  // Auto Destroy
        );
        UE_LOG(LogTemp, Log, TEXT("[TurnEnemy] %s Laser impact FX spawned at target location"), *GetName());
    }

    // 데미지 적용 (타겟이 아직 살아있고 유효하면)
    if (CurrentAttackTarget.IsValid())
    {
        ACOECharacter* Target = CurrentAttackTarget.Get();
        UGameplayStatics::ApplyDamage(
            Target,
            LaserBeamDamage,
            GetController(),
            this,
            nullptr
        );
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Applied laser damage (%.1f) to %s"),
            *GetName(), LaserBeamDamage, *Target->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s Target is no longer valid for damage application"), *GetName());
    }
}

void ATurnEnemy::OnSkillFireEnd()
{
    UE_LOG(LogTemp, Warning, TEXT("[TurnEnemy] %s OnSkillFireEnd - Starting rotation to original direction"), *GetName());

    // Fire_End 단계: 원래 정면 방향으로 회전 시작
    TargetRotation = OriginalRotation; // 원래 바라보던 방향으로 설정

    // 원래 방향으로 회전 타이머 시작
    GetWorld()->GetTimerManager().SetTimer(
        MovementTimerHandle,
        this,
        &ATurnEnemy::UpdateSkillEndRotation,
        0.016f, // 약 60fps
        true
    );
}
