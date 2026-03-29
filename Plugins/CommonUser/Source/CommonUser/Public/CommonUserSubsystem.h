// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonUserTypes.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/OnlineReplStructs.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/WeakObjectPtr.h"
#include "GameplayTagContainer.h"
// #include "Online/Connectivity.h"
// #include "Online/Connectivity.h"
#include "CommonUserSubsystem.generated.h"

#if COMMONUSER_OSSV1
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineError.h"
#else
#endif

class FNativeGameplayTag;
class IOnlineSubsystem;

/** 공통 사용자 서브시스템에서 사용되는 태그 목록 */
struct FCommonUserTags
{
    // 일반적인 심각도 수준과 특정 시스템 메시지

    static COMMONUSER_API FNativeGameplayTag SystemMessage_Error;  // SystemMessage.Error
    static COMMONUSER_API FNativeGameplayTag SystemMessage_Warning; // SystemMessage.Warning
    static COMMONUSER_API FNativeGameplayTag SystemMessage_Display; // SystemMessage.Display

    /** 모든 플레이어 초기화 시도가 실패했습니다. 사용자는 다시 시도하기 전에 무언가 조치를 해야 합니다 */
    static COMMONUSER_API FNativeGameplayTag SystemMessage_Error_InitializeLocalPlayerFailed; // SystemMessage.Error.InitializeLocalPlayerFailed


    // 플랫폼 특성 태그입니다. 게임 인스턴스 또는 다른 시스템이 적절한 플랫폼에 대해 이 태그들로 SetTraitTags를 호출할 것으로 예상됩니다

    /** 이 태그는 컨트롤러 ID가 서로 다른 시스템 사용자에 직접 매핑되는 콘솔 플랫폼을 의미합니다. false이면 동일한 사용자가 여러 컨트롤러를 가질 수 있습니다 */
    static COMMONUSER_API FNativeGameplayTag Platform_Trait_RequiresStrictControllerMapping; // Platform.Trait.RequiresStrictControllerMapping

    /** 이 태그는 플랫폼에 온라인 사용자가 하나만 있고 모든 플레이어가 index 0을 사용한다는 의미입니다 */
    static COMMONUSER_API FNativeGameplayTag Platform_Trait_SingleOnlineUser; // Platform.Trait.SingleOnlineUser
};

/** 개별 사용자의 논리적 표현입니다. 초기화된 모든 로컬 플레이어에 대해 이 객체가 하나씩 존재합니다 */
UCLASS(MinimalAPI, BlueprintType)
class UCommonUserInfo : public UObject
{
    GENERATED_BODY()

public:
    /** 이 사용자의 기본 컨트롤러 입력 장치입니다. 추가 보조 장치를 가질 수도 있습니다 */
    UPROPERTY(BlueprintReadOnly, Category = UserInfo)
    FInputDeviceId PrimaryInputDevice;

    /** 이 사용자의 로컬 플랫폼 상 논리 사용자입니다. 게스트 사용자는 기본 사용자에 연결됩니다 */
    UPROPERTY(BlueprintReadOnly, Category = UserInfo)
    FPlatformUserId PlatformUser;
    
    /** 이 사용자에게 LocalPlayer가 할당되어 있으면, 완전히 생성된 후 GameInstance localplayers 배열의 인덱스와 일치합니다 */
    UPROPERTY(BlueprintReadOnly, Category = UserInfo)
    int32 LocalPlayerIndex = -1;

    /** true이면 이 사용자는 게스트가 될 수 있습니다 */
    UPROPERTY(BlueprintReadOnly, Category = UserInfo)
    bool bCanBeGuest = false;

    /** true이면 이 사용자는 기본 사용자 0에 연결된 게스트 사용자입니다 */
    UPROPERTY(BlueprintReadOnly, Category = UserInfo)
    bool bIsGuest = false;

    /** 사용자의 초기화 과정 전체 상태입니다 */
    UPROPERTY(BlueprintReadOnly, Category = UserInfo)
    ECommonUserInitializationState InitializationState = ECommonUserInitializationState::Invalid;

    /** 이 사용자가 로그인에 성공했으면 true를 반환합니다 */
    UFUNCTION(BlueprintCallable, Category = UserInfo)
    COMMONUSER_API bool IsLoggedIn() const;

    /** 이 사용자가 로그인 중이면 true를 반환합니다 */
    UFUNCTION(BlueprintCallable, Category = UserInfo)
    COMMONUSER_API bool IsDoingLogin() const;

