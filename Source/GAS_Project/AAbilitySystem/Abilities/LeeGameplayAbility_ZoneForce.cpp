// 26.11.12 신규 - AOE 장판 안에서 받는 밀기/당기기 힘 (피해자 쪽 어빌리티)

#include "LeeGameplayAbility_ZoneForce.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionRadialForce.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS_Project/MyTags.h"

// 생성자 — ZoneForce 이벤트 트리거 등록, 다른 어빌리티와 독립 실행, 새 장판이면 다시 시작
ULeeGameplayAbility_ZoneForce::ULeeGameplayAbility_ZoneForce(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup    = ELeeAbilityActivationGroup::Independent;
	bRetriggerInstancedAbility = true;

	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Death_Dying);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = MyTags::Souls::Event_Combat_ZoneForce;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

// [서버] 중심·반경·유지 시간은 TargetData(LocationInfo)에, 세기는 EventMagnitude에 실어 피해자에게 보낸다
void ULeeGameplayAbility_ZoneForce::SendZoneForceEvent(AActor* InstigatorActor, AActor* VictimActor, const FVector& Center, float Radius, float Strength, float Duration)
{
	if (!VictimActor || FMath::IsNearlyZero(Strength) || Duration <= 0.0f || Radius <= 0.0f)
	{
		return;
	}

	const UAbilitySystemComponent* VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(VictimActor);
	if (!VictimASC
		|| VictimASC->HasMatchingGameplayTag(MyTags::Souls::Status_Death_Dying)
		|| VictimASC->HasMatchingGameplayTag(MyTags::Souls::Status_SuperArmor))
	{
		return;
	}

	// 위치 = 장판 중심, 스케일 X = 반경, 스케일 Y = 유지 시간 (값 4개를 이벤트 하나에 싣기 위한 약속)
	FGameplayAbilityTargetData_LocationInfo* ZoneData = new FGameplayAbilityTargetData_LocationInfo();
	ZoneData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	ZoneData->TargetLocation.LiteralTransform = FTransform(FQuat::Identity, Center, FVector(Radius, Duration, 1.0f));

	FGameplayEventData Payload;
	Payload.EventTag = MyTags::Souls::Event_Combat_ZoneForce;
	Payload.Instigator = InstigatorActor;
	Payload.Target = VictimActor;
	Payload.EventMagnitude = Strength;
	Payload.TargetData.Add(ZoneData);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(VictimActor, MyTags::Souls::Event_Combat_ZoneForce, Payload);
}

// 이벤트 값으로 장판 중심 기준 원형 힘을 건다 (반경 밖이면 힘이 0, 수평 방향만)
void ULeeGameplayAbility_ZoneForce::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const FGameplayAbilityTargetData_LocationInfo* ZoneData = TriggerEventData
		? static_cast<const FGameplayAbilityTargetData_LocationInfo*>(TriggerEventData->TargetData.Get(0))
		: nullptr;
	if (!ZoneData || FMath::IsNearlyZero(TriggerEventData->EventMagnitude))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FTransform& ZoneTransform = ZoneData->TargetLocation.LiteralTransform;
	const FVector Center = ZoneTransform.GetLocation();
	const float Radius = ZoneTransform.GetScale3D().X;
	const float Duration = ZoneTransform.GetScale3D().Y;
	const float Strength = TriggerEventData->EventMagnitude;

	UAbilityTask_ApplyRootMotionRadialForce* ForceTask = UAbilityTask_ApplyRootMotionRadialForce::ApplyRootMotionRadialForce(
		this, NAME_None, Center, /*LocationActor*/nullptr,
		/*Strength*/FMath::Abs(Strength), Duration, Radius,
		/*bIsPush*/Strength > 0.0f, /*bIsAdditive*/true, /*bNoZForce*/true,
		/*StrengthDistanceFalloff*/nullptr, /*StrengthOverTime*/nullptr,
		/*bUseFixedWorldDirection*/false, /*FixedWorldDirection*/FRotator::ZeroRotator,
		/*VelocityOnFinishMode*/ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity,
		/*SetVelocityOnFinish*/FVector::ZeroVector, /*ClampVelocityOnFinish*/0.0f);
	ForceTask->OnFinish.AddDynamic(this, &ThisClass::OnZoneForceFinished);
	ForceTask->ReadyForActivation();
}

// 장판 힘 유지 시간 끝
void ULeeGameplayAbility_ZoneForce::OnZoneForceFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
