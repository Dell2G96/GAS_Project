// Fill out your copyright notice in the Description page of Project Settings.
// 26.09.22 신규 작성 — StateTree 에셋 자동 저작 구현

#include "LeeStateTreeAuthoringLibrary.h"

#if WITH_EDITOR

#include "StateTree.h"
#include "StateTreeEditorData.h"
#include "StateTreeState.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorModule.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "Conditions/StateTreeCommonConditions.h"
#include "Tasks/StateTreeMoveToTask.h"

#include "GAS_Project/AAI/StateTree/LeeStateTreeConditions_Combat.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AAI/StateTree/LeeStateTreeConsiderations_Combat.h"
#include "GAS_Project/AAI/StateTree/LeeStateTreeEvaluator_CombatContext.h"
#include "GAS_Project/AAI/StateTree/LeeStateTreeTask_ClaimAttackToken.h"
#include "GAS_Project/AAI/StateTree/LeeStateTreeTask_FaceTarget.h"
#include "GAS_Project/AAI/StateTree/LeeStateTreeTask_MaintainGuard.h"
#include "GAS_Project/AAI/StateTree/LeeStateTreeTask_MoveToEQSLocation.h"
#include "GAS_Project/AAI/StateTree/LeeStateTreeTask_SetMovementGait.h"
#include "GAS_Project/AAI/StateTree/LeeStateTreeTask_StrafeAroundTarget.h"
#include "GAS_Project/AAI/LeeStateTreeTask_MeleeAttack.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace LeeAuthoring
{
	/**
	 * Consideration을 State에 추가한다.
	 * 엔진이 AddTask<T>/AddEnterCondition<T>는 제공하는데 Consideration용 헬퍼만 없어서
	 * AddTask의 구현을 그대로 본떠 만들었다.
	 * ID 발급과 Instance 초기화를 빼먹으면 컴파일은 통과하지만 런타임에 바인딩이 깨진다.
	 */
	template<typename T, typename... TArgs>
	TStateTreeEditorNode<T>& AddConsideration(UStateTreeState& State, TArgs&&... InArgs)
	{
		FStateTreeEditorNode& Item = State.Considerations.AddDefaulted_GetRef();
		Item.ID = FGuid::NewGuid();
		Item.Node.InitializeAs<T>(Forward<TArgs>(InArgs)...);
		const FStateTreeNodeBase& Node = Item.Node.GetMutable<FStateTreeNodeBase>();
		if (const UScriptStruct* InstanceType = Cast<const UScriptStruct>(Node.GetInstanceDataType()))
		{
			Item.Instance.InitializeAs(InstanceType);
		}
		return static_cast<TStateTreeEditorNode<T>&>(Item);
	}

	/** Evaluator가 내보낸 bool 출력(거리 티어 플래그 등)을 State 진입 조건으로 거는 헬퍼 */
	inline void AddBoolTierCondition(UStateTreeEditorData& EditorData, UStateTreeState& State,
	                                 const FStateTreeEditorNode& EvalNode, const FString& SourceProperty)
	{
		TStateTreeEditorNode<FStateTreeCompareBoolCondition>& Cond = State.AddEnterCondition<FStateTreeCompareBoolCondition>();
		Cond.GetInstanceData().bRight = true;
		EditorData.AddPropertyBinding(EvalNode, SourceProperty, Cond, TEXT("bLeft"));
	}
}

/**
 * PackagePath 안에서 아직 쓰이지 않은 _vNN 이름을 찾는다.
 * 덮어쓰기 대신 새 버전을 만드는 정책이라, 손으로 튜닝한 이전 버전이 보존된다.
 */
FString ULeeStateTreeAuthoringLibrary::MakeVersionedAssetName(const FString& PackagePath, const FString& BaseAssetName)
{
	for (int32 Version = 1; Version <= 99; ++Version)
	{
		const FString Candidate = FString::Printf(TEXT("%s_v%02d"), *BaseAssetName, Version);
		const FString PackageName = PackagePath / Candidate;
		if (!FPackageName::DoesPackageExist(PackageName) && !FindPackage(nullptr, *PackageName))
		{
			return Candidate;
		}
	}
	return FString();
}

