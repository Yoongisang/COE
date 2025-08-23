// Fill out your copyright notice in the Description page of Project Settings.


#include "Exploration/ExplorationGameMode.h"
#include "ExplorationPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "ExplorationPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"


AExplorationGameMode::AExplorationGameMode()
{
}

void AExplorationGameMode::SwitchPlayerCharacter(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
    if (SwitchableCharacterClasses.Num() <= 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExplorationGM] No other characters to switch to"));
        return;
    }

    // 현재 플레이어 컨트롤러 가져오기
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[ExplorationGM] PlayerController not found"));
        return;
    }

    // 현재 플레이어 데이터 저장
    SavePlayerData();

    // 기존 폰 저장 및 UnPossess
    APawn* OldPawn = PC->GetPawn();

    // 다음 캐릭터 인덱스 계산
    CurrentCharacterIndex = (CurrentCharacterIndex + 1) % SwitchableCharacterClasses.Num();

    // 새 DefaultPawnClass 설정
    if (SwitchableCharacterClasses.IsValidIndex(CurrentCharacterIndex))
    {
        DefaultPawnClass = SwitchableCharacterClasses[CurrentCharacterIndex];

        // 새 캐릭터 스폰
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        if (AExplorationPlayer* NewPlayer = GetWorld()->SpawnActor<AExplorationPlayer>(
            DefaultPawnClass, SpawnLocation, SpawnRotation, SpawnParams))
        {
            // 저장된 데이터 복원
            RestorePlayerData(NewPlayer);

            NewPlayer->SetActorLocation(SpawnLocation, false, nullptr, ETeleportType::TeleportPhysics);
            NewPlayer->SetActorRotation(SpawnRotation, ETeleportType::TeleportPhysics);

            // 즉시 지면에 붙이기
            if (UCharacterMovementComponent* MovementComp = NewPlayer->GetCharacterMovement())
            {
                MovementComp->SetMovementMode(MOVE_Walking);
                MovementComp->ForceReplicationUpdate();  // 네트워크 업데이트 강제
            }

            // 즉시 UnPossess → Possess
            PC->UnPossess();
            PC->Possess(NewPlayer);
          
            // ExplorationPlayerController에 새 캐릭터 설정
            if (auto* ExplorationPC = Cast<AExplorationPlayerController>(PC))
            {
                ExplorationPC->ExplorationChar = NewPlayer;
                ExplorationPC->COEChar = NewPlayer; 
                UE_LOG(LogTemp, Warning, TEXT("[ExplorationGM] Both chars updated"));
            }

            // 다음 프레임에서 한 번 더 위치 보정
            FTimerHandle LocationFixHandle;
            GetWorld()->GetTimerManager().SetTimer(LocationFixHandle,
                [NewPlayer, SpawnLocation]()
                {
                    if (IsValid(NewPlayer))
                    {
                        NewPlayer->SetActorLocation(SpawnLocation, false, nullptr, ETeleportType::TeleportPhysics);
                    }
                },
                0.01f, false);  // 아주 짧은 지연
            
            // 기존 폰 제거
            if (IsValid(OldPawn))
            {
                OldPawn->Destroy();
            }
            
            UE_LOG(LogTemp, Log, TEXT("[ExplorationGM] Switched to character index: %d"), CurrentCharacterIndex);
        }
    }
}

void AExplorationGameMode::SavePlayerData()
{
    if (AExplorationPlayer* CurrentPlayer = Cast<AExplorationPlayer>(
        UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        SavedCharacterStats = CurrentPlayer->CharacterStats;
        SavedPartyMembers = CurrentPlayer->PartyMembers;
        UE_LOG(LogTemp, Log, TEXT("[ExplorationGM] Player data saved"));
    }
}

void AExplorationGameMode::RestorePlayerData(AExplorationPlayer* NewPlayer)
{
    if (NewPlayer)
    {
        NewPlayer->CharacterStats = SavedCharacterStats;
        NewPlayer->PartyMembers = SavedPartyMembers;
        UE_LOG(LogTemp, Log, TEXT("[ExplorationGM] Player data restored"));
    }
}
