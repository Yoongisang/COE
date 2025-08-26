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
#include "Kismet/GameplayStatics.h" // UGameplayStatics 사용을 위해 추가
#include "GameFramework/PlayerController.h"   
#include "GameFramework/PlayerController.h" // GetInstigatorController 사용을 위해 추가
#include "GameFramework/Actor.h" // GetInstigatorController 사용을 위해 추가

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
	if (IsValid(Character))
	{
		IsAiming = Character->bIsAiming;
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

void UCOEAnimInstance::HealAnim()
{
	// HealMontage가 할당되어있고 Montage가 실행중이 아닐경우 Montage실행
	if (IsValid(HealMontage))
	{
		if (!Montage_IsPlaying(HealMontage))
		{
			Montage_Play(HealMontage);

		}

	}
}

void UCOEAnimInstance::SkillAnim()
{
	// SkillMontage가 할당되어있고 Montage가 실행중이 아닐경우 Montage실행
	if (IsValid(SkillMontage))
	{
		if (!Montage_IsPlaying(SkillMontage))
		{
			Montage_Play(SkillMontage);

		}

	}
}

void UCOEAnimInstance::ParryAnim()
{
	// ParryMontage가 할당되어있고 Montage가 실행중이 아닐경우 Montage실행
	if (IsValid(ParryMontage))
	{
		if (!Montage_IsPlaying(ParryMontage))
		{
			Montage_Play(ParryMontage);

		}

	}
}

void UCOEAnimInstance::DodgeAnim()
{
	// DodgeMontage가 할당되어있고 Montage가 실행중이 아닐경우 Montage실행
	if (IsValid(DodgeMontage))
	{
		if (!Montage_IsPlaying(DodgeMontage))
		{
			Montage_Play(DodgeMontage);

		}

	}
}

void UCOEAnimInstance::AnimNotify_End()
{
	if (!Character) 
		return;


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
							// TurnPlayer가 맞다면, 새로운 공격 시퀀스 함수를 호출합니다.
							TurnPlayer->OnAttackMontageEnded();
							return;
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

						COEChar->bIsAttacking = false;
					}, 0.3f, false);
				UE_LOG(LogTemp, Log, TEXT("[AnimNotify_End] Bridge/Manager OK -> TurnEnd scheduled"));
				return;
			}
		}
		// Exploration 모드 (TurnBridge 없음)
		COEChar->bIsAttacking = false;
		UE_LOG(LogTemp, Log, TEXT("[AnimNotify_End] Exploration mode - bIsAttacking = false"));
	}
	
}


void UCOEAnimInstance::AnimNotify_DoDefaultAttack()
{
	if (!Character)
		return;

	Character->DoDefaultAttack();
	UE_LOG(LogTemp, Log, TEXT("DoDefaultAttack"));
}

void UCOEAnimInstance::AnimNotify_Parry_Start()
{
	if (!Character)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[AnimNotify] ParryStart called for %s"), *Character->GetName());

	// TurnPlayer로 캐스팅해서 패링 무적 시작
	if (auto* TurnPlayer = Cast<ATurnPlayer>(Character))
	{
		TurnPlayer->StartParryInvincibility();
	}

}

void UCOEAnimInstance::AnimNotify_Parry_End()
{
	if (!Character)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[AnimNotify] ParryEnd called for %s"), *Character->GetName());

	// TurnPlayer로 캐스팅해서 패링 무적 종료
	if (auto* TurnPlayer = Cast<ATurnPlayer>(Character))
	{
		TurnPlayer->EndParryInvincibility();
	}

}

void UCOEAnimInstance::AnimNotify_ParryAnim_End()
{
	if (!Character)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[AnimNotify] ParryAnimEnd called for %s"), *Character->GetName());

	// TurnPlayer로 캐스팅해서 방어 행동 완전 종료
	if (auto* TurnPlayer = Cast<ATurnPlayer>(Character))
	{
		TurnPlayer->EndDefenseAction();
	}

}

void UCOEAnimInstance::AnimNotify_Dodge_Start()
{
	if (!Character)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[AnimNotify] DodgeStart called for %s"), *Character->GetName());

	// TurnPlayer로 캐스팅해서 회피 무적 시작
	if (auto* TurnPlayer = Cast<ATurnPlayer>(Character))
	{
		TurnPlayer->StartDodgeInvincibility();
	}

}

