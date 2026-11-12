// 26.11.12 신규 - 안개벽(봉쇄된 입구) 통과 어빌리티

#include "LeeGameplayAbility_PassBossEntrance.h"

#include "LeeBossEntranceBase.h"
#include "GameFramework/Pawn.h"
#include "GAS_Project/MyTags.h"

// 서버 전용 + 상호작용 활성화 이벤트로 트리거 (화톳불 휴식과 같은 방식)
ULeeGameplayAbility_PassBossEntrance::ULeeGameplayAbility_PassBossEntrance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = MyTags::Lyra::Ability_Interaction_Activate;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

// 이벤트의 Instigator(플레이어)를 Target(입구) 안쪽으로 통과시키고 바로 종료
void ULeeGameplayAbility_PassBossEntrance::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APawn* PlayerPawn = TriggerEventData ? const_cast<APawn*>(Cast<APawn>(TriggerEventData->Instigator.Get())) : nullptr;
	ALeeBossEntranceBase* Entrance = TriggerEventData ? const_cast<ALeeBossEntranceBase*>(Cast<ALeeBossEntranceBase>(TriggerEventData->Target.Get())) : nullptr;

	if (PlayerPawn && Entrance)
	{
		Entrance->PassThrough(PlayerPawn);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