    /** 특정 권한에 대해 가장 최근에 조회한 결과를 반환합니다. 한 번도 조회하지 않았다면 unknown을 반환합니다 */
    UFUNCTION(BlueprintCallable, Category = UserInfo)
    COMMONUSER_API ECommonUserPrivilegeResult GetCachedPrivilegeResult(ECommonUserPrivilege Privilege, ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;

    /** 기능의 일반적인 사용 가능 여부를 묻습니다. 캐시된 결과와 상태를 함께 반영합니다 */
    UFUNCTION(BlueprintCallable, Category = UserInfo)
    COMMONUSER_API ECommonUserAvailability GetPrivilegeAvailability(ECommonUserPrivilege Privilege) const;

    /** 주어진 컨텍스트의 net id를 반환합니다 */
    UFUNCTION(BlueprintCallable, Category = UserInfo)
    COMMONUSER_API FUniqueNetIdRepl GetNetId(ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;

    /** 이 사용자의 사람이 읽을 수 있는 닉네임을 반환합니다. UpdateCachedNetId 또는 SetNickname에서 캐시된 값을 반환합니다 */
    UFUNCTION(BlueprintCallable, Category = UserInfo)
    COMMONUSER_API FString GetNickname(ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;

    /** 이 사용자의 사람이 읽을 수 있는 닉네임을 수정합니다. 여러 게스트를 설정할 때 사용할 수 있지만, 실제 사용자에 대해서는 플랫폼 닉네임으로 덮어써집니다 */
    UFUNCTION(BlueprintCallable, Category = UserInfo)
    COMMONUSER_API void SetNickname(const FString& NewNickname, ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game);

    /** 이 플레이어에 대한 내부 디버그 문자열을 반환합니다 */
    UFUNCTION(BlueprintCallable, Category = UserInfo)
    COMMONUSER_API FString GetDebugString() const;

    /** 플랫폼 사용자 ID에 대한 접근자 */
    COMMONUSER_API FPlatformUserId GetPlatformUserId() const;

    /** 이전 함수에서 정수형을 기대할 때 사용하는 플랫폼 사용자 인덱스를 반환합니다 */
    COMMONUSER_API int32 GetPlatformUserIndex() const;

    // 내부 데이터이며, 온라인 서브시스템에서만 접근하도록 의도되었습니다

    /** 각 온라인 시스템에 대한 캐시 데이터 */
    struct FCachedData
    {
       /** 시스템별 캐시된 net id */
       FUniqueNetIdRepl CachedNetId;

       /** 캐시된 닉네임. net ID가 변경될 수 있을 때마다 갱신됩니다 */
       FString CachedNickname;

       /** 다양한 사용자 권한에 대한 캐시된 값 */
       TMap<ECommonUserPrivilege, ECommonUserPrivilegeResult> CachedPrivileges;
    };

    /** 컨텍스트별 캐시입니다. game은 항상 존재하지만 다른 컨텍스트는 없을 수도 있습니다 */
    TMap<ECommonUserOnlineContext, FCachedData> CachedDataMap;
    
    /** 해결 규칙에 따라 캐시 데이터를 조회합니다 */
    COMMONUSER_API FCachedData* GetCachedData(ECommonUserOnlineContext Context);
    COMMONUSER_API const FCachedData* GetCachedData(ECommonUserOnlineContext Context) const;

    /** 캐시된 권한 결과를 업데이트합니다. 필요하면 game으로 전파됩니다 */
    COMMONUSER_API void UpdateCachedPrivilegeResult(ECommonUserPrivilege Privilege, ECommonUserPrivilegeResult Result, ECommonUserOnlineContext Context);

    /** 캐시된 권한 결과를 업데이트합니다. 필요하면 game으로 전파됩니다 */
    COMMONUSER_API void UpdateCachedNetId(const FUniqueNetIdRepl& NewId, ECommonUserOnlineContext Context);

    /** 이 객체의 소유 서브시스템을 반환합니다 */
    COMMONUSER_API class UCommonUserSubsystem* GetSubsystem() const;
};


/** 초기화 과정이 성공하거나 실패할 때의 델리게이트입니다 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FCommonUserOnInitializeCompleteMulticast, const UCommonUserInfo*, UserInfo, bool, bSuccess, FText, Error, ECommonUserPrivilege, RequestedPrivilege, ECommonUserOnlineContext, OnlineContext);
DECLARE_DYNAMIC_DELEGATE_FiveParams(FCommonUserOnInitializeComplete, const UCommonUserInfo*, UserInfo, bool, bSuccess, FText, Error, ECommonUserPrivilege, RequestedPrivilege, ECommonUserOnlineContext, OnlineContext);

/** 시스템 오류 메시지가 전송될 때의 델리게이트입니다. 게임은 메시지 유형 태그를 이용해 이를 사용자에게 표시할 수 있습니다 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCommonUserHandleSystemMessageDelegate, FGameplayTag, MessageType, FText, TitleText, FText, BodyText);

/** 권한이 변경될 때의 델리게이트입니다. 게임 플레이 중 온라인 상태 등이 바뀌는지 확인하는 데 바인딩할 수 있습니다 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FCommonUserAvailabilityChangedDelegate, const UCommonUserInfo*, UserInfo, ECommonUserPrivilege, Privilege, ECommonUserAvailability, OldAvailability, ECommonUserAvailability, NewAvailability);


/** 초기화 함수용 파라미터 구조체입니다. 일반적으로 async 노드 같은 래퍼 함수에서 채워집니다 */
USTRUCT(BlueprintType)
struct FCommonUserInitializeParams
{
    GENERATED_BODY()
    
    /** 사용할 로컬 플레이어 인덱스입니다. 플레이어 생성이 가능하면 현재보다 큰 값도 지정할 수 있습니다 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Default)
    int32 LocalPlayerIndex = 0;

    /** 플랫폼 사용자와 입력 장치를 선택하던 이전 방식입니다 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Default)
    int32 ControllerId = -1;

    /** 이 사용자의 기본 컨트롤러 입력 장치입니다. 추가 보조 장치를 가질 수도 있습니다 */
    UPROPERTY(BlueprintReadOnly, Category = UserInfo)
    FInputDeviceId PrimaryInputDevice;

    /** 이 사용자의 논리적 플랫폼 사용자입니다 */
    UPROPERTY(BlueprintReadOnly, Category = UserInfo)
    FPlatformUserId PlatformUser;
    
    /** 일반적으로 CanPlay 또는 CanPlayOnline이며, 필요한 권한 수준을 지정합니다 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Default)
    ECommonUserPrivilege RequestedPrivilege = ECommonUserPrivilege::CanPlay;

    /** 어떤 온라인 컨텍스트에 로그인할지 지정합니다. game은 관련된 모든 곳에 로그인한다는 뜻입니다 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Default)
    ECommonUserOnlineContext OnlineContext = ECommonUserOnlineContext::Game;

    /** 초기 로그인 시 새 로컬 플레이어를 생성할 수 있는지 여부입니다 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Default)
    bool bCanCreateNewLocalPlayer = false;

    /** 이 플레이어가 실제 온라인 존재 없이 게스트 사용자일 수 있는지 여부입니다 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Default)
    bool bCanUseGuestLogin = false;

    /** 로그인 오류를 표시하지 않을지 여부입니다. 게임이 직접 오류를 표시합니다 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Default)
    bool bSuppressLoginErrors = false;

    /** 바인딩되어 있으면 완료 시 이 동적 델리게이트를 호출합니다 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Default)
    FCommonUserOnInitializeComplete OnUserInitializeComplete;
};

/**
 * 사용자 신원과 로그인 상태의 질의 및 변경을 처리하는 게임 서브시스템입니다.
 * 게임 인스턴스마다 하나씩 생성되며, 블루프린트나 C++ 코드에서 접근할 수 있습니다.
 * 게임 전용 하위 클래스가 존재하면 이 기본 서브시스템은 생성되지 않습니다.
 */
UCLASS(MinimalAPI, BlueprintType, Config=Engine)
class UCommonUserSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UCommonUserSubsystem() { }

    COMMONUSER_API virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    COMMONUSER_API virtual void Deinitialize() override;
    COMMONUSER_API virtual bool ShouldCreateSubsystem(UObject* Outer) const override;


    /** 요청된 초기화가 완료되면 BP 델리게이트가 호출됩니다 */
    UPROPERTY(BlueprintAssignable, Category = CommonUser)
    FCommonUserOnInitializeCompleteMulticast OnUserInitializeComplete;

    /** 시스템이 오류/경고 메시지를 보낼 때 호출되는 BP 델리게이트입니다 */
    UPROPERTY(BlueprintAssignable, Category = CommonUser)
    FCommonUserHandleSystemMessageDelegate OnHandleSystemMessage;

    /** 사용자에 대한 권한 사용 가능 여부가 변경될 때 호출되는 BP 델리게이트입니다 */
    UPROPERTY(BlueprintAssignable, Category = CommonUser)
    FCommonUserAvailabilityChangedDelegate OnUserPrivilegeChanged;

    /** OnHandleSystemMessage를 통해 시스템 메시지를 보냅니다 */
    UFUNCTION(BlueprintCallable, Category = CommonUser)
    COMMONUSER_API virtual void SendSystemMessage(FGameplayTag MessageType, FText TitleText, FText BodyText);

    /** 최대 로컬 플레이어 수를 설정합니다. 기존 플레이어는 삭제하지 않습니다 */
    UFUNCTION(BlueprintCallable, Category = CommonUser)
    COMMONUSER_API virtual void SetMaxLocalPlayers(int32 InMaxLocalPLayers);

    /** 최대 로컬 플레이어 수를 반환합니다 */
    UFUNCTION(BlueprintPure, Category = CommonUser)
    COMMONUSER_API int32 GetMaxLocalPlayers() const;

    /** 현재 로컬 플레이어 수를 반환합니다. 항상 최소 1 이상입니다 */
    UFUNCTION(BlueprintPure, Category = CommonUser)
    COMMONUSER_API int32 GetNumLocalPlayers() const;

    /** 지정한 로컬 플레이어의 초기화 상태를 반환합니다 */
    UFUNCTION(BlueprintPure, Category = CommonUser)
    COMMONUSER_API ECommonUserInitializationState GetLocalPlayerInitializationState(int32 LocalPlayerIndex) const;

    /** GameInstance의 지정한 로컬 플레이어 인덱스에 대한 사용자 정보를 반환합니다. 실행 중인 게임에서는 0이 항상 유효합니다 */
    UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = CommonUser)
    COMMONUSER_API const UCommonUserInfo* GetUserInfoForLocalPlayerIndex(int32 LocalPlayerIndex) const;

    /** 폐기됨, 가능하면 PlatformUserId를 사용하세요 */
    UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = CommonUser)
    COMMONUSER_API const UCommonUserInfo* GetUserInfoForPlatformUserIndex(int32 PlatformUserIndex) const;

    /** 지정한 플랫폼 사용자 인덱스의 주 사용자 정보를 반환합니다. null을 반환할 수 있습니다 */
    UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = CommonUser)
    COMMONUSER_API const UCommonUserInfo* GetUserInfoForPlatformUser(FPlatformUserId PlatformUser) const;

