// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameUIManagerSubsystem.generated.h"

/**
 * 
 */
UCLASS(Abstract, config=Game)
class COMMONGAME_API UGameUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UGameUIManagerSubsystem();

	void SwitchToPolicy(class UGameUIPolicy* InPolicy);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void NotifyPlayerAdd(class UCommonLocalPlayer* LocalPlayer);
	virtual void NotifyPlayerRemoved(UCommonLocalPlayer* LocalPlayer);
	virtual void NotifyPlayerDestroyed(UCommonLocalPlayer* LocalPlayer);

	const UGameUIPolicy* GetCurrentUIPolicy() const {return CurrentPolicy ;}
	UGameUIPolicy* GetCurrentUIPolicy () {return CurrentPolicy ;}

	UPROPERTY(Transient)
	TObjectPtr<UGameUIPolicy> CurrentPolicy;

	UPROPERTY(Config, EditAnywhere)
	TSoftClassPtr<UGameUIPolicy> DefaultUIPolicyClass;


	
};
