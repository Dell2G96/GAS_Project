// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeInputConfig.h"

#include "InputAction.h"
#include "GAS_Project/LeeLogChannels.h"

ULeeInputConfig::ULeeInputConfig(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

const UInputAction* ULeeInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag,
	bool bLogNotFound) const
{
	for (const FLeeInputAction& Action : NativeInputActions)
	{
		if (Action.InputAction && (Action.InputTag == InputTag))
		{
			return Action.InputAction;
		}
	}
	if (bLogNotFound)
	{
		UE_LOG(LogLee, Error, TEXT("Can't Not Find NaiveInputAction for Tag [%s] in InputConfig [%s]"), *InputTag.ToString(), *GetName());
	}

	return nullptr;
}

const UInputAction* ULeeInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag,
	bool bLogNotFound) const
{
	for (const FLeeInputAction& Action : AbilityInputActions)
	{
		if (Action.InputAction && (Action.InputTag == InputTag))
		{
			return Action.InputAction;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogLee, Error, TEXT("Can't find AbilityInputAction for InputTag [%s] on InputConfig [%s]."), *InputTag.ToString(), *GetNameSafe(this));
	}

	return nullptr;
}
