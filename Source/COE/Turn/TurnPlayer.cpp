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
#include "TurnHudWidget.h"
#include "Components/WidgetComponent.h"

ATurnPlayer::ATurnPlayer()
{
	GetCharacterMovement()->bOrientRotationToMovement = false; // 이동 방향으로 자동 회전 꺼기
	bUseControllerRotationYaw = true;

	// 타겟 선택 컴포넌트 생성
	TargetSelector = CreateDefaultSubobject<UTargetSelectionComponent>(TEXT("TargetSelector"));

	// Turn HUD 위젯 컴포넌트 생성
	TurnHudWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("TurnHudWidgetComponent"));
	if (TurnHudWidgetComponent)
	{
		TurnHudWidgetComponent->SetupAttachment(RootComponent);
		// 캐릭터 앞쪽 위치로 설정
		TurnHudWidgetComponent->SetRelativeLocation(FVector(150.0f, 150.0f, 50.0f));

		// 월드 스페이스 설정 (항상 카메라를 바라봄)
		TurnHudWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		TurnHudWidgetComponent->SetDrawSize(FVector2D(400.0f, 200.0f));

		// 반투명 설정을 위한 속성
		TurnHudWidgetComponent->SetDrawAtDesiredSize(true);

		// 기본적으로 숨김 상태
		SetTurnHudVisible(false);
		//TurnHudWidgetComponent->SetVisibility(true);
	}

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
	

	// 타겟 선택 시스템 초기화
	InitializeTargetSelector();

	// Turn HUD 초기화 (지연 호출로 PlayerController 준비 대기)
	FTimerHandle HudInitHandle;
	GetWorld()->GetTimerManager().SetTimer(
		HudInitHandle,
		this,
		&ATurnPlayer::InitializeTurnHud,
		0.5f,
		false
	);
}

void ATurnPlayer::UpdateCursor()
{
	//자신의 컨트롤러가 PlayerContoller로 캐스트
	if (!PlayerController)
		return;
	
	// 타겟 선택 중인지 확인
	bool bIsTargetSelecting = IsSelectingTarget();

	// 1) 타겟 선택 중: 커서 감춤 + 룩/무브 입력 잠금 + GameOnly
	if (bIsTargetSelecting)
	{
		PlayerController->bShowMouseCursor = false;
		PlayerController->bEnableClickEvents = false;
		PlayerController->bEnableMouseOverEvents = false;

		PlayerController->SetIgnoreLookInput(true);
		PlayerController->SetIgnoreMoveInput(true);

		PlayerController->SetInputMode(FInputModeGameOnly());


		bHasSavedRotation = true;
	
		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] UpdateCursor: TargetSelecting (cursor hidden, look locked)"));
		return;
	}

	//조준 중이거나 공격 중일때는 커서가 안보이게
	if (bIsAiming || bIsAttacking)
	{
		// 타겟 선택 중이거나 조준 중이거나 공격 중일 때
		PlayerController->bShowMouseCursor = false;
		PlayerController->bEnableClickEvents = false;
		PlayerController->bEnableMouseOverEvents = false;
		PlayerController->SetInputMode(FInputModeGameOnly());
	
		PlayerController->SetIgnoreLookInput(false);
		// 조준 중에는 마우스 룩 허용
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

void ATurnPlayer::OnAttackMontageEnded()
{
	if (CurrentAttackState == EAttackSequenceState::Attacking)
	{
		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Attack montage finished. Returning to origin."));
		// '원래 위치로 복귀' 상태로 전환
		CurrentAttackState = EAttackSequenceState::Returning;
	}
}

void ATurnPlayer::SetAiming(bool bNewAiming)
{
	if (!CanPerformAction() || IsSelectingTarget())
		return;

	//부모로직 실행(ACOECharacter)
	Super::SetAiming(bNewAiming);

	//자식 bIsAiming 갱신
	bIsAiming = bNewAiming;

	//커서 상태 갱신
	UpdateCursor();

	// HUD 업데이트 추가
	UpdateHudForTurnState();
}

//기본 공격
void ATurnPlayer::UseSkill_Q()
{
	if (!CanPerformAction() || IsSelectingTarget())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Q pressed - Starting target selection for basic attack"));

	PendingSkillType = ESkillTargetType::Attack;
	PendingSkillName = TEXT("BasicAttack");

	if (TargetSelector)
	{
		TargetSelector->StartTargetSelection(ESkillTargetType::Attack);
	}

	//// 기본 공격 처리
	//if (!bIsAttacking)
	//{
	//	DefaultAttack();
	//	// AP +1 (클램프)
	//	CharacterStats.CurrentAP = FMath::Clamp(CharacterStats.CurrentAP + 1, 0, CharacterStats.MAXAP);
	//
	//	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Used Q skill → CurrentAP: %d"), CharacterStats.CurrentAP);
	//
	//}

}

//Heal 스킬
void ATurnPlayer::UseSkill_W()
{
	if (!CanPerformAction() || IsSelectingTarget())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] S pressed - Starting target selection for Skill W"));

	PendingSkillType = ESkillTargetType::Heal;
	PendingSkillName = TEXT("SkillW");

	if (TargetSelector)
	{
		TargetSelector->StartTargetSelection(ESkillTargetType::Heal);
	}
}

