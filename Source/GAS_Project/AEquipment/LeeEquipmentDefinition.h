// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LeeEquipmentDefinition.generated.h"

USTRUCT(Blueprintable)
struct FLeeEquipmentActorToSpawn
{
	GENERATED_BODY()

	// 스폰 대상
	UPROPERTY(EditAnywhere, Category=Equipment)
	TSubclassOf<AActor> ActorToSpawn;

	// 어느 본에 붙일지 결정
	UPROPERTY(EditAnywhere, Category=Equipment)
	FName AttachSocket;

	// 소켓에서 어느정도 트랜스폼을 더 할 것인지 결정
	UPROPERTY(EditAnywhere, Category=Equipment)
	FTransform AttachTransform;
	
};


UCLASS(Blueprintable)
class GAS_PROJECT_API ULeeEquipmentDefinition : public UObject
{
	GENERATED_BODY()
public:
	ULeeEquipmentDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TSubclassOf<class ULeeEquipmentInstance> InstanceType;

	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<FLeeEquipmentActorToSpawn> ActorToSpawns;
	
};
