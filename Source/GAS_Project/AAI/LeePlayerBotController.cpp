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
	// 1순위: 연결된 PlayerState의 팀 (팀 정보가 PlayerState에 있는 플레이어형 봇)
	if (const ILeeTeamAgentInterface* PSWithTeamInterface = Cast<ILeeTeamAgentInterface>(PlayerState))
	{
		const FGenericTeamId PSTeamId = PSWithTeamInterface->GetGenericTeamId();
		if (PSTeamId != FGenericTeamId::NoTeam)
		{
			return PSTeamId;
		}
	}

	// 2순위: PlayerState에 팀이 없으면(PlayerState 없는 Enemy 몬스터 등) 빙의 중인 Pawn의 팀을 사용
	if (const IGenericTeamAgentInterface* PawnWithTeam = Cast<IGenericTeamAgentInterface>(GetPawn()))
	{
		return PawnWithTeam->GetGenericTeamId();
	}

	return FGenericTeamId::NoTeam;
}

FOnLeeTeamIndexChangedDelegate* ALeePlayerBotController::GetOnTeamIndexChangedDelegate()
{
    return &OnTeamChangedDelegate;
}

ETeamAttitude::Type ALeePlayerBotController::GetTeamAttitudeTowards(const AActor& Other) const
{
	FGenericTeamId OtherTeamId = FGenericTeamId::NoTeam;

	if (const APawn* OtherPawn = Cast<APawn>(&Other))
	{
		// 1순위: 상대 컨트롤러에서 팀 조회 (AI 봇 등 컨트롤러가 팀을 들고 있는 경우)
		if (const ILeeTeamAgentInterface* ControllerTeam = Cast<ILeeTeamAgentInterface>(OtherPawn->GetController()))
		{
			OtherTeamId = ControllerTeam->GetGenericTeamId();
		}

		// 2순위: 상대 PlayerState에서 팀 조회 (플레이어는 팀 정보가 PlayerState에 있음)
		if (OtherTeamId == FGenericTeamId::NoTeam)
		{
			if (const ILeeTeamAgentInterface* PlayerStateTeam = Cast<ILeeTeamAgentInterface>(OtherPawn->GetPlayerState()))
			{
				OtherTeamId = PlayerStateTeam->GetGenericTeamId();
			}
		}

		// 3순위: 상대 폰 자체에서 팀 조회 (PlayerState 없이 AI가 조종하는 Enemy 등)
		if (OtherTeamId == FGenericTeamId::NoTeam)
		{
			if (const IGenericTeamAgentInterface* PawnTeam = Cast<IGenericTeamAgentInterface>(OtherPawn))
			{
				OtherTeamId = PawnTeam->GetGenericTeamId();
			}
		}
	}
	// 폰이 아닌 일반 액터가 팀 인터페이스를 구현한 경우
	else if (const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(&Other))
	{
		OtherTeamId = OtherTeamAgent->GetGenericTeamId();
	}

	// 어느 한쪽이라도 팀이 없으면 중립
	if (OtherTeamId == FGenericTeamId::NoTeam || GetGenericTeamId() == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}

	// 팀 ID가 다르면 적대, 같으면 우호
	return (OtherTeamId.GetId() != GetGenericTeamId().GetId()) ? ETeamAttitude::Hostile : ETeamAttitude::Friendly;
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
