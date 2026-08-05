// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeStateTreeEvaluator_CombatContext.h"

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"
#include "GAS_Project/AAI/LeeTargetSelectionComponent.h"
#include "GAS_Project/AAI/Token/LeeAttackTokenComponent.h"
#include "GAS_Project/AAI/Token/LeeAttackerProfileComponent.h"
#include "GAS_Project/ACharacter/LeeTargetLockTargetComponent.h"

namespace
{
	// 락온/피니셔와 동일한 기준점 규칙: ULeeTargetLockTargetComponent 우선, 없으면 액터 위치
	FVector GetFocusLocation(const AActor* Actor)
	{
		if (!Actor)
		{
			return FVector::ZeroVector;
		}

		if (const ULeeTargetLockTargetComponent* FocusComp = ULeeTargetLockTargetComponent::FindTargetLockTargetComponent(Actor))
		{
			return FocusComp->GetFocusLocation();
		}

		return Actor->GetActorLocation();
	}

	// ACharacter면 스케일 적용된 캡슐 반경, 아니면 0
	float GetCapsuleRadius(const AActor* Actor)
	{
		const ACharacter* Character = Cast<ACharacter>(Actor);
		return (Character && Character->GetCapsuleComponent()) ? Character->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.f;
	}
}

// 매 프레임: 자신 스탯 비율 → 타겟 조회(ULeeTargetSelectionComponent가 유일한 권위) → 거리/각도/티어 계산
void FLeeStateTreeEvaluator_CombatContext::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	{
		static double LastEntryLogTime = -1.0;
		const double Now = FPlatformTime::Seconds();
		if (Now - LastEntryLogTime > 1.0)
		{
			LastEntryLogTime = Now;
		}
	}

	if (!Data.Actor)
	{
		return;
	}

	// 자신 스태미나/체력 비율 (ULeeGameplayAbility_AttackMelee/_Guard와 동일하게 GetNumericAttribute 사용)
	if (UAbilitySystemComponent* SelfASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor))
	{
		const float MaxStamina = SelfASC->GetNumericAttribute(ULeeSoulsStatSet::GetMaxStaminaAttribute());
		const float Stamina = SelfASC->GetNumericAttribute(ULeeSoulsStatSet::GetStaminaAttribute());
		Data.SelfStaminaRatio = MaxStamina > 0.f ? (Stamina / MaxStamina) : 1.f;

		const float MaxHealth = SelfASC->GetNumericAttribute(ULeeSoulsStatSet::GetMaxHealthAttribute());
		const float Health = SelfASC->GetNumericAttribute(ULeeSoulsStatSet::GetHealthAttribute());
		Data.SelfHealthRatio = MaxHealth > 0.f ? (Health / MaxHealth) : 1.f;
	}

	// 자신의 공격 프로필 — Linked Asset 파라미터가 프로퍼티 경로(예: AttackerProfileComponent.LightAttack)로 바로 참조할 수 있도록 출력
	Data.AttackerProfileComponent = ULeeAttackerProfileComponent::FindAttackerProfileComponent(Data.Actor);

	// 타겟 조회 — 판단은 하지 않는다, ULeeTargetSelectionComponent의 결론을 그대로 옮긴다
	const ULeeTargetSelectionComponent* SelectionComp = Data.AIController
		? ULeeTargetSelectionComponent::FindTargetSelectionComponent(Data.AIController)
		: nullptr;
	Data.TargetActor = SelectionComp ? SelectionComp->GetCurrentTarget() : nullptr;

	{
		static double LastDebugLogTime = -1.0;
		const double Now = Data.Actor->GetWorld() ? Data.Actor->GetWorld()->GetTimeSeconds() : 0.0;
		if (Now - LastDebugLogTime > 1.0)
		{
			LastDebugLogTime = Now;
			
		}
	}

	if (!Data.TargetActor)
	{
		Data.AttackTokenComponent = nullptr;
		Data.CachedTargetForTokenLookup = nullptr;
		Data.DistanceToTarget = 0.f;
		Data.AngleToTarget = 0.f;
		Data.bIsTargetInFront = false;
		Data.bIsCloseTier = false;
		Data.bIsMidTier = false;
		Data.bIsFarTier = false;
		Data.bIsAttackSuppressed = false;
		return;
	}

	// 타겟이 바뀐 프레임에만 AttackTokenComponent 재해석
	if (Data.CachedTargetForTokenLookup != Data.TargetActor)
	{
		Data.AttackTokenComponent = ULeeAttackTokenComponent::FindAttackTokenComponent(Data.TargetActor);
		Data.CachedTargetForTokenLookup = Data.TargetActor;
	}

	// 거리: 캡슐 반경을 뺀 표면-표면 거리, bUse2DDistance면 Z 무시
	FVector SelfLocation = GetFocusLocation(Data.Actor);
	FVector TargetLocation = GetFocusLocation(Data.TargetActor);
	if (Data.bUse2DDistance)
	{
		SelfLocation.Z = 0.f;
		TargetLocation.Z = 0.f;
	}

	const float RawDistance = FVector::Dist(SelfLocation, TargetLocation);
	const float SelfRadius = GetCapsuleRadius(Data.Actor);
	const float TargetRadius = GetCapsuleRadius(Data.TargetActor);
	Data.DistanceToTarget = FMath::Max(0.f, RawDistance - SelfRadius - TargetRadius);

	// 각도: 자신 정면 기준 수평(2D) 부호 있는 각도. 전방 반구(±90도) 판정
	const FVector SelfForward = Data.Actor->GetActorForwardVector().GetSafeNormal2D();
	const FVector ToTarget = (TargetLocation - SelfLocation).GetSafeNormal2D();
	const float SignedAngleRad = FMath::Atan2(
		FVector::CrossProduct(SelfForward, ToTarget).Z,
		FVector::DotProduct(SelfForward, ToTarget));
	Data.AngleToTarget = FMath::RadiansToDegrees(SignedAngleRad);
	Data.bIsTargetInFront = FMath::Abs(Data.AngleToTarget) <= 90.f;

	// 히스테리시스 상태 머신 (P0-6) — CurrentTier: 0=Close, 1=Mid, 2=Far
	switch (Data.CurrentTier)
	{
	case 1: // Mid
		if (Data.DistanceToTarget <= Data.CloseEnterDistance)
		{
			Data.CurrentTier = 0;
		}
		else if (Data.DistanceToTarget >= Data.FarEnterDistance)
		{
			Data.CurrentTier = 2;
		}
		break;
	case 0: // Close
		if (Data.DistanceToTarget >= Data.CloseExitDistance)
		{
			Data.CurrentTier = 1;
		}
		break;
	case 2: // Far
		if (Data.DistanceToTarget <= Data.FarExitDistance)
		{
			Data.CurrentTier = 1;
		}
		break;
	default:
		Data.CurrentTier = 1;
		break;
	}

	Data.bIsCloseTier = (Data.CurrentTier == 0);
	Data.bIsMidTier = (Data.CurrentTier == 1);
	Data.bIsFarTier = (Data.CurrentTier == 2);

	// 디버그/애니용 참고 값 — 현재 어택 토큰을 하나도 Claim할 수 없는 상태인지 (StateTree 판정에는 쓰지 않는다)
	Data.bIsAttackSuppressed = false;
	if (Data.AttackTokenComponent && Data.AttackerProfileComponent)
	{
		TArray<FLeeAttackReservationConfig> Candidates;
		Candidates.Add(Data.AttackerProfileComponent->LightAttack);
		Candidates.Add(Data.AttackerProfileComponent->HeavyAttack);
		if (Data.AttackerProfileComponent->bCanRangedAttack)
		{
			Candidates.Add(Data.AttackerProfileComponent->RangedAttack);
		}
		Data.bIsAttackSuppressed = !Data.AttackTokenComponent->CanClaimAny(Data.Actor, Candidates);
	}
}
