// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnCombatBridgeComponent.h"
#include "CombatManager.h"              // ACombatManager
#include "BaseCode/COEGameInstance.h"            // UCOEGameInstance
#include "BaseCode/COECharacter.h"               // ACOECharacter
#include "Kismet/GameplayStatics.h"     // GetGameInstance, GetAllActorsOfClass
#include "GameFramework/Actor.h"
#include "TurnPlayer.h"
#include "TurnEnemy.h"
#include "TurnHudWidget.h"
#include "TimerManager.h" 
#include "EngineUtils.h" 
UTurnCombatBridgeComponent::UTurnCombatBridgeComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // 이벤트 드리븐
}

UTurnCombatBridgeComponent::~UTurnCombatBridgeComponent()
{
    // 소멸자에서 델리게이트 정리
    UnbindFromManagerDelegates();
}

void UTurnCombatBridgeComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<ACOECharacter>(GetOwner());      // 소유자 캐릭터 캐시
    GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this)); // GI 캐시

    FindCombatManager();                                    // 매니저 탐색
    BindToManagerDelegates();                               // 델리게이트 연결(있으면)
    UE_LOG(LogTemp, Warning, TEXT("[Bridge] Bound delegates: %s"), *OwnerCharacter->GetName());
    if (bAutoRegisterOnBeginPlay)
    {
        InitializeForCombat();                              // 자동 등록/세팅
    }
}

void UTurnCombatBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // EndPlay에서도 안전하게 해제
    UnbindFromManagerDelegates();

    // 타이머도 정리
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearAllTimersForObject(this);
    }

    Super::EndPlay(EndPlayReason);
}

void UTurnCombatBridgeComponent::InitializeForCombat()
{
    if (!OwnerCharacter) return;                            // 캐릭터가 아닐 경우 조기 종료

    if (!GI)
    {
        GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this));
    }
    if (GI)
    {
        GI->AutoAssignTeam(OwnerCharacter);                // TurnPlayer→Player, TurnEnemy→Enemy
        GI->SetAlive(OwnerCharacter, true);                // 전투 시작 시 생존 처리
    }

    if (!Manager) FindCombatManager();
    if (Manager && OwnerCharacter)
    {
        Manager->RegisterParticipant(OwnerCharacter);      // 전투 참가 등록
        UE_LOG(LogTemp, Warning, TEXT("[Bridge] RegisterParticipant"));
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

    // TODO: 캐릭터의 실제 스킬 함수 바인딩 지점
    // 예) if (SkillId=="Q") OwnerCharacter->CastQ(Target);

    if (SkillId == TEXT("RightClickRanged"))
    {
        UseRangedBasic(Target, 10.f);                      // 즉시형 예시로 처리
        return;
    }

    // 지연형 스킬은 애니메이션/투사체 종료 시점에서 NotifySkillFinished() 호출 필요
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
        // 타깃이 없으면 그냥 턴만 종료해 플로우가 멈추지 않게 함
        UE_LOG(LogTemp, Warning, TEXT("[Bridge] Target invalid. Ending turn without damage."));
        Manager->NotifyTurnActionEnd();
        return;
    }

    // 즉시형: 데미지 적용 → 턴 종료
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
    // 애님/투사체 종료 등 액션 완료 시점에서 호출 → 다음 턴으로 진행
    Manager->NotifyTurnActionEnd();
}

void UTurnCombatBridgeComponent::MarkDead(bool bEndTurnIfMine)
{
    if (!OwnerCharacter || !GI)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Bridge] MarkDead: Missing Owner/GI"));
        return;
    }
    GI->SetAlive(OwnerCharacter, false);                   // GI에 사망 반영(승패 판정 근거)

    if (bEndTurnIfMine && Manager && Manager->GetActiveCharacter() == OwnerCharacter)
    {
        // 내 턴 도중 즉시 사망했다면 턴을 밀어 진행 정체 방지
        Manager->NotifyTurnActionEnd();
    }
}

void UTurnCombatBridgeComponent::FindCombatManager()
{
    if (Manager) return;                                   // 이미 찾았으면 스킵

    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(this, ACombatManager::StaticClass(), Found);
    Manager = Found.Num() > 0 ? Cast<ACombatManager>(Found[0]) : nullptr; // 전투 맵에 하나라고 가정

    if (!Manager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Bridge] CombatManager not found in level"));
    }
}

