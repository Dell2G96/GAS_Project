// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeEquipmentManagerComponent.h"

#include "AbilitySystemGlobals.h"
#include "LeeEquipmentDefinition.h"
#include "LeeEquipmentInstance.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/ACharacter/LeePawnExtensionComponent.h"
#include "Net/UnrealNetwork.h"

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

	// 26.11.01 추가 - 새 항목을 클라이언트로 복제하도록 표시
	MarkItemDirty(NewEntry);

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

			// 26.11.01 추가 - 항목 제거를 클라이언트로 복제하도록 표시
			MarkArrayDirty();
		}
	}
}

// 클라이언트에서 장비 항목이 제거되기 직전 호출. 장착 해제 연출(애니메이션 레이어 복구 등)을 실행한다
void FLeeEquipmentList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		const FLeeAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance != nullptr)
		{
			Entry.Instance->OnUnEquipped();
		}
	}
}

// 클라이언트에서 장비 항목이 추가된 직후 호출. 서버의 EquipItem처럼 OnEquipped(애니메이션 레이어 연결 등)를 실행한다
void FLeeEquipmentList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FLeeAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance != nullptr)
		{
			Entry.Instance->OnEquipped();
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

	// 26.11.01 추가 - 장비 목록과 장비 인스턴스(서브오브젝트)를 클라이언트로 복제
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

// 복제할 프로퍼티 등록
void ULeeEquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, EquipmentList);
}

// 복제 준비가 끝난 시점에, 이미 장착된 장비 인스턴스를 서브오브젝트로 등록한다
void ULeeEquipmentManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	if (IsUsingRegisteredSubObjectList())
	{
		for (const FLeeAppliedEquipmentEntry& Entry : EquipmentList.Entries)
		{
			if (ULeeEquipmentInstance* Instance = Entry.Instance)
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
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

			// 26.11.01 추가 - 장비 인스턴스를 클라이언트로 복제하도록 서브오브젝트로 등록
			if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
			{
				AddReplicatedSubObject(Result);
			}
		}
	}
	return Result;
}

void ULeeEquipmentManagerComponent::UnEquipItem(ULeeEquipmentInstance* ItemInstance)
{
	if (ItemInstance)
	{
		// 26.11.01 추가 - 서브오브젝트 복제 목록에서 제거
		if (IsUsingRegisteredSubObjectList())
		{
			RemoveReplicatedSubObject(ItemInstance);
		}

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
