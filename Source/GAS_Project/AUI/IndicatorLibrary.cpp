// Fill out your copyright notice in the Description page of Project Settings.


#include "IndicatorLibrary.h"

#include "GAS_Project/System/LeeIndicatorManagerComponent.h"

UIndicatorLibrary::UIndicatorLibrary()
{
}

ULeeIndicatorManagerComponent* UIndicatorLibrary::GetIndicatorManagerComponent(AController* Controller)
{
	return ULeeIndicatorManagerComponent::GetComponent(Controller);
}
