// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePawnExtensionComponent.h"

#include "Components/GameFrameworkComponentManager.h"
// 26.09.23 추가 - 막힌 Feature 이름 출력을 위해 InitState 인터페이스가 필요하다
#include "Components/GameFrameworkInitStateInterface.h"
#include "Net/UnrealNetwork.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"

const FName ULeePawnExtensionComponent::NAME_ActorFeatureName("PawnExtension");

ULeePawnExtensionComponent::ULeePawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void ULeePawnExtensionComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULeePawnExtensionComponent, PawnData);
}


void ULeePawnExtensionComponent::OnRegister()
{
	Super::OnRegister();

	// 올바른 Actor에 등록되었는지 확인:
	{
		if (!GetPawn<APawn>())
		{
			UE_LOG(LogLee, Error, TEXT("this component has been added to a BP whose base class is not a Pawn!"));
			return;
		}
	}

	RegisterInitStateFeature();

	// 디버깅을 위한 함수
	UGameFrameworkComponentManager* Manager = UGameFrameworkComponentManager::GetForActor(GetOwningActor());
}

void ULeePawnExtensionComponent::OnRep_PawnData()
{
	// 클라이언트에서 PawnData를 수신하면 InitState 체인 재개
	CheckDefaultInitialization();
}

void ULeePawnExtensionComponent::SetPawnData(const class ULeePawnData* InPawnData)
{
	APawn* Pawn = GetPawnChecked<APawn>();
	if (Pawn->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		return;
	}
	
	PawnData = InPawnData;
}

void ULeePawnExtensionComponent::SetUpPlayerInputComponent()
{
	// InitState 상태 변환 시작
	CheckDefaultInitialization();
}


void ULeePawnExtensionComponent::BeginPlay()
{
	Super::BeginPlay();

	// FeatureName에 NAME_None을 넣으면, Actor에 등록된 Feature Component의 InitState 상태를 관찰하겠다는 의미:
	BindOnActorInitStateChanged(NAME_None, FGameplayTag(), false);

	// InitState_Spawned로 상태 변환:
	// - TryToChangeInitState는 아래와 같이 진행된다:
	//   1. CanChangeInitState로 상태 변환 가능성 유무 판단
	//   2. HandleChangeInitState로 내부 상태 변경 (Feature Component)
	//   3. BindOnActorInitStateChanged로 Bind된 Delegate를 조건에 맞게 호출해 줌
	//      - HakPawnExtensionComponent의 경우, 모든 Actor의 Feature 상태 변화에 대해 OnActorInitStateChanged()가 호출됨
	ensure(TryToChangeInitState(MyTags::InitState::Spawned));

	// - 현재 강제 업데이트 진행 (물론 CanChangeInitState와 HandleChangleInitState를 진행해준다)
	CheckDefaultInitialization();

	
}

void ULeePawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	
	Super::EndPlay(EndPlayReason);
}

void ULeePawnExtensionComponent::InitializeAbilitySystem(class ULeeAbilitySystemComponent* InASC, AActor* InOwnerActor)
{
	check(InASC && InOwnerActor);

	if (AbilitySystemComponent == InASC)
	{
		return;
	}

	if (AbilitySystemComponent)
	{
		UnInitializeAbilitySystem();
	}

	APawn* Pawn =GetPawnChecked<APawn>();
	AActor* ExistingAvater = InASC->GetAvatarActor();

	/*
	26.10.29 수정 - 리스폰 시 ASC(PlayerState 소유)에 이전 Pawn이 Avatar로 남아 있으면 check에서 멈추는 문제 발생
	 이전 Pawn이 아직 Avatar라면 그 Pawn의 ASC 연결을 먼저 해제 (Lyra 방식)
	*/
	if ((ExistingAvater != nullptr) && (ExistingAvater != Pawn))
	{
		if (ULeePawnExtensionComponent* OtherExtensionComponent = FindPawnExtensionComponent(ExistingAvater))
		{
			OtherExtensionComponent->UnInitializeAbilitySystem();
		}
	}

	AbilitySystemComponent = InASC;
	AbilitySystemComponent->InitAbilityActorInfo(InOwnerActor, Pawn);

	OnAbilitySystemInitialized.Broadcast();
}

