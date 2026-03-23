// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameState.h"

#include "LeeExperienceManagerComponent.h"

ALeeGameState::ALeeGameState()
{
	ExperienceManagerComponent = CreateDefaultSubobject<ULeeExperienceManagerComponent>(TEXT("ExperienceManager"));
}
