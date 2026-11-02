// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameplayAbility_OpenBossLobby.h"

#include "CommonActivatableWidget.h"
#include "CommonUIExtensions.h"
#include "LeeBossLobbyWidget.h"
#include "LeeBossRequestActor.h"
#include "GameFramework/PlayerController.h"
#include "GAS_Project/MyTags.h"

// UI만 여는 어빌리티라 서버를 거치지 않는다 (LocalOnly)
ULeeGameplayAbility_OpenBossLobby::ULeeGameplayAbility_OpenBossLobby(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

// 상호작용 대상(도움 요청 Actor)의 로비 위젯을 띄우고, 위젯에 Actor 정보를 넘긴 뒤 종료
void ULeeGameplayAbility_OpenBossLobby::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ALeeBossRequestActor* RequestActor = TriggerEventData ? Cast<ALeeBossRequestActor>(const_cast<AActor*>(ToRawPtr(TriggerEventData->Target))) : nullptr;
	APlayerController* PlayerController = ActorInfo ? ActorInfo->PlayerController.Get() : nullptr;

	if (RequestActor && PlayerController && PlayerController->IsLocalController())
	{
		if (UClass* WidgetClass = RequestActor->GetLobbyWidgetClass().LoadSynchronous())
		{
			UCommonActivatableWidget* Widget = UCommonUIExtensions::PushContentToLayer_ForPlayer(
				PlayerController->GetLocalPlayer(), MyTags::Lyra::UI_Layer_Menu, WidgetClass);

			if (ULeeBossLobbyWidget* LobbyWidget = Cast<ULeeBossLobbyWidget>(Widget))
			{
				LobbyWidget->InitializeLobby(RequestActor);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
