// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeFinishTargetComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS_Project/MyTags.h"

ULeeFinishTargetComponent::ULeeFinishTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;

	ExecutionTriggerTag = MyTags::Souls::Ability_Execution;
	AssassinationTriggerTag = MyTags::Souls::Ability_Assassination;
}

void ULeeFinishTargetComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULeeFinishTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ActiveOverlaps.Reset();
	EnterTimeSecondsByComp.Reset();
	CurrentTarget.Reset();
	CurrentType = ELeeFinishType::None;

	Super::EndPlay(EndPlayReason);
}

void ULeeFinishTargetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	EvaluateCurrentTarget();
}

void ULeeFinishTargetComponent::RegisterEnemyComponent(ULeeFinishInteractionComponent* /*EnemyComp*/)
{
	// Deprecated compatibility shim. 후보 등록은 Enemy overlap box가 직접 호출한다.
}

void ULeeFinishTargetComponent::UnregisterEnemyComponent(ULeeFinishInteractionComponent* EnemyComp)
{
	if (EnemyComp)
	{
		ActiveOverlaps.Remove(EnemyComp);
		EnterTimeSecondsByComp.Remove(EnemyComp);
		EvaluateCurrentTarget();
	}
}

void ULeeFinishTargetComponent::RegisterOverlap(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type)
{
	if (!SourceComp || Type == ELeeFinishType::None)
	{
		return;
	}

	const TWeakObjectPtr<ULeeFinishInteractionComponent> SourceKey(SourceComp);
	ELeeFinishType* ExistingType = ActiveOverlaps.Find(SourceKey);
	if (!ExistingType)
	{
		EnterTimeSecondsByComp.Add(SourceKey, FPlatformTime::Seconds());
		ActiveOverlaps.Add(SourceKey, Type);
		EvaluateCurrentTarget();
		return;
	}

	if (*ExistingType != Type)
	{
		if (Type == ELeeFinishType::Assassination || *ExistingType == ELeeFinishType::None)
		{
			*ExistingType = Type;
		}
		else if (*ExistingType == ELeeFinishType::Assassination && Type == ELeeFinishType::Execution)
		{
			// 양쪽 박스가 잠깐 겹칠 때 암살 우선순위를 유지한다.
		}
		else
		{
			*ExistingType = Type;
		}

		EvaluateCurrentTarget();
	}
}

void ULeeFinishTargetComponent::UnregisterOverlap(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type)
{
	if (!SourceComp)
	{
		return;
	}

	const TWeakObjectPtr<ULeeFinishInteractionComponent> SourceKey(SourceComp);
	ELeeFinishType* ExistingType = ActiveOverlaps.Find(SourceKey);
	if (!ExistingType)
	{
		return;
	}

	if (*ExistingType == Type)
	{
		const ELeeFinishType FallbackType = ResolveFallbackTypeAfterUnregister(SourceComp, Type);
		if (FallbackType == ELeeFinishType::None)
		{
			ActiveOverlaps.Remove(SourceKey);
			EnterTimeSecondsByComp.Remove(SourceKey);
		}
		else
		{
			*ExistingType = FallbackType;
		}
	}

	EvaluateCurrentTarget();
}

void ULeeFinishTargetComponent::AddCandidate(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type)
{
	RegisterOverlap(SourceComp, Type);
}

void ULeeFinishTargetComponent::RemoveCandidate(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type)
{
	UnregisterOverlap(SourceComp, Type);
}

bool ULeeFinishTargetComponent::HasCandidate(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type) const
{
	if (!SourceComp)
	{
		return false;
	}

	const ELeeFinishType* ExistingType = ActiveOverlaps.Find(TWeakObjectPtr<ULeeFinishInteractionComponent>(SourceComp));
	return ExistingType && *ExistingType == Type;
}

ELeeFinishType ULeeFinishTargetComponent::ResolveFallbackTypeAfterUnregister(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType RemovedType) const
{
	if (!SourceComp)
	{
		return ELeeFinishType::None;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return ELeeFinishType::None;
	}

	if (RemovedType == ELeeFinishType::Assassination &&
		SourceComp->IsCandidateValidFor(OwnerActor, ELeeFinishType::Execution))
	{
		return ELeeFinishType::Execution;
	}

	if (RemovedType == ELeeFinishType::Execution &&
		SourceComp->IsCandidateValidFor(OwnerActor, ELeeFinishType::Assassination))
	{
		return ELeeFinishType::Assassination;
	}

	return ELeeFinishType::None;
}

