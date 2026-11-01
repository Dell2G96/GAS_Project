// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameSession.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GAS_Project/GameModes/LeeExperienceDefinition.h"
#include "GAS_Project/GameModes/LeeExperienceManagerComponent.h"

ALeeGameSession::ALeeGameSession(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool ALeeGameSession::ProcessAutoLogin()
{
	// This is actually handled in LyraGameMode::TryDedicatedServerLogin
	return true;

}

// 원격 플레이어 접속 요청 시 현재 Experience의 PlayMode로 허용 여부 결정 (빈 문자열 = 허용)
FString ALeeGameSession::ApproveLogin(const FString& Options)
{
	FString ErrorMessage = Super::ApproveLogin(Options);
	if (!ErrorMessage.IsEmpty())
	{
		return ErrorMessage;
	}

	AGameStateBase* GameState = GetWorld()->GetGameState();
	ULeeExperienceManagerComponent* ExperienceComponent = GameState ? GameState->FindComponentByClass<ULeeExperienceManagerComponent>() : nullptr;

	// Experience 로딩 중이면 일단 허용 → 로드 완료 시 OnExperienceLoaded에서 다시 검사
	if (!ExperienceComponent || !ExperienceComponent->IsExperienceLoaded())
	{
		return ErrorMessage;
	}

	const ULeeExperienceDefinition* Experience = ExperienceComponent->GetCurrentExperienceChecked();

	if (Experience->PlayMode == ELeePlayMode::SingleField)
	{
		return TEXT("Single player area");
	}

	// ApproveLogin은 원격 접속만 호출되므로 GetNumPlayers()에는 호스트가 포함되어 있다
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (GameMode && GameMode->GetNumPlayers() >= Experience->MaxPlayers)
	{
		return TEXT("Boss session is full");
	}

	return ErrorMessage;
}

// 서버에서만 Experience 로드 완료 콜백 등록 (이미 로드됐으면 즉시 호출됨)
void ALeeGameSession::BeginPlay()
{
	Super::BeginPlay();

	AGameStateBase* GameState = GetWorld()->GetGameState();
	ULeeExperienceManagerComponent* ExperienceComponent = GameState ? GameState->FindComponentByClass<ULeeExperienceManagerComponent>() : nullptr;
	if (HasAuthority() && ExperienceComponent)
	{
		ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnLeeExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
	}
}

// 접속 순서대로 인원을 세며, 필드면 원격 플레이어 전부, 보스전이면 MaxPlayers 초과분을 내보냄
void ALeeGameSession::OnExperienceLoaded(const ULeeExperienceDefinition* Experience)
{
	// KickPlayer가 PC를 Destroy하므로 순회 중 목록이 바뀌지 않게 먼저 모아 둔다
	TArray<APlayerController*> PlayersToKick;
	int32 PlayerCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		++PlayerCount;

		// 호스트(로컬 플레이어)는 내보내지 않는다
		if (PC->IsLocalController())
		{
			continue;
		}

		if (Experience->PlayMode == ELeePlayMode::SingleField || PlayerCount > Experience->MaxPlayers)
		{
			PlayersToKick.Add(PC);
		}
	}

	const FText Reason = FText::FromString(Experience->PlayMode == ELeePlayMode::SingleField ? TEXT("Single player area") : TEXT("Boss session is full"));
	for (APlayerController* PC : PlayersToKick)
	{
		KickPlayer(PC, Reason);
	}
}

void ALeeGameSession::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
}

void ALeeGameSession::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
}
