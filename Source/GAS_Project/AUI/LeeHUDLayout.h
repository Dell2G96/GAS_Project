// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LeeActivatableWidget.h"
#include "LeeHUDLayout.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, meta=(DisplayName="Lee HUD Layout"), Category="Lee|HUD")
class GAS_PROJECT_API ULeeHUDLayout : public ULeeActivatableWidget
{
	GENERATED_BODY()
public:
	ULeeHUDLayout(const FObjectInitializer& ObjectInitializer);

	

    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;

protected:
    void HandleEscapeAction();
    
    /** 
    * 컨트롤러 연결이 끊겼을 때 호출되는 콜백입니다.
    * 이 함수는 이제 플레이어에게 매핑된 입력 장치가 없는지 확인합니다.
    * 그런 경우 플레이할 수 없으므로 DisplayControllerDisconnectedMenu를 호출합니다.
    */
    void HandleInputDeviceConnectionChanged(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId);

    /**
     * 컨트롤러의 소유 플랫폼 유저가 변경되었을 때 호출되는 콜백입니다.
     * 더 이상 "Controller Disconnected" 메뉴를 표시할 필요가 없는지 확인하는 데 사용합니다.
     */
    void HandleInputDevicePairingChanged(FInputDeviceId InputDeviceId, FPlatformUserId NewUserPlatformId, FPlatformUserId OldUserPlatformId);
    
    /**
     * 플레이어의 컨트롤러 상태가 변경되었음을 이 위젯에 알립니다.
     * 다음 틱에 처리하도록 타이머를 예약하고,
     * "controller disconnected" 위젯을 보여줘야 하는지 확인합니다.
     */
    void NotifyControllerStateChangeForDisconnectScreen();

    /**
     * 플레이어에 연결된 컨트롤러 상태를 확인합니다.
     * 연결된 컨트롤러가 없다면 Disconnect 메뉴를 표시해야 합니다.
     * 반대로 연결된 컨트롤러가 있다면, 이미 표시 중인 경우 Disconnect 메뉴를 숨길 수 있습니다.
     */
    virtual void ProcessControllerDevicesHavingChangedForDisconnectScreen();

    /**
     * 이 플랫폼이 "controller disconnected" 화면을 지원하는지 반환합니다.
     */
    virtual bool ShouldPlatformDisplayControllerDisconnectScreen() const;
    
    /**
    * ControllerDisconnectedMenuClass를 메뉴 레이어(UI.Layer.Menu)에 푸시합니다.
    */
    UFUNCTION(BlueprintNativeEvent, Category="Controller Disconnect Menu")
    void DisplayControllerDisconnectedMenu();

    /**
    * 활성화되어 있다면 controller disconnected 메뉴를 숨깁니다.
    */
    UFUNCTION(BlueprintNativeEvent, Category="Controller Disconnect Menu")
    void HideControllerDisconnectedMenu();
    
    /**
     * 사용자가 "Pause" 또는 "Escape" 버튼을 눌렀을 때 표시될 메뉴입니다.
     */
    UPROPERTY(EditDefaultsOnly)
    TSoftClassPtr<UCommonActivatableWidget> EscapeMenuClass;

    /** 
    * 사용자의 모든 컨트롤러가 연결 해제되었을 때 표시할 위젯입니다.
    */
	
    //  UPROPERTY(EditDefaultsOnly, Category="Controller Disconnect Menu")
    // TSubclassOf<ULeeControllerDisconnectedScreen> ControllerDisconnectedScreen;

    /**
     * 이 플랫폼에서 "Controller Disconnected" 화면을 표시하기 위해 필요한 플랫폼 태그입니다.
     *
     * 이 태그들이 해당 플랫폼의 INI 파일에 설정되어 있지 않다면,
     * 컨트롤러 연결 해제 화면은 절대 표시되지 않습니다.
     */
    UPROPERTY(EditDefaultsOnly, Category="Controller Disconnect Menu")
    FGameplayTagContainer PlatformRequiresControllerDisconnectScreen;

    /** 활성화된 "Controller Disconnected" 메뉴가 있다면 그 위젯을 가리키는 포인터입니다. */
    UPROPERTY(Transient)
    TObjectPtr<UCommonActivatableWidget> SpawnedControllerDisconnectScreen;

    /** 플레이어의 컨트롤러 상태를 처리할 때 사용하는 FSTicker의 핸들입니다. */
    FTSTicker::FDelegateHandle RequestProcessControllerStateHandle;
};