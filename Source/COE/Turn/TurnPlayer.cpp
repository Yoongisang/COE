// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnPlayer.h"
#include "TurnEnemy.h"
#include "GameFramework/PlayerController.h"    
#include "GameFramework/InputSettings.h"  
#include "GameFramework/CharacterMovementComponent.h"
#include "BaseCode/COEGameInstance.h"
#include "TurnCombatBridgeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Exploration/ExplorationPlayer.h"
#include "CombatManager.h"
#include "BaseCode/COEAnimInstance.h"

ATurnPlayer::ATurnPlayer()
{
	GetCharacterMovement()->bOrientRotationToMovement = false; // 이동 방향으로 자동 회전 꺼기
	bUseControllerRotationYaw = true;

}

void ATurnPlayer::BeginPlay()
{
	Super::BeginPlay();

	PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		SavedControlRotation = PlayerController->GetControlRotation();
		bHasSavedRotation = false;
	}

	GI = GetCOEGameInstance();
	

	UpdateCursor();
}

void ATurnPlayer::UpdateCursor()
{
	//자신의 컨트롤러가 PlayerContoller로 캐스트
	if (PlayerController)
	{
		//조준 중이거나 공격 중일때는 커서가 안보이게
		if (bIsAiming || bIsAttacking)
		{
			// 조준 중이거나 공격 중일 때
			PlayerController->bShowMouseCursor = false;
			PlayerController->bEnableClickEvents = false;
			PlayerController->bEnableMouseOverEvents = false;
			PlayerController->SetInputMode(FInputModeGameOnly());
			//마우스 룩 차단 해제
			PlayerController->SetIgnoreLookInput(false);
			//Rotation 변화 체크
			bHasSavedRotation = true;
		}
		//조준 중이 아니거나 공격 중이 아닐때는 커서가 보이게
		else
		{
			//마우스 커서 표시
			PlayerController->bShowMouseCursor = true;
			//클릭/오버 이벤트 활성화
			PlayerController->bEnableClickEvents = true;
			PlayerController->bEnableMouseOverEvents = true;
			//마우스 룩 차단
			PlayerController->SetIgnoreLookInput(true);

			//Game + UI 모드로 전환 UI 입려도 받도록 설정
			FInputModeGameAndUI InputMode;
			//뷰포트에서 마우스가 나가지 않도록
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			//마우스 좌클릭을해도 커서 유지
			InputMode.SetHideCursorDuringCapture(false);
			PlayerController->SetInputMode(InputMode);
		
			//Rotation변화가 있었다면 초기 Rotation으로 변경
			if (bHasSavedRotation)
			{
				PlayerController->SetControlRotation(SavedControlRotation);
				bHasSavedRotation = false;
			}
		}
	}
}

void ATurnPlayer::SetAiming(bool bNewAiming)
{
	if (!CanPerformAction())
		return;

	//부모로직 실행(ACOECharacter)
	Super::SetAiming(bNewAiming);
	//자식 bIsAiming 갱신
	bIsAiming = bNewAiming;
	//커서 상태 갱신
	UpdateCursor();
}

void ATurnPlayer::UseSkill_Q()
{
	// 기본 공격 처리
	if (!bIsAttacking)
	{
		DefaultAttack();
		// AP +1 (클램프)
		CharacterStats.CurrentAP = FMath::Clamp(CharacterStats.CurrentAP + 1, 0, CharacterStats.MAXAP);

		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Used Q skill → CurrentAP: %d"), CharacterStats.CurrentAP);

	}

}

void ATurnPlayer::Parry()
{

	// 방어 가능한 상태인지 확인
	if (!CanPerformDefense())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] Parry failed - cannot perform defense"));
		return;
	}

	// 방어 상태 설정
	bIsDefense = true;

	if (IsValid(AnimInstance))
	{
		AnimInstance->ParryAnim();

		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] 패링!"));
	}
	else
	{
		EndDefenseAction();
	}
		

}

