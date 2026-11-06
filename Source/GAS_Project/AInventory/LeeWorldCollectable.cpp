// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeWorldCollectable.h"

#include "Async/TaskGraphInterfaces.h"
#include "LeeInventoryItemDefinition.h"
#include "LeeInventoryItemInstance.h"


struct FInteractionQuery;


ALeeWorldCollectable::ALeeWorldCollectable()
{

}

// 상호작용 옵션 제공 (26.11.06 수정 - 문구 앞에 아이템 이름을 붙인다. 이름이 없으면 기존 Option.Text 그대로)
void ALeeWorldCollectable::GatherInteractionOptions(const FInteractionQuery& InteractQuery,
	FInteractionOptionBuilder& InteractionBuilder)
{
	if (!bOptionTextCached)
	{
		const FText DisplayName = GetPickupDisplayName();
		if (DisplayName.IsEmpty())
		{
			CachedOptionText = Option.Text;
		}
		else
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Name"), DisplayName);
			Args.Add(TEXT("Action"), Option.Text);
			CachedOptionText = FText::Format(PickupTextFormat, Args);
		}
		bOptionTextCached = true;
	}

	FInteractionOption OptionToShow = Option;
	OptionToShow.Text = CachedOptionText;
	InteractionBuilder.AddInteractionOption(OptionToShow);
}

// 26.11.06 추가 - 템플릿(아이템 정의) 우선, 없으면 인스턴스의 아이템 정의에서 DisplayName을 가져온다
FText ALeeWorldCollectable::GetPickupDisplayName() const
{
	TSubclassOf<ULeeInventoryItemDefinition> ItemDef;
	if (StaticInventory.Templates.Num() > 0)
	{
		ItemDef = StaticInventory.Templates[0].ItemDef;
	}
	else if (StaticInventory.Instances.Num() > 0 && StaticInventory.Instances[0].Item)
	{
		ItemDef = StaticInventory.Instances[0].Item->GetItemDef();
	}

	return ItemDef ? GetDefault<ULeeInventoryItemDefinition>(ItemDef)->DisplayName : FText::GetEmpty();
}

FInventoryPickup ALeeWorldCollectable::GetPickupInventory() const
{
	return StaticInventory;
}
