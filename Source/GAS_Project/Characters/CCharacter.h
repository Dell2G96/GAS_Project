// CCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "GAS_Project/Utils/CStructTypes.h"
#include "CCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FASCInitialized, UAbilitySystemComponent*, ASC, UAttributeSet*, AS);

UCLASS(Abstract)
class GAS_PROJECT_API ACCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()
protected:
	ACCharacter();
public:
	virtual void ServerSideInit(); 
	virtual void ClientSideInit();
	bool IsLocallyControlledByPlayer() const ;
	virtual  void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	const TMap<ECabilityInputID, TSubclassOf<class UGameplayAbility>>& GetAbilities() const;


protected: 
	virtual void BeginPlay() override;
	virtual  void PossessedBy(AController* NewController) override;


public:	
	virtual void Tick(float DeltaTime) override;
	
	/*********************************************************************/
	/*						Gameplay Ability                             */
	/*********************************************************************/
public:
 	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const ;
	virtual class UAttributeSet* GetAttributeSet() const ;

	UFUNCTION(Server, Reliable,WithValidation)
	void Server_SendGameplayEventToSelf(const FGameplayTag& EventTag,const FGameplayEventData& EventData);
	

protected:
	virtual void BindGASChangeDelegate();
	virtual void DeathTagUpdated(const FGameplayTag Tag, int32 NewCount);
	void StunTagUpdated(const FGameplayTag Tag, int32 NewCount);
	//void AimTagUpdated(const FGameplayTag Tag, int32 NewCount);
	// void SetIsAimming(bool bIsAimming);
	// virtual void OnAimStateChanged(bool bIsAimming);
	virtual void MaxHealthUpdated(const struct FOnAttributeChangeData& Data);
	virtual void MaxManaUpdated(const struct FOnAttributeChangeData& Data);
	
protected:
 	UPROPERTY(VisibleDefaultsOnly, Category="GAS|Gameplay Ability")
 	class UCAbilitySystemComponent* CAbilitySystemComponent;

	UPROPERTY()
	class UCAttributeSet* CAttributeSet;

	/*********************************************************************/
	/*						       UI                                    */
	/*********************************************************************/
protected:
	UPROPERTY(VisibleDefaultsOnly, Category="GAS|Gameplay Ability")
	class UWidgetComponent* OverHeadWidgetComponent;

	virtual void ConfigureOverHeadStatusWidget();

	UPROPERTY(EditDefaultsOnly, Category="GAS|UI")
	float HeadStatuGaugeVisiblityCheckUpdateGap = 1.f;

	UPROPERTY(EditDefaultsOnly, Category="GAS|UI")
	float HeadStatuGaugeVisiblityRangeSquared = 1000000.f;

	FTimerHandle HeadStatGuageVisibilityUpdateTimerHandle;

	
	virtual void UpdateHeadGuageVisibility();
	void SetStatusGaugeEnable(bool bIsEnabled);
	/*********************************************************************/
	/*						      Stun                                   */
	/*********************************************************************/
private:
	UPROPERTY(EditDefaultsOnly, Category="GAS|Stun")
	class UAnimMontage* StunMontage;

	virtual void OnStun();
	virtual void OnRecoverFromStun();
	
	/*********************************************************************/
	/*						Death And Respawn                            */
	/*********************************************************************/
public:
	bool IsDead() const;
	void RespawnImmediately();

private:
	FTransform MeshRelativeTransform;
	
	UPROPERTY(EditDefaultsOnly, Category="GAS|Death")
	float DeathMontageFinishTimerShift = -0.8f;
	
	UPROPERTY(EditDefaultsOnly, Category="GAS|Death")
	UAnimMontage* DeathMontage;

	FTimerHandle DeathMontageTimerHandle;
	void DeathMontageFinished();
	void SetRagdollEnabled(bool bIsEnabled);
	
	void PlayDeathAnim();
	void StartDeathSequence();
	void Respawn();
	
	virtual void OnDead();
	virtual void OnRespawn();
	/*********************************************************************/
	/*						Team ID			                             */
	/*********************************************************************/
public:
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID);
	virtual FGenericTeamId GetGenericTeamId() const;
private:
	UPROPERTY(ReplicatedUsing= OnRep_TeamID)
	FGenericTeamId TeamID;

	UFUNCTION()
	virtual void OnRep_TeamID();

	/*********************************************************************/
	/*						  AI			                             */
	/*********************************************************************/
private:
	
};


	
