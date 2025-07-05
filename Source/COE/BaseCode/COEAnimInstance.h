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
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

};
