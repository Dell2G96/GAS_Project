// /Source/…/GA_Combo.cpp
#include "GA_Combo.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayTagsManager.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"

UGA_Combo::UGA_Combo()
{
	// 이 어빌리티 자체 태깅(기본공격)
	{
		FGameplayTagContainer AssetTags;
		AssetTags.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());
		SetAssetTags(AssetTags);

		// 자기 자신 블럭(선택)
		BlockAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());
	}

	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 전투상태 필요 / 대기상태면 차단(선택)
	ActivationRequiredTags.AddTag(UCAbilitySystemStatics::GetBattleModeTag());
	ActivationBlockedTags.AddTag(UCAbilitySystemStatics::GetIdleModeTag());
}

void UGA_Combo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	// if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	// {
	// 	UAbilityTask_PlayMontageAndWait* PlayComboMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ComboMontage);
	// 	PlayComboMontageTask->OnBlendOut.AddDynamic(this, &UGA_Combo::K2_EndAbility);
	// 	PlayComboMontageTask->OnCancelled.AddDynamic(this, &UGA_Combo::K2_EndAbility);
	// 	PlayComboMontageTask->OnCompleted.AddDynamic(this, &UGA_Combo::K2_EndAbility);
	// 	PlayComboMontageTask->OnInterrupted.AddDynamic(this, &UGA_Combo::K2_EndAbility);
	// 	PlayComboMontageTask->ReadyForActivation();
	//
	// 	UAbilityTask_WaitGameplayEvent* WaitComboChangeEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetComboChangeEventTag(), nullptr, false, false);
	// 	WaitComboChangeEventTask->EventReceived.AddDynamic(this, &UGA_Combo::ComboChangedEventReceived);
	// 	WaitComboChangeEventTask->ReadyForActivation();
	// }


	// 초기화
	bComboWindowOpen    = false;
	CandidateNextSection= NAME_None;
	NextComboName       = NAME_None;

	// 몽타주 재생(서버 + 예측클라 모두)
	{
		UAbilityTask_PlayMontageAndWait* Play =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, ComboMontage
			);
		Play->OnCompleted.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		Play->OnBlendOut.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		Play->OnInterrupted.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		Play->OnCancelled.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		Play->ReadyForActivation();
	}

	// 창 오픈(ComboChange.*) : 하위태그 허용
	{
		UAbilityTask_WaitGameplayEvent* WaitOpen =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, GetComboChangeEventTag(), /*OptTarget*/nullptr,
				/*OnlyTriggerOnce*/ false,
				/*OnlyMatchExact*/  false   // ComboChange.M2 같은 하위태그 받음
			);
		WaitOpen->EventReceived.AddDynamic(this, &UGA_Combo::OnComboWindowOpened);
		WaitOpen->ReadyForActivation();
	}

	// 창 종료(ComboChangeEnd) : 정확히 일치
	{
		UAbilityTask_WaitGameplayEvent* WaitEnd =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, GetComboChangeEventEndTag(), /*OptTarget*/nullptr,
				/*OnlyTriggerOnce*/ false,
				/*OnlyMatchExact*/  false
			);
		WaitEnd->EventReceived.AddDynamic(this, &UGA_Combo::OnComboWindowEnded);
		WaitEnd->ReadyForActivation();
	}


	//NextComboName = NAME_None;
	SetupWaitComboInputPress();
}

void UGA_Combo::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


// 추가함수
void UGA_Combo::OnComboWindowOpened(FGameplayEventData Data)
{
	// 태그 마지막 토큰을 후보 섹션명으로 사용 (예: ComboChange.M2 -> "M2")
	TArray<FName> Tokens;
	UGameplayTagsManager::Get().SplitGameplayTagFName(Data.EventTag, Tokens);
	CandidateNextSection = Tokens.Num() > 0 ? Tokens.Last() : NAME_None;

	bComboWindowOpen = (CandidateNextSection != NAME_None);

	UE_LOG(LogTemp, Warning, TEXT("[Combo] Window OPEN, candidate=%s"),
		*CandidateNextSection.ToString());
}

void UGA_Combo::OnComboWindowEnded(FGameplayEventData Data)
{
	
	// 창 종료 타이밍: 입력이 승인되어 있으면 '즉시' 점프
	if (NextComboName != NAME_None)
	{
		if (UAnimInstance* Anim = GetOwnerAnimInstance())
		{
			if (ComboMontage && Anim->Montage_IsPlaying(ComboMontage))
			{
				UE_LOG(LogTemp, Log, TEXT("[Combo] Window END -> JUMP to %s"),
					*NextComboName.ToString());

				// 즉시 점프(서버도, 예측클라도 각자 실행)
				Anim->Montage_JumpToSection(NextComboName, ComboMontage);
			}
		}
	}

	// 창 종료 후 초기화
	bComboWindowOpen = false;
	CandidateNextSection = NAME_None;
	NextComboName = NAME_None;

}

void UGA_Combo::OnInputPressed(float TimeWaited)
{
	// 다음 입력도 계속 받을 수 있도록 재설치
	SetupWaitComboInputPress();

	// 창이 열려 있을 때만 입력 승인
	if (bComboWindowOpen && CandidateNextSection != NAME_None)
	{
		NextComboName = CandidateNextSection;
		UE_LOG(LogTemp, Log, TEXT("[Combo] INPUT accepted -> Next=%s"),
			*NextComboName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Combo] INPUT ignored (window closed)"));
	}

}

UAnimInstance* UGA_Combo::GetOwnerAnimInstance() const
{
	if (USkeletalMeshComponent* Skel = GetOwningComponentFromActorInfo())
	{
		return Skel->GetAnimInstance();
	}
	return nullptr;

}

FGameplayTag UGA_Combo::GetComboChangeEventTag()
{
	return MyTags::Abilities::ComboChange;

}

FGameplayTag UGA_Combo::GetComboChangeEventEndTag()
{
	return MyTags::Abilities::ComboChangeEnd;

}

void UGA_Combo::SetupWaitComboInputPress()
{
	// UAbilityTask_WaitInputPress* WaitInputPress = UAbilityTask_WaitInputPress::WaitInputPress(this);
	// WaitInputPress->OnPress.AddDynamic(this, &UGA_Combo::HandleInputPress);
	// WaitInputPress->ReadyForActivation();

	if (UAbilityTask_WaitInputPress* Wait = UAbilityTask_WaitInputPress::WaitInputPress(this))
	{
		Wait->OnPress.AddDynamic(this, &UGA_Combo::OnInputPressed);
		Wait->ReadyForActivation();
	}


	
}

void UGA_Combo::HandleInputPress(float TimeWaited)
{
	SetupWaitComboInputPress();
	TryCommitCombo();
}

void UGA_Combo::TryCommitCombo()
{
	if (NextComboName == NAME_None)
	{
		return;
	}

	UAnimInstance* OwnerAnimInst = GetOwnerAnimInstance();
	if (!OwnerAnimInst)
	{
		return;
	}

	OwnerAnimInst->Montage_SetNextSection(OwnerAnimInst->Montage_GetCurrentSection(ComboMontage), NextComboName, ComboMontage);
}

void UGA_Combo::ComboChangedEventReceived(FGameplayEventData Data)
{
	FGameplayTag EventTag = Data.EventTag;

	if (EventTag == GetComboChangeEventEndTag())
	{
		NextComboName = NAME_None;
		return;
	}
	
	TArray<FName> TagNames;
	UGameplayTagsManager::Get().SplitGameplayTagFName(EventTag, TagNames);

	NextComboName = TagNames.Last();
}