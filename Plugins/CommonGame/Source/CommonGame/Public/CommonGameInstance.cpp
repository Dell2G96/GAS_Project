// Fill out your copyright notice in the Description page of Project Settings.


#include "CommonGameInstance.h"

#include "CommonLocalPlayer.h"
#include "CommonSessionSubsystem.h"
#include "CommonUISettings.h"
#include "CommonUserSubsystem.h"
#include "GameUIManagerSubsystem.h"
#include "ICommonUIModule.h"


// #include "LogCommonGame.h"
// #include "Messaging/CommonGameDialog.h"
// #include "Messaging/CommonMessagingSubsystem.h"


UCommonGameInstance::UCommonGameInstance(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	
}

void UCommonGameInstance::HandleSystemMessage(FGameplayTag MessageType, FText Title, FText Message)
{
	/** 첫 번째 플레이어의 오류 다이얼로그로 심각한 메시지를 전달합니다 */
	ULocalPlayer* FirstPlayer = GetFirstGamePlayer();
	// 첫 번째 플레이어의 오류 다이얼로그로 심각한 메시지를 전달합니다
	//TODO : 수정해야한다
	
	// if (FirstPlayer && MessageType.MatchesTag(FCommonUserTags::SystemMessage_Error))
	// {
	// 	if (UCommonMessagingSubsystem* Messaging = FirstPlayer->GetSubsystem<UCommonMessagingSubsystem>())
	// 	{
	// 		Messaging->ShowError(UCommonGameDialogDescriptor::CreateConfirmationOk(Title, Message));
	// 	}
	// }
}

void UCommonGameInstance::HandlePrivilegeChanged(const UCommonUserInfo* UserInfo, ECommonUserPrivilege Privilege,
	ECommonUserAvailability OldAvailability, ECommonUserAvailability NewAvailability)
{
	/** 첫 번째 플레이어의 플레이 권한이 손실되면 오류를 표시하고 연결을 끊습니다 */
	// 기본적으로 첫 번째 플레이어의 플레이 권한이 손실되면 오류를 표시하고 연결을 끊습니다
	if (Privilege == ECommonUserPrivilege::CanPlay && OldAvailability == ECommonUserAvailability::NowAvailable && NewAvailability != ECommonUserAvailability::NowAvailable)
	{
		// UE_LOG(Log, Error, TEXT("HandlePrivilegeChanged: 플레이어 %d는 더 이상 게임을 플레이할 권한이 없습니다!"), UserInfo->LocalPlayerIndex);
		// TODO: 하위 클래스에서 게임별로 특정 작업을 수행할 수 있습니다
		// ReturnToMainMenu();
	}
}


void UCommonGameInstance::HandlerUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error,
	ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
	// 자식 클래스에서 재정의
}


int32 UCommonGameInstance::AddLocalPlayer(class ULocalPlayer* NewPlayer, FPlatformUserId UserId)
{
	int32 ReturnValue = Super::AddLocalPlayer(NewPlayer, UserId);
	if (ReturnValue != INDEX_NONE)
	{
		if (!PrimaryPlayer.IsValid())
		{
			PrimaryPlayer = NewPlayer;
		}

		// GameUIManagerSubsystem을 통해 NotifyPlayerAdded() 호출로 GameLayoyt을 추가
		GetSubsystem<UGameUIManagerSubsystem>()->NotifyPlayerAdd(Cast<UCommonLocalPlayer>(NewPlayer));

	}
	return ReturnValue;
}

bool UCommonGameInstance::RemoveLocalPlayer(ULocalPlayer* ExistingPlayer)
{
	if (PrimaryPlayer == ExistingPlayer)
	{
		PrimaryPlayer.Reset();
	}
	
	GetSubsystem<UGameUIManagerSubsystem>()->NotifyPlayerDestroyed(Cast<UCommonLocalPlayer>(ExistingPlayer));
	return Super::RemoveLocalPlayer(ExistingPlayer);
}

