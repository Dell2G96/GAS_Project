#include "GA_Combo.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagsManager.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "Kismet/GameplayStatics.h"

UGA_Combo::UGA_Combo()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());
	SetAssetTags(Tags);
	BlockAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());
 
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
 
	// ✅ 중요! LocalPredicted로 설정
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    
	// ✅ 추가: 클라이언트도 로컬에서 예측 실행 허용
	bReplicateInputDirectly = false;  // 입력을 직접 복제하지 않음

	const FGameplayTag Tag_BattleMode = UCAbilitySystemStatics::GetBattleModeTag();
	const FGameplayTag Tag_IdleMode = UCAbilitySystemStatics::GetIdleModeTag();

	ActivationRequiredTags.AddTag(Tag_BattleMode);
	ActivationBlockedTags.AddTag(Tag_IdleMode);
}
void UGA_Combo::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                const FGameplayAbilityActorInfo* ActorInfo,
                                const FGameplayAbilityActivationInfo ActivationInfo,
                                const FGameplayEventData* TriggerEventData)
{
    const TCHAR* RoleStr = K2_HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
    
    UE_LOG(LogTemp, Warning, TEXT("[%s] ActivateAbility START"), RoleStr);
    
    if (!K2_CommitAbility())
    {
       UE_LOG(LogTemp, Warning, TEXT("[%s] CommitAbility FAILED!"), RoleStr);
       K2_EndAbility();
       return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[%s] CommitAbility SUCCESS"), RoleStr);

    if (!ComboMontage)
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] ComboMontage is NULL!"), RoleStr);
        K2_EndAbility();
        return;
    }

    bool bHasAuth = HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo);
    UE_LOG(LogTemp, Warning, TEXT("[%s] HasAuthorityOrPredictionKey: %d"), RoleStr, bHasAuth ? 1 : 0);

    if (bHasAuth)
    {
       UE_LOG(LogTemp, Warning, TEXT("[%s] Creating PlayMontageTask (Slot: Full)..."), RoleStr);
       
       UAbilityTask_PlayMontageAndWait* Play = 
           UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
               this, 
               NAME_None,
               ComboMontage
           );
       
       if (!Play)
       {
           UE_LOG(LogTemp, Error, TEXT("[%s] Failed to create PlayMontageTask!"), RoleStr);
           K2_EndAbility();
           return;
       }
       
       Play->OnCompleted.AddDynamic(this, &UGA_Combo::K2_EndAbility);
       Play->OnBlendOut.AddDynamic(this, &UGA_Combo::K2_EndAbility);
       Play->OnInterrupted.AddDynamic(this, &UGA_Combo::K2_EndAbility);
       Play->OnCancelled.AddDynamic(this, &UGA_Combo::K2_EndAbility);
       Play->ReadyForActivation();

       UE_LOG(LogTemp, Warning, TEXT("[%s] PlayMontageTask ready"), RoleStr);

       // ✅ 테스트: 바로 이벤트 발동해보기
       FGameplayEventData EventData;
       EventData.EventTag = MyTags::Abilities::ComboChange;
       
       UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
       if (ASC)
       {
           UE_LOG(LogTemp, Warning, TEXT("[%s] TEST: Sending ComboChange event..."), RoleStr);
           ASC->HandleGameplayEvent(EventData.EventTag, &EventData);
       }

       //WaitComboEventTask는 일단 주석처리 (테스트용)
       WaitComboEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
           this, GetComboChangeEventTag(), nullptr, false, false
       );
       WaitComboEventTask->EventReceived.AddDynamic(this, &UGA_Combo::ComboChangedEventReceived);
       WaitComboEventTask->ReadyForActivation();
    }

    UE_LOG(LogTemp, Warning, TEXT("[%s] Creating CurrentInputTask..."), RoleStr);
    
    CurrentInputTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
    
    if (!CurrentInputTask)
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] Failed to create CurrentInputTask!"), RoleStr);
        K2_EndAbility();
        return;
    }
    
    CurrentInputTask->OnPress.AddDynamic(this, &UGA_Combo::HandleInputPress);
    CurrentInputTask->ReadyForActivation();

    UE_LOG(LogTemp, Warning, TEXT("[%s] CurrentInputTask ready"), RoleStr);

    NextComboName = NAME_None;
    
    UE_LOG(LogTemp, Warning, TEXT("[%s] ActivateAbility END"), RoleStr);
}



