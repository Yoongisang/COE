// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCode/COECharacter.h"
#include "ExplorationPlayer.generated.h"

/**
 * 
 */
UCLASS()
class COE_API AExplorationPlayer : public ACOECharacter
{
	GENERATED_BODY()

public:
	/** 파티 Array */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	TArray<TObjectPtr<ACharacter>> PartyMembers;

	TObjectPtr<class UCOEGameInstance> GI;

public:
	virtual void BeginPlay() override;

public:

	/** 파티 HP 전체 회복 */
	UFUNCTION(BlueprintCallable, Category = "Consumable|Exploration")
	void UseExplorationFullHeal();

private:

	/** GI 호출 헬퍼 */
	UCOEGameInstance* GetCOEGameInstance() const;

	/** HP 회복 범위 보정 */
	void ClampSelfHP() { CharacterStats.CurrentHP = FMath::Clamp(CharacterStats.CurrentHP, 0.f, CharacterStats.MAXHP); }
};