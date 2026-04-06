

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "UObject/Interface.h"

#include "UObject/ObjectPtr.h"
#include "IPickupable.generated.h"

template<typename InterfaceType> class TScriptInterface;

class AActor;
class ULeeInventoryItemDefinition;
class ULeeInventoryItemInstance;
class ULeeInventoryManagerComponent;
class UObject;
struct FFrame;

///////////////////////////////////////////////
//FPickupTemplate
USTRUCT(BlueprintType)
struct FPickupTemplate
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	int32 StackCount = 1;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<ULeeInventoryItemDefinition> ItemDef;
};

///////////////////////////////////////////////
//FPickupInstance
USTRUCT(BlueprintType)
struct FPickupInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<ULeeInventoryItemInstance> Item = nullptr;
};


///////////////////////////////////////////////
//FInventoryPickup
USTRUCT(BlueprintType)
struct FInventoryPickup
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FPickupInstance> Instances;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FPickupTemplate> Templates;
	
};


///////////////////////////////////////////////
//UPickupable
UINTERFACE(BlueprintType, meta=(CannotImplementInterfaceInBlueprint))
class UPickupable : public UInterface
{
	GENERATED_BODY()
};


///////////////////////////////////////////////
//IPickupable
class IPickupable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	virtual FInventoryPickup GetPickupInventory() const = 0;
};


///////////////////////////////////////////////
//
UCLASS()
class UPickupableStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UPickupableStatics();
	
public:
	UFUNCTION(BlueprintPure)
	static TScriptInterface<IPickupable> GetFirstPickupableFromActor(AActor* Actor);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta=(WorldContext = "Ability") )
	static void AddPickupToInventory(ULeeInventoryManagerComponent* InventoryManagerComponent, TScriptInterface<IPickupable> Pickup);
	
};




































