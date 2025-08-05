// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnPlayer.h"
#include "GameFramework/PlayerController.h"    
#include "GameFramework/InputSettings.h"  
#include "GameFramework/CharacterMovementComponent.h"

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
	//부모로직 실행(ACOECharacter)
	Super::SetAiming(bNewAiming);
	//자식 bIsAiming 갱신
	bIsAiming = bNewAiming;
	//커서 상태 갱신
	UpdateCursor();
}
