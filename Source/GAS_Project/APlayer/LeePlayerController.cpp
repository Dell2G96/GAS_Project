// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePlayerController.h"

#include "LeePlayerState.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/ASavePoint/LeeSavePointSubsystem.h"
#include "GAS_Project/ACamera/LeePlayerCameraManager.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/GameModes/LeeBossPartySubsystem.h"

ALeePlayerController::ALeePlayerController(const FObjectInitializer& ObjectInitializer)
{
	PlayerCameraManagerClass = ALeePlayerCameraManager::StaticClass();
}

void ALeePlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (ULeeAbilitySystemComponent* LeeASC= GetLeeAbilitySystemComponent())
	{
		LeeASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}
	Super::PostProcessInput(DeltaTime, bGamePaused);
}

class ALeePlayerState* ALeePlayerController::GetLeePlayerState() const
{
	return CastChecked<ALeePlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

class ULeeAbilitySystemComponent* ALeePlayerController::GetLeeAbilitySystemComponent() const
{
	const ALeePlayerState* LeePlayerState = GetLeePlayerState();
	return (LeePlayerState ? LeePlayerState->GetLeeAbilitySystemComponent() : nullptr);
}

// [서버] 부활 요청을 SavePointSubsystem에 전달 (사망 대기 중인지 검사는 Subsystem이 담당)
void ALeePlayerController::ServerRequestRespawn_Implementation()
{
	if (ULeeSavePointSubsystem* SavePointSubsystem = GetWorld()->GetSubsystem<ULeeSavePointSubsystem>())
	{
		SavePointSubsystem->RequestRespawn(this);
	}
}

// 26.11.02 추가 - 관전 대상 전환용 좌클릭 바인딩 (공격 입력과 겹치지만 사망 중에만 동작)
void ALeePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ThisClass::HandleSpectateNextInput);
}

// 내 캐릭터가 사망 중(Status.Death.Dying)일 때만 서버에 다음 관전 대상을 요청
void ALeePlayerController::HandleSpectateNextInput()
{
	const ULeeAbilitySystemComponent* LeeASC = GetLeeAbilitySystemComponent();
	if (LeeASC && LeeASC->HasMatchingGameplayTag(MyTags::Souls::Status_Death_Dying))
	{
		ServerSpectateNextTeammate();
	}
}

// [서버] 보스 파티 Subsystem이 다음 생존 동료로 뷰 타겟을 바꾼다
void ALeePlayerController::ServerSpectateNextTeammate_Implementation()
{
	if (ULeeBossPartySubsystem* BossPartySubsystem = GetWorld()->GetSubsystem<ULeeBossPartySubsystem>())
	{
		BossPartySubsystem->SpectateNextTeammate(this);
	}
}
