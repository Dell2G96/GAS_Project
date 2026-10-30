// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameplayAbility_Rest.h"

#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "LeeSavePointActor.h"
#include "LeeSavePointSubsystem.h"
#include "GameplayEffect.h"
#include "GameFramework/Pawn.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AAI/LeeTargetSelectionComponent.h"
#include "GAS_Project/APlayer/LeePlayerState.h"
#include "GAS_Project/ASaveGame/LeeSaveSubsystem.h"

// 서버 전용 + 상호작용 활성화 이벤트(Ability.Interaction.Activate)로 트리거
ULeeGameplayAbility_Rest::ULeeGameplayAbility_Rest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = MyTags::Lyra::Ability_Interaction_Activate;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

// 기본 조건 + Enemy에게 타겟으로 잡혀 있지 않을 때만 휴식 가능 (false면 F 프롬프트도 숨겨진다)
bool ULeeGameplayAbility_Rest::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	return !IsTargetedByEnemy(Avatar);
}

// 회복 GE 적용 → 상태 GE 제거 → 마지막 휴식 세이브 포인트 기록 → Enemy 리셋 후 바로 종료
void ULeeGameplayAbility_Rest::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 상호작용 이벤트: Instigator = 플레이어 Pawn, Target = 세이브 포인트
	const APawn* PlayerPawn = TriggerEventData ? Cast<APawn>(TriggerEventData->Instigator.Get()) : nullptr;
	const ALeeSavePointActor* SavePoint = TriggerEventData ? Cast<ALeeSavePointActor>(TriggerEventData->Target.Get()) : nullptr;
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();

	if (!PlayerPawn || !SavePoint || !AbilitySystem || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. HP/스태미나 회복 (GE로 적용해야 PostGameplayEffectExecute를 거쳐 HUD가 갱신된다)
	if (RestEffect)
	{
		ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, RestEffect.GetDefaultObject(), GetAbilityLevel());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_Rest] RestEffect가 설정되지 않음. BP에서 지정해주세요."));
	}

	// 2. 그로기 등 남아 있는 상태 GE 제거
	if (!EffectTagsToRemoveOnRest.IsEmpty())
	{
		AbilitySystem->RemoveActiveEffectsWithGrantedTags(EffectTagsToRemoveOnRest);
	}

	// 3. 다음 부활 위치로 쓸 세이브 포인트 기록 (PlayerState는 부활해도 유지된다)
	if (ALeePlayerState* LeePlayerState = PlayerPawn->GetPlayerState<ALeePlayerState>())
	{
		LeePlayerState->SetLastRestedSavePointId(SavePoint->GetSavePointId());
	}

	// 26.10.30 추가 - 로컬 플레이어면 휴식 세이브 포인트를 디스크에 저장 (자기 PC의 자기 진행만 저장)
	if (PlayerPawn->IsLocallyControlled())
	{
		if (ULeeSaveSubsystem* SaveSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<ULeeSaveSubsystem>())
		{
			SaveSubsystem->RecordRest(GetWorld(), SavePoint->GetSavePointId());
		}
	}

	// 4. 모든 Enemy를 처음 자리로 리셋
	if (ULeeSavePointSubsystem* SavePointSubsystem = GetWorld()->GetSubsystem<ULeeSavePointSubsystem>())
	{
		SavePointSubsystem->ResetEnemies();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// 모든 Controller의 타겟 선정 컴포넌트를 확인한다 (Enemy가 죽으면 컴포넌트가 타겟을 해제하므로 사망한 Enemy는 자동 제외)
bool ULeeGameplayAbility_Rest::IsTargetedByEnemy(const AActor* PlayerPawn)
{
	if (!PlayerPawn)
	{
		return false;
	}

	for (TActorIterator<AController> It(PlayerPawn->GetWorld()); It; ++It)
	{
		const ULeeTargetSelectionComponent* TargetSelection = ULeeTargetSelectionComponent::FindTargetSelectionComponent(*It);
		if (TargetSelection && TargetSelection->GetCurrentTarget() == PlayerPawn)
		{
			return true;
		}
	}
	return false;
}
