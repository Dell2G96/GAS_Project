// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "LeeBotCreationComponent.generated.h"


UCLASS(Blueprintable, Abstract)
class GAS_PROJECT_API ULeeBotCreationComponent : public UGameStateComponent
{
	GENERATED_BODY()
public:
	ULeeBotCreationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

private:
	void OnExperienceLoaded(const class ULeeExperienceDefinition* Experience);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Gameplay)
	int32 NumBotsToCreate = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Gameplay)
	TSubclassOf<class AAIController> BotControllerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Gameplay)
	TArray<FString> RandomBotNames;

	TArray<FString> RemainingBotNames;

protected:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AAIController>> SpawnedBotList;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Gameplay)
	virtual void SpawnOneBot();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, category=Gameplay)
	virtual void RemoveOneBot();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, category=Gameplay)
	void ServerCreateBots();

	
#if WITH_SERVER_CODE
public:
	void Cheat_AddBot() { SpawnOneBot(); }
	void Cheat_RemoveBot() { RemoveOneBot(); }

	FString CreateBotName(int32 PlayerIndex);
#endif
	

};
