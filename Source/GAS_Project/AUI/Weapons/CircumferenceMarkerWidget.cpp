// Fill out your copyright notice in the Description page of Project Settings.


#include "CircumferenceMarkerWidget.h"

UCircumferenceMarkerWidget::UCircumferenceMarkerWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void UCircumferenceMarkerWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyMarkerWidget.Reset();
}

void UCircumferenceMarkerWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	MyMarkerWidget->SetRadius(Radius);
	MyMarkerWidget->SetMarkerList(MarkerList);
}

TSharedRef<SWidget> UCircumferenceMarkerWidget::RebuildWidget()
{
	MyMarkerWidget = SNew(SCircumferenceMarkerWidget)
		.MarkerBrush(&MarkerImage)
		.Radius(this->Radius)
		.MarkerList(this->MarkerList);
	return MyMarkerWidget.ToSharedRef();
}