/**
 * Enemy 전투 StateTree를 통째로 생성한다.
 * 구조: Root → 거리 티어(Close/Mid/Far) → 각 티어의 행동 State.
 * Close 아래의 MeleeAttack만 Utility 선택(Consideration 기반)을 쓰고 나머지는 순서 선택을 쓴다.
 */
UStateTree* ULeeStateTreeAuthoringLibrary::BuildEnemyCombatStateTree(const FString& PackagePath, const FString& BaseAssetName, const FLeeEnemyTreeBuildConfig& Config)
{
	const FString AssetName = MakeVersionedAssetName(PackagePath, BaseAssetName);
	if (AssetName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[LeeAuthoring] 사용 가능한 버전 이름을 찾지 못했습니다: %s"), *BaseAssetName);
		return nullptr;
	}

	const FString PackageName = PackagePath / AssetName;
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("[LeeAuthoring] 패키지 생성 실패: %s"), *PackageName);
		return nullptr;
	}

	UStateTree* StateTree = NewObject<UStateTree>(Package, UStateTree::StaticClass(), *AssetName,
		RF_Public | RF_Standalone | RF_Transactional);

	// --- EditorData + Schema 구성 (UStateTreeFactory::FactoryCreateNew와 동일한 절차) ---
	const TNonNullSubclassOf<UStateTreeEditorData> EditorDataClass =
		FStateTreeEditorModule::GetModule().GetEditorDataClass(UStateTreeAIComponentSchema::StaticClass());
	UStateTreeEditorData* EditorData = NewObject<UStateTreeEditorData>(StateTree, EditorDataClass, NAME_None, RF_Transactional);
	EditorData->Schema = NewObject<UStateTreeSchema>(EditorData, UStateTreeAIComponentSchema::StaticClass());
	StateTree->EditorData = EditorData;

	// --- 전투 컨텍스트 Evaluator: 거리/각도/티어/토큰 컴포넌트를 매 프레임 산출해 트리 전체에 공급 ---
	TStateTreeEditorNode<FLeeStateTreeEvaluator_CombatContext>& Eval =
		EditorData->AddEvaluator<FLeeStateTreeEvaluator_CombatContext>();

	UStateTreeState& Root = EditorData->AddRootState();
	Root.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder;

	// 거리 티어를 State로 나누는 이유: Consideration 하나로 뭉치면 "가까운데 원거리를 쏘는" 조합을
	// 점수로만 막아야 해서 튜닝이 어렵다. 티어는 진입 조건으로 잘라내는 편이 예측 가능하다.
	UStateTreeState& CloseState = Root.AddChildState(TEXT("Close"));
	CloseState.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder;
	LeeAuthoring::AddBoolTierCondition(*EditorData, CloseState, Eval, TEXT("bIsCloseTier"));

	UStateTreeState& MidState = Root.AddChildState(TEXT("Mid"));
	MidState.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder;
	LeeAuthoring::AddBoolTierCondition(*EditorData, MidState, Eval, TEXT("bIsMidTier"));

	UStateTreeState& FarState = Root.AddChildState(TEXT("Far"));
	FarState.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder;
	LeeAuthoring::AddBoolTierCondition(*EditorData, FarState, Eval, TEXT("bIsFarTier"));

	// ---------------------------------------------------------------------
	// Close: 근접 공격 → (토큰 부족 시) 가드 스트래핑 → 일반 스트래핑
	// ---------------------------------------------------------------------
	// 근접 태그가 둘 다 비어 있으면 MeleeAttack 부모 자체를 만들지 않는다.
	// 자식 없는 부모가 남으면 Close 진입 때마다 선택 실패만 반복한다(26.10.20).
	const bool bHasAnyMeleeTag = Config.LightAttackAbilityTag.IsValid() || Config.HeavyAttackAbilityTag.IsValid();
	if (!bHasAnyMeleeTag)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeAuthoring] 근접 어빌리티 태그(Light/Heavy)가 모두 비어 있어 MeleeAttack State를 만들지 않습니다: %s"), *AssetName);
	}

	if (bHasAnyMeleeTag)
	{
		UStateTreeState& MeleeParent = CloseState.AddChildState(TEXT("MeleeAttack"));
		// 약/강 선택은 Consideration 점수 기반. 항상 최고점만 고르면 패턴이 읽히므로
		// 점수 가중 랜덤을 써서 같은 상황에서도 행동이 갈리게 한다.
		MeleeParent.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenAtRandomWeightedByUtility;

		// 부모에서 "근접 공격 중 하나라도 토큰을 잡을 수 있는가"를 먼저 걸러
		// 자식 두 개를 헛되이 평가하지 않게 한다.
		TStateTreeEditorNode<FLeeStateTreeCondition_CanClaimAnyMeleeAttack>& AnyMeleeCond =
			MeleeParent.AddEnterCondition<FLeeStateTreeCondition_CanClaimAnyMeleeAttack>();
		EditorData->AddPropertyBinding(Eval, TEXT("AttackTokenComponent"), AnyMeleeCond, TEXT("TokenComponent"));
		EditorData->AddPropertyBinding(Eval, TEXT("AttackerProfileComponent"), AnyMeleeCond, TEXT("ProfileComponent"));

		// 약공격 / 강공격 — 구조가 같아 람다로 묶었다
		auto BuildMeleeChild = [&](const TCHAR* StateName, const TCHAR* ProfileProperty,
		                           const FGameplayTag& AbilityTag, bool bIsHeavy) -> void
		{
			// 태그가 빈 State는 Activate Ability가 항상 실패하므로 만들지 않는다(26.10.20 Morgana HeavyAttack).
			if (!AbilityTag.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("[LeeAuthoring] %s 어빌리티 태그가 비어 있어 State를 만들지 않습니다: %s"), StateName, *AssetName);
				return;
			}

			UStateTreeState& AttackState = MeleeParent.AddChildState(StateName);

			const FString ReservationSource = FString::Printf(TEXT("AttackerProfileComponent.%s"), ProfileProperty);

			// 진입 조건: 이 공격 종류의 토큰을 지금 잡을 수 있는가 (점유는 하지 않는다)
			TStateTreeEditorNode<FLeeStateTreeCondition_CanClaimAttackToken>& ClaimCond =
				AttackState.AddEnterCondition<FLeeStateTreeCondition_CanClaimAttackToken>();
			EditorData->AddPropertyBinding(Eval, TEXT("AttackTokenComponent"), ClaimCond, TEXT("TokenComponent"));
			EditorData->AddPropertyBinding(Eval, ReservationSource, ClaimCond, TEXT("Reservation"));

			// Utility: 마지막 사용 후 오래 지났을수록 높은 점수 → 같은 공격 연타를 억제한다
			TStateTreeEditorNode<FLeeConsideration_TimeSinceLastUse>& TimeConsideration =
				LeeAuthoring::AddConsideration<FLeeConsideration_TimeSinceLastUse>(AttackState);
			// 사용 기록은 ClaimAttackToken이 Reservation.QuotaTag로 남기므로 같은 태그로 조회해야 한다.
			// 예전에는 어빌리티 태그를 넣어서 기록을 못 찾고 항상 1.0이 나왔다(26.10.20 발견).
			// 2단계 중첩 경로 바인딩은 조용히 실패하는 문제가 있어(26.10.19) 원거리처럼 상수로 넣는다.
			TimeConsideration.GetInstanceData().AttackTag = bIsHeavy
				? MyTags::Souls::Attacker_Melee_Heavy
				: MyTags::Souls::Attacker_Melee_Light;
			EditorData->AddPropertyBinding(Eval, TEXT("Actor"), TimeConsideration, TEXT("RequesterPawn"));

			// 강공격만: 타겟이 그로기 등 취약 상태일 때 가중치를 올린다
			if (bIsHeavy && Config.TargetVulnerableTag.IsValid())
			{
				TStateTreeEditorNode<FLeeConsideration_TargetHasTag>& VulnerableConsideration =
					LeeAuthoring::AddConsideration<FLeeConsideration_TargetHasTag>(AttackState);
				VulnerableConsideration.GetInstanceData().TargetTag = Config.TargetVulnerableTag;
				VulnerableConsideration.GetInstanceData().ScoreWhenPresent = 1.f;
				VulnerableConsideration.GetInstanceData().ScoreWhenAbsent = 0.25f;
				EditorData->AddPropertyBinding(Eval, TEXT("TargetActor"), VulnerableConsideration, TEXT("TargetActor"));
			}

			// 실제 점유는 여기서 한 번만. 조건(가부 확인)과 예약(점유)을 분리해 TOCTOU를 피한다.
			TStateTreeEditorNode<FLeeStateTreeTask_ClaimAttackToken>& ClaimTask =
				AttackState.AddTask<FLeeStateTreeTask_ClaimAttackToken>();
			EditorData->AddPropertyBinding(Eval, TEXT("AttackTokenComponent"), ClaimTask, TEXT("TokenComponent"));
			EditorData->AddPropertyBinding(Eval, ReservationSource, ClaimTask, TEXT("Reservation"));

			TStateTreeEditorNode<FLeeStateTreeTask_ActivateAbility>& AbilityTask =
				AttackState.AddTask<FLeeStateTreeTask_ActivateAbility>();
			AbilityTask.GetInstanceData().AbilityTag = AbilityTag;

			AttackState.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted,
				EStateTreeTransitionType::GotoState, &Root);
			// 쿨타임 등으로 활성화에 실패하면 즉시 Root로 돌아가 같은 틱에 다시 실패하는 반복이 생긴다.
			// 원거리 State와 같은 짧은 지연을 둬서 그 사이클을 늦춘다(26.10.20).
			FStateTreeTransition& FailedTransition = AttackState.AddTransition(EStateTreeTransitionTrigger::OnStateFailed,
				EStateTreeTransitionType::GotoState, &Root);
			FailedTransition.bDelayTransition = true;
			FailedTransition.DelayDuration = 0.5f;
			FailedTransition.DelayRandomVariance = 0.2f;
		};

		BuildMeleeChild(TEXT("LightAttack"), TEXT("LightAttack"), Config.LightAttackAbilityTag, false);
		BuildMeleeChild(TEXT("HeavyAttack"), TEXT("HeavyAttack"), Config.HeavyAttackAbilityTag, true);
	}

	// 근접 공격 유무와 상관없이 Close의 대기 행동은 항상 만든다.
	{
		// 토큰을 못 잡았을 때의 대기 행동. 가드 스트래핑을 먼저 두어
		// "붙어는 있는데 때리지 못하는" 구간이 무방비로 보이지 않게 한다.
		if (Config.bUseGuardStrafe)
		{
			UStateTreeState& GuardStrafe = CloseState.AddChildState(TEXT("GuardStrafe"));

			TStateTreeEditorNode<FLeeStateTreeTask_SetMovementGait>& GaitTask =
				GuardStrafe.AddTask<FLeeStateTreeTask_SetMovementGait>();
			GaitTask.GetInstanceData().Gait = ELeeMovementGait::Walk;

			TStateTreeEditorNode<FLeeStateTreeTask_MaintainGuard>& GuardTask =
				GuardStrafe.AddTask<FLeeStateTreeTask_MaintainGuard>();
			EditorData->AddPropertyBinding(Eval, TEXT("TargetActor"), GuardTask, TEXT("TargetActor"));

			TStateTreeEditorNode<FLeeStateTreeTask_StrafeAroundTarget>& StrafeTask =
				GuardStrafe.AddTask<FLeeStateTreeTask_StrafeAroundTarget>();
			EditorData->AddPropertyBinding(Eval, TEXT("TargetActor"), StrafeTask, TEXT("TargetActor"));
			// 원거리 아키타입이면 반경을 크게 줘서 "맴돌기"가 아니라 "물러나기"처럼 보이게 한다.
			if (Config.bCanRangedAttack)
			{
				StrafeTask.GetInstanceData().StrafeRadius = Config.RangedCloseRetreatRadius;
			}

			GuardStrafe.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted,
				EStateTreeTransitionType::GotoState, &Root);
		}

		UStateTreeState& PlainStrafe = CloseState.AddChildState(TEXT("Strafe"));
		{
			TStateTreeEditorNode<FLeeStateTreeTask_SetMovementGait>& GaitTask =
				PlainStrafe.AddTask<FLeeStateTreeTask_SetMovementGait>();
			GaitTask.GetInstanceData().Gait = ELeeMovementGait::Walk;

			TStateTreeEditorNode<FLeeStateTreeTask_StrafeAroundTarget>& StrafeTask =
				PlainStrafe.AddTask<FLeeStateTreeTask_StrafeAroundTarget>();
			EditorData->AddPropertyBinding(Eval, TEXT("TargetActor"), StrafeTask, TEXT("TargetActor"));
			if (Config.bCanRangedAttack)
			{
				StrafeTask.GetInstanceData().StrafeRadius = Config.RangedCloseRetreatRadius;
			}

			PlainStrafe.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted,
				EStateTreeTransitionType::GotoState, &Root);
		}
	}

	// ---------------------------------------------------------------------
	// Mid / Far: 원거리 아키타입이면 사격을 먼저 시도하고, 아니면 접근한다
	// ---------------------------------------------------------------------
	auto BuildRangedState = [&](UStateTreeState& Parent) -> void
	{
		UStateTreeState& RangedState = Parent.AddChildState(TEXT("RangedAttack"));

		TStateTreeEditorNode<FLeeStateTreeCondition_CanClaimAttackToken>& ClaimCond =
			RangedState.AddEnterCondition<FLeeStateTreeCondition_CanClaimAttackToken>();
		EditorData->AddPropertyBinding(Eval, TEXT("AttackTokenComponent"), ClaimCond, TEXT("TokenComponent"));
		EditorData->AddPropertyBinding(Eval, TEXT("AttackerProfileComponent.RangedAttack"), ClaimCond, TEXT("Reservation"));

		// 재발사 간격 제어(R7) — 마지막 사용 후 RangedMinFireInterval이 지나기 전에는 진입하지 않는다.
		// 간격이 없으면 토큰만 잡히면 매 틱 연속 발사된다(26.10.19 발견).
		TStateTreeEditorNode<FLeeStateTreeCondition_TimeSinceLastUse>& IntervalCond =
			RangedState.AddEnterCondition<FLeeStateTreeCondition_TimeSinceLastUse>();
		EditorData->AddPropertyBinding(Eval, TEXT("Actor"), IntervalCond, TEXT("RequesterPawn"));
		// 2단계 중첩 경로("AttackerProfileComponent.RangedAttack.QuotaTag") 바인딩이 조용히 실패해서
		// AttackTag가 항상 빈 태그로 남는 문제가 있었다(26.10.19 발견 — 간격 제어가 완전히 무효화됨).
		// 원거리 Quota 태그는 어차피 전역 고정값이라 직접 상수로 넣는다.
		IntervalCond.GetInstanceData().AttackTag = MyTags::Souls::Attacker_Ranged;
		IntervalCond.GetInstanceData().MinSeconds = Config.RangedMinFireInterval;

		// GA 쿨타임 중에는 진입하지 않는다(26.10.21). 이게 없으면 쿨타임이 RangedMinFireInterval보다 길 때
		// 진입 → GA 활성화 실패 → 0.5초 정지가 반복되고, 그동안 Reposition으로 넘어가지 못한다.
		if (Config.RangedCooldownTag.IsValid())
		{
			TStateTreeEditorNode<FLeeStateTreeCondition_SelfHasGameplayTag>& CooldownCond =
				RangedState.AddEnterCondition<FLeeStateTreeCondition_SelfHasGameplayTag>();
			CooldownCond.GetInstanceData().Tag = Config.RangedCooldownTag;
			CooldownCond.GetInstanceData().bInvert = true;
		}

		TStateTreeEditorNode<FLeeStateTreeTask_ClaimAttackToken>& ClaimTask =
			RangedState.AddTask<FLeeStateTreeTask_ClaimAttackToken>();
		EditorData->AddPropertyBinding(Eval, TEXT("AttackTokenComponent"), ClaimTask, TEXT("TokenComponent"));
		EditorData->AddPropertyBinding(Eval, TEXT("AttackerProfileComponent.RangedAttack"), ClaimTask, TEXT("Reservation"));

		// 사격 전 정면 정렬. 정렬이 끝나야 다음 Task가 의미 있는 방향으로 쏜다.
		TStateTreeEditorNode<FLeeStateTreeTask_FaceTarget>& FaceTask =
			RangedState.AddTask<FLeeStateTreeTask_FaceTarget>();
		EditorData->AddPropertyBinding(Eval, TEXT("TargetActor"), FaceTask, TEXT("TargetActor"));

		TStateTreeEditorNode<FLeeStateTreeTask_ActivateAbility>& AbilityTask =
			RangedState.AddTask<FLeeStateTreeTask_ActivateAbility>();
		AbilityTask.GetInstanceData().AbilityTag = Config.RangedAttackAbilityTag;

		RangedState.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted,
			EStateTreeTransitionType::GotoState, &Root);

		// 실패 즉시 Root로 돌아가면 같은 틱에 RangedAttack을 다시 골라 매 틱 재시도하게 된다.
		// FaceTarget이 매 틱 Focus를 설정했다가(진입) 바로 해제(이탈)하는 게 반복돼
		// 회전이 끊기는 것처럼 보인다(26.10.19 발견). 짧은 지연을 둬서 그 사이클을 늦춘다.
		FStateTreeTransition& FailedTransition = RangedState.AddTransition(
			EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, &Root);
		FailedTransition.bDelayTransition = true;
		FailedTransition.DelayDuration = 0.5f;
		FailedTransition.DelayRandomVariance = 0.2f;
	};

	// 원거리 공격을 못 쓰는 동안(쿨타임/토큰 없음) EQS가 고른 지점으로 걸어서 자리를 옮긴다(26.10.21).
	// 이게 없으면 Approach가 "이미 사거리 안"이라 즉시 성공 → Root → Approach…를 반복해 제자리에 서 있었다.
	auto BuildRepositionState = [&](UStateTreeState& Parent) -> void
	{
		UStateTreeState& Reposition = Parent.AddChildState(TEXT("Reposition"));

		TStateTreeEditorNode<FLeeStateTreeTask_SetMovementGait>& GaitTask =
			Reposition.AddTask<FLeeStateTreeTask_SetMovementGait>();
		GaitTask.GetInstanceData().Gait = ELeeMovementGait::Walk;

		TStateTreeEditorNode<FLeeStateTreeTask_MoveToEQSLocation>& MoveTask =
			Reposition.AddTask<FLeeStateTreeTask_MoveToEQSLocation>();
		MoveTask.GetInstanceData().QueryTemplate = Config.RangedRepositionQuery;
		EditorData->AddPropertyBinding(Eval, TEXT("TargetActor"), MoveTask, TEXT("TargetActor"));

		Reposition.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted,
			EStateTreeTransitionType::GotoState, &Root);
		// EQS 결과가 없으면 즉시 실패한다. 바로 Root로 가면 같은 틱에 다시 골라 매 틱 EQS를 돌리게 되므로 지연을 둔다.
		FStateTreeTransition& FailedTransition = Reposition.AddTransition(
			EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, &Root);
		FailedTransition.bDelayTransition = true;
		FailedTransition.DelayDuration = 0.5f;
		FailedTransition.DelayRandomVariance = 0.2f;
	};

	auto BuildApproachState = [&](UStateTreeState& Parent, ELeeMovementGait Gait, const TCHAR* AcceptanceRadiusSourceProperty) -> void
	{
		UStateTreeState& Approach = Parent.AddChildState(TEXT("Approach"));

		TStateTreeEditorNode<FLeeStateTreeTask_SetMovementGait>& GaitTask =
			Approach.AddTask<FLeeStateTreeTask_SetMovementGait>();
		GaitTask.GetInstanceData().Gait = Gait;

		TStateTreeEditorNode<FStateTreeMoveToTask>& MoveTask = Approach.AddTask<FStateTreeMoveToTask>();
		EditorData->AddPropertyBinding(Eval, TEXT("TargetActor"), MoveTask, TEXT("TargetActor"));
		// 원거리 아키타입이면 플레이어 바로 앞까지 붙지 않고, 자신의 공격범위 경계(가장 먼 지점)에서 멈춘다.
		// Evaluator("Lee Combat Context") 노드의 CloseExitDistance/FarExitDistance 값을 그대로 바인딩하므로,
		// StateTree 에디터에서 그 노드를 선택해 숫자만 바꾸면 코드 수정 없이 바로 반영된다(26.10.20).
		if (AcceptanceRadiusSourceProperty)
		{
			EditorData->AddPropertyBinding(Eval, AcceptanceRadiusSourceProperty, MoveTask, TEXT("AcceptableRadius"));
		}

		Approach.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted,
			EStateTreeTransitionType::GotoState, &Root);
		Approach.AddTransition(EStateTreeTransitionTrigger::OnStateFailed,
			EStateTreeTransitionType::GotoState, &Root);
	};

	if (Config.bCanRangedAttack)
	{
		BuildRangedState(MidState);
		BuildRangedState(FarState);

		// 순서 선택이므로 RangedAttack이 막히면 Reposition, EQS가 없으면 기존처럼 Approach로 넘어간다
		if (Config.RangedRepositionQuery)
		{
			BuildRepositionState(MidState);
			BuildRepositionState(FarState);
		}
	}
	// 중거리는 걷기로 간격을 좁히고, 원거리는 달려서 붙는다.
	// 원거리 아키타입의 접근은 공격범위 경계(플레이어에게는 가장 먼 지점)에서 멈춘다 —
	// 그렇지 않으면 MoveTo 기본 동작대로 거의 붙을 때까지 다가가버린다(26.10.20 발견).
	// Mid의 Approach는 "쿨다운 등으로 원거리 공격을 못 쓸 때"의 대체 행동인데, 원거리 아키타입이면
	// 여기도 거리 제한이 없으면 결국 근접 거리까지 계속 걸어가 버린다(26.10.20 발견).
	BuildApproachState(MidState, ELeeMovementGait::Walk, Config.bCanRangedAttack ? TEXT("CloseExitDistance") : nullptr);
	BuildApproachState(FarState, ELeeMovementGait::Run, Config.bCanRangedAttack ? TEXT("FarExitDistance") : nullptr);

	// ---------------------------------------------------------------------
	// Idle: 타겟이 없어 Close/Mid/Far 중 아무 티어도 진입 조건을 만족 못할 때의 fallback.
	// 진입 조건을 두지 않아 Root가 앞의 세 티어를 모두 거부했을 때만 선택된다(순서 선택이므로 항상 마지막).
	// 이게 없으면 Root가 선택할 자식을 못 찾아 트리 전체가 즉시 Failed로 끝난다(26.10.19 R8).
	// ---------------------------------------------------------------------
	{
		UStateTreeState& IdleState = Root.AddChildState(TEXT("Idle"));

		// SetMovementGait은 생성자에서 bConsideredForCompletion=false이므로 완료 판정 없이 계속 Running을 유지한다.
		// Root는 매 틱 Close/Mid/Far의 진입 조건을 다시 평가하므로, 타겟이 잡혀 티어가 유효해지면
		// 별도 이벤트 전이 없이도 자동으로 그쪽으로 재선택된다.
		TStateTreeEditorNode<FLeeStateTreeTask_SetMovementGait>& IdleGaitTask =
			IdleState.AddTask<FLeeStateTreeTask_SetMovementGait>();
		IdleGaitTask.GetInstanceData().Gait = ELeeMovementGait::Walk;

		// Idle의 Task는 절대 완료되지 않으므로(SetMovementGait), OnStateCompleted/Failed 전이로는
		// 영원히 빠져나올 수 없다. 타겟이 생겨 티어 중 하나라도 켜지면 매 틱 다시 확인해서
		// Root로 돌아가 재선택이 일어나게 한다(26.10.19 발견 — 세션 시작 시 타겟이 없어 Idle로
		// 들어간 뒤, 나중에 타겟이 생겨도 영원히 Idle에 머무르던 버그).
		auto AddTierExitTransition = [&](const TCHAR* TierProperty)
		{
			FStateTreeTransition& Transition = IdleState.AddTransition(
				EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &Root);
			TStateTreeEditorNode<FStateTreeCompareBoolCondition>& Cond =
				Transition.AddCondition<FStateTreeCompareBoolCondition>();
			Cond.GetInstanceData().bRight = true;
			EditorData->AddPropertyBinding(Eval, TierProperty, Cond, TEXT("bLeft"));
		};
		AddTierExitTransition(TEXT("bIsCloseTier"));
		AddTierExitTransition(TEXT("bIsMidTier"));
		AddTierExitTransition(TEXT("bIsFarTier"));
	}

	// --- 컴파일 ---
	// 컴파일을 빼먹으면 에셋은 만들어지지만 런타임 데이터가 비어 AI가 전혀 움직이지 않는다.
	FStateTreeCompilerLog Log;
	if (!UStateTreeEditingSubsystem::CompileStateTree(StateTree, Log))
	{
		UE_LOG(LogTemp, Error, TEXT("[LeeAuthoring] StateTree 컴파일 실패: %s"), *AssetName);
		return nullptr;
	}

	// --- 저장 ---
	FAssetRegistryModule::AssetCreated(StateTree);
	Package->MarkPackageDirty();

	const FString FileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	if (!UPackage::SavePackage(Package, StateTree, *FileName, SaveArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("[LeeAuthoring] 패키지 저장 실패: %s"), *FileName);
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("[LeeAuthoring] StateTree 생성 완료: %s"), *PackageName);
	return StateTree;
}