void ULeePawnExtensionComponent::UnInitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		return;
	}
	if (AbilitySystemComponent->GetAvatarActor() == GetOwner())
	{
		// 26.10.29 수정 - ASC는 PlayerState에 남아 다음 Pawn이 재사용하므로, 이 Pawn과의 연결을 실제로 끊어야 한다
		// 이전에는 포인터만 비우고 ASC의 AvatarActor를 그대로 두어, 리스폰한 Pawn이 InitializeAbilitySystem에서 멈췄다
		FGameplayTagContainer AbilityTypesToIgnore;
		AbilityTypesToIgnore.AddTag(MyTags::Souls::Ability_Behavior_SurvivesDeath);

		AbilitySystemComponent->CancelAbilities(nullptr, &AbilityTypesToIgnore);
		AbilitySystemComponent->RemoveAllGameplayCues();

		if (AbilitySystemComponent->GetOwnerActor() != nullptr)
		{
			AbilitySystemComponent->SetAvatarActor(nullptr);
		}
		else
		{
			// Owner가 없으면 Avatar뿐 아니라 ActorInfo 전체를 비운다
			AbilitySystemComponent->ClearActorInfo();
		}

		OnAbilitySystemUnInitialized.Broadcast();
	}
	AbilitySystemComponent = nullptr;
}

void ULeePawnExtensionComponent::HandleControllerChanged()
{
	if (AbilitySystemComponent && (AbilitySystemComponent->GetAvatarActor() == GetPawnChecked<APawn>()))
	{
		ensure(AbilitySystemComponent->AbilityActorInfo->OwnerActor == AbilitySystemComponent->GetOwnerActor());
		if (AbilitySystemComponent->GetOwnerActor() == nullptr)
		{
			UnInitializeAbilitySystem();
		}
		else
		{
			AbilitySystemComponent->RefreshAbilityActorInfo();
		}
	}

	CheckDefaultInitialization();
}

void ULeePawnExtensionComponent::HandlePlayerStateReplicated()
{
	CheckDefaultInitialization();
}

void ULeePawnExtensionComponent::SetupPlayerInputComponent()
{
	CheckDefaultInitialization();
}

void ULeePawnExtensionComponent::OnAbilitySystemInitialized_RegistedAndCall(
	FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnAbilitySystemInitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemInitialized.Add(Delegate);
	}
	
	if (AbilitySystemComponent)
	{
		Delegate.Execute();
	}
}

void ULeePawnExtensionComponent::OnAbilitySystemUnInitialized_Registed(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnAbilitySystemUnInitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemUnInitialized.Add(Delegate);
	}
}

void ULeePawnExtensionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName != NAME_ActorFeatureName)
	{
		// HakPawnExtensionComponent는 다른 Feature Component들의 상태가 DataAvailable를 관찰하여, Sync를 맞추는 구간이 있었다 (CanChangeInitState)
		// - 이를 가능케하기 위해, OnActorInitStateChanged에서는 DataAvailable에 대해 지속적으로 CheckDefaultInitialization을 호출하여, 상태를 확인한다
		const FGameplayTag& InitTags = FGameplayTag();
		if (Params.FeatureState == MyTags::InitState::DataAvailable)
		{
			CheckDefaultInitialization();
		}
	}
}

