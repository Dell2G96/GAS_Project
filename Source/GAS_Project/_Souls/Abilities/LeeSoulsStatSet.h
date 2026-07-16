// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS_Project/AAbilitySystem/AttributeSets/LeeAttributeSet.h"
#include "LeeSoulsStatSet.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeSoulsStatSet : public ULeeAttributeSet
{
	GENERATED_BODY()

public:
	ULeeSoulsStatSet();

	ATTRIBUTE_ACCESSORS(ULeeSoulsStatSet, Health);
	ATTRIBUTE_ACCESSORS(ULeeSoulsStatSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(ULeeSoulsStatSet, Stamina);
	ATTRIBUTE_ACCESSORS(ULeeSoulsStatSet, MaxStamina);



	UPROPERTY(BlueprintReadOnly, Category = "Lee|SoulsStats")
	FGameplayAttributeData Health;


	UPROPERTY(BlueprintReadOnly, Category = "Lee|SoulsStats")
	FGameplayAttributeData MaxHealth;


	UPROPERTY(BlueprintReadOnly, Category = "Lee|SoulsStats")
	FGameplayAttributeData Stamina;


	UPROPERTY(BlueprintReadOnly, Category = "Lee|SoulsStats")
	FGameplayAttributeData MaxStamina;

	mutable FLeeAttributeEvent OnHealthChanged;
	mutable FLeeAttributeEvent OnStaminaChanged;
	mutable FLeeAttributeEvent OnMaxHealthChanged;
	mutable FLeeAttributeEvent OnMaxStaminaChanged;
	
	// Delegate to broadcast when the health attribute reaches zero
	mutable FLeeAttributeEvent OnOutOfHealth;

	// [신규] 스태미나가 0에 도달했을 때 브로드캐스트 — LeeFinisherTargetComponent가 수신하여 그로기 GE 적용
	mutable FLeeAttributeEvent OnOutOfStamina;

	// [방어 시스템 — 빌드 검증 완료, 학습용 주석 처리] 데미지 판정 완료 브로드캐스트.
	// ULeeExecCalc_Damage가 Spec DynamicAssetTags에 기록한 판정 결과(Souls.DamageResult.*)를
	// 어트리뷰트 변경 확정 후 알린다 → ULeeDefenseComponent가 수신해 이벤트 발송/GE 적용 담당.
	// (ExecCalc 내부에서 이벤트를 직접 쏘지 않기 위한 구조 변경, 리팩토링)
	mutable FLeeAttributeEvent OnDamageResolved;

	// 체력이 0이 되는 시점을 추적
	bool bOutOfHealth;

	// 스태미나 0 도달 여부 추적 (중복 브로드캐스트 방지)
	bool bOutOfStamina;

	// 변경 전에 체력을 저장
	float MaxHealthBeforeAttributeChange;
	float HealthBeforeAttributeChange;

	// 변경 전에 스태미나를 저장
	float MaxStaminaBeforeAttributeChange;
	float StaminaBeforeAttributeChange;

protected:
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

};
