
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ACPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void SetupInputComponent() override;

private:
	UPROPERTY(EditDefaultsOnly, Category="A|Input")
	TArray<TObjectPtr<class UInputMappingContext>> InputMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category = "A|Input|Movement")
	TObjectPtr<class UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "A|Input|Movement")
	TObjectPtr<class UInputAction> RunAction;

	UPROPERTY(EditDefaultsOnly, Category = "A|Input|Movement")
	TObjectPtr<class UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "A|Input|Movement")
	TObjectPtr<class UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "A|Input|Abilities")
	TObjectPtr<class UInputAction> PrimaryAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "A|Input|Abilities")
	TObjectPtr<class UInputAction> SecondaryAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "A|Input|Abilities")
	TObjectPtr<class UInputAction> TertiaryAction;

	void Jump();
	void StopJumping();
	void Run();
	void StopRuning();
	void Move(const struct FInputActionValue& Value);
	void Look(const struct FInputActionValue& Value);
	void Primary();
	void Tertiary();
	void Secondary();
	void ActivateAbility(const struct FGameplayTag& AbilityTag) const;

	bool IsAlive() const;

};
