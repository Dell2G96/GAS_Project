// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "UnrealWidgetFwd.h"
#include "Components/WidgetComponent.h"
#include "CWidgetComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAS_PROJECT_API UCWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

	
protected:
	virtual void BeginPlay() override;

	
	UPROPERTY(EditAnywhere)
	TMap<struct FGameplayAttribute, struct FGameplayAttribute> AttributeMap;
	
	TWeakObjectPtr<class ACCharacter> BaseCharacter;
	TWeakObjectPtr<class UCAbilitySystemComponent> AbilitySystemComponent;
	TWeakObjectPtr<class UCAttributeSet> AttributeSet;
	
	void InitialAbilitySystemData();
	bool IsASCInitialized() const;
	void InitializeAttributeDelegate();
	void BindWidgetToAttributeChanges(UWidget* WidgetObject, const TTuple<FGameplayAttribute,FGameplayAttribute>& Pair) const;
	
	UFUNCTION()
	void OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS);
	
	UFUNCTION()
	void BindToAttributeChange();

};
