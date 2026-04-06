// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractionStatics.h"

#include "IInteractableTarget.h"
#include "Engine/OverlapResult.h"

UInteractionStatics::UInteractionStatics()
	:Super(FObjectInitializer::Get())
{
}

AActor* UInteractionStatics::GetActorFromInteractableTarget(TScriptInterface<IInteractableTarget> InteractableTarget)
{
	if (UObject* Object = InteractableTarget.GetObject())
	{
		if (AActor* Actor = Cast<AActor>(Object))
		{
			return Actor;
		}
		else if (UActorComponent* ActorComponent = Cast<UActorComponent>(Object))
		{
			return ActorComponent->GetOwner();
		}
		else
		{
			unimplemented();
		}
	}
	
	return nullptr;
}

void UInteractionStatics::GetInteractableTargetsFromActor(AActor* Actor,
	TArray<TScriptInterface<IInteractableTarget>>& OutInteractableTargets)
{
	TScriptInterface<IInteractableTarget> InteractableActor(Actor);
	if (InteractableActor)
	{
		OutInteractableTargets.Add(InteractableActor);	
	}
	
	TArray<UActorComponent*> InteractableComponents = Actor? Actor->GetComponentsByInterface(UInteractableTarget::StaticClass()) : TArray<UActorComponent*>();
	for (UActorComponent* InteractableComponent : InteractableComponents)
	{
		OutInteractableTargets.Add(TScriptInterface<IInteractableTarget>(InteractableActor));
	}
}

void UInteractionStatics::AppendInteractableTargetsFromOverlapResults(const TArray<FOverlapResult>& OverlapResults,
	TArray<TScriptInterface<IInteractableTarget>>& OutInteractableTargets)
{
	for (const FOverlapResult& Overlap : OverlapResults)
	{
		TScriptInterface<IInteractableTarget> InteractableActor(Overlap.GetActor());
		if (InteractableActor)
		{
			OutInteractableTargets.AddUnique(InteractableActor);
		}

		TScriptInterface<IInteractableTarget> InteractableComponent(Overlap.GetComponent());
		if (InteractableComponent)
		{
			OutInteractableTargets.AddUnique(InteractableComponent);
		}
	}
}

void UInteractionStatics::AppendInteractableTargetsFromHitResult(const FHitResult& HitResult,
	TArray<TScriptInterface<IInteractableTarget>>& OutInteractableTargets)
{
	TScriptInterface<IInteractableTarget> InteractableActor(HitResult.GetActor());
	if (InteractableActor)
	{
		OutInteractableTargets.AddUnique(InteractableActor);
	}

	TScriptInterface<IInteractableTarget> InteractableComponent(HitResult.GetComponent());
	if (InteractableComponent)
	{
		OutInteractableTargets.AddUnique(InteractableComponent);
	}
}