    /** 유니크 net id에 대한 사용자 정보를 반환합니다. null을 반환할 수 있습니다 */
    UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = CommonUser)
    COMMONUSER_API const UCommonUserInfo* GetUserInfoForUniqueNetId(const FUniqueNetIdRepl& NetId) const;

    /** 폐기됨, 가능하면 InputDeviceId를 사용하세요 */
    UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = CommonUser)
    COMMONUSER_API const UCommonUserInfo* GetUserInfoForControllerId(int32 ControllerId) const;

    /** 지정한 입력 장치에 대한 사용자 정보를 반환합니다. null을 반환할 수 있습니다 */
    UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = CommonUser)
    COMMONUSER_API const UCommonUserInfo* GetUserInfoForInputDevice(FInputDeviceId InputDevice) const;

    /**
     * 로컬 플레이어를 생성하거나 갱신하는 과정을 시작하려고 시도합니다. 로그인과 플레이어 컨트롤러 생성이 포함됩니다.
     * 과정이 성공하거나 실패하면 OnUserInitializeComplete 델리게이트가 방송됩니다.
     *
     * @param LocalPlayerIndex Game Instance에서 원하는 LocalPlayer 인덱스. 0은 주 플레이어, 1 이상은 로컬 멀티플레이어
     * @param PrimaryInputDevice 이 사용자에 매핑할 물리 컨트롤러. 유효하지 않으면 기본 장치를 사용합니다
     * @param bCanUseGuestLogin    true이면 실제 Unique Net Id 없이 게스트로 가능
     *
     * @returns 과정이 시작되면 true, 제대로 시작하기 전에 실패하면 false
     */
    UFUNCTION(BlueprintCallable, Category = CommonUser)
    COMMONUSER_API virtual bool TryToInitializeForLocalPlay(int32 LocalPlayerIndex, FInputDeviceId PrimaryInputDevice, bool bCanUseGuestLogin);

    /**
     * 온라인 권한 확인을 포함한 전체 온라인 로그인을 위해 이미 로컬 로그인된 사용자를 가져오는 과정을 시작합니다.
     * 과정이 성공하거나 실패하면 OnUserInitializeComplete 델리게이트가 방송됩니다.
     *
     * @param LocalPlayerIndex Game Instance에서 기존 LocalPlayer의 인덱스
     *
     * @returns 과정이 시작되면 true, 제대로 시작하기 전에 실패하면 false
     */
    UFUNCTION(BlueprintCallable, Category = CommonUser)
    COMMONUSER_API virtual bool TryToLoginForOnlinePlay(int32 LocalPlayerIndex);

    /**
     * 일반적인 사용자 로그인 및 초기화 과정을 시작합니다. params 구조체를 사용해 어떤 항목에 로그인할지 결정합니다.
     * 과정이 성공하거나 실패하면 OnUserInitializeComplete 델리게이트가 방송됩니다.
     * AsyncAction_CommonUserInitialize는 Event graph에서 사용하기 위한 여러 래퍼 함수를 제공합니다.
     *
     * @returns 과정이 시작되면 true, 제대로 시작하기 전에 실패하면 false
     */
    UFUNCTION(BlueprintCallable, Category = CommonUser)
    COMMONUSER_API virtual bool TryToInitializeUser(FCommonUserInitializeParams Params);

    /** 
     * 새로운 컨트롤러와 기존 컨트롤러의 입력을 감지해 로그인시키는 과정을 시작합니다.
     * 현재 활성 GameViewportClient에 키 입력 핸들러를 삽입하며, 빈 키 배열로 다시 호출하면 꺼집니다.
     *
     * @param AnyUserKeys     기본 사용자도 포함해 어떤 사용자에게서든 감지할 키입니다. 초기 Press Start 화면에 사용하거나, 비워서 비활성화합니다
     * @param NewUserKeys     플레이어 컨트롤러가 없는 새 사용자를 위해 감지할 키입니다. 분할 화면/로컬 멀티플레이어에 사용하거나, 비워서 비활성화합니다
     * @param Params         키 입력 감지 후 TryToInitializeUser에 전달할 파라미터입니다
     */
    UFUNCTION(BlueprintCallable, Category = CommonUser)
    COMMONUSER_API virtual void ListenForLoginKeyInput(TArray<FKey> AnyUserKeys, TArray<FKey> NewUserKeys, FCommonUserInitializeParams Params);

    /** 진행 중인 초기화 시도를 취소하려고 시도합니다. 모든 플랫폼에서 동작하지 않을 수 있지만 콜백은 비활성화됩니다 */
    UFUNCTION(BlueprintCallable, Category = CommonUser)
    COMMONUSER_API virtual bool CancelUserInitialization(int32 LocalPlayerIndex);

    /** 온라인 시스템에서 사용자를 로그아웃시키고, 첫 번째 사용자가 아니라면 필요에 따라 플레이어를 완전히 제거합니다 */
    UFUNCTION(BlueprintCallable, Category = CommonUser)
    COMMONUSER_API virtual bool TryToLogOutUser(int32 LocalPlayerIndex, bool bDestroyPlayer = false);

    /** 오류 후 메인 메뉴로 돌아올 때 로그인 및 초기화 상태를 재설정합니다 */
    UFUNCTION(BlueprintCallable, Category = CommonUser)
    COMMONUSER_API virtual void ResetUserState();

    /** 유효한 신원을 가진 실제 플랫폼 사용자일 수 있으면 true를 반환합니다(현재 로그인 중이 아니어도)  */
    COMMONUSER_API virtual bool IsRealPlatformUserIndex(int32 PlatformUserIndex) const;

    /** 유효한 신원을 가진 실제 플랫폼 사용자일 수 있으면 true를 반환합니다(현재 로그인 중이 아니어도) */
    COMMONUSER_API virtual bool IsRealPlatformUser(FPlatformUserId PlatformUser) const;

    /** 인덱스를 id로 변환합니다 */
    COMMONUSER_API virtual FPlatformUserId GetPlatformUserIdForIndex(int32 PlatformUserIndex) const;

    /** id를 인덱스로 변환합니다 */
    COMMONUSER_API virtual int32 GetPlatformUserIndexForId(FPlatformUserId PlatformUser) const;

    /** 입력 장치에 대한 사용자를 가져옵니다 */
    COMMONUSER_API virtual FPlatformUserId GetPlatformUserIdForInputDevice(FInputDeviceId InputDevice) const;

    /** 사용자의 기본 입력 장치 ID를 가져옵니다 */
    COMMONUSER_API virtual FInputDeviceId GetPrimaryInputDeviceForPlatformUser(FPlatformUserId PlatformUser) const;

    /** 플랫폼 상태나 옵션이 변경될 때 게임 코드에서 캐시된 특성 태그를 설정하는 데 사용합니다 */
    COMMONUSER_API virtual void SetTraitTags(const FGameplayTagContainer& InTags);

    /** 기능 사용 가능성에 영향을 주는 현재 태그를 가져옵니다 */
    const FGameplayTagContainer& GetTraitTags() const { return CachedTraitTags; }

    /** 특정 플랫폼/기능 태그가 활성화되었는지 확인합니다 */
    UFUNCTION(BlueprintPure, Category=CommonUser)
    bool HasTraitTag(const FGameplayTag TraitTag) const { return CachedTraitTags.HasTag(TraitTag); }

    /** 시작 시 Press Start/입력 확인 화면을 표시해야 하는지 확인합니다. 게임은 이 함수를 호출하거나 특성 태그를 직접 확인할 수 있습니다 */
    UFUNCTION(BlueprintPure, BlueprintPure, Category=CommonUser)
    COMMONUSER_API virtual bool ShouldWaitForStartInput() const;


    // 저수준 온라인 시스템 정보를 다루는 함수들

