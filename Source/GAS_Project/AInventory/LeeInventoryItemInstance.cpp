// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeInventoryItemInstance.h"

#include "LeeInventoryItemDefinition.h"

ULeeInventoryItemInstance::ULeeInventoryItemInstance(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

const ULeeInventoryItemFragment* ULeeInventoryItemInstance::FindFragmentByClass(
	TSubclassOf<ULeeInventoryItemFragment> FragmentClass) const
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		// InventoryItemDefinition은 모든 멤버 변수가 EditDefaultsOnly로 선언되어 있으므로,
		// GetDefault로 가져와도 무관하다
		// - Fragment 정보는 Instance가 아닌 Definition에 있다
		return GetDefault<ULeeInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}
	return nullptr;
}

void ULeeInventoryItemInstance::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void ULeeInventoryItemInstance::RemoveStatTagstack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

bool ULeeInventoryItemInstance::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

int32 ULeeInventoryItemInstance::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}
