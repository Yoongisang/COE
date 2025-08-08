// Copyright Epic Games, Inc. All Rights Reserved.

#include "COECharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "COEAnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "COEGameInstance.h"
#include "Exploration/ExplorationEnemy.h"
#include "EngineUtils.h"
#include "Turn/TurnCombatBridgeComponent.h"
//#include "EnhancedInputComponent.h"
//#include "EnhancedInputSubsystems.h"
//#include "InputActionValue.h"

//DEFINE_LOG_CATEGORY(LogTemplateCharacter);

ACOECharacter::ACOECharacter()
{
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false; //
	bUseControllerRotationYaw = true; //컨트롤러 따라가게 바꿈
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; //컨트롤러 따라가게 바꿈
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(200.f,80.f, 50.f);

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = true; //컨트롤러 따라가게 바꿈
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	CharacterStats.MAXHP = 50;

	
}

void ACOECharacter::BeginPlay()
{
	Super::BeginPlay();
	//AnimInstance 캐스트
	AnimInstance = Cast<UCOEAnimInstance>(GetMesh()->GetAnimInstance());
	CharacterStats.CurrentHP = CharacterStats.MAXHP;
	UE_LOG(LogTemp, Log, TEXT("Damaged : %f"), CharacterStats.CurrentHP);

	auto* GI = GetWorld()->GetGameInstance<UCOEGameInstance>();
	if (!GI) return;

	// 1) 플레이어 위치 복원
	FString Curr = UGameplayStatics::GetCurrentLevelName(this, true);
	if (GI->ReturnMapName.ToString() == Curr)
	{
		if (auto* PC = Cast<ACOECharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			PC->SetActorLocation(GI->ReturnLocation);
			UE_LOG(LogTemp, Log, TEXT("[ExplorationGM] 위치 복원 → %s"),
				*GI->ReturnLocation.ToString());
		}

		// 2) 쓰러진 적 제거
		if (GI->EnemyToRemoveName.Num() > 0)
		{
			for (const FName& EnemyName : GI->EnemyToRemoveName)
			{
				for (TActorIterator<AExplorationEnemy> It(GetWorld()); It; ++It)
				{
					// 탐색 모드에서 사용된 적 클래스로 캐스팅
					if (It->GetFName() == EnemyName)
					{
						It->Destroy();
						UE_LOG(LogTemp, Log, TEXT("[ExplorationGM] 제거된 적 → %s"),
							*EnemyName.ToString());
						break;
					}
				}
			}
		
		}

		GI->bPlayerInitiative = false; // 플레이어가 먼저 공격
		GI->bPlayerWasDetected = false;
		GI->ReturnLocation = FVector::ZeroVector;
		GI->ReturnMapName = NAME_None; // 실제 탐색맵 이름으로 바꿔야 함
		//GI->EnemyToRemoveName = NAME_None;
	}
}

void ACOECharacter::DefaultAttack()
{
	//공격 상태
	
	//AnimInstance가 nullptr이 아니라면 DefaultAttackAnim 실행
	if (IsValid(AnimInstance))
	{
		
		AnimInstance->DefaultAttackAnim();
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Null"));
	}
	UE_LOG(LogTemp, Log, TEXT("DefaultAttack"));

}

void ACOECharacter::DoDefaultAttack()
{
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.bTraceComplex = false;
	Params.bReturnPhysicalMaterial = false;

	float AttackRange = 250.f;
	float AttackRadius = 50.f;
	FVector StartPos = GetActorLocation();
	FVector EndPos = GetActorLocation() + GetActorForwardVector() * AttackRange;
	FQuat Rot = FRotationMatrix::MakeFromZ(EndPos - StartPos).ToQuat();
	bool Result = GetWorld()->SweepSingleByChannel
	(
		HitResult,									//충돌 결과를 저장할 변수					
		StartPos,									//시작 지점
		EndPos,										//끝 지점
		Rot,							//회전 (기본 값)
		ECC_GameTraceChannel3,						//충돌 채널 (Visibilirty)
		FCollisionShape::MakeSphere(AttackRadius),	//형태 : Sphere(구) MakeSphere(반지름)
		Params										//충돌 쿼리 파라미터
	);


	
	//FVector Vec = GetActorForwardVector() * AttackRange;
	FVector Center = (StartPos + EndPos) * 0.5f;
	float HalfHeight = AttackRange * 0.5f;
	//FQuat Rotation = FRotationMatrix::MakeFromZ(Vec).ToQuat();
	FColor DrawColor;

	DrawColor = Result ? FColor::Green : FColor::Red;

	DrawDebugCapsule(GetWorld(), Center, HalfHeight, AttackRadius, Rot, DrawColor, false, 2.f);

	if (Result && HitResult.GetActor())
	{
		UE_LOG(LogTemp, Log, TEXT("Hit : %s"), *HitResult.GetActor()->GetName());
		//공격판정이 들어가면 데미지 적용
		UGameplayStatics::ApplyDamage(HitResult.GetActor(), 10.f, GetInstigatorController(), this, nullptr);
		//Enemy로 탐색상태의 적 캐스팅
		if (AExplorationEnemy* Enemy = Cast<AExplorationEnemy>(HitResult.GetActor()))
		{	
			//전투맵 리스트 확인
			if (Enemy->PossibleBattleLevels.Num() > 0)
			{
				//SelectedBattleMap에 전투맵 리스트 할당
				FName SelectedBattleMap = Enemy->PossibleBattleLevels[FMath::RandRange(0, Enemy->PossibleBattleLevels.Num() - 1)];
				//GameInstance에 전투 정보 저장
				if (UCOEGameInstance* GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this)))
				{
					FString CurrLevel = UGameplayStatics::GetCurrentLevelName(this, true);
					FName ThisEnemyName = HitResult.GetActor()->GetFName();
					
					GI->bPlayerInitiative = true; // 플레이어가 먼저 공격
					GI->bPlayerWasDetected = false;
					GI->ReturnLocation = GetActorLocation();
					GI->ReturnMapName = FName(*CurrLevel); // 실제 탐색맵 이름으로 바꿔야 함
					GI->EnemyToRemoveName.AddUnique(ThisEnemyName);
				}

				//전투맵으로 이동
				UGameplayStatics::OpenLevel(this, SelectedBattleMap);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Enemy has no PossibleBattleLevels!"));
			}
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("DefaultAttack() called in TurnBattleLevel!"));
}

