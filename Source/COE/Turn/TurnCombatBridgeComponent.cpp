// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnCombatBridgeComponent.h"
#include "CombatManager.h"              // ACombatManager
#include "BaseCode/COEGameInstance.h"            // UCOEGameInstance
#include "BaseCode/COECharacter.h"               // ACOECharacter
#include "Kismet/GameplayStatics.h"     // GetGameInstance, GetAllActorsOfClass
#include "GameFramework/Actor.h"
#include "TurnPlayer.h"
#include "TurnEnemy.h"

UTurnCombatBridgeComponent::UTurnCombatBridgeComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // 이벤트 드리븐
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
    UnbindFromManagerDelegates();                           // 안전한 해제
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

    // 델리게이트는 동일 바인딩 중복을 피하기 위해 바인딩 전에 언바인드 권장
    UnbindFromManagerDelegates();

    Manager->OnTurnStarted.AddDynamic(this, &UTurnCombatBridgeComponent::HandleTurnStarted);
    Manager->OnTurnEnded.AddDynamic(this, &UTurnCombatBridgeComponent::HandleTurnEnded);
}

void UTurnCombatBridgeComponent::UnbindFromManagerDelegates()
{
    if (!Manager) return;

    Manager->OnTurnStarted.RemoveDynamic(this, &UTurnCombatBridgeComponent::HandleTurnStarted);
    Manager->OnTurnEnded.RemoveDynamic(this, &UTurnCombatBridgeComponent::HandleTurnEnded);
}

void UTurnCombatBridgeComponent::HandleTurnStarted(ACOECharacter* ActiveCharacter, int32 Round)
{
    if (!ActiveCharacter) return;

    if (!GI) GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this));
    if (!GI) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

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
            GetWorld()->GetTimerManager().SetTimer(PossessDelayHandle, [this, PC, Pawn]()
                {
                    PC->Possess(Pawn);

                    // 3단계: Possess 후 입력 설정
                    PC->SetInputMode(FInputModeGameOnly());
                    PC->bShowMouseCursor = true;
                    PC->SetIgnoreLookInput(false);

                    if (auto* TP = Cast<ATurnPlayer>(Pawn))
                    {
                        TP->UpdateCursor();
                    }

                }, 1.3f, false); // 블렌드 시간보다 약간 길게
        }
    }
    else
    {
        // Enemy 턴: 살아있는 Player 중 랜덤 선택해 카메라만 전환
        const TArray<ACOECharacter*> AlivePlayers = GI->GetAliveTeamMembers(ECombatTeam::Player);
        if (AlivePlayers.Num() > 0)
        {
            // 현재 ViewTarget이 살아있는 Player인지 확인
            AActor* CurrentTarget = PC->GetViewTarget();
            bool bCurrentTargetValid = false;

            for (ACOECharacter* Player : AlivePlayers)
            {
                if (CurrentTarget == Player)
                {
                    bCurrentTargetValid = true;
                    break;
                }
            }

            // 현재 타겟이 유효하지 않을 때만 카메라 이동
            if (!bCurrentTargetValid)
            {
                ACOECharacter* BestTarget = AlivePlayers[0]; // 첫 번째 살아있는 Player
                PC->SetViewTargetWithBlend(BestTarget, 0.8f, EViewTargetBlendFunction::VTBlend_Cubic);
                UE_LOG(LogTemp, Log, TEXT("[Camera] Enemy turn - switching to valid Player target: %s"),
                    *BestTarget->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("[Camera] Enemy turn - keeping current valid target"));
            }
        }

        // 새로 추가: Enemy 자동 행동 시작 
        if (auto* TurnEnemy = Cast<ATurnEnemy>(ActiveCharacter))
        {
            // 현재 활성 캐릭터와 이 컴포넌트의 소유자가 같을 때만 실행
            if (ActiveCharacter == OwnerCharacter)
            {
                UE_LOG(LogTemp, Warning, TEXT("[Bridge] Starting action for Enemy: %s"), *TurnEnemy->GetName());
                TurnEnemy->ExecuteEnemyTurn();
            }
        }
    }


    if (ActiveCharacter == OwnerCharacter)
    {
        // 내 턴 시작: UI 열기, 입력 허용, AI 트리거 등은 BP에서 바인딩하여 처리
        OnMyTurnStarted.Broadcast(Round);
        UE_LOG(LogTemp, Log, TEXT("[Bridge] My Turn START (Round %d) : %s"), Round, *OwnerCharacter->GetName());
    }
}

void UTurnCombatBridgeComponent::HandleTurnEnded(ACOECharacter* ActiveCharacter, int32 Round)
{
    if (ActiveCharacter == OwnerCharacter)
    {
        // 내 턴 종료: UI 닫기, 입력 잠금 등은 BP에서 처리
        OnMyTurnEnded.Broadcast(Round);
        UE_LOG(LogTemp, Log, TEXT("[Bridge] My Turn END   (Round %d) : %s"), Round, *OwnerCharacter->GetName());
    }
}


