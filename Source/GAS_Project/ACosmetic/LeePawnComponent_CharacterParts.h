// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LeeCharacterPartTypes.h"
#include "LeeCosmeticAnimationType.h"
#include "Components/PawnComponent.h"
#include "UObject/Object.h"
#include "LeePawnComponent_CharacterParts.generated.h"


USTRUCT()
struct FLeeAppliedCharacterPartEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FLeeCharacterPart Part;

	UPROPERTY()
	int32 PartHandle = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<UChildActorComponent> SpawnedComponent = nullptr;
	
};

USTRUCT(BlueprintType)
struct FLeeCharacterPartList
{
	GENERATED_BODY()

	FLeeCharacterPartList()
		: OwnerComponent(nullptr)
	{
		
	}

	FLeeCharacterPartList(ULeePawnComponent_CharacterParts* InOwnerComponent)
		:OwnerComponent(InOwnerComponent)
	{
		
	}

	bool SpawnActorForEntry(FLeeAppliedCharacterPartEntry& Entry);
	void DestroyActorForEntry(FLeeAppliedCharacterPartEntry& Entry);

	FLeeCharacterPartHandle AddEntry(FLeeCharacterPart NewPart);
	void RemoveEntry(FLeeCharacterPartHandle Handle);

	FGameplayTagContainer CollectCombinedTags() const;

	/** 현재 인스턴스화된 Character Part */
	UPROPERTY()
	TArray<FLeeAppliedCharacterPartEntry> Entries;

	/** 해당 LeeCharacterPartList의 Owner인 PawnComponent */
	UPROPERTY()
	TObjectPtr<ULeePawnComponent_CharacterParts> OwnerComponent;

	/** 앞서 보았던 PartHandle의 값을 할당 및 관리하는 변수 */
	int32 PartHandleCounter = 0;
	
	
};

UCLASS()
class GAS_PROJECT_API ULeePawnComponent_CharacterParts : public UPawnComponent
{
	GENERATED_BODY()
public:
	ULeePawnComponent_CharacterParts(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	USkeletalMeshComponent* GetParentMeshComponent() const;
	USceneComponent* GetSceneComponentToAttachTo() const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category= Cosmetics)
	FGameplayTagContainer GetCombinedTags(FGameplayTag RequiredPrefix) const;

	void BroadcastChanged();

	FLeeCharacterPartHandle AddCharacterPart(const FLeeCharacterPart& NewPart);
	void RemoveCharacterPart(FLeeCharacterPartHandle Handle);

	UPROPERTY()
	FLeeCharacterPartList CharacterPartList;

	UPROPERTY(EditAnywhere, Category= Cosmetics)
	FLeeAnimBodyStyleSelectionSet BodyMeshes;
};
