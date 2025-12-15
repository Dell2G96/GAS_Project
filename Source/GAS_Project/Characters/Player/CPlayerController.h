// CPlayerController.h
#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "GAS_Project/Utils/CStructTypes.h"
#include "CPlayerController.generated.h"

UCLASS()
class GAS_PROJECT_API ACPlayerController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// 서버에서만 실행
	//void OnPossess(APawn* InPawn) override;
	void OnPossess(APawn* NewPawn) override;
	
	//클라이언트에서만 실행 , ListenServer
	void AcknowledgePossession(class APawn* NewPawn) override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;


	//
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupInputComponent() override;
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendGameplayEventToSelf(const FGameplayTag& EventTag, const FGameplayEventData& EventData);

private:
	void Jump();
	void StopJumping();
	void Run();
	void StopRuning();
	void Move(const struct FInputActionValue& Value);
	void Look(const struct FInputActionValue& Value);


	// ✅ 추가: Started/Completed로 분리
	void HandleAbilityInputPressed(ECabilityInputID InputId);
	void HandleAbilityInput(const FInputActionValue& InputActionValue, ECabilityInputID InputID);
	void HandleAbilityInputReleased(ECabilityInputID InputId);
	
	// void LearnAbiltiyLeaderDown(const FInputActionValue& InputActionValue);
	// void LearnAbiltiyLeaderUp(const FInputActionValue& InputActionValue);
	// bool bIsLearnAbilityLeaderDown = false;


	void ActivateAbility(const struct FGameplayTag& AbilityTag) const;

	// Pawn(캐릭터)로 이벤트 전송
	void SendEventToPawn(const struct FGameplayTag& Tag);

	/*********************************************************************/
	/*						Input Mapping                                */
	/*********************************************************************/
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
	
	UPROPERTY(EditDefaultsOnly, Category="GAS|Input")
	TMap<ECabilityInputID, UInputAction*> GameplayAbilityInputActions;

	bool IsKnockdown();
	bool IsAlive();
	/*********************************************************************/
	/*								UI							        */
	/*********************************************************************/
private:
	void SpawnGameplayWidget();

	UPROPERTY(EditDefaultsOnly, Category="GAS|UI")
	TSubclassOf<class UGameplayWidget> GameplayWidgetClass;

	UPROPERTY()
	class UGameplayWidget* GameplayWidget;
	
private:
	UPROPERTY()
	class ACPlayerCharacter* OwnerCharacter;

	// UPROPERTY(EditDefaultsOnly, Category="GAS|Input")
	// TSubclassOf<class UGameplaywidget> GameplayWidgetClass;

	UPROPERTY(Replicated)
	FGenericTeamId TeamID;
	
};