void ACOECharacter::Fire()
{
	UE_LOG(LogTemp, Log, TEXT("Fire"));

	if (IsValid(AnimInstance))
	{
		//bIsShooting = true;

		//AnimInstance->PlayAttackMontage();

		float AttackRange = 10000.f;

		FHitResult HitResult;

		//auto
		APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);

		FVector AimLocation = CameraManager->GetCameraLocation();
		FVector TargetLocation = AimLocation + CameraManager->GetActorForwardVector() * AttackRange;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		bool Result = GetWorld()->LineTraceSingleByChannel
		(
			OUT HitResult,
			AimLocation,
			TargetLocation,
			ECollisionChannel::ECC_GameTraceChannel3,
			Params

		);

		if (Result)
		{
			TargetLocation = HitResult.ImpactPoint;
			DrawDebugLine(GetWorld(), AimLocation, TargetLocation, FColor::Green, false, 2.f);
			UE_LOG(LogTemp, Log, TEXT("Hit : %s"), *HitResult.GetActor()->GetName());

			//공격판정이 들어가면 데미지 적용
			UGameplayStatics::ApplyDamage(HitResult.GetActor(), 10.f, GetInstigatorController(), this, nullptr);
		}
		else
		{
			DrawDebugLine(GetWorld(), AimLocation, TargetLocation, FColor::Red, false, 2.f);
		}

		// 원거리 Montage 적용시 소켓 위치에서 Aim ImpactPoint로 공격이 갈 수 있게 조정
		//FTransform SocketTransform = GetMesh()->GetSocketTransform(FName("ArrowSocket"));
		//SocketLocation = SocketTransform.GetLocation();
		//FVector DeltaVector = TargetLocation - SocketLocation;
		//SocketRotation = FRotationMatrix::MakeFromX(DeltaVector).Rotator();

	}
}

void ACOECharacter::SetAiming(bool bNewAiming)
{
	bIsAiming = bNewAiming;
	GetWorldTimerManager().ClearTimer(AimingInterpTimerHandle); // 이전 타이머 정리

	StartSocketOffset = CameraBoom->SocketOffset;
	TargetSocketOffset = bIsAiming
		? FVector(300.f, 80.f, 50.f)
		: FVector(200.f, 80.f, 50.f);


	InterpAlpha = 0.f;
	bInterpToAiming = bIsAiming;

	GetWorldTimerManager().SetTimer
	(
		AimingInterpTimerHandle,
		this,
		&ACOECharacter::UpdateAimingInterp,
		0.01f, // 10ms 간격
		true   // 반복 실행
	);
}

float ACOECharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	//받은 데미지 표시
	float HP = CharacterStats.CurrentHP;

	HP -= DamageAmount;
	CharacterStats.CurrentHP = HP;

	if (HP <= 0)
	{
		SetLifeSpan(2.f);
	}


	UE_LOG(LogTemp, Log, TEXT("Damaged : %f"), HP);
	return 0.f;
}

void ACOECharacter::UpdateAimingInterp()
{
	InterpAlpha +=  0.05f; // 0~1로 점점 증가 (속도 조절 가능)

	FVector NewOffset = FMath::Lerp(StartSocketOffset, TargetSocketOffset, InterpAlpha);
	CameraBoom->SocketOffset = NewOffset;

	if (InterpAlpha >= 1.0f)
	{
		CameraBoom->SocketOffset = TargetSocketOffset;
		GetWorldTimerManager().ClearTimer(AimingInterpTimerHandle);
	}
}


