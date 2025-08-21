// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"     
#include "InputAction.h"        
#include "InputActionValue.h" 
#include "TurnPlayer.h"
#include "Engine/LocalPlayer.h"      
#include "GameFramework/Character.h"


void ATurnPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Add Input Mapping Contexts

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{

		// SkillAction Input
		EnhancedInputComponent->BindAction(QSkillAction, ETriggerEvent::Triggered, this, &ATurnPlayerController::OnSkillQ);
		EnhancedInputComponent->BindAction(WSkillAction, ETriggerEvent::Triggered, this, &ATurnPlayerController::OnSkillW);
		EnhancedInputComponent->BindAction(ESkillAction, ETriggerEvent::Triggered, this, &ATurnPlayerController::OnSkillE);
		EnhancedInputComponent->BindAction(ASkillAction, ETriggerEvent::Triggered, this, &ATurnPlayerController::OnSkillA);
		EnhancedInputComponent->BindAction(SSkillAction, ETriggerEvent::Triggered, this, &ATurnPlayerController::OnSkillS);
		EnhancedInputComponent->BindAction(DSkillAction, ETriggerEvent::Triggered, this, &ATurnPlayerController::OnSkillD);

		//Look 
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ATurnPlayerController::Look);

		//MouseLeftClick
		EnhancedInputComponent->BindAction(MouseLeftClick, ETriggerEvent::Triggered, this, &ATurnPlayerController::DoMouseLeftClick);

		//MouseRightClick
		EnhancedInputComponent->BindAction(MouseRightClick, ETriggerEvent::Started, this, &ATurnPlayerController::DoMouseRightClickStart);
		EnhancedInputComponent->BindAction(MouseRightClick, ETriggerEvent::Completed, this, &ATurnPlayerController::DoMouseRightClickEnd);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ATurnPlayerController::Look(const FInputActionValue& Value)
{

	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);

}

void ATurnPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	TurnChar = Cast<ATurnPlayer>(InPawn);
	UE_LOG(LogTemp, Log, TEXT("[TPC] OnPossess -> %s"), *GetNameSafe(TurnChar));
}

void ATurnPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
	TurnChar = nullptr;
	UE_LOG(LogTemp, Log, TEXT("[TPC] OnUnPossess"));

}


void ATurnPlayerController::BeginPlay()
{
	Super::BeginPlay();

	TurnChar = Cast<ATurnPlayer>(GetCharacter());
}

void ATurnPlayerController::DoLook(float Yaw, float Pitch)
{
	if (TurnChar != nullptr)
	{
		// add yaw and pitch input to controller
		TurnChar->AddControllerYawInput(Yaw);
		TurnChar->AddControllerPitchInput(Pitch);
	}
}

void ATurnPlayerController::OnSkillQ()
{
	if (!TurnChar || TurnChar->bIsAttacking)
		return;
	

	if (TurnChar->CanPerformAction())
	{
		TurnChar->UseSkill_Q();
		UE_LOG(LogTemp, Log, TEXT("Input : Q"));
	}
	// Enemy Turn 중이면 패링 모드
	else if (TurnChar->IsEnemyTurnActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] 패링!"));
		return;
	}
}

void ATurnPlayerController::OnSkillW()
{
	if (!TurnChar || TurnChar->bIsAttacking)
		return;

	if (TurnChar->CanPerformAction())
	{
		TurnChar->UseHPPotion();
		UE_LOG(LogTemp, Log, TEXT("Input : W"));
	}
	// Enemy Turn 중이면 패링 모드
	else if (TurnChar->IsEnemyTurnActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TurnPlayer] 회피!"));
		return;
	}
}

void ATurnPlayerController::OnSkillE()
{
	if (!TurnChar || TurnChar->bIsAttacking)
		return;

	// Enemy Turn 중이면 동작하지 않음
	if (TurnChar->IsEnemyTurnActive())
	{
		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] E - Enemy Turn 중 비활성"));
		return;
	}

	TurnChar->UseAPPotion();
	UE_LOG(LogTemp, Log, TEXT("Input : E"));
}

void ATurnPlayerController::OnSkillA()
{
	if (!TurnChar || TurnChar->bIsAttacking)
		return;

	// Enemy Turn 중이면 동작하지 않음
	if (TurnChar->IsEnemyTurnActive())
	{
		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] A - Enemy Turn 중 비활성"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Input : A"));
}

void ATurnPlayerController::OnSkillS()
{
	if (!TurnChar || TurnChar->bIsAttacking)
		return;

	// Enemy Turn 중이면 동작하지 않음
	if (TurnChar->IsEnemyTurnActive())
	{
		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] S - Enemy Turn 중 비활성"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Input : S"));
}

void ATurnPlayerController::OnSkillD()
{
	if (!TurnChar || TurnChar->bIsAttacking)
		return;

	// Enemy Turn 중이면 동작하지 않음
	if (TurnChar->IsEnemyTurnActive())
	{
		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] D - Enemy Turn 중 비활성"));
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("Input : D"));
}

void ATurnPlayerController::DoMouseLeftClick()
{
	if (!TurnChar)
		return;

	// Enemy Turn 중이면 동작하지 않음
	if (TurnChar->IsEnemyTurnActive())
	{
		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Mouse Left Click - Enemy Turn 중 비활성"));
		return;
	}

	if (!TurnChar->bIsAiming)
		return;
		
	UE_LOG(LogTemp, Log, TEXT("MouseLeftClick"));
	TurnChar->Fire();

}

void ATurnPlayerController::DoMouseRightClickStart()
{
	if (!TurnChar)
		return;

	// Enemy Turn 중이면 동작하지 않음
	if (TurnChar->IsEnemyTurnActive())
	{
		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Mouse Right Click Start - Enemy Turn 중 비활성"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("MouseRightClickStart"));
	TurnChar->SetAiming(true);

}

void ATurnPlayerController::DoMouseRightClickEnd()
{
	if (!TurnChar)
		return;

	// Enemy Turn 중이면 동작하지 않음
	if (TurnChar->IsEnemyTurnActive())
	{
		UE_LOG(LogTemp, Log, TEXT("[TurnPlayer] Mouse Right Click End - Enemy Turn 중 비활성"));
		return;
	}

	TurnChar->SetAiming(false);
	UE_LOG(LogTemp, Log, TEXT("MouseRightClickEnd"));
}