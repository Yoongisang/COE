// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "DetectPlayerComponent.generated.h"

//  Acotr가 탐지스피어에 들어왔을때 Delegate 호출
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDetectedSignature, AActor*, OtherActor)
// Acotr가 탐지스피어에서 벗어났을때 Delegate 호출
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLostSignature, AActor*, OtherActor)
/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class COE_API UDetectPlayerComponent : public USphereComponent
{
	GENERATED_BODY()
public:

    UDetectPlayerComponent();

    /** 플레이어가 탐지 됐을때 */
    UPROPERTY(BlueprintAssignable, Category = "Detection")
    FOnDetectedSignature OnPlayerDetected;

    /** 플레이어가 탐지에서 벗어났을때 */
    UPROPERTY(BlueprintAssignable, Category = "Detection")
    FOnLostSignature OnPlayerLost;

protected:

    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void HandleEndOverlap(UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);
};
