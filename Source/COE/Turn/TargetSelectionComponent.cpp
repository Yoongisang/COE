// Fill out your copyright notice in the Description page of Project Settings.


#include "TargetSelectionComponent.h"
#include "BaseCode/COECharacter.h"
#include "BaseCode/COEGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"     // 누락된 헤더 추가
#include "GameFramework/Character.h"         // 누락된 헤더 추가
#include "TimerManager.h"    
#include "TurnPlayer.h"


UTargetSelectionComponent::UTargetSelectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

    // 기본값 초기화
    SelectionState = ETargetSelectionState::None;
    CurrentSkillType = ESkillTargetType::Universal;
    CurrentTargetIndex = 0;
    TempCamera = nullptr;
    TempCameraActor = nullptr;

}



void UTargetSelectionComponent::BeginPlay()
{
    Super::BeginPlay();

    // 캐시 초기화
    OwnerCharacter = Cast<ACOECharacter>(GetOwner());
    PlayerController = Cast<APlayerController>(UGameplayStatics::GetPlayerController(this, 0));
    GameInstance = Cast<UCOEGameInstance>(UGameplayStatics::GetGameInstance(this));

    if (!OwnerCharacter.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[TargetSelection] Owner is not a COECharacter"));
    }
}

void UTargetSelectionComponent::StartTargetSelection(ESkillTargetType SkillType)
{
    if (SelectionState != ETargetSelectionState::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TargetSelection] Already in selection mode"));
        return;
    }

    CurrentSkillType = SkillType;
    SelectionState = ETargetSelectionState::Selecting;
    CurrentTargetIndex = 0;

    // 원래 상태 저장
    if (PlayerController.IsValid())
    {
        OriginalViewTarget = PlayerController->GetViewTarget();
        if (OwnerCharacter.IsValid())
        {
            OriginalPlayerRotation = OwnerCharacter->GetActorRotation();

            // 현재 회전값을 다시 저장 (최신 상태로 업데이트)
            if (OwnerCharacter.IsValid())
            {
                OriginalPlayerRotation = OwnerCharacter->GetActorRotation();
                UE_LOG(LogTemp, Log, TEXT("[TargetSelection] Current rotation saved: %s"), *OriginalPlayerRotation.ToString());
            }
        }
    }

    // 타겟 목록 업데이트
    UpdateTargetList();

    if (ValidTargets.Num() > 0)
    {
        CurrentTarget = ValidTargets[0];
        SetupCameraForTarget(CurrentTarget.Get());
        UE_LOG(LogTemp, Log, TEXT("[TargetSelection] Started with %d valid targets"), ValidTargets.Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[TargetSelection] No valid targets found"));
        CancelTargetSelection();
    }
}

void UTargetSelectionComponent::CancelTargetSelection()
{
    if (SelectionState == ETargetSelectionState::None)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[TargetSelection] Cancel initiated."));

    FinalizeSelection(); // 새로운 정리 함수 호출

    OnTargetSelectionCancelled.Broadcast();
}

void UTargetSelectionComponent::FinalizeSelection()
{
    if (SelectionState == ETargetSelectionState::None)
    {
        return; // 이미 정리되었으면 중복 실행 방지
    }

    UE_LOG(LogTemp, Log, TEXT("[TargetSelection] Finalizing selection and resetting state."));

    // 타이머 정리
    GetWorld()->GetTimerManager().ClearTimer(RotationTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(CameraMovementTimerHandle);
    RotationState = ERotationState::Idle;

    // 캐릭터 회전 설정 복원
    if (OwnerCharacter.IsValid())
    {
        OwnerCharacter->bUseControllerRotationYaw = bOriginalUseControllerRotationYaw;
        // 원래 회전값으로 즉시 되돌림 (중요)
        OwnerCharacter->SetActorRotation(OriginalPlayerRotation);
    }

    // 원래 카메라로 복원
    RestoreOriginalCamera();

    // 블렌드가 끝난 후 카메라 액터를 파괴하도록 지연 호출
    FTimerHandle CleanupHandle;
    GetWorld()->GetTimerManager().SetTimer(
        CleanupHandle,
        this,
        &UTargetSelectionComponent::CleanupTemporaryCamera,
        0.9f, // RestoreOriginalCamera의 블렌드 시간(0.8f)보다 약간 길게
        false
    );

    // 상태 변수 초기화
    SelectionState = ETargetSelectionState::None;
    CurrentSkillType = ESkillTargetType::Universal;
    ValidTargets.Empty();
    CurrentTarget.Reset();
    CurrentTargetIndex = 0;
}

void UTargetSelectionComponent::ConfirmTarget()
{
    if (SelectionState != ETargetSelectionState::Selecting || !IsTargetValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TargetSelection] Cannot confirm - invalid state or target"));
        return;
    }

    SelectionState = ETargetSelectionState::Confirmed;

    UE_LOG(LogTemp, Log, TEXT("[TargetSelection] Target confirmed: %s"),
        CurrentTarget.IsValid() ? *CurrentTarget->GetName() : TEXT("None"));

    // 이벤트만 브로드캐스트하고, 상태 정리는 호출자(TurnPlayer)에게 맡김
    OnTargetSelected.Broadcast(CurrentTarget.Get(), CurrentSkillType);
}

