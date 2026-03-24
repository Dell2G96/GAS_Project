// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Subsystems/WorldSubsystem.h"

#include "LeeTeamSubsystem.generated.h"


class AActor;
class ALeePlayerState;
class ALeeTeamInfoBase;
class ALeeTeamPrivateInfo;
class ALeeTeamPublicInfo;
class FSubsystemCollectionBase;
class ULeeTeamDisplayAsset;
struct FFrame;
struct FGameplayTag;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLeeTeamDisplayAssetChangedDelegate, const ULeeTeamDisplayAsset*, DisplayAsset);

USTRUCT()
struct FLeeTeamTrackingInfo
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TObjectPtr<ALeeTeamPublicInfo> PublicInfo = nullptr;

    UPROPERTY()
    TObjectPtr<ALeeTeamPrivateInfo> PrivateInfo = nullptr;

    UPROPERTY()
    TObjectPtr<ULeeTeamDisplayAsset> DisplayAsset = nullptr;

    UPROPERTY()
    FOnLeeTeamDisplayAssetChangedDelegate OnTeamDisplayAssetChanged;

public:
    void SetTeamInfo(ALeeTeamInfoBase* Info);
    void RemoveTeamInfo(ALeeTeamInfoBase* Info);
};

// 두 액터의 팀 소속을 비교한 결과
UENUM(BlueprintType)
enum class ELeeTeamComparison : uint8
{
    // 두 액터가 동일한 팀 소속
    OnSameTeam,

    // 두 액터가 서로 다른 팀 소속
    DifferentTeams,

    // 액터 중 하나 또는 둘 다 유효하지 않거나 어떤 팀에도 속하지 않음
    InvalidArgument
};

/** 팀 기반 액터(폰, 플레이어 스테이트 등)의 팀 정보에 쉽게 접근하기 위한 월드 서브시스템 */
UCLASS(MinimalAPI)
class ULeeTeamSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
     ULeeTeamSubsystem();

    //~USubsystem 인터페이스
     virtual void Initialize(FSubsystemCollectionBase& Collection) override;
     virtual void Deinitialize() override;
    //~USubsystem 인터페이스 끝

    // 새로운 팀 등록을 시도합니다
     bool RegisterTeamInfo(ALeeTeamInfoBase* TeamInfo);

    // 팀 등록 해제를 시도합니다. 실패 시 false를 반환합니다
     bool UnregisterTeamInfo(ALeeTeamInfoBase* TeamInfo);

    // 가능한 경우 해당 액터의 팀을 변경합니다
    // 주의: 이 함수는 Authority에서만 호출할 수 있습니다
     bool ChangeTeamForActor(AActor* ActorToChange, int32 NewTeamId);

    // 해당 오브젝트가 속한 팀을 반환합니다. 팀에 속하지 않은 경우 INDEX_NONE을 반환합니다
     int32 FindTeamFromObject(const UObject* TestObject) const;

    // 해당 액터와 연관된 플레이어 스테이트를 반환합니다. 없을 경우 INDEX_NONE을 반환합니다
     const ALeePlayerState* FindPlayerStateFromActor(const AActor* PossibleTeamActor) const;

    // 해당 오브젝트가 속한 팀을 반환합니다. 팀에 속하지 않은 경우 INDEX_NONE을 반환합니다
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Teams, meta=(Keywords="Get"))
     void FindTeamFromActor(const UObject* TestActor, bool& bIsPartOfTeam, int32& TeamId) const;

    // 두 액터의 팀을 비교하여 같은 팀, 다른 팀, 또는 유효하지 않은 경우를 나타내는 값을 반환합니다
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Teams, meta=(ExpandEnumAsExecs=ReturnValue))
     ELeeTeamComparison CompareTeams(const UObject* A, const UObject* B, int32& TeamIdA, int32& TeamIdB) const;

    // 두 액터의 팀을 비교하여 같은 팀, 다른 팀, 또는 유효하지 않은 경우를 나타내는 값을 반환합니다
     ELeeTeamComparison CompareTeams(const UObject* A, const UObject* B) const;

    // 아군 공격 설정을 고려하여 Instigator가 Target에게 데미지를 줄 수 있는지 반환합니다
     bool CanCauseDamage(const UObject* Instigator, const UObject* Target, bool bAllowDamageToSelf = true) const;

    // 지정된 팀 태그에 스택을 추가합니다 (StackCount가 1 미만이면 아무 작업도 하지 않음)
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
     void AddTeamTagStack(int32 TeamId, FGameplayTag Tag, int32 StackCount);

    // 지정된 팀 태그에서 스택을 제거합니다 (StackCount가 1 미만이면 아무 작업도 하지 않음)
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
     void RemoveTeamTagStack(int32 TeamId, FGameplayTag Tag, int32 StackCount);

    // 지정된 태그의 스택 수를 반환합니다 (태그가 없으면 0 반환)
    UFUNCTION(BlueprintCallable, Category=Teams)
     int32 GetTeamTagStackCount(int32 TeamId, FGameplayTag Tag) const;

    // 지정된 태그의 스택이 하나 이상 있으면 true를 반환합니다
    UFUNCTION(BlueprintCallable, Category=Teams)
     bool TeamHasTag(int32 TeamId, FGameplayTag Tag) const;

    // 지정된 팀이 존재하면 true를 반환합니다
    UFUNCTION(BlueprintCallable, Category=Teams)
     bool DoesTeamExist(int32 TeamId) const;

    // 관찰자의 시점에서 지정된 팀의 팀 디스플레이 에셋을 가져옵니다
    // (로컬 플레이어가 항상 블루팀인 상황 등에 대응하기 위해 관찰자도 지정해야 합니다)
    UFUNCTION(BlueprintCallable, Category=Teams)
     ULeeTeamDisplayAsset* GetTeamDisplayAsset(int32 TeamId, int32 ViewerTeamId);

    // 관찰자의 시점에서 지정된 팀의 실제 적용될 팀 디스플레이 에셋을 가져옵니다
    // (로컬 플레이어가 항상 블루팀인 상황 등에 대응하기 위해 관찰자도 지정해야 합니다)
    UFUNCTION(BlueprintCallable, Category = Teams)
     ULeeTeamDisplayAsset* GetEffectiveTeamDisplayAsset(int32 TeamId, UObject* ViewerTeamAgent);

    // 팀 목록을 가져옵니다
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Teams)
     TArray<int32> GetTeamIDs() const;

    // 팀 디스플레이 에셋이 수정되었을 때 호출됩니다. 팀 색상을 구독 중인 모든 대상을 갱신합니다
     void NotifyTeamDisplayAssetModified(ULeeTeamDisplayAsset* ModifiedAsset);

    // 지정된 팀 ID의 팀 디스플레이 에셋 변경 알림에 등록합니다
     FOnLeeTeamDisplayAssetChangedDelegate& GetTeamDisplayAssetChangedDelegate(int32 TeamId);

private:
    UPROPERTY()
    TMap<int32, FLeeTeamTrackingInfo> TeamMap;

    FDelegateHandle CheatManagerRegistrationHandle;
};
