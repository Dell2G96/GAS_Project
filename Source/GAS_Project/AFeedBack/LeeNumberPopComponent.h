// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ControllerComponent.h"
#include "LeeNumberPopComponent.generated.h"

USTRUCT(BlueprintType)
struct FLeeNumberPopRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Lee|Number Pops")
	FVector WorldLocation;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Lee|Number Pops")
	FGameplayTagContainer SourceTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Lee|Number Pops")
	FGameplayTagContainer TargetTags;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Lee|Number Pops")
	int32 NumberToDisplay = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Lee|Number Pops")
	bool bIsCriticalDamage = false;

	FLeeNumberPopRequest()
		:WorldLocation(ForceInitToZero)
	{
		
	}
	
};

UCLASS(Abstract)
class GAS_PROJECT_API ULeeNumberPopComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	ULeeNumberPopComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category= Foo)
	virtual void AddNumberPop(const FLeeNumberPopRequest& NewRequest){}
	
	

};
