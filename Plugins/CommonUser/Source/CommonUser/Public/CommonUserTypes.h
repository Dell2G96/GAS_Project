// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


#if COMMONUSER_OSSV1

// Online Subsystem(OSS v1) 관련 include 및 전방 선언
#include "OnlineSubsystemTypes.h"
class IOnlineSubsystem;
struct FOnlineError;
using FOnlineErrorType = FOnlineError;
using ELoginStatusType = ELoginStatus::Type;

#else

// Online Services(OSS v2) 관련 include 및 전방 선언
#include "Online/Connectivity.h"
#include "Online/OnlineError.h"
namespace UE::Online
{
    enum class ELoginStatus : uint8;
    enum class EPrivilegeResults : uint32;
    enum class EUserPrivileges : uint8;
    using IAuthPtr = TSharedPtr<class IAuth>;
    using IOnlineServicesPtr = TSharedPtr<class IOnlineServices>;
    template <typename OpType>
    class TOnlineResult;
    struct FAuthLogin;
    struct FConnectionStatusChanged;
    struct FExternalUIShowLoginUI;
    struct FAuthLoginStatusChanged;
    struct FQueryUserPrivilege;
    struct FAccountInfo;
}
using FOnlineErrorType = UE::Online::FOnlineError;
using ELoginStatusType = UE::Online::ELoginStatus;

#endif

#include "CommonUserTypes.generated.h"


/** 온라인 쿼리를 어디서, 어떤 방식으로 실행할지 지정하는 열거형 */
UENUM(BlueprintType)
enum class ECommonUserOnlineContext : uint8
{
    /** 게임 코드에서 호출할 때 사용합니다. 기본 시스템을 사용하지만, 여러 컨텍스트의 결과를 합칠 수 있는 특수 처리가 적용됩니다 */
    Game,

    /** 엔진의 기본 온라인 시스템입니다. 항상 존재하며 Service 또는 Platform 중 하나와 동일합니다 */
    Default,
    
    /** 외부 서비스를 명시적으로 요청합니다. 존재하지 않을 수도 있습니다 */
    Service,

    /** 외부 서비스를 우선 찾고, 없으면 기본값으로 폴백합니다 */
    ServiceOrDefault,
    
    /** 플랫폼 시스템을 명시적으로 요청합니다. 존재하지 않을 수도 있습니다 */
    Platform,

    /** 플랫폼 시스템을 우선 찾고, 없으면 기본값으로 폴백합니다 */
    PlatformOrDefault,

    /** 잘못된 시스템입니다 */
    Invalid
};

/** 특정 사용자의 초기화 상태를 설명하는 열거형 */
UENUM(BlueprintType)
enum class ECommonUserInitializationState : uint8
{
    /** 사용자가 로그인 과정을 시작하지 않았습니다 */
    Unknown,

    /** 플레이어가 로컬 로그인을 통해 사용자 ID를 획득하는 중입니다 */
    DoingInitialLogin,

    /** 플레이어가 네트워크 로그인을 수행 중입니다. 이미 로컬 로그인은 완료된 상태입니다 */
    DoingNetworkLogin,

    /** 플레이어 로그인에 완전히 실패했습니다 */
    FailedtoLogin,

    
    /** 플레이어가 로그인되어 있으며 온라인 기능에 접근할 수 있습니다 */
    LoggedInOnline,

    /** 플레이어가 로컬로는 로그인되어 있지만(게스트 또는 실제 사용자), 온라인 작업은 수행할 수 없습니다 */
    LoggedInLocalOnly,


    /** 잘못된 상태 또는 사용자입니다 */
    Invalid,
};

/** 사용자에게 허용되는 다양한 권한과 기능을 설명하는 열거형 */
UENUM(BlueprintType)
enum class ECommonUserPrivilege : uint8
{
    /** 사용자가 오프라인/온라인을 포함해 게임을 플레이할 수 있는지 여부 */
    CanPlay,

    /** 사용자가 온라인 모드에서 플레이할 수 있는지 여부 */
    CanPlayOnline,

    /** 사용자가 텍스트 채팅을 사용할 수 있는지 여부 */
    CanCommunicateViaTextOnline,

