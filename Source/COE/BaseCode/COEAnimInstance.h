// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "COEAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class COE_API UCOEAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
private:
	//움직임 체크 (Idle 판단)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	bool ShouldMove;
	//추락 체크 (점프 또는 추락 판단)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	bool IsFalling;
	//속도 벡터
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	FVector Velocity;
	//수평입력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	float Horizontal;
	//수직입력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	float Vertical;
	//XY 벡터크기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	float GroundSpeed;
	//조준 방향과 이동방향 좌우 각도 차이
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	float YawOffset;
	//캐릭터 조준 중인 회전값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	FRotator AimRotation;
public:
	//캐릭터 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<class ACOECharacter> Character;
	//캐릭터 움직임 컴포넌트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<class UCharacterMovementComponent> CharacterMovement;
public:
	UCOEAnimInstance();
	virtual void NativeInitializeAnimation() override;
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

};
