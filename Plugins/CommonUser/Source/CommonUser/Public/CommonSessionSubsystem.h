// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonUserTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/ObjectPtr.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/PrimaryAssetId.h"
#include "UObject/WeakObjectPtr.h"
#include "PartyBeaconClient.h"
#include "PartyBeaconHost.h"
#include "PartyBeaconState.h"
#if! COMMONUSER_OSSV1
#include "Online/Sessions.h"
#endif



class APlayerController;
class AOnlineBeaconHost;
class ULocalPlayer;
namespace ETravelFailure { enum Type : int; }
struct FOnlineResultInformation;

#if COMMONUSER_OSSV1
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#else
#include "Online/Lobbies.h"
#include "Online/OnlineAsyncOpHandle.h"
#endif // COMMONUSER_OSSV1

#include "CommonSessionSubsystem.generated.h"

class UWorld;
class FCommonSession_OnlineSessionSettings;

#if COMMONUSER_OSSV1
class FCommonOnlineSearchSettingsOSSv1;
using FCommonOnlineSearchSettings = FCommonOnlineSearchSettingsOSSv1;
#else
class FCommonOnlineSearchSettingsOSSv2;
using FCommonOnlineSearchSettings = FCommonOnlineSearchSettingsOSSv2;
#endif // COMMONUSER_OSSV1


//////////////////////////////////////////////////////////////////////
// UCommonSession_HostSessionRequest

/** 게임 세션에서 사용할 온라인 기능과 연결 방식을 지정합니다 */
UENUM(BlueprintType)
enum class ECommonSessionOnlineMode : uint8
{
    Offline,
    LAN,
    Online
};

/** 게임플레이 세션 호스팅 시 사용할 파라미터를 저장하는 요청 객체입니다 */
UCLASS(MinimalAPI, BlueprintType)
class UCommonSession_HostSessionRequest : public UObject
{
    GENERATED_BODY()

public:
    /** 세션이 완전한 온라인 세션인지, 아니면 다른 유형인지를 나타냅니다 */
    UPROPERTY(BlueprintReadWrite, Category=Session)
    ECommonSessionOnlineMode OnlineMode;

    /** 가능하다면 플레이어 호스팅 로비를 생성할지 여부입니다 */
    UPROPERTY(BlueprintReadWrite, Category = Session)
    bool bUseLobbies;

    /** 사용 가능한 경우 음성 채팅이 활성화된 로비를 생성할지 여부입니다 */
    UPROPERTY(BlueprintReadWrite, Category = Session)
    bool bUseLobbiesVoiceChat;

    /** 세션이 사용자의 존재 정보에 표시되도록 할지 여부입니다 */
    UPROPERTY(BlueprintReadWrite, Category = Session)
    bool bUsePresence;

    /** 매치메이킹 중 이 게임 모드가 어떤 종류인지 지정하는 문자열입니다 */
    UPROPERTY(BlueprintReadWrite, Category=Session)
    FString ModeNameForAdvertisement;

    /** 게임플레이 시작 시 로드될 맵입니다. 유효한 Primary Asset 최상위 맵이어야 합니다 */
    UPROPERTY(BlueprintReadWrite, Category=Session, meta=(AllowedTypes="World"))
    FPrimaryAssetId MapID;

    /** 게임의 URL 옵션으로 전달될 추가 인수입니다 */
    UPROPERTY(BlueprintReadWrite, Category=Session)
    TMap<FString, FString> ExtraArgs;

    /** 게임 세션당 허용되는 최대 플레이어 수입니다 */
    UPROPERTY(BlueprintReadWrite, Category=Session)
    int32 MaxPlayerCount = 16;

public:
    /** 실제로 사용해야 할 최대 플레이어 수를 반환합니다. 자식 클래스에서 재정의할 수 있습니다 */
    COMMONUSER_API virtual int32 GetMaxPlayers() const;

    /** 게임플레이 중 사용될 전체 맵 이름을 반환합니다 */
    COMMONUSER_API virtual FString GetMapName() const;

    /** ServerTravel에 전달될 전체 URL을 구성합니다 */
    COMMONUSER_API virtual FString ConstructTravelURL() const;

