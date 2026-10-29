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

};
