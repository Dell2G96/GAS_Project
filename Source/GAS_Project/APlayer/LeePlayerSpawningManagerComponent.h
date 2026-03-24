// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "LeePlayerSpawningManagerComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeePlayerSpawningManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	ULeePlayerSpawningManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// UActorComponent
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~UActorComponent

protected:
	APlayerStart* GetFirstRandomUnoccupiedPlayerStart(AController* Controller, const TArray<class ALeePlayerStart*>& StartPoints) const;

	virtual void OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation) {}

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = OnChoosePlayerStart))
	AActor* K2_OnChoosePlayerStart(AController* Player, const TArray<ALeePlayerStart*>& PlayerStarts);

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = OnFinishRestartPlayer))
	void K2_OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation);

private:
	AActor* ChoosePlayerStart(AController* Player);
	bool ControllerCanRestart(AController* Player);
	void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation);
	friend class ALeeGameModeBase;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ALeePlayerStart>> CachedPlayerStarts;

private:
	void OnLevelAdded(ULevel* InLevel, UWorld* InWorld);
	void HandleOnActorSpawned(AActor* SpawnedActor);
	
#if WITH_EDITOR
	APlayerStart* FindPlayFromHereStart(AController* Player);
#endif

};