//강한 공격
void ATurnPlayer::UseSkill_E()
{
	if (!CanPerformAction() || IsSelectingTarget())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] D pressed - Starting target selection for Skill D"));

	PendingSkillType = ESkillTargetType::Attack;
	PendingSkillName = TEXT("SkillE");

	if (TargetSelector)
	{
		TargetSelector->StartTargetSelection(ESkillTargetType::Attack);
	}
}

// HP 포션 사용
void ATurnPlayer::UseSkill_A()
{
	if (!CanPerformAction() || IsSelectingTarget())
	{
		return;
	}

	// HP 포션 보유 확인
	if (!GI || !GI->HasHPPotion())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] No HP potion available"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] W pressed - Starting target selection for HP potion"));

	PendingSkillType = ESkillTargetType::Heal;
	PendingSkillName = TEXT("HPPotion");

	if (TargetSelector)
	{
		TargetSelector->StartTargetSelection(ESkillTargetType::Heal);
	}
}

// AP 포션 사용
void ATurnPlayer::UseSkill_S()
{
	if (!CanPerformAction() || IsSelectingTarget())
	{
		return;
	}

	// AP 포션 보유 확인
	if (!GI || !GI->HasAPPotion())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] No AP potion available"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] E pressed - Starting target selection for AP potion"));

	PendingSkillType = ESkillTargetType::Buff;
	PendingSkillName = TEXT("APPotion");

	if (TargetSelector)
	{
		TargetSelector->StartTargetSelection(ESkillTargetType::Buff);
	}
}

void ATurnPlayer::UseSkill_D()
{
	// 행동 가능 상태 체크
	if (!CanPerformAction() || IsSelectingTarget())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] UseSkill_D - Cannot perform action"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] UseSkill_D - 턴 넘김"));

	// 즉시 턴 종료 (타겟 선택 없이 바로 실행)
	RequestEndTurn();
}

