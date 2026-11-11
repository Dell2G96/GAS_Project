// 26.11.05 신규 - Stone 보스 AI (Behavior Tree + StateTree 혼용)

#include "BTService_LeeBossContext.h"
#include "LeeBossBrainComponent.h"

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"
#include "GAS_Project/AAI/LeeTargetSelectionComponent.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"

namespace
{
	// ACharacter면 스케일 적용된 캡슐 반경, 아니면 0 (Unity 빌드 이름 충돌을 피하려고 고유 이름 사용)
	float GetBossContextCapsuleRadius(const AActor* Actor)
	{
		const ACharacter* Character = Cast<ACharacter>(Actor);
		return (Character && Character->GetCapsuleComponent()) ? Character->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.f;
	}
}

UBTService_LeeBossContext::UBTService_LeeBossContext()
{
	NodeName = TEXT("Lee Boss Context");
	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	Interval = 0.1f;
	RandomDeviation = 0.f;
	// 브랜치 진입 시점에 바로 한 번 갱신해야 데코레이터가 초기값(빈 타겟/페이즈 0)으로 판단하지 않는다
	bCallTickOnSearchStart = true;

	TargetActorKey = LeeBossBBKeys::TargetActor;
	DistanceToTargetKey = LeeBossBBKeys::DistanceToTarget;
	SelfHealthRatioKey = LeeBossBBKeys::SelfHealthRatio;
	PhaseKey = LeeBossBBKeys::Phase;
	GroggyKey = LeeBossBBKeys::bIsGroggy;
	DeadKey = LeeBossBBKeys::bIsDead;
	TargetHealingKey = LeeBossBBKeys::bTargetHealing;
	PhaseTransitionPendingKey = LeeBossBBKeys::bPhaseTransitionPending;
}

void UBTService_LeeBossContext::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB)
	{
		return;
	}

	APawn* Pawn = AIC->GetPawn();

	// --- 자신: 체력 비율 / 그로기 / 사망 ---
	float HealthRatio = 1.f;
	bool bGroggy = false;
	bool bDead = false;
	if (UAbilitySystemComponent* SelfASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		const float MaxHealth = SelfASC->GetNumericAttribute(ULeeSoulsStatSet::GetMaxHealthAttribute());
		const float Health = SelfASC->GetNumericAttribute(ULeeSoulsStatSet::GetHealthAttribute());
		HealthRatio = MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 1.f;

		bGroggy = SelfASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy);
		bDead = SelfASC->HasMatchingGameplayTag(MyTags::Souls::Status_Death_Dying)
			|| SelfASC->HasMatchingGameplayTag(MyTags::Souls::Status_Death_Dead);
	}

	// --- 타겟: ULeeTargetSelectionComponent가 유일한 권위 (컨트롤러 → 폰 순서로 찾음, Evaluator와 동일) ---
	const ULeeTargetSelectionComponent* SelectionComp = ULeeTargetSelectionComponent::FindTargetSelectionComponent(AIC);
	if (!SelectionComp && Pawn)
	{
		SelectionComp = ULeeTargetSelectionComponent::FindTargetSelectionComponent(Pawn);
	}
	AActor* Target = (SelectionComp && !bDead) ? SelectionComp->GetCurrentTarget() : nullptr;

	float Distance = 0.f;
	bool bTargetHealing = false;
	if (Target && Pawn)
	{
		// 캡슐 반경을 뺀 표면-표면 수평 거리 (StateTree Evaluator의 DistanceToTarget과 같은 기준)
		Distance = FMath::Max(0.f, FVector::Dist2D(Pawn->GetActorLocation(), Target->GetActorLocation())
			- GetBossContextCapsuleRadius(Pawn) - GetBossContextCapsuleRadius(Target));

		if (const UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
		{
			bTargetHealing = TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Estus_Drinking);
		}
	}

	BB->SetValueAsObject(TargetActorKey, Target);
	BB->SetValueAsFloat(DistanceToTargetKey, Distance);
	BB->SetValueAsFloat(SelfHealthRatioKey, HealthRatio);
	BB->SetValueAsBool(GroggyKey, bGroggy);
	BB->SetValueAsBool(DeadKey, bDead);
	BB->SetValueAsBool(TargetHealingKey, bTargetHealing);

	// --- 페이즈: 한 번 올라가면 내려가지 않는다 ---
	int32 Phase = BB->GetValueAsInt(PhaseKey);
	if (Phase < 1)
	{
		Phase = 1;
		BB->SetValueAsInt(PhaseKey, Phase);
	}

	// 26.11.08 수정 - 반응 가드 감지는 StateTree 평가기(Lee Combat Context)로 옮겼다
	ULeeBossBrainComponent* Brain = AIC->FindComponentByClass<ULeeBossBrainComponent>();
	const float Threshold = Brain ? Brain->Phase2HealthThreshold : Phase2HealthThreshold;
	if (Phase < 2)
	{
		// 26.11.11 수정 - 즉시 전환하지 않고 "예약 → 하던 행동이 끝나면 전환"으로 바꿨다
		UpdatePhaseTransition(*BB, UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn), NodeMemory,
			HealthRatio, Threshold, bGroggy, bDead);
	}
}

