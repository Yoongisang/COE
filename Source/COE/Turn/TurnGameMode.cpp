// Fill out your copyright notice in the Description page of Project Settings.


#include "Turn/TurnGameMode.h"
#include "EngineUtils.h"
#include "TurnEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "BaseCode/COEGameInstance.h"

void ATurnGameMode::BeginPlay()
{
    Super::BeginPlay();

    // TurnEnemy Ŭ���� ��� ã�� ���ε�
    for (TActorIterator<ATurnEnemy> It(GetWorld()); It; ++It)
    {
        ATurnEnemy* Enemy = *It;
        if (Enemy)
        {
            RemainingEnemies++;
            //Enemy�� OnDaed.Broadcast()�� ȣ���� ������ ��ϵ� ATurnGameMode::OnEnemyDied�� ����ǵ���
            Enemy->OnDead.AddUObject(this, &ATurnGameMode::OnEnemyDied);
        }
    }

    // ���� ���� �ϳ��� ������ ��� ���� ó��
    CheckEndOfCombat();
}

void ATurnGameMode::OnEnemyDied()
{
    RemainingEnemies--;
    UE_LOG(LogTemp, Log, TEXT("[TurnGameMode] OnEnemyDied �� RemainingEnemies = %d"), RemainingEnemies);
    CheckEndOfCombat();
}

void ATurnGameMode::CheckEndOfCombat()
{
    if (RemainingEnemies > 0)
        return;

    UE_LOG(LogTemp, Warning, TEXT("[TurnGM] CheckEndOfCombat: RemainingEnemies=%d"), RemainingEnemies);

    // -- 1) GameInstance�� �����ϰ� �������� --
    UCOEGameInstance* GI = GetWorld()->GetGameInstance<UCOEGameInstance>();
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("[TurnGM] GameInstance ĳ��Ʈ ����!"));
        return;
    }

    // -- 2) ����� �� �̸� ��ȿ�� üũ(Optional) --
    if (GI->ReturnMapName.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("[TurnGM] ReturnMapName�� �������� �ʾҽ��ϴ�."));
        return;
    }

    // -- 3) ���� ��ȯ�� ���⼭ �� �� ���� --
    UE_LOG(LogTemp, Warning, TEXT("[TurnGM] ���� ��ȯ �õ�: %s"), *GI->ReturnMapName.ToString());
    UGameplayStatics::OpenLevel(GetWorld(), GI->ReturnMapName);
}