void ATurnPlayer::ExecuteSkillOnTarget(ACOECharacter* TargetCharacter, ESkillTargetType SkillType)
{
	if (!IsValid(TargetCharacter))
	{
		UE_LOG(LogTemp, Error, TEXT("[TurnPlayer] ExecuteSkillOnTarget called with invalid target"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Executing %s on target: %s"),
		*PendingSkillName, *TargetCharacter->GetName());

	// 스킬 이름에 따라 적절한 실행 함수 호출
	if (PendingSkillName == TEXT("BasicAttack"))
	{
		ExecuteBasicAttack(TargetCharacter);

		// 기본공격은 즉시 상태 초기화 (이동 시퀀스로 처리)
		PendingSkillType = ESkillTargetType::Universal;
		PendingSkillName = TEXT("");

	}
	else if (PendingSkillName == TEXT("SkillW"))
	{
		ExecuteSkillW(TargetCharacter);
	}
	else if (PendingSkillName == TEXT("SkillE"))
	{
		ExecuteSkillE(TargetCharacter);
	}
	else if (PendingSkillName == TEXT("HPPotion"))
	{
		ExecuteSkillA(TargetCharacter);

		// HP 포션은 즉시 효과 적용하므로 바로 초기화
		PendingSkillType = ESkillTargetType::Universal;
		PendingSkillName = TEXT("");

	}
	else if (PendingSkillName == TEXT("APPotion"))
	{
		ExecuteSkillS(TargetCharacter);
		// AP 포션은 즉시 효과 적용하므로 바로 초기화
		PendingSkillType = ESkillTargetType::Universal;
		PendingSkillName = TEXT("");

	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] Unknown skill: %s"), *PendingSkillName);
		
		// 알 수 없는 스킬인 경우 초기화
		PendingSkillType = ESkillTargetType::Universal;
		PendingSkillName = TEXT("");

	}

	// HUD 상태 업데이트
	UpdateHudForTurnState();
}

void ATurnPlayer::OnTargetSelectionCancelled()
{
	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Target selection cancelled"));

	// 1. 상태 초기화
	CurrentAttackTarget.Reset();

	// 2. 캐릭터 회전 복원
	if (PlayerController)
	{
		SetActorRotation(OriginalRotation);
		PlayerController->SetControlRotation(OriginalRotation);
	}

	// 대기 상태 초기화
	PendingSkillType = ESkillTargetType::Universal;
	PendingSkillName = TEXT("");

	// 커서 상태 업데이트 (다시 스킬 선택 가능 상태로)
	UpdateCursor();

	// HUD 상태 업데이트
	UpdateHudForTurnState();
}

bool ATurnPlayer::IsSelectingTarget() const
{
	return TargetSelector && TargetSelector->GetSelectionState() != ETargetSelectionState::None;
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

//void ATurnPlayer::UseHPPotion()
//{
//	if (!GI->TryConsumeHPPotion())
//	{
//		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] No HP potion left."));
//		return;
//	}
//
//	// GI 기본값 기반 + 캐릭터 보정
//	float Heal = GI->BaseHPPotionAmount;
//	CharacterStats.CurrentHP += Heal;
//	ClampHPAP();          // 범위 보정
//	RequestEndTurn();     // 즉시 턴 종료
//	UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] UseHPPotion."));
//
//}

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

//void ATurnPlayer::UseAPPotion()
//{
//	if (GI)
//	{
//		if (!GI->TryConsumeAPPotion())
//		{
//			UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] No AP potion left."));
//			return;
//		}
//
//		// GI 기본값 기반 + 캐릭터 보정 (정수화)
//		float raw = static_cast<float>(GI->BaseAPPotionAmount);
//		const int32 Gain = FMath::Max(0, FMath::RoundToInt(raw));
//		CharacterStats.CurrentAP += Gain;
//		ClampHPAP();          // 범위 보정
//		RequestEndTurn();     // 즉시 턴 종료
//		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] UseAPPotion."));
//	}
//}

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

	// GameInstance가 초기화되지 않았으면 불가
	if (!GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] CanPerformAction: GameInstance not ready"));
		return false;
	}

	// 현재 활성 캐릭터 확인
	ACOECharacter* ActiveChar = TurnBridge->GetManager()->GetActiveCharacter();
	if (!ActiveChar)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] CanPerformAction: No active character"));
		return false;
	}

	// 자신의 턴인지 확인
	return ActiveChar == this;

}

