// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeAbilityEndListener.h"

#include "AbilitySystemComponent.h"

// 감시를 시작한다. 이미 구독 중이면 먼저 해제 후 새 SpecHandle로 다시 구독한다
void ULeeAbilityEndListener::Bind(UAbilitySystemComponent* InASC, FGameplayAbilitySpecHandle InSpecHandle)
{
	Unbind();

	if (!InASC || !InSpecHandle.IsValid())
	{
		return;
	}

	WatchedASC = InASC;
	WatchedSpecHandle = InSpecHandle;
	bEnded = false;

	DelegateHandle = InASC->OnAbilityEnded.AddUObject(this, &ULeeAbilityEndListener::HandleAbilityEnded);
}

// 구독을 해제한다
void ULeeAbilityEndListener::Unbind()
{
	if (UAbilitySystemComponent* ASC = WatchedASC.Get())
	{
		ASC->OnAbilityEnded.Remove(DelegateHandle);
	}

	DelegateHandle.Reset();
	WatchedASC = nullptr;
	WatchedSpecHandle = FGameplayAbilitySpecHandle();
}

// 감시 중인 SpecHandle과 정확히 일치하는 종료만 기록 (다른 어빌리티의 종료와 혼동 방지)
void ULeeAbilityEndListener::HandleAbilityEnded(const FAbilityEndedData& EndedData)
{
	if (EndedData.AbilitySpecHandle == WatchedSpecHandle)
	{
		bEnded = true;
	}
}