bool ULeePawnExtensionComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();
	const FGameplayTag& InitTags =FGameplayTag();

	// InitState_Spawned 초기화
	if (!CurrentState.IsValid() && DesiredState == MyTags::InitState::Spawned)
	{
		// Pawn이 잘 세팅만 되어있으면 바로 Spawned로 넘어감!
		if (Pawn)
		{
			return true;
		}
	}

	// Spawned -> DataAvailable
	if (CurrentState == MyTags::InitState::Spawned && DesiredState == MyTags::InitState::DataAvailable)
	{
		// 아마 PawnData를 누군가 설정하는 모양이다
		if (!PawnData)
		{
			UE_LOG(LogLee, Warning, TEXT("PawnExtComponent Spawned->DataAvailable Failed: PawnData is null"));
			return false;
		}

		// LocallyControlled인 Pawn이 Controller가 없으면 에러!
		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		if (bIsLocallyControlled)
		{
			if (!GetController<AController>())
			{
				UE_LOG(LogLee, Warning, TEXT("PawnExtComponent Spawned->DataAvailable Failed: Locally controlled but no Controller"));
				return false;
			}
		}

		return true;
	}

	// DataAvailable -> DataInitialized
	if (CurrentState == MyTags::InitState::DataAvailable && DesiredState == MyTags::InitState::DataInitialized)
	{
		// Actor에 바인드된 모든 Feature들이 DataAvailable 상태일 때, DataInitialized로 넘어감:
		// - HaveAllFeaturesReachedInitState 확인
		bool bAllReady = Manager->HaveAllFeaturesReachedInitState(Pawn, MyTags::InitState::DataAvailable);
#pragma region 26.09.23 수정 - 어떤 Feature가 막혔는지 이름으로 출력
		// 기존 로그는 "모두 준비되지 않았다"고만 알려 원인 추적이 불가능했다.
		// 엔진의 HaveAllFeaturesReachedInitState()가 bool만 반환하므로,
		// 폰의 Feature 컴포넌트를 직접 훑어 아직 도달하지 못한 것의 이름을 찍는다.
		//
		// 레벨을 Warning에서 Verbose로 낮춘 이유: 이 대기는 스폰 직후 몇 프레임 동안
		// 반드시 발생하는 정상 동작이다. 항상 Warning으로 찍으면 진짜 문제를 가린다.
		// 필요할 때만 콘솔에서 `Log LogLee Verbose`로 켜서 본다.
		if (!bAllReady)
		{
			UE_LOG(LogLee, Verbose, TEXT("[PawnExt] DataAvailable->DataInitialized 대기 중 — 아직 준비되지 않은 Feature:"));

			TArray<UActorComponent*> PawnComponents;
			Pawn->GetComponents(PawnComponents);
			for (UActorComponent* PawnComponent : PawnComponents)
			{
				const IGameFrameworkInitStateInterface* Feature = Cast<IGameFrameworkInitStateInterface>(PawnComponent);
				if (!Feature)
				{
					continue;
				}

				const FName FeatureName = Feature->GetFeatureName();
				const FGameplayTag FeatureState = Manager->GetInitStateForFeature(Pawn, FeatureName);
				if (FeatureState != MyTags::InitState::DataAvailable
					&& FeatureState != MyTags::InitState::DataInitialized
					&& FeatureState != MyTags::InitState::GameplayReady)
				{
					UE_LOG(LogLee, Verbose, TEXT("    %s (%s) = %s"),
						*FeatureName.ToString(),
						*PawnComponent->GetClass()->GetName(),
						FeatureState.IsValid() ? *FeatureState.ToString() : TEXT("(상태 없음)"));
				}
			}
		}
#pragma endregion
		return bAllReady;
	}

	// DataInitialized -> GameplayReady
	if (CurrentState == MyTags::InitState::DataInitialized && DesiredState == MyTags::InitState::GameplayReady)
	{
		return true;
	}

	// 위의 선형적인(linear) transition이 아니면 false!
	return false;
}

void ULeePawnExtensionComponent::CheckDefaultInitialization()
{
	CheckDefaultInitializationForImplementers();

	const FGameplayTag& InitTags = FGameplayTag();

	static const TArray<FGameplayTag> StateChain =
	{
		MyTags::InitState::Spawned,
		MyTags::InitState::DataAvailable,
		MyTags::InitState::DataInitialized,
		MyTags::InitState::GameplayReady
	};

	ContinueInitStateChain(StateChain);

}
