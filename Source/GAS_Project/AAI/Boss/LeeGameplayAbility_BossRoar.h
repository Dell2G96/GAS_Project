#pragma once
#include "CoreMinimal.h"
#include "GAS_Project/AAbilitySystem/Abilities/LeeGameplayAbility.h"
#include "LeeGameplayAbility_BossRoar.generated.h"

/** 몽타주를 나중에 지정하는 페이즈 전환 어빌리티. 비어 있으면 즉시 끝난다. */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_BossRoar : public ULeeGameplayAbility
{
	GENERATED_BODY()
public:
	ULeeGameplayAbility_BossRoar();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|Boss")
	TObjectPtr<UAnimMontage> RoarMontage;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
private:
	UFUNCTION() void FinishRoar();
};
