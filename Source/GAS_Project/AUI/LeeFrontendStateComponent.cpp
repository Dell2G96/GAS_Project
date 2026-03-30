// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeFrontendStateComponent.h"

#include "CommonGameInstance.h"
#include "CommonSessionSubsystem.h"
#include "CommonUserSubsystem.h"
#include "ControlFlowManager.h"
#include "Kismet/GameplayStatics.h"
#include "NativeGameplayTags.h"
#include "PrimaryGameLayout.h"
#include "GAS_Project/GameModes/LeeExperienceManagerComponent.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace FrontendTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_PLATFORM_TRAIT_SINGLEONLINEUSER, "Platform.Trait.SingleOnlineUser");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_MENU, "UI.Layer.Menu");
}


ULeeFrontendStateComponent::ULeeFrontendStateComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer) 
{
}

void ULeeFrontendStateComponent::BeginPlay()
{
	Super::BeginPlay();

	// Experience 로드가 완료 될 떄까지 대기
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	ULeeExperienceManagerComponent* ExperienceManagerComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
	check(ExperienceManagerComponent);

	ExperienceManagerComponent->CallOrRegister_OnExperienceLoaded_HighPriority(FOnLeeExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
	
}

void ULeeFrontendStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

bool ULeeFrontendStateComponent::ShouldShowLoadingScreen(FString& OutReason) const
{
	if (bShouldShowLoadingScreen)
	{
		OutReason = TEXT("Frontend Flow Pending...");
       
		if (FrontEndFlow.IsValid())
		{
			const TOptional<FString> StepDebugName = FrontEndFlow->GetCurrentStepDebugName();
			if (StepDebugName.IsSet())
			{
				OutReason = StepDebugName.GetValue();
			}
		}
       
		return true;
	}

	return false;
}

void ULeeFrontendStateComponent::OnExperienceLoaded(const class ULeeExperienceDefinition* Experience)
{
	UE_LOG(LogTemp, Error, TEXT("*** FrontendState: Experience LOADED! ***"));
	
	FControlFlow& Flow = FControlFlowStatics::Create(this, TEXT("FrontendFlow"))
	  .QueueStep(TEXT("Wait For User Initialization"), this, &ThisClass::FlowStep_WaitForUserInitialization)
	  .QueueStep(TEXT("Try Show Press Start Screen"), this, &ThisClass::FlowStep_TryShowPressStartScreen)
	  .QueueStep(TEXT("Try Join Requested Session"), this, &ThisClass::FlowStep_TryJoinRequestedSession)
	  .QueueStep(TEXT("Try Show Main Screen"), this, &ThisClass::FlowStep_TryShowMainScreen);

	Flow.ExecuteFlow();

	FrontEndFlow = Flow.AsShared();
}

void ULeeFrontendStateComponent::FlowStep_WaitForUserInitialization(FControlFlowNodeRef SubFlow)
{
	UE_LOG(LogTemp, Warning, TEXT("*** FlowStepWaitForUserInitialization START ***"));

	
	// 강제 연결 종료였다면, 모든 사용자 및 세션 상태를 명시적으로 초기화
	bool bWasHardDisconnect = false;
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AGameModeBase>();
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);

	// 디버깅 추가
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("No GameInstance!"));
		SubFlow->ContinueFlow();
		return;
	}

	UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();
	if (!UserSubsystem) {
		UE_LOG(LogTemp, Error, TEXT("No UserSubsystem!"));
		SubFlow->ContinueFlow();  // 강제 진행
		return;
	}
	// 디버깅 추가

	if (ensure(GameMode) && UGameplayStatics::HasOption(GameMode->OptionsString, TEXT("closed")))
	{
		bWasHardDisconnect = true;
	}
	// 강제 연결 종료일 때만 사용자 상태 초기화
	/*UCommonUserSubsystem* */UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();
	if (ensure(UserSubsystem) && bWasHardDisconnect)
	{
		UserSubsystem->ResetUserState();
	}

	// 세션은 항상 초기화
	UCommonSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UCommonSessionSubsystem>();
	if (ensure(SessionSubsystem))
	{
		SessionSubsystem->CleanUpSessions();
	}

	// Reset 시도 (실패해도 무시)
	UserSubsystem->ResetUserState();
    
	UE_LOG(LogTemp, Warning, TEXT("*** FlowStepWaitForUserInitialization END ***"));

	SubFlow->ContinueFlow();
}

