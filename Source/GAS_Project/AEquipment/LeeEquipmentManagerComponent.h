// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySet.h"
#include "UObject/Object.h"
#include "LeeEquipmentManagerComponent.generated.h"


//		FLeeAppliedEquipmentEntry	//

USTRUCT(BlueprintType)
struct FLeeAppliedEquipmentEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<class ULeeEquipmentDefinition> EquipmentDefinition;

	UPROPERTY()
	TObjectPtr<class ULeeEquipmentInstance> Instance = nullptr;

	UPROPERTY()
	FLeeAbilitySet_GrantedHandles GrantedHandles;
	
};

USTRUCT(BlueprintType)
struct FLeeEquipmentList
{
	GENERATED_BODY()

	FLeeEquipmentList(class UActorComponent* InOwnerComponent = nullptr)
		:OwnerComponent(InOwnerComponent)
	{}

	class ULeeEquipmentInstance* AddEntry(TSubclassOf<ULeeEquipmentDefinition> EquipmentDefinition);
	void RemoveEntry(ULeeEquipmentInstance* Instance);

	ULeeAbilitySystemComponent* GetAbilitySystemComponent() const;

	UPROPERTY()
	TArray<FLeeAppliedEquipmentEntry> Entries;
	
	UPROPERTY()
	TObjectPtr<UActorComponent> OwnerComponent;
	
};



UCLASS()
class GAS_PROJECT_API ULeeEquipmentManagerComponent : public UPawnComponent
{
	GENERATED_BODY()
	
public:
	ULeeEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	class ULeeEquipmentInstance* EquipItem(TSubclassOf<class ULeeEquipmentDefinition> EquipmentDefinition);
	void UnEquipItem(ULeeEquipmentInstance* ItemInstance);

	UFUNCTION(BlueprintCallable)
	TArray<class ULeeEquipmentInstance*> GetEquipmentInstancesOfType(TSubclassOf<class ULeeEquipmentInstance> InstanceType) const;

	ULeeEquipmentInstance* GetFirstInstanceOfType(TSubclassOf<ULeeEquipmentInstance>InstanceType);

	template<typename T>
	T* GetFirstInstanceOfType()
	{
		return (T*)GetFirstInstanceOfType(T::StaticClass());
	}

	//~UActorComponent interface
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	//~End of UActorComponent interface

	// 26.10.29 추가 - 장착 중인 모든 장비 해제 (Pawn이 ASC를 잃거나 사라질 때 사용)
	void UnEquipAllItems();

protected:
	// 26.10.29 추가 - Pawn의 ASC 연결이 끊길 때 호출. ASC가 아직 유효한 시점에 장비와 부여된 어빌리티를 회수한다
	void HandleAbilitySystemUnInitialized();

public:
	UPROPERTY()
	FLeeEquipmentList EquipmentList;
};
