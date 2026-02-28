// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameModeBase.h"

#include "LeeExperienceDefinition.h"
#include "LeeExperienceManagerComponent.h"
#include "LeeGameState.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/ACharacter/LeeCharacter.h"
#include "GAS_Project/APlayer/LeePlayerController.h"
#include "GAS_Project/APlayer/LeePlayerState.h"
#include "GAS_Project/ACharacter/LeePawnData.h"
#include "GAS_Project/ACharacter/LeePawnExtensionComponent.h"
#include "Kismet/GameplayStatics.h"

ALeeGameModeBase::ALeeGameModeBase()
{
	GameStateClass = ALeeGameState::StaticClass();
	PlayerControllerClass = ALeePlayerController::StaticClass();
	PlayerStateClass = ALeePlayerState::StaticClass();
	DefaultPawnClass = ALeeCharacter::StaticClass();
}

void ALeeGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);


	// 아직 게임인스턴스를 통해 초기화 작업이 진행되므로
	// 현 프레임에서는 Lyra의 Experience 를 처리 할 수 없음
	// 따라서 한 프레임 뒤에서 이벤트를 받아서 처리
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleMatcheAssignmentIfNotExpectingOne);
}

void ALeeGameModeBase::InitGameState()
{
	Super::InitGameState();

	ULeeExperienceManagerComponent* ExperienceManagerComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
	check(ExperienceManagerComponent);

	ExperienceManagerComponent->CallOrRegister_OnExperienceLoaded(FOnLeeExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
	
}

UClass* ALeeGameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const ULeePawnData* PawnData = GetPawnDataForController(InController))
	{
		if (PawnData->PawnClass)
		{
			return PawnData->PawnClass;
		}
	}
	
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ALeeGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (IsExperienceLoaded())
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
}

APawn* ALeeGameModeBase::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer,
	const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;
	SpawnInfo.bDeferConstruction = true;

	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo))
		{
			if (ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn))
			{
				if (const ULeePawnData* PawnData = GetPawnDataForController(NewPlayer))
				{
					PawnExtComp->SetPawnData(PawnData);
				}
			}
			SpawnedPawn->FinishSpawning(SpawnTransform);
			return SpawnedPawn;
		}
	}
	return nullptr;
}

void ALeeGameModeBase::HandleMatcheAssignmentIfNotExpectingOne()
{
	FPrimaryAssetId ExperienceId;

	UWorld* World = GetWorld();

	if (!ExperienceId.IsValid() && UGameplayStatics::HasOption(OptionsString, TEXT("Experience")))
	{
		const FString ExperienceFromOptions = UGameplayStatics::ParseOption(OptionsString, TEXT("Experience"));
		ExperienceId = FPrimaryAssetId(FName(FPrimaryAssetType((*ULeeExperienceDefinition::StaticClass()->GetName()))),FName(*ExperienceFromOptions));
	}
	if (!ExperienceId.IsValid()) 
	{
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType("LeeExperienceDefinition"), FName("B_DefaultExperience"));
	}
	OnMatchAssignmentGiven(ExperienceId);
}

void ALeeGameModeBase::OnMatchAssignmentGiven(const FPrimaryAssetId& ExperienceId)
{
	check(ExperienceId.IsValid());

	ULeeExperienceManagerComponent* ExperienceManagerComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
	check(ExperienceManagerComponent);
	ExperienceManagerComponent->ServerSetCurrentExperience(ExperienceId);
}

bool ALeeGameModeBase::IsExperienceLoaded() const
{
	check(GameState);
	ULeeExperienceManagerComponent* ExperienceManagerComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
	check(ExperienceManagerComponent);
	
	return ExperienceManagerComponent->IsExperienceLoaded();
}

void ALeeGameModeBase::OnExperienceLoaded(const ULeeExperienceDefinition* CurrentExperience)
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Cast<APlayerController>(*Iterator);

		if (PC && PC->GetPawn() == nullptr)
		{
			if (PlayerCanRestart(PC))
			{
				RestartPlayer(PC);
			}
		}
	}
}

const class ULeePawnData* ALeeGameModeBase::GetPawnDataForController(const AController* InController) const
{
	if (InController)
	{
		if (const ALeePlayerState* LeePS = InController->GetPlayerState<ALeePlayerState>())
		{
			if (const ULeePawnData* PawnData = LeePS->GetPawnData<ULeePawnData>())
			{
				return PawnData;
			}
		}
	}
	check(GameState);
	ULeeExperienceManagerComponent* ExperienceManagerComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
	check(ExperienceManagerComponent);

	if (ExperienceManagerComponent->IsExperienceLoaded())
	{
		const ULeeExperienceDefinition* Experience = ExperienceManagerComponent->GetCurrentExperienceChecked();
		if (Experience->DefaultPawnData)
		{
			return Experience->DefaultPawnData;
		}
	}

	return nullptr;
}