    /** 사용자가 음성 채팅을 사용할 수 있는지 여부 */
    CanCommunicateViaVoiceOnline,

    /** 사용자가 다른 사용자가 생성한 콘텐츠에 접근할 수 있는지 여부 */
    CanUseUserGeneratedContent,

    /** 사용자가 크로스플레이에 참여할 수 있는지 여부 */
    CanUseCrossPlay,

    /** 잘못된 권한입니다(유효한 항목 수이기도 합니다) */
    Invalid_Count              UMETA(Hidden)
};

/** 권한 또는 기능의 일반적인 사용 가능 여부를 나타내는 열거형으로, 여러 소스의 정보를 합칩니다 */
UENUM(BlueprintType)
enum class ECommonUserAvailability : uint8
{
    /** 상태를 전혀 알 수 없으며 조회가 필요합니다 */
    Unknown,

    /** 이 기능을 지금 바로 사용할 수 있습니다 */
    NowAvailable,

    /** 일반적인 로그인 절차가 완료되면 사용 가능할 수도 있습니다 */
    PossiblyAvailable,

    /** 네트워크 연결 같은 이유로 지금은 사용할 수 없지만, 나중에는 가능할 수도 있습니다 */
    CurrentlyUnavailable,

    /** 계정 또는 플랫폼의 강제 제한 때문에 이 세션 동안에는 절대 사용할 수 없습니다 */
    AlwaysUnavailable,

    /** 잘못된 기능입니다 */
    Invalid,
};

/** 사용자가 특정 권한을 사용할 수 있거나 없는 구체적인 이유를 나타내는 열거형 */
UENUM(BlueprintType)
enum class ECommonUserPrivilegeResult : uint8
{
    /** 상태를 알 수 없으며 조회가 필요합니다 */
    Unknown,

    /** 이 권한을 완전히 사용할 수 있습니다 */
    Available,

    /** 사용자가 완전히 로그인하지 않았습니다 */
    UserNotLoggedIn,

    /** 사용자가 게임 또는 콘텐츠를 소유하지 않았습니다 */
    LicenseInvalid,

    /** 이 기능을 사용하려면 게임을 업데이트하거나 패치해야 합니다 */
    VersionOutdated,

    /** 네트워크 연결이 없습니다. 다시 연결하면 해결될 수 있습니다 */
    NetworkConnectionUnavailable,

    /** 보호자 설정 실패 */
    AgeRestricted,

    /** 계정에 필요한 구독 또는 계정 유형이 없습니다 */
    AccountTypeRestricted,

    /** 서비스에 의해 차단된 것 같은 다른 계정/사용자 제한이 있습니다 */
    AccountUseRestricted,

    /** 기타 플랫폼별 실패입니다 */
    PlatformFailure,
};

/** 서로 다른 비동기 작업의 진행 상태를 추적하는 데 사용됩니다 */
enum class ECommonUserAsyncTaskState : uint8
{
    /** 작업이 아직 시작되지 않았습니다 */
    NotStarted,
    /** 작업이 현재 처리 중입니다 */
    InProgress,
    /** 작업이 성공적으로 완료되었습니다 */
    Done,
    /** 작업 완료에 실패했습니다 */
    Failed
};

/** 온라인 오류에 대한 상세 정보입니다. 사실상 FOnlineError의 래퍼입니다. */
USTRUCT(BlueprintType)
struct FOnlineResultInformation
{
    GENERATED_BODY()

    /** 작업이 성공했는지 여부입니다. 성공했다면 이 구조체의 오류 필드에는 추가 정보가 들어있지 않습니다 */
    UPROPERTY(BlueprintReadOnly)
    bool bWasSuccessful = true;

    /** 고유 오류 ID입니다. 특정 처리된 오류와 비교하는 데 사용할 수 있습니다 */
    UPROPERTY(BlueprintReadOnly)
    FString ErrorId;

    /** 사용자에게 표시할 오류 텍스트입니다 */
    UPROPERTY(BlueprintReadOnly)
    FText ErrorText;

    /**
     * FOnlineErrorType에서 초기화합니다
     * @param InOnlineError 초기화에 사용할 온라인 오류입니다
     */
    void COMMONUSER_API FromOnlineError(const FOnlineErrorType& InOnlineError);
};