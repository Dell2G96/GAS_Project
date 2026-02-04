// CCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/GAS/CAttributeSet.h"
#include "GAS_Project/Utils/CStructTypes.h"
#include "CCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FASCInitialized, UAbilitySystemComponent*, ASC, UAttributeSet*, AS);

UCLASS(Abstract)
class GAS_PROJECT_API ACCharacter : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
    GENERATED_BODY()
public:
	ACCharacter();
	virtual  void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual  UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual  UAttributeSet* GetAttributeSet() const ;
	bool IsAlive() const { return bAlive; }
	void SetAlive(bool bAliveStatus) { bAlive = bAliveStatus; }
	
	void BindGASChangeDelegate();


	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartDeathSequence();

	UPROPERTY(BlueprintAssignable)
	FASCInitialized OnASCInitialized;

	UFUNCTION(BlueprintCallable, Category="GAS|Death")
	virtual void HandleRespawn();

	UFUNCTION(BlueprintCallable, Category="GAS|Death")
	void ResetAttributes();

protected:
	// void OnHealthChanged(const FOnAttributeChangeData& AttributeChangeData);
	// void OnStaminaChanged(const FOnAttributeChangeData& AttributeChangeData);
	virtual void HandleDeath();

private:
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true), Replicated)
	bool bAlive = true;

public:
	bool IsLocallyControlledByPlayer() const ;
	const TMap<ECAbilityInputID, TSubclassOf<class UGameplayAbility>>& GetAbilities() const;

protected: 
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;


public:	
	virtual void Tick(float DeltaTime) override;
	 
	/*********************************************************************/
	/*						Gameplay Ability                             */
	/*********************************************************************/
public:
	UFUNCTION(Server, Reliable,WithValidation)
	void Server_SendGameplayEventToSelf(const FGameplayTag& EventTag,const FGameplayEventData& EventData);

protected:
	void DeathTagUpdated(const FGameplayTag Tag, int32 NewCount);
	void StunTagUpdated(const FGameplayTag Tag, int32 NewCount);
	
	virtual void MaxHealthUpdated(const struct FOnAttributeChangeData& Data);
	virtual void MaxStaminaUpdated(const struct FOnAttributeChangeData& Data);
	
protected:
 	UPROPERTY(VisibleDefaultsOnly, Category="GAS|Gameplay Ability")
 	class UCAbilitySystemComponent* CAbilitySystemComponent;

	UPROPERTY()
	class UCAttributeSet* CAttributeSet;
	/*********************************************************************/
	/*						       Component                             */
	/*********************************************************************/

public:
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly ,Category="GAS|Components")
	class UMotionWarpingComponent* MotionWarpingComponent;
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
	
protected:
	void PlayDeathAnim();
	void StartDeathSequence();
	void Respawn();
	
	virtual void OnDead();
	virtual void OnRespawn();
	
private:
	FTransform MeshRelativeTransform;
	
	UPROPERTY(EditDefaultsOnly, Category="GAS|Death")
	float DeathMontageFinishTimerShift = -0.8f;

protected:
	UPROPERTY(EditDefaultsOnly, Category="GAS|Death")
	UAnimMontage* DeathMontage;
	
	FTimerHandle DeathMontageTimerHandle;
	void DeathMontageFinished();
	void SetRagdollEnabled(bool bIsEnabled);
	

	/*********************************************************************/
	/*								Team ID			                     */
	/*********************************************************************/
public:
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID);
	virtual FGenericTeamId GetGenericTeamId() const;
protected:
	UPROPERTY(EditDefaultsOnly,ReplicatedUsing= OnRep_TeamID)
	FGenericTeamId TeamID;

	UFUNCTION()
	virtual void OnRep_TeamID();

	/*********************************************************************/
	/*								  AI			                     */
	/*********************************************************************/

public:
	UFUNCTION(NetMulticast, reliable,BlueprintCallable)
   void Multicast_SendGameplayEventToActor(AActor* Target, FGameplayTag EventTag, FGameplayEventData Payload);



};


	
