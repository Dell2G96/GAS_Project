// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS_Project/System/LeeGameplayTagStack.h"
#include "UObject/Object.h"
#include "LeeInventoryItemInstance.generated.h"

class ULeeInventoryItemFragment;
/**
 * 
 */
UCLASS(BlueprintType)
class GAS_PROJECT_API ULeeInventoryItemInstance : public UObject
{
	GENERATED_BODY()
public:
	ULeeInventoryItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 26.11.01 추가 - 서브오브젝트로 클라이언트에 복제될 수 있도록 허용
	virtual bool IsSupportedForNetworking() const override { return true; }

	UFUNCTION(BlueprintCallable, blueprintPure = false, meta=(DeterminesOutputType = FragmentClass))
	const ULeeInventoryItemFragment* FindFragmentByClass(TSubclassOf<ULeeInventoryItemFragment> FragmentClass) const;

	UFUNCTION(BlueprintCallable, Category=Inventory)
	TSubclassOf<class ULeeInventoryItemDefinition> GetItemDef() const
	{
		return ItemDef;
	}

	template<typename  ResultClass>
	const ResultClass* FindFragmentByClass() const
	{
		return (ResultClass*)FindFragmentByClass(ResultClass::StaticClass());
	}

	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);
	void RemoveStatTagstack(FGameplayTag Tag, int32 StackCount);

	bool HasStatTag(FGameplayTag Tag) const;
	
	void SetItemDef(TSubclassOf<ULeeInventoryItemDefinition> InDef);

	
	
	
	
	UFUNCTION(BlueprintCallable, Category= Inventory)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	UPROPERTY()
	FLeeGameplayTagStackContainer StatTags;

	UPROPERTY(Replicated)
	TSubclassOf<class ULeeInventoryItemDefinition> ItemDef;
};
