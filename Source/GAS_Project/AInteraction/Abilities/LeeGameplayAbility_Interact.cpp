// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameplayAbility_Interact.h"

#include "AbilitySystemComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AInteraction/IInteractableTarget.h"
#include "GAS_Project/AInteraction/InteractionStatics.h"
#include "GAS_Project/AInteraction/Tasks/AbilityTask_GrantNearbyInteraction.h"
#include "GAS_Project/APlayer/LeePlayerController.h"
#include "GAS_Project/AUI/IndicatorDescriptor.h"
#include "GAS_Project/System/LeeIndicatorManagerComponent.h"


ULeeGameplayAbility_Interact::ULeeGameplayAbility_Interact(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	ActivationPolicy = ELeeAbilityActivationPolicy::OnSpawn;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void ULeeGameplayAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	if (AbilitySystem && AbilitySystem->GetOwnerRole() == ROLE_Authority )
	{
		UAbilityTask_GrantNearbyInteraction* Task = UAbilityTask_GrantNearbyInteraction::GrantAbilitiesForNearbyInteractors(this, InteractionScanRange, InteractionScanRate);
		Task->ReadyForActivation();
	}
}

void ULeeGameplayAbility_Interact::UpdateInteractions(const TArray<FInteractionOption>& InteractionOptions)
{
	if (ALeePlayerController* PC = GetLeePlayerControllerFromActorInfo())
	{
		if (ULeeIndicatorManagerComponent* IndicatorManager = ULeeIndicatorManagerComponent::GetComponent(PC))
		{
			for (UIndicatorDescriptor* Indicator : Indicators)
			{
				IndicatorManager->RemoveIndicator(Indicator);
			}
			Indicators.Reset();
			
			for (const FInteractionOption& InteractionOption : InteractionOptions)
			{
				AActor* InteractableTargetActor = UInteractionStatics::GetActorFromInteractableTarget(InteractionOption.InteractableTarget);
				
				TSoftClassPtr<UUserWidget> InteractionWidgetClass = InteractionOption.InteractionWidgetClass.IsNull() ? DefaultInteractionWidgetClass : InteractionOption.InteractionWidgetClass;
				
				UIndicatorDescriptor* Indicator = NewObject<UIndicatorDescriptor>();
				Indicator->SetDataObject(InteractableTargetActor);
				Indicator->SetSceneComponent(InteractableTargetActor->GetRootComponent());
				Indicator->SetIndicatorClass(InteractionWidgetClass);
				IndicatorManager->AddIndicator(Indicator);
				
				Indicators.Add(Indicator);
			}
		}
		else
		{
			
		}
	}
	CurrentOptions = InteractionOptions;
}

void ULeeGameplayAbility_Interact::TriggerInteraction()
{
	  if (CurrentOptions.Num() == 0)
    {
	  	// 현재 사용 가능한 상호작용 옵션이 없으면 즉시 종료
       return;  
    }

    UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
    // 상호작용을 실행할 능력 시스템 컴포넌트를 가져옵니다
    if (AbilitySystem)
    {
       const FInteractionOption& InteractionOption = CurrentOptions[0];
       // 첫 번째(우선순위가 높은) 상호작용 옵션을 선택합니다

       AActor* Instigator = GetAvatarActorFromActorInfo();
       // 상호작용을 시작한 주체(플레이어 캐릭터)를 가져옵니다
       AActor* InteractableTargetActor = UInteractionStatics::GetActorFromInteractableTarget(InteractionOption.InteractableTarget);
       // 인터랙션 대상의 실제 액터를 가져옵니다
    	
       // 대상이 전달할 이벤트 데이터를 커스터마이즈할 수 있도록 허용합니다
       // 대상 액터만 아는 커스텀 데이터가 필요할 경우를 대비합니다
       FGameplayEventData Payload;
       Payload.EventTag = MyTags::Lyra::Ability_Interaction_Activate;  
    	// 상호작용 활성화 이벤트 태그 설정
       Payload.Instigator = Instigator;  // 이벤트 발생자(플레이어) 설정
       Payload.Target = InteractableTargetActor;  // 기본 타겟 설정

   
       // 필요시 인터랙션 대상이 이벤트 데이터를 조작할 수 있도록 합니다
       // 예: 벽의 버튼이 문 액터를 지정하고 싶을 때 Target을 문 액터로 오버라이드할 수 있습니다
       InteractionOption.InteractableTarget->CustomizeInteractionEventData(          MyTags::Lyra::Ability_Interaction_Activate , Payload);

      
       // 페이로드에서 타겟 액터를 가져와 상호작용의 '아바타'로 사용하고,
       // 원본 InteractableTarget 액터를 소유자 액터로 사용합니다
       AActor* TargetActor = const_cast<AActor*>(ToRawPtr(Payload.Target));

       // 상호작용에 필요한 액터 정보를 초기화합니다
       FGameplayAbilityActorInfo ActorInfo;
       ActorInfo.InitFromActor(InteractableTargetActor, TargetActor, InteractionOption.TargetAbilitySystem);

       // 이벤트 태그를 사용하여 능력을 트리거합니다
       const bool bSuccess = InteractionOption.TargetAbilitySystem->TriggerAbilityFromGameplayEvent(
          InteractionOption.TargetInteractionAbilityHandle,  // 실행할 능력 핸들
          &ActorInfo,                                        // 액터 정보
          MyTags::Lyra::Ability_Interaction_Activate,                  // 트리거 태그
          &Payload,                                          // 이벤트 페이로드
          *InteractionOption.TargetAbilitySystem             // 대상 ASC
       );
    }
}

















