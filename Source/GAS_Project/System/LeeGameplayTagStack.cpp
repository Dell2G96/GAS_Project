// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameplayTagStack.h"

void FLeeGameplayTagStackContainer::AddStack(FGameplayTag Tag, int32 StackCount)
{
	if (!Tag.IsValid())
	{
		return;
	}
	if (StackCount > 0)
	{
		for (FLeeGameplayTagStack& Stack : Stacks )
		{
			if (Stack.Tag == Tag)
			{
				const int32 NewCount = Stack.StackCount + StackCount;
				Stack.StackCount = NewCount;
				TagToCountMap[Tag] = NewCount;
				return;
			}
		}

		FLeeGameplayTagStack& NewStack = Stacks.Emplace_GetRef(Tag, StackCount);

		TagToCountMap.Add(Tag, StackCount);
	}
	
}

void FLeeGameplayTagStackContainer::RemoveStack(FGameplayTag Tag, int32 StackCount)
{
	if (!Tag.IsValid())
	{
		return;
	}
	if (StackCount > 0)
	{
		for (auto It = Stacks.CreateIterator(); It; ++It)
		{
			FLeeGameplayTagStack& Stack = *It;
			if (Stack.Tag == Tag)
			{
				if (Stack.StackCount <= StackCount)
				{
					It.RemoveCurrent();
					TagToCountMap.Remove(Tag);
				}
				else
				{
					const int32 NewCount = Stack.StackCount - StackCount;
					Stack.StackCount = NewCount;
					TagToCountMap[Tag] = NewCount;
				}
				return;
			}
		}
	}
}