void ATurnPlayer::Fire()
{
	// 우클릭 조준일 때만 AP 소모
	const bool bShouldSpendAP = bIsAiming;
	const int32 APCost = 1;

	if (bShouldSpendAP)
	{
		if (CharacterStats.CurrentAP < APCost)
		{
			UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] Not enough AP to Fire. CurrentAP: %d"),
				CharacterStats.CurrentAP);
			return; // AP 부족 → 발사 안 함
		}
		// AP 차감
		CharacterStats.CurrentAP = FMath::Clamp(CharacterStats.CurrentAP - APCost, 0, CharacterStats.MAXAP);
	}

	// 부모 Fire() 실행 (기존 발사 처리)
	Super::Fire();

	// AP 소모했고, 0이 되었으면 턴 종료
	if (bShouldSpendAP && CharacterStats.CurrentAP == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] AP reached 0"));
		//RequestEndTurn();
	}
}

void ATurnPlayer::UseHPPotion()
{
	if (!GI->TryConsumeHPPotion())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] No HP potion left."));
		return;
	}

	// GI 기본값 기반 + 캐릭터 보정
	float Heal = GI->BaseHPPotionAmount;
	CharacterStats.CurrentHP += Heal;
	ClampHPAP();          // 범위 보정
	RequestEndTurn();     // 즉시 턴 종료
	UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] UseHPPotion."));

}

void ATurnPlayer::Dodge()
{
	// 방어 가능한 상태인지 확인
	if (!CanPerformDefense())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] Dodge failed - cannot perform defense"));
		return;
	}

	
	// 방어 상태 설정
	bIsDefense = true;

	if (IsValid(AnimInstance))
	{
		AnimInstance->DodgeAnim();
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayerController] 회피!"));
	}
	else
	{
		EndDefenseAction();
	}
		
	
}

void ATurnPlayer::UseAPPotion()
{
	if (GI)
	{
		if (!GI->TryConsumeAPPotion())
		{
			UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] No AP potion left."));
			return;
		}

		// GI 기본값 기반 + 캐릭터 보정 (정수화)
		float raw = static_cast<float>(GI->BaseAPPotionAmount);
		const int32 Gain = FMath::Max(0, FMath::RoundToInt(raw));
		CharacterStats.CurrentAP += Gain;
		ClampHPAP();          // 범위 보정
		RequestEndTurn();     // 즉시 턴 종료
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] UseAPPotion."));
	}
}

bool ATurnPlayer::IsEnemyTurnActive() const
{
	if (!GI) return false;

	// CombatManager를 통해 현재 활성 캐릭터 확인
	if (TurnBridge && TurnBridge->GetManager())
	{
		ACOECharacter* ActiveChar = TurnBridge->GetManager()->GetActiveCharacter();
		if (!ActiveChar) return false;

		ECombatTeam ActiveTeam = GI->GetTeam(ActiveChar);
		return ActiveTeam == ECombatTeam::Enemy;
	}

	return false;
}

bool ATurnPlayer::CanPerformAction() const
{
	// 공격 중이면 불가
	if (bIsAttacking) 
		return false;

	// 자신의 턴이 아니면 불가
	if (!TurnBridge || !TurnBridge->GetManager()) 
		return false;

	ACOECharacter* ActiveChar = TurnBridge->GetManager()->GetActiveCharacter();
	return (ActiveChar == this);

}

bool ATurnPlayer::CanPerformDefense() const
{
	return IsEnemyTurnActive() && !bIsDefense;
}

void ATurnPlayer::StartParryInvincibility()
{
	UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] %s - Parry invincibility started!"), *GetName());
	bIsParryInvincible = true;
}

void ATurnPlayer::EndParryInvincibility()
{
	UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] %s - Parry invincibility ended!"), *GetName());
	bIsParryInvincible = false;
}

void ATurnPlayer::StartDodgeInvincibility()
{
	UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] %s - Dodge invincibility started!"), *GetName());
	bIsDodgeInvincible = true;
}

void ATurnPlayer::EndDodgeInvincibility()
{
	UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] %s - Dodge invincibility ended!"), *GetName());
	bIsDodgeInvincible = false;
}

void ATurnPlayer::EndDefenseAction()
{
	UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] %s - Defense action completed!"), *GetName());

	// 모든 방어 상태 해제
	bIsDefense = false;
	bIsParryInvincible = false;
	bIsDodgeInvincible = false;

	// 패링 반격 대상 초기화
	ParryCounterTarget.Reset();
}

void ATurnPlayer::OnParrySuccess(ATurnEnemy* Attacker)
{
	if (!IsValid(Attacker))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] OnParrySuccess called but Attacker is invalid"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] %s - Parry SUCCESS against %s!"),
		*GetName(), *Attacker->GetName());

	// 패링 성공 보상: AP +2
	GainAP(2);
	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Parry successful! AP +2 (Current: %d)"),
		CharacterStats.CurrentAP);

	// 반격 대상 설정
	ParryCounterTarget = Attacker;

	// 즉시 반격 실행
	ExecuteParryCounter(Attacker);
}

