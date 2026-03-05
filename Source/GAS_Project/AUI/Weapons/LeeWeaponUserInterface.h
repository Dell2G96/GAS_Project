// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "LeeWeaponUserInterface.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeWeaponUserInterface : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	ULeeWeaponUserInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintImplementableEvent)
	void OnWeaponChanged(class ULeeWeaponInstance* OldWeapon, ULeeWeaponInstance* NewWeapon);

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(Transient)
	TObjectPtr<ULeeWeaponInstance> CurrentInstance;
};
