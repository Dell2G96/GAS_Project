// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeHUDLayout.h"

#include "CommonUIExtensions.h"
#include "CommonUISettings.h"
#include "ICommonUIModule.h"
#include "UITag.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "GameFramework/InputSettings.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
#include "Input/CommonUIInputTypes.h"

#if WITH_EDITOR
#include "CommonUIVisibilitySubsystem.h"
#endif  // WITH_EDITOR


UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_ACTION_ESCAPE, "UI.Action.Escape");

ULeeHUDLayout::ULeeHUDLayout(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
, SpawnedControllerDisconnectScreen(nullptr)
{
	// 기본적으로 주 입력 장치가 컨트롤러의 플랫폼에서만 연결 해제 화면이 필요
	PlatformRequiresControllerDisconnectScreen.AddTag(MyTags::Lyra::Platform_Trait_Input_PrimarlyController);
}

void ULeeHUDLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();


	RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(TAG_UI_ACTION_ESCAPE), false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleEscapeAction)));

	// 컨트롤러 연결 해제 화면을 표시할 수 있다면, 컨트롤러 상태 변경 델리게이트를 구독합니다.
	if (ShouldPlatformDisplayControllerDisconnectScreen())
	{
		// 입력 장치 연결 상태 변경 시 바인딩
		IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
		DeviceMapper.GetOnInputDeviceConnectionChange().AddUObject(this, &ThisClass::HandleInputDeviceConnectionChanged);
		DeviceMapper.GetOnInputDevicePairingChange().AddUObject(this, &ThisClass::HandleInputDevicePairingChanged);    
	}
	
}

void ULeeHUDLayout::NativeDestruct()
{
	Super::NativeDestruct();

	// 입력 장치 연결 상태 변경 바인딩을 제거합니다.
	IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	DeviceMapper.GetOnInputDeviceConnectionChange().RemoveAll(this);
	DeviceMapper.GetOnInputDevicePairingChange().RemoveAll(this);

	if (RequestProcessControllerStateHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(RequestProcessControllerStateHandle);
		RequestProcessControllerStateHandle.Reset();
	}
}

void ULeeHUDLayout::HandleEscapeAction()
{
	if (ensure(!EscapeMenuClass.IsNull()))
	{
		UCommonUIExtensions::PushStreamedContentToLayer_ForPlayer(GetOwningLocalPlayer(), MyTags::Lyra::UI_LAYER_MENU, EscapeMenuClass);
	}
}

void ULeeHUDLayout::HandleInputDeviceConnectionChanged(EInputDeviceConnectionState NewConnectionState,
	FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId)
{
	const FPlatformUserId OwningLocalPlayerId = GetOwningLocalPlayer()->GetPlatformUserId();

	ensure(OwningLocalPlayerId.IsValid());

	// 이 장치 연결 변경은 다른 플레이어에게 발생한 것이므로, 우리 쪽에서는 무시합니다.
	if (PlatformUserId != OwningLocalPlayerId)
	{
		return;
	}

	NotifyControllerStateChangeForDisconnectScreen();
}

void ULeeHUDLayout::HandleInputDevicePairingChanged(FInputDeviceId InputDeviceId, FPlatformUserId NewUserPlatformId,
	FPlatformUserId OldUserPlatformId)
{
	const FPlatformUserId OwningLocalPlayerId = GetOwningLocalPlayer()->GetPlatformUserId();

	ensure(OwningLocalPlayerId.IsValid());

	// 이 페어링 변경이 우리 로컬 플레이어와 관련 있다면, 상태 변경을 알립니다.
	if (NewUserPlatformId == OwningLocalPlayerId || OldUserPlatformId == OwningLocalPlayerId)
	{
		NotifyControllerStateChangeForDisconnectScreen();  
	}
}

void ULeeHUDLayout::NotifyControllerStateChangeForDisconnectScreen()
{
	// 여기까지 왔다면 반드시 컨트롤러 상태 변경 델리게이트가 바인딩된 경우여야 합니다.
	ensure(ShouldPlatformDisplayControllerDisconnectScreen());

	// 아직 예약되지 않았다면, 다음 틱에서 장치 상태 처리를 하도록 큐에 등록합니다.
	if (!RequestProcessControllerStateHandle.IsValid())
	{
		RequestProcessControllerStateHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float DeltaTime)
		{
		   RequestProcessControllerStateHandle.Reset();
		   ProcessControllerDevicesHavingChangedForDisconnectScreen();
		   return false;
		}));
	}
}

