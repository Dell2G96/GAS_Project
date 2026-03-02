// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "LeeCosmeticAnimationType.generated.h"

//////////////////////////////////////////////
//					AnimLayer				//
//////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FLeeAnimLayerSelectionEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> Layer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer RequiredTags;
	
};

//////////////////////////////////////////////
//			FLeeAnimLayerSelectionSet		//
//////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FLeeAnimLayerSelectionSet
{   
	GENERATED_BODY()

	// 코스매틱 태그에 기반하여 해당 AnimLayer를 반환
	TSubclassOf<class UAnimInstance> SelectBestLayer(const FGameplayTagContainer& CosmeticTags) const; 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLeeAnimLayerSelectionEntry> LayerRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> DefaultLayer;
};


//////////////////////////////////////////////
//					AnimBody				//
//////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FLeeAnimBodyStyleSelectionEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USkeletalMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Categories = "Cosmetic"))
	FGameplayTagContainer RequiredTags;
	
};

USTRUCT(BlueprintType)
struct FLeeAnimBodyStyleSelectionSet
{
	GENERATED_BODY()

	USkeletalMesh* SelectBestBodyStyle(const FGameplayTagContainer& CosmeticTags) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLeeAnimBodyStyleSelectionEntry> MeshRules;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USkeletalMesh> DefaultMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UPhysicsAsset> ForcedPhysicsAsset = nullptr;
	

	
	
	
};