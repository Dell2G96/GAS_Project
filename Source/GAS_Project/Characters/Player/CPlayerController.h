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
	void HandleAbilityInputPressed(ECAbilityInputID InputId);
	void HandleAbilityInput(const FInputActionValue& InputActionValue, ECAbilityInputID InputID);
	void HandleAbilityInputReleased(ECAbilityInputID InputId);
	
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
	TMap<ECAbilityInputID, UInputAction*> GameplayAbilityInputActions;

	bool IsKnockdown();
	bool IsAlive();

	/*********************************************************************/
	/*							Execution							     */
	/*********************************************************************/
public:
	virtual void Tick(float DeltaSeconds) override;
protected:
	// [ADDED] “처형 가능” 프롬프트 위젯 클래스(BP에서 지정)
	UPROPERTY(EditDefaultsOnly, Category="UI|Execution")
	TSubclassOf<UUserWidget> ExecutionWidgetClass;

	// [ADDED] 로컬에서 생성되는 실제 위젯 인스턴스
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ExecutionWidget;

	// [ADDED] 서버가 “이 Enemy는 처형 UI 켜도 됨”이라고 통지한 후보들
	UPROPERTY(Transient)
	TSet<TObjectPtr<ACEnemyBase>> ExecutionCandidates;

	// [ADDED] 현재 화면에 찍고 있는 타겟
	UPROPERTY(Transient)
	TObjectPtr<ACEnemyBase> CurrentExecutionTarget;

	// [ADDED] 위젯 생성/갱신 함수
	void SpawnExecutionWidget();
	void RefreshExecutionTarget();
	void UpdateExecutionWidgetPosition();

public:
	// [ADDED] 서버 -> 클라 통지용(Client RPC)
	UFUNCTION(Client, Reliable)
	void Client_SetExecutionCandidate(class ACEnemyBase* Enemy, bool bShow);
	

public:
	// UFUNCTION(Client, Reliable)
	// void ClientSetExecutionPrompt(ACEnemyBase* TargetEnemy, bool bShow);
	//
	// UFUNCTION(Server, Reliable)
	// void ServerRequestExecution(ACEnemyBase* TargetEnemy);
	
	
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


private:
private:
	UPROPERTY()
	FVector2D SwitchDirection = FVector2D::ZeroVector;

	
	void Input_SwitchTargetTriggered(const FInputActionValue& InputActionValue);
	void Input_SwitchTargetCompleted(const FInputActionValue& InputActionValue);


	UFUNCTION(Server, Reliable)
	void Server_Input_SwitchTargetCompleted(const FGameplayTag& Tag);
	

};