#pragma region 기존코드
	// // 콤보 창 오픈(하위 태그 포함: ComboChange.M2 등)
	// if (K2_HasAuthority())
	// {
	// 	UAbilityTask_WaitGameplayEvent* WaitOpen =
	// 		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
	// 			this, GetComboChangeEventTag(), nullptr,
	// 			/*OnlyTriggerOnce*/ false,
	// 			/*OnlyMatchExact*/ false   // 중요! 하위 태그 잡기
	// 		);
	// 	WaitOpen->EventReceived.AddDynamic(this, &UGA_Combo::OnComboWindowOpened);
	// 	WaitOpen->ReadyForActivation();
	//
	// 	// 콤보 창 종료(정확히 End 태그만)
	// 	{
	// 		UAbilityTask_WaitGameplayEvent* WaitEnd =
	// 			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
	// 				this, GetComboChangeEventEndTag(), nullptr,
	// 				/*OnlyTriggerOnce*/ false,
	// 				/*OnlyMatchExact*/ true
	// 			);
	// 		WaitEnd->EventReceived.AddDynamic(this, &UGA_Combo::OnComboWindowEnded);
	// 		WaitEnd->ReadyForActivation();
	// 	}
	// }
#pragma endregion
	
// 	// 콤보 윈도우 이벤트(예측 키 보유측: 서버+예측 클라) 수신
// 	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
// 	{
// 		// Open: 하위 태그 포함 (예: ComboChange.M2)
// 		WaitOpenTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
// 			this, GetComboChangeEventTag(), /*OptionalExternalTarget*/nullptr,
// 			/*OnlyTriggerOnce*/ false,
// 			/*OnlyMatchExact*/ false);
// 		WaitOpenTask->EventReceived.AddDynamic(this, &UGA_Combo::OnComboWindowOpened);
// 		WaitOpenTask->ReadyForActivation();
//
// 		// End: 정확히 End 태그만
// 		WaitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
// 			this, GetComboChangeEventEndTag(), /*OptionalExternalTarget*/nullptr,
// 			/*OnlyTriggerOnce*/ false,
// 			/*OnlyMatchExact*/ true);
// 		WaitEndTask->EventReceived.AddDynamic(this, &UGA_Combo::OnComboWindowEnded);
// 		WaitEndTask->ReadyForActivation();
// 	}
// 	
// 	// 입력 대기(지속적으로 재-설치하여 여러 번 받음)
// 	SetupWaitInputTask();
// }


FGameplayTag UGA_Combo::GetComboChangeEventTag() const
{
	return MyTags::Abilities::ComboChange;
}

FGameplayTag UGA_Combo::GetComboChangeEventEndTag() const
{
	return MyTags::Abilities::ComboChangeEnd;
}

void UGA_Combo::SetupWaitInputTask()
{
	UAbilityTask_WaitInputPress* WaitInputPress = UAbilityTask_WaitInputPress::WaitInputPress(this);
	WaitInputPress->OnPress.AddDynamic(this, &UGA_Combo::HandleInputPress);
	WaitInputPress->ReadyForActivation();
}

void UGA_Combo::HandleInputPress(float TimeWaited)
{
	// SetupWaitInputTask();
	// TryCommitCombo();

	// ✅ Task 재생성 제거 - 기존 Task가 계속 입력을 받음
	// ✅ 서버에서만 콤보 커밋 처리
	if (HasAuthority(&CurrentActivationInfo))
	{
		TryCommitCombo();
	}
}


void UGA_Combo::TryCommitCombo()
{
	// ✅ 서버에서만 실행됨
	if (NextComboName == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("NextComboName is None"));
		return;
	}

	UAnimInstance* OwnerAnimInst = GetOwnerAnimInstance();
	if (!OwnerAnimInst || !ComboMontage)
	{
		return;
	}

	// 현재 섹션 가져오기
	FName CurrentSection = OwnerAnimInst->Montage_GetCurrentSection(ComboMontage);
    
	// ✅ 서버에서 실행하면 자동으로 클라이언트에 복제됨
	OwnerAnimInst->Montage_SetNextSection(CurrentSection, NextComboName, ComboMontage);
    
	UE_LOG(LogTemp, Log, TEXT("Combo: %s -> %s"), 
		*CurrentSection.ToString(), *NextComboName.ToString());
}
void UGA_Combo::ComboChangedEventReceived(FGameplayEventData Data)
{
	// ✅ 권한 체크
	if (!K2_HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("ComboChangedEventReceived called but NO AUTHORITY!"));
		return;
	}

	FGameplayTag EventTag = Data.EventTag;
	UE_LOG(LogTemp, Warning, TEXT("ComboChangedEventReceived: %s"), *EventTag.ToString());

	if (EventTag == GetComboChangeEventEndTag())
	{
		NextComboName = NAME_None;
		UE_LOG(LogTemp, Log, TEXT("Combo Window Closed"));
		return;
	}
    
	TArray<FName> TagNames;
	UGameplayTagsManager::Get().SplitGameplayTagFName(EventTag, TagNames);
    
	if (TagNames.Num() > 0)
	{
		NextComboName = TagNames.Last();
		UE_LOG(LogTemp, Log, TEXT("Combo Window Opened: %s"), *NextComboName.ToString());
	}
}


