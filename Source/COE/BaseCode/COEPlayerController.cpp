// Copyright Epic Games, Inc. All Rights Reserved.


#include "COEPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h" 
#include "COECharacter.h"

void ACOEPlayerController::SetupInputComponent()
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

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACOEPlayerController::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACOEPlayerController::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACOEPlayerController::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACOEPlayerController::Look);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACOEPlayerController::Look);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}

	

}
ACOEPlayerController::ACOEPlayerController()
{
}

void ACOEPlayerController::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();
	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ACOEPlayerController::Look(const FInputActionValue& Value)
{

	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);

}

void ACOEPlayerController::DoMove(float Right, float Forward)
{
	ACharacter* ControllerCharacter = this->GetCharacter();
	if (ControllerCharacter != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = ControllerCharacter->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		ControllerCharacter->AddMovementInput(ForwardDirection, Forward);
		ControllerCharacter->AddMovementInput(RightDirection, Right);
	}
}

void ACOEPlayerController::DoLook(float Yaw, float Pitch)
{
	
	
	if (APawn* ControllerPawn = GetPawn())
	{
		ControllerPawn->AddControllerYawInput(Yaw);
		ControllerPawn->AddControllerPitchInput(Pitch);
	}
}
void ACOEPlayerController::DoJumpStart()
{
	// signal the character to jump
	ACharacter* ControllerCharacter = this->GetCharacter();
	if (ControllerCharacter != nullptr)
	{
		ControllerCharacter->Jump();
	}
}

void ACOEPlayerController::DoJumpEnd()
{
	// signal the character to stop jumping
	ACharacter* ControllerCharacter = this->GetCharacter();
	if (ControllerCharacter != nullptr)
	{
		ControllerCharacter->StopJumping();
	}
}