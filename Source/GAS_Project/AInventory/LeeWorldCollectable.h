// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IPickupable.h"
#include "GameFramework/Actor.h"
#include "GAS_Project/AInteraction/IInteractableTarget.h"
#include "GAS_Project/AInteraction/InteractionOption.h"
#include "LeeWorldCollectable.generated.h"


class UObject;
struct FInteractionQuery;


UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ALeeWorldCollectable : public AActor, public IPickupable, public IInteractableTarget 
{
	GENERATED_BODY()

public:
	
	ALeeWorldCollectable();
	
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,FInteractionOptionBuilder& InteractionBuilder) override;
	
	virtual FInventoryPickup GetPickupInventory() const override;
	
protected:
	UPROPERTY(EditAnywhere)
	FInteractionOption Option;
	
	UPROPERTY(EditAnywhere)
	FInventoryPickup StaticInventory;
};
