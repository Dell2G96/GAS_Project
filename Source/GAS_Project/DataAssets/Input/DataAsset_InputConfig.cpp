// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAsset_InputConfig.h"

class UInputAction* UDataAsset_InputConfig::FindNativeInputActionByTag(const FGameplayTag InInputTag) const
{
	for (const FCInputActionConfig& InputActionConfig : NativeInputActions)
	{
		if (InputActionConfig.InputTag == InInputTag && InputActionConfig.InputAction )
		{
			return InputActionConfig.InputAction;
		}
	}

	return nullptr;
}
