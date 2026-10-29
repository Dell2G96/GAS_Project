// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "ModularPlayerController.h"
#include "GAS_Project/ATeam/LeeTeamAgentInterface.h"
#include "LeePlayerBotController.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class GAS_PROJECT_API ALeePlayerBotController : public AModularAIController, public ILeeTeamAgentInterface
{
	GENERATED_BODY()

public:
	ALeePlayerBotController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ILeeTeamAgentInterface 
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnLeeTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	// ~ILeeTeamAgentInterface

	void ServerRestartController();

	UFUNCTION(BlueprintCallable, Category= "Lee AI Player Controller")
	void UpdateTeamAttitude(UAIPerceptionComponent* AIPerception);

	virtual void OnUnPossess() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=BotIdentifier)
	int32 BotIdentifier;

	// 26.10.29 추가 - 처음 스폰된 PlayerStart. 리셋(휴식/사망) 후 재시작 시 항상 이 자리에서 다시 생성된다
	// (레거시: 기존 몹별 스포너(ULeeBotCreationComponent)로 생성된 봇만 사용. 새 방식은 HomeSpawnPoint)
	UPROPERTY(Transient)
	TWeakObjectPtr<class ALeePlayerStart> HomePlayerStart;

	// 26.10.29 추가 - 이 봇을 생성한 스폰 지점. 처음 스폰과 리셋 후 재시작 모두 이 자리에서 생성된다
	UPROPERTY(Transient)
	TWeakObjectPtr<class ALeeEnemySpawnPoint> HomeSpawnPoint;

	// 26.10.29 추가 - 이 Controller가 조종할 몹의 PawnData (메시·어빌리티). 비어 있으면 레거시 Experience EnemyPawnClasses[BotIdentifier] 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|Enemy")
	TObjectPtr<const class ULeePawnData> EnemyPawnData;

	// 26.10.29 추가 - 네임플레이트에 표시할 몹 이름 기본값 (스폰 지점의 DisplayNameOverride가 있으면 그 값을 사용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|Enemy")
	FText EnemyDisplayName;

private:
	UFUNCTION()
	void OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

protected:
	virtual void OnPlayerStateChanged();

private:
	void BroadcastOnPlayerStateChanged();

protected:
	// AController Interface
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	// ~AController

private:
	UPROPERTY()
	FOnLeeTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;

	
};
