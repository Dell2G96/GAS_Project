// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "LeeAttributeSet.generated.h"

/**
 * 
 */

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

// * 속성 이벤트를 브로드캐스트하는 데 사용되는 델리게이트입니다.
// * @param EffectInstigator   이 이벤트를 최초로 발생시킨 액터
// * @param EffectCauser       실제로 변경을 유발한 물리적 액터
// * @param EffectSpec         이 변경에 대한 전체 이펙트 스펙
// * @param EffectMagnitude    클램핑 이전의 원시 크기값
// * @param OldValue           속성이 변경되기 이전의 값
// * @param NewValue           변경된 이후의 값

DECLARE_MULTICAST_DELEGATE_SixParams(FLeeAttributeEvent,
	AActor* /*EffectInstigator*/,
	AActor* /*EffectCauser*/,
	const struct FGameplayEffectSpec* /*EffectSpec*/,
	float /*EffectMagnitude*/,
	float /*OldValue*/,
	float /*NewValue*/);


UCLASS()
class GAS_PROJECT_API ULeeAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	ULeeAttributeSet();

	class ULeeAbilitySystemComponent* GetLeeAbilitySystemComponent() const;
};
