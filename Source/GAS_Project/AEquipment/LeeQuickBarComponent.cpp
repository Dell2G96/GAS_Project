// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeQuickBarComponent.h"

#include "LeeEquipmentDefinition.h"
#include "LeeEquipmentInstance.h"
#include "LeeEquipmentManagerComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACharacter/LeePawnExtensionComponent.h"
#include "GAS_Project/AInventory/LeeInventoryFragment_EquippableItem.h"
#include "GAS_Project/AInventory/LeeInventoryItemInstance.h"
#include "Net/UnrealNetwork.h"


ULeeQuickBarComponent::ULeeQuickBarComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void ULeeQuickBarComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Slots);
	DOREPLIFETIME(ThisClass, ActiveSlotIndex);
}

void ULeeQuickBarComponent::BeginPlay()
{
	if (Slots.Num() < NumSlots)
	{
		Slots.AddDefaulted(NumSlots - Slots.Num());
	}

	Super::BeginPlay();

	// 26.10.29 추가 - 장비 장착은 서버에서만 하므로, 서버에서만 Pawn 교체(리스폰)를 감지한다
	if (AController* OwnerController = GetController<AController>())
	{
		if (OwnerController->HasAuthority())
		{
			OwnerController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::HandlePossessedPawnChanged);
		}
	}
}

// Controller의 Pawn이 바뀌면 이전 Pawn 장비 참조를 버리고, 새 Pawn의 ASC 초기화 후 재장착하도록 등록한다
void ULeeQuickBarComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// 이전 Pawn의 장비는 그 Pawn의 EquipmentManager가 ASC 해제 시점에 이미 정리한다. 여기서는 참조만 버린다
	// 주의: 이 이벤트는 Pawn의 Possessed(BP 초기 지급) 이후에 방송되므로, 새 Pawn에 이미 장착된 장비는 유지한다
	if (EquippedItem && EquippedItem->GetPawn() != NewPawn)
	{
		EquippedItem = nullptr;
	}

	if (ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(NewPawn))
	{
		// ASC가 이미 준비됐으면 즉시, 아니면 준비되는 순간 호출된다
		PawnExtComp->OnAbilitySystemInitialized_RegistedAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleNewPawnAbilitySystemInitialized));
	}
}

// 새 Pawn의 ASC가 준비되면 활성 슬롯 장비를 재장착한다 (최초 스폰처럼 아직 슬롯이 없거나 이미 장착됐으면 무시)
void ULeeQuickBarComponent::HandleNewPawnAbilitySystemInitialized()
{
	if (EquippedItem == nullptr && Slots.IsValidIndex(ActiveSlotIndex) && FindEquipmentManger())
	{
		EquipItemInSlot();
	}
}


void ULeeQuickBarComponent::CycleActiveSlotForward()
{
	if (Slots.Num() < 2)
	{
		return;
	}
	const int32 OldIndex = (ActiveSlotIndex < 0 ? Slots.Num() -1  : ActiveSlotIndex);
	int32 NewIndex = ActiveSlotIndex;
	do
	{
		NewIndex = (NewIndex + 1) % Slots.Num();
		if (Slots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex(NewIndex);
			return ;
		}
	} while (NewIndex != OldIndex);
}

void ULeeQuickBarComponent::CycleActiveSlotBackward()
{
	if (Slots.Num() < 2)
	{
		return;
	}
	const int32 OldIndex = (ActiveSlotIndex < 0 ? Slots.Num()-1 : ActiveSlotIndex);
	int32 NewIndex = ActiveSlotIndex;
	do
	{
		NewIndex = (NewIndex - 1 + Slots.Num()) % Slots.Num();
		if (Slots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex(NewIndex);
			return;
		}
	} while (NewIndex != OldIndex);

}


