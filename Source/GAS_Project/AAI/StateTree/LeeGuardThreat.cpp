// 26.11.08 신규 - Enemy 공용 반응 가드: 플레이어 근접 공격의 타격 시점 감지

#include "LeeGuardThreat.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GAS_Project/AAbilitySystem/Abilities/LeeGameplayAbility_AttackMelee.h"
#include "GAS_Project/Notifies/ANS_WeaponTrace.h"

// 기억을 모두 지운다
void FLeeGuardThreatState::Reset()
{
	Montage = nullptr;
	LastPosition = 0.f;
	RolledNotifyIndex = INDEX_NONE;
	bGuardRolled = false;
	bPerfectRolled = false;
	bRefreshIssued = false;
}

namespace
{
	// 타깃이 근접 공격 어빌리티로 재생 중인 몽타주와 AnimInstance를 찾는다 (없으면 false)
	bool FindAttackMontage(const AActor* Target, UAnimMontage*& OutMontage, UAnimInstance*& OutAnimInstance)
	{
		const ACharacter* Character = Cast<ACharacter>(Target);
		if (!Character || !Character->IsPlayerControlled() || !Character->GetMesh())
		{
			return false;
		}

		const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Target));
		const UGameplayAbility* Animating = ASC ? ASC->GetAnimatingAbility() : nullptr;
		if (!Animating || !Animating->IsA<ULeeGameplayAbility_AttackMelee>())
		{
			return false;
		}

		OutMontage = ASC->GetCurrentMontage();
		OutAnimInstance = Character->GetMesh()->GetAnimInstance();
		return OutMontage && OutAnimInstance && OutAnimInstance->Montage_IsPlaying(OutMontage);
	}
}

// 판정 구간 탐색 → 구간마다 확률 판정 → 지금 가드/퍼펙트 갱신 여부 계산
FLeeGuardThreatResult LeeGuardThreat::Update(FLeeGuardThreatState& State, const AActor* Target,
	float GuardChance, float PerfectGuardChance, float PerfectGuardLeadTime)
{
	FLeeGuardThreatResult Result;

	UAnimMontage* Montage = nullptr;
	UAnimInstance* AnimInstance = nullptr;
	if (!FindAttackMontage(Target, Montage, AnimInstance))
	{
		State.Reset();
		return Result;
	}

	const float Position = AnimInstance->Montage_GetPosition(Montage);

	// 다른 몽타주로 바뀌었거나 같은 몽타주를 처음부터 다시 재생하면 새 공격으로 본다
	if (State.Montage.Get() != Montage || Position < State.LastPosition)
	{
		State.Reset();
		State.Montage = Montage;
	}
	State.LastPosition = Position;

	// 현재 구간: 진행 중인 판정 구간, 없으면 가장 가까운 다음 판정 구간
	int32 CurrentIndex = INDEX_NONE;
	float CurrentStart = 0.f;
	for (int32 Index = 0; Index < Montage->Notifies.Num(); ++Index)
	{
		const FAnimNotifyEvent& Notify = Montage->Notifies[Index];
		if (!Notify.NotifyStateClass || !Notify.NotifyStateClass->IsA<UANS_WeaponTrace>())
		{
			continue;
		}

		const float Start = Notify.GetTriggerTime();
		const float End = Start + Notify.GetDuration();
		if (Position >= Start && Position <= End)
		{
			CurrentIndex = Index;
			CurrentStart = Position; // 이미 판정 중 → 남은 시간 0
			break;
		}
		if (Start > Position && (CurrentIndex == INDEX_NONE || Start < CurrentStart))
		{
			CurrentIndex = Index;
			CurrentStart = Start;
		}
	}

	if (CurrentIndex == INDEX_NONE)
	{
		return Result;
	}

	const float PlayRate = FMath::Max(KINDA_SMALL_NUMBER, AnimInstance->Montage_GetPlayRate(Montage) * Montage->RateScale);
	const float TimeToHit = (CurrentStart - Position) / PlayRate;

	// 새 구간을 처음 봤을 때만 확률을 굴린다
	if (State.RolledNotifyIndex != CurrentIndex)
	{
		State.RolledNotifyIndex = CurrentIndex;
		State.bGuardRolled = FMath::FRand() < GuardChance;
		State.bPerfectRolled = State.bGuardRolled && FMath::FRand() < PerfectGuardChance;
		State.bRefreshIssued = false;
	}

	const bool bInLeadTime = TimeToHit <= PerfectGuardLeadTime;

	Result.bThreatActive = true;
	// 일반 가드는 공격을 보자마자, 퍼펙트는 타격 직전에 올린다 (그래야 GA_Guard의 퍼펙트 판정 시간 안에 맞는다)
	Result.bShouldGuardNow = State.bGuardRolled && (!State.bPerfectRolled || bInLeadTime);
	Result.bShouldRefreshPerfect = State.bPerfectRolled && !State.bRefreshIssued && bInLeadTime;
	if (Result.bShouldRefreshPerfect)
	{
		State.bRefreshIssued = true;
	}
	return Result;
}
