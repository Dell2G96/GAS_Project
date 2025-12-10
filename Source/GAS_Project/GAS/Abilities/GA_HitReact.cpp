// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_HitReact.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

void UGA_HitReact::CacheHitDirectionVectors(AActor* Instigator)
{
	AvatarForward = GetAvatarActorFromActorInfo()->GetActorForwardVector();

	const FVector AvatarLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
	const FVector InstigatorLocation = Instigator->GetActorLocation();

	ToInstigator = InstigatorLocation - AvatarLocation;
	ToInstigator.Normalize();
}

