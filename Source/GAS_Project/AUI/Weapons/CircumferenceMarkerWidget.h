// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SCirumferenceMarkerWidget.h"
#include "Components/Widget.h"
#include "CircumferenceMarkerWidget.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UCircumferenceMarkerWidget : public UWidget
{
	GENERATED_BODY()
public:
	UCircumferenceMarkerWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void SynchronizeProperties() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Apperance)
	TArray<FCircumferenceMarkerEntry> MarkerList;
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Apperance)
	float Radius = 48.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Apperance)
	FSlateBrush MarkerImage;

	UPROPERTY(EditAnywhere, Category= Corner)
	uint8 bReticleCornerOutSideSpreadRadius : 1;

	TSharedPtr<SCircumferenceMarkerWidget> MyMarkerWidget;
	
};
