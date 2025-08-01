// Fill out your copyright notice in the Description page of Project Settings.


#include "DetectPlayerComponent.h"
#include "GameFramework/Actor.h"
#include "ExplorationPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "BaseCode/COEGameInstance.h"
#include "ExplorationEnemy.h"
#include "GameFramework/CharacterMovementComponent.h"


UDetectPlayerComponent::UDetectPlayerComponent()
{
   
}

void UDetectPlayerComponent::BeginPlay()
{
    Super::BeginPlay();
    // 충돌 반경설정
    SetSphereRadius(100.f);
    SetCollisionProfileName(TEXT("OverlapAllDynamic"));

    // Bind overlap events
    OnComponentBeginOverlap.AddDynamic(this, &UDetectPlayerComponent::HandleBeginOverlap);
    OnComponentEndOverlap.AddDynamic(this, &UDetectPlayerComponent::HandleEndOverlap);
}

void UDetectPlayerComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (Cast<AExplorationPlayer>(OtherActor))
    {
        OnPlayerDetected.Broadcast(OtherActor);
        UE_LOG(LogTemp, Log, TEXT("Detect"));

        auto* Enemy = Cast<AExplorationEnemy>(GetOwner());
        //전투맵 리스트 확인
        if (Enemy->PossibleBattleLevels.Num() > 0)
        {
            //SelectedBattleMap에 전투맵 리스트 할당
            FName SelectedBattleMap = Enemy->PossibleBattleLevels[FMath::RandRange(0, Enemy->PossibleBattleLevels.Num() - 1)];
            //GameInstance에 전투 정보 저장
            if (UCOEGameInstance* GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this)))
            {
                FString CurrLevel = UGameplayStatics::GetCurrentLevelName(this, true);

                GI->bPlayerInitiative = false; // 감지당해 넘어가는 것이므로 적이 우선
                GI->bPlayerWasDetected = true;
                GI->ReturnLocation = OtherActor->GetActorLocation();
                GI->ReturnMapName = FName(*CurrLevel); // 실제 탐색맵 이름으로 바꿔야 함
                GI->EnemyToRemoveName.AddUnique(Enemy->GetFName());
            }

            //전투맵으로 이동
            UGameplayStatics::OpenLevel(this, SelectedBattleMap);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Enemy has no PossibleBattleLevels!"));
        }
    }
}





void UDetectPlayerComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (Cast<AExplorationPlayer>(OtherActor))
    {
        OnPlayerLost.Broadcast(OtherActor);
        UE_LOG(LogTemp, Log, TEXT("UnDetect"));
    }
}