void UTurnCombatBridgeComponent::BindToManagerDelegates()
{
    if (!Manager) return;

    // 이미 바인딩되어 있는지 확인 후 바인딩
    if (!bDelegatesBound)
    {
        Manager->OnTurnStarted.AddDynamic(this, &UTurnCombatBridgeComponent::HandleTurnStarted);
        Manager->OnTurnEnded.AddDynamic(this, &UTurnCombatBridgeComponent::HandleTurnEnded);
        bDelegatesBound = true;
        UE_LOG(LogTemp, Log, TEXT("[Bridge] Delegates bound for %s"), *OwnerCharacter->GetName());
    }
}

void UTurnCombatBridgeComponent::UnbindFromManagerDelegates()
{
    if (!Manager) return;

    // 바인딩되어 있을 때만 해제
    if (bDelegatesBound)
    {
        Manager->OnTurnStarted.RemoveDynamic(this, &UTurnCombatBridgeComponent::HandleTurnStarted);
        Manager->OnTurnEnded.RemoveDynamic(this, &UTurnCombatBridgeComponent::HandleTurnEnded);
        bDelegatesBound = false;
        UE_LOG(LogTemp, Log, TEXT("[Bridge] Delegates unbound for %s"), *OwnerCharacter->GetName());
    }
}

void UTurnCombatBridgeComponent::HandleTurnStarted(ACOECharacter* ActiveCharacter, int32 Round)
{
    // 유효성 검사 강화
    if (!IsValid(ActiveCharacter) || !IsValid(OwnerCharacter) || HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
    {
        return;
    }

    if (!GI) GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this));
    if (!GI) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    // --- Part 1: 현재 플레이어가 조종하는 캐릭터의 Bridge만 카메라/UI/Possess를 제어 ---
    if (PC->GetPawn() == OwnerCharacter)
    {
        const ECombatTeam ActiveCharacterTeam = GI->GetTeam(ActiveCharacter);

        // 모든 플레이어 HUD를 일단 숨김
        for (TActorIterator<ATurnPlayer> It(GetWorld()); It; ++It)
        {
            if (ATurnPlayer* Player = *It)
            {
                Player->SetTurnHudVisible(false);
                Player->SetTurnHudMode(ETurnHudMode::None);
            }
        }

        if (ActiveCharacterTeam == ECombatTeam::Player)
        {
            // [아군 턴 로직]
            if (APawn* PawnToPossess = Cast<APawn>(ActiveCharacter))
            {
                PC->SetViewTargetWithBlend(PawnToPossess, 1.2f, EViewTargetBlendFunction::VTBlend_Cubic);

                FTimerHandle PossessDelayHandle;
                GetWorld()->GetTimerManager().SetTimer(PossessDelayHandle, [this, PC, PawnToPossess]() {
                    if (IsValid(PawnToPossess) && IsValid(PC))
                    {
                        PC->Possess(PawnToPossess);
                        if (auto* TP = Cast<ATurnPlayer>(PawnToPossess))
                        {
                            TP->UpdateCursor();
                            TP->SetTurnHudVisible(true);
                            TP->SetTurnHudMode(ETurnHudMode::PlayerTurn);
                        }
                    }
                    }, 1.3f, false);
            }
        }
        else // [적 턴 로직]
        {
            // ★ 핵심 수정: Enemy가 미리 자신의 타겟을 결정하도록 함
            if (auto* TurnEnemy = Cast<ATurnEnemy>(ActiveCharacter))
            {
                // Enemy가 공격할 타겟을 미리 선정
                const TArray<ACOECharacter*> AlivePlayers = GI->GetAliveTeamMembers(ECombatTeam::Player);

                if (AlivePlayers.Num() > 0)
                {
                    // 이 Enemy가 공격할 타겟 랜덤 선택
                    ACOECharacter* EnemyTarget = AlivePlayers[FMath::RandRange(0, AlivePlayers.Num() - 1)];

                    // Enemy에게 타겟 미리 설정 (중요!)
                    TurnEnemy->SetAssignedTarget(EnemyTarget);

                    // GameInstance에도 방어자로 설정 (동일한 타겟)
                    GI->CurrentDefendingPlayer = EnemyTarget;

                    UE_LOG(LogTemp, Warning, TEXT("[Bridge] %s will attack %s. Setting up defense for %s."),
                        *TurnEnemy->GetName(), *EnemyTarget->GetName(), *EnemyTarget->GetName());

                    // 공격 대상이 될 플레이어에게 카메라 및 조종권 이전
                    PC->SetViewTargetWithBlend(EnemyTarget, 0.8f, EViewTargetBlendFunction::VTBlend_Cubic);

                    if (APawn* PawnToPossess = Cast<APawn>(EnemyTarget))
                    {
                        FTimerHandle PossessDelayHandle;
                        GetWorld()->GetTimerManager().SetTimer(PossessDelayHandle, [this, PC, PawnToPossess, EnemyTarget]() {
                            if (IsValid(PawnToPossess) && IsValid(PC))
                            {
                                PC->Possess(PawnToPossess);
                                if (auto* TP = Cast<ATurnPlayer>(PawnToPossess))
                                {
                                    TP->UpdateCursor();
                                    TP->SetTurnHudVisible(true);
                                    TP->SetTurnHudMode(ETurnHudMode::EnemyTurn);
                                    UE_LOG(LogTemp, Warning, TEXT("[Bridge] Defense mode activated for TARGET: %s"), *EnemyTarget->GetName());
                                }
                            }
                            }, 0.85f, false);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[Bridge] No alive players found for enemy turn!"));
                }
            }
        }
    }

    // --- Part 2: 턴을 받은 캐릭터의 Bridge는 자신의 행동만 개시 ---
    if (ActiveCharacter == OwnerCharacter)
    {
        OnMyTurnStarted.Broadcast(Round);
        UE_LOG(LogTemp, Log, TEXT("[Bridge] My Turn START (Round %d) : %s"), Round, *OwnerCharacter->GetName());

        // Enemy라면 AI 행동 시작
        if (auto* TurnEnemy = Cast<ATurnEnemy>(ActiveCharacter))
        {
            // 타이밍 수정: Enemy 공격을 충분히 지연시켜서 방어 모드 설정 완료 대기
            FTimerHandle EnemyAttackDelayHandle;
            GetWorld()->GetTimerManager().SetTimer(EnemyAttackDelayHandle,
                [this, TurnEnemy]()
                {
                    if (!IsValid(this) || !IsValid(TurnEnemy)) return;

                    // 이미 Part 1에서 설정된 타겟을 사용하여 공격
                    if (TurnEnemy->AssignedTarget.IsValid())
                    {
                        ACOECharacter* TargetToAttack = TurnEnemy->AssignedTarget.Get();
                        UE_LOG(LogTemp, Warning, TEXT("[Bridge] %s attacking PRE-ASSIGNED target %s."),
                            *TurnEnemy->GetName(), *TargetToAttack->GetName());
                        TurnEnemy->ExecuteEnemyTurnWithTarget(TargetToAttack);
                    }
                    else
                    {
                        // 백업: 할당된 타겟이 없으면 일반 로직 사용
                        UE_LOG(LogTemp, Warning, TEXT("[Bridge] No pre-assigned target. Using fallback logic."));
                        TurnEnemy->ExecuteEnemyTurn();
                    }
                },
                1.5f, false
            );
        }
    }
}


