// Fill out your copyright notice in the Description page of Project Settings.


#include "CAbilitySystemStatics.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "CAbilitySystemComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "Kismet/GameplayStatics.h"

FGameplayTag UCAbilitySystemStatics::GetBasicAttackAbilityTag()
{
	return MyTags::Abilities::BasicAttack;
}

FGameplayTag UCAbilitySystemStatics::GetBasicAttackInputPressedTag()
{
	return MyTags::Abilities::BasicAttackPressed;
}

FGameplayTag UCAbilitySystemStatics::GetBasicAttackInputReleasedTag()
{
	return MyTags::Abilities::BasicAttackReleased;
}

FGameplayTag UCAbilitySystemStatics::GetGuardInputPressedTag()
{
	return MyTags::Abilities::GuardPressed;
}

FGameplayTag UCAbilitySystemStatics::GetGuardInputReleasedTag()
{
	return MyTags::Abilities::GuardReleased;
}

FGameplayTag UCAbilitySystemStatics::GetGuardingTag()
{
	return MyTags::Status::Guarding;
}

FGameplayTag UCAbilitySystemStatics::GetPerfectGuardTag()
{
	return MyTags::Status::PerfectGuard;
}


FGameplayTag UCAbilitySystemStatics::GetBattleModeTag()
{
	return MyTags::Status::BattleMode;

}

FGameplayTag UCAbilitySystemStatics::GetIdleModeTag()
{
	return MyTags::Status::IdleMode;
}

FGameplayTag UCAbilitySystemStatics::GetKnockdownStatTag()
{
	return MyTags::Status::Knockdown;
}

FGameplayTag UCAbilitySystemStatics::GetDeadStatTag()
{
	return MyTags::Status::Dead;

}

FGameplayTag UCAbilitySystemStatics::GetStunStatTag()
{
	return MyTags::Status::Stun;

}

FGameplayTag UCAbilitySystemStatics::GetAimStatTag()
{
	return FGameplayTag::RequestGameplayTag("stats.aim");
}


FGameplayTag UCAbilitySystemStatics::GetCameraShakeCueTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.camerashake");

}

FGameplayTag UCAbilitySystemStatics::GetHealthFullStatTag()
{
	return MyTags::Status::HealthFull;
}

FGameplayTag UCAbilitySystemStatics::GetHealthEmptyStatTag()
{
	return MyTags::Status::HealthEmpty;
}

FGameplayTag UCAbilitySystemStatics::GetStaminaFullStatTag()
{
	return MyTags::Status::StaminaFull;
}

FGameplayTag UCAbilitySystemStatics::GetStaminaEmptyStatTag()
{
	return MyTags::Status::StaminaEmpty;
}

FGameplayTag UCAbilitySystemStatics::GetCrosshairTag()
{
	return FGameplayTag::RequestGameplayTag("stats.crosshair");
}

FGameplayTag UCAbilitySystemStatics::GetTargetUpdatedTag()
{
	return FGameplayTag::RequestGameplayTag("target.updated");
}

bool UCAbilitySystemStatics::IsActorDead(const AActor* ActorToCheck)
{
	return ActorHasTag(ActorToCheck, GetDeadStatTag());

}

bool UCAbilitySystemStatics::ActorHasTag(const AActor* ActorToCheck, const FGameplayTag& Tag)
{
	const IAbilitySystemInterface* ActorISA = Cast<IAbilitySystemInterface>(ActorToCheck);
	if (ActorISA)
	{
		UAbilitySystemComponent* ActorASC = ActorISA->GetAbilitySystemComponent();
		if (ActorASC)
		{
			return ActorASC->HasMatchingGameplayTag(Tag);
		}
	}
	return false;
}