    /** 이 요청이 유효하면 true를 반환하고, 유효하지 않으면 false를 반환하며 오류를 기록합니다 */
    COMMONUSER_API virtual bool ValidateAndLogErrors(FText& OutError) const;
};


//////////////////////////////////////////////////////////////////////
// UCommonSession_SearchResult

/** 온라인 시스템에서 반환된, 참가 가능한 게임 세션을 나타내는 결과 객체입니다 */
UCLASS(MinimalAPI, BlueprintType)
class UCommonSession_SearchResult : public UObject
{
    GENERATED_BODY()

public:
    /** 사람이 읽는 용도가 아닌, 세션의 내부 설명을 반환합니다 */
    UFUNCTION(BlueprintCallable, Category=Session)
    COMMONUSER_API FString GetDescription() const;

    /** 임의의 문자열 설정을 가져옵니다. 설정이 없으면 bFoundValue는 false가 됩니다 */
    UFUNCTION(BlueprintPure, Category=Sessions)
    COMMONUSER_API void GetStringSetting(FName Key, FString& Value, bool& bFoundValue) const;

    /** 임의의 정수 설정을 가져옵니다. 설정이 없으면 bFoundValue는 false가 됩니다 */
    UFUNCTION(BlueprintPure, Category = Sessions)
    COMMONUSER_API void GetIntSetting(FName Key, int32& Value, bool& bFoundValue) const;

    /** 사용 가능한 비공개 연결 수입니다 */
    UFUNCTION(BlueprintPure, Category=Sessions)
    COMMONUSER_API int32 GetNumOpenPrivateConnections() const;

    /** 사용 가능한 공개 연결 수입니다 */
    UFUNCTION(BlueprintPure, Category=Sessions)
    COMMONUSER_API int32 GetNumOpenPublicConnections() const;

    /** 이미 채워진 연결을 포함하여, 가능한 최대 공개 연결 수입니다 */
    UFUNCTION(BlueprintPure, Category = Sessions)
    COMMONUSER_API int32 GetMaxPublicConnections() const;

    /** 검색 결과에 대한 핑입니다. MAX_QUERY_PING이면 도달 불가 상태입니다 */
    UFUNCTION(BlueprintPure, Category=Sessions)
    COMMONUSER_API int32 GetPingInMs() const;

public:
    /** 플랫폼별 구현체에 대한 포인터입니다 */
#if COMMONUSER_OSSV1
    FOnlineSessionSearchResult Result;
#else
    TSharedPtr<const UE::Online::FLobby> Lobby;

    UE::Online::FOnlineSessionId SessionID;
#endif // COMMONUSER_OSSV1

};


//////////////////////////////////////////////////////////////////////
// UCommonSession_SearchSessionRequest

/** 세션 검색이 완료되었을 때 호출되는 델리게이트입니다 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FCommonSession_FindSessionsFinished, bool bSucceeded, const FText& ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCommonSession_FindSessionsFinishedDynamic, bool, bSucceeded, FText, ErrorMessage);

/** 세션 검색을 설명하는 요청 객체입니다. 검색이 완료되면 이 객체가 업데이트됩니다 */
UCLASS(MinimalAPI, BlueprintType)
class UCommonSession_SearchSessionRequest : public UObject
{
    GENERATED_BODY()

public:
    /** 전체 온라인 게임을 찾는지, 아니면 LAN 같은 다른 유형을 찾는지 지정합니다 */
    UPROPERTY(BlueprintReadWrite, Category = Session)
    ECommonSessionOnlineMode OnlineMode;

    /** 사용 가능한 경우 플레이어 호스팅 로비를 찾을지 여부입니다. false이면 등록된 서버 세션만 검색합니다 */
    UPROPERTY(BlueprintReadWrite, Category = Session)
    bool bUseLobbies;

    /** 발견된 모든 세션 목록입니다. OnSearchFinished가 호출되면 유효합니다 */
    UPROPERTY(BlueprintReadOnly, Category=Session)
    TArray<TObjectPtr<UCommonSession_SearchResult>> Results;

