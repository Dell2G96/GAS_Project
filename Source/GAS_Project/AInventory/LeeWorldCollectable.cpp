// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeWorldCollectable.h"

ALeeWorldCollectable::ALeeWorldCollectable()
{

}

void ALeeWorldCollectable::GatherInteractionOptions(const FInteractionQuery& InteractQuery,
	FInteractionOptionBuilder& InteractionBuilder)
{
	InteractionBuilder.AddInteractionOption(Option);
}

FInventoryPickup ALeeWorldCollectable::GetPickupInventory() const
{
	return StaticInventory;
}
