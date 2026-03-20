// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePlayerBotController.h"

#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/GameModes/LeeGameModeBase.h"
#include "Perception/AIPerceptionComponent.h"
#include "GameFramework/PlayerState.h"


ALeePlayerBotController::ALeePlayerBotController(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	bWantsPlayerState = true;
	bStopAILogicOnUnposses = false;
}

void ALeePlayerBotController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	UE_LOG(LogLee, Error, TEXT("플레이어 봇 컨트롤러(%s)에는 팀 ID를 직접 설정할 수 없습니다. 팀 ID는 연결된 플레이어 스테이트에 의해 결정됩니다."), *GetPathNameSafe(this));
}

FGenericTeamId ALeePlayerBotController::GetGenericTeamId() const
{
	if (ILeeTeamAgentInterface* PSWithTeamInterface = Cast<ILeeTeamAgentInterface>(PlayerState))
	{
		return PSWithTeamInterface->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

FOnLeeTeamIndexChangedDelegate* ALeePlayerBotController::GetOnTeamIndexChangedDelegate()
{
    return &OnTeamChangedDelegate;
}

ETeamAttitude::Type ALeePlayerBotController::GetTeamAttitudeTowards(const AActor& Other) const
{
	if (const APawn* OtherPawn = Cast<APawn>(&Other))
	{
		if (const ILeeTeamAgentInterface* TeamAgent = Cast<ILeeTeamAgentInterface>(OtherPawn->GetController()))
		{
			FGenericTeamId OtherTeamId = TeamAgent->GetGenericTeamId();

			// 상대 폰의 ID 확인
			if (OtherTeamId.GetId() != GetGenericTeamId().GetId())
			{
				return ETeamAttitude::Hostile;
			}
			else
			{
				return ETeamAttitude::Friendly;
			}
		}
	}
	return ETeamAttitude::Neutral;
}

void ALeePlayerBotController::ServerRestartController()
{
	if (GetNetMode() == NM_Client)
	{
		return;
	}

	ensure((GetPawn() == nullptr) && IsInState(NAME_Inactive));

	if (IsInState(NAME_Inactive) || (IsInState(NAME_Spectating)))
	{
		ALeeGameModeBase* const GameMode = GetWorld()->GetAuthGameMode<ALeeGameModeBase>();

		if ((GameMode == nullptr) || !GameMode->ControllerCanRestart(this))
		{
			return;
		}

		// 아직 pawn 에 붙어있다면 분리시킴
		if (GetPawn() != nullptr)
		{
			UnPossess();
		}

		// 입력을 다시 활성화
		ResetIgnoreInputFlags();

		GameMode->RestartPlayer(this);
	}
}

void ALeePlayerBotController::UpdateTeamAttitude(UAIPerceptionComponent* AIPerception)
{
	if (AIPerception)
	{
		AIPerception->RequestStimuliListenerUpdate();
	}
}

void ALeePlayerBotController::OnUnPossess()
{
	Super::OnUnPossess();
}

void ALeePlayerBotController::OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	ConditionalBroadcastTeamChanged(this, IntegerToGenericTeamId(OldTeam), IntegerToGenericTeamId(NewTeam));
}

void ALeePlayerBotController::OnPlayerStateChanged()
{
	
}

void ALeePlayerBotController::BroadcastOnPlayerStateChanged()
{
	OnPlayerStateChanged();

	FGenericTeamId OldTeamId = FGenericTeamId::NoTeam;
	if (LastSeenPlayerState != nullptr)
	{
		if (ILeeTeamAgentInterface* PlayerStateTeamInterface = Cast<ILeeTeamAgentInterface>(LastSeenPlayerState))
		{
			OldTeamId = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().RemoveAll(this);
		}
	}

	// 새 플레이어 스테이트가 있다면
	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (PlayerState != nullptr)
	{
		if (ILeeTeamAgentInterface* PlayerStateTeamInterface = Cast<ILeeTeamAgentInterface>(PlayerState))
		{
			NewTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnPlayerStateChangedTeam);
		}
	}

	// 팀이 실제로 변경된 경우 
	ConditionalBroadcastTeamChanged(this, OldTeamId, NewTeamID);
	
	LastSeenPlayerState = PlayerState;
}

void ALeePlayerBotController::InitPlayerState()
{
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void ALeePlayerBotController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();

}

void ALeePlayerBotController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();

}
