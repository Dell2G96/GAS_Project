// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeBossPartySubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "LeeExperienceDefinition.h"
#include "LeeExperienceManagerComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACharacter/LeeHealthComponent.h"
#include "GAS_Project/AMessage/LeeVerbMessage.h"
#include "GAS_Project/APlayer/LeePlayerState.h"
#include "GAS_Project/APlayer/LeePlayerController.h"
#include "GAS_Project/ABossParty/LeeBossExpeditionSubsystem.h"

// 게임/PIE 월드에서만 생성한다 (에디터 프리뷰 월드 등 제외)
bool ULeeBossPartySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// GameState의 ExperienceManager에서 현재 Experience를 읽어 PlayMode가 보스전인지 확인한다
bool ULeeBossPartySubsystem::IsBossMultiplayer(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	const ULeeExperienceManagerComponent* ExperienceComponent = GameState ? GameState->FindComponentByClass<ULeeExperienceManagerComponent>() : nullptr;
	if (!ExperienceComponent || !ExperienceComponent->IsExperienceLoaded())
	{
		return false;
	}

	return ExperienceComponent->GetCurrentExperienceChecked()->PlayMode == ELeePlayMode::MultiplayerBoss;
}

// 보스전 Experience면 반환, 아니면 nullptr
const ULeeExperienceDefinition* ULeeBossPartySubsystem::GetBossExperience() const
{
	if (!IsBossMultiplayer(this))
	{
		return nullptr;
	}

	const ULeeExperienceManagerComponent* ExperienceComponent = GetWorld()->GetGameState()->FindComponentByClass<ULeeExperienceManagerComponent>();
	return ExperienceComponent->GetCurrentExperienceChecked();
}

// 플레이어가 조종 중이고, 사망하지 않았고, 넉다운도 아닌지
bool ULeeBossPartySubsystem::IsPlayerStanding(const APawn* Pawn)
{
	if (!IsPlayerNotDead(Pawn))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	return !(ASC && ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Downed));
}

// 플레이어가 조종 중이고, 사망이 시작되지 않았는지 (넉다운은 살아 있는 것으로 본다)
bool ULeeBossPartySubsystem::IsPlayerNotDead(const APawn* Pawn)
{
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return false;
	}

	const ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(Pawn);
	return HealthComponent && HealthComponent->GetDeathState() == ELeeDeathState::NotDead;
}

// 모든 PlayerController의 Pawn 중 조건에 맞는 것만 모은다
TArray<APawn*> ULeeBossPartySubsystem::GetPlayerPawns(bool bStandingOnly) const
{
	TArray<APawn*> Result;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
		if (bStandingOnly ? IsPlayerStanding(Pawn) : IsPlayerNotDead(Pawn))
		{
			Result.Add(Pawn);
		}
	}
	return Result;
}

// 보스전이고, 피해자를 제외한 "서 있는" 동료가 1명 이상이면 넉다운 가능 (마지막 생존자는 바로 사망)
bool ULeeBossPartySubsystem::CanEnterDowned(const APawn* VictimPawn) const
{
	if (bWipeInProgress || bVictoryInProgress || !GetBossExperience() || !VictimPawn || !VictimPawn->IsPlayerControlled())
	{
		return false;
	}

	for (APawn* Pawn : GetPlayerPawns(/*bStandingOnly*/true))
	{
		if (Pawn != VictimPawn)
		{
			return true;
		}
	}
	return false;
}

// 사망한 플레이어 처리 — 서 있는 동료가 없으면 전멸, 있으면 사망자는 관전 시작
void ULeeBossPartySubsystem::HandlePlayerDeathStarted(APawn* DeadPawn)
{
	if (bWipeInProgress || bVictoryInProgress || !GetBossExperience() || !DeadPawn || !DeadPawn->IsPlayerControlled())
	{
		return;
	}

	// 이 Pawn을 관전하던 사람은 다른 동료로 옮긴다
	RetargetSpectatorsWatching(DeadPawn);

	if (GetPlayerPawns(/*bStandingOnly*/true).Num() == 0)
	{
		StartPartyWipe();
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(DeadPawn->GetController()))
	{
		SpectateNextTeammate(PlayerController);
	}
}

// 사망하지 않은 동료(넉다운 포함) 중 Current 다음 순서의 Pawn을 고른다 (끝이면 처음으로)
APawn* ULeeBossPartySubsystem::FindNextSpectateTarget(const APawn* Exclude, const AActor* Current) const
{
	TArray<APawn*> Candidates = GetPlayerPawns(/*bStandingOnly*/false);
	Candidates.Remove(const_cast<APawn*>(Exclude));
	if (Candidates.Num() == 0)
	{
		return nullptr;
	}

	const int32 CurrentIndex = Candidates.IndexOfByKey(Current);
	return Candidates[(CurrentIndex + 1) % Candidates.Num()];
}

