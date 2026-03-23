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
#include "AimUIWidget.h"
#include "COEUserWidget.h"
#include "Components/WidgetComponent.h"


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
	CameraBoom->SocketOffset = FVector(100.f,80.f, 50.f);

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = true; //컨트롤러 따라가게 바꿈
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// HP 위젯 컴포넌트 생성
	HPWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPWidgetComponent"));
	if (HPWidgetComponent)
	{
		HPWidgetComponent->SetupAttachment(RootComponent);
		// 캐릭터 머리 위 위치로 설정
		HPWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));

		// 월드 스페이스 설정 (항상 카메라를 바라봄)
		HPWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		HPWidgetComponent->SetDrawSize(FVector2D(200.0f, 40.0f));

		// 거리에 따른 스케일 설정
		HPWidgetComponent->SetDrawAtDesiredSize(true);

		// 기본적으로 숨김 상태
		HPWidgetComponent->SetVisibility(false);
	}

	
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

	// 레벨 전환이 완료되었으므로 플래그를 false로 초기화합니다.
	GI->bIsTransitioning = false;

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


	}

	//TurnBComvatBridegeComponent 할당
	if (!TurnBridge)
	{
		TurnBridge = FindComponentByClass<UTurnCombatBridgeComponent>();
	}

	// 조준 UI 위젯 초기화
	InitializeAimUIWidget();

	// HP 위젯 초기화
	InitializeHPWidget();

}

void ACOECharacter::DefaultAttack()
{

	bIsAttacking = true;

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
		Rot,										//회전 (기본 값)
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

	//DrawDebugCapsule(GetWorld(), Center, HalfHeight, AttackRadius, Rot, DrawColor, false, 2.f);

	// 파티클 스폰
	SpawnDefaultAttackEmitter();

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

				// 전투 진입 시 슬로우모션 효과와 함께 레벨 전환
				StartCombatTransition(Enemy, SelectedBattleMap, true); // 플레이어 선공

			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Enemy has no PossibleBattleLevels!"));
			}
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("DefaultAttack() called in TurnBattleLevel!"));
}

void ACOECharacter::StartCombatTransition(AExplorationEnemy* Enemy, FName BattleMapName, bool bPlayerInitiative)
{
	if (!Enemy) 
		return;

	UCOEGameInstance* GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this));
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("[CombatTransition] GameInstance is null!"));
		return;
	}

	// 이미 레벨 전환이 진행 중인지 확인
	if (GI->bIsTransitioning)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatTransition] Blocked duplicate transition request."));
		return; // 중복 호출 방지
	}

	// 전환 시작: 플래그를 true로 설정하여 다른 요청을 막음
	GI->bIsTransitioning = true;

	UE_LOG(LogTemp, Warning, TEXT("[CombatTransition] 전투 진입 시작 - 슬로우모션 활성화"));

	// 조준 UI 강제로 숨김 (전투 진입 시)
	HideAimUI();

	// 1) GameInstance에 전투 정보 저장
	FString CurrLevel = UGameplayStatics::GetCurrentLevelName(this, true);
	FName ThisEnemyName = Enemy->GetFName();

	GI->bPlayerInitiative = bPlayerInitiative;
	GI->bPlayerWasDetected = !bPlayerInitiative;
	GI->ReturnLocation = GetActorLocation();
	GI->ReturnMapName = FName(*CurrLevel);
	GI->EnemyToRemoveName.AddUnique(ThisEnemyName);
	

	// 2) 월드 슬로우모션 시작 (0.1배속으로 설정)
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.3f);

	// 3) 전투 진입 사운드 재생 (선택사항)
	//if (CombatEnterSound)
	//{
	//	UGameplayStatics::PlaySound2D(this, CombatEnterSound, 1.0f, 1.0f);
	//}

	// 4) 일정 시간 후 레벨 전환 (실제 시간으로 1.초)
	FTimerHandle TransitionHandle;
	GetWorld()->GetTimerManager().SetTimer(
		TransitionHandle,
		[this, BattleMapName]()
		{
			// 슬로우모션 해제 후 레벨 전환
			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);

			UE_LOG(LogTemp, Warning, TEXT("[CombatTransition] 레벨 전환 실행: %s"), *BattleMapName.ToString());
			UGameplayStatics::OpenLevel(this, BattleMapName);
		},
		0.3f,    // 실제 시간으로 1.초
		false
	);

	// 5) 시각적 효과 추가 (선택사항 - 블루프린트에서 구현 권장)
	//OnCombatTransitionStarted.Broadcast();
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
			//DrawDebugLine(GetWorld(), AimLocation, TargetLocation, FColor::Green, false, 2.f);
			UE_LOG(LogTemp, Log, TEXT("Hit : %s"), *HitResult.GetActor()->GetName());

			//공격판정이 들어가면 데미지 적용
			UGameplayStatics::ApplyDamage(HitResult.GetActor(), 10.f, GetInstigatorController(), this, nullptr);
		}
		else
		{
			//DrawDebugLine(GetWorld(), AimLocation, TargetLocation, FColor::Red, false, 2.f);
		}

		// 원거리 Montage 적용시 소켓 위치에서 Aim ImpactPoint로 공격이 갈 수 있게 조정
		FTransform SocketTransform = GetMesh()->GetSocketTransform(FName("RangedSocket"));
		SocketLocation = SocketTransform.GetLocation();
		FVector DeltaVector = TargetLocation - SocketLocation;
		SocketRotation = FRotationMatrix::MakeFromX(DeltaVector).Rotator();

		SpawnRangedEmitter(TargetLocation);
	}
}

