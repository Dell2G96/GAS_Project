// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeEquipmentInstance.h"

#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LeeInventoryFragment_EquippableItem)

ULeeEquipmentInstance::ULeeEquipmentInstance(const FObjectInitializer& ObjectInitializer)
:Super(ObjectInitializer)

{
}

APawn* ULeeEquipmentInstance::GetPawn() const
{
	return Cast<APawn>(GetOuter());
}

void ULeeEquipmentInstance::SpawnEquipmentActors(const TArray<struct FLeeEquipmentActorToSpawn>& ActorsToSpawn)
{
	if (APawn* OwningPawn = GetPawn())
	{
		USceneComponent* AttachTarget = OwningPawn->GetRootComponent();
		if (ACharacter* Char = Cast<ACharacter>(OwningPawn))
		{
			AttachTarget = Char->GetMesh();
		}

		for (const FLeeEquipmentActorToSpawn& SpawnInfo : ActorsToSpawn)
		{
			AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(SpawnInfo.ActorToSpawn, FTransform::Identity, OwningPawn);
			NewActor->FinishSpawning(FTransform::Identity, true);

			NewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);

			NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, SpawnInfo.AttachSocket);

			SpawnedActors.Add(NewActor);
		}
	}
}

void ULeeEquipmentInstance::DestroyEquipmentActors()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
}

APawn* ULeeEquipmentInstance::GetTypedPawn(TSubclassOf<APawn> PawnType) const
{
	APawn* Result = nullptr;
	if (UClass* ActualPawnType = PawnType)
	{
		if (GetOuter()->IsA(ActualPawnType))
		{
			Result = Cast<APawn>(GetOuter());
		}
	}
	return Result;
}

void ULeeEquipmentInstance::OnEquipped()
{
	K2_OnEquipped();
}

void ULeeEquipmentInstance::OnUnEquipped()
{
	K2_OnUnequipped();
}
