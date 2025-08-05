// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnPlayer.h"
#include "GameFramework/PlayerController.h"    
#include "GameFramework/InputSettings.h"  
#include "GameFramework/CharacterMovementComponent.h"

ATurnPlayer::ATurnPlayer()
{
	GetCharacterMovement()->bOrientRotationToMovement = false; // �̵� �������� �ڵ� ȸ�� ����
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
	//�ڽ��� ��Ʈ�ѷ��� PlayerContoller�� ĳ��Ʈ
	if (PlayerController)
	{
		//���� ���̰ų� ���� ���϶��� Ŀ���� �Ⱥ��̰�
		if (bIsAiming || bIsAttacking)
		{
			// ���� ���̰ų� ���� ���� ��
			PlayerController->bShowMouseCursor = false;
			PlayerController->bEnableClickEvents = false;
			PlayerController->bEnableMouseOverEvents = false;
			PlayerController->SetInputMode(FInputModeGameOnly());
			//���콺 �� ���� ����
			PlayerController->SetIgnoreLookInput(false);
			//Rotation ��ȭ üũ
			bHasSavedRotation = true;
		}
		//���� ���� �ƴϰų� ���� ���� �ƴҶ��� Ŀ���� ���̰�
		else
		{
			//���콺 Ŀ�� ǥ��
			PlayerController->bShowMouseCursor = true;
			//Ŭ��/���� �̺�Ʈ Ȱ��ȭ
			PlayerController->bEnableClickEvents = true;
			PlayerController->bEnableMouseOverEvents = true;
			//���콺 �� ����
			PlayerController->SetIgnoreLookInput(true);

			//Game + UI ���� ��ȯ UI �Է��� �޵��� ����
			FInputModeGameAndUI InputMode;
			//����Ʈ���� ���콺�� ������ �ʵ���
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			//���콺 ��Ŭ�����ص� Ŀ�� ����
			InputMode.SetHideCursorDuringCapture(false);
			PlayerController->SetInputMode(InputMode);
		
			//Rotation��ȭ�� �־��ٸ� �ʱ� Rotation���� ����
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
	//�θ���� ����(ACOECharacter)
	Super::SetAiming(bNewAiming);
	//�ڽ� bIsAiming ����
	bIsAiming = bNewAiming;
	//Ŀ�� ���� ����
	UpdateCursor();
}
