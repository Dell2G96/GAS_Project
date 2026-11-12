// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonPlayerController.h"
#include "ModularPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "LeePlayerController.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ALeePlayerController : public ACommonPlayerController
{
	GENERATED_BODY()
public:
	ALeePlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

	class ALeePlayerState* GetLeePlayerState() const;
	class ULeeAbilitySystemComponent* GetLeeAbilitySystemComponent() const;

	// 사망 화면에서 '예'를 눌렀을 때 서버에 부활 요청 (실제 처리는 ULeeSavePointSubsystem::RequestRespawn)
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lee|Death")
	void ServerRequestRespawn();

	// 26.11.02 추가 - 보스전 관전 중 다음 생존 동료로 카메라 전환 요청 (좌클릭 / BP에서도 호출 가능)
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lee|Spectate")
	void ServerSpectateNextTeammate();

	// 26.11.02 추가 - 보스 로비에서 준비 / 준비 취소 요청 (로비 HUD의 [준비] 버튼)
	UFUNCTION(Server, Reliable)
	void ServerSetBossLobbyReady(bool bReady);

	// 26.11.02 추가 - 보스 처치 후 서버가 각 클라이언트에게 자기 필드맵으로 돌아가라고 지시
	UFUNCTION(Client, Reliable)
	void ClientReturnToField();

	// 26.11.12 추가 - 첫 도전 보스전 시작 시 서버가 각 클라이언트에게 자기 세이브에 도전 기록을 남기라고 지시
	UFUNCTION(Client, Reliable)
	void ClientRecordBossAttempt(FName BossId);

	// 26.11.12 추가 - 보스 처치 시 서버가 각 클라이언트에게 자기 세이브에 처치 기록을 남기라고 지시
	UFUNCTION(Client, Reliable)
	void ClientRecordBossDefeated(FName BossId);

protected:
	virtual void SetupInputComponent() override;

	// 좌클릭: 사망 중일 때만 관전 대상 전환을 요청
	void HandleSpectateNextInput();

};