void UTurnCombatBridgeComponent::HandleTurnEnded(ACOECharacter* ActiveCharacter, int32 Round)
{
    if (!IsValid(ActiveCharacter) || !IsValid(OwnerCharacter))
    {
        return;
    }

    // 컴포넌트가 파괴 중인지 확인
    if (HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
    {
        return;
    }

    if (ActiveCharacter == OwnerCharacter)
    {
        // 내 턴 종료: HUD 숨김
        if (auto* TP = Cast<ATurnPlayer>(OwnerCharacter))
        {
            if (IsValid(TP) && TP->TurnHudWidget && IsValid(TP->TurnHudWidget))
            {
                TP->SetTurnHudMode(ETurnHudMode::None);
                UE_LOG(LogTemp, Log, TEXT("[Bridge] HUD hidden for: %s"), *TP->GetName());
            }
        }

        // 델리게이트 브로드캐스트 전 유효성 재확인
        if (IsValid(this) && !HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
        {
            OnMyTurnEnded.Broadcast(Round);
            UE_LOG(LogTemp, Log, TEXT("[Bridge] My Turn END   (Round %d) : %s"), Round, *OwnerCharacter->GetName());
        }
    }
     else
    {
        // 다른 캐릭터의 턴이 종료된 경우, 내가 TurnPlayer라면 HUD 상태 업데이트
        if (auto* TP = Cast<ATurnPlayer>(OwnerCharacter))
        {
            // 안전한 지연 호출로 HUD 상태 업데이트
            FTimerHandle UpdateHandle;
            GetWorld()->GetTimerManager().SetTimer(UpdateHandle,
                [this, TP]()
                {
                    if (IsValid(this) && IsValid(TP) && !HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
                    {
                        TP->UpdateHudForTurnState();
                    }
                }, 0.1f, false);
        }
    }
}