// 관전자의 뷰 타겟을 다음 동료로 바꾼다 (서버에서 SetViewTarget → 엔진이 소유 클라이언트에 전달)
void ULeeBossPartySubsystem::SpectateNextTeammate(APlayerController* SpectatorController)
{
	if (!SpectatorController || bWipeInProgress)
	{
		return;
	}

	// 살아 있는 플레이어는 관전하지 않는다
	if (IsPlayerNotDead(SpectatorController->GetPawn()))
	{
		return;
	}

	if (APawn* NextTarget = FindNextSpectateTarget(SpectatorController->GetPawn(), SpectatorController->GetViewTarget()))
	{
		SpectatorController->SetViewTargetWithBlend(NextTarget, 0.5f);
	}
}

// LostTarget을 보고 있던 사망자들의 카메라를 다른 동료로 옮긴다
void ULeeBossPartySubsystem::RetargetSpectatorsWatching(const APawn* LostTarget)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (PlayerController && PlayerController->GetViewTarget() == LostTarget && PlayerController->GetPawn() != LostTarget)
		{
			SpectateNextTeammate(PlayerController);
		}
	}
}

// 전멸: 넉다운 중인 플레이어를 모두 사망시키고, 전원에게 YOU DIED를 보낸 뒤 일정 시간 후 로비로 이동
void ULeeBossPartySubsystem::StartPartyWipe()
{
	bWipeInProgress = true;

	for (APawn* Pawn : GetPlayerPawns(/*bStandingOnly*/false))
	{
		if (ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(Pawn))
		{
			HealthComponent->TriggerDeathWithoutDamage();
		}
	}

	SendMessageToAllPlayers(MyTags::Souls::Message_Death_PartyWiped);

	const float TravelDelay = GetBossExperience()->WipeTravelDelay;
	GetWorld()->GetTimerManager().SetTimer(WipeTravelTimerHandle, this, &ThisClass::TravelToLobby, FMath::Max(TravelDelay, 0.01f), false);
}

// Experience에 지정된 로비 맵으로 접속을 유지한 채 이동한다 (보스맵을 벗어나므로 보스도 초기화됨)
void ULeeBossPartySubsystem::TravelToLobby()
{
	const ULeeExperienceDefinition* Experience = GetBossExperience();
	const FString LobbyMapName = Experience ? Experience->BossLobbyMap.GetLongPackageName() : FString();
	if (LobbyMapName.IsEmpty())
	{
		UE_LOG(LogLee, Warning, TEXT("[BossParty] Experience에 BossLobbyMap이 지정되지 않아 로비로 이동할 수 없습니다."));
		return;
	}

	GetWorld()->ServerTravel(LobbyMapName);
}

// 26.11.02 변경 - 전멸 메시지 루프를 함수로 분리 (보스 처치 VICTORY에도 재사용)
// 로컬 플레이어(호스트)는 바로 방송, 원격 플레이어는 PlayerState의 Client RPC로 전달
void ULeeBossPartySubsystem::SendMessageToAllPlayers(FGameplayTag MessageTag)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!PlayerController)
		{
			continue;
		}

		FLeeVerbMessage Message;
		Message.Verb = MessageTag;
		Message.Instigator = PlayerController;
		Message.Target = PlayerController->PlayerState;

		if (PlayerController->IsLocalController())
		{
			UGameplayMessageSubsystem::Get(PlayerController).BroadcastMessage(Message.Verb, Message);
		}
		else if (ALeePlayerState* LeePlayerState = PlayerController->GetPlayerState<ALeePlayerState>())
		{
			LeePlayerState->ClientBroadcastMessage(Message);
		}
	}
}

// 26.11.02 추가 - 보스(Actor Tag "Boss") 사망 → 전원 VICTORY 표시, VictoryTravelDelay 뒤 각자 필드로 복귀
void ULeeBossPartySubsystem::HandleBossDeathStarted(AActor* DeadActor)
{
	if (bWipeInProgress || bVictoryInProgress || !DeadActor || !DeadActor->HasAuthority()
		|| !DeadActor->ActorHasTag(TEXT("Boss")) || !GetBossExperience())
	{
		return;
	}

	bVictoryInProgress = true;
	SendMessageToAllPlayers(MyTags::Souls::Message_Boss_Victory);

	const float TravelDelay = GetBossExperience()->VictoryTravelDelay;
	GetWorld()->GetTimerManager().SetTimer(VictoryTravelTimerHandle, this, &ThisClass::ReturnPartyToField, FMath::Max(TravelDelay, 0.01f), false);
}

// 26.11.02 추가 - 원격 플레이어부터 각자 필드로 보내고, 호스트는 1초 뒤 복귀 (호스트가 먼저 나가면 연결 끊김으로 처리되므로)
void ULeeBossPartySubsystem::ReturnPartyToField()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ALeePlayerController* PlayerController = Cast<ALeePlayerController>(It->Get());
		if (PlayerController && !PlayerController->IsLocalController())
		{
			PlayerController->ClientReturnToField();
		}
	}

	GetWorld()->GetTimerManager().SetTimer(VictoryTravelTimerHandle, this, &ThisClass::ReturnHostToField, 1.0f, false);
}

// 26.11.02 추가 - 호스트 자신의 세션 정리 + 필드 복귀
void ULeeBossPartySubsystem::ReturnHostToField()
{
	if (ULeeBossExpeditionSubsystem* ExpeditionSubsystem = ULeeBossExpeditionSubsystem::Get(this))
	{
		ExpeditionSubsystem->ReturnToField();
	}
}
