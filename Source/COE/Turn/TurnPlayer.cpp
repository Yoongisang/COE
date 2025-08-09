// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnPlayer.h"
#include "GameFramework/PlayerController.h"    
#include "GameFramework/InputSettings.h"  
#include "GameFramework/CharacterMovementComponent.h"
#include "Turn/TurnCombatBridgeComponent.h"

ATurnPlayer::ATurnPlayer()
{
	GetCharacterMovement()->bOrientRotationToMovement = false; // �̵� �������� �ڵ� ȸ�� ����
	bUseControllerRotationYaw = true;
}

void ATurnPlayer::BeginPlay()
{
	Super::BeginPlay();

	//���� ����
	CharacterStats.Agility = 10.f;

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

void ATurnPlayer::UseSkill_Q()
{
	// �⺻ ���� ó��
	DefaultAttack();

	// AP +1 (Ŭ����)
	CharacterStats.CurrentAP = FMath::Clamp(CharacterStats.CurrentAP + 1, 0, CharacterStats.MAXAP);

	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Used Q skill �� CurrentAP: %d"), CharacterStats.CurrentAP);

	RequestEndTurn();
	UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Used Q skill, TurnEnd"));
}

bool ATurnPlayer::UseSkill_WithCost(int32 Cost)
{
	if (Cost > 0)
	{
		if (CharacterStats.CurrentAP < Cost)
		{
			UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] Not enough AP for skill. Need: %d, Cur: %d"),
				Cost, CharacterStats.CurrentAP);
			return false;
		}

		CharacterStats.CurrentAP = FMath::Clamp(CharacterStats.CurrentAP - Cost, 0, CharacterStats.MAXAP);
	}

	// ��ų ȿ�� ����...
	// TODO: ���� ��ų ���� ����

	// ��Ģ: �ڽ�Ʈ�� ��ų�� ���� �� ��� �� ����
	RequestEndTurn();

	return true;
}

void ATurnPlayer::Fire()
{
	// ��Ŭ�� ������ ���� AP �Ҹ�
	const bool bShouldSpendAP = bIsAiming;
	const int32 APCost = 1;

	if (bShouldSpendAP)
	{
		if (CharacterStats.CurrentAP < APCost)
		{
			UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] Not enough AP to Fire. CurrentAP: %d"),
				CharacterStats.CurrentAP);
			return; // AP ���� �� �߻� �� ��
		}
		// AP ����
		CharacterStats.CurrentAP = FMath::Clamp(CharacterStats.CurrentAP - APCost, 0, CharacterStats.MAXAP);
	}

	// �θ� Fire() ���� (���� �߻� ó��)
	Super::Fire();

	// AP �Ҹ��߰�, 0�� �Ǿ����� �� ����
	if (bShouldSpendAP && CharacterStats.CurrentAP == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] AP reached 0 �� Ending turn"));
		RequestEndTurn();
	}
}

void ATurnPlayer::RequestEndTurn()
{
	if (TurnBridge)
	{
		//������ ���� �ѱ�� ��츦 ���� ���ܵ�
		TurnBridge->NotifySkillFinished();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] RequestEndTurn called but TurnBridge is null"));
	}
}
