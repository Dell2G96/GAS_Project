// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LeeInventoryItemInstance.generated.h"

class ULeeInventoryItemFragment;
/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeInventoryItemInstance : public UObject
{
	GENERATED_BODY()
public:
	ULeeInventoryItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	const ULeeInventoryItemFragment* FindFragmentByClass(TSubclassOf<ULeeInventoryItemFragment> FragmentClass) const;

	template<typename  ResultClass>
	const ResultClass* FindFragmentByClass() const
	{
		return (ResultClass*)FindFragmentByClass(ResultClass::StaticClass());
	}

	UPROPERTY()
	TSubclassOf<class ULeeInventoryItemDefinition> ItemDef;
};
