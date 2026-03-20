// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeBotCreationComponent.h"

#include "AIController.h"
#include "LeeExperienceManagerComponent.h"
#include "LeeGameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/ACharacter/LeeHealthComponent.h"
#include "GAS_Project/ACharacter/LeePawnExtensionComponent.h"
#include "Kismet/GameplayStatics.h"

ULeeBotCreationComponent::ULeeBotCreationComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void ULeeBotCreationComponent::BeginPlay()
{
	Super::BeginPlay();

	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	ULeeExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded_LowPriority(FOnLeeExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void ULeeBotCreationComponent::OnExperienceLoaded(const class ULeeExperienceDefinition* Experience)
{
	
#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		ServerCreateBots();
	}
#endif
	
}

#if WITH_SERVER_CODE
//
// void ULeeBotCreationComponent::ServerCreateBots_Implementation()
// {
// 	if (BotControllerClass == nullptr)
// 	{
// 		return;
// 	}
//
// 	RemainingBotNames = RandomBotNames;
//
// 	// Determine how many bots to spawn
// 	int32 EffectiveBotCount = NumBotsToCreate;
//
// 	// Give the developer settings a chance to override it
// 	if (GIsEditor)
// 	{
// 		const ULeeDeveloperSettings* DeveloperSettings = GetDefault<ULeeDeveloperSettings>();
// 		
// 		if (DeveloperSettings->bOverrideBotCount)
// 		{
// 			EffectiveBotCount = DeveloperSettings->OverrideNumPlayerBotsToSpawn;
// 		}
// 	}
//
// 	// Give the URL a chance to override it
// 	if (AGameModeBase* GameModeBase = GetGameMode<AGameModeBase>())
// 	{
// 		EffectiveBotCount = UGameplayStatics::GetIntOption(GameModeBase->OptionsString, TEXT("NumBots"), EffectiveBotCount);
// 	}
//
// 	// Create them
// 	for (int32 Count = 0; Count < EffectiveBotCount; ++Count)
// 	{
// 		SpawnOneBot();
// 	}
// }

FString ULeeBotCreationComponent::CreateBotName(int32 PlayerIndex)
{
	FString Result;
	if (RemainingBotNames.Num() > 0)
	{
		const int32 NameIndex = FMath::RandRange(0, RemainingBotNames.Num() - 1);
		Result = RemainingBotNames[NameIndex];
		RemainingBotNames.RemoveAtSwap(NameIndex);
	}
	else
	{
		//@TODO: PlayerId is only being initialized for players right now
		PlayerIndex = FMath::RandRange(260, 260+100);
		Result = FString::Printf(TEXT("Tinplate %d"), PlayerIndex);
	}
	return Result;
}

void ULeeBotCreationComponent::SpawnOneBot()
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = GetComponentLevel();
	SpawnInfo.ObjectFlags |= RF_Transient;
	AAIController* NewController = GetWorld()->SpawnActor<AAIController>(BotControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);

	if (NewController != nullptr)
	{
		ALeeGameModeBase* GameMode = GetGameMode<ALeeGameModeBase>();
		check(GameMode);

		if (NewController->PlayerState != nullptr)
		{
			NewController->PlayerState->SetPlayerName(CreateBotName(NewController->PlayerState->GetPlayerId()));
		}
		GameMode->GenericPlayerInitialization(NewController);
		GameMode->RestartPlayer(NewController);

		if (NewController->GetPawn() != nullptr)
		{
			if (ULeePawnExtensionComponent* PawnExtensionComponent = NewController->GetPawn()->FindComponentByClass<ULeePawnExtensionComponent>())
			{
				PawnExtensionComponent->CheckDefaultInitialization();
			}
		}

		SpawnedBotList.Add(NewController);
	}
}


void ULeeBotCreationComponent::RemoveOneBot()
{
	if (SpawnedBotList.Num() > 0)
	{
		const int32 BotToRemoveIndex = FMath::RandRange(0, SpawnedBotList.Num() -1);

		AAIController* BotToRmove = SpawnedBotList[BotToRemoveIndex];
		SpawnedBotList.RemoveAtSwap(BotToRemoveIndex);

		if (BotToRmove)
		{
			if (APawn* ControlledPawn = BotToRmove->GetPawn())
			{
				if (ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(ControlledPawn))
				{
					HealthComponent->DamageSelfDestruct();
				}
				else
				{
					ControlledPawn->Destroy();
				}
			}

			BotToRmove->Destroy();
		}
	}
}

void ULeeBotCreationComponent::ServerCreateBots()
{
	UE_LOG(LogLee, Error, TEXT("Create BOt spawn"));

	if (BotControllerClass == nullptr)
	{
		return;
	}
	
	RemainingBotNames = RandomBotNames;
	int32 EffectiveBotCount = NumBotsToCreate;
	for (int32 Count = 0; Count < EffectiveBotCount; ++Count)
	{
		SpawnOneBot();
	}
	
}


#else // !WITH_SERVER_CODE
void ULeeBotCreationComponent::ServerCreateBots_Implementation()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in LeeClient!"));
}

void ULeeBotCreationComponent::SpawnOneBot()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in LeeClient!"));
}

void ULeeBotCreationComponent::RemoveOneBot()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in LeeClient!"));
}

#endif