UAnimInstance* UGA_Combo::GetOwnerAnimInstance() const
{
	if (USkeletalMeshComponent* SkelComp = GetOwningComponentFromActorInfo())
	{
		return SkelComp->GetAnimInstance();
	}
	return nullptr;
}

// void UGA_Combo::OnComboWindowOpened(FGameplayEventData Data)
// {
// 	// 태그의 마지막 토큰을 후보 섹션명으로 사용 (예: ComboChange.M2 -> "M2")
// 	TArray<FName> Tokens;
// 	UGameplayTagsManager::Get().SplitGameplayTagFName(Data.EventTag, Tokens);
// 	CandidateNextSection = Tokens.Num() > 0 ? Tokens.Last() : NAME_None;
//
// 	bComboWindowOpen = true;
// 	NextComboName    = NAME_None; // 새 창이 열렸으니 이전 입력 확정은 초기화
//
// 	UE_LOG(LogTemp, Verbose, TEXT("[Combo] Window OPEN, candidate=%s"),
// 		*CandidateNextSection.ToString());
// }
//
// void UGA_Combo::OnComboWindowEnded(FGameplayEventData Data)
// {
// 	// // 창 닫힘 시점: 입력이 이미 승인돼 있으면 즉시 점프
// 	// if (NextComboName != NAME_None)
// 	// {
// 	// 	if (UAnimInstance* Anim = GetOwnerAnimInstance())
// 	// 	{
// 	// 		if (ComboMontage && Anim->Montage_IsPlaying(ComboMontage))
// 	// 		{
// 	// 			UE_LOG(LogTemp, Log, TEXT("[Combo] Window END -> JumpTo %s"),
// 	// 				*NextComboName.ToString());
// 	// 			Anim->Montage_JumpToSection(NextComboName, ComboMontage);
// 	// 		}
// 	// 	}
// 	// }
// 	// else
// 	// {
// 	// 	UE_LOG(LogTemp, Verbose, TEXT("[Combo] Window END w/o input"));
// 	// }
// 	//
// 	// // 창 종료 후 초기화
// 	// bComboWindowOpen = false;
// 	// CandidateNextSection = NAME_None;
// 	// NextComboName = NAME_None;
//
//
// 	// 섹션 점프는 서버만 수행(권한 측 동기화)
// 	if (HasAuthority(&CurrentActivationInfo) && NextComboName != NAME_None)
// 	{
// 		if (UAnimInstance* Anim = GetOwnerAnimInstance())
// 		{
// 			if (ComboMontage && Anim->Montage_IsPlaying(ComboMontage))
// 			{
// 				// 선택 1) 직접 점프
// 				//Anim->Montage_JumpToSection(NextComboName, ComboMontage);
//
// 				const int32 Idx = ComboMontage->GetSectionIndex(NextComboName);
// 				if (Idx != INDEX_NONE)
// 				{
// 					Anim->Montage_JumpToSection(NextComboName, ComboMontage);
// 				}
// 				else
// 				{
// 					UE_LOG(LogTemp, Warning, TEXT("Invalid section: %s"), *NextComboName.ToString());
// 				}
// 			}
// 		}
// 		else
// 		{
// 			UE_LOG(LogTemp, Verbose, TEXT("[Combo] Window END (no jump)"));
// 		}
// 	}
//
// 	// 창 종료 후 초기화
// 	bComboWindowOpen     = false;
// 	CandidateNextSection = NAME_None;
// 	NextComboName        = NAME_None;
// }
//
// void UGA_Combo::OnInputPressed(float /*TimeWaited*/)
// {
// 	// 다음 입력도 계속 받을 수 있도록 재-설치
// 	SetupWaitInputTask();
//
// 	// 창이 열려 있을 때만 입력을 승인
// 	if (bComboWindowOpen && CandidateNextSection != NAME_None)
// 	{
// 		NextComboName = CandidateNextSection;
// 		UE_LOG(LogTemp, Log, TEXT("[Combo] INPUT accepted -> Next=%s"),
// 			*NextComboName.ToString());
// 	}
// 	else
// 	{
// 		UE_LOG(LogTemp, Verbose, TEXT("[Combo] INPUT ignored (window closed)"));
// 	}
// }
//
//
//
// UAnimInstance* UGA_Combo::GetOwnerAnimInstance() const
// {
// 	if (USkeletalMeshComponent* SkelComp = GetOwningComponentFromActorInfo())
// 	{
// 		return SkelComp->GetAnimInstance();
// 	}
// 	return nullptr;
// }
