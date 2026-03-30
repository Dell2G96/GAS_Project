// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonUserSubsystem.h"
#include "Engine/CancellableAsyncAction.h"

#include "AsyncAction_CommonUserInitialize.generated.h"


enum class ECommonUserOnlineContext : uint8;
enum class ECommonUserPrivilege : uint8;
struct FInputDeviceId;

class FText;
class UObject;
struct FFrame;

/**
 * 사용자 초기화를 위한 다양한 기능들을 처리하는 비동기 액션 클래스
 */
UCLASS(MinimalAPI)
class UAsyncAction_CommonUserInitialize : public UCancellableAsyncAction
{
    GENERATED_BODY()

public:
    /**
     * 커먼 유저 시스템으로 로컬 플레이어를 초기화합니다. 
     * 플랫폼별 로그인 및 권한 검사를 포함합니다.
     * 프로세스가 성공하거나 실패하면 OnInitializationComplete 델리게이트를 브로드캐스트합니다.
     *
     * @param LocalPlayerIndex 게임 인스턴스에서 ULocalPlayer의 원하는 인덱스, 0은 주 플레이어, 1+는 로컬 멀티플레이어용
     * @param PrimaryInputDevice 사용자용 주 입력 디바이스, 유효하지 않으면 시스템 기본값 사용
     * @param bCanUseGuestLogin true이면 실제 시스템 네트워크 ID 없이 게스트로 플레이 가능
     */
    UFUNCTION(BlueprintCallable, Category = CommonUser, meta = (BlueprintInternalUseOnly = "true"))
    static  UAsyncAction_CommonUserInitialize* InitializeForLocalPlay(UCommonUserSubsystem* Target, int32 LocalPlayerIndex, FInputDeviceId PrimaryInputDevice, bool bCanUseGuestLogin);

    /**
     * 기존 사용자를 플랫폼별 온라인 백엔드에 로그인하여 완전한 온라인 플레이를 활성화합니다.
     * 프로세스가 성공하거나 실패하면 OnInitializationComplete 델리게이트를 브로드캐스트합니다.
     *
     * @param LocalPlayerIndex 게임 인스턴스에서 기존 LocalPlayer의 인덱스
     */
    UFUNCTION(BlueprintCallable, Category = CommonUser, meta = (BlueprintInternalUseOnly = "true"))
    static  UAsyncAction_CommonUserInitialize* LoginForOnlinePlay(UCommonUserSubsystem* Target, int32 LocalPlayerIndex);

    /** 초기화가 성공하거나 실패할 때 호출되는 델리게이트 */
    UPROPERTY(BlueprintAssignable)
    FCommonUserOnInitializeCompleteMulticast OnInitializationComplete;

    /** 필요한 경우 콜백을 보내고 실패 처리 */
     void HandleFailure();

    /** 래퍼 델리게이트, 적절한 경우 OnInitializationComplete로 전달 */
    UFUNCTION()
     virtual void HandleInitializationComplete(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext);

protected:
    /** 실제 초기화 시작 */
     virtual void Activate() override;

    TWeakObjectPtr<UCommonUserSubsystem> Subsystem;
    FCommonUserInitializeParams Params;
};
