// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ValueGauge.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UValueGauge : public UUserWidget
{
	GENERATED_BODY()

	
public:
	virtual void NativePreConstruct() override;
	void SetAndBoundToGameplayAttribute(class UAbilitySystemComponent* AbilitySystemComponent, const struct FGameplayAttribute& Attribute, const FGameplayAttribute& MaxAttribute);
	void SetValue(float NewValue, float NewMaxValue);
	
private:
	void ValueChanged(const struct FOnAttributeChangeData& ChangedData);
	void MaxValueChanged(const struct FOnAttributeChangeData& ChangedData);

	float CachedValue;
	float CachedMaxValue;
	
	UPROPERTY(EditAnywhere, Category="Visual")
	FLinearColor BarColor;

	UPROPERTY(EditAnywhere, Category="Visual")
	FSlateFontInfo ValueTextFont;

	UPROPERTY(EditAnywhere, Category="Visual")
	bool bValueTextVisible = true;

	UPROPERTY(EditAnywhere, Category="Visual")
	bool bProgressBarVisible = true;
	
	UPROPERTY(VisibleAnywhere, meta=(BindWidget))
	class UProgressBar* ProgressBar;

	UPROPERTY(VisibleAnywhere, meta=(BindWidget))
	class UTextBlock* ValueText;
};