    /** 세션 검색이 완료되었을 때 호출되는 네이티브 델리게이트입니다 */
    FCommonSession_FindSessionsFinished OnSearchFinished;

    /** 서브시스템이 완료 델리게이트를 실행할 때 사용합니다 */
    COMMONUSER_API void NotifySearchFinished(bool bSucceeded, const FText& ErrorMessage);

private:
    /** 세션 검색이 완료되었을 때 호출되는 델리게이트입니다 */
    UPROPERTY(BlueprintAssignable, Category = "Events", meta = (DisplayName = "On Search Finished", AllowPrivateAccess = true))
    FCommonSession_FindSessionsFinishedDynamic K2_OnSearchFinished;
};


//////////////////////////////////////////////////////////////////////
// CommonSessionSubsystem Events

/**
 * 로컬 사용자가 플랫폼 오버레이 같은 외부 소스에서 세션 참가를 요청했을 때 발생하는 이벤트입니다.
 * 일반적으로 게임은 플레이어를 세션으로 전환해야 합니다.
 * @param LocalPlatformUserId 초대를 수락한 로컬 사용자 ID입니다. 사용자가 아직 로그인하지 않았을 수 있으므로 플랫폼 사용자 ID를 사용합니다.
 * @param RequestedSession 요청된 세션입니다. 요청 처리 중 오류가 있으면 null일 수 있습니다.
 * @param RequestedSessionResult 요청된 세션 처리 결과입니다
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FCommonSessionOnUserRequestedSession, const FPlatformUserId& /*LocalPlatformUserId*/, UCommonSession_SearchResult* /*RequestedSession*/, const FOnlineResultInformation& /*RequestedSessionResult*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCommonSessionOnUserRequestedSession_Dynamic, const FPlatformUserId&, LocalPlatformUserId, UCommonSession_SearchResult*, RequestedSession, const FOnlineResultInformation&, RequestedSessionResult);

/**
 * 세션 참가가 완료되었을 때 발생하는 이벤트입니다. 내부 세션에 참가한 뒤, 성공했다면 서버로 이동하기 전에 호출됩니다.
 * 이벤트 매개변수는 성공 여부 또는 이동을 막는 오류인지 여부를 나타냅니다.
 * @param Result 세션 참가 결과입니다
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FCommonSessionOnJoinSessionComplete, const FOnlineResultInformation& /*Result*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCommonSessionOnJoinSessionComplete_Dynamic, const FOnlineResultInformation&, Result);

/**
 * 호스팅을 위한 세션 생성이 완료되었을 때 발생하는 이벤트입니다. 맵으로 이동하기 직전에 호출됩니다.
 * 이벤트 매개변수는 성공 여부 또는 이동을 막는 오류인지 여부를 나타냅니다.
 * @param Result 세션 참가 결과입니다
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FCommonSessionOnCreateSessionComplete, const FOnlineResultInformation& /*Result*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCommonSessionOnCreateSessionComplete_Dynamic, const FOnlineResultInformation&, Result);

/**
 * 로컬 사용자가 플랫폼 오버레이 같은 외부 소스에서 세션 삭제를 요청했을 때 발생하는 이벤트입니다.
 * 게임은 플레이어를 세션 밖으로 전환해야 합니다.
 * @param LocalPlatformUserId 삭제 요청을 만든 로컬 사용자 ID입니다. 사용자가 아직 로그인하지 않았을 수 있으므로 플랫폼 사용자 ID를 사용합니다.
 * @param SessionName 세션의 이름 식별자입니다.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FCommonSessionOnDestroySessionRequested, const FPlatformUserId& /*LocalPlatformUserId*/, const FName& /*SessionName*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCommonSessionOnDestroySessionRequested_Dynamic, const FPlatformUserId&, LocalPlatformUserId, const FName&, SessionName);

/**
 * 세션 참가가 완료된 뒤, 연결 문자열을 해석하고 클라이언트가 이동하기 전에 발생하는 이벤트입니다.
 * @param URL 추가 인수를 포함한 세션의 해석된 연결 문자열입니다
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FCommonSessionOnPreClientTravel, FString& /*URL*/);