#else // WITH_EDITOR

FString ULeeStateTreeAuthoringLibrary::MakeVersionedAssetName(const FString&, const FString&)
{
	return FString();
}

UStateTree* ULeeStateTreeAuthoringLibrary::BuildEnemyCombatStateTree(const FString&, const FString&, const FLeeEnemyTreeBuildConfig&)
{
	return nullptr;
}

#endif // WITH_EDITOR

#pragma region 26.09.22 추가 - 파이썬 호출용 FName 버전
namespace
{
	/** FName을 태그로 변환한다. 비어 있으면 빈 태그를, 등록되지 않은 이름이면 경고 후 빈 태그를 반환한다 */
	bool ResolveTagName(FName TagName, FGameplayTag& OutTag, bool& bOutHadError)
	{
		OutTag = FGameplayTag();
		if (TagName.IsNone())
		{
			return true;   // 지정하지 않은 것은 정상
		}

		OutTag = FGameplayTag::RequestGameplayTag(TagName, /*ErrorIfNotFound*/ false);
		if (!OutTag.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("[LeeAuthoring] 등록되지 않은 태그입니다: %s"), *TagName.ToString());
			bOutHadError = true;
			return false;
		}
		return true;
	}
}

// 파이썬에서 호출하기 위한 FName 진입점. 이름을 태그로 바꿔 구조체 버전에 넘긴다.
UStateTree* ULeeStateTreeAuthoringLibrary::BuildEnemyCombatStateTreeByTagNames(
	const FString& PackagePath,
	const FString& BaseAssetName,
	bool bCanRangedAttack,
	FName LightAttackTagName,
	FName HeavyAttackTagName,
	FName RangedAttackTagName,
	FName TargetVulnerableTagName,
	bool bUseGuardStrafe,
	float RangedCloseRetreatRadius,
	float RangedMinFireInterval,
	FName RangedCooldownTagName,
	UEnvQuery* RangedRepositionQuery)
{
	bool bHadError = false;

	FLeeEnemyTreeBuildConfig Config;
	Config.bCanRangedAttack = bCanRangedAttack;
	Config.bUseGuardStrafe = bUseGuardStrafe;
	Config.RangedCloseRetreatRadius = RangedCloseRetreatRadius;
	Config.RangedMinFireInterval = RangedMinFireInterval;
	Config.RangedRepositionQuery = RangedRepositionQuery;
	ResolveTagName(RangedCooldownTagName, Config.RangedCooldownTag, bHadError);
	ResolveTagName(LightAttackTagName, Config.LightAttackAbilityTag, bHadError);
	ResolveTagName(HeavyAttackTagName, Config.HeavyAttackAbilityTag, bHadError);
	ResolveTagName(RangedAttackTagName, Config.RangedAttackAbilityTag, bHadError);
	ResolveTagName(TargetVulnerableTagName, Config.TargetVulnerableTag, bHadError);

	// 태그 하나라도 잘못되면 만들지 않는다. 잘못된 태그로 생성하면
	// 에셋은 멀쩡해 보이는데 런타임에 어빌리티를 못 찾아 원인 추적이 어려워진다.
	if (bHadError)
	{
		UE_LOG(LogTemp, Error, TEXT("[LeeAuthoring] 태그 오류로 생성을 중단했습니다: %s"), *BaseAssetName);
		return nullptr;
	}

	return BuildEnemyCombatStateTree(PackagePath, BaseAssetName, Config);
}
#pragma endregion