void UCOEAnimInstance::AnimNotify_Dodge_End()
{
	if (!Character)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[AnimNotify] DodgeEnd called for %s"), *Character->GetName());

	// TurnPlayer로 캐스팅해서 회피 무적 종료 (여기서 AP +1 보상 처리됨)
	if (auto* TurnPlayer = Cast<ATurnPlayer>(Character))
	{
		TurnPlayer->EndDodgeInvincibility();
	}

}

void UCOEAnimInstance::AnimNotify_DodgeAnim_End()
{
	if (!Character)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[AnimNotify] DodgeAnimEnd called for %s"), *Character->GetName());

	// TurnPlayer로 캐스팅해서 방어 행동 완전 종료
	if (auto* TurnPlayer = Cast<ATurnPlayer>(Character))
	{
		TurnPlayer->EndDefenseAction();
	}
}

void UCOEAnimInstance::AnimNotify_WSkill_Start()
{
	if (!Character)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[AnimNotify] WSkill_Start %s"), *Character->GetName());

	// TurnPlayer로 캐스팅해서 방어 행동 완전 종료
	if (auto* TurnPlayer = Cast<ATurnPlayer>(Character))
	{
		auto* Target = TurnPlayer->TargetSelector->GetCurrentTarget();
		// Heal 파티클 스폰 (타겟에게)
		TurnPlayer->SpawnHealEmitter(Target);

		// Heal
		Target->CharacterStats.CurrentHP = FMath::Clamp(
			Target->CharacterStats.CurrentHP + 20.f,
			0.f,
			Target->CharacterStats.MAXHP
		);
	}
}

void UCOEAnimInstance::AnimNotify_WSkill_End()
{
	if (!Character)
		return;

	// TurnPlayer로 캐스팅해서 방어 행동 완전 종료
	if (auto* TurnPlayer = Cast<ATurnPlayer>(Character))
	{
		UE_LOG(LogTemp, Log, TEXT("[AnimNotify] WSkill_End called for %s"), *Character->GetName());
		TurnPlayer->bIsAttacking = false;

		// 대기 중이던 스킬 상태 초기화 (이제 여기서 초기화!)
		TurnPlayer->PendingSkillType = ESkillTargetType::Universal;
		TurnPlayer->PendingSkillName = TEXT("");
		TurnPlayer->RequestEndTurn(); // 모든 후처리를 RequestEndTurn에 위임
	}
}

void UCOEAnimInstance::AnimNotify_ESkill_Start()
{
	if (!Character)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[AnimNotify] ESkill_Start %s"), *Character->GetName());

	// TurnPlayer로 캐스팅해서 방어 행동 완전 종료
	if (auto* TurnPlayer = Cast<ATurnPlayer>(Character))
	{
		auto* Target = TurnPlayer->TargetSelector->GetCurrentTarget();
		// Skill 파티클 스폰 (타겟에게)
		TurnPlayer->SpawnSkillEmitter(Target);

		// TODO: 스킬 D 구체적인 로직 구현
		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Skill D executed on %s"), *Target->GetName());

		// 예시: 강력한 공격
		UGameplayStatics::ApplyDamage(Target, 100.f, TurnPlayer->GetController(), TurnPlayer, nullptr);
	
	}
}

void UCOEAnimInstance::AnimNotify_ESkill_End()
{
	if (!Character)
		return;

	// TurnPlayer로 캐스팅해서 Heal 스킬 시작
	if (auto* TurnPlayer = Cast<ATurnPlayer>(Character))
	{
		UE_LOG(LogTemp, Log, TEXT("[AnimNotify] ESkill_End called for %s"), *Character->GetName());
		TurnPlayer->bIsAttacking = false;

		// 대기 중이던 스킬 상태 초기화 (이제 여기서 초기화!)
		TurnPlayer->PendingSkillType = ESkillTargetType::Universal;
		TurnPlayer->PendingSkillName = TEXT("");

		TurnPlayer->RequestEndTurn(); // 모든 후처리를 RequestEndTurn에 위임
	}
}
