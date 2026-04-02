// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeIndicatorManagerComponent.h"

#include "GAS_Project/AUI/IndicatorDescriptor.h"

ULeeIndicatorManagerComponent::ULeeIndicatorManagerComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	bAutoRegister = true;
	bAutoActivate = true;
}

ULeeIndicatorManagerComponent* ULeeIndicatorManagerComponent::GetComponent(AController* Controller)
{
	if (Controller)
	{
		return Controller->FindComponentByClass<ULeeIndicatorManagerComponent>();
	}
	return nullptr;
}

void ULeeIndicatorManagerComponent::AddIndicator(class UIndicatorDescriptor* IndicatorDescriptor)
{
	IndicatorDescriptor->SetIndicatorManagerComponent(this);
	OnIndicatorAdded.Broadcast(IndicatorDescriptor);
	Indicators.Add(IndicatorDescriptor);
}

void ULeeIndicatorManagerComponent::RemoveIndicator(UIndicatorDescriptor* IndicatorDescriptor)
{
	if (IndicatorDescriptor)
	{
		ensure(IndicatorDescriptor->GetIndicatorManagerComponent() == this);
		OnIndicatorRemoved.Broadcast(IndicatorDescriptor);
		Indicators.Remove(IndicatorDescriptor);
	}
}
