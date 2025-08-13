// Fill out your copyright notice in the Description page of Project Settings.


#include "Turn/TurnGameMode.h"
#include "EngineUtils.h"
#include "TurnEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "BaseCode/COEGameInstance.h"
#include "BaseCode/COECharacter.h"       // 전투 참가자 베이스
#include "TurnCombatBridgeComponent.h"   // 브리지
#include "CombatManager.h"               // 매니저
#include "TimerManager.h"                // 지연 호출용

void ATurnGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    // TurnEnemy 클래스 모두 찾아 바인딩 
    for (TActorIterator<ATurnEnemy> It(GetWorld()); It; ++It)
    {
        ATurnEnemy* Enemy = *It;
        if (Enemy)
        {
            RemainingEnemies++;
            //Enemy가 OnDaed.Broadcast()를 호출할 때마다 등록된 ATurnGameMode::OnEnemyDied가 실행되도록
            Enemy->OnDead.AddUObject(this, &ATurnGameMode::OnEnemyDied);
        }
    }

    // 만약 적이 하나도 없으면 즉시 종료 처리
    CheckEndOfCombat();
    // ---------------------------기존코드 구분----------------------------//
 // 1) CombatManager 찾기 (배치되어 있다고 가정)
    ACombatManager* Manager = nullptr;
    for (TActorIterator<ACombatManager> It(GetWorld()); It; ++It)
    {
        Manager = *It; break;
    }
    if (!Manager)
    {
        UE_LOG(LogTemp, Error, TEXT("[TurnGM] CombatManager not found in level"));
        return;
    }

    // 2) 모든 전투 캐릭터의 브리지 초기화 -> 팀/생존/이니셔티브 세팅 + 참가자 등록 + 델리게이트 바인딩
    int32 InitCount = 0;
    for (TActorIterator<ACOECharacter> It(GetWorld()); It; ++It)
    {
        if (auto* Bridge = It->FindComponentByClass<UTurnCombatBridgeComponent>())
        {
            Bridge->InitializeForCombat();
            InitCount++;
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("[TurnGM] Initialized Bridges: %d"), InitCount);

    // 3) 전투 시작 (지연 호출로 참가자 등록/바인딩 완료 보장)
    UE_LOG(LogTemp, Warning, TEXT("[TurnGM] StartCombat() scheduled (0.1s)"));
    FTimerHandle StartCombatHandle;
    GetWorld()->GetTimerManager().SetTimer(
        StartCombatHandle,
        [Manager]()
        {
            if (!Manager) return;
            UE_LOG(LogTemp, Warning, TEXT("[TurnGM] StartCombat()"));
            Manager->StartCombat(); // 여기서 단 한 번만 호출
        },
        0.1f, false
    );


}

void ATurnGameMode::OnEnemyDied()
{
    RemainingEnemies--;
    UE_LOG(LogTemp, Log, TEXT("[TurnGameMode] OnEnemyDied → RemainingEnemies = %d"), RemainingEnemies);
    CheckEndOfCombat();
}

void ATurnGameMode::CheckEndOfCombat()
{
    if (RemainingEnemies > 0)
        return;

    UE_LOG(LogTemp, Warning, TEXT("[TurnGM] CheckEndOfCombat: RemainingEnemies=%d"), RemainingEnemies);

    // -- 1) GameInstance를 안전하게 가져오기 --
    UCOEGameInstance* GI = GetWorld()->GetGameInstance<UCOEGameInstance>();
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("[TurnGM] GameInstance 캐스트 실패!"));
        return;
    }

    // -- 2) 저장된 맵 이름 유효성 체크(Optional) --
    if (GI->ReturnMapName.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("[TurnGM] ReturnMapName이 설정되지 않았습니다."));
        return;
    }

    // -- 3) 레벨 전환은 여기서 단 한 번만 --
    UE_LOG(LogTemp, Warning, TEXT("[TurnGM] 레벨 전환 시도: %s"), *GI->ReturnMapName.ToString());
    UGameplayStatics::OpenLevel(GetWorld(), GI->ReturnMapName);
}