#if COMMONUSER_OSSV1
    /** 특정 유형의 OSS 인터페이스를 반환합니다. 없으면 null을 반환합니다 */
    COMMONUSER_API IOnlineSubsystem* GetOnlineSubsystem(ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;

    /** 특정 유형의 identity 인터페이스를 반환합니다. 없으면 null을 반환합니다 */
    COMMONUSER_API IOnlineIdentity* GetOnlineIdentity(ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;

    /** OSS 시스템의 사람이 읽을 수 있는 이름을 반환합니다 */
    COMMONUSER_API FName GetOnlineSubsystemName(ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;

    /** 현재 온라인 연결 상태를 반환합니다 */
    COMMONUSER_API EOnlineServerConnectionStatus::Type GetConnectionStatus(ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;
#else
    /** 서비스 제공자 유형을 반환합니다. 없으면 None을 반환합니다. */
    COMMONUSER_API UE::Online::EOnlineServices GetOnlineServicesProvider(ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;
    
    /** 특정 유형의 auth 인터페이스를 반환합니다. 없으면 null을 반환합니다 */
    COMMONUSER_API UE::Online::IAuthPtr GetOnlineAuth(ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;

    /** 현재 온라인 연결 상태를 반환합니다 */
    COMMONUSER_API UE::Online::EOnlineServicesConnectionStatus GetConnectionStatus(ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;
#endif

    /** 현재 백엔드 서버에 연결되어 있으면 true를 반환합니다 */
    COMMONUSER_API bool HasOnlineConnection(ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;

    /** 지정한 온라인 시스템에서 로컬 사용자의 현재 로그인 상태를 반환합니다. 실제 플랫폼 사용자에게만 동작합니다 */
    COMMONUSER_API ELoginStatusType GetLocalUserLoginStatus(FPlatformUserId PlatformUser, ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;

    /** 로컬 플랫폼 사용자의 유니크 net id를 반환합니다 */
    COMMONUSER_API FUniqueNetIdRepl GetLocalUserNetId(FPlatformUserId PlatformUser, ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;

    /** 로컬 플랫폼 사용자의 닉네임을 반환합니다. common user Info에 캐시되어 있습니다 */
    COMMONUSER_API FString GetLocalUserNickname(FPlatformUserId PlatformUser, ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;

    /** 사용자 ID를 디버그 문자열로 변환합니다 */
    COMMONUSER_API FString PlatformUserIdToString(FPlatformUserId UserId);

    /** 컨텍스트를 디버그 문자열로 변환합니다 */
    COMMONUSER_API FString ECommonUserOnlineContextToString(ECommonUserOnlineContext Context);

    /** 권한 확인에 대한 사람이 읽을 수 있는 문자열을 반환합니다 */
    COMMONUSER_API virtual FText GetPrivilegeDescription(ECommonUserPrivilege Privilege) const;
    COMMONUSER_API virtual FText GetPrivilegeResultDescription(ECommonUserPrivilegeResult Result) const;

    /** 
     * 기존 로컬 사용자의 로그인 과정을 시작합니다. 콜백이 예약되지 못하면 false를 반환합니다.
     * 이는 저수준 상태 머신을 활성화하며, user info의 초기화 상태는 변경하지 않습니다
     */
    DECLARE_DELEGATE_FiveParams(FOnLocalUserLoginCompleteDelegate, const UCommonUserInfo* /*UserInfo*/, ELoginStatusType /*NewStatus*/, FUniqueNetIdRepl /*NetId*/, const TOptional<FOnlineErrorType>& /*Error*/, ECommonUserOnlineContext /*Type*/);
    COMMONUSER_API virtual bool LoginLocalUser(const UCommonUserInfo* UserInfo, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext Context, FOnLocalUserLoginCompleteDelegate OnComplete);

    /** 특정 로컬 사용자에게 로컬 플레이어를 할당하고 필요에 따라 콜백을 호출합니다 */
    COMMONUSER_API virtual void SetLocalPlayerUserInfo(ULocalPlayer* LocalPlayer, const UCommonUserInfo* UserInfo);

    /** 기본 동작이 있는 컨텍스트를 구체적인 컨텍스트로 해석합니다 */
    COMMONUSER_API ECommonUserOnlineContext ResolveOnlineContext(ECommonUserOnlineContext Context) const;

    /** 플랫폼 컨텍스트와 서비스 인터페이스가 분리되어 있으면 true입니다 */
    COMMONUSER_API bool HasSeparatePlatformContext() const;

protected:
    /** 각 온라인 컨텍스트의 상태와 포인터를 캐시하는 내부 구조체입니다 */
    struct FOnlineContextCache
    {
#if COMMONUSER_OSSV1
       /** 기본 서브시스템 포인터입니다. 게임 인스턴스가 존재하는 동안 유효합니다 */
       IOnlineSubsystem* OnlineSubsystem = nullptr;

       /** 캐시된 identity 시스템입니다. 항상 유효합니다 */
       IOnlineIdentityPtr IdentityInterface;

       /** HandleNetworkConnectionStatusChanged 핸들러에 마지막으로 전달된 연결 상태입니다 */
       EOnlineServerConnectionStatus::Type    CurrentConnectionStatus = EOnlineServerConnectionStatus::Normal;
#else
       /** 온라인 서비스. 특정 서비스에 접근하는 데 사용합니다 */
       UE::Online::IOnlineServicesPtr OnlineServices;
       /** 캐시된 auth 서비스입니다 */
       UE::Online::IAuthPtr AuthService;
       /** 로그인 상태 변경 이벤트 핸들 */
       UE::Online::FOnlineEventDelegateHandle LoginStatusChangedHandle;
       /** 연결 상태 변경 이벤트 핸들 */
       UE::Online::FOnlineEventDelegateHandle ConnectionStatusChangedHandle;
       /** HandleNetworkConnectionStatusChanged 핸들러에 마지막으로 전달된 연결 상태입니다 */
       UE::Online::EOnlineServicesConnectionStatus CurrentConnectionStatus = UE::Online::EOnlineServicesConnectionStatus::NotConnected;
#endif

       /** 상태를 초기화합니다. 모든 shared ptr를 지우는 것이 중요합니다 */
       void Reset()
       {
#if COMMONUSER_OSSV1
          OnlineSubsystem = nullptr;
          IdentityInterface.Reset();
          CurrentConnectionStatus = EOnlineServerConnectionStatus::Normal;
#else
          OnlineServices.Reset();
          AuthService.Reset();
          CurrentConnectionStatus = UE::Online::EOnlineServicesConnectionStatus::NotConnected;
#endif
       }
    };

    /** 진행 중인 로그인 요청을 나타내는 내부 구조체입니다 */
    struct FUserLoginRequest : public TSharedFromThis<FUserLoginRequest>
    {
       FUserLoginRequest(UCommonUserInfo* InUserInfo, ECommonUserPrivilege InPrivilege, ECommonUserOnlineContext InContext, FOnLocalUserLoginCompleteDelegate&& InDelegate)
          : UserInfo(TWeakObjectPtr<UCommonUserInfo>(InUserInfo))
          , DesiredPrivilege(InPrivilege)
          , DesiredContext(InContext)
          , Delegate(MoveTemp(InDelegate))
          {}

       /** 로그인하려는 로컬 사용자입니다 */
       TWeakObjectPtr<UCommonUserInfo> UserInfo;

       /** 로그인 요청의 전체 상태입니다. 여러 소스에서 올 수 있습니다 */
       ECommonUserAsyncTaskState OverallLoginState = ECommonUserAsyncTaskState::NotStarted;

       /** 플랫폼 auth를 사용하려는 시도의 상태입니다. 시작되면 OSSv1에서는 즉시 Failed로 전환됩니다. 플랫폼 auth를 지원하지 않기 때문입니다 */
       ECommonUserAsyncTaskState TransferPlatformAuthState = ECommonUserAsyncTaskState::NotStarted;

       /** AutoLogin 시도의 상태입니다 */
       ECommonUserAsyncTaskState AutoLoginState = ECommonUserAsyncTaskState::NotStarted;

       /** 외부 로그인 UI 시도의 상태입니다 */
       ECommonUserAsyncTaskState LoginUIState = ECommonUserAsyncTaskState::NotStarted;

       /** 요청된 최종 권한입니다 */
       ECommonUserPrivilege DesiredPrivilege = ECommonUserPrivilege::Invalid_Count;

       /** 권한 요청 상태입니다 */
       ECommonUserAsyncTaskState PrivilegeCheckState = ECommonUserAsyncTaskState::NotStarted;

       /** 최종적으로 로그인할 컨텍스트입니다 */
       ECommonUserOnlineContext DesiredContext = ECommonUserOnlineContext::Invalid;

       /** 현재 로그인 중인 온라인 시스템입니다 */
       ECommonUserOnlineContext CurrentContext = ECommonUserOnlineContext::Invalid;

       /** 완료 시 사용자 콜백입니다 */
       FOnLocalUserLoginCompleteDelegate Delegate;

       /** 사용자에게 표시할 가장 최근/관련 오류입니다 */
       TOptional<FOnlineErrorType> Error;
    };


    /** 새 user info 객체를 생성합니다 */
    COMMONUSER_API virtual UCommonUserInfo* CreateLocalUserInfo(int32 LocalPlayerIndex);

    /** const getter를 위한 비 const 래퍼입니다 */
    FORCEINLINE UCommonUserInfo* ModifyInfo(const UCommonUserInfo* Info) { return const_cast<UCommonUserInfo*>(Info); }

    /** OSS에서 user info를 갱신합니다 */
    COMMONUSER_API virtual void RefreshLocalUserInfo(UCommonUserInfo* UserInfo);

    /** 가능하면 권한 사용 가능성 변경 알림을 보냅니다. 현재 값을 이전 캐시 값과 비교합니다 */
    COMMONUSER_API virtual void HandleChangedAvailability(UCommonUserInfo* UserInfo, ECommonUserPrivilege Privilege, ECommonUserAvailability OldAvailability);

    /** 사용자에 캐시된 권한을 갱신하고 델리게이트를 알립니다 */
    COMMONUSER_API virtual void UpdateUserPrivilegeResult(UCommonUserInfo* UserInfo, ECommonUserPrivilege Privilege, ECommonUserPrivilegeResult Result, ECommonUserOnlineContext Context);

    /** 유형별 온라인 시스템의 내부 데이터를 가져옵니다. 서비스에 대해서는 null을 반환할 수 있습니다 */
    COMMONUSER_API const FOnlineContextCache* GetContextCache(ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game) const;
    COMMONUSER_API FOnlineContextCache* GetContextCache(ECommonUserOnlineContext Context = ECommonUserOnlineContext::Game);

    /** 델리게이트가 바인딩되기 전에 시스템 객체를 생성하고 설정합니다 */
    COMMONUSER_API virtual void CreateOnlineContexts();
    COMMONUSER_API virtual void DestroyOnlineContexts();

    /** 온라인 델리게이트를 바인딩합니다 */
    COMMONUSER_API virtual void BindOnlineDelegates();

    /** 단일 사용자를 강제로 로그아웃시키고 초기화 해제합니다 */
    COMMONUSER_API virtual void LogOutLocalUser(FPlatformUserId PlatformUser);

    /** 로그인 요청의 다음 단계를 수행합니다. 완료될 수도 있습니다. 완료되면 true를 반환합니다 */
    COMMONUSER_API virtual void ProcessLoginRequest(TSharedRef<FUserLoginRequest> Request);

    /** OSS에서 로그인 호출을 수행합니다. AutoLogin이 시작되면 true를 반환합니다 */
    COMMONUSER_API virtual bool TransferPlatformAuth(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);

    /** OSS에서 AutoLogin을 수행합니다. AutoLogin이 시작되면 true를 반환합니다. */
    COMMONUSER_API virtual bool AutoLogin(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);

    /** OSS에서 ShowLoginUI를 호출합니다. ShowLoginUI가 시작되면 true를 반환합니다. */
    COMMONUSER_API virtual bool ShowLoginUI(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);

    /** OSS에서 QueryUserPrivilege를 호출합니다. QueryUserPrivilege가 시작되면 true를 반환합니다. */
    COMMONUSER_API virtual bool QueryUserPrivilege(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);

    /** OSS 전용 함수들 */
#if COMMONUSER_OSSV1
    COMMONUSER_API virtual ECommonUserPrivilege ConvertOSSPrivilege(EUserPrivileges::Type Privilege) const;
    COMMONUSER_API virtual EUserPrivileges::Type ConvertOSSPrivilege(ECommonUserPrivilege Privilege) const;
    COMMONUSER_API virtual ECommonUserPrivilegeResult ConvertOSSPrivilegeResult(EUserPrivileges::Type Privilege, uint32 Results) const;

    COMMONUSER_API void BindOnlineDelegatesOSSv1();
    COMMONUSER_API bool AutoLoginOSSv1(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);
    COMMONUSER_API bool ShowLoginUIOSSv1(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);
    COMMONUSER_API bool QueryUserPrivilegeOSSv1(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);
#else
    COMMONUSER_API virtual ECommonUserPrivilege ConvertOnlineServicesPrivilege(UE::Online::EUserPrivileges Privilege) const;
    COMMONUSER_API virtual UE::Online::EUserPrivileges ConvertOnlineServicesPrivilege(ECommonUserPrivilege Privilege) const;
    COMMONUSER_API virtual ECommonUserPrivilegeResult ConvertOnlineServicesPrivilegeResult(UE::Online::EUserPrivileges Privilege, UE::Online::EPrivilegeResults Results) const;

    COMMONUSER_API void BindOnlineDelegatesOSSv2();
    COMMONUSER_API void CacheConnectionStatus(ECommonUserOnlineContext Context);
    COMMONUSER_API bool TransferPlatformAuthOSSv2(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);
    COMMONUSER_API bool AutoLoginOSSv2(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);
    COMMONUSER_API bool ShowLoginUIOSSv2(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);
    COMMONUSER_API bool QueryUserPrivilegeOSSv2(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);
    COMMONUSER_API TSharedPtr<UE::Online::FAccountInfo> GetOnlineServiceAccountInfo(UE::Online::IAuthPtr AuthService, FPlatformUserId InUserId) const;
#endif

    /** OSS 함수의 콜백들입니다 */
#if COMMONUSER_OSSV1
    COMMONUSER_API virtual void HandleIdentityLoginStatusChanged(int32 PlatformUserIndex, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& NewId, ECommonUserOnlineContext Context);
    COMMONUSER_API virtual void HandleUserLoginCompleted(int32 PlatformUserIndex, bool bWasSuccessful, const FUniqueNetId& NetId, const FString& Error, ECommonUserOnlineContext Context);
    COMMONUSER_API virtual void HandleControllerPairingChanged(int32 PlatformUserIndex, FControllerPairingChangedUserInfo PreviousUser, FControllerPairingChangedUserInfo NewUser);
    COMMONUSER_API virtual void HandleNetworkConnectionStatusChanged(const FString& ServiceName, EOnlineServerConnectionStatus::Type LastConnectionStatus, EOnlineServerConnectionStatus::Type ConnectionStatus, ECommonUserOnlineContext Context);
    COMMONUSER_API virtual void HandleOnLoginUIClosed(TSharedPtr<const FUniqueNetId> LoggedInNetId, const int PlatformUserIndex, const FOnlineError& Error, ECommonUserOnlineContext Context);
    COMMONUSER_API virtual void HandleCheckPrivilegesComplete(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults, ECommonUserPrivilege RequestedPrivilege, TWeakObjectPtr<UCommonUserInfo> CommonUserInfo, ECommonUserOnlineContext Context);
#else
    COMMONUSER_API virtual void HandleAuthLoginStatusChanged(const UE::Online::FAuthLoginStatusChanged& EventParameters, ECommonUserOnlineContext Context);
    COMMONUSER_API virtual void HandleUserLoginCompletedV2(const UE::Online::TOnlineResult<UE::Online::FAuthLogin>& Result, FPlatformUserId PlatformUser, ECommonUserOnlineContext Context);
    COMMONUSER_API virtual void HandleOnLoginUIClosedV2(const UE::Online::TOnlineResult<UE::Online::FExternalUIShowLoginUI>& Result, FPlatformUserId PlatformUser, ECommonUserOnlineContext Context);
    COMMONUSER_API virtual void HandleNetworkConnectionStatusChanged(const UE::Online::FConnectionStatusChanged& EventParameters, ECommonUserOnlineContext Context);
    COMMONUSER_API virtual void HandleCheckPrivilegesComplete(const UE::Online::TOnlineResult<UE::Online::FQueryUserPrivilege>& Result, TWeakObjectPtr<UCommonUserInfo> CommonUserInfo, UE::Online::EUserPrivileges DesiredPrivilege, ECommonUserOnlineContext Context);
#endif

    /**
     * 입력 장치(예: 게임패드)가 연결되거나 연결 해제되었을 때의 콜백입니다.
     */
    COMMONUSER_API virtual void HandleInputDeviceConnectionChanged(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId);

    COMMONUSER_API virtual void HandleLoginForUserInitialize(const UCommonUserInfo* UserInfo, ELoginStatusType NewStatus, FUniqueNetIdRepl NetId, const TOptional<FOnlineErrorType>& Error, ECommonUserOnlineContext Context, FCommonUserInitializeParams Params);
    COMMONUSER_API virtual void HandleUserInitializeFailed(FCommonUserInitializeParams Params, FText Error);
    COMMONUSER_API virtual void HandleUserInitializeSucceeded(FCommonUserInitializeParams Params);

    /** 로그인/Press Start 로직을 처리하는 콜백입니다 */
    COMMONUSER_API virtual bool OverrideInputKeyForLogin(FInputKeyEventArgs& EventArgs);


    /** 이전 오버라이드 핸들러입니다. 취소 시 복원됩니다 */
    FOverrideInputKeyHandler WrappedInputKeyHandler;

    /** 어떤 사용자에게서든 감지할 키 목록입니다 */
    TArray<FKey> LoginKeysForAnyUser;

    /** 새로 매핑되지 않은 사용자를 위해 감지할 키 목록입니다 */
    TArray<FKey> LoginKeysForNewUser;

    /** 키 입력으로 시작하는 로그인에 사용할 파라미터입니다 */
    FCommonUserInitializeParams ParamsForLoginKey;

    /** 최대 로컬 플레이어 수입니다 */
    int32 MaxNumberOfLocalPlayers = 0;
    
    /** 전용 서버인지 여부입니다. 전용 서버는 LocalPlayer가 필요하지 않습니다 */
    bool bIsDedicatedServer = false;

    /** 현재 진행 중인 로그인 요청 목록입니다 */
    TArray<TSharedRef<FUserLoginRequest>> ActiveLoginRequests;

    /** 로컬 플레이어 인덱스에서 사용자로의 각 로컬 사용자 정보입니다 */
    UPROPERTY()
    TMap<int32, TObjectPtr<UCommonUserInfo>> LocalUserInfos;
    
    /** 캐시된 플랫폼/모드 특성 태그입니다 */
    FGameplayTagContainer CachedTraitTags;

    /** 이 범위 밖에서는 접근하지 마세요. 초기화 중에만 사용합니다 */
    FOnlineContextCache* DefaultContextInternal = nullptr;
    FOnlineContextCache* ServiceContextInternal = nullptr;
    FOnlineContextCache* PlatformContextInternal = nullptr;

    friend UCommonUserInfo;
};