// HUD용 별도 함수 추가 - 로그 없이 체크만
bool ATurnPlayer::IsMyTurnActive() const
{
	if (!TurnBridge || !TurnBridge->GetManager() || !GI)
		return false;

	ACOECharacter* ActiveChar = TurnBridge->GetManager()->GetActiveCharacter();
	return ActiveChar == this;
}

// 현재 턴 상태를 안전하게 확인하는 함수
ETurnState ATurnPlayer::GetCurrentTurnState() const
{
	if (!TurnBridge || !TurnBridge->GetManager() || !GI)
		return ETurnState::None;

	ACOECharacter* ActiveChar = TurnBridge->GetManager()->GetActiveCharacter();
	if (!ActiveChar)
		return ETurnState::None;

	if (ActiveChar == this)
		return ETurnState::MyTurn;

	ECombatTeam ActiveTeam = GI->GetTeam(ActiveChar);
	if (ActiveTeam == ECombatTeam::Player)
		return ETurnState::AllyTurn;
	else if (ActiveTeam == ECombatTeam::Enemy)
		return ETurnState::EnemyTurn;

	return ETurnState::None;
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

void ATurnPlayer::UpdateAttackMovement()
{
	if (CurrentAttackState == EAttackSequenceState::None)
	{
		GetWorld()->GetTimerManager().ClearTimer(MovementTimerHandle);
		return;
	}

	// 현재 위치와 프레임 시간(DeltaTime) 가져오기
	FVector CurrentLocation = GetActorLocation();
	const float DeltaTime = GetWorld()->GetTimerManager().GetTimerRate(MovementTimerHandle);

	// 상태에 따라 다른 로직 수행
	switch (CurrentAttackState)
	{
	case EAttackSequenceState::MovingToTarget:
	{
		if (!CurrentAttackTarget.IsValid())
		{
			// 대상이 사라지면 시퀀스 중단
			CurrentAttackState = EAttackSequenceState::Returning;
			break;
		}

		// 목표 지점 계산 (적 앞 AttackDistance 만큼 떨어진 곳)
		const FVector TargetLocation = CurrentAttackTarget->GetActorLocation();
		const FVector DirectionToTarget = (GetActorLocation() - TargetLocation).GetSafeNormal();
		const FVector Destination = TargetLocation + (DirectionToTarget * AttackDistance);

		// 목표 지점까지의 거리 확인
		const float DistanceToDestination = FVector::Dist(GetActorLocation(), Destination);

		// 목표 지점에 거의 도착했다면 이동을 멈추고 공격 상태로 전환
		if (DistanceToDestination < 10.0f)
		{
			// 이동 중지 (AddMovementInput을 더 이상 호출하지 않음)
			GetCharacterMovement()->StopMovementImmediately(); // 혹시 모를 관성을 제거

			UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Arrived at target. Attacking."));
			CurrentAttackState = EAttackSequenceState::Attacking;
			DefaultAttack(); // 공격 애니메이션 재생
		}
		else
		{
			
			// 목표 방향으로 이동하라는 '입력'을 매 프레임 추가.
			// 이 함수가 캐릭터의 속도(Velocity)를 만들어 애니메이션을 재생.
			const FVector MoveDirection = (Destination - GetActorLocation()).GetSafeNormal();
			AddMovementInput(MoveDirection, 1.0f);
		}
		break;
	}

	case EAttackSequenceState::Returning:
	{
		// 원래 위치까지의 거리 확인
		const float DistanceToOrigin = FVector::Dist(GetActorLocation(), OriginalLocation);

		// 원래 위치에 거의 도착했다면 시퀀스 종료
		if (DistanceToOrigin < 10.0f)
		{
			GetWorld()->GetTimerManager().ClearTimer(MovementTimerHandle);
			SetActorLocationAndRotation(OriginalLocation, OriginalRotation); // 최종 위치/회전 고정
			if (PlayerController)
			{
				PlayerController->SetControlRotation(OriginalRotation);
			}

			CurrentAttackState = EAttackSequenceState::None;
			CurrentAttackTarget.Reset();
			bIsAttacking = false;

			UpdateCursor();
			RequestEndTurn(); // 턴 종료
		}
		else
		{
			
			// 원래 위치 방향으로 이동 입력을 추가.
			const FVector MoveDirection = (OriginalLocation - GetActorLocation()).GetSafeNormal();
			AddMovementInput(MoveDirection, 1.0f);
		}
		break;
	}

	default:
		break;
	}

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

void ATurnPlayer::InitializeTargetSelector()
{
	if (TargetSelector)
	{
		// 타겟 선택 완료 이벤트 바인딩
		TargetSelector->OnTargetSelected.AddDynamic(this, &ATurnPlayer::ExecuteSkillOnTarget);

		// 타겟 선택 취소 이벤트 바인딩
		TargetSelector->OnTargetSelectionCancelled.AddDynamic(this, &ATurnPlayer::OnTargetSelectionCancelled);

		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] TargetSelector initialized for %s"), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[TurnPlayer] TargetSelector is null for %s"), *GetName());
	}

}