float UCAbilitySystemStatics::GetStaticCooldownDurationForAbility(const UGameplayAbility* Ability)
{
	if (!Ability)
	{
		return 0.0f;
	}

	const UGameplayEffect* CooldownEffect = Ability->GetCooldownGameplayEffect();
	if (!CooldownEffect)
	{
		return 0.0f;
	}
	float CooldownDuraction = 0.0f;
	CooldownEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(1, CooldownDuraction);
	return CooldownDuraction;
}

float UCAbilitySystemStatics::GetStaticCostForAbility(const UGameplayAbility* Ability)
{
	if (!Ability) return 0.0f;

	const UGameplayEffect* CostEffect = Ability->GetCostGameplayEffect();
	if (!CostEffect || CostEffect->Modifiers.Num() == 0.) return 0.0f;

	float Cost = 0.0f;
	CostEffect->Modifiers[0].ModifierMagnitude.GetStaticMagnitudeIfPossible(1, Cost);
	return FMath::Abs(Cost);
}

bool UCAbilitySystemStatics::CheckAbilityCost(const struct FGameplayAbilitySpec& AbilitySpec,
	const class UAbilitySystemComponent& ASC)
{
	const UGameplayAbility* AbilityCDO = AbilitySpec.Ability;
	if (AbilityCDO)
	{
		return AbilityCDO->CheckCost(AbilitySpec.Handle, ASC.AbilityActorInfo.Get());
	}

	return false;
}

bool UCAbilitySystemStatics::CheckAbilityCostStatic(const UGameplayAbility* AbilityCDO,
	const UAbilitySystemComponent& ASC)
{
	if (AbilityCDO)
	{
		return AbilityCDO->CheckCost(FGameplayAbilitySpecHandle(),ASC.AbilityActorInfo.Get());
	}
	return false;
}

float UCAbilitySystemStatics::GetStaminaCostFor(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC,
	int AbilityLevel)
{
	float StaminaCost = 0.0f;
	if (AbilityCDO)
	{
		UGameplayEffect* CostEffect = AbilityCDO->GetCostGameplayEffect();
		if (CostEffect)
		{
			FGameplayEffectSpecHandle EffectSpec = ASC.MakeOutgoingSpec(CostEffect->GetClass(), AbilityLevel, ASC.MakeEffectContext());
			CostEffect->Modifiers[0].ModifierMagnitude.AttemptCalculateMagnitude(*EffectSpec.Data.Get(), StaminaCost);
		}
	}
	return FMath::Abs(StaminaCost); 
}

float UCAbilitySystemStatics::GetCooldownDurationFor(const UGameplayAbility* AbilityCDO,
	const UAbilitySystemComponent& ASC, int AbilityLevel)
{
	float CooldownDuration = 0.0f;
	if (AbilityCDO)
	{
		UGameplayEffect* CooldownEffect = AbilityCDO->GetCooldownGameplayEffect();
		if (CooldownEffect)
		{
			FGameplayEffectSpecHandle EffectSpec = ASC.MakeOutgoingSpec(CooldownEffect->GetClass(), AbilityLevel, ASC.MakeEffectContext());
			CooldownEffect->DurationMagnitude.AttemptCalculateMagnitude(*EffectSpec.Data.Get(), CooldownDuration);
		}
	}
	return FMath::Abs(CooldownDuration); 
}

float UCAbilitySystemStatics::GetCooldownRemainingFor(const UGameplayAbility* AbilityCDO,
	const UAbilitySystemComponent& ASC)
{
	if (!AbilityCDO) return 0.0f;

	UGameplayEffect* CooldownEffect = AbilityCDO->GetCooldownGameplayEffect();
	if (!CooldownEffect) return 0.0f;

	FGameplayEffectQuery CooldownQuery;
	CooldownQuery.EffectDefinition = CooldownEffect->GetClass();

	float CooldownRemaining = 0.f;
	FJsonSerializableArrayFloat CooldownTimeRemainings = ASC.GetActiveEffectsTimeRemaining(CooldownQuery);

	for (float Remaining : CooldownTimeRemainings)
	{
		if (Remaining > CooldownRemaining)
		{
			CooldownRemaining = Remaining;
		}
	}

	return CooldownRemaining;
}


