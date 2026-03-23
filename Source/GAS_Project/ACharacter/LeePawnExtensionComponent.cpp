// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePawnExtensionComponent.h"

#include "Components/GameFrameworkComponentManager.h"
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
	check(!ExistingAvater);

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
		if (!bAllReady)
		{
			UE_LOG(LogLee, Warning, TEXT("PawnExtComponent DataAvailable->DataInitialized Waiting... Not all features reached DataAvailable."));
		}
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
