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

namespace
{
	float GetCapsuleRadius(const AActor* Actor)
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
			- GetCapsuleRadius(Pawn) - GetCapsuleRadius(Target));

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

	ULeeBossBrainComponent* Brain = AIC->FindComponentByClass<ULeeBossBrainComponent>();
	if (Brain)
	{
		Brain->UpdateGuardThreat(Target);
	}
	const float Threshold = Brain ? Brain->Phase2HealthThreshold : Phase2HealthThreshold;
	if (Phase < 2 && !bDead && HealthRatio <= Threshold)
	{
		// Pending을 먼저 켠다 — 더 높은 우선순위의 PhaseTransition 브랜치(포효)가 2페이즈 전투보다 먼저 선택되게
		BB->SetValueAsBool(PhaseTransitionPendingKey, true);
		BB->SetValueAsInt(PhaseKey, 2);
	}
}

FString UBTService_LeeBossContext::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nPhase2 at HP <= %.0f%%"), *Super::GetStaticDescription(), Phase2HealthThreshold * 100.f);
}
