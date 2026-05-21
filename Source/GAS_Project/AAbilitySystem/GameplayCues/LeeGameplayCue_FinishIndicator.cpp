// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayCue_FinishIndicator.h"

#include "GameFramework/Character.h"
#include "GAS_Project/AUI/IndicatorDescriptor.h"
#include "GAS_Project/AUI/IndicatorLibrary.h"
#include "GAS_Project/System/LeeIndicatorManagerComponent.h"
#include "Kismet/GameplayStatics.h"

ALeeGameplayCue_FinishIndicator::ALeeGameplayCue_FinishIndicator()
{
	bAutoDestroyOnRemove = true;
	AutoDestroyDelay = 0.1f;
}

bool ALeeGameplayCue_FinishIndicator::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (!MyTarget || FinishPromptWidgetClass.IsNull())
	{
		return false;
	}

	if (ActiveDescriptor)
	{
		return true;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(MyTarget, 0);
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return true;
	}

	ULeeIndicatorManagerComponent* IndicatorManager = UIndicatorLibrary::GetIndicatorManagerComponent(PlayerController);
	if (!IndicatorManager)
	{
		return false;
	}

	USceneComponent* AttachComponent = MyTarget->GetRootComponent();
	if (ACharacter* Character = Cast<ACharacter>(MyTarget))
	{
		AttachComponent = Character->GetMesh();
	}

	if (!AttachComponent)
	{
		return false;
	}

	ActiveDescriptor = NewObject<UIndicatorDescriptor>(this);
	ActiveDescriptor->SetIndicatorClass(FinishPromptWidgetClass);
	ActiveDescriptor->SetSceneComponent(AttachComponent);
	ActiveDescriptor->SetComponentSocketName(IndicatorSocketName);
	ActiveDescriptor->SetDataObject(MyTarget);
	ActiveDescriptor->SetWorldPositionOffset(IndicatorWorldOffset);
	ActiveDescriptor->SetProjectionMode(EActorCanvasProjectionMode::ComponentPoint);
	ActiveDescriptor->SetHAlign(HAlign_Center);
	ActiveDescriptor->SetVAlign(VAlign_Center);
	ActiveDescriptor->SetPriority(IndicatorPriority);
	ActiveDescriptor->SetDesiredVisibility(true);
	ActiveDescriptor->SetAutoRemoveWhenIndicatorComponentIsNull(true);

	IndicatorManager->AddIndicator(ActiveDescriptor);
	return true;
}

bool ALeeGameplayCue_FinishIndicator::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (ActiveDescriptor)
	{
		ActiveDescriptor->UnregisterIndicator();
		ActiveDescriptor = nullptr;
	}

	return true;
}
