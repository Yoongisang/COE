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

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = true; //컨트롤러 따라가게 바꿈
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ACOECharacter::BeginPlay()
{
	Super::BeginPlay();
	//AnimInstance 캐스트
	AnimInstance = Cast<UCOEAnimInstance>(GetMesh()->GetAnimInstance());
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

void ACOECharacter::DoDefaultAttck()
{
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	float AttackRange = 100.f;
	float AttackRadius = 50.f;
	FVector StartPos = GetActorLocation();
	FVector EndPos = GetActorLocation() + GetActorForwardVector() * AttackRange;

	bool Result = GetWorld()->SweepSingleByChannel
	(
		HitResult,									//충돌 결과를 저장할 변수					
		StartPos,									//시작 지점
		EndPos,										//끝 지점
		FQuat::Identity,							//회전 (기본 값)
		ECC_GameTraceChannel3,						//충돌 채널 (Visibilirty)
		FCollisionShape::MakeSphere(AttackRange),	//형태 : Sphere(구) MakeSphere(반지름)
		Params										//충돌 쿼리 파라미터
	);


	
	FVector Vec = GetActorForwardVector() * AttackRange;
	FVector Center = GetActorLocation() + Vec * 0.5f;
	float HalfHeight = AttackRange * 0.5f + AttackRadius;
	FQuat Rotation = FRotationMatrix::MakeFromZ(Vec).ToQuat();
	FColor DrawColor;

	DrawColor = Result ? FColor::Green : FColor::Red;

	DrawDebugCapsule(GetWorld(), Center, HalfHeight, AttackRadius, Rotation, DrawColor, false, 2.f);

	if (Result && HitResult.GetActor())
	{
		UE_LOG(LogTemp, Log, TEXT("Hit : %s"), *HitResult.GetActor()->GetName());
		//공격판정이 들어가면 데미지 적용
		UGameplayStatics::ApplyDamage(HitResult.GetActor(), 10.f, GetInstigatorController(), this, nullptr);
	}

}

float ACOECharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	//받은 데미지 표시
	UE_LOG(LogTemp, Log, TEXT("Damaged : %f"), DamageAmount);
	return 0.f;
}


