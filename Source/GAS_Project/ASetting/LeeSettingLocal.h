// // Copyright Epic Games, Inc. All Rights Reserved.
//
// #pragma once
//
// #include "GameFramework/GameUserSettings.h"
// #include "InputCoreTypes.h"
//
// // #include "LeeSettingsLocal.generated.h"
//
// enum class ECommonInputType : uint8;
// enum class ELeeDisplayablePerformanceStat : uint8;
// enum class ELeeStatDisplayMode : uint8;
//
// class ULeeLocalPlayer;
// class UObject;
// class USoundControlBus;
// class USoundControlBusMix;
// struct FFrame;
//
// USTRUCT()
// struct FLeeScalabilitySnapshot
// {
//     GENERATED_BODY()
//
//     FLeeScalabilitySnapshot();
//
//     Scalability::FQualityLevels Qualities;
//     bool bActive = false;
//     bool bHasOverrides = false;
// };
//
// /**
//  * ULeeSettingsLocal
//  * 
//  * Lee 로컬 설정 클래스. 게임 사용자 설정을 확장하여 프론트엔드/게임 모드별 성능, 디스플레이, 오디오, 입력 등을 관리합니다.
//  */
// UCLASS()
// class ULeeSettingsLocal : public UGameUserSettings
// {
//     GENERATED_BODY()
//
// public:
//
//     ULeeSettingsLocal();
//
//     static ULeeSettingsLocal* Get();
//
//     //~UObject interface
//     virtual void BeginDestroy() override;
//     //~End of UObject interface
//
//     //~UGameUserSettings interface
//     virtual void SetToDefaults() override;
//     virtual void LoadSettings(bool bForceReload) override;
//     virtual void ConfirmVideoMode() override;
//     virtual float GetEffectiveFrameRateLimit() override;
//     virtual void ResetToCurrentSettings() override;
//     virtual void ApplyNonResolutionSettings() override;
//     virtual int32 GetOverallScalabilityLevel() const override;
//     virtual void SetOverallScalabilityLevel(int32 Value) override;
//     //~End of UGameUserSettings interface
//
//     void OnExperienceLoaded();
//     void OnHotfixDeviceProfileApplied();
//
//     //////////////////////////////////////////////////////////////////
//     // Frontend state (프론트엔드 상태)
//
// public:
//     /** 프론트엔드 성능 설정 사용 여부를 설정합니다. */
//     void SetShouldUseFrontendPerformanceSettings(bool bInFrontEnd);
// protected:
//     /** 프론트엔드 성능 설정을 사용해야 하는지 확인합니다. */
//     bool ShouldUseFrontendPerformanceSettings() const;
// private:
//     bool bInFrontEndForPerformancePurposes = false;
//
//     //////////////////////////////////////////////////////////////////
//     // Performance stats (성능 통계)
// public:
//     /** 지정된 성능 통계의 표시 모드를 반환합니다. */
//     ELeeStatDisplayMode GetPerfStatDisplayState(ELeeDisplayablePerformanceStat Stat) const;
//     
//     /** 지정된 성능 통계의 표시 모드를 설정합니다. */
//     void SetPerfStatDisplayState(ELeeDisplayablePerformanceStat Stat, ELeeStatDisplayMode DisplayMode);
//
//     /** 성능 통계 표시 상태가 변경되거나 설정이 적용될 때 발생하는 이벤트 */
//     DECLARE_EVENT(ULeeSettingsLocal, FPerfStatSettingsChanged);
//     FPerfStatSettingsChanged& OnPerfStatDisplayStateChanged() { return PerfStatSettingsChangedEvent; }
//
//     // Latency flash indicators (지연 플래시 표시기)
//     /** 플랫폼이 지연 마커를 지원하는지 확인합니다. */
//     static bool DoesPlatformSupportLatencyMarkers();
//     
//     DECLARE_EVENT(ULeeSettingsLocal, FLatencyFlashInidicatorSettingChanged);
//     UFUNCTION()
//     void SetEnableLatencyFlashIndicators(const bool bNewVal);
//     UFUNCTION()
//     bool GetEnableLatencyFlashIndicators() const { return bEnableLatencyFlashIndicators; }
//     FLatencyFlashInidicatorSettingChanged& OnLatencyFlashInidicatorSettingsChangedEvent() { return LatencyFlashInidicatorSettingsChangedEvent; }
//
//     // Latency tracking stats (지연 추적 통계)
//     /** 플랫폼이 지연 추적 통계를 지원하는지 확인합니다. */
//     static bool DoesPlatformSupportLatencyTrackingStats();
//     
//     DECLARE_EVENT(ULeeSettingsLocal, FLatencyStatEnabledSettingChanged);
//     FLatencyStatEnabledSettingChanged& OnLatencyStatIndicatorSettingsChangedEvent() { return LatencyStatIndicatorSettingsChangedEvent; }
//     
//     UFUNCTION()
//     void SetEnableLatencyTrackingStats(const bool bNewVal);
//     UFUNCTION()
//     bool GetEnableLatencyTrackingStats() const { return bEnableLatencyTrackingStats; }
//
// private:
//
//     /** 지연 추적 통계 설정을 적용합니다. */
//     void ApplyLatencyTrackingStatSetting();
//     
//     // HUD에 표시할 통계 목록
//     UPROPERTY(Config)
//     TMap<ELeeDisplayablePerformanceStat, ELeeStatDisplayMode> DisplayStatList;
//
//     // 표시 통계 위젯 컨테이너가 바인딩할 이벤트
//     FPerfStatSettingsChanged PerfStatSettingsChangedEvent;
//
//     // true이면 입력 지연 측정을 위한 지연 플래시 마커를 활성화합니다.
//     UPROPERTY(Config)
//     bool bEnableLatencyFlashIndicators = false;
//
//     // 플레이어 입력이 바인딩할 지연 플래시 표시기 설정 변경 이벤트
//     FLatencyFlashInidicatorSettingChanged LatencyFlashInidicatorSettingsChangedEvent;
//
//     // 지연 통계 토글 변경 이벤트
//     FLatencyStatEnabledSettingChanged LatencyStatIndicatorSettingsChangedEvent;
//
//     // true이면 ILatencyMarkerModule 모듈을 통해 지연 통계를 추적합니다.
//     // 더 자세한 지연 중심 성능 통계를 볼 수 있습니다.
//     // 기본값은 플랫폼 지원 여부에 따라 true/false로 설정됩니다.
//     UPROPERTY(Config)
//     bool bEnableLatencyTrackingStats;
//
//     //////////////////////////////////////////////////////////////////
//     // Brightness/Gamma (밝기/감마)
// public:
//     UFUNCTION()
//     float GetDisplayGamma() const;
//     UFUNCTION()
//     void SetDisplayGamma(float InGamma);
//
// private:
//     /** 디스플레이 감마를 적용합니다. */
//     void ApplyDisplayGamma();
//     
//     UPROPERTY(Config)
//     float DisplayGamma = 2.2f;
//
//     //////////////////////////////////////////////////////////////////
//     // Display (디스플레이)
// public:
//     UFUNCTION()
//     float GetFrameRateLimit_OnBattery() const;
//     UFUNCTION()
//     void SetFrameRateLimit_OnBattery(float NewLimitFPS);
//
//     UFUNCTION()
//     float GetFrameRateLimit_InMenu() const;
//     UFUNCTION()
//     void SetFrameRateLimit_InMenu(float NewLimitFPS);
//
//     UFUNCTION()
//     float GetFrameRateLimit_WhenBackgrounded() const;
//     UFUNCTION()
//     void SetFrameRateLimit_WhenBackgrounded(float NewLimitFPS);
//
//     UFUNCTION()
//     float GetFrameRateLimit_Always() const;
//     UFUNCTION()
//     void SetFrameRateLimit_Always(float NewLimitFPS);
//
// protected:
//     /** 효과적인 프레임레이트 제한을 업데이트합니다. */
//     void UpdateEffectiveFrameRateLimit();
//
// private:
//     UPROPERTY(Config)
//     float FrameRateLimit_OnBattery;
//     UPROPERTY(Config)
//     float FrameRateLimit_InMenu;
//     UPROPERTY(Config)
//     float FrameRateLimit_WhenBackgrounded;
//
//     //////////////////////////////////////////////////////////////////
//     // Display - Mobile quality settings (디스플레이 - 모바일 품질 설정)
// public:
//     
//     /** 기본 모바일 프레임레이트를 반환합니다. */
//     static int32 GetDefaultMobileFrameRate();
//     /** 최대 모바일 프레임레이트를 반환합니다. */
//     static int32 GetMaxMobileFrameRate();
//
//     /** 지정된 FPS가 지원되는 모바일 프레임 페이스인지 확인합니다. */
//     static bool IsSupportedMobileFramePace(int32 TestFPS);
//
//     // 현재 디바이스 프로파일에서 전체 품질이 제한되는 첫 번째 프레임레이트를 반환합니다.
//     int32 GetFirstFrameRateWithQualityLimit() const;
//
//     // 전체 프레임레이트 제한이 있는 최저 품질 수준을 반환합니다(제한 없으면 -1).
//     int32 GetLowestQualityWithFrameRateLimit() const;
//
//     /** 모바일 디바이스 기본값으로 재설정합니다. */
//     void ResetToMobileDeviceDefaults();
//
//     /** 지원되는 최대 전체 품질 수준을 반환합니다. */
//     int32 GetMaxSupportedOverallQualityLevel() const;
//
// private:
//     /** 모바일 FPS 모드를 설정합니다. */
//     void SetMobileFPSMode(int32 NewLimitFPS);
//
//     /** 대상 FPS에 따라 모바일 해상도 품질을 클램프합니다. */
//     void ClampMobileResolutionQuality(int32 TargetFPS);
//     /** FromFPS에서 ToFPS로 모바일 해상도 품질을 재매핑합니다. */
//     void RemapMobileResolutionQuality(int32 FromFPS, int32 ToFPS);
//
//     /** 모바일 FPS 품질 수준을 클램프합니다. */
//     void ClampMobileFPSQualityLevels(bool bWriteBack);
//     /** 모바일 품질을 클램프합니다. */
//     void ClampMobileQuality();
//     
//     /** 모든 스케일러빌리티 채널의 최고 수준을 반환합니다. */
//     int32 GetHighestLevelOfAnyScalabilityChannel() const;
//
//     /* 활성 모드의 오버라이드를 기반으로 입력 수준을 수정합니다. */
//     void OverrideQualityLevelsToScalabilityMode(const FLeeScalabilitySnapshot& InMode, Scalability::FQualityLevels& InOutLevels);
//
//     /* 활성 디바이스 프로파일의 기본 허용 수준에 따라 입력 수준을 클램프합니다. */
//     void ClampQualityLevelsToDeviceProfile(const Scalability::FQualityLevels& ClampLevels, Scalability::FQualityLevels& InOutLevels);
//
// public:
//     /** 원하는 모바일 프레임레이트 제한을 반환합니다. */
//     int32 GetDesiredMobileFrameRateLimit() const { return DesiredMobileFrameRateLimit; }
//
//     void SetDesiredMobileFrameRateLimit(int32 NewLimitFPS);
//
// private:
//     UPROPERTY(Config)
//     int32 MobileFrameRateLimit = 30;
//
//     FLeeScalabilitySnapshot DeviceDefaultScalabilitySettings;
//
//     bool bSettingOverallQualityGuard = false;
//
//     int32 DesiredMobileFrameRateLimit = 0;
//
// private:
//
//     //////////////////////////////////////////////////////////////////
//     // Display - Console quality presets (디스플레이 - 콘솔 품질 프리셋)
// public:
//     UFUNCTION()
//     FString GetDesiredDeviceProfileQualitySuffix() const;
//     UFUNCTION()
//     void SetDesiredDeviceProfileQualitySuffix(const FString& InDesiredSuffix);
//
// protected:
//     /** 현재 게임 모드에 맞게 디바이스 프로파일, FPS 모드 등을 업데이트합니다. */
//     void UpdateGameModeDeviceProfileAndFps();
//
//     /** 콘솔 프레임 페이싱을 업데이트합니다. */
//     void UpdateConsoleFramePacing();
//     /** 데스크톱 프레임 페이싱을 업데이트합니다. */
//     void UpdateDesktopFramePacing();
//     /** 모바일 프레임 페이싱을 업데이트합니다. */
//     void UpdateMobileFramePacing();
//
//     /** 동적 해상도 프레임 타임을 업데이트합니다. */
//     void UpdateDynamicResFrameTime(float TargetFPS);
//
// private:
//     UPROPERTY(Transient)
//     FString DesiredUserChosenDeviceProfileSuffix;
//
//     UPROPERTY(Transient)
//     FString CurrentAppliedDeviceProfileOverrideSuffix;
//
//     UPROPERTY(config)
//     FString UserChosenDeviceProfileSuffix;
//
//     //////////////////////////////////////////////////////////////////
//     // Audio - Volume (오디오 - 볼륨)
// public:
//     DECLARE_EVENT_OneParam(ULeeSettingsLocal, FAudioDeviceChanged, const FString& /*DeviceId*/);
//     FAudioDeviceChanged OnAudioOutputDeviceChanged;
//
// public:
//     /** 헤드폰 모드(HRTF) 사용 여부를 반환합니다. */
//     UFUNCTION()
//     bool IsHeadphoneModeEnabled() const;
//
//     /** 헤드폰 모드(HRTF)를 활성화/비활성화합니다. (au.DisableBinauralSpatialization 설정에 의해 무시될 수 있음) */
//     UFUNCTION()
//     void SetHeadphoneModeEnabled(bool bEnabled);
//
//     /** 헤드폰 모드를 변경할 수 있는지 반환합니다(플랫폼에 의해 강제되지 않은 경우). */
//     UFUNCTION()
//     bool CanModifyHeadphoneModeEnabled() const;
//
// public:
//     /** 헤드폰 모드(HRTF)를 *원할 때* 사용 여부; 실제 적용 여부와 무관 */
//     UPROPERTY(Transient)
//     bool bDesiredHeadphoneMode;
//
// private:
//     /** 헤드폰 모드(HRTF) 사용 여부 */
//     UPROPERTY(config)
//     bool bUseHeadphoneMode;
//
// public:
//     /** High Dynamic Range Audio 모드(HDR Audio) 사용 여부를 반환합니다. */
//     UFUNCTION()
//     bool IsHDRAudioModeEnabled() const;
//
//     /** High Dynamic Range Audio 모드(HDR Audio)를 활성화/비활성화합니다. */
//     UFUNCTION()
//     void SetHDRAudioModeEnabled(bool bEnabled);
//
//     /** High Dynamic Range Audio 모드(HDR Audio) 사용 여부 */
//     UPROPERTY(config)
//     bool bUseHDRAudioMode;
//
// public:
//     /** 이 플랫폼이 자동 벤치마크를 실행할 수 있는지 반환합니다. */
//     UFUNCTION(BlueprintCallable, Category = Settings)
//     bool CanRunAutoBenchmark() const;
//
//     /** 사용자가 자동 벤치마크를 처음 실행해야 하는지 반환합니다. */
//     UFUNCTION(BlueprintCallable, Category = Settings)
//     bool ShouldRunAutoBenchmarkAtStartup() const;
//
//     /** 자동 벤치마크를 실행하고 선택적으로 즉시 저장합니다. */
//     UFUNCTION(BlueprintCallable, Category = Settings)
//     void RunAutoBenchmark(bool bSaveImmediately);
//
//     /** 품질 스케일러빌리티 설정만 적용합니다. */
//     void ApplyScalabilitySettings();
//
//     UFUNCTION()
//     float GetOverallVolume() const;
//     UFUNCTION()
//     void SetOverallVolume(float InVolume);
//
//     UFUNCTION()
//     float GetMusicVolume() const;
//     UFUNCTION()
//     void SetMusicVolume(float InVolume);
//
//     UFUNCTION()
//     float GetSoundFXVolume() const;
//     UFUNCTION()
//     void SetSoundFXVolume(float InVolume);
//
//     UFUNCTION()
//     float GetDialogueVolume() const;
//     UFUNCTION()
//     void SetDialogueVolume(float InVolume);
//
//     UFUNCTION()
//     float GetVoiceChatVolume() const;
//     UFUNCTION()
//     void SetVoiceChatVolume(float InVolume);
//
//     //////////////////////////////////////////////////////////////////
//     // Audio - Sound (오디오 - 사운드)
// public:
//     /** 사용자 오디오 출력 디바이스 ID를 반환합니다. */
//     UFUNCTION()
//     FString GetAudioOutputDeviceId() const { return AudioOutputDeviceId; }
//
//     /** 사용자 오디오 디바이스를 ID로 설정합니다. */
//     UFUNCTION()
//     void SetAudioOutputDeviceId(const FString& InAudioOutputDeviceId);
//
// private:
//     UPROPERTY(Config)
//     FString AudioOutputDeviceId;
//     
//     /** 사운드 클래스에 볼륨을 설정합니다. */
//     void SetVolumeForSoundClass(FName ChannelName, float InVolume);
//     
//
//     //////////////////////////////////////////////////////////////////
//     // Safezone (안전 영역)
// public:
//     UFUNCTION()
//     bool IsSafeZoneSet() const { return SafeZoneScale != -1; }
//     UFUNCTION()
//     float GetSafeZone() const { return SafeZoneScale >= 0 ? SafeZoneScale : 0; }
//     UFUNCTION()
//     void SetSafeZone(float Value) { SafeZoneScale = Value; ApplySafeZoneScale(); }
//
//     /** 안전 영역 스케일을 적용합니다. */
//     void ApplySafeZoneScale();
// private:
//     /** 사운드 컨트롤 버스에 볼륨을 설정합니다. */
//     void SetVolumeForControlBus(USoundControlBus* InSoundControlBus, float InVolume);
//
//     //////////////////////////////////////////////////////////////////
//     // Keybindings (키 바인딩)
// public:
//     
//     // 컨트롤러 표현을 설정합니다. 한 플랫폼에서 여러 컨트롤러 종류를 지원할 수 있습니다.
//     // 예: Win64 게임은 Xbox 또는 Playstation 컨트롤러로 플레이 가능.
//     UFUNCTION()
//     void SetControllerPlatform(const FName InControllerPlatform);
//     UFUNCTION()
//     FName GetControllerPlatform() const;
//
// private:
//     /** 사용자 컨트롤 버스 믹스를 로드합니다. */
//     void LoadUserControlBusMix();
//
//     UPROPERTY(Config)
//     float OverallVolume = 1.0f;
//     UPROPERTY(Config)
//     float MusicVolume = 1.0f;
//     UPROPERTY(Config)
//     float SoundFXVolume = 1.0f;
//     UPROPERTY(Config)
//     float DialogueVolume = 1.0f;
//     UPROPERTY(Config)
//     float VoiceChatVolume = 1.0f;
//
//     UPROPERTY(Transient)
//     TMap<FName/*SoundClassName*/, TObjectPtr<USoundControlBus>> ControlBusMap;
//
//     UPROPERTY(Transient)
//     TObjectPtr<USoundControlBusMix> ControlBusMix = nullptr;
//
//     UPROPERTY(Transient)
//     bool bSoundControlBusMixLoaded;
//
//     UPROPERTY(Config)
//     float SafeZoneScale = -1;
//
//     /**
//      * 플레이어가 사용하는 컨트롤러 이름. 현재 플랫폼에서 사용 가능한 UCommonInputBaseControllerData 이름에 매핑됩니다.
//      * 게임패드 데이터는 <Platform>Game.ini 파일의 +ControllerData=...에 등록됩니다.
//      */
//     UPROPERTY(Config)
//     FName ControllerPlatform;
//
//     UPROPERTY(Config)
//     FName ControllerPreset = TEXT("Default");
//
//     /** 사용자가 선택한 현재 입력 구성 이름. */
//     UPROPERTY(Config)
//     FName InputConfigName = TEXT("Default");
//
//     // Replays (리플레이)
// public:
//
//     UFUNCTION()
//     bool ShouldAutoRecordReplays() const { return bShouldAutoRecordReplays; }
//     UFUNCTION()
//     void SetShouldAutoRecordReplays(bool bEnabled) { bShouldAutoRecordReplays = bEnabled;}
//
//     UFUNCTION()
//     int32 GetNumberOfReplaysToKeep() const { return NumberOfReplaysToKeep; }
//     UFUNCTION()
//     void SetNumberOfReplaysToKeep(int32 InNumberOfReplays) { NumberOfReplaysToKeep = InNumberOfReplays; }
//
// private:
//
//     UPROPERTY(Config)
//     bool bShouldAutoRecordReplays = false;
//
//     UPROPERTY(Config)
//     int32 NumberOfReplaysToKeep = 5;
//
// private:
//     /** 앱 활성화 상태 변경 시 호출됩니다. */
//     void OnAppActivationStateChanged(bool bIsActive);
//     /** 가능한 디바이스 프로파일 변경으로 인해 재적용합니다. */
//     void ReapplyThingsDueToPossibleDeviceProfileChange();
//
// private:
//     FDelegateHandle OnApplicationActivationStateChangedHandle;
// };