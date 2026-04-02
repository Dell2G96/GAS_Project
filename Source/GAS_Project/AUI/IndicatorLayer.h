// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "UObject/Object.h"
#include "IndicatorLayer.generated.h"

class SActorCanvas;
class SWidget;
class UObject;


UCLASS()
class GAS_PROJECT_API UIndicatorLayer : public UWidget
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Apperance)
	FSlateBrush ArrowBrush;

protected:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

protected:
	TSharedPtr<SActorCanvas> MyActorCanvas;
};
