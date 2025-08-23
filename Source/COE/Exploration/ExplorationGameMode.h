// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCode/COEGameMode.h"
#include "BaseCode/FCharacterStats.h"
#include "ExplorationGameMode.generated.h"



/**
 * 
 */
UCLASS()
class COE_API AExplorationGameMode : public ACOEGameMode
{
	GENERATED_BODY()
	
public:
    AExplorationGameMode();

protected:
    /** 변경 가능한 캐릭터 블루프린트 클래스들 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Switching")
    TArray<TSubclassOf<class AExplorationPlayer>> SwitchableCharacterClasses;

    /** 현재 캐릭터 인덱스 */
    UPROPERTY(BlueprintReadOnly, Category = "Character Switching")
    int32 CurrentCharacterIndex = 0;

public:
    /** 플레이어 캐릭터 변경 함수 */
    UFUNCTION(BlueprintCallable, Category = "Character Switching")
    void SwitchPlayerCharacter(const FVector& SpawnLocation, const FRotator& SpawnRotation);

    /** 현재 플레이어의 스탯/데이터 저장 */
    UFUNCTION(BlueprintCallable, Category = "Character Switching")
    void SavePlayerData();

    /** 새 캐릭터에 저장된 데이터 복원 */
    UFUNCTION(BlueprintCallable, Category = "Character Switching")
    void RestorePlayerData(AExplorationPlayer* NewPlayer);

private:
    /** 저장된 플레이어 데이터 */
    UPROPERTY()
    FCharacterStats SavedCharacterStats;

    UPROPERTY()
    TArray<TObjectPtr<ACharacter>> SavedPartyMembers;

};