void ACOECharacter::SetAiming(bool bNewAiming)
{
	bIsAiming = bNewAiming;
	GetWorldTimerManager().ClearTimer(AimingInterpTimerHandle); // 이전 타이머 정리

	StartSocketOffset = CameraBoom->SocketOffset;
	TargetSocketOffset = bIsAiming
		? FVector(300.f, 80.f, 50.f)
		: FVector(100.f, 80.f, 50.f);


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

	// 조준 UI 표시/숨김 처리
	if (bIsAiming)
	{
		ShowAimUI();
	}
	else
	{
		HideAimUI();
	}

}

float ACOECharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	//받은 데미지 표시
	float HP = CharacterStats.CurrentHP;

	HP -= DamageAmount;
	CharacterStats.CurrentHP = HP;

	// HP 변경 알림
	OnHPChanged();

	//피격 애니메이션 재생
	if (DamageAmount > 0 && IsValid(AnimInstance))
	{
		AnimInstance->HitAnim();
	}

	// 사망시 HP 위젯 표시 갱신
	if (HP <= 0)
	{
		// 사망 시 HP 위젯 숨김
		SetHPWidgetVisible(false);
		SetLifeSpan(2.f);
	}


	UE_LOG(LogTemp, Log, TEXT("Damaged : %f"), HP);
	return 0;
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

void ACOECharacter::SpawnRangedEmitter(FVector TargetLocation)
{
	   // 파티클 스폰
	   // 1. RangedSocket에서 발사 효과
	if (MuzzleFlashParticle)
	{
		UGameplayStatics::SpawnEmitterAttached(
			MuzzleFlashParticle,
			GetMesh(),
			FName("RangedSocket"),  // 소켓에 직접 부착
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true  // Auto Destroy
		);
	}

	// 2. ImpactPoint에서 충돌 효과
	if (ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactParticle,
			TargetLocation,  // 충돌 지점
			FRotator::ZeroRotator,
			FVector(1.0f),
			true  // Auto Destroy
		);
	}
}

void ACOECharacter::SpawnDefaultAttackEmitter()
{
	// 기본공격 파티클 스폰
	if (DefaultAttackParticle)
	{
		UGameplayStatics::SpawnEmitterAttached(
			DefaultAttackParticle,
			GetMesh(),
			FName("DefaultAttackSocket"),  // 소켓에 직접 부착
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true  // Auto Destroy
		);
	}
}

void ACOECharacter::SpawnHealEmitter(ACOECharacter* Target)
{
	// Heal 파티클 스폰
	if (HealParticle)
	{
		// 캐릭터 발밑 위치 계산
		FVector TargetLocation = Target->GetActorLocation();
		FVector GroundLocation = TargetLocation;
		GroundLocation.Z -= Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(); // 캡슐 높이만큼 내리기

		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			HealParticle,
			GroundLocation,                    // 바닥 위치
			FRotator::ZeroRotator,            // 회전
			true                              // Auto Destroy
		);
	}
}

