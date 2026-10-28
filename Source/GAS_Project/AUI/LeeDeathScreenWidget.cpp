// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeDeathScreenWidget.h"

#include "GAS_Project/MyTags.h"
#include "GAS_Project/AMessage/LeeVerbMessage.h"

// 사망/부활 메시지 채널 구독
void ULeeDeathScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	YouDiedListenerHandle = MessageSystem.RegisterListener(
		MyTags::Souls::Message_Death_YouDied, this, &ThisClass::HandleYouDiedMessage);
	RespawnListenerHandle = MessageSystem.RegisterListener(
		MyTags::Souls::Message_Death_Respawn, this, &ThisClass::HandleRespawnMessage);
}

// 구독 해제
void ULeeDeathScreenWidget::NativeDestruct()
{
	YouDiedListenerHandle.Unregister();
	RespawnListenerHandle.Unregister();

	Super::NativeDestruct();
}

// 사망 메시지 수신 → BP OnYouDied
void ULeeDeathScreenWidget::HandleYouDiedMessage(FGameplayTag Channel, const FLeeVerbMessage& Message)
{
	OnYouDied();
}

// 부활 메시지 수신 → BP OnRespawn
void ULeeDeathScreenWidget::HandleRespawnMessage(FGameplayTag Channel, const FLeeVerbMessage& Message)
{
	OnRespawn();
}
