

#include "LeeAttributeSet.h"

#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"

ULeeAttributeSet::ULeeAttributeSet()
	:Super()
{
}

class ULeeAbilitySystemComponent* ULeeAttributeSet::GetLeeAbilitySystemComponent() const
{
	return Cast<ULeeAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}
