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
#include "GAS_Project/ABossEncounter/LeeBossEncounterManager.h"

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
	if (ALeeBossEncounterManager* Encounter = EncounterManager.Get())
	{
		Encounter->OnPhaseChanged.Remove(EncounterPhaseHandle);
	}
	EncounterPhaseHandle.Reset();
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

	// 26.11.12 추가 - 보스전 관리자가 있는 맵에서는 전투 단계가 될 때까지 BT를 시작하지 않는다
	if (WaitForEncounterCombat())
	{
		return;
	}

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

// 26.11.12 추가 - 관리자가 없거나 이미 전투 중이면 false(바로 시작). 아니면 단계 변경을 한 번만 구독하고 true
bool ULeeBossBrainComponent::WaitForEncounterCombat()
{
	ALeeBossEncounterManager* Encounter = ALeeBossEncounterManager::Find(this);
	if (!Encounter || Encounter->GetPhase() == ELeeBossEncounterPhase::Combat)
	{
		return false;
	}

	if (!EncounterPhaseHandle.IsValid())
	{
		EncounterManager = Encounter;
		EncounterPhaseHandle = Encounter->OnPhaseChanged.AddUObject(this, &ThisClass::HandleEncounterPhaseChanged);
	}
	return true;
}

// 26.11.12 추가 - 전투 단계가 되면 BT 시작
void ULeeBossBrainComponent::HandleEncounterPhaseChanged(ELeeBossEncounterPhase NewPhase)
{
	if (NewPhase == ELeeBossEncounterPhase::Combat)
	{
		StartBossBrain();
	}
}
