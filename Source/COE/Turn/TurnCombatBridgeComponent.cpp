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
    if (!IsValid(ActiveCharacter) || !IsValid(OwnerCharacter))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Bridge] HandleTurnStarted: Invalid character pointers"));
        return;
    }

    // 컴포넌트가 파괴 중인지 확인
    if (HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Bridge] HandleTurnStarted: Component is being destroyed"));
        return;
    }

    if (!GI) GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this));
    if (!GI)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Bridge] HandleTurnStarted: No GameInstance"));
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Bridge] HandleTurnStarted: No PlayerController"));
        return;
    }

    // 활성 캐릭터의 브리지만 아래 로직을 실행
    if (ActiveCharacter != OwnerCharacter)
    {
        return;
    }

    const ECombatTeam Team = GI->GetTeam(ActiveCharacter);


    if (Team == ECombatTeam::Player)
    {
        // Player 턴: 카메라 전환 → 딜레이 → Possess 순서로 처리
        if (APawn* Pawn = Cast<APawn>(ActiveCharacter))
        {
            // 1단계: 먼저 부드럽게 카메라만 이동
            PC->SetViewTargetWithBlend(ActiveCharacter, 1.2f, EViewTargetBlendFunction::VTBlend_Cubic);

            // 2단계: 카메라 블렌드 완료 후 Possess (지연 실행)
            FTimerHandle PossessDelayHandle;
            GetWorld()->GetTimerManager().SetTimer(PossessDelayHandle,
                [this, PC, Pawn, ActiveCharacter, Round]()
                {
                    // 타이머 실행 시점에서 다시 유효성 검사
                    if (!IsValid(this) || !IsValid(PC) || !IsValid(Pawn) || !IsValid(ActiveCharacter))
                    {
                        return;
                    }

                    // 컴포넌트가 파괴 중인지 재확인
                    if (HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
                    {
                        return;
                    }

                    PC->Possess(Pawn);

                    // 3단계: Possess 후 입력 설정
                    PC->SetInputMode(FInputModeGameOnly());
                    PC->bShowMouseCursor = true;
                    PC->SetIgnoreLookInput(false);

                    if (auto* TP = Cast<ATurnPlayer>(Pawn))
                    {
                        TP->UpdateCursor();

                        // **오직 활성 캐릭터의 HUD만 표시**
                        FTimerHandle HudShowHandle;
                        GetWorld()->GetTimerManager().SetTimer(HudShowHandle,
                            [TP]()
                            {
                                if (IsValid(TP))
                                {
                                    TP->SetTurnHudVisible(true);
                                    TP->SetTurnHudMode(ETurnHudMode::PlayerTurn);
                                    TP->RefreshTurnHud();
                                    UE_LOG(LogTemp, Warning, TEXT("[Bridge] ONLY Active Player HUD shown for: %s"), *TP->GetName());
                                }
                            }, 0.1f, false);
                    }

                    // 내 캐릭터의 턴이라면 추가 처리
                    if (ActiveCharacter == OwnerCharacter && IsValid(this))
                    {
                        OnMyTurnStarted.Broadcast(Round);
                        UE_LOG(LogTemp, Log, TEXT("[Bridge] My Turn START (Round %d) : %s"), Round, *OwnerCharacter->GetName());
                    }

                }, 1.3f, false); // 블렌드 시간보다 약간 길게
        }
		return;
    }
    else
    {
  
            // Enemy 턴: 살아있는 Player 중 랜덤 선택해 카메라 전환 + PlayerController도 Possess
            const TArray<ACOECharacter*> AlivePlayers = GI->GetAliveTeamMembers(ECombatTeam::Player);
            if (AlivePlayers.Num() > 0)
            {
                // 완전 랜덤 선택 (현재 타겟 상관없이)
                ACOECharacter* RandomTarget = AlivePlayers[FMath::RandRange(0, AlivePlayers.Num() - 1)];

                PC->SetViewTargetWithBlend(RandomTarget, 0.8f, EViewTargetBlendFunction::VTBlend_Cubic);
                UE_LOG(LogTemp, Log, TEXT("[Camera] Enemy turn - random target selected: %s"),
                    *RandomTarget->GetName());

                // 2) 블렌드가 끝나갈 타이밍에 해당 아군 Pawn을 Possess
                if (APawn* TargetPawn = Cast<APawn>(RandomTarget))
                {
                    FTimerHandle PossessDelayHandle;
                    GetWorld()->GetTimerManager().SetTimer(PossessDelayHandle,
                        [this, PC, TargetPawn, RandomTarget]()
                        {
                            // 타이머 실행 시점에서 다시 유효성 검사
                            if (!IsValid(this) || !IsValid(PC) || !IsValid(TargetPawn) || !IsValid(RandomTarget))
                            {
                                return;
                            }

                            // 컴포넌트가 파괴 중인지 재확인
                            if (HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
                            {
                                return;
                            }

                            PC->Possess(TargetPawn);

                            // 3) 적 턴이므로 입력은 잠그고, 커서는 보이게 유지
                            PC->SetInputMode(FInputModeGameOnly());
                            PC->bShowMouseCursor = false;
                            PC->bEnableClickEvents = false;
                            PC->bEnableMouseOverEvents = false;
                            PC->SetIgnoreLookInput(true);

                            if (auto* TP = Cast<ATurnPlayer>(TargetPawn))
                            {
                                TP->UpdateCursor();

                                // HUD 표시 (적 턴, 방어 버튼만) - 안전한 지연 호출
                                FTimerHandle HudShowHandle;
                                GetWorld()->GetTimerManager().SetTimer(HudShowHandle,
                                    [TP]()
                                    {
                                        if (IsValid(TP))
                                        {
                                            TP->SetTurnHudVisible(true);
                                            TP->SetTurnHudMode(ETurnHudMode::EnemyTurn);
                                            TP->RefreshTurnHud();
                                            UE_LOG(LogTemp, Log, TEXT("[Bridge] Enemy turn HUD shown for: %s"), *TP->GetName());
                                        }
                                    }, 0.1f, false);
                            }
                        }, 0.85f, false); // 블렌드(0.8s)보다 약간 뒤에 Possess
                }
            }
    
        // Enemy 자동 행동 시작 (내가 활성 캐릭터일 때만)
        if (auto* TurnEnemy = Cast<ATurnEnemy>(ActiveCharacter))
        {
            if (ActiveCharacter == OwnerCharacter)
            {
                UE_LOG(LogTemp, Warning, TEXT("[Bridge] Starting action for Enemy: %s"), *TurnEnemy->GetName());
                TurnEnemy->ExecuteEnemyTurn();
            }
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


