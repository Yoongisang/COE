// Fill out your copyright notice in the Description page of Project Settings.


#include "Turn/TurnGameMode.h"
#include "EngineUtils.h"
#include "TurnEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "BaseCode/COEGameInstance.h"

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