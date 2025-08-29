// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "COEAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class COE_API UCOEAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	// 기본공격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimMontage")
	TObjectPtr<class UAnimMontage> DefaultAttackMontage;

	// Heal(W)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimMontage")
	TObjectPtr<class UAnimMontage> HealMontage;

	// SKill(E)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimMontage")
	TObjectPtr<class UAnimMontage> SkillMontage;

	// 패링
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimMontage")
	TObjectPtr<class UAnimMontage> ParryMontage;

	// 회피
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimMontage")
	TObjectPtr<class UAnimMontage> DodgeMontage;

	// 피격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimMontage")
	TObjectPtr<class UAnimMontage> HitMontage;

public:
	//움직임 체크 (Idle 판단)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	bool ShouldMove;
	//추락 체크 (점프 또는 추락 판단)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	bool IsFalling;

	//조준 체크
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	bool IsAiming;

	//속도 벡터
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	FVector Velocity;
	//수평입력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	float Horizontal;
	//수직입력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	float Vertical;
	//XY 벡터크기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	float GroundSpeed;
	//가속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	FVector Acceleration;

	//조준 방향과 이동방향 좌우 각도 차이
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	float YawOffset;
	//캐릭터 조준 중인 회전값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ani", meta = (AllowPrivateAccess = "true"))
	FRotator AimRotation;
public:
	//캐릭터 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<class ACOECharacter> Character;
	//캐릭터 움직임 컴포넌트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<class UCharacterMovementComponent> CharacterMovement;
public:
	UCOEAnimInstance();
	virtual void NativeInitializeAnimation() override;
	//CharacterMovement에 연결된 폰의 움직임 받아옴
	virtual void NativeBeginPlay() override;
	//실시간으로 CharacterMovement관련값 업데이트
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	// 기본공격 
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void DefaultAttackAnim();

	// Heal 
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void HealAnim();

	// Skill
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void SkillAnim();

	// 패리
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void ParryAnim();

	// 회피
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void DodgeAnim();

	// 피격
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void HitAnim();

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void AnimNotify_End();
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void AnimNotify_DoDefaultAttack();

	/** 패링 무적 시작 (Parry_Start에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Animation|Defense")
	void AnimNotify_Parry_Start();

	/** 패링 무적 종료 (Parry_End에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Animation|Defense")
	void AnimNotify_Parry_End();

	/** 패링 애니메이션 완전 종료 (ParryAnim_End에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Animation|Defense")
	void AnimNotify_ParryAnim_End();

	/** 회피 무적 시작 (Dodge_Start에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Animation|Defense")
	void AnimNotify_Dodge_Start();

	/** 회피 무적 종료 (Dodge_End에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Animation|Defense")
	void AnimNotify_Dodge_End();

	/** 회피 애니메이션 완전 종료 (DodgeAnim_End에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Animation|Defense")
	void AnimNotify_DodgeAnim_End();

	/** HealEmitter 스폰 시점 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Defense")
	void AnimNotify_WSkill_Start();

	/** HealAnim 종료 시점 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Defense")
	void AnimNotify_WSkill_End();

	/** SkillEmitter 스폰 시점 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Defense")
	void AnimNotify_ESkill_Start();

	/** SkillAnim 종료 시점 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Defense")
	void AnimNotify_ESkill_End();

	/** 보스 SkillAnim 시작 시점 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Defense")
	void AnimNotify_Fire();

	/** 보스 SkillAnim FX 스폰 시점 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Defense")
	void AnimNotify_Fire_Spawn();

	/** 보스 SkillAnim 종료 시점 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Defense")
	void AnimNotify_Fire_End();

};
