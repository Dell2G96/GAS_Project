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

	// 26.11.06 추가 - 상호작용 문구 형식. {Name}=아이템 정의 DisplayName, {Action}=Option.Text (예: "카타나 줍기")
	UPROPERTY(EditAnywhere, Category = "Lee|Interaction")
	FText PickupTextFormat = NSLOCTEXT("Lee", "PickupTextFormat", "{Name} {Action}");

private:
	// 26.11.06 추가 - 주울 아이템의 DisplayName (없으면 빈 텍스트)
	FText GetPickupDisplayName() const;

	// 26.11.06 추가 - 문구는 한 번만 만들어 캐시 (매 스캔 새 FText를 만들면 프롬프트가 계속 다시 그려진다)
	FText CachedOptionText;
	bool bOptionTextCached = false;
};