EHitDirection UCAbilitySystemStatics::GetHitDirection(const FVector& TargetForward, const FVector& ToInstigator)
{
	const float Dot = FVector::DotProduct(TargetForward, ToInstigator);
	if (Dot < -0.5f)
	{
		return EHitDirection::Back;
	}
	if (Dot < 0.5f)
	{
		// 왼쪽 또는 오른쪽
		const FVector Cross = FVector::CrossProduct(TargetForward, ToInstigator);
		if (Cross.Z < 0.f)
		{
			return EHitDirection::Left;
		}
		return EHitDirection::Right;
	}
	return EHitDirection::Forward;
}

FName UCAbilitySystemStatics::GetHitDirectionName(const EHitDirection& HitDirection)
{
	switch (HitDirection)
	{
		case EHitDirection::Right: return FName("Right");
		case EHitDirection::Left: return FName("Left");
		case EHitDirection::Back: return FName("Back");
		case EHitDirection::Forward: return FName("Forward");
		default: return FName("None");
	}
}

void UCAbilitySystemStatics::	SendDamageEventToPlayer(AActor* Target, const TSubclassOf<UGameplayEffect>& DamageEffect,
	FGameplayEventData& Payload, const FGameplayTag& DataTag, float Damage, const FGameplayTag& EventTagOverride,
	UObject* OptionalParticleSystem)
{
	ACCharacter* PlayerCharacter = Cast<ACCharacter>(Target);
	if (!IsValid(PlayerCharacter)) return;
	if (!PlayerCharacter->IsAlive()) return;

	UCAbilitySystemComponent* TargetASC =
	   Cast<UCAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target));
	if (!IsValid(TargetASC)) return;

	// 이미 죽었거나 쓰러져 있으면 추가 데미지 처리/이벤트 스팸 방지
	if (TargetASC->HasMatchingGameplayTag(MyTags::Status::Dead) || TargetASC->HasMatchingGameplayTag(MyTags::Status::Knockdown))
	{
		return;
	}

	FGameplayTag EventTag;
	if (!EventTagOverride.MatchesTagExact(MyTags::None))
	{
		EventTag = EventTagOverride;
	}
	else
	{
		UCAttributeSet* AttributeSet = Cast<UCAttributeSet>(PlayerCharacter->GetAttributeSet());
		if (!IsValid(AttributeSet)) return;

		const bool bLethal = AttributeSet->GetHealth() - Damage <= 0.f;
		EventTag = bLethal ? MyTags::Events::Player::Knockdown : MyTags::Events::Hit::LightHit;
	}
	
	Payload.OptionalObject = OptionalParticleSystem;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(PlayerCharacter, EventTag, Payload);
	//PlayerCharacter->Multicast_SendGameplayEventToActor(PlayerCharacter, EventTag, Payload);

	if (!IsValid(TargetASC)) return;

	FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffect, 1.f, ContextHandle);

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, DataTag, -Damage);

	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void UCAbilitySystemStatics::SendDamageEventToPlayers(TArray<AActor*> Targets,
	const TSubclassOf<UGameplayEffect>& DamageEffect, FGameplayEventData& Payload, const FGameplayTag& DataTag,
	float Damage, const FGameplayTag& EventTagOverride, UObject* OptionalParticleSystem)
{
	for (AActor* Target : Targets)
	{
		SendDamageEventToPlayer(Target, DamageEffect, Payload, DataTag, Damage, EventTagOverride, OptionalParticleSystem);
	}
}

