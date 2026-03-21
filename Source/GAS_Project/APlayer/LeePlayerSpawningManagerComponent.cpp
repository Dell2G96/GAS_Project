// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePlayerSpawningManagerComponent.h"

#include "EngineUtils.h"
#include "LeePlayerStart.h"
#include "Engine/PlayerStartPIE.h"
#include "GameFramework/PlayerState.h"
#include "Slate/SGameLayerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPlayerSpawning, Log, All);

ULeePlayerSpawningManagerComponent::ULeePlayerSpawningManagerComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(false);
	bAutoRegister = true;
	bAutoActivate = true;
	bWantsInitializeComponent = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULeePlayerSpawningManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	FWorldDelegates::LevelAddedToWorld.AddUObject(this, &ThisClass::OnLevelAdded);

	UWorld* World = GetWorld();
	World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &ThisClass::HandleOnActorSpawned));

	for (TActorIterator<ALeePlayerStart> It(World); It; ++It)
	{
		if (ALeePlayerStart* PlayerStart = *It)
		{
			CachedPlayerStarts.Add(PlayerStart);
		}
	}
}

void ULeePlayerSpawningManagerComponent::OnLevelAdded(ULevel* InLevel, UWorld* InWorld)
{
	if (InWorld ==GetWorld())
	{
		for (AActor* Actor : InLevel->Actors)
		{
			if (ALeePlayerStart* PlayerStart = Cast<ALeePlayerStart>(Actor))
			{
				ensure(!CachedPlayerStarts.Contains(PlayerStart));
				CachedPlayerStarts.Add(PlayerStart);
			}
		}
	}
}

void ULeePlayerSpawningManagerComponent::HandleOnActorSpawned(AActor* SpawnedActor)
{
	if (ALeePlayerStart* PlayerStart = Cast<ALeePlayerStart>(SpawnedActor))
	{
		CachedPlayerStarts.Add(PlayerStart);
	}
}

AActor* ULeePlayerSpawningManagerComponent::ChoosePlayerStart(AController* Player)
{
	if (Player)
	{
#if WITH_EDITOR
		if (APlayerStart* PlayerStart = FindPlayFromHereStart(Player))
		{
			return PlayerStart;
		}
#endif

		TArray<ALeePlayerStart*> StarterPoints;
		for (auto StartIt = CachedPlayerStarts.CreateIterator(); StartIt; ++StartIt)
		{
			if (ALeePlayerStart* Start = (*StartIt).Get())
			{
				StarterPoints.Add(Start);
			}
			else
			{
				StartIt.RemoveCurrent();
			}
		}

		if (APlayerState* PlayerState = Player->GetPlayerState<APlayerState>())
		{
			if (PlayerState->IsOnlyASpectator())
			{
				if (!StarterPoints.IsEmpty())
				{
					return StarterPoints[FMath::RandRange(0, StarterPoints.Num() - 1)];
				}

				return nullptr;
			}
		}

		AActor* PlayerStart = K2_OnChoosePlayerStart(Player, StarterPoints);

		if (!PlayerStart)
		{
			PlayerStart = GetFirstRandomUnoccupiedPlayerStart(Player, StarterPoints);
		}

		if (ALeePlayerStart* LeeStart = Cast<ALeePlayerStart>(PlayerStart))
		{
			LeeStart->TryClaim(Player);
		}
		return PlayerStart;
	}

	return nullptr;
}

#if WITH_EDITOR
APlayerStart* ULeePlayerSpawningManagerComponent::FindPlayFromHereStart(AController* Player)
{
	if (Player->IsA<APlayerController>())
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<ALeePlayerStart> It(World); It; ++It)
			{
				if (APlayerStart* PlayerStart = *It)
				{
					if (PlayerStart->IsA<APlayerStartPIE>())
					{
						return PlayerStart;
					}
				}
			}
		}
	}
	return nullptr;
}
#endif


bool ULeePlayerSpawningManagerComponent::ControllerCanRestart(AController* Player)
{
	bool bCanRestart = true;

	return bCanRestart;
}

void ULeePlayerSpawningManagerComponent::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	OnFinishRestartPlayer(NewPlayer, StartRotation);
	K2_OnFinishRestartPlayer(NewPlayer, StartRotation);
}


void ULeePlayerSpawningManagerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}



APlayerStart* ULeePlayerSpawningManagerComponent::GetFirstRandomUnoccupiedPlayerStart(AController* Controller,
	const TArray<class ALeePlayerStart*>& StartPoints) const
{
	if (Controller)
	{
		TArray<ALeePlayerStart*> UnOccupiedStartPoints;
		TArray<ALeePlayerStart*> OccupiedStartPoints;

		for (ALeePlayerStart* StartPoint : StartPoints)
		{
			ELeePlayerStartLocationOccupancy State = StartPoint->GetLocationOccupancy(Controller);

			switch (State)
			{
			case ELeePlayerStartLocationOccupancy::Empty:
				UnOccupiedStartPoints.Add(StartPoint);
				break;
			case ELeePlayerStartLocationOccupancy::Partial:
				OccupiedStartPoints.Add(StartPoint);
				break;
			}
		}

		if (UnOccupiedStartPoints.Num() > 0)
		{
			return UnOccupiedStartPoints[FMath::RandRange(0, UnOccupiedStartPoints.Num() - 1)];
		}
		else if (OccupiedStartPoints.Num() > 0)
		{
			return OccupiedStartPoints[FMath::RandRange(0, OccupiedStartPoints.Num() - 1)];
		}
	}

	return nullptr;
}









