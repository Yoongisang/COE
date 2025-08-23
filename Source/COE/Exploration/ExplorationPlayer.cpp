// Fill out your copyright notice in the Description page of Project Settings.


#include "ExplorationPlayer.h"
#include "BaseCode/COEAnimInstance.h"
#include "BaseCode/COEGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/InputSettings.h"
#include "ExplorationGameMode.h"

void AExplorationPlayer::BeginPlay()
{
    Super::BeginPlay();

  
    GI = GetCOEGameInstance();

    // PlayerController 설정 확인
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        // 입력 모드 설정
        PC->SetInputMode(FInputModeGameOnly());
        UE_LOG(LogTemp, Log, TEXT("[ExplorationPlayer] Controller properly set"));
    }
}

UCOEGameInstance* AExplorationPlayer::GetCOEGameInstance() const
{
    return GetGameInstance<UCOEGameInstance>();
}

void AExplorationPlayer::SaveCurrentTransform(FVector& OutLocation, FRotator& OutRotation) const
{
    OutLocation = GetActorLocation();
    OutRotation = GetActorRotation();
}

void AExplorationPlayer::UseExplorationFullHeal()
{
    if (GI)
    {
        if (!GI->TryConsumeExplorationHeal())
        {
            UE_LOG(LogTemp, Warning, TEXT("[ExplorationPlayer] No Exploration Heal left."));
            return;
        }

        auto HealFullIfHasStat = [](ACharacter* Char)
            {
                if (!Char) return;

                // 공용 스탯 컴포넌트/인터페이스가 있다면 그쪽으로 처리 권장.
                if (auto* EP = Cast<AExplorationPlayer>(Char))
                {
                    EP->CharacterStats.CurrentHP = EP->CharacterStats.MAXHP;
                    EP->ClampSelfHP();
                }
                // TurnPlayer 등 다른 타입은 IHealableInterface로 FullHeal 처리하는 게 베스트.
            };

        // 본인
        HealFullIfHasStat(this);

        // 파티 전원
        for (const auto& M : PartyMembers)
        {
            HealFullIfHasStat(M.Get());
        }

        UE_LOG(LogTemp, Log, TEXT("[ExplorationPlayer] Exploration full-heal used."));
    }
}

void AExplorationPlayer::SwitchToNextCharacter()
{
    // GameMode에 캐릭터 변경 요청
    if (AExplorationGameMode* GM = Cast<AExplorationGameMode>(GetWorld()->GetAuthGameMode()))
    {
        FVector CurrentLocation;
        FRotator CurrentRotation;
        SaveCurrentTransform(CurrentLocation, CurrentRotation);

        GM->SwitchPlayerCharacter(CurrentLocation, CurrentRotation);
    }
}