void ATurnPlayer::ExecuteBasicAttack(ACOECharacter* Target)
{
	if (!IsValid(Target) || CurrentAttackState != EAttackSequenceState::None)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Starting attack sequence on %s"), *Target->GetName());

	bIsAttacking = true; // 다른 행동을 막기 위해 공격 상태 플래그 설정

	// AP +1 (클램프)
	GainAP(1);
	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Used Q skill → CurrentAP: %d"), CharacterStats.CurrentAP);


	//// 시퀀스에 필요한 정보 저장
	CurrentAttackTarget = Target;
	OriginalLocation = GetActorLocation();
	// TargetSelector가 타겟 선택을 시작할 때 저장해 둔,
	// 캐릭터의 '진짜' 원래 회전값을 가져옵니다.
	if (TargetSelector)
	{
		OriginalRotation = TargetSelector->OriginalPlayerRotation;
	}
	else
	{
		// 만약을 대비한 폴백
		OriginalRotation = GetActorRotation();
	}
	// 1. '적으로 이동' 상태로 전환
	CurrentAttackState = EAttackSequenceState::MovingToTarget;

	// 2. 이동을 처리할 타이머 시작 (매 프레임처럼 동작)
	GetWorld()->GetTimerManager().SetTimer(
		MovementTimerHandle,
		this,
		&ATurnPlayer::UpdateAttackMovement,
		0.016f, // 약 60fps
		true
	);
}

void ATurnPlayer::ExecuteSkillW(ACOECharacter* Target)
{
	if (!IsValid(Target))
	{
		return;
	}

	// AP 소모 확인
	if (!SpendAP(2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] Not enough AP for Skill W. CurrentAP: %d"), CharacterStats.CurrentAP);
		OnTargetSelectionCancelled();
		return; // AP 부족 시 스킬 사용 취소
	}
	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Used W skill → CurrentAP: %d"), CharacterStats.CurrentAP);


	// 공격상태로 만들어 다른 행동이 안되도록
	bIsAttacking = true;

	//// 시퀀스에 필요한 정보 저장
	CurrentAttackTarget = Target; // 스킬 대상 저장
	
	// TargetSelector가 타겟 선택을 시작할 때 저장해 둔 원래 회전값을 가져옴.,
	if (TargetSelector)
	{
		OriginalRotation = TargetSelector->OriginalPlayerRotation;
	}
	else
	{
		// 만약을 대비한 폴백
		OriginalRotation = GetActorRotation();
	}

	// AnimMonatage 실행
	if (IsValid(AnimInstance))
	{
		AnimInstance->HealAnim();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] AnimInstance is null. Ending turn for Skill W."));
		bIsAttacking = false;
		CurrentAttackTarget.Reset();
		RequestEndTurn(); // 애니메이션이 없으면 바로 턴 종료
	}

}

