// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CommonGameViewportClient.h"

#include "LeeGameViewportClient.generated.h"


class UGameInstance;
class UObject;

UCLASS(BlueprintType)
class GAS_PROJECT_API ULeeGameViewportClient : public UCommonGameViewportClient
{
	GENERATED_BODY()

public:
	ULeeGameViewportClient();

	virtual void Init(struct FWorldContext& WorldContext, UGameInstance* OwningGameInstance, bool bCreateNewAudioDevice = true) override;
};
