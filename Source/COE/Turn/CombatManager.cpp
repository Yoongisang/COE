// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatManager.h"
#include "Kismet/GameplayStatics.h"
#include "BaseCode/COEGameInstance.h"
#include "BaseCode/COECharacter.h"

ACombatManager::ACombatManager()
{
    PrimaryActorTick.bCanEverTick = false; // 이벤트 드리븐 구조이므로 틱 불필요
}

void ACombatManager::BeginPlay()
{
    Super::BeginPlay();
    // 전역 전투 메타데이터를 보관하는 GameInstance를 캐싱해 매 프레임 조회를 피함
    GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this)); // GI는 null일 수 있음
}

void ACombatManager::EnterState(uint8 NewState)
{
    // CombatState의 단일 소스는 GameInstance. 상태 전이는 반드시 GI를 통해 기록
    if (GI)
    {
        GI->SetCombatState(static_cast<ECombatState>(NewState)); // (uint8→Enum) 안전 캐스팅
    }
}

void ACombatManager::StartCombat()
{
    Round = 0;                              // 라운드 카운터 초기화(0부터 시작)
    if (GI) GI->ResetCombatData();          // 이전 전투 잔여 메타데이터 초기화(팀은 정책에 따라 유지)

    BuildTurnOrder();                        // 참가자들을 이니셔티브 기준으로 정렬
    EnterState(static_cast<uint8>(ECombatState::Setup)); // 전투 준비 상태 진입
    OnCombatStarted.Broadcast();             // UI/사운드 등 외부 시스템에 전투 시작 신호
    BeginNextTurn();                         // 첫 턴 진입(인덱스는 BuildTurnOrder에서 -1로 초기화됨)
}

void ACombatManager::EndCombat(bool bPlayerWon)
{
    // 승패에 따라 상태 전환 후 외부에 통지
    EnterState(static_cast<uint8>(bPlayerWon ? ECombatState::Victory : ECombatState::Defeat));
    OnCombatEnded.Broadcast(bPlayerWon, Round); // 종료 시 라운드 수 전달
}

bool ACombatManager::RegisterParticipant(ACOECharacter* Character)
{
    // 전투 참가자 등록(약포인터 배열). 무효/중복 방지 처리 포함
    if (!IsValid(Character)) return false;            // 널/파괴 대기 상태 방지
    if (Participants.Contains(Character)) return false; // 이미 등록된 경우 무시
    Participants.Add(Character);                       // 약포인터로 저장 → GC/레벨 전환 안전
    return true;
}

bool ACombatManager::UnregisterParticipant(ACOECharacter* Character)
{
    // 전투 참가자 해제: Participants와 TurnOrder에서 모두 제거
    if (!IsValid(Character)) return false;
    const int32 Removed = Participants.RemoveAll([&](const TWeakObjectPtr<ACOECharacter>& P) { return P.Get() == Character; }); // 약포인터 비교로 제거
    TurnOrder.RemoveAll([&](const FTurnEntry& E) { return E.Character.Get() == Character; }); // 턴 큐에서도 제거(턴 도중 파괴 등 보호)
    return Removed > 0;                                 // 하나 이상 제거되었을 때만 true
}

void ACombatManager::BuildTurnOrder()
{
    // 현재 참가자/생존 상태를 기준으로 새 턴 큐 구성
    TurnOrder.Reset();
    for (const auto& P : Participants)                  // 약포인터 배열 순회
    {
        ACOECharacter* C = P.Get();                     // 약포인터 → 실제 포인터
        if (!IsValid(C)) continue;                      // 무효 참가자 스킵
        if (GI && !GI->IsAlive(C)) continue;            // 사망자는 턴 대상 제외
        const int32 Ini = GI ? GI->GetInitiative(C) : 10; // 이니셔티브 조회(기본 10)
        FTurnEntry E; E.Character = C; E.Initiative = Ini; E.TieBreak = FMath::Rand(); // 동률 시 TieBreak로 정렬 안정화
        TurnOrder.Add(E);
    }
    TurnOrder.Sort();                                    // 높은 Initiative 우선, 동률은 TieBreak 오름차순
    CurrentIndex = -1;                                   // BeginNextTurn에서 ++되어 0부터 시작하도록 준비
}

