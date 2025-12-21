#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAttributesInitialized);
UCLASS()
class GAS_PROJECT_API UCAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	ATTRIBUTE_ACCESSORS(ThisClass, Health);
	ATTRIBUTE_ACCESSORS(ThisClass, MaxHealth);
	ATTRIBUTE_ACCESSORS(ThisClass, CachedHealthPercent)
	ATTRIBUTE_ACCESSORS(ThisClass, Stamina);
	ATTRIBUTE_ACCESSORS(ThisClass, MaxStamina);
	ATTRIBUTE_ACCESSORS(ThisClass, CachedStaminaPercent)
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//사전 속성 변경
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	//사후 게임 속성 변경
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData &Data)  override;

	// UPROPERTY(BlueprintAssignable)
	// FAttributesInitialized OnAttributesInitialized;
	//
	// UPROPERTY(ReplicatedUsing = OnRep_AttributesInitialized)
	// bool bAttributesInitialized = false;

	// UFUNCTION()
	// void OnRep_AttributesInitialized();
	//
	void RescaleHealth();
	void RescaleStamina();
	

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health)
	struct FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth)
	struct FGameplayAttributeData MaxHealth;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina)
	struct FGameplayAttributeData Stamina;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina)
	struct FGameplayAttributeData MaxStamina;

	UPROPERTY()
	FGameplayAttributeData CachedHealthPercent;
	
	UPROPERTY()
	FGameplayAttributeData CachedStaminaPercent;
	
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);
	
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);
	


	
};
