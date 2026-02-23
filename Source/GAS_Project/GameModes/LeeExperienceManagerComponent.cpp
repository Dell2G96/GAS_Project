// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeExperienceManagerComponent.h"

void ULeeExperienceManagerComponent::CallorRegister_OnExperienceLoaded(FOnLeeExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded.Add(MoveTemp(Delegate));
	}
}