void ULeeFinishTargetComponent::EvaluateCurrentTarget()
{
	AActor* BestTarget = nullptr;
	ELeeFinishType BestType = ELeeFinishType::None;
	float BestScore = FLT_MAX;

	for (auto It = ActiveOverlaps.CreateIterator(); It; ++It)
	{
		ULeeFinishInteractionComponent* SourceComp = It.Key().Get();
		if (!SourceComp || !SourceComp->GetOwner())
		{
			It.RemoveCurrent();
			continue;
		}

		const ELeeFinishType Type = It.Value();
		if (!SourceComp->IsCandidateValidFor(GetOwner(), Type))
		{
			continue;
		}

		const float Score = ScoreCandidate(SourceComp, Type);
		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = SourceComp->GetOwner();
			BestType = Type;
		}
	}

	SetCurrentTarget(BestTarget, BestType);
}

float ULeeFinishTargetComponent::ScoreCandidate(const ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type) const
{
	if (!SourceComp || !SourceComp->GetOwner() || !GetOwner())
	{
		return FLT_MAX;
	}

	const float DistanceScore = FVector::DistSquared(GetOwner()->GetActorLocation(), SourceComp->GetOwner()->GetActorLocation());

	switch (PriorityRule)
	{
	case ELeeFinishPriorityRule::DistanceOnly:
		return DistanceScore;
	case ELeeFinishPriorityRule::LastEntered:
	{
		const double* EnterTime = EnterTimeSecondsByComp.Find(TWeakObjectPtr<ULeeFinishInteractionComponent>(const_cast<ULeeFinishInteractionComponent*>(SourceComp)));
		return EnterTime ? static_cast<float>(-*EnterTime) : DistanceScore;
	}
	case ELeeFinishPriorityRule::AssassinationFirstThenDistance:
	default:
		return (Type == ELeeFinishType::Assassination) ? DistanceScore - 1000000.0f : DistanceScore;
	}
}

void ULeeFinishTargetComponent::SetCurrentTarget(AActor* NewTarget, ELeeFinishType NewType)
{
	if (CurrentTarget.Get() == NewTarget && CurrentType == NewType)
	{
		return;
	}

	CurrentTarget = NewTarget;
	CurrentType = NewType;
	OnFinishTargetChanged.Broadcast(NewTarget, NewType);
}

bool ULeeFinishTargetComponent::TryActivateFinish()
{
	AActor* Target = CurrentTarget.Get();
	if (!Target || CurrentType == ELeeFinishType::None)
	{
		return false;
	}

	const FGameplayTag TriggerTag = (CurrentType == ELeeFinishType::Assassination)
		? AssassinationTriggerTag : ExecutionTriggerTag;

	if (!TriggerTag.IsValid())
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();
	FGameplayEventData Payload;
	Payload.EventTag = TriggerTag;
	Payload.Instigator = OwnerActor;
	Payload.Target = Target;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, TriggerTag, Payload);
	return true;
}

float ULeeFinishTargetComponent::GetFinishTimerProgress() const
{
	if (CurrentType == ELeeFinishType::Assassination)
	{
		return 1.0f;
	}

	AActor* Target = CurrentTarget.Get();
	if (!Target)
	{
		return 0.0f;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!ASC)
	{
		return 0.0f;
	}

	FGameplayEffectQuery GroggyQuery;
	GroggyQuery.EffectTagQuery = FGameplayTagQuery::MakeQuery_MatchTag(MyTags::Souls::Status_Groggy);
	TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(GroggyQuery);

	if (Handles.IsEmpty())
	{
		FGameplayEffectQuery LegacyQuery;
		LegacyQuery.EffectTagQuery = FGameplayTagQuery::MakeQuery_MatchTag(MyTags::Status::Groggy);
		Handles = ASC->GetActiveEffects(LegacyQuery);
	}

	if (Handles.IsEmpty())
	{
		return 0.0f;
	}

	const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(Handles[0]);
	if (!ActiveGE)
	{
		return 0.0f;
	}

	const float Duration = ActiveGE->GetDuration();
	if (Duration <= 0.0f)
	{
		return 1.0f;
	}

	const float WorldTime = ASC->GetWorld() ? ASC->GetWorld()->GetTimeSeconds() : 0.0f;
	const float Elapsed = WorldTime - ActiveGE->StartWorldTime;
	const float Remaining = FMath::Max(0.0f, Duration - Elapsed);
	return FMath::Clamp(Remaining / Duration, 0.0f, 1.0f);
}
