// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CommonGameInstance.generated.h"

enum class ECommonUserAvailability : uint8;
enum class ECommonUserPrivilege : uint8;

class FText;
class UCommonUserInfo;
class UCommonSession_SearchResult;
struct FOnlineResultInformation;
class ULocalPlayer;
class USocialManager;
class UObject;
struct FFrame;
struct FGameplayTag;

UCLASS(Abstract, Config=Game)
class COMMONGAME_API UCommonGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	/** CommonUser에서 오는 오류/경고를 처리합니다. 게임별로 재정의할 수 있습니다 */
	UFUNCTION()
	virtual void HandleSystemMessage(FGameplayTag MessageType, FText Title, FText Message);

	UFUNCTION()
	 virtual void HandlePrivilegeChanged(const UCommonUserInfo* UserInfo, ECommonUserPrivilege Privilege, ECommonUserAvailability OldAvailability, ECommonUserAvailability NewAvailability);

	UFUNCTION()
	 virtual void HandlerUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext);


	/** 보통 플레이어가 연결이 끊겼을 때 사용자 및 세션 상태를 초기화하는 호출입니다 */
	 virtual void ResetUserAndSessionState();

	/**
	 * 요청된 세션 흐름
	 *   어떤 요청이 특정 세션에 참여하도록 사용자에게 요구합니다(예: OnUserRequestedSession을 통한 플랫폼 오버레이).
	 *   이 요청은 SetRequestedSession에서 처리됩니다.
	 *   요청된 세션에 즉시 참여할 수 있는지 확인합니다(CanJoinRequestedSession). 가능하면 요청된 세션에 참여합니다(JoinRequestedSession)
	 *   그렇지 않으면 요청된 세션을 캐시하고, 세션에 참여할 수 있는 상태가 되도록 게임에 지시합니다(ResetGameAndJoinRequestedSession)
	 */
	/** 외부 소스(예: 플랫폼 오버레이)에서 세션 초대를 수락했을 때 처리합니다. 게임별로 재정의하도록 의도되었습니다 */
	 virtual void OnUserRequestedSession(const FPlatformUserId& PlatformUserId, class UCommonSession_SearchResult* InRequestedSession, const FOnlineResultInformation& RequestedSessionResult);

	/** OSS가 세션 파괴를 요청했을 때 처리합니다 */
	 virtual void OnDestroySessionRequested(const FPlatformUserId& PlatformUserId, const FName& SessionName);

	/** 요청된 세션을 가져옵니다 */
	UCommonSession_SearchResult* GetRequestedSession() const { return RequestedSession; }
	/** 요청된 세션을 설정(또는 해제)합니다. 이 값이 설정되면 요청된 세션 흐름이 시작됩니다 */
	 virtual void SetRequestedSession(UCommonSession_SearchResult* InRequestedSession);
	/** 요청된 세션에 참여할 수 있는지 확인합니다. 게임별로 재정의할 수 있습니다 */
	 virtual bool CanJoinRequestedSession() const;
	/** 요청된 세션에 참여합니다 */
	 virtual void JoinRequestedSession();
	/** 요청된 세션에 참여할 수 있도록 게임을 해당 상태로 되돌립니다 */
	 virtual void ResetGameAndJoinRequestedSession();
	
	
	UCommonGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual int32 AddLocalPlayer(class ULocalPlayer* NewPlayer, FPlatformUserId UserId) override;
	virtual bool RemoveLocalPlayer(ULocalPlayer* ExistingPlayer) override;

	 virtual void Init() override;
	 virtual void ReturnToMainMenu() override;

private:
	/** 주 플레이어입니다 */
	TWeakObjectPtr<class ULocalPlayer> PrimaryPlayer;

	/** 플레이어가 참여 요청한 세션입니다 */
	UPROPERTY()
	TObjectPtr<UCommonSession_SearchResult> RequestedSession;
};
