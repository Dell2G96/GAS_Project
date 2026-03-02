// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LeeInventoryItemDefinition.generated.h"

UCLASS(Abstract, DefaultToInstanced, EditInlineNew)
class ULeeInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:
	
};


UCLASS(Blueprintable)
class GAS_PROJECT_API ULeeInventoryItemDefinition : public UObject
{
	GENERATED_BODY()
public:
	ULeeInventoryItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	const ULeeInventoryItemFragment* FindFragmentByClass(TSubclassOf<ULeeInventoryItemFragment> FragmentClass) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Display)
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category= Display)
	TArray<TObjectPtr<ULeeInventoryItemFragment>> Fragments;
};
