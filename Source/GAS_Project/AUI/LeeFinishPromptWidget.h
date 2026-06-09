// // Fill out your copyright notice in the Description page of Project Settings.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "ActorIndicatorWidget.h"
// #include "Blueprint/UserWidget.h"
// #include "GameplayTagContainer.h"
// #include "LeeFinishPromptWidget.generated.h"
//
// class UImage;
// class UIndicatorDescriptor;
// class UMaterialInstanceDynamic;
// class UAbilitySystemComponent;
//
// /**
//  * Finish prompt widget base for W_FinishPrompt.
//  * The indicator descriptor supplies the enemy actor as DataObject.
//  */
// UCLASS(Abstract, Blueprintable)
// class GAS_PROJECT_API ULeeFinishPromptWidget : public UUserWidget, public IIndicatorWidgetInterface
// {
// 	GENERATED_BODY()
//
// public:
// 	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
//
// 	virtual void BindIndicator_Implementation(UIndicatorDescriptor* Indicator) override;
// 	virtual void UnbindIndicator_Implementation(const UIndicatorDescriptor* Indicator) override;
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lee|Finish")
// 	AActor* GetFinishTargetActor() const { return FinishTarget.Get(); }
//
// 	UFUNCTION(BlueprintCallable, Category = "Lee|Finish")
// 	void SetFinishTargetActor(AActor* TargetActor);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lee|Finish")
// 	float GetFinishProgress() const { return CachedProgress; }
//
// protected:
// 	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Lee|Finish")
// 	TObjectPtr<UImage> RingImage;
//
// 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finish")
// 	FName ProgressMaterialParameterName = TEXT("Progress");
//
// 	UFUNCTION(BlueprintImplementableEvent, Category = "Lee|Finish")
// 	void OnFinishProgressUpdated(float NewProgress);
//
// private:
// 	void RefreshRingMaterial();
// 	float QueryFinishProgress() const;
// 	float QueryEffectProgress(UAbilitySystemComponent* ASC, FGameplayTag EffectTag) const;
//
// 	UPROPERTY(Transient)
// 	TObjectPtr<UIndicatorDescriptor> BoundIndicator;
//
// 	UPROPERTY(Transient)
// 	TWeakObjectPtr<AActor> FinishTarget;
//
// 	UPROPERTY(Transient)
// 	TObjectPtr<UMaterialInstanceDynamic> RingDynamicMaterial;
//
// 	float CachedProgress = 0.0f;
// };
