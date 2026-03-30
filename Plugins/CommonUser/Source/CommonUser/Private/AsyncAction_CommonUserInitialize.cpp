// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncAction_CommonUserInitialize.h"

#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_CommonUserInitialize)

UAsyncAction_CommonUserInitialize* UAsyncAction_CommonUserInitialize::InitializeForLocalPlay(
    UCommonUserSubsystem* Target, int32 LocalPlayerIndex, FInputDeviceId PrimaryInputDevice, bool bCanUseGuestLogin)
{
    if (!PrimaryInputDevice.IsValid())
    {
        /** 기본 디바이스로 설정 */
        PrimaryInputDevice = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();
    }

    /** 비동기 액션 객체 생성 */
    UAsyncAction_CommonUserInitialize* Action = NewObject<UAsyncAction_CommonUserInitialize>();

    /** 게임 인스턴스에 등록 */
    Action->RegisterWithGameInstance(Target);

    if (Target && Action->IsRegistered())
    {
        Action->Subsystem = Target;
        
        /** 로컬 플레이 권한 설정 */
        Action->Params.RequestedPrivilege = ECommonUserPrivilege::CanPlay;
        Action->Params.LocalPlayerIndex = LocalPlayerIndex;
        Action->Params.PrimaryInputDevice = PrimaryInputDevice;
        Action->Params.bCanUseGuestLogin = bCanUseGuestLogin;
        /** 새 로컬 플레이어 생성 가능 */
        Action->Params.bCanCreateNewLocalPlayer = true;
    }
    else
    {
        /** 등록 실패 시 즉시 파괴 */
        Action->SetReadyToDestroy();
    }

    return Action;
}

UAsyncAction_CommonUserInitialize* UAsyncAction_CommonUserInitialize::LoginForOnlinePlay(
    UCommonUserSubsystem* Target, int32 LocalPlayerIndex)
{
    /** 비동기 액션 객체 생성 */
    UAsyncAction_CommonUserInitialize* Action = NewObject<UAsyncAction_CommonUserInitialize>();

    /** 게임 인스턴스에 등록 */
    Action->RegisterWithGameInstance(Target);

    if (Target && Action->IsRegistered())
    {
        Action->Subsystem = Target;
        
        /** 온라인 플레이 권한 설정 */
        Action->Params.RequestedPrivilege = ECommonUserPrivilege::CanPlayOnline;
        Action->Params.LocalPlayerIndex = LocalPlayerIndex;
        /** 새 로컬 플레이어 생성 불가 */
        Action->Params.bCanCreateNewLocalPlayer = false;
    }
    else
    {
        /** 등록 실패 시 즉시 파괴 */
        Action->SetReadyToDestroy();
    }

    return Action;
}

void UAsyncAction_CommonUserInitialize::HandleFailure()
{
    const UCommonUserInfo* UserInfo = nullptr;
    if (Subsystem.IsValid())
    {
        /** 해당 로컬 플레이어의 사용자 정보 조회 */
        UserInfo = Subsystem->GetUserInfoForLocalPlayerIndex(Params.LocalPlayerIndex);
    }
    /** 초기화 완료 콜백 호출 (실패) */
    HandleInitializationComplete(UserInfo, false, NSLOCTEXT("CommonUser", "LoginFailedEarly", "Unable to start login process"), Params.RequestedPrivilege, Params.OnlineContext);
}

void UAsyncAction_CommonUserInitialize::HandleInitializationComplete(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
    if (ShouldBroadcastDelegates())
    {
        /** 초기화 완료 델리게이트 브로드캐스트 */
        OnInitializationComplete.Broadcast(UserInfo, bSuccess, Error, RequestedPrivilege, OnlineContext);
    }

    /** 액션 완료 후 파괴 */
    SetReadyToDestroy();
}

void UAsyncAction_CommonUserInitialize::Activate()
{
    if (Subsystem.IsValid())
    {
        /** 초기화 완료 콜백 바인딩 */
        Params.OnUserInitializeComplete.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UAsyncAction_CommonUserInitialize, HandleInitializationComplete));
        /** 사용자 초기화 시도 */
        bool bSuccess = Subsystem->TryToInitializeUser(Params);

        if (!bSuccess)
        {
            /** 다음 프레임에 실패 처리 (1프레임 지연) */
            FTimerManager* TimerManager = GetTimerManager();
            
            if (TimerManager)
            {
                TimerManager->SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UAsyncAction_CommonUserInitialize::HandleFailure));
            }
        }
    }
    else
    {
        /** 서브시스템 없으면 즉시 파괴 */
        SetReadyToDestroy();
    }  
}