// Fill out your copyright notice in the Description page of Project Settings.


#include "CAbilitySystemStatics.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GAS_Project/MyTags.h"

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
	return FGameplayTag::RequestGameplayTag("stats.health.full");
}

FGameplayTag UCAbilitySystemStatics::GetHealthEmptyStatTag()
{
	return FGameplayTag::RequestGameplayTag("stats.health.empty");
}

FGameplayTag UCAbilitySystemStatics::GetManaFullStatTag()
{
	return FGameplayTag::RequestGameplayTag("stats.mana.full");

}

FGameplayTag UCAbilitySystemStatics::GetManaEmptyStatTag()
{
	return FGameplayTag::RequestGameplayTag("stats.mana.empty");
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

float UCAbilitySystemStatics::GetManaCostFor(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC,
	int AbilityLevel)
{
	float ManaCost = 0.0f;
	if (AbilityCDO)
	{
		UGameplayEffect* CostEffect = AbilityCDO->GetCostGameplayEffect();
		if (CostEffect)
		{
			FGameplayEffectSpecHandle EffectSpec = ASC.MakeOutgoingSpec(CostEffect->GetClass(), AbilityLevel, ASC.MakeEffectContext());
			CostEffect->Modifiers[0].ModifierMagnitude.AttemptCalculateMagnitude(*EffectSpec.Data.Get(), ManaCost);
		}
	}
	return FMath::Abs(ManaCost); 
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
