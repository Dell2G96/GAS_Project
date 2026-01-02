// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CAniminstance.h"
#include "CPlayerAnim.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UCPlayerAnim : public UCAniminstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly ,Category="AnimData|References")
	class ACPlayerCharacter* OwningPlayerCharacter;

	
	
};
