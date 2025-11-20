// CPlayerController.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "GAS_Project/Utils/CStructTypes.h"
#include "CPlayerController.generated.h"

enum class ECabilityInputID : uint8;

UCLASS()
class GAS_PROJECT_API ACPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void SetupInputComponent() override;

private:
	UPROPERTY(EditDefaultsOnly, Category="GAS|Input")
	TArray<TObjectPtr<class UInputMappingContext>> InputMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Movement")
	TObjectPtr<class UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Movement")
	TObjectPtr<class UInputAction> RunAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Movement")
	TObjectPtr<class UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Movement")
	TObjectPtr<class UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<class UInputAction> EquipAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<class UInputAction> BasicAttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<class UInputAction> HeavyAttackAction;

	UPROPERTY(EditDefaultsOnly, Category="GAS|Input")
	TMap<ECabilityInputID, UInputAction*> GameplayAbilityInputActions;

private:
	void Jump();
	void StopJumping();
	void Run();
	void StopRuning();
	void Move(const struct FInputActionValue& Value);
	void Look(const struct FInputActionValue& Value);

	// ✅ 추가: Started/Completed로 분리
	void HandleAbilityInputPressed(ECabilityInputID InputId);
	void HandleAbilityInputReleased(ECabilityInputID InputId);

	void BasicAttack();
	void HeavyAttack();
	void Equip();
	void UnEquipTest();

	void ActivateAbility(const struct FGameplayTag& AbilityTag) const;
	bool IsAlive() const;
};
