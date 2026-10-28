// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeEquipmentManagerComponent.h"

#include "AbilitySystemGlobals.h"
#include "LeeEquipmentDefinition.h"
#include "LeeEquipmentInstance.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/ACharacter/LeePawnExtensionComponent.h"

ULeeEquipmentInstance* FLeeEquipmentList::AddEntry(TSubclassOf<ULeeEquipmentDefinition> EquipmentDefinition)
{
	ULeeEquipmentInstance* Result = nullptr;
	check(EquipmentDefinition != nullptr);
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());

	const ULeeEquipmentDefinition* EquipmentCDO = GetDefault<ULeeEquipmentDefinition>(EquipmentDefinition);

	TSubclassOf<ULeeEquipmentInstance> InstanceType = EquipmentCDO->InstanceType;
	if (!InstanceType)
	{
		InstanceType = ULeeEquipmentInstance::StaticClass();
	}

	FLeeAppliedEquipmentEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.EquipmentDefinition = EquipmentDefinition;
	NewEntry.Instance = NewObject<ULeeEquipmentInstance>(OwnerComponent->GetOwner(), InstanceType);
	Result = NewEntry.Instance;

	ULeeAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	check(ASC);
	{
		for (const TObjectPtr<ULeeAbilitySet> AbilitySet : EquipmentCDO->AbilitySetsToGrant)
		{
			AbilitySet->GiveToAbilitySystem(ASC, &NewEntry.GrantedHandles, Result);
		}
	}

	Result->SpawnEquipmentActors(EquipmentCDO->ActorToSpawns);

	return Result;
}

void FLeeEquipmentList::RemoveEntry(ULeeEquipmentInstance* Instance)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FLeeAppliedEquipmentEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			// 26.10.29 수정 - Pawn이 이미 ASC를 잃은 뒤(파괴 직전)에도 장비 Actor는 정리되도록 check 대신 null 허용
			if (ULeeAbilitySystemComponent* ASC = GetAbilitySystemComponent())
			{
				Entry.GrantedHandles.TakeFromAbilitySystem(ASC);
			}

			Instance->DestroyEquipmentActors();
			EntryIt.RemoveCurrent();
		}
	}
}

ULeeAbilitySystemComponent* FLeeEquipmentList::GetAbilitySystemComponent() const
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();

	return Cast<ULeeAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor));
	
}

ULeeEquipmentManagerComponent::ULeeEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
	, EquipmentList(this)
{
	// 26.10.29 추가 - InitializeComponent / UninitializeComponent 호출을 받기 위해 필요
	bWantsInitializeComponent = true;
}

// Pawn의 ASC 해제 이벤트를 구독한다 (ASC는 PlayerState에 남으므로, Pawn이 바뀔 때 장비를 회수해야 한다)
void ULeeEquipmentManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(GetOwner()))
	{
		PawnExtComp->OnAbilitySystemUnInitialized_Registed(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleAbilitySystemUnInitialized));
	}
}

// 컴포넌트가 제거될 때 남은 장비를 모두 해제한다 (Lyra 원본과 동일)
void ULeeEquipmentManagerComponent::UninitializeComponent()
{
	UnEquipAllItems();

	Super::UninitializeComponent();
}

// 장착 중인 모든 장비를 해제한다. 장비 목록은 서버에서만 관리하므로 서버에서만 동작
void ULeeEquipmentManagerComponent::UnEquipAllItems()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// UnEquipItem이 Entries를 수정하므로 복사본으로 순회한다
	TArray<ULeeEquipmentInstance*> AllEquipmentInstances;
	for (const FLeeAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		AllEquipmentInstances.Add(Entry.Instance);
	}

	for (ULeeEquipmentInstance* EquipInstance : AllEquipmentInstances)
	{
		UnEquipItem(EquipInstance);
	}
}

// Pawn의 ASC 연결이 끊기는 순간(사망/교체) 장비를 해제해, PlayerState ASC에 장비 어빌리티가 남지 않게 한다
void ULeeEquipmentManagerComponent::HandleAbilitySystemUnInitialized()
{
	UnEquipAllItems();
}

class ULeeEquipmentInstance* ULeeEquipmentManagerComponent::EquipItem(
	TSubclassOf<class ULeeEquipmentDefinition> EquipmentDefinition)
{
	ULeeEquipmentInstance* Result = nullptr;
	if (EquipmentDefinition)
	{
		Result = EquipmentList.AddEntry(EquipmentDefinition);
		if (Result)
		{
			Result->OnEquipped();
		}
	}
	return Result;
}

void ULeeEquipmentManagerComponent::UnEquipItem(ULeeEquipmentInstance* ItemInstance)
{
	if (ItemInstance)
	{
		ItemInstance->OnUnEquipped();
		EquipmentList.RemoveEntry(ItemInstance);
	}
}

ULeeEquipmentInstance* ULeeEquipmentManagerComponent::GetFirstInstanceOfType(
	TSubclassOf<ULeeEquipmentInstance> InstanceType)
{
	for (FLeeAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (ULeeEquipmentInstance* Instance = Entry.Instance)
		{
			if (Instance->IsA(InstanceType))
			{
				return Instance;
			}
		}
	}
	return nullptr;
}

TArray<class ULeeEquipmentInstance*> ULeeEquipmentManagerComponent::GetEquipmentInstancesOfType(
	TSubclassOf<class ULeeEquipmentInstance> InstanceType) const
{
	TArray<ULeeEquipmentInstance*> Result;

	for (const FLeeAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (ULeeEquipmentInstance* Instance = Entry.Instance)
		{
			if (Instance->IsA(InstanceType))
			{
				Result.Add(Instance);
				
			}
		}
		
	}
	return Result;
}