// 26.11.11 추가 - 포효 예약/대기/시작 판정 (Phase < 2일 때만 호출)
void UBTService_LeeBossContext::UpdatePhaseTransition(UBlackboardComponent& BB, UAbilitySystemComponent* SelfASC, uint8* NodeMemory,
	float HealthRatio, float Threshold, bool bGroggy, bool bDead) const
{
	FBTLeeBossContextMemory* Memory = CastInstanceNodeMemory<FBTLeeBossContextMemory>(NodeMemory);
	if (!SelfASC || bDead)
	{
		return;
	}

	const bool bQueued = SelfASC->HasMatchingGameplayTag(MyTags::Souls::Status_Boss_PhaseChangeQueued);
	const double Now = SelfASC->GetWorld() ? SelfASC->GetWorld()->GetTimeSeconds() : 0.0;

	// 1) HP 기준 도달 → 예약 시작 (예약 태그가 붙어 있는 동안 새 공격을 시작하지 못한다)
	if (!bQueued)
	{
		if (HealthRatio <= Threshold)
		{
			SelfASC->AddLooseGameplayTag(MyTags::Souls::Status_Boss_PhaseChangeQueued);
			Memory->QueuedStartTime = Now;
		}
		return;
	}

	// 서비스 메모리가 초기화된 경우(BT 재시작 등) 지금부터 다시 센다
	if (Memory->QueuedStartTime < 0.0)
	{
		Memory->QueuedStartTime = Now;
	}

	// 2) 그로기·처형당하는 중에는 시간과 상관없이 끝날 때까지 기다린다
	//    (처형당하는 어빌리티는 Exclusive_Blocking이라 이때 포효를 시작하면 활성화에 실패해 포효가 생략된다)
	if (bGroggy || SelfASC->HasMatchingGameplayTag(MyTags::Souls::Status_Finisher_Victim))
	{
		return;
	}

	// 3) 한가해졌거나(공격·피격 리액션 없음) 최대 대기 시간을 넘으면 → 전환 시작
	const ULeeAbilitySystemComponent* LeeASC = Cast<ULeeAbilitySystemComponent>(SelfASC);
	const bool bBusy = LeeASC && LeeASC->HasActiveExclusiveAbility();
	const bool bTimedOut = (Now - Memory->QueuedStartTime) >= MaxPhaseChangeWaitSeconds;
	if (bBusy && !bTimedOut)
	{
		return;
	}

	SelfASC->RemoveLooseGameplayTag(MyTags::Souls::Status_Boss_PhaseChangeQueued);
	Memory->QueuedStartTime = -1.0;

	// Pending을 먼저 켠다 — 더 높은 우선순위의 PhaseTransition 브랜치(포효)가 2페이즈 전투보다 먼저 선택되게
	// (시간 초과로 들어온 경우 하던 공격은 기존처럼 끊기고 포효한다)
	BB.SetValueAsBool(PhaseTransitionPendingKey, true);
	BB.SetValueAsInt(PhaseKey, 2);
}

// 노드 메모리 크기 — AI마다 포효 예약 시각을 따로 보관
uint16 UBTService_LeeBossContext::GetInstanceMemorySize() const
{
	return sizeof(FBTLeeBossContextMemory);
}

// 노드 메모리 초기화 — 예약 없음 상태로 시작
void UBTService_LeeBossContext::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<FBTLeeBossContextMemory>(NodeMemory, InitType);
}

FString UBTService_LeeBossContext::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nPhase2 at HP <= %.0f%%"), *Super::GetStaticDescription(), Phase2HealthThreshold * 100.f);
}
