// Fill out your copyright notice in the Description page of Project Settings.


#include "COEAnimInstance.h"
#include "COECharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimMontage.h"
#include "Kismet/KismetMathLibrary.h"
#include "Turn/TurnPlayer.h"
#include "Turn/TurnCombatBridgeComponent.h"
#include "Turn/CombatManager.h"
#include "Turn/TurnEnemy.h"

UCOEAnimInstance::UCOEAnimInstance()
{
}

void UCOEAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (IsValid(Character)) return;

	APawn* Pawn = TryGetPawnOwner();
	if (IsValid(Pawn))
	{
		Character = Cast<ACOECharacter>(Pawn);
		if (IsValid(Character))
		{
			//CharacterMovement에 연결된 폰의 움직임 받아옴
			CharacterMovement = Character->GetCharacterMovement();
		}

	}
}

void UCOEAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	if (IsValid(Character)) return;

	APawn* Pawn = TryGetPawnOwner();
	if (IsValid(Pawn))
	{
		Character = Cast<ACOECharacter>(Pawn);
		if (IsValid(Character))
		{
			//CharacterMovement에 연결된 폰의 움직임 받아옴
			CharacterMovement = Character->GetCharacterMovement();
		}
		
	}
}

void UCOEAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	if (IsValid(CharacterMovement))
	{
		//캐릭터 속도, 회전 받아오기 UnrotateVector는 캐릭터 기준으로 만들어주는용 그리고 단위벡터로 만들어줌
		Velocity = CharacterMovement->Velocity;
		FRotator Rotation = Character->GetActorRotation();
		FVector UnrotateVector = Rotation.UnrotateVector(Velocity);

		UnrotateVector.Normalize();
		//좌우 움직임 단위벡터
		Horizontal = UnrotateVector.Y;
		Vertical = UnrotateVector.X;
		//속도 XY기준크기 == 평지 속력
		GroundSpeed = Velocity.Size2D();
		//캐릭터 가속도
		Acceleration = CharacterMovement->GetCurrentAcceleration();
		if (GroundSpeed >= 3.0 /* && Acceleration != FVector::Zero()*/)
		{
			ShouldMove = true;
		}
		else
		{
			ShouldMove = false;
		}
		//점프 || 추락 판정
		IsFalling = CharacterMovement->IsFalling();
		//시야 회전
		AimRotation = Character->GetBaseAimRotation();
		FRotator RotFromX = UKismetMathLibrary::MakeRotFromX(Velocity);

		FRotator DeltaRotation = AimRotation - RotFromX;
		DeltaRotation.Normalize();

		YawOffset = DeltaRotation.Yaw;
	}
}

void UCOEAnimInstance::DefaultAttackAnim()
{
	// DefaultAttackMontage가 할당되어있고 Montage가 실행중이 아닐경우 Montage실행
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
	if (!Character) 
		return;

	Character->bIsAttacking = false;

	// TurnBridge가 있고 Manager가 있으면 턴 종료 처리를 지연
// ACOECharacter로 캐스트하여 TurnPlayer와 TurnEnemy 모두 처리
	if (auto* COEChar = Cast<ACOECharacter>(Character))
	{
		if (COEChar->TurnBridge && COEChar->TurnBridge->GetOwner())
		{
			if (COEChar->TurnBridge->GetManager() != nullptr)
			{
				FTimerHandle DelayHandle;
				GetWorld()->GetTimerManager().SetTimer(DelayHandle, [COEChar]()
					{
						// TurnPlayer인 경우 RequestEndTurn() 호출
						if (auto* TurnPlayer = Cast<ATurnPlayer>(COEChar))
						{
							TurnPlayer->RequestEndTurn();
							UE_LOG(LogTemp, Log, TEXT("[AnimNotify_End] TurnPlayer RequestEndTurn called"));
						}
						// TurnEnemy인 경우 FinishEnemyTurn() 호출
						else if (auto* TurnEnemy = Cast<ATurnEnemy>(COEChar))
						{
							TurnEnemy->FinishEnemyTurn();
							UE_LOG(LogTemp, Log, TEXT("[AnimNotify_End] TurnEnemy FinishEnemyTurn called"));
						}
						// 일반적인 경우 브리지를 통해 턴 종료
						else
						{
							COEChar->TurnBridge->NotifySkillFinished();
							UE_LOG(LogTemp, Log, TEXT("[AnimNotify_End] Generic NotifySkillFinished called"));
						}
					}, 0.3f, false);
				UE_LOG(LogTemp, Log, TEXT("[AnimNotify_End] Bridge/Manager OK -> TurnEnd scheduled"));
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("bIsAttacking == false"));
}


void UCOEAnimInstance::AnimNotify_DoDefaultAttack()
{
	Character->DoDefaultAttack();
	UE_LOG(LogTemp, Log, TEXT("DoDefaultAttack"));
}
