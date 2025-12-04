// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GAS_Project/Utils/CStructTypes.h"
#include "GameplayWidget.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UGameplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	// void ConfigureAbilities(const TMap<ECabilityInputID, TSubclassOf<class UGameplayAbility>>& Abilities);

public:
	UPROPERTY(meta=(BindWidget))
	class UValueGauge* HealthBar;

	UPROPERTY(meta=(BindWidget))
	class UValueGauge* ManaBar;

	// UPROPERTY(meta=(BindWidget))
	// class UAbilityListView* AbilityListView;
	

	// void PlayShopPopupAnimation(bool bPlayForward);
	// void SetOwiningPawnInputEnabled(bool bPawnInputEnabled);
	// void SetShowMouseCursor(bool bShowMouseCursor);
	// void SetFoucsToGameAndUI();
	// void SetFoucsToGameOnly();

	UPROPERTY()
	class UCAbilitySystemComponent* OwnerAbilitySystemComponent;
	
};
