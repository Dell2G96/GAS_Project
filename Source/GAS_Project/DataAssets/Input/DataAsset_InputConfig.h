// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "DataAsset_InputConfig.generated.h"

USTRUCT(BlueprintType)
struct FCInputActionConfig
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "A_|InputConfig")
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "A_|InputConfig")
	class UInputAction* InputAction;
};

UCLASS()
class GAS_PROJECT_API UDataAsset_InputConfig : public UDataAsset
{
	GENERATED_BODY()


public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UInputMappingContext* MappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(TitleProperty = "InputTag"))
	TArray<FCInputActionConfig> NativeInputActions;

	class UInputAction* FindNativeInputActionByTag(const FGameplayTag InInputTag) const;
};
