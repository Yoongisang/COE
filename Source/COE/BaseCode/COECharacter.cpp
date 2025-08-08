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
	bUseControllerRotationYaw = true; //��Ʈ�ѷ� ���󰡰� �ٲ�
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; //��Ʈ�ѷ� ���󰡰� �ٲ�
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
	FollowCamera->bUsePawnControlRotation = true; //��Ʈ�ѷ� ���󰡰� �ٲ�
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	CharacterStats.MAXHP = 50;

	
}

void ACOECharacter::BeginPlay()
{
	Super::BeginPlay();
	//AnimInstance ĳ��Ʈ
	AnimInstance = Cast<UCOEAnimInstance>(GetMesh()->GetAnimInstance());
	CharacterStats.CurrentHP = CharacterStats.MAXHP;
	UE_LOG(LogTemp, Log, TEXT("Damaged : %f"), CharacterStats.CurrentHP);

	auto* GI = GetWorld()->GetGameInstance<UCOEGameInstance>();
	if (!GI) return;

	// 1) �÷��̾� ��ġ ����
	FString Curr = UGameplayStatics::GetCurrentLevelName(this, true);
	if (GI->ReturnMapName.ToString() == Curr)
	{
		if (auto* PC = Cast<ACOECharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			PC->SetActorLocation(GI->ReturnLocation);
			UE_LOG(LogTemp, Log, TEXT("[ExplorationGM] ��ġ ���� �� %s"),
				*GI->ReturnLocation.ToString());
		}

		// 2) ������ �� ����
		if (GI->EnemyToRemoveName.Num() > 0)
		{
			for (const FName& EnemyName : GI->EnemyToRemoveName)
			{
				for (TActorIterator<AExplorationEnemy> It(GetWorld()); It; ++It)
				{
					// Ž�� ��忡�� ���� �� Ŭ������ ĳ����
					if (It->GetFName() == EnemyName)
					{
						It->Destroy();
						UE_LOG(LogTemp, Log, TEXT("[ExplorationGM] ���ŵ� �� �� %s"),
							*EnemyName.ToString());
						break;
					}
				}
			}
		
		}

		GI->bPlayerInitiative = false; // �÷��̾ ���� ����
		GI->bPlayerWasDetected = false;
		GI->ReturnLocation = FVector::ZeroVector;
		GI->ReturnMapName = NAME_None; // ���� Ž���� �̸����� �ٲ�� ��
		//GI->EnemyToRemoveName = NAME_None;
	}
}

void ACOECharacter::DefaultAttack()
{
	//���� ����
	
	//AnimInstance�� nullptr�� �ƴ϶�� DefaultAttackAnim ����
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
		HitResult,									//�浹 ����� ������ ����					
		StartPos,									//���� ����
		EndPos,										//�� ����
		Rot,							//ȸ�� (�⺻ ��)
		ECC_GameTraceChannel3,						//�浹 ä�� (Visibilirty)
		FCollisionShape::MakeSphere(AttackRadius),	//���� : Sphere(��) MakeSphere(������)
		Params										//�浹 ���� �Ķ����
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
		//���������� ���� ������ ����
		UGameplayStatics::ApplyDamage(HitResult.GetActor(), 10.f, GetInstigatorController(), this, nullptr);
		//Enemy�� Ž�������� �� ĳ����
		if (AExplorationEnemy* Enemy = Cast<AExplorationEnemy>(HitResult.GetActor()))
		{	
			//������ ����Ʈ Ȯ��
			if (Enemy->PossibleBattleLevels.Num() > 0)
			{
				//SelectedBattleMap�� ������ ����Ʈ �Ҵ�
				FName SelectedBattleMap = Enemy->PossibleBattleLevels[FMath::RandRange(0, Enemy->PossibleBattleLevels.Num() - 1)];
				//GameInstance�� ���� ���� ����
				if (UCOEGameInstance* GI = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this)))
				{
					FString CurrLevel = UGameplayStatics::GetCurrentLevelName(this, true);
					FName ThisEnemyName = HitResult.GetActor()->GetFName();
					
					GI->bPlayerInitiative = true; // �÷��̾ ���� ����
					GI->bPlayerWasDetected = false;
					GI->ReturnLocation = GetActorLocation();
					GI->ReturnMapName = FName(*CurrLevel); // ���� Ž���� �̸����� �ٲ�� ��
					GI->EnemyToRemoveName.AddUnique(ThisEnemyName);
				}

				//���������� �̵�
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

			//���������� ���� ������ ����
			UGameplayStatics::ApplyDamage(HitResult.GetActor(), 10.f, GetInstigatorController(), this, nullptr);
		}
		else
		{
			DrawDebugLine(GetWorld(), AimLocation, TargetLocation, FColor::Red, false, 2.f);
		}

		// ���Ÿ� Montage ����� ���� ��ġ���� Aim ImpactPoint�� ������ �� �� �ְ� ����
		//FTransform SocketTransform = GetMesh()->GetSocketTransform(FName("ArrowSocket"));
		//SocketLocation = SocketTransform.GetLocation();
		//FVector DeltaVector = TargetLocation - SocketLocation;
		//SocketRotation = FRotationMatrix::MakeFromX(DeltaVector).Rotator();

	}
}

void ACOECharacter::SetAiming(bool bNewAiming)
{
	bIsAiming = bNewAiming;
	GetWorldTimerManager().ClearTimer(AimingInterpTimerHandle); // ���� Ÿ�̸� ����

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
		0.01f, // 10ms ����
		true   // �ݺ� ����
	);
}

float ACOECharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	//���� ������ ǥ��
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
	InterpAlpha +=  0.05f; // 0~1�� ���� ���� (�ӵ� ���� ����)

	FVector NewOffset = FMath::Lerp(StartSocketOffset, TargetSocketOffset, InterpAlpha);
	CameraBoom->SocketOffset = NewOffset;

	if (InterpAlpha >= 1.0f)
	{
		CameraBoom->SocketOffset = TargetSocketOffset;
		GetWorldTimerManager().ClearTimer(AimingInterpTimerHandle);
	}
}


