// 26.10.30 사용 안함 - Enemy 생성은 ALeeEnemySpawnPoint + ULeeEnemySpawnSubsystem으로 교체됨. 아래 코드는 전부 주석 처리 (레거시 보관용)

// // Fill out your copyright notice in the Description page of Project Settings.


// #include "LeeBotCreationComponent.h"

// #include "AIController.h"
// #include "LeeExperienceManagerComponent.h"
// #include "LeeGameModeBase.h"
// #include "GameFramework/PlayerState.h"
// #include "GAS_Project/LeeLogChannels.h"
// #include "GAS_Project/ACharacter/LeeHealthComponent.h"
// #include "GAS_Project/ACharacter/LeePawnExtensionComponent.h"
// #include "Kismet/GameplayStatics.h"

// ULeeBotCreationComponent::ULeeBotCreationComponent(const FObjectInitializer& ObjectInitializer)
// 	:Super(ObjectInitializer)
// {
// }

// void ULeeBotCreationComponent::BeginPlay()
// {
// 	Super::BeginPlay();

// 	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
// 	ULeeExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
// 	check(ExperienceComponent);
// 	ExperienceComponent->CallOrRegister_OnExperienceLoaded_LowPriority(FOnLeeExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
// }

// void ULeeBotCreationComponent::OnExperienceLoaded(const class ULeeExperienceDefinition* Experience)
// {
	
// #if WITH_SERVER_CODE
// 	if (HasAuthority())
// 	{
// 		ServerCreateBots();
// 	}
// #endif
	
// }

// #if WITH_SERVER_CODE
// //
// // void ULeeBotCreationComponent::ServerCreateBots_Implementation()
// // {
// // 	if (BotControllerClass == nullptr)
// // 	{
// // 		return;
// // 	}
// //
// // 	RemainingBotNames = RandomBotNames;
// //
// // 	// Determine how many bots to spawn
// // 	int32 EffectiveBotCount = NumBotsToCreate;
// //
// // 	// Give the developer settings a chance to override it
// // 	if (GIsEditor)
// // 	{
// // 		const ULeeDeveloperSettings* DeveloperSettings = GetDefault<ULeeDeveloperSettings>();
// // 		
// // 		if (DeveloperSettings->bOverrideBotCount)
// // 		{
// // 			EffectiveBotCount = DeveloperSettings->OverrideNumPlayerBotsToSpawn;
// // 		}
// // 	}
// //
// // 	// Give the URL a chance to override it
// // 	if (AGameModeBase* GameModeBase = GetGameMode<AGameModeBase>())
// // 	{
// // 		EffectiveBotCount = UGameplayStatics::GetIntOption(GameModeBase->OptionsString, TEXT("NumBots"), EffectiveBotCount);
// // 	}
// //
// // 	// Create them
// // 	for (int32 Count = 0; Count < EffectiveBotCount; ++Count)
// // 	{
// // 		SpawnOneBot();
// // 	}
// // }

// FString ULeeBotCreationComponent::CreateBotName(int32 PlayerIndex)
// {
// 	FString Result;
// 	if (RemainingBotNames.Num() > 0)
// 	{
// 		const int32 NameIndex = FMath::RandRange(0, RemainingBotNames.Num() - 1);
// 		Result = RemainingBotNames[NameIndex];
// 		RemainingBotNames.RemoveAtSwap(NameIndex);
// 	}
// 	else
// 	{
// 		//@TODO: PlayerId is only being initialized for players right now
// 		PlayerIndex = FMath::RandRange(260, 260+100);
// 		Result = FString::Printf(TEXT("Tinplate %d"), PlayerIndex);
// 	}
// 	return Result;
// }

// void ULeeBotCreationComponent::SpawnOneBot()
// {
// 	FActorSpawnParameters SpawnInfo;
// 	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
// 	SpawnInfo.OverrideLevel = GetComponentLevel();
// 	SpawnInfo.ObjectFlags |= RF_Transient;
// 	AAIController* NewController = GetWorld()->SpawnActor<AAIController>(BotControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);

// 	if (NewController != nullptr)
// 	{
// 		ALeeGameModeBase* GameMode = GetGameMode<ALeeGameModeBase>();
// 		check(GameMode);

// 		if (NewController->PlayerState != nullptr)
// 		{
// 			NewController->PlayerState->SetPlayerName(CreateBotName(NewController->PlayerState->GetPlayerId()));
// 		}
// 		GameMode->GenericPlayerInitialization(NewController);
// 		GameMode->RestartPlayer(NewController);

// 		if (NewController->GetPawn() != nullptr)
// 		{
// 			if (ULeePawnExtensionComponent* PawnExtensionComponent = NewController->GetPawn()->FindComponentByClass<ULeePawnExtensionComponent>())
// 			{
// 				PawnExtensionComponent->CheckDefaultInitialization();
// 			}
// 		}

// 		SpawnedBotList.Add(NewController);
// 	}
// }


// void ULeeBotCreationComponent::RemoveOneBot()
// {
// 	if (SpawnedBotList.Num() > 0)
// 	{
// 		const int32 BotToRemoveIndex = FMath::RandRange(0, SpawnedBotList.Num() -1);

// 		AAIController* BotToRmove = SpawnedBotList[BotToRemoveIndex];
// 		SpawnedBotList.RemoveAtSwap(BotToRemoveIndex);

// 		if (BotToRmove)
// 		{
// 			if (APawn* ControlledPawn = BotToRmove->GetPawn())
// 			{
// 				if (ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(ControlledPawn))
// 				{
// 					HealthComponent->DamageSelfDestruct();
// 				}
// 				else
// 				{
// 					ControlledPawn->Destroy();
// 				}
// 			}

// 			BotToRmove->Destroy();
// 		}
// 	}
// }

// void ULeeBotCreationComponent::ServerCreateBots()
// {
// #pragma region 26.09.23 수정 - 정상 동작 로그가 Error 레벨이라 진짜 에러에 섞이던 문제
// 	// 봇 생성 시작을 알리는 디버그 로그다. 에러가 아닌데 Error 레벨이라
// 	// 크래시 리포트와 로그 필터에서 실제 에러와 구분되지 않았다. Verbose로 낮춘다.
// 	UE_LOG(LogLee, Verbose, TEXT("[BotCreation] 봇 생성 시작 (요청 수: %d)"), NumBotsToCreate);
// #pragma endregion

// 	if (BotControllerClass == nullptr)
// 	{
// 		return;
// 	}
	
// 	RemainingBotNames = RandomBotNames;
// 	int32 EffectiveBotCount = NumBotsToCreate;
// 	for (int32 Count = 0; Count < EffectiveBotCount; ++Count)
// 	{
// 		SpawnOneBot();
// 	}
	
// }


// #else // !WITH_SERVER_CODE
// void ULeeBotCreationComponent::ServerCreateBots_Implementation()
// {
// 	ensureMsgf(0, TEXT("Bot functions do not exist in LeeClient!"));
// }

// void ULeeBotCreationComponent::SpawnOneBot()
// {
// 	ensureMsgf(0, TEXT("Bot functions do not exist in LeeClient!"));
// }

// void ULeeBotCreationComponent::RemoveOneBot()
// {
// 	ensureMsgf(0, TEXT("Bot functions do not exist in LeeClient!"));
// }

// #endif




