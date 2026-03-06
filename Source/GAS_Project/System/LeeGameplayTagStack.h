// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "LeeGameplayTagStack.generated.h"

USTRUCT(BlueprintType)
struct FLeeGameplayTagStack
{
	GENERATED_BODY()

	FLeeGameplayTagStack(){}
	FLeeGameplayTagStack(FGameplayTag InTag, int32 InStackCount)
		:Tag(InTag)
		, StackCount(InStackCount)
	{}

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	int32 StackCount = 0;
};

USTRUCT(BlueprintType)
struct FLeeGameplayTagStackContainer
{
	GENERATED_BODY()

	FLeeGameplayTagStackContainer(){}

	void AddStack(FGameplayTag Tag, int32 StackCount);
	void RemoveStack(FGameplayTag Tag, int32 StackCount);

	int32 GetStackCount(FGameplayTag Tag) const
	{
		return TagToCountMap.FindRef(Tag);
	}

	bool ContainsTag(FGameplayTag Tag) const
	{
		return TagToCountMap.Contains(Tag);
	}

	UPROPERTY()
	TArray<FLeeGameplayTagStack> Stacks;
	TMap<FGameplayTag, int32> TagToCountMap;
};