/**
 * 세션 생태계의 여러 지점에서 발생하며, 사용자에게 표시 가능한 세션 상태를 나타내는 이벤트입니다.
 * 이것은 온라인 기능용으로 사용하면 안 되며(OnCreateSessionComplete 또는 OnJoinSessionComplete를 사용), rich presence 같은 기능에 사용해야 합니다
 */
UENUM(BlueprintType)
enum class ECommonSessionInformationState : uint8
{
    OutOfGame,
    Matchmaking,
    InGame
};
DECLARE_MULTICAST_DELEGATE_ThreeParams(FCommonSessionOnSessionInformationChanged, ECommonSessionInformationState /*SessionStatus*/, const FString& /*GameMode*/, const FString& /*MapName*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCommonSessionOnSessionInformationChanged_Dynamic, ECommonSessionInformationState, SessionStatus, const FString&, GameMode, const FString&, MapName);

//////////////////////////////////////////////////////////////////////
// UCommonSessionSubsystem

/** 
 * 온라인 게임의 호스팅과 참여 요청을 처리하는 게임 서브시스템입니다.
 * 게임 인스턴스마다 하나씩 생성되며, 블루프린트나 C++ 코드에서 접근할 수 있습니다.
 * 게임 전용 하위 클래스가 존재하면 이 기본 서브시스템은 생성되지 않습니다.
 */
UCLASS(MinimalAPI, BlueprintType, Config=Engine)
class UCommonSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UCommonSessionSubsystem() { }

    COMMONUSER_API virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    COMMONUSER_API virtual void Deinitialize() override;
    COMMONUSER_API virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    /** 기본 옵션으로 온라인 게임용 호스트 세션 요청을 생성합니다. 생성 후 수정할 수 있습니다 */
    UFUNCTION(BlueprintCallable, Category = Session)
    COMMONUSER_API virtual UCommonSession_HostSessionRequest* CreateOnlineHostSessionRequest();

    /** 기본 온라인 게임을 찾기 위한 기본 옵션으로 세션 검색 객체를 생성합니다. 생성 후 수정할 수 있습니다 */
    UFUNCTION(BlueprintCallable, Category = Session)
    COMMONUSER_API virtual UCommonSession_SearchSessionRequest* CreateOnlineSearchSessionRequest();

    /** 세션 요청 정보로 새 온라인 게임을 생성합니다. 성공하면 강제 맵 전환이 시작됩니다 */
    UFUNCTION(BlueprintCallable, Category=Session)
    COMMONUSER_API virtual void HostSession(APlayerController* HostingPlayer, UCommonSession_HostSessionRequest* Request);

    /** 기존 세션을 찾거나, 적절한 세션이 없으면 새로 생성하는 과정을 시작합니다 */
    UFUNCTION(BlueprintCallable, Category=Session)
    COMMONUSER_API virtual void QuickPlaySession(APlayerController* JoiningOrHostingPlayer, UCommonSession_HostSessionRequest* Request);

    /** 기존 세션에 참여하는 과정을 시작합니다. 성공하면 지정된 서버에 연결됩니다 */
    UFUNCTION(BlueprintCallable, Category=Session)
    COMMONUSER_API virtual void JoinSession(APlayerController* JoiningPlayer, UCommonSession_SearchResult* Request);

    /** 검색 요청과 일치하는 참가 가능한 세션 목록을 온라인 시스템에 질의합니다 */
    UFUNCTION(BlueprintCallable, Category=Session)
    COMMONUSER_API virtual void FindSessions(APlayerController* SearchingPlayer, UCommonSession_SearchSessionRequest* Request);

    /** 활성 세션이 있으면 정리합니다. 메인 메뉴로 돌아가는 경우 등에 호출됩니다 */
    UFUNCTION(BlueprintCallable, Category=Session)
    COMMONUSER_API virtual void CleanUpSessions();

    //////////////////////////////////////////////////////////////////////
    // 이벤트

    /** 로컬 사용자가 초대를 수락했을 때의 네이티브 델리게이트 */
    FCommonSessionOnUserRequestedSession OnUserRequestedSessionEvent;
    /** 로컬 사용자가 초대를 수락했을 때 브로드캐스트되는 이벤트 */
    UPROPERTY(BlueprintAssignable, Category = "Events", meta = (DisplayName = "On User Requested Session"))
    FCommonSessionOnUserRequestedSession_Dynamic K2_OnUserRequestedSessionEvent;

    /** JoinSession 호출이 완료되었을 때의 네이티브 델리게이트 */
    FCommonSessionOnJoinSessionComplete OnJoinSessionCompleteEvent;
    /** JoinSession 호출이 완료되었을 때 브로드캐스트되는 이벤트 */
    UPROPERTY(BlueprintAssignable, Category = "Events", meta = (DisplayName = "On Join Session Complete"))
    FCommonSessionOnJoinSessionComplete_Dynamic K2_OnJoinSessionCompleteEvent;

    /** CreateSession 호출이 완료되었을 때의 네이티브 델리게이트 */
    FCommonSessionOnCreateSessionComplete OnCreateSessionCompleteEvent;
    /** CreateSession 호출이 완료되었을 때 브로드캐스트되는 이벤트 */
    UPROPERTY(BlueprintAssignable, Category = "Events", meta = (DisplayName = "On Create Session Complete"))
    FCommonSessionOnCreateSessionComplete_Dynamic K2_OnCreateSessionCompleteEvent;

    /** 표시용 세션 정보가 변경되었을 때의 네이티브 델리게이트 */
    FCommonSessionOnSessionInformationChanged OnSessionInformationChangedEvent;
    /** 표시용 세션 정보가 변경되었을 때 브로드캐스트되는 이벤트 */
    UPROPERTY(BlueprintAssignable, Category = "Events", meta = (DisplayName = "On Session Information Changed"))
    FCommonSessionOnSessionInformationChanged_Dynamic K2_OnSessionInformationChangedEvent;

    /** 플랫폼 세션 삭제가 요청되었을 때의 네이티브 델리게이트 */
    FCommonSessionOnDestroySessionRequested OnDestroySessionRequestedEvent;
    /** 플랫폼 세션 삭제가 요청되었을 때 브로드캐스트되는 이벤트 */
    UPROPERTY(BlueprintAssignable, Category = "Events", meta = (DisplayName = "On Leave Session Requested"))
    FCommonSessionOnDestroySessionRequested_Dynamic K2_OnDestroySessionRequestedEvent;

    /** 클라이언트 이동 전에 연결 URL을 수정하는 네이티브 델리게이트 */
    FCommonSessionOnPreClientTravel OnPreClientTravelEvent;

    // 구성 설정입니다. 자식 클래스나 설정 파일에서 덮어쓸 수 있습니다

    /** 세션 검색 및 호스트 요청에서 bUseLobbies의 기본값을 설정합니다 */
    UPROPERTY(Config)
    bool bUseLobbiesDefault = true;

    /** 세션 호스트 요청에서 bUseLobbiesVoiceChat의 기본값을 설정합니다 */
    UPROPERTY(Config)
    bool bUseLobbiesVoiceChatDefault = false;

    /** 게임 세션을 생성하거나 참여할 때 서버 이동 전에 예약 비콘 흐름을 활성화합니다 */ 
    UPROPERTY(Config)
    bool bUseBeacons = true;

protected:
    // 세션을 생성하거나 참여하는 과정에서 호출되는 함수입니다. 게임별 동작으로 재정의할 수 있습니다

    /** 퀵 플레이 호스트 설정에서 세션 요청을 채우는 데 호출됩니다. 게임별 동작으로 재정의할 수 있습니다 */
    COMMONUSER_API virtual TSharedRef<FCommonOnlineSearchSettings> CreateQuickPlaySearchSettings(UCommonSession_HostSessionRequest* Request, UCommonSession_SearchSessionRequest* QuickPlayRequest);

    /** 퀵 플레이 검색이 완료되었을 때 호출됩니다. 게임별 동작으로 재정의할 수 있습니다 */
    COMMONUSER_API virtual void HandleQuickPlaySearchFinished(bool bSucceeded, const FText& ErrorMessage, TWeakObjectPtr<APlayerController> JoiningOrHostingPlayer, TStrongObjectPtr<UCommonSession_HostSessionRequest> HostRequest);

    /** 세션으로 이동하는 데 실패했을 때 호출됩니다 */
    COMMONUSER_API virtual void TravelLocalSessionFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ReasonString);

    /** 새 세션이 생성되었거나 생성에 실패했을 때 호출됩니다 */
    COMMONUSER_API virtual void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

    /** 세션 생성을 마무리할 때 호출됩니다 */
    COMMONUSER_API virtual void FinishSessionCreation(bool bWasSuccessful);

    /** 새로 호스팅된 세션 맵으로 이동한 뒤 호출됩니다 */
    COMMONUSER_API virtual void HandlePostLoadMap(UWorld* World);

