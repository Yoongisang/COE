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
    // �浹 �ݰ漳��
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
        //������ ����Ʈ Ȯ��
        if (Enemy->PossibleBattleLevels.Num() > 0)
        {
            //SelectedBattleMap�� ������ ����Ʈ �Ҵ�
            FName SelectedBattleMap = Enemy->PossibleBattleLevels[FMath::RandRange(0, Enemy->PossibleBattleLevels.Num() - 1)];
            //GameInstance�� ���� ���� ����
            if (UCOEGameInstance* GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this)))
            {
                FString CurrLevel = UGameplayStatics::GetCurrentLevelName(this, true);

                GI->bPlayerInitiative = false; // �������� �Ѿ�� ���̹Ƿ� ���� �켱
                GI->bPlayerWasDetected = true;
                GI->ReturnLocation = OtherActor->GetActorLocation();
                GI->ReturnMapName = FName(*CurrLevel); // ���� Ž���� �̸����� �ٲ�� ��
                GI->EnemyToRemoveName.AddUnique(Enemy->GetFName());
            }

            //���������� �̵�
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
