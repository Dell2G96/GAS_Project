// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "CCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FASCInitialized, UAbilitySystemComponent*, ASC, UAttributeSet*, AS);

UCLASS(Abstract)
class GAS_PROJECT_API ACCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACCharacter();

	/****************************************************************/
	/*						GamePlay Basic							*/
	/****************************************************************/
	// 네트워크
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	bool IsAlive() const{return bAlive;}
	void SetAlive(bool bAliveStatus){bAlive = bAliveStatus;};
protected:
	virtual void HandleDeath();
public:
	UPROPERTY(BlueprintAssignable)
	FASCInitialized OnASCInitialized;

	UFUNCTION(BlueprintCallable, Category="Crash|Death")
	virtual void HandleRespawn();

	UFUNCTION(BlueprintCallable, Category="Crash|Death")
	void ResetAttributes();
	
	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bAlive = true;
	
	/****************************************************************/
	/*						GamePlay Ability						*/
	/****************************************************************/
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const{return nullptr;};
	
protected:
	void GiveStartUpAbilities(); // 서버에서 1회만
	void InitAttributes() const; // 서버에서 1회만

	void OnHealthChanged(const struct FOnAttributeChangeData& AttributeChangeData);
	

protected:
	UPROPERTY(EditDefaultsOnly, Category="Crash|Abilities")
	TArray<TSubclassOf<class UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditDefaultsOnly, Category="Crash|Effects")
	TSubclassOf<class UGameplayEffect> InitializeAttributesEffects;

	UPROPERTY(EditDefaultsOnly, Category="Crash|Effects")
	TSubclassOf<class UGameplayEffect> ResetAttributesEffects;
	

};