void ATurnPlayer::ExecuteSkillE(ACOECharacter* Target)
{
	if (!IsValid(Target))
	{
		return;
	}

	// AP 소모 확인
	if (!SpendAP(3))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] Not enough AP for Skill E. CurrentAP: %d"), CharacterStats.CurrentAP);
		OnTargetSelectionCancelled();
		return; // AP 부족 시 스킬 사용 취소
	}
	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Used E skill → CurrentAP: %d"), CharacterStats.CurrentAP);


	// 공격상태로 만들어 다른 행동이 안되도록
	bIsAttacking = true;

	CurrentAttackTarget = Target; // 스킬 대상 저장

	// TargetSelector가 타겟 선택을 시작할 때 저장해 둔 원래 회전값을 가져옴
	if (TargetSelector)
	{
		OriginalRotation = TargetSelector->OriginalPlayerRotation;
	}
	else
	{
		// 만약을 대비한 폴백
		OriginalRotation = GetActorRotation();
	}

	// AnimMonatage 실행
	if (IsValid(AnimInstance))
	{
		AnimInstance->SkillAnim();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] AnimInstance is null. Ending turn for Skill E."));
		bIsAttacking = false;
		CurrentAttackTarget.Reset();
		RequestEndTurn(); // 애니메이션이 없으면 바로 턴 종료
	}
}


// HP 포션 사용
void ATurnPlayer::ExecuteSkillA(ACOECharacter* Target)
{
	if (!IsValid(Target) || !GI)
	{
		return;
	}

	if (!GI->TryConsumeHPPotion())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] Failed to consume HP potion"));
		return;
	}

	// HP 회복 적용
	float HealAmount = GI->BaseHPPotionAmount;

	if (auto* TargetStats = &Target->CharacterStats)
	{
		TargetStats->CurrentHP = FMath::Clamp(
			TargetStats->CurrentHP + HealAmount,
			0.f,
			TargetStats->MAXHP
		);
	}

	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] HP Potion used on %s (Heal: %f)"),
		*Target->GetName(), HealAmount);

	RequestEndTurn(); // 즉시 턴 종료

}

//AP 포션 사용
void ATurnPlayer::ExecuteSkillS(ACOECharacter* Target)
{
	if (!IsValid(Target) || !GI)
	{
		return;
	}

	if (!GI->TryConsumeAPPotion())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] Failed to consume AP potion"));
		return;
	}

	// AP 회복 적용
	int32 APGain = GI->BaseAPPotionAmount;

	if (auto* TargetStats = &Target->CharacterStats)
	{
		TargetStats->CurrentAP = FMath::Clamp(
			TargetStats->CurrentAP + APGain,
			0,
			TargetStats->MAXAP
		);
	}

	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] AP Potion used on %s (Gain: %d)"),
		*Target->GetName(), APGain);

	RequestEndTurn(); // 즉시 턴 종료
}

void ATurnPlayer::InitializeTurnHud()
{
	if (!TurnHudWidgetClass || !TurnHudWidgetComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] TurnHudWidgetClass or TurnHudWidgetComponent not set for %s"), *GetName());
		return;
	}

	// PlayerController가 준비될 때까지 대기
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	}

	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("[TurnPlayer] No PlayerController found for TurnHUD initialization"));
		return;
	}

	// 위젯 생성 및 설정
	TurnHudWidget = CreateWidget<UTurnHudWidget>(PC, TurnHudWidgetClass);
	if (TurnHudWidget)
	{
		// 위젯 컴포넌트에 설정
		TurnHudWidgetComponent->SetWidget(TurnHudWidget);

		// 캐릭터와 바인딩
		TurnHudWidget->BindToCharacter(this);

		// 위젯 컴포넌트 설정 조정
		TurnHudWidgetComponent->SetDrawSize(FVector2D(400.0f, 200.0f));
		TurnHudWidgetComponent->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
		TurnHudWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		TurnHudWidgetComponent->SetRelativeLocation(FVector(150.0f, 150.0f, 50.0f)); // 캐릭터 머리 위

		// 기본적으로 표시 (테스트용)
		SetTurnHudVisible(false);
		SetTurnHudMode(ETurnHudMode::None);

		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] Turn HUD initialized and shown for %s"), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[TurnPlayer] Failed to create Turn HUD Widget for %s"), *GetName());
	}
}