void UCommonGameInstance::Init()
{
	Super::Init();

	/** 서브시스템들이 초기화된 후 서로 연결합니다 */
	FGameplayTagContainer PlatformTraits = ICommonUIModule::GetSettings().GetPlatformTraits();

	UCommonUserSubsystem* UserSubsystem = GetSubsystem<UCommonUserSubsystem>();
	if (ensure(UserSubsystem))
	{
		/** 플랫폼 특성 태그를 설정하고 시스템 메시지 및 권한 변경 이벤트를 연결합니다 */
		UserSubsystem->SetTraitTags(PlatformTraits);
		UserSubsystem->OnHandleSystemMessage.AddDynamic(this, &UCommonGameInstance::HandleSystemMessage);
		UserSubsystem->OnUserPrivilegeChanged.AddDynamic(this, &UCommonGameInstance::HandlePrivilegeChanged);
		UserSubsystem->OnUserInitializeComplete.AddDynamic(this, &UCommonGameInstance::HandlerUserInitialized);
	}

	UCommonSessionSubsystem* SessionSubsystem = GetSubsystem<UCommonSessionSubsystem>();
	if (ensure(SessionSubsystem))
	{
		/** 세션 요청 및 파괴 이벤트를 연결합니다 */
		SessionSubsystem->OnUserRequestedSessionEvent.AddUObject(this, &UCommonGameInstance::OnUserRequestedSession);
		SessionSubsystem->OnDestroySessionRequestedEvent.AddUObject(this, &UCommonGameInstance::OnDestroySessionRequested);
	}

}


void UCommonGameInstance::ResetUserAndSessionState()
{
	/** 사용자 및 세션 상태를 초기화합니다 */
	UCommonUserSubsystem* UserSubsystem = GetSubsystem<UCommonUserSubsystem>();
	if (ensure(UserSubsystem))
	{
		UserSubsystem->ResetUserState();
	}

	UCommonSessionSubsystem* SessionSubsystem = GetSubsystem<UCommonSessionSubsystem>();
	if (ensure(SessionSubsystem))
	{
		SessionSubsystem->CleanUpSessions();
	}
	
}

void UCommonGameInstance::ReturnToMainMenu()
{
	/** 메인 메뉴로 돌아갈 때 기본적으로 모든 것을 초기화합니다 */
	ResetUserAndSessionState();

	Super::ReturnToMainMenu();
}




void UCommonGameInstance::OnUserRequestedSession(const FPlatformUserId& PlatformUserId,
	class UCommonSession_SearchResult* InRequestedSession, const FOnlineResultInformation& RequestedSessionResult)
{
	/** 사용자가 세션을 요청했을 때 호출됩니다 */
	if (InRequestedSession)
	{
		SetRequestedSession(InRequestedSession);
	}
	else
	{
		/** 요청된 세션이 실패한 경우 오류 메시지를 표시합니다 */
		HandleSystemMessage(FCommonUserTags::SystemMessage_Error, NSLOCTEXT("CommonGame", "Warning_RequestedSessionFailed", "Requested Session Failed"), RequestedSessionResult.ErrorText);
	}
}

void UCommonGameInstance::OnDestroySessionRequested(const FPlatformUserId& PlatformUserId, const FName& SessionName)
{
	/** 
  * 세션 파괴가 요청되었을 때 호출됩니다
  * 프로젝트가 세션을 파괴하고 전환할 수 있는 올바른 상태인지 확인하세요
  */
	// UE_LOG(LogCommonGame, Verbose, TEXT("[%hs] PlatformUserId:%d, SessionName: %s)"), __FUNCTION__, PlatformUserId.GetInternalId(), *SessionName.ToString());

	ReturnToMainMenu();
}

void UCommonGameInstance::SetRequestedSession(UCommonSession_SearchResult* InRequestedSession)
{
	/** 요청된 세션을 설정합니다 */
	RequestedSession = InRequestedSession;
	if (RequestedSession)
	{
		if (CanJoinRequestedSession())
		{
			JoinRequestedSession();
		}
		else
		{
			ResetGameAndJoinRequestedSession();
		}
	}
}

bool UCommonGameInstance::CanJoinRequestedSession() const
{
	/** 기본 동작은 항상 요청된 세션에 참여할 수 있도록 허용합니다 */
	return true;
}

void UCommonGameInstance::JoinRequestedSession()
{
	/** 요청된 세션에 참여합니다 */
	if (RequestedSession)
	{
		if (ULocalPlayer* const FirstPlayer = GetFirstGamePlayer())
		{
			UCommonSessionSubsystem* SessionSubsystem = GetSubsystem<UCommonSessionSubsystem>();
			if (ensure(SessionSubsystem))
			{
				// 현재 요청된 세션을 처리하므로 요청된 세션을 초기화합니다.
				UCommonSession_SearchResult* LocalRequestedSession = RequestedSession;
				RequestedSession = nullptr;
				SessionSubsystem->JoinSession(FirstPlayer->PlayerController, LocalRequestedSession);
			}
		}
	}
}

void UCommonGameInstance::ResetGameAndJoinRequestedSession()
{
	/** 기본 동작은 메인 메뉴로 돌아가는 것입니다. 게임이 준비 상태일 때 JoinRequestedSession을 호출해야 합니다. */
	ReturnToMainMenu();
}






