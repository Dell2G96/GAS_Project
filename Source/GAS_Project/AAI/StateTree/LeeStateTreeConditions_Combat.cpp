// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeStateTreeConditions_Combat.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/AAI/Token/LeeAttackerProfileComponent.h"
#include "GAS_Project/LeeLogChannels.h"

// 지정된 한 종류의 공격을 지금 Claim할 수 있는지 예약 없이 확인 (TokenComponent 없음 = 제한 없이 허용)
bool FLeeStateTreeCondition_CanClaimAttackToken::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.TokenComponent)
	{
		return true;
	}

	return Data.TokenComponent->CanClaim(Data.RequesterPawn, Data.Reservation.QuotaTag, Data.Reservation.Cost);
}

// 지정된 공격을 마지막으로 쓴 지 MinSeconds 이상 지났는지 확인 (기록이 없으면 통과)
bool FLeeStateTreeCondition_TimeSinceLastUse::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	ULeeAttackerProfileComponent* Profile = ULeeAttackerProfileComponent::FindAttackerProfileComponent(Data.RequesterPawn);
	if (!Profile)
	{
		return true;
	}

	return Profile->GetTimeSinceLastUse(Data.AttackTag) >= Data.MinSeconds;
}

// 근접 공격(Light/Heavy) 중 하나라도 지금 Claim 가능한지 확인
bool FLeeStateTreeCondition_CanClaimAnyMeleeAttack::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.ProfileComponent)
	{
		
		return false;
	}

	if (!Data.TokenComponent)
	{
	 
		return true;
	}

	TArray<FLeeAttackReservationConfig> Candidates;
	Candidates.Add(Data.ProfileComponent->LightAttack);
	Candidates.Add(Data.ProfileComponent->HeavyAttack);

	const bool bResult = Data.TokenComponent->CanClaimAny(Data.RequesterPawn, Candidates);

	 

	return bResult;
}

// 타겟 ASC가 TargetTag를 보유하고 있는지 확인
bool FLeeStateTreeCondition_TargetHasGameplayTag::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.TargetActor || !Data.TargetTag.IsValid())
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.TargetActor);
	return ASC && ASC->HasMatchingGameplayTag(Data.TargetTag);
}

// 자기 자신 ASC가 Tag를 보유하는지 확인 (bInvert=true면 "없을 때" 통과)
bool FLeeStateTreeCondition_SelfHasGameplayTag::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	// 태그를 지정하지 않았으면 검사할 게 없으므로 항상 통과시킨다 (조건이 State를 막아버리지 않게)
	if (!Data.Tag.IsValid())
	{
		return true;
	}

	const UAbilitySystemComponent* ASC = Data.Actor ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor) : nullptr;
	const bool bHasTag = ASC && ASC->HasMatchingGameplayTag(Data.Tag);
	return Data.bInvert ? !bHasTag : bHasTag;
}
