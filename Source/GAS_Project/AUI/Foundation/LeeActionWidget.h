// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActionWidget.h"
#include "LeeActionWidget.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class GAS_PROJECT_API ULeeActionWidget : public UCommonActionWidget
{
	GENERATED_BODY()

public:

	// UCommonActionWidget
	virtual FSlateBrush GetIcon() const override;
	// UCommonActionWidget

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	const TObjectPtr<class UInputAction> AssociatedInputAction;

private:
	class UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem() const;
	
};