void UTargetSelectionComponent::SelectNextTarget()
{
    if (SelectionState != ETargetSelectionState::Selecting || ValidTargets.Num() == 0)
    {
        return;
    }

    ChangeTargetIndex(1);
}

void UTargetSelectionComponent::SelectPreviousTarget()
{
    if (SelectionState != ETargetSelectionState::Selecting || ValidTargets.Num() == 0)
    {
        return;
    }

    ChangeTargetIndex(-1);
}

bool UTargetSelectionComponent::IsTargetValid() const
{
    if (!CurrentTarget.IsValid() || !GameInstance.IsValid())
    {
        return false;
    }

    ECombatTeam TargetTeam = GameInstance->GetTeam(CurrentTarget.Get());
    ECombatTeam PlayerTeam = ECombatTeam::Player; // 플레이어는 항상 Player 팀

    switch (CurrentSkillType)
    {
    case ESkillTargetType::Attack:
        // 공격 스킬은 적에게만
        return TargetTeam == ECombatTeam::Enemy;

    case ESkillTargetType::Heal:
    case ESkillTargetType::Buff:
        // 힐/버프는 아군에게만 (본인 포함)
        return TargetTeam == ECombatTeam::Player;

    case ESkillTargetType::Universal:
        // 범용은 모든 대상
        return true;

    default:
        return false;
    }
}

void UTargetSelectionComponent::UpdateTargetList()
{
    ValidTargets.Empty();

    if (!GameInstance.IsValid())
    {
        return;
    }

    // 월드의 모든 COECharacter 검색
    for (TActorIterator<ACOECharacter> It(GetWorld()); It; ++It)
    {
        ACOECharacter* Character = *It;
        if (!IsValid(Character) || !GameInstance->IsAlive(Character))
        {
            continue;
        }

        // 스킬 타입에 따른 필터링
        ECombatTeam CharacterTeam = GameInstance->GetTeam(Character);

        bool bValidTarget = false;
        switch (CurrentSkillType)
        {
        case ESkillTargetType::Attack:
            bValidTarget = (CharacterTeam == ECombatTeam::Enemy);
            break;

        case ESkillTargetType::Heal:
        case ESkillTargetType::Buff:
            bValidTarget = (CharacterTeam == ECombatTeam::Player);
            break;

        case ESkillTargetType::Universal:
            bValidTarget = true;
            break;
        }

        if (bValidTarget)
        {
            ValidTargets.Add(Character);
        }
    }

    // 팀별 정렬 (Player 먼저, Enemy 나중에)
    ValidTargets.Sort([this](const TWeakObjectPtr<ACOECharacter>& A, const TWeakObjectPtr<ACOECharacter>& B)
        {
            if (!A.IsValid() || !B.IsValid() || !GameInstance.IsValid())
            {
                return false;
            }

            ECombatTeam TeamA = GameInstance->GetTeam(A.Get());
            ECombatTeam TeamB = GameInstance->GetTeam(B.Get());

            if (TeamA != TeamB)
            {
                return TeamA == ECombatTeam::Player; // Player 팀이 먼저 오도록
            }

            return A->GetName() < B->GetName(); // 같은 팀 내에서는 이름순
        });

    UE_LOG(LogTemp, Log, TEXT("[TargetSelection] Found %d valid targets for skill type %d"),
        ValidTargets.Num(), (int32)CurrentSkillType);
}

void UTargetSelectionComponent::ChangeTargetIndex(int32 Delta)
{
    if (ValidTargets.Num() == 0)
    {
        return;
    }

    // 인덱스 순환
    CurrentTargetIndex = (CurrentTargetIndex + Delta + ValidTargets.Num()) % ValidTargets.Num();
    CurrentTarget = ValidTargets[CurrentTargetIndex];

    if (CurrentTarget.IsValid())
    {
        SetupCameraForTarget(CurrentTarget.Get());
        UE_LOG(LogTemp, Log, TEXT("[TargetSelection] Target changed to: %s (%d/%d)"),
            *CurrentTarget->GetName(), CurrentTargetIndex + 1, ValidTargets.Num());
    }
}

