#include "GA_Combo.h"

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
	// 5.6 경고 대응: AbilityTags -> AssetTags
	FGameplayTagContainer Tags;
	Tags.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());
	SetAssetTags(Tags);
	BlockAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());

	// ✅ Instancing Policy 설정
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// ✅ NetExecutionPolicy 설정 (클라이언트도 예측 실행)
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

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
	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	// 초기 상태
	bComboWindowOpen = false;
	CandidateNextSection = NAME_None;
	NextComboName = NAME_None;

	if (HasAuthorityOrPredictionKey(ActorInfo,&ActivationInfo))
	{
		
		// 몽타주 재생
		UAbilityTask_PlayMontageAndWait* Play =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ComboMontage);
		Play->OnCompleted.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		Play->OnBlendOut.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		Play->OnInterrupted.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		Play->OnCancelled.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		Play->ReadyForActivation();

		
	}

	// 콤보 창 오픈(하위 태그 포함: ComboChange.M2 등)
	{
		UAbilityTask_WaitGameplayEvent* WaitOpen =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, GetComboChangeEventTag(), nullptr,
				/*OnlyTriggerOnce*/ false,
				/*OnlyMatchExact*/ false // 중요! 하위 태그 잡기
			);
		WaitOpen->EventReceived.AddDynamic(this, &UGA_Combo::OnComboWindowOpened);
		WaitOpen->ReadyForActivation();
	}

	// 콤보 창 종료(정확히 End 태그만)
	{
		UAbilityTask_WaitGameplayEvent* WaitEnd =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, GetComboChangeEventEndTag(), nullptr,
				/*OnlyTriggerOnce*/ false,
				/*OnlyMatchExact*/ true
			);
		WaitEnd->EventReceived.AddDynamic(this, &UGA_Combo::OnComboWindowEnded);
		WaitEnd->ReadyForActivation();
	}

	// 입력 대기(지속적으로 재-설치하여 여러 번 받음)
	SetupWaitInputTask();
}

FGameplayTag UGA_Combo::GetComboChangeEventTag() const
{
	return MyTags::Abilities::ComboChange;
}

FGameplayTag UGA_Combo::GetComboChangeEventEndTag() const
{
	return MyTags::Abilities::ComboChangeEnd;
}

void UGA_Combo::Multicast_JumpToSection_Implementation(FName SectionName)
{
	UE_LOG(LogTemp, Log, TEXT("[Combo] Multicast JumpTo %s"), *SectionName.ToString());
    
	if (UAnimInstance* Anim = GetOwnerAnimInstance())
	{
		if (ComboMontage && Anim->Montage_IsPlaying(ComboMontage))
		{
			Anim->Montage_JumpToSection(SectionName, ComboMontage);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Combo] Montage not playing when trying to jump"));
		}
	}
}

void UGA_Combo::OnComboWindowOpened(FGameplayEventData Data)
{
	// // 태그의 마지막 토큰을 후보 섹션명으로 사용 (예: ComboChange.M2 -> "M2")
	// TArray<FName> Tokens;
	// UGameplayTagsManager::Get().SplitGameplayTagFName(Data.EventTag, Tokens);
	// CandidateNextSection = Tokens.Num() > 0 ? Tokens.Last() : NAME_None;
	//
	// bComboWindowOpen = true;
	// NextComboName = NAME_None; // 새 창이 열렸으니 이전 입력 확정은 초기화
	//
	// UE_LOG(LogTemp, Verbose, TEXT("[Combo] Window OPEN, candidate=%s"),
	//        *CandidateNextSection.ToString());

	// 창 닫힘 시점: 입력이 이미 승인돼 있으면 즉시 점프
	if (NextComboName != NAME_None)
	{
		UE_LOG(LogTemp, Log, TEXT("[Combo] Window END -> Requesting Jump to %s"), 
			*NextComboName.ToString());
        
		// ✅ 서버라면 Multicast로 모든 클라이언트에 전파
		if (GetActorInfo().IsNetAuthority())
		{
			Multicast_JumpToSection(NextComboName);
		}
		// ✅ 클라이언트라면 서버에 요청
		else
		{
			// 예측 실행: 로컬에서 즉시 점프
			if (UAnimInstance* Anim = GetOwnerAnimInstance())
			{
				if (ComboMontage && Anim->Montage_IsPlaying(ComboMontage))
				{
					Anim->Montage_JumpToSection(NextComboName, ComboMontage);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Combo] Window END w/o input"));
	}
    
	// 창 종료 후 초기화
	bComboWindowOpen = false;
	CandidateNextSection = NAME_None;
	NextComboName = NAME_None;
}

void UGA_Combo::OnComboWindowEnded(FGameplayEventData Data)
{
	// 창 닫힘 시점: 입력이 이미 승인돼 있으면 즉시 점프
	if (NextComboName != NAME_None)
	{
		if (UAnimInstance* Anim = GetOwnerAnimInstance())
		{
			if (ComboMontage && Anim->Montage_IsPlaying(ComboMontage))
			{
				UE_LOG(LogTemp, Log, TEXT("[Combo] Window END -> JumpTo %s"),
				       *NextComboName.ToString());
				Anim->Montage_JumpToSection(NextComboName, ComboMontage);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Combo] Window END w/o input"));
	}

	// 창 종료 후 초기화
	bComboWindowOpen = false;
	CandidateNextSection = NAME_None;
	NextComboName = NAME_None;
}

void UGA_Combo::OnInputPressed(float /*TimeWaited*/)
{
	// 다음 입력도 계속 받을 수 있도록 재-설치
	SetupWaitInputTask();

	// 창이 열려 있을 때만 입력을 승인
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

void UGA_Combo::SetupWaitInputTask()
{
	if (UAbilityTask_WaitInputPress* Wait = UAbilityTask_WaitInputPress::WaitInputPress(this))
	{
		Wait->OnPress.AddDynamic(this, &UGA_Combo::OnInputPressed);
		Wait->ReadyForActivation();
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
