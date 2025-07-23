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
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimMontage")
	TObjectPtr<class UAnimMontage> DefaultAttackMontage;

public:
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
	//가속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	FVector Acceleration;
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
	//CharacterMovement에 연결된 폰의 움직임 받아옴
	virtual void NativeBeginPlay() override;
	//실시간으로 CharacterMovement관련값 업데이트
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void DefaultAttackAnim();
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void AnimNotify_End();
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void AnimNotify_DoDefaultAttack();
};