void UTargetSelectionComponent::SetupCameraForTarget(ACOECharacter* Target)
{
    if (!IsValid(Target) || !GameInstance.IsValid() || !PlayerController.IsValid())
    {
        return;
    }

    ECombatTeam TargetTeam = GameInstance->GetTeam(Target);

    if (TargetTeam == ECombatTeam::Enemy)
    {
        // 적 타겟팅: 플레이어가 적을 바라보도록 부드럽게 회전 시작
        RotatePlayerToTarget(Target);

        // 플레이어 카메라 사용
        if (OwnerCharacter.IsValid() && OriginalViewTarget.Get() == OwnerCharacter.Get())
        {
            PlayerController->SetViewTargetWithBlend(
                OwnerCharacter.Get(),
                0.5f, // 블렌드 시간 단축
                EViewTargetBlendFunction::VTBlend_EaseInOut,
                1.0f);
        }
    }
    else if (TargetTeam == ECombatTeam::Player)
    {
        // 진행중인 회전 타이머 중지
        GetWorld()->GetTimerManager().ClearTimer(RotationTimerHandle);
        RotationState = ERotationState::Idle;

        // 캐릭터 회전 설정 복원
        if (OwnerCharacter.IsValid())
        {
            OwnerCharacter->bUseControllerRotationYaw = bOriginalUseControllerRotationYaw;
        }

        // 아군 타겟팅: 각 아군 캐릭터 정면에서 바라보는 카메라로 직접 전환
        SetupPlayerFrontCamera(Target);
    }
}

void UTargetSelectionComponent::RotatePlayerToTarget(ACOECharacter* Target)
{
    if (!IsValid(Target) || !OwnerCharacter.IsValid())
    {
        return;
    }

    FVector PlayerLocation = OwnerCharacter->GetActorLocation();
    FVector TargetLocation = Target->GetActorLocation();
    FVector Direction = (TargetLocation - PlayerLocation).GetSafeNormal();

    TargetRotation = Direction.Rotation();
    TargetRotation.Pitch = 0.0f;
    TargetRotation.Roll = 0.0f;

    RotationState = ERotationState::RotatingToTarget;

    if (!GetWorld()->GetTimerManager().IsTimerActive(RotationTimerHandle))
    {
        if (OwnerCharacter.IsValid())
        {
            bOriginalUseControllerRotationYaw = OwnerCharacter->bUseControllerRotationYaw;
            OwnerCharacter->bUseControllerRotationYaw = false;
        }
        UE_LOG(LogTemp, Log, TEXT("[TargetSelection] Starting smooth rotation timer."));
        GetWorld()->GetTimerManager().SetTimer(
            RotationTimerHandle, this, &UTargetSelectionComponent::UpdateRotation, 0.016f, true);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[TargetSelection] Updating target rotation for active timer."));
    }
}

void UTargetSelectionComponent::UpdateRotation()
{
    if (RotationState == ERotationState::Idle || !PlayerController.IsValid() || !OwnerCharacter.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(RotationTimerHandle);
        return;
    }

    ACharacter* OwnerAsChar = OwnerCharacter.Get();
    AController* Controller = PlayerController.Get();

    const FRotator CurrentControlRotation = Controller->GetControlRotation();
    const FRotator NewControlRotation = FMath::RInterpTo(CurrentControlRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), RotationInterpSpeed);
    Controller->SetControlRotation(NewControlRotation);

    const FRotator CurrentActorRotation = OwnerAsChar->GetActorRotation();
    const FRotator NewActorRotation = FMath::RInterpTo(CurrentActorRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), RotationInterpSpeed);
    OwnerAsChar->SetActorRotation(NewActorRotation);

    if (CurrentControlRotation.Equals(TargetRotation, 1.0f))
    {
        Controller->SetControlRotation(TargetRotation);
        OwnerAsChar->SetActorRotation(TargetRotation);
        GetWorld()->GetTimerManager().ClearTimer(RotationTimerHandle);

        if (OwnerCharacter.IsValid())
        {
            OwnerAsChar->bUseControllerRotationYaw = bOriginalUseControllerRotationYaw;
        }

        UE_LOG(LogTemp, Log, TEXT("[TargetSelection] Smooth rotation finished. State was: %d"), (int32)RotationState);

        if (RotationState == ERotationState::RotatingToOriginal)
        {
            CurrentSkillType = ESkillTargetType::Universal;
            ValidTargets.Empty();
            CurrentTarget.Reset();
            CurrentTargetIndex = 0;

            RestoreOriginalCamera();
            CleanupTemporaryCamera();

            OnTargetSelectionCancelled.Broadcast();
        }

        RotationState = ERotationState::Idle;
    }
}