void ACOECharacter::SpawnSkillEmitter(ACOECharacter* Target)
{
	// Skill 파티클 스폰
	if (SkillParticle)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnSkillEmitter] SkillParticle found, spawning..."));

		// 캐릭터 발밑 위치 계산
		FVector TargetLocation = Target->GetActorLocation();
		FVector GroundLocation = TargetLocation;
		GroundLocation.Z -= Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(); // 캡슐 높이만큼 내리기

		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			SkillParticle,
			GroundLocation,                    // 바닥 위치
			FRotator::ZeroRotator,
			true
		);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SpawnSkillEmitter] SkillParticle is NULL on %s"), *GetName());
	}
}

void ACOECharacter::InitializeAimUIWidget()
{
	// AimUIWidgetClass가 설정되어 있고 아직 위젯이 생성되지 않았다면 생성
	if (AimUIWidgetClass && !AimUIWidget)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			// 위젯 생성
			AimUIWidget = CreateWidget<UAimUIWidget>(PC, AimUIWidgetClass);
			if (AimUIWidget)
			{
				// 뷰포트에 추가 (높은 ZOrder로 다른 UI보다 위에 표시)
				AimUIWidget->AddToViewport(999);

				// 초기에는 숨김 상태
				AimUIWidget->HideAimUI();

				UE_LOG(LogTemp, Log, TEXT("[COECharacter] Aim UI Widget initialized for %s"), *GetName());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[COECharacter] Failed to create Aim UI Widget for %s"), *GetName());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[COECharacter] No PlayerController found - Aim UI Widget will be created later"));
		}
	}
	else if (!AimUIWidgetClass)
	{
		UE_LOG(LogTemp, Log, TEXT("[COECharacter] AimUIWidgetClass not set for %s - Aim UI disabled"), *GetName());
	}
}

void ACOECharacter::ShowAimUI()
{
	// 위젯이 없으면 다시 초기화 시도
	if (!AimUIWidget)
	{
		InitializeAimUIWidget();
	}

	// 위젯이 있으면 표시
	if (AimUIWidget)
	{
		AimUIWidget->ShowAimUI();
		UE_LOG(LogTemp, Log, TEXT("[COECharacter] Aim UI shown for %s"), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[COECharacter] Cannot show Aim UI - widget not available for %s"), *GetName());
	}
}

void ACOECharacter::HideAimUI()
{
	// 위젯이 있으면 숨김
	if (AimUIWidget)
	{
		AimUIWidget->HideAimUI();
		UE_LOG(LogTemp, Log, TEXT("[COECharacter] Aim UI hidden for %s"), *GetName());
	}
}

void ACOECharacter::SetHPWidgetVisible(bool bVisible)
{
	if (HPWidgetComponent)
	{
		HPWidgetComponent->SetVisibility(bVisible);
	}

	if (HPWidget)
	{
		HPWidget->SetWidgetVisible(bVisible);
	}
}

void ACOECharacter::RefreshHPWidget()
{
	if (HPWidget)
	{
		HPWidget->UpdateHp();
	}
}

void ACOECharacter::InitializeHPWidget()
{
	if (!HPWidgetClass || !HPWidgetComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[COECharacter] HPWidgetClass or HPWidgetComponent not set for %s"), *GetName());
		return;
	}

	// 위젯 생성 및 설정
	HPWidget = CreateWidget<UCOEUserWidget>(GetWorld(), HPWidgetClass);
	if (HPWidget)
	{
		// 위젯 컴포넌트에 설정
		HPWidgetComponent->SetWidget(HPWidget);

		// 캐릭터와 바인딩
		HPWidget->BindToCharacter(this);

		// 기본적으로 표시 (필요에 따라 조건 추가)
		SetHPWidgetVisible(true);

		UE_LOG(LogTemp, Log, TEXT("[COECharacter] HP Widget initialized for %s"), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[COECharacter] Failed to create HP Widget for %s"), *GetName());
	}
}

void ACOECharacter::OnHPChanged()
{
	// HP 위젯 업데이트
	RefreshHPWidget();
}
