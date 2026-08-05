// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeAttackerProfileComponent.h"

// AttackTag의 사용 시각을 현재 월드 시각으로 갱신
void ULeeAttackerProfileComponent::RecordUse(FGameplayTag AttackTag)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !AttackTag.IsValid())
	{
		return;
	}

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	LastUseTime.Add(AttackTag, Now);
}

// AttackTag를 마지막으로 사용한 뒤 경과한 시간(초)을 반환. 미사용 시 매우 큰 값을 반환
double ULeeAttackerProfileComponent::GetTimeSinceLastUse(FGameplayTag AttackTag) const
{
	const double* Found = LastUseTime.Find(AttackTag);
	if (!Found)
	{
		return TNumericLimits<double>::Max();
	}

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	return Now - *Found;
}