void UTargetSelectionComponent::SetupPlayerFrontCamera(ACOECharacter* PlayerTarget)
{
    if (!IsValid(PlayerTarget) || !PlayerController.IsValid())
    {
        return;
    }

    // 1. 목표 카메라 위치/회전 계산
    const FVector PlayerLocation = PlayerTarget->GetActorLocation();
    const FVector PlayerForward = PlayerTarget->GetActorForwardVector();
    TargetCameraLocation = PlayerLocation + (PlayerForward * 200.0f) + FVector(0, 0, 50.f);
    TargetCameraRotation = (PlayerLocation - TargetCameraLocation).GetSafeNormal().Rotation();

    // 2. 임시 카메라 생성 및 초기 설정
    if (!IsValid(TempCameraActor))
    {
        CreateTemporaryCameraActor();
        if (!IsValid(TempCameraActor)) return;

        // 생성된 임시 카메라를 현재 플레이어의 카메라 위치로 옮겨서 시작
        FVector CurrentViewLocation;
        FRotator CurrentViewRotation;
        PlayerController->GetPlayerViewPoint(CurrentViewLocation, CurrentViewRotation);
        TempCameraActor->SetActorLocationAndRotation(CurrentViewLocation, CurrentViewRotation);

        // 현재 뷰에서 임시 카메라로 시점 전환
        PlayerController->SetViewTargetWithBlend(TempCameraActor, 0.8f, EViewTargetBlendFunction::VTBlend_Cubic);
    }

    // 3. 부드러운 이동을 위한 타이머 시작
    GetWorld()->GetTimerManager().SetTimer(
        CameraMovementTimerHandle,
        this,
        &UTargetSelectionComponent::UpdatePlayerCameraMovement,
        0.016f, // ~60fps
        true
    );
}

void UTargetSelectionComponent::CreateTemporaryCameraActor()
{
    // 기존 임시 카메라 정리
    CleanupTemporaryCamera();

    // 임시 카메라 액터 생성
    TempCameraActor = GetWorld()->SpawnActor<AActor>();
    if (!TempCameraActor)
    {
        UE_LOG(LogTemp, Error, TEXT("[TargetSelection] Failed to create temporary camera actor"));
        return;
    }

    // 카메라 컴포넌트 추가
    TempCamera = NewObject<UCameraComponent>(TempCameraActor);
    TempCameraActor->SetRootComponent(TempCamera);

    // 카메라 설정 (FOV, 클리핑 등 기본값)
    if (TempCamera)
    {
        TempCamera->SetFieldOfView(90.0f);
    }

    UE_LOG(LogTemp, Log, TEXT("[TargetSelection] Created temporary camera actor"));
}

void UTargetSelectionComponent::CleanupTemporaryCamera()
{
    if (IsValid(TempCameraActor))
    {
        TempCameraActor->Destroy();
        TempCameraActor = nullptr;
    }
    TempCamera = nullptr;
}

void UTargetSelectionComponent::RestoreOriginalCamera()
{
    if (PlayerController.IsValid() && OriginalViewTarget.IsValid())
    {
        PlayerController->SetViewTargetWithBlend(OriginalViewTarget.Get(), 0.8f, EViewTargetBlendFunction::VTBlend_Cubic);
    }
}

void UTargetSelectionComponent::UpdatePlayerCameraMovement()
{
    if (!IsValid(TempCameraActor))
    {
        GetWorld()->GetTimerManager().ClearTimer(CameraMovementTimerHandle);
        return;
    }

    const float DeltaTime = GetWorld()->GetTimerManager().GetTimerRate(CameraMovementTimerHandle);

    // 현재 카메라 위치에서 목표 위치로 부드럽게 보간
    const FVector NewLocation = FMath::VInterpTo(TempCameraActor->GetActorLocation(), TargetCameraLocation, DeltaTime, CameraInterpSpeed);
    const FRotator NewRotation = FMath::RInterpTo(TempCameraActor->GetActorRotation(), TargetCameraRotation, DeltaTime, CameraInterpSpeed);

    TempCameraActor->SetActorLocationAndRotation(NewLocation, NewRotation);

    // 목표 위치에 도달하면 타이머 중지
    if (TempCameraActor->GetActorLocation().Equals(TargetCameraLocation, 1.0f))
    {
        GetWorld()->GetTimerManager().ClearTimer(CameraMovementTimerHandle);
    }
}
