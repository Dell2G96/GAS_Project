// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeHeroComponent.h"

#include "LeePawnExtensionComponent.h"
#include "GAS_Project/LeeLogChannels.h"

#include "Components/GameFrameworkComponentManager.h"
#include "LeeGameplayTags.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACamera/LeeCameraComponent.h"
#include "GAS_Project/APlayer/LeePlayerState.h"



const FName ULeeHeroComponent::NAME_ActorFeatureName("Hero");

ULeeHeroComponent::ULeeHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
}

void ULeeHeroComponent::OnRegister()
{
	Super::OnRegister();

	{
		if (!GetPawn<APawn>())
		{
			UE_LOG(LogLee, Error, TEXT("ULeeHeroComponent::OnRegister() - Pawn이 존재하지 않습니다. Pawn이 존재하는 Actor에 컴포넌트를 추가해주세요. Actor : %s"), *GetOwner()->GetName());
		}
	}

	 RegisterInitStateFeature();
}

void ULeeHeroComponent::BeginPlay()
{
	Super::BeginPlay();

	// PawnExtensionComponent에 대해서 (PawnExtension Feature) OnActorInitStateChanged() 관찰하도록 (Observing)
	BindOnActorInitStateChanged(ULeePawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	// InitState_Spawned로 초기화
	ensure(TryToChangeInitState(MyTags::InitState::Spawned));
	
	// ForceUpdate 진행
	CheckDefaultInitialization();
}

void ULeeHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void ULeeHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	const FGameplayTag& InitTag = FGameplayTag();

	if (Params.FeatureName == ULeePawnExtensionComponent::NAME_ActorFeatureName)
	{
		// - CanChangeInitState 확인
		if (Params.FeatureState == MyTags::InitState::DataInitialized)
		{
			CheckDefaultInitialization();
		}
	}
	
}

bool ULeeHeroComponent::CanChangeInitState(class UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	const FGameplayTag& InitTags = FGameplayTag();
	APawn* Pawn = GetPawn<APawn>();
	ALeePlayerState* LeePS = GetPlayerState<ALeePlayerState>();

	if (!CurrentState.IsValid() && DesiredState == MyTags::InitState::Spawned)
	{
		if (Pawn)
		{
			return true;
		}
	}

	if (CurrentState == MyTags::InitState::Spawned && DesiredState == MyTags::InitState::DataAvailable)
	{
		if (!LeePS)
		{
			return false;
		}
		return true;
	}

	if (CurrentState == MyTags::InitState::DataAvailable && DesiredState == MyTags::InitState::DataInitialized)
	{
		// PawnExtensionComponent가 DataInitialized될 때까지 기다림 (== 모든 Feature Component가 DataAvailable인 상태)
		return LeePS && Manager->HasFeatureReachedInitState(Pawn, ULeePawnExtensionComponent::NAME_ActorFeatureName, MyTags::InitState::DataInitialized);
	}

	// DataInitialized -> GameplayReady
	if (CurrentState == MyTags::InitState::DataInitialized && DesiredState == MyTags::InitState::GameplayReady)
	{
		return true;
	}

	return false;
	
}

void ULeeHeroComponent::HandleChangeInitState(class UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState)
{

	const FGameplayTag& InitTags = FGameplayTag();

	if (CurrentState == MyTags::InitState::DataAvailable && DesiredState == MyTags::InitState::DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		ALeePlayerState* LeePS = GetPlayerState<ALeePlayerState>();
		if (!ensure(Pawn && LeePS))
		{
			return;
		}

		// ToDo :  Input과 Camera에 대한 핸들링... 

		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const ULeePawnData* PawnData = nullptr;
		if (ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnData = PawnExtComp->GetPawnData<ULeePawnData>();
		}

		if (bIsLocallyControlled && PawnData)
		{
			if (ULeeCameraComponent* CameraComp = ULeeCameraComponent::FindCameraComponent(Pawn))
			{
				CameraComp->DetermineCameraModeDelegate.BindUObject(this, &ThisClass::DetermineCameraMode);
			}
		}
		
	}

}

void ULeeHeroComponent::CheckDefaultInitialization()
{

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

TSubclassOf<class ULeeCameraMode> ULeeHeroComponent::DetermineCameraMode() const
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return nullptr;
	}

	if (ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const ULeePawnData* PawnData = PawnExtComp->GetPawnData<ULeePawnData>())
		{
			return PawnData->DefaultCameraMode;
		}
	}
	return nullptr;
}


