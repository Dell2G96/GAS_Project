// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "LeeAbilitySet.generated.h"

//////////////////////////////////////////////
//		FLeeAbilitySet_GameplayAbility		//
//////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FLeeAbilitySet_GameplayAbility
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class ULeeGameplayAbility> Ability = nullptr;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly)
	int32 AbilityLevel = 1;
	
};


//////////////////////////////////////////////
//		FLeeAbilitySet_GrantedHandles		//
//////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FLeeAbilitySet_GrantedHandles
{
	GENERATED_BODY()

	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void TakeFromAbilitySystem(class ULeeAbilitySystemComponent* LeeASC);

protected:
	// 허용된 GameplayAbilitySpecHandle
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
	
};


//////////////////////////////////////////////
//				ULeeAbilitySet				//
//////////////////////////////////////////////
UCLASS()
class GAS_PROJECT_API ULeeAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	ULeeAbilitySet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void GiveToAbilitySystem(ULeeAbilitySystemComponent* LeeASC, FLeeAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject = nullptr);
	
	UPROPERTY(EditDefaultsOnly, Category="Gameplay Abilities")
	TArray<FLeeAbilitySet_GameplayAbility> GrantedGameplayAbilities;
};
