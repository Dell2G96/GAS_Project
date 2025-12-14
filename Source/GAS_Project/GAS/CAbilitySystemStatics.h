
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "CAbilitySystemStatics.generated.h"


UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	Left,
	Right,
	Forward,
	Back,
};

USTRUCT(BlueprintType)
struct FClosestActorWithTagResult
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY(BlueprintReadWrite)
	float Distance {0.f};
	
};

UCLASS()
class GAS_PROJECT_API UCAbilitySystemStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static FGameplayTag GetBasicAttackAbilityTag();
	static FGameplayTag GetBasicAttackInputPressedTag();
	static FGameplayTag GetBasicAttackInputReleasedTag();
	static FGameplayTag GetBattleModeTag();
	static FGameplayTag GetIdleModeTag();
	static FGameplayTag GetKnockdownStatTag();
	static FGameplayTag GetDeadStatTag();
	static FGameplayTag GetStunStatTag();
	static FGameplayTag GetAimStatTag();
	static FGameplayTag GetCameraShakeCueTag();
	static FGameplayTag GetHealthFullStatTag();
	static FGameplayTag GetHealthEmptyStatTag();
	static FGameplayTag GetStaminaFullStatTag();
	static FGameplayTag GetStaminaEmptyStatTag();

	static FGameplayTag GetCrosshairTag();
	static FGameplayTag GetTargetUpdatedTag();


	static bool IsActorDead(const AActor* ActorToCheck);
	static bool ActorHasTag(const AActor* ActorToCheck, const FGameplayTag& Tag);

	static float GetStaticCooldownDurationForAbility(const UGameplayAbility* Ability);
	static float GetStaticCostForAbility(const UGameplayAbility* Ability);
	static bool CheckAbilityCost(const struct FGameplayAbilitySpec& AbilitySpec, const class UAbilitySystemComponent& ASC);
	static bool CheckAbilityCostStatic(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC);
	static float GetStaminaCostFor(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC, int AbilityLevel);
	static float GetCooldownDurationFor(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC, int AbilityLevel);
	static float GetCooldownRemainingFor(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC);
	//------------------------------------------------------------------------------------------------------------------------------
	UFUNCTION(BlueprintPure)
	static EHitDirection GetHitDirection(const FVector& TargetForward, const FVector& ToInstigator);

	UFUNCTION(BlueprintPure)
	static FName GetHitDirectionName(const EHitDirection& HitDirection);

	UFUNCTION(BlueprintCallable)
	static void SendDamageEventToPlayer(AActor* Target, const TSubclassOf<UGameplayEffect>& DamageEffect, UPARAM(ref) FGameplayEventData& Payload, const FGameplayTag& DataTag, float Damage, const FGameplayTag& EventTagOverride, UObject* OptionalParticleSystem = nullptr);

	UFUNCTION(BlueprintCallable)
	static void SendDamageEventToPlayers(TArray<AActor*> Targets, const TSubclassOf<UGameplayEffect>& DamageEffect, UPARAM(ref) FGameplayEventData& Payload, const FGameplayTag& DataTag, float Damage, const FGameplayTag& EventTagOverride, UObject* OptionalParticleSystem = nullptr);

	
	UFUNCTION(BlueprintCallable, Category="GAS|Abilities")
	static TArray<AActor*> HitBoxHitTest
	(
		AActor* AvatarActor,
		float HitBoxRadius,
		float HitBoxForwardOffset = 0.f,
		float HitBoxElevatOffset = 0.f,
		bool bDrawDebugs = false
	);

	UFUNCTION(BlueprintCallable, Category="GAS|Abilities")
	static void DrawHitBoxHitDebugs
	(
		const UObject* WorldContextObject,
		const TArray<FHitResult>& HitResults,
		const FVector& HitBoxLocation,
		float HitBoxRadius
	);

	UFUNCTION(BlueprintCallable)
	static FClosestActorWithTagResult FindClosestActorWithTag(const UObject* WorldContextObject, const FVector& Origin, const FName& Tag);

	// UFUNCTION(BlueprintCallable)
	// static void SendDamageEventToPlayer(
	// 	AActor* Target,	const TSubclassOf<class UGameplayEffect>& DamageEffect,	const struct FGameplayEventData& Payload,const struct FGameplayTag& DataTag, float Damage);																

};
