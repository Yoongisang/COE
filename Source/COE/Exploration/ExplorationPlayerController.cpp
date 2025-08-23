// Fill out your copyright notice in the Description page of Project Settings.


#include "ExplorationPlayerController.h"
#include "ExplorationPlayer.h"
#include "BaseCode/COEPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"     
#include "Engine/LocalPlayer.h"          
#include "InputMappingContext.h"         

void AExplorationPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// ChangeCharacter
		EnhancedInputComponent->BindAction(ChangeCharacter, ETriggerEvent::Started, this, &AExplorationPlayerController::DoChangeCharacter);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}

}

void AExplorationPlayerController::SetupInputMappingContext()
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// 기존 컨텍스트 모두 제거
		Subsystem->ClearAllMappings();

		// 다시 추가
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			if (CurrentContext)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
				UE_LOG(LogTemp, Log, TEXT("[ExplorationPC] Added mapping context: %s"), *CurrentContext->GetName());
			}
		}
	}
}

void AExplorationPlayerController::BeginPlay()
{
	Super::BeginPlay();
	ExplorationChar = Cast<AExplorationPlayer>(GetCharacter());

	// 부모의 COEChar와 동기화
	if (ExplorationChar)
	{
		COEChar = ExplorationChar;
		UE_LOG(LogTemp, Warning, TEXT("[ExplorationPC] BeginPlay - COEChar synced: %s"), *COEChar->GetName());
	}

	// 초기 캐릭터가 없을 경우를 대비한 지연 재시도
	if (!ExplorationChar)
	{
		FTimerHandle RetryHandle;
		GetWorld()->GetTimerManager().SetTimer(RetryHandle,
			[this]()
			{
				ExplorationChar = Cast<AExplorationPlayer>(GetCharacter());
				if (ExplorationChar)
				{
					COEChar = ExplorationChar;  // 여기서도 동기화
					UE_LOG(LogTemp, Log, TEXT("[ExplorationPC] Character found on retry and synced"));
				}
			},
			0.1f, false);
	}
}

void AExplorationPlayerController::DoMouseLeftClick()
{
	if (!ExplorationChar->bIsAiming)
	{
		ExplorationChar->UseExplorationFullHeal();
	}
	else
	{
		Super::DoMouseLeftClick();
	}
	


}

void AExplorationPlayerController::DoChangeCharacter()
{
	if (ExplorationChar->bIsAttacking || ExplorationChar->bIsAiming)
		return;

	ExplorationChar->SwitchToNextCharacter();
}

void AExplorationPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ExplorationChar = Cast<AExplorationPlayer>(InPawn);

	if (ExplorationChar)
	{
		// 핵심: 부모 클래스의 COEChar도 업데이트!
		COEChar = ExplorationChar;

		UE_LOG(LogTemp, Warning, TEXT("[ExplorationPC] OnPossess -> %s"), *ExplorationChar->GetName());
		UE_LOG(LogTemp, Warning, TEXT("[ExplorationPC] COEChar updated to: %s"),
			COEChar ? *COEChar->GetName() : TEXT("NULL"));

		// 입력 매핑 컨텍스트 다시 설정
		SetupInputMappingContext();

		// 입력 모드 재설정
		SetInputMode(FInputModeGameOnly());

		// 입력 컴포넌트 재활성화
		if (InputComponent)
		{
			InputComponent->bBlockInput = false;
			UE_LOG(LogTemp, Log, TEXT("[ExplorationPC] Input component reactivated"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ExplorationPC] OnPossess failed to cast to ExplorationPlayer"));
	}
}