void ULeeHUDLayout::ProcessControllerDevicesHavingChangedForDisconnectScreen()
{
	// 여기까지 왔다면 반드시 컨트롤러 상태 변경 델리게이트가 바인딩된 경우여야 합니다.
	ensure(ShouldPlatformDisplayControllerDisconnectScreen());
    
	const FPlatformUserId OwningLocalPlayerId = GetOwningLocalPlayer()->GetPlatformUserId();
    
	ensure(OwningLocalPlayerId.IsValid());

	// 우리 플레이어에게 매핑된 모든 입력 장치를 가져옵니다.
	const IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	TArray<FInputDeviceId> MappedInputDevices;
	const int32 NumDevicesMappedToUser = DeviceMapper.GetAllInputDevicesForUser(OwningLocalPlayerId, OUT MappedInputDevices);

	// 이 플랫폼 유저에게 매핑된 다른 연결된 게임패드 장치가 있는지 확인합니다.
	bool bHasConnectedController = false;

	for (const FInputDeviceId MappedDevice : MappedInputDevices)
	{
		if (DeviceMapper.GetInputDeviceConnectionState(MappedDevice) == EInputDeviceConnectionState::Connected)
		{
			const FHardwareDeviceIdentifier HardwareInfo = UInputDeviceSubsystem::Get()->GetInputDeviceHardwareIdentifier(MappedDevice);
			if (HardwareInfo.PrimaryDeviceType == EHardwareDevicePrimaryType::Gamepad)
			{
				bHasConnectedController = true;
			}
		}        
	}

	// 이 유저에게 매핑된 게임패드 입력 장치가 없다면, 다시 연결하라는 토스트를 띄우고자 합니다.
	if (!bHasConnectedController)
	{
		DisplayControllerDisconnectedMenu();
	}
	// 그렇지 않다면, 현재 표시 중일 경우 해당 화면을 숨길 수 있습니다.
	else if (SpawnedControllerDisconnectScreen)
	{
		HideControllerDisconnectedMenu();
	}
}

bool ULeeHUDLayout::ShouldPlatformDisplayControllerDisconnectScreen() const
{
	// 이 메뉴는 주 입력 장치가 컨트롤러인 플랫폼에서만 표시하고자 합니다.
	bool bHasAllRequiredTags = ICommonUIModule::GetSettings().GetPlatformTraits().HasAll(PlatformRequiresControllerDisconnectScreen);

	// 에디터에서 에뮬레이션 중인 태그도 함께 확인합니다.
#if WITH_EDITOR
	const FGameplayTagContainer& PlatformEmulationTags = UCommonUIVisibilitySubsystem::Get(GetOwningLocalPlayer())->GetVisibilityTags();
	bHasAllRequiredTags |= PlatformEmulationTags.HasAll(PlatformRequiresControllerDisconnectScreen);
#endif  // WITH_EDITOR

	return bHasAllRequiredTags;
}

void ULeeHUDLayout::DisplayControllerDisconnectedMenu_Implementation()
{
	UE_LOG(LogLee, Log, TEXT("[%hs] Display controller disconnected menu!"), __func__);

	// if (ControllerDisconnectedScreen)
	// {
	// 	// "controller disconnected" 위젯을 메뉴 레이어에 푸시합니다.
	// 	SpawnedControllerDisconnectScreen = UCommonUIExtensions::PushContentToLayer_ForPlayer(GetOwningLocalPlayer(), MyTags::Lyra::UI_LAYER_MENU, ControllerDisconnectedScreen);
	// }
}

void ULeeHUDLayout::HideControllerDisconnectedMenu_Implementation()
{
	UE_LOG(LogLee, Log, TEXT("[%hs] Hide controller disconnected menu!"), __func__);
    
	UCommonUIExtensions::PopContentFromLayer(SpawnedControllerDisconnectScreen);
	SpawnedControllerDisconnectScreen = nullptr;
}
