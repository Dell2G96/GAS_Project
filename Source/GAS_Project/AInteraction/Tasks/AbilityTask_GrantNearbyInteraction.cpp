// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilityTask_GrantNearbyInteraction.h"

#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GAS_Project/AInteraction/IInteractableTarget.h"
#include "GAS_Project/AInteraction/InteractionOption.h"
#include "GAS_Project/AInteraction/InteractionQuery.h"
#include "GAS_Project/AInteraction/InteractionStatics.h"
#include "GAS_Project/APhysics/LeeCollisionChannels.h"
#include "TimerManager.h"



UAbilityTask_GrantNearbyInteraction::UAbilityTask_GrantNearbyInteraction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


UAbilityTask_GrantNearbyInteraction* UAbilityTask_GrantNearbyInteraction::GrantAbilitiesForNearbyInteractors(
	UGameplayAbility* OwningAbility, float InteractionScanRange, float InteractionScanRate)
{
	UAbilityTask_GrantNearbyInteraction* MyObj = NewAbilityTask<UAbilityTask_GrantNearbyInteraction>(OwningAbility);
	MyObj->InteractionScanRange = InteractionScanRange;
	MyObj->InteractionScanRate = InteractionScanRate;
	return MyObj;
}


void UAbilityTask_GrantNearbyInteraction::Activate()
{
	Super::Activate();
	
	UWorld* World = GetWorld();
	World->GetTimerManager().SetTimer(QueryTimerHandle, this, &ThisClass::QueryInteractables, InteractionScanRate, true);
}



void UAbilityTask_GrantNearbyInteraction::OnDestroy(bool AbilityEnded)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(QueryTimerHandle);
	}

	Super::OnDestroy(AbilityEnded);

}

void UAbilityTask_GrantNearbyInteraction::QueryInteractables()
{
	
	UWorld* World = GetWorld();
	AActor* ActorOwner = GetAvatarActor();
	
	if (World && ActorOwner)
	{
		// 충돌 오버랩 검사 시 사용할 파라미터 생성
		// 이름 지정, 자기 자신을 무시하지 않는 형태
		FCollisionQueryParams Params(SCENE_QUERY_STAT(UAbilityTask_GrantNearbyInteraction), false);

		// 오버랩 결과 담는 배열
		TArray<FOverlapResult> OverlapResults;
		
		// 케릭터 위치를 중심으로 구형 범위 안의 오브젝트를 탐색
		// 반경 내에서 Interaction 채널과 겹치는 대상 식별
		World->OverlapMultiByChannel(OUT OverlapResults, ActorOwner->GetActorLocation(), FQuat::Identity, Lee_TraceChannel_Interaction, FCollisionShape::MakeSphere(InteractionScanRange), Params);

		if (OverlapResults.Num() > 0)
		{
			TArray<TScriptInterface<IInteractableTarget>> InteractableTargets;
			
			//오버랩 결과 중 IInteractableTarget 인터페이스를 구현한 대상만 간추림
			UInteractionStatics::AppendInteractableTargetsFromOverlapResults(OverlapResults, OUT InteractableTargets);
			
			FInteractionQuery InteractionQuery;
			InteractionQuery.RequestingAvatar = ActorOwner;
			InteractionQuery.RequestingController = Cast<AController>(ActorOwner->GetOwner());

			// 모든 대상에서 수집한 상호작용 옵션을 저장할 배열
			TArray<FInteractionOption> Options;
			for (TScriptInterface<IInteractableTarget>& InteractiveTarget : InteractableTargets)
			{
				// 각 인터랙터블 대상이 옵션을 채워 넣을 수 있도록 빌더를 생성
				FInteractionOptionBuilder InteractionBuilder(InteractiveTarget, Options);
				InteractiveTarget->GatherInteractionOptions(InteractionQuery, InteractionBuilder);
			}

			// Check if any of the options need to grant the ability to the user before they can be used.
			for (FInteractionOption& Option : Options)
			{
				if (Option.InteractionAbilityToGrant)
				{
					// Grant the ability to the GAS, otherwise it won't be able to do whatever the interaction is.
					FObjectKey ObjectKey(Option.InteractionAbilityToGrant);
					if (!InteractionAbilityCache.Find(ObjectKey))
					{
						// 어빌리티 스펙을 생성합니다.
						// 레벨 1, 입력 인덱스 없음, 소스 오브젝트는 this 태스크
						FGameplayAbilitySpec Spec(Option.InteractionAbilityToGrant, 1, INDEX_NONE, this);
						FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(Spec);
						InteractionAbilityCache.Add(ObjectKey, Handle);
					}
				}
			}
		}
	}
}
