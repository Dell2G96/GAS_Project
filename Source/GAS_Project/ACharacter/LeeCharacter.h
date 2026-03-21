// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GenericTeamAgentInterface.h"
#include "ModularCharacter.h"
#include "GameFramework/Character.h"
#include "GAS_Project/ATeam/LeeTeamAgentInterface.h"
#include "LeeCharacter.generated.h"


USTRUCT()
struct FLeeReplicatedAcceleration
{
	GENERATED_BODY()
	
	UPROPERTY()
	uint8 AccelXYRadians = 0;	// Direction of XY accel component, quantized to represent [0, 2*pi]

	UPROPERTY()
	uint8 AccelXYMagnitude = 0;	//Accel rate of XY component, quantized to represent [0, MaxAcceleration]

	UPROPERTY()
	int8 AccelZ = 0;	// Raw Z accel rate component, quantized to represent [-MaxAcceleration, MaxAcceleration]
};


USTRUCT()
struct FSharedRepMovement
{
	GENERATED_BODY()

	FSharedRepMovement();

	bool FillForCharacter(ACharacter* Character);
	bool Equals(const FSharedRepMovement& Other, ACharacter* Character) const;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	UPROPERTY(Transient)
	FRepMovement RepMovement;

	UPROPERTY(Transient)
	float RepTimeStamp = 0.0f;

	UPROPERTY(Transient)
	uint8 RepMovementMode = 0;

	UPROPERTY(Transient)
	bool bProxyIsJumpForceApplied = false;

	UPROPERTY(Transient)
	bool bIsCrouched = false;
};


template<>
struct TStructOpsTypeTraits<FSharedRepMovement> : public TStructOpsTypeTraitsBase2<FSharedRepMovement>
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};

UCLASS(Config = Game, Meta = (ShortTooltip = "The base character pawn class used by this project."))
class GAS_PROJECT_API ALeeCharacter : public AModularCharacter, public  IAbilitySystemInterface , public IGameplayCueInterface, public IGameplayTagAssetInterface, public ILeeTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALeeCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	UFUNCTION(BlueprintCallable, Category = "Lee|Character")
	class ALeePlayerController* GetLeePlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "Lee|Character")
	class ALeePlayerState* GetLeePlayerState() const;

	UFUNCTION(BlueprintCallable, Category="Lee|Character")
	class ULeeAbilitySystemComponent* GetLeeAbilitySystemComponent() const;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;

	void ToggleCrouch();
	
	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Reset() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	//~End of AActor interface

	//~APawn interface
	virtual void NotifyControllerChanged() override;
	//~End of APawn interface

	//~ILeeTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnLeeTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of ILeeTeamAgentInterface interface

	UFUNCTION(NetMulticast, unreliable)
	void FastSharedReplication(const FSharedRepMovement& SharedRepMovement);

	FSharedRepMovement LastSharedReplication;

	virtual bool UpdateSharedReplication();

protected:
	void OnAbilitySystemInitialized();
	void OnAbilitySystemUnInitialized();

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) final;

	void InitializeGameplayTags();
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;
	
	UFUNCTION()
	virtual void OnDeathStarted(AActor* OwningActor);

	UFUNCTION()
	virtual void OnDeathFinished(AActor* OwningActor);

	void DisableMovementAndCollision();
	void DestroyDueToDeath();
	void UninitAndDestroy();

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnDeathFinished"))
	void K2_OnDeathFinished();

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;
	void SetMovementModeTag(EMovementMode MovementMode, uint8 CustomMovementMode, bool bTagEnabled);

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual bool CanJumpInternal_Implementation() const;

private:
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lee|Character")
	// TSubclassOf<class ULeeInputComponent> InputComponentClass;

	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lee|Character",meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class ULeePawnExtensionComponent> PawnExtComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lee|Character" ,Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULeeHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lee|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULeeCameraComponent> CameraComponent;
	
	UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedAcceleration)
	FLeeReplicatedAcceleration ReplicatedAcceleration;

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	UPROPERTY()
	FOnLeeTeamIndexChangedDelegate OnTeamChangedDelegate;


protected:
	// Called to determine what happens to the team ID when possession ends
	virtual FGenericTeamId DetermineNewTeamAfterPossessionEnds(FGenericTeamId OldTeamID) const
	{
		// This could be changed to return, e.g., OldTeamID if you want to keep it assigned afterwards, or return an ID for some neutral faction, or etc...
		return FGenericTeamId::NoTeam;
	}

	
private:
	UFUNCTION()
	void OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	UFUNCTION()
	void OnRep_ReplicatedAcceleration();

	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);

};
