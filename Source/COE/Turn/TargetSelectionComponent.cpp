// Fill out your copyright notice in the Description page of Project Settings.


#include "TargetSelectionComponent.h"
#include "BaseCode/COECharacter.h"
#include "BaseCode/COEGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/SpringArmComponent.h"

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

    UE_LOG(LogTemp, Log, TEXT("[TargetSelection] Target selection cancelled"));

    // 상태 초기화
    SelectionState = ETargetSelectionState::None;
    CurrentSkillType = ESkillTargetType::Universal;
    ValidTargets.Empty();
    CurrentTarget.Reset();
    CurrentTargetIndex = 0;

    // 카메라 및 회전 복원
    RestoreOriginalCamera();
    CleanupTemporaryCamera();

    // 원래 플레이어 회전 복원
    if (OwnerCharacter.IsValid())
    {
        OwnerCharacter->SetActorRotation(OriginalPlayerRotation);
    }

    // 이벤트 브로드캐스트
    OnTargetSelectionCancelled.Broadcast();
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

    // 이벤트 브로드캐스트
    OnTargetSelected.Broadcast(CurrentTarget.Get(), CurrentSkillType);

    // 선택 완료 후 정리
    RestoreOriginalCamera();
    CleanupTemporaryCamera();

    // 상태 초기화
    SelectionState = ETargetSelectionState::None;
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
        // 적 타겟팅: 플레이어가 적을 바라보도록 회전
        RotatePlayerToTarget(Target);

        // 플레이어 카메라 사용 (이미 적을 바라보고 있으므로)
        if (OwnerCharacter.IsValid())
        {
            PlayerController->SetViewTargetWithBlend(
                OwnerCharacter.Get(), 
                1.0f,
                EViewTargetBlendFunction::VTBlend_EaseInOut, 
                2.0f);
            
            // 혹시 모를 드리프트 방지: 컨트롤러=파운 회전 싱크
            PlayerController->SetControlRotation(OwnerCharacter->GetActorRotation());
        }
    }
    else if (TargetTeam == ECombatTeam::Player)
    {
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

    FRotator LookRotation = Direction.Rotation();
    LookRotation.Pitch = 0.0f; // 수평 회전만
    LookRotation.Roll = 0.0f;

    if (AController* C = OwnerCharacter->GetController())
    {
        C->SetControlRotation(LookRotation);
    }

    OwnerCharacter->SetActorRotation(LookRotation);
    UE_LOG(LogTemp, Log, TEXT("[TargetSelection] Player rotated to face %s"), *Target->GetName());
}

void UTargetSelectionComponent::SetupPlayerFrontCamera(ACOECharacter* PlayerTarget)
{
    if (!IsValid(PlayerTarget) || !PlayerController.IsValid())
    {
        return;
    }

    // 기존 임시 카메라가 있다면 위치만 업데이트, 없다면 새로 생성
    if (!IsValid(TempCameraActor))
    {
        CreateTemporaryCameraActor();
        if (!IsValid(TempCameraActor))
        {
            return;
        }
    }

    // 타겟 캐릭터 정면에서 바라보는 카메라 위치 계산
    FVector PlayerLocation = PlayerTarget->GetActorLocation();
    FVector PlayerForward = PlayerTarget->GetActorForwardVector();

    // 캐릭터 정면 200cm, 위쪽 50cm 위치에 카메라 배치
    FVector CameraLocation = PlayerLocation + (PlayerForward * 200.0f) + FVector(0, 0, 50);

    // 캐릭터를 바라보는 방향 계산
    FVector CameraDirection = (PlayerLocation - CameraLocation).GetSafeNormal();
    FRotator CameraRotation = CameraDirection.Rotation();

    // 카메라 위치 및 회전 설정
    TempCameraActor->SetActorLocation(CameraLocation);
    TempCameraActor->SetActorRotation(CameraRotation);

    // 카메라로 부드럽게 전환 (블렌드 시간을 조금 더 길게)
    PlayerController->SetViewTargetWithBlend(TempCameraActor, 0.8f, EViewTargetBlendFunction::VTBlend_Cubic);

    UE_LOG(LogTemp, Log, TEXT("[TargetSelection] Setup front camera for player: %s"), *PlayerTarget->GetName());
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
        PlayerController->SetViewTargetWithBlend(OriginalViewTarget.Get(), 0.5f);
    }
}