void ATurnPlayer::UpdateHudForTurnState()
{
	if (!TurnHudWidget)
		return;

	// 현재 턴 상태 확인 (로그 없는 버전 사용)
	ETurnState TurnState = GetCurrentTurnState();

	// 공격 중이면 HUD 숨김
	if (bIsAttacking)
	{
		SetTurnHudMode(ETurnHudMode::None);
		return;
	}

	// 턴 상태에 따른 HUD 모드 결정
	switch (TurnState)
	{
	case ETurnState::MyTurn:
	{
		// 조준 중이면 조준 모드
		if (bIsAiming)
		{
			SetTurnHudMode(ETurnHudMode::Aiming);
		}
		// 타겟 선택 중이면 타겟팅 모드
		else if (IsSelectingTarget())
		{
			SetTurnHudMode(ETurnHudMode::Targeting);
		}
		// 일반 플레이어 턴
		else
		{
			SetTurnHudMode(ETurnHudMode::PlayerTurn);
		}
		break;
	}

	case ETurnState::EnemyTurn:
	{
		// 현재 카메라 타겟이 자신인지 확인
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (PC->GetViewTarget() == this)
			{
				SetTurnHudMode(ETurnHudMode::EnemyTurn);
			}
			else
			{
				SetTurnHudMode(ETurnHudMode::None);
			}
		}
		else
		{
			SetTurnHudMode(ETurnHudMode::None);
		}
		break;
	}

	case ETurnState::AllyTurn:
	case ETurnState::None:
	default:
	{
		SetTurnHudMode(ETurnHudMode::None);
		break;
	}
	}

}

void ATurnPlayer::RequestEndTurn()
{
	// 1. 상태 초기화
	CurrentAttackState = EAttackSequenceState::None;
	CurrentAttackTarget.Reset();

	// 2. 캐릭터 회전 복원
	if (PlayerController)
	{
		SetActorRotation(OriginalRotation);
		PlayerController->SetControlRotation(OriginalRotation);
	}

	// 3. 커서 및 입력 모드 업데이트
	UpdateCursor();
	
	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] State reset and ending turn."));

	// 4. CombatManager에 턴 종료 알림
	if (TurnBridge)
	{
		TurnBridge->NotifySkillFinished();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] RequestEndTurn called but TurnBridge is null"));
	}

	// 턴 종료 시 HUD 숨김
	SetTurnHudMode(ETurnHudMode::None);
}

void ATurnPlayer::SetTurnHudVisible(bool bVisible)
{
	if (TurnHudWidgetComponent)
	{
		TurnHudWidgetComponent->SetVisibility(bVisible);
	}

	if (TurnHudWidget)
	{
		TurnHudWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void ATurnPlayer::SetTurnHudMode(ETurnHudMode Mode)
{
	CurrentHudMode = Mode; // 모드 저장

	if (TurnHudWidget)
	{
		TurnHudWidget->SetHudMode(Mode);
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] SetTurnHudMode called: %s"), *UEnum::GetValueAsString(Mode));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] SetTurnHudMode called but TurnHudWidget is null. Mode: %d"), (int32)Mode);
	}
}

void ATurnPlayer::RefreshTurnHud()
{
	if (TurnHudWidget)
	{
		TurnHudWidget->UpdateAPDisplay();
		TurnHudWidget->UpdateSkillButtons();
	}
}
