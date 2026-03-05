// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "LeeReticleWidgetBase.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class GAS_PROJECT_API ULeeReticleWidgetBase : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	ULeeReticleWidgetBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable)
	void InitializeFromWeapon(class ULeeWeaponInstance* InWeapon);

	UFUNCTION(BlueprintImplementableEvent)
	void OnWeaponInitialized();

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ULeeWeaponInstance> WeaponInstance;
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<class ULeeInventoryItemInstance> InventoryInstance;
	
};
