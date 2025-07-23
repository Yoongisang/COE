// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCode/COEAnimInstance.h"
#include "COECharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimMontage.h"
#include "kismet/kismetMathLibrary.h"


UCOEAnimInstance::UCOEAnimInstance()
{
}

void UCOEAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UCOEAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	APawn* Pawn = TryGetPawnOwner();
	if (IsValid(Pawn))
	{
		Character = Cast<ACOECharacter>(Pawn);
		if (IsValid(Character))
		{
			//CharacterMovement�� ����� ���� ������ �޾ƿ�
			CharacterMovement = Character->GetCharacterMovement();
		}
		
	}
}

void UCOEAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	if (IsValid(CharacterMovement))
	{
		//ĳ���� �ӵ�, ȸ�� �޾ƿ��� UnrotateVector�� ĳ���� �������� ������ִ¿� �׸��� �������ͷ� �������
		Velocity = CharacterMovement->Velocity;
		FRotator Rotation = Character->GetActorRotation();
		FVector UnrotateVector = Rotation.UnrotateVector(Velocity);

		UnrotateVector.Normalize();
		//�¿� ������ ��������
		Horizontal = UnrotateVector.Y;
		Vertical = UnrotateVector.X;
		//�ӵ� XY����ũ�� == ���� �ӷ�
		GroundSpeed = Velocity.Size2D();
		//ĳ���� ���ӵ�
		Acceleration = CharacterMovement->GetCurrentAcceleration();
		if (GroundSpeed >= 3.0 /* && Acceleration != FVector::Zero()*/)
		{
			ShouldMove = true;
		}
		else
		{
			ShouldMove = false;
		}
		//���� || �߶� ����
		IsFalling = CharacterMovement->IsFalling();
		//�þ� ȸ��
		AimRotation = Character->GetBaseAimRotation();
		FRotator RotFromX = UKismetMathLibrary::MakeRotFromX(Velocity);

		FRotator DeltaRotation = AimRotation - RotFromX;
		DeltaRotation.Normalize();

		YawOffset = DeltaRotation.Yaw;
	}
}

void UCOEAnimInstance::DefaultAttackAnim()
{
	// DefaultAttackMontage�� �Ҵ�Ǿ��ְ� Montage�� �������� �ƴҰ�� Montage����
	if (IsValid(DefaultAttackMontage))
	{
		if (!Montage_IsPlaying(DefaultAttackMontage))
		{
			Montage_Play(DefaultAttackMontage);

		}

	}
}

void UCOEAnimInstance::AnimNotify_End()
{
	Character->bIsAttacking = false;
	UE_LOG(LogTemp, Log, TEXT("bIsAttacking == false"));
}

void UCOEAnimInstance::AnimNotify_DoDefaultAttack()
{
	Character->DoDefaultAttack();
	UE_LOG(LogTemp, Log, TEXT("DoDefaultAttack"));
}
