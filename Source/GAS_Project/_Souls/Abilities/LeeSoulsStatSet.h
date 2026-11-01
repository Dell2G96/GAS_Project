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
	ATTRIBUTE_ACCESSORS(ULeeSoulsStatSet, EstusCharges);
	ATTRIBUTE_ACCESSORS(ULeeSoulsStatSet, MaxEstusCharges);
	ATTRIBUTE_ACCESSORS(ULeeSoulsStatSet, EstusHealAmount);



	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Lee|SoulsStats")
	FGameplayAttributeData Health;


	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Lee|SoulsStats")
	FGameplayAttributeData MaxHealth;


	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "Lee|SoulsStats")
	FGameplayAttributeData Stamina;


	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "Lee|SoulsStats")
	FGameplayAttributeData MaxStamina;

	// 26.10.30 추가 - 에스트 남은 횟수 (GA_DrinkEstus의 Cost GE가 1씩 소모, 휴식/부활 시 Max로 충전)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EstusCharges, Category = "Lee|SoulsStats")
	FGameplayAttributeData EstusCharges;

	// 26.10.30 추가 - 에스트 최대 횟수 (기본 3)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxEstusCharges, Category = "Lee|SoulsStats")
	FGameplayAttributeData MaxEstusCharges;

	// 26.10.30 추가 - 에스트 1회 회복량 (고정값, 기본 40). 병 강화 시 GE로 이 값을 올린다
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EstusHealAmount, Category = "Lee|SoulsStats")
	FGameplayAttributeData EstusHealAmount;

	mutable FLeeAttributeEvent OnHealthChanged;
	mutable FLeeAttributeEvent OnStaminaChanged;
	mutable FLeeAttributeEvent OnMaxHealthChanged;
	mutable FLeeAttributeEvent OnMaxStaminaChanged;
	// 26.10.30 추가 - 에스트 횟수 변경 (HUD 표시용)
	mutable FLeeAttributeEvent OnEstusChargesChanged;
	
	// Delegate to broadcast when the health attribute reaches zero
	mutable FLeeAttributeEvent OnOutOfHealth;

	// [신규] 스태미나가 0에 도달했을 때 브로드캐스트 — LeeFinisherTargetComponent가 수신하여 그로기 GE 적용
	mutable FLeeAttributeEvent OnOutOfStamina;

	// [방어 시스템 — 빌드 검증 완료,] 데미지 판정 완료 브로드캐스트.
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

	// 변경 전에 에스트 횟수를 저장
	float EstusChargesBeforeAttributeChange;

protected:
	// 26.11.02 추가 - 서버 값이 클라이언트에 복제될 때 호출. ASC에 변경을 알리고 HUD용 이벤트를 브로드캐스트한다
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_EstusCharges(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxEstusCharges(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_EstusHealAmount(const FGameplayAttributeData& OldValue);

	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

};
