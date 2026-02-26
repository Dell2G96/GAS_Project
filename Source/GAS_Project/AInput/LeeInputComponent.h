// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "GameplayTagContainer.h"
#include "LeeInputConfig.h"
#include "LeeInputComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	ULeeInputComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	template<class UserClass, typename FuncType>
	void BindNativeAction(const class ULeeInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound);

	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityAction(const class ULeeInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc , TArray<uint32>& BindHandles, bool bLogIfNotFound);
	
};

template <class UserClass, typename FuncType>
void ULeeInputComponent::BindNativeAction(const class ULeeInputConfig* InputConfig, const FGameplayTag& InputTag,
	ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound)
{
	check(InputConfig);

	if (const UInputAction* IA = InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound))
	{
		BindAction(IA,TriggerEvent, Object, Func);
	}
}

template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void ULeeInputComponent::BindAbilityAction(const class ULeeInputConfig* InputConfig, const FGameplayTag& InputTag,
	ETriggerEvent TriggerEvent, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc,
	TArray<uint32>& BindHandles, bool bLogIfNotFound)
{
	check(InputConfig);

	for (const FLeeInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (PressedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Triggered,Object, PressedFunc, Action.InputTag).Gethandle());	
			}
			if (ReleasedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Completed,Object, ReleasedFunc, Action.InputTag).Gethandle());	
			}
		}
	}
	
}
