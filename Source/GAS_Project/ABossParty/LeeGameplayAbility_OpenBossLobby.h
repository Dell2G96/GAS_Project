// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/AAbilitySystem/Abilities/LeeGameplayAbility.h"
#include "LeeGameplayAbility_OpenBossLobby.generated.h"

// 26.11.02 추가 - 도움 요청 Actor와 상호작용하면 실행되는 로컬 전용 어빌리티. 로비 위젯을 Menu 레이어에 띄우고 바로 종료한다
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_OpenBossLobby : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_OpenBossLobby(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
