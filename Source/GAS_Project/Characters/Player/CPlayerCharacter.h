#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "CPlayerCharacter.generated.h"

UCLASS()
class GAS_PROJECT_API ACPlayerCharacter : public ACCharacter
{
	GENERATED_BODY()

public:
	ACPlayerCharacter();

	// ASC/AttributeSet은 PlayerState 보유 것을 사용
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual class UAttributeSet* GetAttributeSet() const override;
	// 서버: 소유권 확정 시 PS의 ASC를 Character(AVAtar) 와 연결
	virtual void PossessedBy(AController* NewController) override;
	// 클라 : PS 복제 도착 시 동일 초기화
	virtual void OnRep_PlayerState() override;

	void ServerSideInit();
	void ClientSideInit();
	
	// virtual void DeathTagUpdated(const FGameplayTag Tag, int32 NewCount) override;
	virtual void MaxHealthUpdated(const struct FOnAttributeChangeData& Data) override;
	virtual void MaxStaminaUpdated(const struct FOnAttributeChangeData& Data) override;
	
	virtual void BeginPlay() override;
	virtual void PawnClientRestart() override;
	virtual  void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	
	// 무기 컴포넌트: 플레이어 전용 컴포넌트로 단일화
	UFUNCTION(BlueprintCallable)
	FORCEINLINE class UCWeaponComponent* GetWeaponComponent() const { return WeaponComponent.Get(); }

private:
	/********************************************************/
	/*						Stun                            */
	/********************************************************/
	virtual void OnStun() override;
	virtual void OnRecoverFromStun() override;

	/**********************************************************/
	/*					Knocked Down State                    */
	/**********************************************************/
public:
	UFUNCTION(BlueprintCallable, Category="GAS|Knockdown")
	bool IsKnockedDown() const;

	void KnockdownTagUpdated(const FGameplayTag Tag, int32 NewCount);
private:
	void StartKnockdownSequence();
	virtual void OnKnockdown();
private:
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true), Replicated)
	bool bKnockdown = false;
	
private:
	// UPROPERTY(EditDefaultsOnly, Category="GAS|UI")
	// TSubclassOf<>
	
	

	/********************************************************/
	/*                     Death and Respawn                */
	/********************************************************/
	virtual void OnDead() override;
	virtual void OnRespawn() override;
	
	/********************************************************/
	/*                     Camera View		                */
	/********************************************************/
	
	UPROPERTY(VisibleAnywhere, Category="GAS|Camera")
	TObjectPtr<class USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category="GAS|Camera")
	TObjectPtr<class UCameraComponent> Camera;
	
	UPROPERTY(VisibleAnywhere, Category="GAS|Comp")
	TObjectPtr<class UCWeaponComponent> WeaponComponent;

	UPROPERTY(Transient)
	class ACPlayerController* OwnerController = nullptr;

	UPROPERTY(Transient)
	class ACPlayerState* CachedPlayerState = nullptr;

	//추가 함수
public:
	void EnterKnockdown();
	void BeginRevive(AActor* Reviver);
	void CancleRevive();
	void CompleteRevive();

private:
	FTimerHandle KnockdownTimerHandle;

	float KnockdownTotalTime = 10.f;
	float KnockdownRemainingTime = 10.f;

	bool bBeingRevived = false;

	void StartKnockdownTimer(float Time);
	void OnKnockdownExpired();
};
