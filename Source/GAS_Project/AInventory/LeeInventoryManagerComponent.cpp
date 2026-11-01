// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeInventoryManagerComponent.h"

#include "LeeInventoryItemDefinition.h"
#include "LeeInventoryItemInstance.h"
#include "Net/UnrealNetwork.h"


//////////////////////////////////////////////////////////////////////
// FLyraInventoryEntry
FString FLeeInventoryEntry::GetDebugString() const
{
	TSubclassOf<ULeeInventoryItemDefinition> ItemDef;
	if (Instance != nullptr)
	{
		ItemDef = Instance->GetItemDef();
	}
	return FString::Printf(TEXT("%s (%d x %s)"), *GetNameSafe(Instance), StackCount, *GetNameSafe(ItemDef));
	
}


//////////////////////////////////////////////////////////////////////
// FLyraInventoryList

void FLeeInventoryList::AddEntry(ULeeInventoryItemInstance* Instance)
{
	unimplemented();
}

void FLeeInventoryList::RemoveEntry(ULeeInventoryItemInstance* Instance)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FLeeInventoryEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			EntryIt.RemoveCurrent();

			// 26.11.01 수정 - 항목 제거를 클라이언트로 복제하도록 표시
			MarkArrayDirty();
		}
	}
	
}


TArray<ULeeInventoryItemInstance*> FLeeInventoryList::GetAllItem() const
{
	TArray<ULeeInventoryItemInstance*> Results;
	Results.Reserve(Entries.Num());
	for (const FLeeInventoryEntry& Entry : Entries)
	{
		if (Entry.Instance != nullptr) //@TODO: Would prefer to not deal with this here and hide it further?
		{
			Results.Add(Entry.Instance);
		}
	}
	return Results;
}



ULeeInventoryItemInstance* FLeeInventoryList::AddEntry(TSubclassOf<class ULeeInventoryItemDefinition> ItemDef, int32 StackCount)
{
	ULeeInventoryItemInstance* Result = nullptr;
	
	check(ItemDef);
	check(OwnerComponent);

	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	FLeeInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	
	//NewEntry.Instance = NewObject<ULeeInventoryItemInstance>(OwningActor);
	NewEntry.Instance = NewObject<ULeeInventoryItemInstance>(OwnerComponent->GetOwner());
	//NewEntry.Instance->ItemDef = ItemDef;
	NewEntry.Instance->SetItemDef(ItemDef);

	for (const ULeeInventoryItemFragment* Fragment : GetDefault<ULeeInventoryItemDefinition>(ItemDef)->Fragments)
	{
		if (Fragment)
		{
			Fragment->OnInstanceCreated(NewEntry.Instance);
		}
	}
	NewEntry.StackCount = StackCount;
	Result = NewEntry.Instance;

	// 26.11.01 추가 - 새 항목을 클라이언트로 복제하도록 표시
	MarkItemDirty(NewEntry);
	
	
	return Result;
	
}

//////////////////////////////////////////////////////////////////////
// ULeeInventoryManagerComponent

ULeeInventoryManagerComponent::ULeeInventoryManagerComponent(const FObjectInitializer& InObjectInitializer)
	:Super(InObjectInitializer)
	,InventoryList(this) 
{
	// 26.11.01 추가 - 인벤토리 목록과 아이템 인스턴스(서브오브젝트)를 클라이언트로 복제
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

// 복제 준비가 끝난 시점에, 이미 들고 있는 아이템 인스턴스를 서브오브젝트로 등록한다
void ULeeInventoryManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	if (IsUsingRegisteredSubObjectList())
	{
		for (const FLeeInventoryEntry& Entry : InventoryList.Entries)
		{
			if (ULeeInventoryItemInstance* Instance = Entry.Instance)
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

void ULeeInventoryManagerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
}



ULeeInventoryItemInstance* ULeeInventoryManagerComponent::AddItemDefinition(
	TSubclassOf<class ULeeInventoryItemDefinition> ItemDef, int32 StackCount)
{
	ULeeInventoryItemInstance* Result = nullptr;
	if (ItemDef)
	{
		Result = InventoryList.AddEntry(ItemDef, StackCount);

		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && Result)
		{
			AddReplicatedSubObject(Result);	
		}
	}
	return Result;
}

void ULeeInventoryManagerComponent::AddItemInstance(ULeeInventoryItemInstance* ItemInstance)
{
	InventoryList.AddEntry(ItemInstance);
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && ItemInstance)
	{
		AddReplicatedSubObject(ItemInstance);
	}
}

void ULeeInventoryManagerComponent::RemoveItemInstance(ULeeInventoryItemInstance* ItemInstance)
{
	InventoryList.RemoveEntry(ItemInstance);

	if (ItemInstance && IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(ItemInstance);
	}
}

TArray<ULeeInventoryItemInstance*> ULeeInventoryManagerComponent::GetAllItems() const
{
	return InventoryList.GetAllItem();

}