void ATurnPlayer::OnDodgeSuccess(ATurnEnemy* Attacker)
{
	if (!IsValid(Attacker))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] OnDodgeSuccess called but Attacker is invalid"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] %s - Dodge SUCCESS against %s!"),
		*GetName(), *Attacker->GetName());

	// 회피 성공 보상: AP +1
	GainAP(1);
	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Dodge successful! AP +1 (Current: %d)"),
		CharacterStats.CurrentAP);

}

float ATurnPlayer::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 무적 상태 체크
	if (IsInvincible())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] %s is invincible! Damage blocked: %f"),
			*GetName(), DamageAmount);

		// 패링 무적 중이고 공격자가 TurnEnemy라면 패링 성공 처리
		if (bIsParryInvincible)
		{
			if (ATurnEnemy* AttackingEnemy = Cast<ATurnEnemy>(DamageCauser))
			{
				OnParrySuccess(AttackingEnemy);
			}
		}
		// 회피 무적 중이고 공격자가 TurnEnemy라면 회피 성공 처리
		else if (bIsDodgeInvincible)
		{
			if (ATurnEnemy* AttackingEnemy = Cast<ATurnEnemy>(DamageCauser))
			{
				OnDodgeSuccess(AttackingEnemy);
			}
		}


		return 0.0f; // 데미지 무효화
	}

	// 일반적인 데미지 처리 (부모 클래스 호출)
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

}

UCOEGameInstance* ATurnPlayer::GetCOEGameInstance() const
{
	return GetGameInstance<UCOEGameInstance>();
}

void ATurnPlayer::ClampHPAP()
{
	CharacterStats.CurrentHP = FMath::Clamp(CharacterStats.CurrentHP, 0.f, CharacterStats.MAXHP);
	CharacterStats.CurrentAP = FMath::Clamp(CharacterStats.CurrentAP, 0, CharacterStats.MAXAP);
}

void ATurnPlayer::ExecuteParryCounter(ATurnEnemy* Target)
{
	if (!IsValid(Target))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] ExecuteParryCounter called but Target is invalid"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] %s executing parry counter against %s"),
		*GetName(), *Target->GetName());

	// 1. 적의 공격 애니메이션 중단 및 스턴 적용
	if (Target->AnimInstance && Target->bIsAttacking)
	{
		// 현재 공격 애니메이션 중단
		Target->AnimInstance->Montage_Stop(0.0f);
		Target->bIsAttacking = false;
		// 추후 즉시 애니메이션 중단한다음 Stun AnimMontage 실행 구현

		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] %s attack animation stopped"), *Target->GetName());
	}

	// 2. 적에게 스턴 애니메이션 적용 (스턴 몽타주가 있다면)
	// TODO: 적에게 스턴 애니메이션을 적용하는 코드 추가
	// if (Target->HasStunMontage()) Target->PlayStunMontage();

	// 3. 적에게 데미지 적용 (패링 반격 데미지)
	float ParryCounterDamage = 15.0f; // 기본 공격보다 높은 데미지
	UGameplayStatics::ApplyDamage(Target, ParryCounterDamage, GetController(), this, nullptr);

	UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] Parry counter damage applied: %f to %s"),
		ParryCounterDamage, *Target->GetName());

	// 4. 파티클 효과 (패링 성공 이펙트)
	SpawnDefaultAttackEmitter(); // 기존 공격 파티클 재사용 (추후 전용 이펙트로 교체 가능)
}

void ATurnPlayer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	PlayerController = Cast<APlayerController>(NewController);
	if (PlayerController)
	{
		SavedControlRotation = PlayerController->GetControlRotation();
	}
	UpdateCursor(); // 소유권 변경 즉시 UI/입력 상태 반영
}

void ATurnPlayer::UnPossessed()
{
	Super::UnPossessed();
	PlayerController = nullptr;
}

void ATurnPlayer::RequestEndTurn()
{
	if (TurnBridge)
	{
		//강제로 턴을 넘기는 경우를 위해 남겨둠
		TurnBridge->NotifySkillFinished();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] RequestEndTurn called but TurnBridge is null"));
	}
}
