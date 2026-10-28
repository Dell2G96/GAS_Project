// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "GAS_Project/AInventory/LeeInventoryItemInstance.h"
#include "LeeQuickBarComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeQuickBarComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	ULeeQuickBarComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="Lee")
	void CycleActiveSlotForward();

	UFUNCTION(BlueprintCallable, Category="Lee")
	void CycleActiveSlotBackward();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category= "Lee")
	void SetActiveSlotIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	TArray<ULeeInventoryItemInstance*> GetSlots() const
	{
		return Slots;
	}

	
	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	class ULeeInventoryItemInstance* GetActiveSlotItem() const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	int32 GetNextFreeItemSlot() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddItemToSlot(int32 SlotIndex, ULeeInventoryItemInstance* Item);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	class ULeeInventoryItemInstance* RemoveItemFromSlot(int32 SlotIndex);

	virtual void BeginPlay() override;

	void UnequipItemInSlot();
	void EquipItemInSlot();
	
	class ULeeEquipmentManagerComponent* FindEquipmentManger() const;

protected:
	UPROPERTY()
	int32 NumSlots = 3;

	UFUNCTION()
	void OnRep_Slots();
	
	UFUNCTION()
	void OnRep_ActiveSlotIndex();

	// 26.10.29 추가 - Controller가 새 Pawn을 빙의하면(리스폰) 이전 Pawn의 장착 정보를 버리고, 새 Pawn ASC 준비를 기다린다
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	// 26.10.29 추가 - 새 Pawn의 ASC가 준비되면 현재 활성 슬롯의 장비를 새 Pawn에 다시 장착한다
	void HandleNewPawnAbilitySystemInitialized();

private:
	UPROPERTY(ReplicatedUsing=OnRep_Slots)
	TArray<TObjectPtr<class ULeeInventoryItemInstance>> Slots;;
	
	UPROPERTY(ReplicatedUsing=OnRep_ActiveSlotIndex)
	int32 ActiveSlotIndex = -1;

	UPROPERTY()
	TObjectPtr<class ULeeEquipmentInstance> EquippedItem;
};

USTRUCT(BlueprintType)
struct FLeeQuickBarSlotsChangeMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TObjectPtr<AActor> Owner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TArray<TObjectPtr<ULeeInventoryItemInstance>> Slots;
};

USTRUCT(BlueprintType)
struct FLeeQuickBarActiveIndexChangedMessage
{
	GENERATED_BODY()

	
	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TObjectPtr<AActor> Owner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 ActiveIndex = 0;
	
};