void ULeeQuickBarComponent::EquipItemInSlot()
{
	check(Slots.IsValidIndex(ActiveSlotIndex));
	check(EquippedItem == nullptr);

	if (ULeeInventoryItemInstance* SlotItem = Slots[ActiveSlotIndex])
	{
		if (const ULeeInventoryFragment_EquippableItem* EquipInfo = SlotItem->FindFragmentByClass<ULeeInventoryFragment_EquippableItem>())
		{
			TSubclassOf<ULeeEquipmentDefinition> EquipDef = EquipInfo->EquipmentDefinition;
			if (EquipDef)
			{
				EquippedItem = FindEquipmentManger()->EquipItem(EquipDef);
				if (EquippedItem)
				{
					EquippedItem->Instigator = SlotItem;
				}
			}
		}
	}
}

void ULeeQuickBarComponent::UnequipItemInSlot()
{
	if (ULeeEquipmentManagerComponent* EquipmentManager = FindEquipmentManger())
	{
		if (EquippedItem)
		{
			EquipmentManager->UnEquipItem(EquippedItem);
			EquippedItem = nullptr;
		}
	}
}

class ULeeEquipmentManagerComponent* ULeeQuickBarComponent::FindEquipmentManger() const
{
	if (AController* OwnerController = Cast<AController>(GetOwner()))
	{
		if (APawn* Pawn = OwnerController->GetPawn())
		{
			return Pawn->FindComponentByClass<ULeeEquipmentManagerComponent>();
		}
	}
	return nullptr;
}

void ULeeQuickBarComponent::SetActiveSlotIndex_Implementation(int32 NewIndex)
{
	if (Slots.IsValidIndex(NewIndex) && (ActiveSlotIndex != NewIndex))
	{
		UnequipItemInSlot();

		ActiveSlotIndex = NewIndex;

		EquipItemInSlot();

		OnRep_ActiveSlotIndex();
	}
}




class ULeeInventoryItemInstance* ULeeQuickBarComponent::GetActiveSlotItem() const
{
	return Slots.IsValidIndex(ActiveSlotIndex) ? Slots[ActiveSlotIndex] : nullptr;
}

int32 ULeeQuickBarComponent::GetNextFreeItemSlot() const
{
	int32 SlotIndex = 0;
	for (const TObjectPtr<ULeeInventoryItemInstance>& ItemPtr : Slots)
	{
		if (ItemPtr == nullptr)
		{
			return SlotIndex;
		}
		++SlotIndex;
	}

	return INDEX_NONE;
}


void ULeeQuickBarComponent::AddItemToSlot(int32 SlotIndex, class ULeeInventoryItemInstance* Item)
{
	if (Slots.IsValidIndex(SlotIndex) && (Item != nullptr))
	{
		if (Slots[SlotIndex] == nullptr)
		{
			Slots[SlotIndex] = Item;

			// OnRep_Slots()를 즉시 호출하면 위젯의 ListenForGameplayMessages가
			// 아직 등록되기 전에 메시지가 브로드캐스트되어 위젯이 수신하지 못하는
			// 레이스 컨디션이 발생한다.
			// 다음 틱으로 지연하여 위젯이 리스너를 등록할 시간을 확보한다.
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::OnRep_Slots);
			}
		}
	}
}


class ULeeInventoryItemInstance* ULeeQuickBarComponent::RemoveItemFromSlot(int32 SlotIndex)
{
	ULeeInventoryItemInstance* Result = nullptr;

	if (ActiveSlotIndex == SlotIndex)
	{
		UnequipItemInSlot();
		ActiveSlotIndex = -1;
	}
	if (Slots.IsValidIndex(SlotIndex))
	{
		Result = Slots[SlotIndex];
		if (Result != nullptr)
		{
			Slots[SlotIndex] = nullptr;
			OnRep_Slots();
		}
	}
	return Result;
}

void ULeeQuickBarComponent::OnRep_Slots()
{
	FLeeQuickBarSlotsChangeMessage Message;
	Message.Owner = GetOwner();
	Message.Slots = Slots;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(MyTags::Lyra::Lyra_QickBar_Message_SlotsChanged, Message);
}

void ULeeQuickBarComponent::OnRep_ActiveSlotIndex()
{
	FLeeQuickBarActiveIndexChangedMessage Message;
	Message.Owner = GetOwner();
	Message.ActiveIndex = ActiveSlotIndex;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(MyTags::Lyra::Lyra_QickBar_Message_ActiveIndexChanged, Message);
}








