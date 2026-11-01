// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "LeeGameSession.generated.h"

UCLASS(Config = Game)
class GAS_PROJECT_API ALeeGameSession : public AGameSession
{
	GENERATED_BODY()

public:
	ALeeGameSession(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	/** Override to disable the default behavior */
	virtual bool ProcessAutoLogin() override;

	// 원격 접속 승인 판단: 필드(SingleField)는 거부, 보스전은 인원 제한 확인
	virtual FString ApproveLogin(const FString& Options) override;

	virtual void BeginPlay() override;

	// Experience 로드 완료 후, 로딩 중에 접속한 원격 플레이어를 PlayMode 규칙으로 다시 검사
	void OnExperienceLoaded(const class ULeeExperienceDefinition* Experience);

	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;

};
