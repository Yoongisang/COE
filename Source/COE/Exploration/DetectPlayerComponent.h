// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "DetectPlayerComponent.generated.h"

//  Acotr�� Ž�����Ǿ �������� Delegate ȣ��
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDetectedSignature, AActor*, OtherActor)
// Acotr�� Ž�����Ǿ�� ������� Delegate ȣ��
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

    /** �÷��̾ Ž�� ������ */
    UPROPERTY(BlueprintAssignable, Category = "Detection")
    FOnDetectedSignature OnPlayerDetected;

    /** �÷��̾ Ž������ ������� */
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