void ACombatManager::BeginNextTurn()
{
    // 턴 큐가 비어있으면 재생성(처음/라운드 롤오버/대거 사망 등)
    if (TurnOrder.Num() == 0)
    {
        BuildTurnOrder();
        if (TurnOrder.Num() == 0) { EndCombat(true); return; } // 보호: 참가자 전멸 등 비정상 상황 처리
    }

    CurrentIndex++;                                      // 다음 턴으로 이동(-1→0, 0→1 ...)
    if (CurrentIndex >= TurnOrder.Num())                 // 마지막 다음이면 라운드 롤오버
    {
        Round++;                                         // 라운드 증가
        BuildTurnOrder();                                // 사망/부활/버프 영향 반영해 재정렬
        CurrentIndex = 0;                                // 새 라운드 첫 참가자부터 시작
    }

    ACOECharacter* Active = GetActiveCharacter();        // 현재 턴 주체
    if (!IsValid(Active)) { BeginNextTurn(); return; }   // 파괴/무효면 스킵하고 다음 턴으로(안전 장치)

    EnterState(static_cast<uint8>(ECombatState::PreTurn)); // 입력 대기/연출 준비 단계
    OnTurnStarted.Broadcast(Active, Round);               // UI/사운드/로그 등 외부 시스템 통지
}

void ACombatManager::NotifyTurnActionEnd()
{
    // 캐릭터 쪽 액션(애님/투사체 등)이 완료된 시점에서 호출되는 엔드포인트
    if (ACOECharacter* Active = GetActiveCharacter())
    {
        OnTurnEnded.Broadcast(Active, Round);            // UI/사운드/로그 등 종료 처리 신호
    }

    CheckVictory();                                      // 액션 결과로 승패가 갈렸는지 확인
    if (GI && (GI->GetCombatState() == ECombatState::Victory || GI->GetCombatState() == ECombatState::Defeat))
    {
        return;                                          // 전투가 종료 상태면 더 진행하지 않음
    }

    EnterState(static_cast<uint8>(ECombatState::PostTurn)); // 후처리 단계 표시
    BeginNextTurn();                                      // 다음 턴으로 진행
}

void ACombatManager::ApplyDamageAndEndTurn(ACOECharacter* InstigatorCharacter, ACOECharacter* TargetCharacter, float Damage)
{
    // 즉시형 스킬용 편의 함수: 데미지를 적용하고 성공/실패와 관계없이 턴 종료
    if (!IsValid(TargetCharacter)) { NotifyTurnActionEnd(); return; } // 타깃 무효면 바로 턴 종료

    // UGameplayStatics::ApplyDamage는 대상의 TakeDamage를 호출하여 실제 체력/사망 처리를 위임
    AController* Ctrl = InstigatorCharacter ? InstigatorCharacter->GetController() : nullptr; // 가해자 컨트롤러(없을 수 있음)
    UGameplayStatics::ApplyDamage(TargetCharacter, Damage, Ctrl, InstigatorCharacter, nullptr); // DamageType은 추후 확장 가능

    // 사망 반영은 대상의 TakeDamage 내부에서 GI->SetAlive(this,false)로 적용하는 것을 권장
    NotifyTurnActionEnd();                                   // 즉시 턴 종료로 플로우 유지
}

ACOECharacter* ACombatManager::GetActiveCharacter() const
{
    // 현재 인덱스가 유효한지 검증 후 해당 엔트리의 캐릭터 반환
    if (!TurnOrder.IsValidIndex(CurrentIndex)) return nullptr; // 초기/에러 방지
    return TurnOrder[CurrentIndex].Character.Get();            // 약포인터 → 실제 포인터
}

void ACombatManager::CheckVictory()
{
    // GameInstance에 기록된 팀/생존 정보를 바탕으로 간단한 승패 판정
    if (!GI) return;                                        // GI 없으면 판정 불가
    const bool bPlayersAlive = GI->HasTeamAlive(ECombatTeam::Player); // 플레이어 팀 생존 여부
    const bool bEnemiesAlive = GI->HasTeamAlive(ECombatTeam::Enemy);   // 적 팀 생존 여부

    if (!bPlayersAlive && !bEnemiesAlive) { EndCombat(false); return; } // 양측 전멸: 임시로 패배 처리(규칙에 맞게 변경 가능)
    if (!bEnemiesAlive) { EndCombat(true); return; }                    // 적 전멸: 승리
    if (!bPlayersAlive) { EndCombat(false); return; }                   // 플레이어 전멸: 패배
}