void ULeeFrontendStateComponent::FlowStep_TryShowPressStartScreen(FControlFlowNodeRef SubFlow)
{
	 const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
    UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();

    // 첫 번째 플레이어가 이미 로그인되어 있는지 확인합니다.  
    // 로그인되어 있다면 Press Start 화면을 건너뛸 수 있습니다.
    if (const UCommonUserInfo* FirstUser = UserSubsystem->GetUserInfoForLocalPlayerIndex(0))
    {
       if (FirstUser->InitializationState == ECommonUserInitializationState::LoggedInLocalOnly ||
          FirstUser->InitializationState == ECommonUserInitializationState::LoggedInOnline)
       {
          SubFlow->ContinueFlow();
          return;
       }
    }

    // 플랫폼이 실제로 'Press Start' 화면을 필요로 하는지 확인합니다.  
    // 이 화면은 여러 온라인 사용자가 있을 수 있고, 어떤 플레이어의 
    // 컨트롤러가 'Start'를 누르느냐에 따라 게임에 로그인할 플레이어가 결정되는 플랫폼에서만 필요합니다.
    if (!UserSubsystem->ShouldWaitForStartInput())
    {
       // 자동 로그인 과정을 시작합니다. 이 과정은 빠르게 끝나며 기본 입력 장치 ID를 사용합니다
       InProgressPressStartScreen = SubFlow;
       UserSubsystem->OnUserInitializeComplete.AddDynamic(this, &ULeeFrontendStateComponent::OnUserInitialized);
       UserSubsystem->TryToInitializeForLocalPlay(0, FInputDeviceId(), false);

       return;
    }

    // Press Start 화면을 추가하고, 비활성화되면 다음 흐름으로 이동합니다.
    if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(this))
    {
       constexpr bool bSuspendInputUntilComplete = true;
       RootLayout->PushWidgetToLayerStackAsync<UCommonActivatableWidget>(FrontendTags::TAG_UI_LAYER_MENU, bSuspendInputUntilComplete, PressStartScreenClass,
          [this, SubFlow](EAsyncWidgetLayerState State, UCommonActivatableWidget* Screen) {
          switch (State)
          {
          case EAsyncWidgetLayerState::AfterPush:
             bShouldShowLoadingScreen = false;
             Screen->OnDeactivated().AddWeakLambda(this, [this, SubFlow]() {
                SubFlow->ContinueFlow();
             });
             break;
          case EAsyncWidgetLayerState::Canceled:
             bShouldShowLoadingScreen = false;
             SubFlow->ContinueFlow();
             return;
          }
       });
    }
}



void ULeeFrontendStateComponent::OnUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error,
	ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
	FControlFlowNodePtr FlowToContinue = InProgressPressStartScreen;
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();

	if (ensure(FlowToContinue.IsValid() && UserSubsystem))
	{
		UserSubsystem->OnUserInitializeComplete.RemoveDynamic(this, &ThisClass::OnUserInitialized);
		InProgressPressStartScreen.Reset();

		if (bSuccess)
		{
			// 성공하면 정상적으로 진행
			FlowToContinue->ContinueFlow();
		}
		else
		{
			FlowToContinue->ContinueFlow();
		}
		
	}
}

void ULeeFrontendStateComponent::FlowStep_TryJoinRequestedSession(FControlFlowNodeRef SubFlow)
{

	UCommonGameInstance* GameInstance = Cast<UCommonGameInstance>(UGameplayStatics::GetGameInstance(this));
	 if (GameInstance->GetRequestedSession() != nullptr && GameInstance->CanJoinRequestedSession())
  	{
  		UCommonSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UCommonSessionSubsystem>();
	    if (ensure(SessionSubsystem))
	    {
		    // 세션 참가 완료 시 계속 진행하거나 취소하기 위해 델리게이트를 바인딩한다
	    	// 세션 참가가 완료된 후 서버트래블도 완료되었는지 확인한다
	    	OnJoinSessionCompleteEventHandle = SessionSubsystem->OnJoinSessionCompleteEvent.AddWeakLambda(this, [this, SubFlow, SessionSubsystem](const FOnlineResultInformation& Result)
	    	{
	    		// 델리게이트를 해제한다 SessionSubsystem는 이 이벤트를 발생시키는 객체이므로 아직 유효해야 한다

	    		SessionSubsystem->OnJoinSessionCompleteEvent.Remove(OnJoinSessionCompleteEventHandle);
	    		OnJoinSessionCompleteEventHandle.Reset();

			    if (Result.bWasSuccessful)
			    {
				    // 더 이상 메인 메뉴로 전환 중이 아님
			    	SubFlow->CancelFlow();
			    }
			    else
			    {
				    // 메인 메뉴로 진행한다
			    	SubFlow->ContinueFlow();
			    	return;
			    }
	    	});
	    	GameInstance->JoinRequestedSession();
	    	return;
	    }
	}
	// 세션 참가 요청을 시작하지 않았다면 이 단계를 건너 뜀
	SubFlow->ContinueFlow();
}

void ULeeFrontendStateComponent::FlowStep_TryShowMainScreen(FControlFlowNodeRef SubFlow)
{
	if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(this))
	{
		constexpr bool bSuspendInputUntilComplete = true;
		RootLayout->PushWidgetToLayerStackAsync<UCommonActivatableWidget>(FrontendTags::TAG_UI_LAYER_MENU, bSuspendInputUntilComplete, MainScreenClass,
		   [this, SubFlow](EAsyncWidgetLayerState State, UCommonActivatableWidget* Screen) {
		   switch (State)
		   {
		   case EAsyncWidgetLayerState::AfterPush:
		   	bShouldShowLoadingScreen = false;
			   SubFlow->ContinueFlow();
			   return;
			case EAsyncWidgetLayerState::Canceled:
				bShouldShowLoadingScreen = false;
				SubFlow->ContinueFlow();
				return;
			 }
		  });
	}
}
