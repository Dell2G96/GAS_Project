// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "CEnemyBase.generated.h"

UCLASS()
class GAS_PROJECT_API ACEnemyBase : public ACCharacter
{
	GENERATED_BODY()

public:
	ACEnemyBase();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	FORCEINLINE class UBoxComponent* GetLeftHandCollision() const { return LeftHandCollision; }
	FORCEINLINE class UBoxComponent* GetRightHandCollision() const { return RightHandCollision;}
protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath() override;

	/*********************************************************************/
	/*								Collision                            */
	/*********************************************************************/
#if WITH_EDITOR
	//~ Begin UObject Interface.
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface
#endif
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS|Collision")
	FName LeftHandSocket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS|Collision")
	FName RightHandSocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS|Collision")
	class UBoxComponent* LeftHandCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS|Collision")
	class UBoxComponent* RightHandCollision;

	
	/*********************************************************************/
	/*								Team ID                              */
	/*********************************************************************/
public:
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	
	

	/*********************************************************************/
	/*						Death And Respawn                            */
	/*********************************************************************/
private:
	virtual void OnDead() override;
	virtual void OnRespawn() override;

public:
	UFUNCTION(BlueprintCallable)
	FORCEINLINE UCAbilitySystemComponent* GetMyAbilitySystemComponent() const { return MyAbilitySystemComponent; }
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS|Ability")
	class UCAbilitySystemComponent* MyAbilitySystemComponent;

	/*********************************************************************/
	/*								AI									 */
	/*********************************************************************/
public:
	// ✅ AnimBP에서 Strafing 태그가 클라에서 안 보일 때를 대비한 복제 상태
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsStrafing() const { return bIsStrafing; }

	// ✅ 서버에서 Strafing 태그 변화를 감지해 bIsStrafing을 갱신
	void SetupStrafingReplicationBridge();
	void HandleStrafingTagChanged(const FGameplayTag Tag, int32 NewCount);

	

	// ✅ Strafing 상태(서버 → 모든 클라)
	UPROPERTY(ReplicatedUsing = OnRep_IsStrafing, VisibleAnywhere, BlueprintReadOnly, Category="GAS|Locomotion")
	bool bIsStrafing = false;

	UFUNCTION()
	void OnRep_IsStrafing();
	

};
