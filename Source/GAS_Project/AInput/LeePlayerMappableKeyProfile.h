// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UserSettings/EnhancedInputUserSettings.h"

#include "LeePlayerMappableKeyProfile.generated.h"


UCLASS()
class GAS_PROJECT_API ULeePlayerMappableKeyProfile : public UEnhancedPlayerMappableKeyProfile
{
	GENERATED_BODY()

protected:

	//~ Begin UEnhancedPlayerMappableKeyProfile interface
	virtual void EquipProfile() override;
	virtual void UnEquipProfile() override;
	//~ End UEnhancedPlayerMappableKeyProfile interface
};


