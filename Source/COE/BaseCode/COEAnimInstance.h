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
	//������ üũ (Idle �Ǵ�)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	bool ShouldMove;
	//�߶� üũ (���� �Ǵ� �߶� �Ǵ�)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	bool IsFalling;
	//�ӵ� ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	FVector Velocity;
	//�����Է�
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	float Horizontal;
	//�����Է�
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	float Vertical;
	//XY ����ũ��
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	float GroundSpeed;
	//���ӵ�
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	FVector Acceleration;
	//���� ����� �̵����� �¿� ���� ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	float YawOffset;
	//ĳ���� ���� ���� ȸ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	FRotator AimRotation;
public:
	//ĳ���� 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<class ACOECharacter> Character;
	//ĳ���� ������ ������Ʈ
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<class UCharacterMovementComponent> CharacterMovement;
public:
	UCOEAnimInstance();
	virtual void NativeInitializeAnimation() override;
	//CharacterMovement�� ����� ���� ������ �޾ƿ�
	virtual void NativeBeginPlay() override;
	//�ǽð����� CharacterMovement���ð� ������Ʈ
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void DefaultAttackAnim();
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void AnimNotify_End();
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void AnimNotify_DoDefaultAttack();
};
