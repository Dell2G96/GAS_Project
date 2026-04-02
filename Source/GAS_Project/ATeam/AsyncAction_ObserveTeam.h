// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/CancellableAsyncAction.h"
#include "UObject/ScriptInterface.h"
#include "UObject/WeakInterfacePtr.h"

#include "AsyncAction_ObserveTeam.generated.h"

class ILeeTeamAgentInterface;
struct FFrame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTeamObservedAsyncDelegate, bool, bTeamSet, int32, TeamId);

UCLASS()
class GAS_PROJECT_API UAsyncAction_ObserveTeam : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	/** 
	 * 지정된 팀 에이전트의 팀 변경을 감시합니다
	 * - 즉시 현재 팀 할당 상태를 한 번 호출합니다
	 * - 팀에 속할 수 있는 모든 객체(ILyraTeamAgentInterface 구현)에 대해
	 *   향후 팀 할당 변경도 계속해서 감시합니다
	 */
	UAsyncAction_ObserveTeam(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", Keywords="Watch"))
	static UAsyncAction_ObserveTeam* ObserveTeam(UObject* TeamAgent);


	// UBlueprintAsysncAction interface
	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;
	// UBlueprintAsysncAction interface

public:
	// 팀이 설정되거나 변경될 때 호출
	UPROPERTY(BlueprintAssignable)
	FTeamObservedAsyncDelegate OnTeamChanged;

private:
	// 지정된 팀 액터의 팀 변경을 감시
	static UAsyncAction_ObserveTeam* InternalObserveTeamChanges(TScriptInterface<ILeeTeamAgentInterface> TeamActor);

private:
	// 감시하던 에이전트의 팀이 변경되었을 떄 호출되는 콜백
	UFUNCTION()
	void OnWatchedAgentChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	// 팀 인터페이스에 대한 포인터
	TWeakInterfacePtr<ILeeTeamAgentInterface> TeamInterfacePtr;
	
};





