TArray<AActor*> UCAbilitySystemStatics::HitBoxHitTest(AActor* AvatarActor, float HitBoxRadius,
                                                      float HitBoxForwardOffset, float HitBoxElevatOffset, bool bDrawDebugs)
{
	if (!IsValid(AvatarActor)) return TArray<AActor*>();

	//자기 자신 제외
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);

	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);

	TArray<FHitResult> HitResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(HitBoxRadius);

	const FVector Forward = AvatarActor->GetActorForwardVector() * HitBoxForwardOffset;
	const FVector HitBoxLocation = AvatarActor->GetActorLocation() + Forward + FVector(0, 0, HitBoxElevatOffset);

	UWorld* World = GEngine->GetWorldFromContextObject(AvatarActor, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World)) return TArray<AActor*>();
	World->SweepMultiByChannel(HitResults, HitBoxLocation, HitBoxLocation, FQuat::Identity, ECC_Visibility, Sphere, QueryParams, ResponseParams);



	TArray<AActor*> ActorsHit;
	for (const FHitResult& Result : HitResults)
	{
		ACCharacter* Character = Cast<ACCharacter>(Result.GetActor());
		if (!IsValid(Character)) continue;
		if (!Character->IsAlive()) continue;
		ActorsHit.AddUnique(Character);
	}
	
	if (bDrawDebugs)
	{
		DrawHitBoxHitDebugs(AvatarActor, HitResults, HitBoxLocation, HitBoxRadius);
	}

	return ActorsHit;
}

void UCAbilitySystemStatics::DrawHitBoxHitDebugs(const UObject* WorldContextObject,
	const TArray<FHitResult>& HitResults, const FVector& HitBoxLocation, float HitBoxRadius)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World)) return;

	DrawDebugSphere(World, HitBoxLocation, HitBoxRadius, 16, FColor::Red, false, 3.f);

	for (const FHitResult& Result : HitResults)
	{
		if (IsValid(Result.GetActor()))
		{
			FVector DebugLocation = Result.GetActor()->GetActorLocation();
			DebugLocation.Z += 100;
			DrawDebugSphere(World, DebugLocation, 30.f, 10, FColor::Green, false, 3.f);
		}
	}
}

FClosestActorWithTagResult UCAbilitySystemStatics::FindClosestActorWithTag(const UObject* WorldContextObject,
	const FVector& Origin, const FName& Tag)
{
	

	TArray<AActor*> ActorsWithTag;
	UGameplayStatics::GetAllActorsWithTag(WorldContextObject, Tag, ActorsWithTag);

	float ClosestDistance = TNumericLimits<float>::Max();
	AActor* ClosesActor = nullptr;

	for (AActor* Actor : ActorsWithTag)
	{
		if (!IsValid(Actor)) continue;
		ACCharacter* BaseCharacter = Cast<ACCharacter>(Actor);
		if (!IsValid(BaseCharacter) || !BaseCharacter->IsAlive()) continue;
		const float Distance = FVector::Dist(Origin, Actor->GetActorLocation());
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosesActor = Actor;
		}
	}
	FClosestActorWithTagResult Result;
	Result.Actor = ClosesActor;
	Result.Distance = ClosestDistance;

	return Result;
	
}

void UCAbilitySystemStatics::AddGamePlayTagToActorIfNone(AActor* InActor, FGameplayTag TagToAdd)
{
	UCAbilitySystemComponent* ASC = Cast<UCAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InActor));
	if (ASC)
	{
		if (!ASC->HasMatchingGameplayTag(TagToAdd))
		{
			ASC->AddLooseGameplayTag(TagToAdd);
		}
	}
		
}

void UCAbilitySystemStatics::RemoveGameplayTagFromActorIfFound(AActor* InActor, FGameplayTag TagToRemove)
{
	UCAbilitySystemComponent* ASC = Cast<UCAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InActor));
	if (ASC)
	{
		if (ASC->HasMatchingGameplayTag(TagToRemove))
		{
			ASC->RemoveLooseGameplayTag(TagToRemove);
		}
	}
}

