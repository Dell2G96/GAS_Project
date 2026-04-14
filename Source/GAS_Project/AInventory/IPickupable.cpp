#include "IPickupable.h"

#include "LeeInventoryManagerComponent.h"


UPickupableStatics::UPickupableStatics()
	:Super(FObjectInitializer::Get())
{

}

TScriptInterface<IPickupable> UPickupableStatics::GetFirstPickupableFromActor(AActor* Actor)
{
	TScriptInterface<IPickupable> PickupableActor(Actor);
	if (PickupableActor)
	{
		return PickupableActor;
	}

	TArray<UActorComponent*> PickupableComponents = Actor ? Actor->GetComponentsByInterface(UPickupable::StaticClass()) : TArray<UActorComponent*>();
	if (PickupableComponents.Num() > 0)
	{
		return TScriptInterface<IPickupable>(PickupableComponents[0]);
	}

	return TScriptInterface<IPickupable>();
}

void UPickupableStatics::AddPickupToInventory(ULeeInventoryManagerComponent* InventoryManagerComponent,
	TScriptInterface<IPickupable> Pickup)
{
	if (InventoryManagerComponent && Pickup)
	{
		const FInventoryPickup& PickupInventory = Pickup->GetPickupInventory();

		for (const FPickupTemplate& Template : PickupInventory.Templates)
		{
			InventoryManagerComponent->AddItemDefinition(Template.ItemDef, Template.StackCount);
		}
		for (const FPickupInstance& Instance : PickupInventory.Instances)
		{
			InventoryManagerComponent->AddItemInstance(Instance.Item);
		}
	}
}