protected:
    // 온라인 시스템에서 생성되거나 결과를 처리하는 동안 호출되는 내부 함수들입니다

    COMMONUSER_API void BindOnlineDelegates();
    COMMONUSER_API void CreateOnlineSessionInternal(ULocalPlayer* LocalPlayer, UCommonSession_HostSessionRequest* Request);
    COMMONUSER_API void FindSessionsInternal(APlayerController* SearchingPlayer, const TSharedRef<FCommonOnlineSearchSettings>& InSearchSettings);
    COMMONUSER_API void JoinSessionInternal(ULocalPlayer* LocalPlayer, UCommonSession_SearchResult* Request);
    COMMONUSER_API void InternalTravelToSession(const FName SessionName);
    COMMONUSER_API void NotifyUserRequestedSession(const FPlatformUserId& PlatformUserId, UCommonSession_SearchResult* RequestedSession, const FOnlineResultInformation& RequestedSessionResult);
    COMMONUSER_API void NotifyJoinSessionComplete(const FOnlineResultInformation& Result);
    COMMONUSER_API void NotifyCreateSessionComplete(const FOnlineResultInformation& Result);
    COMMONUSER_API void NotifySessionInformationUpdated(ECommonSessionInformationState SessionStatusStr, const FString& GameMode = FString(), const FString& MapName = FString());
    COMMONUSER_API void NotifyDestroySessionRequested(const FPlatformUserId& PlatformUserId, const FName& SessionName);
    COMMONUSER_API void SetCreateSessionError(const FText& ErrorText);

