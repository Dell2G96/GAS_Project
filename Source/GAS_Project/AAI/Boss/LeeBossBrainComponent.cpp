// 26.11.05 신규 - Stone 보스 AI (Behavior Tree + StateTree 혼용)

#include "LeeBossBrainComponent.h"

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

#include "BehaviorTree/BehaviorTree.h"
#include "Components/StateTreeComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/AAbilitySystem/Abilities/LeeGameplayAbility_AttackMelee.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"

ULeeBossBrainComponent::ULeeBossBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// 서버에서만 Possess 변경을 구독. 이미 Pawn이 있으면 다음 틱에 시작한다(BP BeginPlay/OnPossess 처리 이후)
void ULeeBossBrainComponent::BeginPlay()
{
	Super::BeginPlay();

	AAIController* AIC = Cast<AAIController>(GetOwner());
	if (!AIC || !AIC->HasAuthority())
	{
		return;
	}

	AIC->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::HandlePossessedPawnChanged);

	if (AIC->GetPawn())
	{
		StartTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::StartBossBrain);
	}
}

void ULeeBossBrainComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AAIController* AIC = Cast<AAIController>(GetOwner()))
	{
		AIC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}

	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(StartTimer);
	if (AAIController* AIC = Cast<AAIController>(GetOwner()))
	{
		UBehaviorTreeComponent* BT = Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent());
		if (bStarted && BT && BT->GetRootTree() == BossBehaviorTree) BT->StopTree(EBTStopMode::Forced);
	}
	bStarted = false;
	Super::EndPlay(EndPlayReason);
}

// OnPossessedPawnChanged는 BP의 On Possess(ReceivePossess) 이후에 브로드캐스트된다 → BP가 켠 StateTree를 여기서 끈다
void ULeeBossBrainComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	bStarted = false;
	AttackStaminaRecoveryTarget = 0.f;
	UpdateGuardThreat(nullptr);
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(StartTimer);
	if (!NewPawn)
	{
		return;
	}

	// BP Possess에서 시작하는 기존 로직이 끝난 뒤 한 번만 교체한다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StartTimer);
		StartTimer = World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			StartBossBrain();
		}));
	}
}

// 보스 Pawn의 컨트롤러에서 Experience가 주입한 컴포넌트를 조회한다.
ULeeBossBrainComponent* ULeeBossBrainComponent::FindForPawn(const AActor* Actor)
{
	const APawn* Pawn = Cast<APawn>(Actor);
	const AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	return Controller ? Controller->FindComponentByClass<ULeeBossBrainComponent>() : nullptr;
}

// 보스가 이번에 소비한 공격 비용을 회복 기준으로 저장한다.
void ULeeBossBrainComponent::BeginAttackStaminaRecovery(float SpentStamina)
{
	AttackStaminaRecoveryTarget = SpentStamina;
}

// 새 공격이 가능해질 때까지 기존 StateTree의 대기를 연장한다.
bool ULeeBossBrainComponent::IsRecoveringAttackStamina()
{
	const AAIController* Controller = Cast<AAIController>(GetOwner());
	const UAbilitySystemComponent* ASC = Controller ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Controller->GetPawn()) : nullptr;
	if (!ASC || AttackStaminaRecoveryTarget <= 0.f)
	{
		return false;
	}

	const float MaxStamina = ASC->GetNumericAttribute(ULeeSoulsStatSet::GetMaxStaminaAttribute());
	const float Stamina = ASC->GetNumericAttribute(ULeeSoulsStatSet::GetStaminaAttribute());
	if (Stamina < FMath::Min(AttackStaminaRecoveryTarget, MaxStamina))
	{
		return true;
	}

	AttackStaminaRecoveryTarget = 0.f;
	return false;
}

// 현재 플레이어의 근접 공격을 식별하고, 새 공격마다 확률을 한 번만 판정한다.
void ULeeBossBrainComponent::UpdateGuardThreat(AActor* Target)
{
	FGameplayAbilitySpecHandle ActiveAttack;
	const APawn* TargetPawn = Cast<APawn>(Target);
	if (TargetPawn && TargetPawn->IsPlayerControlled())
	{
		if (const auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
		{
			for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			{
				if (Spec.IsActive() && Spec.Ability && Spec.Ability->IsA<ULeeGameplayAbility_AttackMelee>())
				{
					ActiveAttack = Spec.Handle;
					break;
				}
			}
		}
	}

	if (GuardThreatTarget.Get() != Target || GuardThreatAttack != ActiveAttack)
	{
		GuardThreatTarget = Target;
		GuardThreatAttack = ActiveAttack;
		bGuardOpportunity = ActiveAttack.IsValid() && FMath::FRand() < ReactiveGuardChance;
	}
}

// 같은 근접 공격에 방패를 반복해서 들지 않도록 남은 기회를 확인한다.
bool ULeeBossBrainComponent::CanReactToGuardThreat(const AActor* Target) const
{
	return bGuardOpportunity && IsGuardThreatActive(Target);
}

// 대상이 바뀌거나 감지한 근접 공격이 끝났으면 가드 유지도 끝낸다.
bool ULeeBossBrainComponent::IsGuardThreatActive(const AActor* Target) const
{
	return Target && GuardThreatTarget.Get() == Target && GuardThreatAttack.IsValid();
}

// 이번 공격의 가드 기회를 사용한다.
void ULeeBossBrainComponent::ConsumeGuardOpportunity()
{
	bGuardOpportunity = false;
}

void ULeeBossBrainComponent::StartBossBrain()
{
	AAIController* AIC = Cast<AAIController>(GetOwner());
	if (!AIC || !AIC->HasAuthority() || !AIC->GetPawn() || bStarted)
	{
		return;
	}

	if (!BossBehaviorTree)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeBossBrain] %s: BossBehaviorTree가 지정되지 않았습니다."), *GetNameSafe(AIC));
		return;
	}
	StopStateTreeLogic();

	bStarted = AIC->RunBehaviorTree(BossBehaviorTree);
	if (!bStarted)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeBossBrain] %s: RunBehaviorTree 실패 (%s)"), *GetNameSafe(AIC), *GetNameSafe(BossBehaviorTree));
	}
}

void ULeeBossBrainComponent::StopStateTreeLogic() const
{
	if (!GetOwner())
	{
		return;
	}

	TArray<UStateTreeComponent*> StateTreeComps;
	GetOwner()->GetComponents(StateTreeComps);
	for (UStateTreeComponent* Comp : StateTreeComps)
	{
		if (Comp->IsRunning())
		{
			Comp->StopLogic(TEXT("LeeBossBrain - Behavior Tree로 전환"));
		}
	}
}
