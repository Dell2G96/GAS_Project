// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeTargetLockTargetComponent.h"

ULeeTargetLockTargetComponent::ULeeTargetLockTargetComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