#if COMMONUSER_OSSV1
    COMMONUSER_API void BindOnlineDelegatesOSSv1();
    COMMONUSER_API void CreateOnlineSessionInternalOSSv1(ULocalPlayer* LocalPlayer, UCommonSession_HostSessionRequest* Request);
    COMMONUSER_API void FindSessionsInternalOSSv1(ULocalPlayer* LocalPlayer);
    COMMONUSER_API void JoinSessionInternalOSSv1(ULocalPlayer* LocalPlayer, UCommonSession_SearchResult* Request);
    COMMONUSER_API TSharedRef<FCommonOnlineSearchSettings> CreateQuickPlaySearchSettingsOSSv1(UCommonSession_HostSessionRequest* Request, UCommonSession_SearchSessionRequest* QuickPlayRequest);
    COMMONUSER_API void CleanUpSessionsOSSv1();

    COMMONUSER_API void HandleSessionFailure(const FUniqueNetId& NetId, ESessionFailure::Type FailureType);
    COMMONUSER_API void HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 LocalUserIndex, FUniqueNetIdPtr AcceptingUserId, const FOnlineSessionSearchResult& SearchResult);
    COMMONUSER_API void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);
    COMMONUSER_API void OnRegisterLocalPlayerComplete_CreateSession(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result);
    COMMONUSER_API void OnUpdateSessionComplete(FName SessionName, bool bWasSuccessful);
    COMMONUSER_API void OnEndSessionComplete(FName SessionName, bool bWasSuccessful);
    COMMONUSER_API void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
    COMMONUSER_API void OnDestroySessionRequested(int32 LocalUserNum, FName SessionName);
    COMMONUSER_API void OnFindSessionsComplete(bool bWasSuccessful);
    COMMONUSER_API void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    COMMONUSER_API void OnRegisterJoiningLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result);
    COMMONUSER_API void FinishJoinSession(EOnJoinSessionCompleteResult::Type Result);

