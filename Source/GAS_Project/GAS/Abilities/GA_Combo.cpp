// GA_Combo.cpp
#include "GA_Combo.h"

#include "GameplayTagsManager.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"

UGA_Combo::UGA_Combo()
{
	// 5.6 경고 대응: AbilityTags -> AssetTags
	FGameplayTagContainer Tags;
	Tags.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());
	SetAssetTags(Tags);

	BlockAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());
}

void UGA_Combo::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Combo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 리소스(쿨다운/비용) 커밋
	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	// 몽타주 필수 체크
	if (!ComboMontage)
	{
		K2_EndAbility();
		return;
	}

	// 몽타주 시작 및 종료시 EndAbility
	{
		UAbilityTask_PlayMontageAndWait* PlayMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, ComboMontage);
		PlayMontage->OnBlendOut.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		PlayMontage->OnCancelled.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		PlayMontage->OnCompleted.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		PlayMontage->OnInterrupted.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		PlayMontage->ReadyForActivation();
	}

	// [중요] 입력은 이벤트로 받습니다(Pressed). WaitInputPress 사용하지 않음.
	{
		UAbilityTask_WaitGameplayEvent* WaitPressed = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, /*Input Pressed*/ MyTags::Abilities::BasicAttackPressed, nullptr, false, false);
		WaitPressed->EventReceived.AddDynamic(this, &UGA_Combo::OnInputPressedEvent);
		WaitPressed->ReadyForActivation();
	}

	// 콤보 윈도우: 섹션명 세팅(애님 노티파이에서 씁니다)
	{
		UAbilityTask_WaitGameplayEvent* WaitComboChange = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, GetComboChangeEventTag(), nullptr, false, false);
		WaitComboChange->EventReceived.AddDynamic(this, &UGA_Combo::ComboChangedEventRecevied);
		WaitComboChange->ReadyForActivation();

		if (K2_HasAuthority())
		{
			UAbilityTask_WaitGameplayEvent* WaitTargetEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, GetComboTargetEventTag());
			WaitTargetEvent->EventReceived.AddDynamic(this, &UGA_Combo::DoDamage);
			WaitTargetEvent->ReadyForActivation();
		}
	}
}

FGameplayTag UGA_Combo::GetComboChangeEventTag()
{
	return MyTags::Abilities::ComboChange;
}

FGameplayTag UGA_Combo::GetComboChangeEventEndTag()
{
	return MyTags::Abilities::ComboChangeEnd;
}

FGameplayTag UGA_Combo::GetComboTargetEventTag()
{
	return MyTags::Abilities::ComboDamage;
}

void UGA_Combo::OnInputPressedEvent(FGameplayEventData Data)
{
	// 입력은 매번 다시 대기(연타 허용)
	UAbilityTask_WaitGameplayEvent* WaitPressed = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, MyTags::Abilities::BasicAttackPressed, nullptr, false, false);
	WaitPressed->EventReceived.AddDynamic(this, &UGA_Combo::OnInputPressedEvent);
	WaitPressed->ReadyForActivation();

	TryCommitCombo();
}

void UGA_Combo::TryCommitCombo()
{
	if (NextComboName == NAME_None)
	{
		return; // 콤보 윈도우가 열리지 않았거나 섹션명이 설정되지 않음
	}

	UAnimInstance* Anim = GetOwnerAnimInstance();
	if (!Anim) return;

	// 현재 섹션 종료 시 다음 섹션으로 넘어가도록 설정
	const FName Current = Anim->Montage_GetCurrentSection(ComboMontage);
	if (Current != NAME_None)
	{
		Anim->Montage_SetNextSection(Current, NextComboName, ComboMontage);
	}
}

void UGA_Combo::ComboChangedEventRecevied(FGameplayEventData Data)
{
	const FGameplayTag EventTag = Data.EventTag;

	// 윈도우 종료
	if (EventTag == GetComboChangeEventEndTag())
	{
		NextComboName = NAME_None;
		return;
	}

	// 태그의 마지막 토큰을 섹션명으로 사용
	// 예) MyTags.Abilities.ComboChange.M2 -> "M2"
	TArray<FName> Tokens;
	UGameplayTagsManager::Get().SplitGameplayTagFName(EventTag, Tokens);
	NextComboName = Tokens.Num() > 0 ? Tokens.Last() : NAME_None;
}

void UGA_Combo::DoDamage(FGameplayEventData Data)
{
	// 필요 시 서버에서 대미지 처리
	return;
}