bool UCAbilitySystemStatics::IsTargetPawnHostile(APawn* QueryPawn, APawn* TargetPawn)
{
	check(QueryPawn && TargetPawn);

	IGenericTeamAgentInterface* QueryTeamAgent = Cast<IGenericTeamAgentInterface>(QueryPawn);
	IGenericTeamAgentInterface* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(TargetPawn);

	if (QueryTeamAgent && TargetTeamAgent)
	{
		return QueryTeamAgent->GetGenericTeamId() != TargetTeamAgent->GetGenericTeamId();
	}

	return false;
}

bool UCAbilitySystemStatics::IsValidBlock(AActor* InAttacker, AActor* InDefender)
{

	check(InAttacker && InDefender);

	const float DotResult = FVector::DotProduct(InAttacker->GetActorForwardVector(),InDefender->GetActorForwardVector());

	return DotResult < 0.f ? true : false;
	// AActor* HitActor = Cast<AActor>(HitResult.GetActor());
	// check(InAttacker && HitActor);
	//
	// if (!InAttacker->HasAuthority()) return false;
	//
	// UCAbilitySystemComponent* TargetASC = Cast<UCAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor));
	// UCAbilitySystemComponent* OwnerASC = Cast<UCAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InAttacker));
	//
	// if (!TargetASC || !OwnerASC)
	// {
	// 	return false;
	// }
	//
	// if (ActorHasTag(HitActor, GetGuardingTag()) || ActorHasTag(HitActor, GetPerfectGuardTag()))
	// {
	// 	return true;
	// }
	// else
	// {
	// 	return false;
	// }
}

bool UCAbilitySystemStatics::ApplyGameplayEffectSpecHandleToTargetActor(AActor* InInstigator, AActor* InTargetActor,
	const FGameplayEffectSpecHandle& InSpecHandle)
{
	UCAbilitySystemComponent* SourceASC = Cast<UCAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InInstigator));
	UCAbilitySystemComponent* TargetASC = Cast<UCAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InTargetActor));

	if (!SourceASC || !TargetASC) return false;

	FActiveGameplayEffectHandle SpecHandle = SourceASC->ApplyGameplayEffectSpecToTarget(*InSpecHandle.Data,TargetASC);
	return SpecHandle.WasSuccessfullyApplied();
	
}

bool UCAbilitySystemStatics::NativeDoseActorHaveTag(AActor* InActor, FGameplayTag TagToCheck)
{
	
	UCAbilitySystemComponent* ASC = Cast<UCAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InActor));

	if (!ASC) return false;
	
	return ASC->HasMatchingGameplayTag(TagToCheck);
}

// void UCAbilitySystemStatics::BP_DoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck)
// {
// 	
// }

//
// void UCAbilitySystemStatics::SendDamageEventToPlayer(AActor* Target,
// 	const TSubclassOf<class UGameplayEffect>& DamageEffect, const struct FGameplayEventData& Payload,
// 	const struct FGameplayTag& DataTag, float Damage)
// {
// 	ACCharacter* PlayerCharacter = Cast<ACCharacter>(Target);
// 	if (!IsValid(PlayerCharacter)) return;
// 	if (!PlayerCharacter->IsAlive()) return;
//
// 	UCAttributeSet* AttributeSet = Cast<UCAttributeSet>(PlayerCharacter->GetAttributeSet());
// 	if (!IsValid(AttributeSet)) return;
//
// 	const bool bLethal = AttributeSet->GetHealth() - Damage <= 0.f;
// 	const FGameplayTag EventTag = bLethal ? MyTags::Events::Player::Death : MyTags::Events::Player::HitReact;
//
// 	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(PlayerCharacter, EventTag, Payload);
//
// 	UAbilitySystemComponent* TargetASC = PlayerCharacter->GetAbilitySystemComponent();
// 	if (!IsValid(TargetASC)) return;
//
// 	FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
// 	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffect, 1.f, ContextHandle);
//
// 	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, DataTag, -Damage);
//
// 	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
// }
