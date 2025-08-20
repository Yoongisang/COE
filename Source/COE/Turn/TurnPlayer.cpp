// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnPlayer.h"
#include "GameFramework/PlayerController.h"    
#include "GameFramework/InputSettings.h"  
#include "GameFramework/CharacterMovementComponent.h"
#include "BaseCode/COEGameInstance.h"
#include "TurnCombatBridgeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Exploration/ExplorationPlayer.h"
#include "CombatManager.h"

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

//bool ATurnPlayer::UseSkill_WithCost(int32 Cost)
//{
//	if (Cost > 0)
//	{
//		if (CharacterStats.CurrentAP < Cost)
//		{
//			UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] Not enough AP for skill. Need: %d, Cur: %d"),
//				Cost, CharacterStats.CurrentAP);
//			return false;
//		}
//
//		CharacterStats.CurrentAP = FMath::Clamp(CharacterStats.CurrentAP - Cost, 0, CharacterStats.MAXAP);
//	}
//
//	// 스킬 효과 실행...
//	// TODO: 실제 스킬 로직 구현
//
//	// 규칙: 코스트형 스킬은 성공 시 즉시 턴 종료
//	RequestEndTurn();
//
//	return true;
//}

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

UCOEGameInstance* ATurnPlayer::GetCOEGameInstance() const
{
	return GetGameInstance<UCOEGameInstance>();
}

void ATurnPlayer::ClampHPAP()
{
	CharacterStats.CurrentHP = FMath::Clamp(CharacterStats.CurrentHP, 0.f, CharacterStats.MAXHP);
	CharacterStats.CurrentAP = FMath::Clamp(CharacterStats.CurrentAP, 0, CharacterStats.MAXAP);
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