#else
    COMMONUSER_API void BindOnlineDelegatesOSSv2();
    COMMONUSER_API void CreateOnlineSessionInternalOSSv2(ULocalPlayer* LocalPlayer, UCommonSession_HostSessionRequest* Request);
    COMMONUSER_API void FindSessionsInternalOSSv2(ULocalPlayer* LocalPlayer);
    COMMONUSER_API void JoinSessionInternalOSSv2(ULocalPlayer* LocalPlayer, UCommonSession_SearchResult* Request);
    COMMONUSER_API TSharedRef<FCommonOnlineSearchSettings> CreateQuickPlaySearchSettingsOSSv2(UCommonSession_HostSessionRequest* HostRequest, UCommonSession_SearchSessionRequest* SearchRequest);
    COMMONUSER_API void CleanUpSessionsOSSv2();

    /** 온라인 서비스에서 시작된 참가 요청을 처리합니다 */
    COMMONUSER_API void OnLobbyJoinRequested(const UE::Online::FUILobbyJoinRequested& EventParams);

    /** 온라인 서비스에서 시작된 SESSION 참가 요청을 처리합니다 */
    COMMONUSER_API void OnSessionJoinRequested(const UE::Online::FUISessionJoinRequested& EventParams);

    /** 주어진 컨트롤러의 로컬 사용자 ID를 가져옵니다 */
    COMMONUSER_API UE::Online::FAccountId GetAccountId(APlayerController* PlayerController) const;
    /** 주어진 세션 이름의 로비 ID를 가져옵니다 */
    COMMONUSER_API UE::Online::FLobbyId GetLobbyId(const FName SessionName) const;
    /** UI 로비 참가 요청용 이벤트 핸들 */
    UE::Online::FOnlineEventDelegateHandle LobbyJoinRequestedHandle;

    /** UI 세션 참가 요청용 이벤트 핸들 */
    UE::Online::FOnlineEventDelegateHandle SessionJoinRequestedHandle;

#endif // COMMONUSER_OSSV1

    COMMONUSER_API void CreateHostReservationBeacon();
    COMMONUSER_API void ConnectToHostReservationBeacon();
    COMMONUSER_API void DestroyHostReservationBeacon();

protected:
    /** 세션 작업이 완료된 후 사용될 이동 URL입니다 */
    FString PendingTravelURL;

    /** 세션 생성 시도의 가장 최근 결과 정보입니다. 나중에 오류 코드를 저장할 수 있도록 여기에 보관합니다 */
    FOnlineResultInformation CreateSessionResult;

    /** 세션 생성 후 해당 세션을 취소/파괴하려는 경우 true입니다 */
    bool bWantToDestroyPendingSession = false;

    /** 전용 서버인지 여부입니다. 전용 서버는 세션 생성에 LocalPlayer가 필요하지 않습니다 */
    bool bIsDedicatedServer = false;

    /** 현재 검색 설정입니다 */
    TSharedPtr<FCommonOnlineSearchSettings> SearchSettings;

    /** 비콘 등록용 일반 비콘 리스너입니다 */
    UPROPERTY(Transient)
    TWeakObjectPtr<AOnlineBeaconHost> BeaconHostListener;
    /** 비콘 호스트의 상태입니다 */
    UPROPERTY(Transient)
    TObjectPtr<UPartyBeaconState> ReservationBeaconHostState;
    /** 이 게임에 대한 접근을 제어하는 비콘입니다 */
    UPROPERTY(Transient)
    TWeakObjectPtr<APartyBeaconHost> ReservationBeaconHost;
    /** 비콘 통신을 위한 공통 클래스 객체입니다 */
    UPROPERTY(Transient)
    TWeakObjectPtr<APartyBeaconClient> ReservationBeaconClient;

    /** 비콘 예약용 팀 수입니다 */
    UPROPERTY(Config)
    int32 BeaconTeamCount = 2;
    /** 비콘 예약용 팀의 크기입니다 */
    UPROPERTY(Config)
    int32 BeaconTeamSize = 8;
    /** 비콘 예약의 최대 수입니다 */
    UPROPERTY(Config)
    int32 BeaconMaxReservations = 16;
};