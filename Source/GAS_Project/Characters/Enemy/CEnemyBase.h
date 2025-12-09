// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "CEnemyBase.generated.h"

UCLASS()
class GAS_PROJECT_API ACEnemyBase : public ACCharacter
{
	GENERATED_BODY()

public:
	ACEnemyBase();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;

protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath() override;

	/*********************************************************************/
	/*						Death And Respawn                            */
	/*********************************************************************/
private:
	virtual void OnDead() override;
	virtual void OnRespawn() override;
//
// private:
// 	UPROPERTY(VisibleAnywhere)
// 	TObjectPtr<class UAbilitySystemComponent> AbilitySystemComponent;
//
// 	UPROPERTY()
// 	TObjectPtr<class UAttributeSet> AttributeSet;
};
