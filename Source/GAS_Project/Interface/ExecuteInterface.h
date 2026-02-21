
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ExecuteInterface.generated.h"

UINTERFACE(MinimalAPI)
class UExecuteInterface : public UInterface
{
	GENERATED_BODY()
};


class GAS_PROJECT_API IExecuteInterface
{
	GENERATED_BODY()

public:
	virtual void SetVictim(AActor* Victim) = 0;
	virtual void PlayVictimMontage(int AttackIndex, AActor* Attacker) = 0;
	virtual void ActivateBloodTrail() = 0;

	// 추가
	virtual bool CanBeExecuted() const = 0;

	virtual void StartExecution(AActor* Target) = 0;
	
};
