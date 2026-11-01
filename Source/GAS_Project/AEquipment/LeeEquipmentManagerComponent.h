// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySet.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"
#include "LeeEquipmentManagerComponent.generated.h"


//		FLeeAppliedEquipmentEntry	//

// 26.11.01 수정 - 장착된 장비 1개. FastArray 항목으로 바꿔 클라이언트로 복제된다
USTRUCT(BlueprintType)
struct FLeeAppliedEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<class ULeeEquipmentDefinition> EquipmentDefinition;

	UPROPERTY()
	TObjectPtr<class ULeeEquipmentInstance> Instance = nullptr;

	// 어빌리티 핸들은 서버에서만 의미가 있으므로 복제하지 않는다
	UPROPERTY(NotReplicated)
	FLeeAbilitySet_GrantedHandles GrantedHandles;

};

// 26.11.01 수정 - 장착 장비 목록. FastArray로 복제하고, 클라이언트에서 추가/제거될 때 OnEquipped/OnUnEquipped를 호출한다
USTRUCT(BlueprintType)
struct FLeeEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	FLeeEquipmentList(class UActorComponent* InOwnerComponent = nullptr)
		:OwnerComponent(InOwnerComponent)
	{}

	class ULeeEquipmentInstance* AddEntry(TSubclassOf<ULeeEquipmentDefinition> EquipmentDefinition);
	void RemoveEntry(ULeeEquipmentInstance* Instance);

	ULeeAbilitySystemComponent* GetAbilitySystemComponent() const;

	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FLeeAppliedEquipmentEntry, FLeeEquipmentList>(Entries, DeltaParms, *this);
	}

	UPROPERTY()
	TArray<FLeeAppliedEquipmentEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;

};

template<>
struct TStructOpsTypeTraits<FLeeEquipmentList> : public TStructOpsTypeTraitsBase2<FLeeEquipmentList>
{
	enum { WithNetDeltaSerializer = true };
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
	virtual void ReadyForReplication() override;
	//~End of UActorComponent interface

	// 26.10.29 추가 - 장착 중인 모든 장비 해제 (Pawn이 ASC를 잃거나 사라질 때 사용)
	void UnEquipAllItems();

protected:
	// 26.10.29 추가 - Pawn의 ASC 연결이 끊길 때 호출. ASC가 아직 유효한 시점에 장비와 부여된 어빌리티를 회수한다
	void HandleAbilitySystemUnInitialized();

public:
	UPROPERTY(Replicated)
	FLeeEquipmentList EquipmentList;
};
