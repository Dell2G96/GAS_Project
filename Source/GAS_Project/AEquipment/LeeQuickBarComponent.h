// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "LeeQuickBarComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeQuickBarComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	ULeeQuickBarComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

	class ULeeEquipmentManagerComponent* FindEquipmentManger() const;
	void UnequipItemInSlot();
	void EquipItemInSlot();

	UFUNCTION(BlueprintCallable)
	void AddItemToSlot(int32 SlotIndex, class ULeeInventoryItemInstance* Item);

	UFUNCTION(BlueprintCallable, Category= "Lee")
	void SetActiveSlotIndex(int32 NewIndex);
	

	UPROPERTY()
	int32 NumSlots = 3;

	UPROPERTY()
	TArray<TObjectPtr<class ULeeInventoryItemInstance>> Slots;

	UPROPERTY()
	int32 ActiveSlotIndex = -1;

	UPROPERTY()
	TObjectPtr<class ULeeEquipmentInstance> EquippedItem;
	
	
};
