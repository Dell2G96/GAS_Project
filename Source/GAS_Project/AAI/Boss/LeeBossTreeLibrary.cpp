#include "LeeBossTreeLibrary.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/BehaviorTree.h"

void UBTDecorator_LeeBossKey::Configure(FName Key, bool bInteger, int32 Value)
{
	BlackboardKey.SelectedKeyName = Key;
	IntValue = Value;
	OperationType = bInteger ? uint8(EArithmeticKeyOperation::Equal) : uint8(Value ? EBasicKeyOperation::Set : EBasicKeyOperation::NotSet);
	FlowAbortMode = EBTFlowAbortMode::Both;
	NotifyObserver = EBTBlackboardRestart::ResultChange;
#if WITH_EDITORONLY_DATA
	BasicOperation = Value ? EBasicKeyOperation::Set : EBasicKeyOperation::NotSet;
	ArithmeticOperation = EArithmeticKeyOperation::Equal;
#endif
}

#if WITH_EDITOR
#include "StateTree.h"
#include "StateTreeEditorData.h"
#include "StateTreeState.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorModule.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "Conditions/StateTreeCommonConditions.h"
#include "Conditions/StateTreeObjectConditions.h"
#include "Tasks/StateTreeMoveToTask.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "LeeBossAttackNodes.h"
#include "GAS_Project/AAI/StateTree/LeeStateTreeConsiderations_Combat.h"
#include "LeeStateTreeNodes_Boss.h"
#include "BTService_LeeBossContext.h"
#include "BTTask_LeeActivateAbilityByTag.h"
#include "GAS_Project/AAI/StateTree/LeeStateTreeAuthoringLibrary.h"
#include "GAS_Project/AAI/StateTree/LeeStateTreeEvaluator_CombatContext.h"
#include "GAS_Project/AAI/StateTree/LeeStateTreeTask_StrafeAroundTarget.h"
#include "GAS_Project/MyTags.h"
#include "LeeBossBrainComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "BehaviorTreeGraph.h"
#include "BehaviorTreeGraphNode_Composite.h"
#include "BehaviorTreeGraphNode_Task.h"
#include "BehaviorTreeGraphNode_Decorator.h"
#include "BehaviorTreeGraphNode_Service.h"
#include "EdGraph/EdGraphSchema.h"
#include "UObject/UnrealType.h"
#include "GameFeatureAction_AddComponents.h"
#include "GAS_Project/GameModes/LeeExperienceDefinition.h"
#include "GAS_Project/GameModes/LeeExperienceActionSet.h"

namespace
{
	constexpr int32 BossBranchCount = 5;

	// BT 런타임 노드와 대응하는 에디터 그래프 노드를 만든다.
	UBehaviorTreeGraphNode* MakeGraphNode(UBehaviorTreeGraph& Graph, UClass* Class, UObject& Instance, int32 X, int32 Y)
	{
		auto* Node = NewObject<UBehaviorTreeGraphNode>(&Graph, Class);
		Node->NodeInstance = &Instance;
		Graph.AddNode(Node, false, false);
		Node->CreateNewGuid();
		Node->AllocateDefaultPins();
		Node->NodePosX = X;
		Node->NodePosY = Y;
		return Node;
	}

	// 부모의 출력 핀과 자식의 입력 핀을 연결한다.
	void ConnectGraphNodes(UEdGraphNode& Parent, UEdGraphNode& Child)
	{
		auto* Output = Parent.Pins.FindByPredicate([](const UEdGraphPin* Pin) { return Pin->Direction == EGPD_Output; });
		auto* Input = Child.Pins.FindByPredicate([](const UEdGraphPin* Pin) { return Pin->Direction == EGPD_Input; });
		check(Output && Input);
		(*Output)->MakeLinkTo(*Input);
	}

	// Selector 아래에 우선순위 순서대로 보스의 다섯 실행 분기를 연결한다.
	void AddBossBranches(UBehaviorTree& Tree, UBehaviorTreeGraph& Graph, UBehaviorTreeGraphNode& Selector,
		UStateTree& Phase1, UStateTree& Phase2)
	{
		int32 Index = 0;
		auto AddBranch = [&](UBTTaskNode& Task, FName Key, bool bInteger, int32 Value)
		{
			auto* Node = MakeGraphNode(Graph, UBehaviorTreeGraphNode_Task::StaticClass(), Task, Index++ * 350, 450);
			ConnectGraphNodes(Selector, *Node);
			auto* Decorator = NewObject<UBTDecorator_LeeBossKey>(&Tree);
			Decorator->Configure(Key, bInteger, Value);
			auto* Sub = NewObject<UBehaviorTreeGraphNode_Decorator>(&Graph);
			Sub->NodeInstance = Decorator;
			Node->AddSubNode(Sub, &Graph);
		};
		AddBranch(*NewObject<UBTTask_Wait>(&Tree), LeeBossBBKeys::bIsDead, false, 1);
		AddBranch(*NewObject<UBTTask_Wait>(&Tree), LeeBossBBKeys::bIsGroggy, false, 1);
		auto* Roar = NewObject<UBTTask_LeeActivateAbilityByTag>(&Tree);
		Roar->AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Souls.Abilities.Boss.Stone.Roar"), false);
		Roar->bSucceedWhenUnavailable = true;
		Roar->ClearBoolKeyOnFinish = LeeBossBBKeys::bPhaseTransitionPending;
		AddBranch(*Roar, LeeBossBBKeys::bPhaseTransitionPending, false, 1);

		auto AddPhase = [&](UStateTree& StateTree, int32 Phase)
		{
			auto* Task = NewObject<UBTTask_RunStateTree>(&Tree);
			// 엔진 태스크가 사용하는 에디터 속성에 기존 페이즈 에셋을 지정한다.
			auto* Property = FindFProperty<FStructProperty>(UBTTask_RunStateTree::StaticClass(), TEXT("StateTreeRef"));
			check(Property && Property->Struct == FStateTreeReference::StaticStruct());
			Property->ContainerPtrToValuePtr<FStateTreeReference>(Task)->SetStateTree(&StateTree);
			AddBranch(*Task, LeeBossBBKeys::Phase, true, Phase);
		};
		AddPhase(Phase2, 2);
		AddPhase(Phase1, 1);
	}

	bool SaveAsset(UObject* Asset)
	{
		FAssetRegistryModule::AssetCreated(Asset);
		Asset->MarkPackageDirty();
		FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Asset->GetOutermost(), Asset, *FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
	}
	template<class T> T* CreateAsset(const FString& Path, const FString& Name)
	{
		return NewObject<T>(CreatePackage(*(Path / Name)), *Name, RF_Public | RF_Standalone | RF_Transactional);
	}

	// 공격 상태는 건드리지 않고, 가드에 진입할 수 있는 대기·이동 상태만 연결한다.
	bool AddReactiveGuard(UStateTreeEditorData& Data, UStateTreeState& Root, const FStateTreeEditorNode& Eval)
	{
		const FName GuardName(TEXT("Reactive Guard"));
		if (Root.Children.ContainsByPredicate([GuardName](const UStateTreeState* Child) { return Child && Child->Name == GuardName; }))
		{
			return true;
		}

		auto BindCondition = [&](const FStateTreeEditorNode& Node)
		{
			Data.AddPropertyBinding(Eval, TEXT("TargetActor"), Node, TEXT("TargetActor"));
			Data.AddPropertyBinding(Eval, TEXT("bIsCloseTier"), Node, TEXT("bIsCloseTier"));
			Data.AddPropertyBinding(Eval, TEXT("bIsTargetInFront"), Node, TEXT("bIsTargetInFront"));
		};
		UStateTreeState& Guard = Root.AddChildState(GuardName);
		Root.Children.RemoveSingle(&Guard);
		Root.Children.Insert(&Guard, 0);
		BindCondition(Guard.AddEnterCondition<FLeeBossCanReactWithGuard>());
		auto& Task = Guard.AddTask<FLeeBossReactiveGuard>();
		Data.AddPropertyBinding(Eval, TEXT("TargetActor"), Task, TEXT("TargetActor"));
		Guard.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Root);

		for (UStateTreeState* Child : Root.Children)
		{
			if (Child && Child->Tasks.ContainsByPredicate([](const FStateTreeEditorNode& Node)
			{
				const UScriptStruct* Type = Node.Node.GetScriptStruct();
				return Type == FLeeStateTreeTask_BossRecovery::StaticStruct()
					|| Type == FLeeStateTreeTask_StrafeAroundTarget::StaticStruct()
					|| Type == FStateTreeMoveToTask::StaticStruct();
			}))
			{
				auto& Transition = Child->AddTransition(EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &Guard);
				Transition.Priority = EStateTreeTransitionPriority::High;
				BindCondition(Transition.AddCondition<FLeeBossCanReactWithGuard>());
			}
		}
		return true;
	}

	UStateTree* BuildPhase(const FString& Path, const FString& Name, int32 Phase)
	{
		auto* Tree = CreateAsset<UStateTree>(Path, Name);
		auto* Data = NewObject<UStateTreeEditorData>(Tree, FStateTreeEditorModule::GetModule().GetEditorDataClass(UStateTreeAIComponentSchema::StaticClass()));
		Data->Schema = NewObject<UStateTreeAIComponentSchema>(Data);
		Tree->EditorData = Data;
		auto& Eval = Data->AddEvaluator<FLeeStateTreeEvaluator_CombatContext>();
		auto& Root = Data->AddRootState();
		auto& Attack = Root.AddChildState(TEXT("Attack - configured pattern pool"));
		auto& Strafe = Root.AddChildState(TEXT("Strafe - no available close attack"));
		auto& Chase = Root.AddChildState(TEXT("Chase"));
		auto& Idle = Root.AddChildState(TEXT("Idle"));
		// 회복 상태는 Root 선택 후보가 아니라 명시적인 전이 대상이다.
		auto& Recovery = Root.AddChildState(TEXT("Recovery"));
		Recovery.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		auto BindAttack = [&](auto& Node, FGameplayTag Ability, FGameplayTag History)
		{
			Node.GetInstanceData().AbilityTag = Ability;
			Node.GetInstanceData().HistoryTag = History;
			Data->AddPropertyBinding(Eval, TEXT("TargetActor"), Node, TEXT("TargetActor"));
			Data->AddPropertyBinding(Eval, TEXT("DistanceToTarget"), Node, TEXT("Distance"));
		};
		Attack.Name = TEXT("Attack Tags - weighted random");
		Attack.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenAtRandomWeightedByUtility;
		auto AddAttack = [&](const TCHAR* Name, FGameplayTag Ability, FGameplayTag History)
		{
			auto& State = Attack.AddChildState(Name);
			auto& Condition = State.AddEnterCondition<FLeeBossCanActivateAbility>();
			BindAttack(Condition, Ability, History);
			auto& Task = State.AddTask<FLeeBossAttackTask>();
			BindAttack(Task, Ability, History);
			FStateTreeEditorNode& Item = State.Considerations.AddDefaulted_GetRef();
			Item.ID = FGuid::NewGuid();
			Item.Node.InitializeAs<FLeeConsideration_TimeSinceLastUse>();
			Item.Instance.InitializeAs<FLeeTimeSinceLastUseConsiderationInstanceData>();
			auto& Score = static_cast<TStateTreeEditorNode<FLeeConsideration_TimeSinceLastUse>&>(Item);
			Score.GetInstanceData().AttackTag = History;
			Data->AddPropertyBinding(Eval, TEXT("Actor"), Score, TEXT("RequesterPawn"));
			State.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Recovery);
			State.AddTransition(EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, &Recovery);
		};
		AddAttack(TEXT("Light Attack"), MyTags::Souls::Ability_Attack_Melee_Light, MyTags::Souls::Attacker_Melee_Light);
		AddAttack(TEXT("Heavy Attack"), MyTags::Souls::Ability_Attack_Melee_Heavy, MyTags::Souls::Attacker_Melee_Heavy);
		auto& Rest = Recovery.AddTask<FLeeStateTreeTask_BossRecovery>();
		Data->AddPropertyBinding(Eval, TEXT("TargetActor"), Rest, TEXT("TargetActor"));
		Attack.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Recovery);
		Recovery.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Root);
		auto& Failed = Attack.AddTransition(EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, &Recovery);
		(void)Failed;
		auto Tier = [&](UStateTreeState& State, const TCHAR* Field)
		{
			auto& C = State.AddEnterCondition<FStateTreeCompareBoolCondition>(); C.GetInstanceData().bRight = true;
			Data->AddPropertyBinding(Eval, Field, C, TEXT("bLeft"));
		};
		Tier(Strafe, TEXT("bIsCloseTier"));
		auto& S = Strafe.AddTask<FLeeStateTreeTask_StrafeAroundTarget>();
		Data->AddPropertyBinding(Eval, TEXT("TargetActor"), S, TEXT("TargetActor"));
		// 거리가 가까워져도 이동 태스크의 완료 여부에 의존하지 않고 공격 가능 시 재선택한다.
		auto& M = Chase.AddTask<FStateTreeMoveToTask>();
		Data->AddPropertyBinding(Eval, TEXT("TargetActor"), M, TEXT("TargetActor"));
		M.GetInstanceData().AcceptableRadius = 200.f;
		M.GetInstanceData().bReachTestIncludesAgentRadius = true;
		M.GetInstanceData().bReachTestIncludesGoalRadius = true;
		// 객체 유효성은 기존 Evaluator의 Mid/Far 외에 타겟 여부를 직접 검사한다.
		auto& HasTarget = Chase.AddEnterCondition<FStateTreeObjectIsValidCondition>();
		Data->AddPropertyBinding(Eval, TEXT("TargetActor"), HasTarget, TEXT("Object"));
		for (auto* State : { &Strafe, &Chase })
		{
			auto& Available = State->AddTransition(EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &Root);
			auto& C = Available.AddCondition<FLeeBossCanActivateAbility>();
			BindAttack(C, MyTags::Souls::Ability_Attack_Melee_Light, MyTags::Souls::Attacker_Melee_Light);
			State->AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Recovery);
			State->AddTransition(EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, &Recovery);
		}
		auto& IdleWait = Idle.AddTask<FLeeStateTreeTask_BossRecovery>();
		Data->AddPropertyBinding(Eval, TEXT("TargetActor"), IdleWait, TEXT("TargetActor"));
		IdleWait.GetInstanceData().bKeepFocusOnTarget = false;
		Idle.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Root);
		AddReactiveGuard(*Data, Root, Eval);
		FStateTreeCompilerLog Log;
		if (!UStateTreeEditingSubsystem::CompileStateTree(Tree, Log) || !SaveAsset(Tree)) return nullptr;
		return Tree;
	}
}
#endif

UBehaviorTree* ULeeBossTreeLibrary::BuildBossTrees(const FString& PackagePath, const FString& BaseName)
{
#if WITH_EDITOR
	FString Version;
	for (int32 Index = 1; Index <= 99; ++Index)
	{
		const FString Candidate = FString::Printf(TEXT("%s_v%02d"), *BaseName, Index);
		bool bExists = false;
		for (const TCHAR* Suffix : {TEXT(""), TEXT("_BB"), TEXT("_Phase1"), TEXT("_Phase2")})
			bExists |= FPackageName::DoesPackageExist(PackagePath / (Candidate + Suffix)) || FindPackage(nullptr, *(PackagePath / (Candidate + Suffix))) != nullptr;
		if (!bExists) { Version = Candidate; break; }
	}
	if (Version.IsEmpty() || !FPackageName::IsValidLongPackageName(PackagePath / Version)) return nullptr;
	UStateTree* Phase1 = BuildPhase(PackagePath, Version + TEXT("_Phase1"), 1);
	UStateTree* Phase2 = BuildPhase(PackagePath, Version + TEXT("_Phase2"), 2);
	if (!Phase1 || !Phase2) return nullptr;
	auto* BB = CreateAsset<UBlackboardData>(PackagePath, Version + TEXT("_BB"));
	auto Key = [&](const TCHAR* Name, UClass* Type)
	{
		FBlackboardEntry& Entry = BB->Keys.AddDefaulted_GetRef(); Entry.EntryName = Name;
		Entry.KeyType = NewObject<UBlackboardKeyType>(BB, Type);
	};
	Key(LeeBossBBKeys::TargetActor, UBlackboardKeyType_Object::StaticClass());
	Key(LeeBossBBKeys::DistanceToTarget, UBlackboardKeyType_Float::StaticClass());
	Key(LeeBossBBKeys::SelfHealthRatio, UBlackboardKeyType_Float::StaticClass());
	Key(LeeBossBBKeys::Phase, UBlackboardKeyType_Int::StaticClass());
	for (const TCHAR* Name : {LeeBossBBKeys::bIsDead, LeeBossBBKeys::bIsGroggy, LeeBossBBKeys::bTargetHealing, LeeBossBBKeys::bPhaseTransitionPending}) Key(Name, UBlackboardKeyType_Bool::StaticClass());
	auto* BT = CreateAsset<UBehaviorTree>(PackagePath, Version);
	BT->BlackboardAsset = BB;
	auto* Root = NewObject<UBTComposite_Selector>(BT);
	BT->RootNode = Root;

	auto* Graph = CastChecked<UBehaviorTreeGraph>(FBlueprintEditorUtils::CreateNewGraph(
		BT, TEXT("Behavior Tree"), UBehaviorTreeGraph::StaticClass(), GetDefault<UBehaviorTreeGraph>()->Schema));
	BT->BTGraph = Graph;
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	UEdGraphNode* GraphRoot = Graph->Nodes[0];
	GraphRoot->NodePosX = 700;
	auto* Selector = MakeGraphNode(*Graph, UBehaviorTreeGraphNode_Composite::StaticClass(), *Root, 700, 180);
	ConnectGraphNodes(*GraphRoot, *Selector);

	// 서브노드 추가 시 호출되는 중간 갱신을 막고, 완성된 그래프를 한 번만 변환한다.
	Graph->LockUpdates();
	auto* Service = NewObject<UBehaviorTreeGraphNode_Service>(Graph);
	Service->NodeInstance = NewObject<UBTService_LeeBossContext>(BT);
	Selector->AddSubNode(Service, Graph);
	AddBossBranches(*BT, *Graph, *Selector, *Phase1, *Phase2);
	Graph->UnlockUpdates();

	if (GetBossBranchCount(BT) != BossBranchCount || !SaveAsset(BB) || !SaveAsset(BT)) return nullptr;
	return BT;
#else
	return nullptr;
#endif
}

// 서비스만 남은 기존 보스 그래프를 복구하고 같은 BT 에셋에 저장한다.
bool ULeeBossTreeLibrary::RepairEmptyBossTree(UBehaviorTree* Tree, UStateTree* Phase1, UStateTree* Phase2)
{
#if WITH_EDITOR
	if (!Tree || !Tree->BlackboardAsset || !Phase1 || !Phase2 || !Cast<UBTComposite_Selector>(Tree->RootNode))
	{
		return false;
	}
	auto* Graph = Cast<UBehaviorTreeGraph>(Tree->BTGraph);
	if (!Graph || GetBossBranchCount(Tree) != 0 || Graph->Nodes.Num() != 2)
	{
		return false;
	}
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		auto* Selector = Cast<UBehaviorTreeGraphNode_Composite>(Node);
		if (Selector && Selector->NodeInstance == Tree->RootNode && Selector->Services.Num() == 1
			&& Cast<UBTService_LeeBossContext>(Selector->Services[0]->NodeInstance))
		{
			Graph->LockUpdates();
			AddBossBranches(*Tree, *Graph, *Selector, *Phase1, *Phase2);
			Graph->UnlockUpdates();
			return GetBossBranchCount(Tree) == BossBranchCount && SaveAsset(Tree);
		}
	}
#endif
	return false;
}

// 에디터 표시와 별도로 게임이 사용하는 루트의 실행 분기 수를 반환한다.
int32 ULeeBossTreeLibrary::GetBossBranchCount(const UBehaviorTree* Tree)
{
	return Tree && Tree->RootNode ? Tree->RootNode->Children.Num() : 0;
}

// 현재 페이즈를 다시 생성하지 않고 가드 상태만 추가한 뒤 컴파일하고 저장한다.
bool ULeeBossTreeLibrary::AddReactiveGuardToPhase(UStateTree* Tree)
{
#if WITH_EDITOR
	auto* Data = Tree ? Cast<UStateTreeEditorData>(Tree->EditorData) : nullptr;
	if (!Data || Data->SubTrees.Num() != 1 || !Data->SubTrees[0])
	{
		return false;
	}
	const FStateTreeEditorNode* Eval = Data->Evaluators.FindByPredicate([](const FStateTreeEditorNode& Node)
	{
		return Node.Node.GetScriptStruct() == FLeeStateTreeEvaluator_CombatContext::StaticStruct();
	});
	if (!Eval || !AddReactiveGuard(*Data, *Data->SubTrees[0], *Eval))
	{
		return false;
	}
	FStateTreeCompilerLog Log;
	return UStateTreeEditingSubsystem::CompileStateTree(Tree, Log) && SaveAsset(Tree);
#else
	return false;
#endif
}

bool ULeeBossTreeLibrary::RemoveDirectBossBrain(UBlueprint* Blueprint)
{
#if WITH_EDITOR
	if (!Blueprint || !Blueprint->SimpleConstructionScript) return false;
	const auto Nodes = Blueprint->SimpleConstructionScript->GetAllNodes();
	for (USCS_Node* Node : Nodes)
	{
		if (Cast<ULeeBossBrainComponent>(Node->ComponentTemplate)) Blueprint->SimpleConstructionScript->RemoveNodeAndPromoteChildren(Node);
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (Blueprint->Status == BS_Error) return false;
	FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
	return UPackage::SavePackage(Blueprint->GetOutermost(), Blueprint, *FPackageName::LongPackageNameToFilename(Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
#else
	return false;
#endif
}

bool ULeeBossTreeLibrary::ConfigureBossExperience(UBlueprint* Controller, UBlueprint* Component, UBehaviorTree* Tree,
	ULeeExperienceDefinition* Experience, ULeeExperienceActionSet* ActionSet)
{
#if WITH_EDITOR
	if (!Controller || !Component || !Tree || !Experience || !ActionSet ||
		!Component->GeneratedClass || !Component->GeneratedClass->IsChildOf(ULeeBossBrainComponent::StaticClass())) return false;
	auto* Defaults = CastChecked<ULeeBossBrainComponent>(Component->GeneratedClass->GetDefaultObject());
	if (Controller->SimpleConstructionScript)
		for (USCS_Node* Node : Controller->SimpleConstructionScript->GetAllNodes())
			if (const auto* Old = Cast<ULeeBossBrainComponent>(Node->ComponentTemplate)) Defaults->Phase2HealthThreshold = Old->Phase2HealthThreshold;
	Defaults->BossBehaviorTree = Tree;
	FBlueprintEditorUtils::MarkBlueprintAsModified(Component);
	FKismetEditorUtilities::CompileBlueprint(Component);
	if (Component->Status == BS_Error) return false;
	UGameFeatureAction_AddComponents* Add = nullptr;
	for (const auto& Action : ActionSet->Actions)
		if (auto* Found = Cast<UGameFeatureAction_AddComponents>(Action.Get())) { Add = Found; break; }
	if (!Add)
	{
		Add = NewObject<UGameFeatureAction_AddComponents>(ActionSet, NAME_None, RF_Transactional);
		ActionSet->Actions.Add(Add);
	}
	Add->ComponentList.RemoveAll([Controller](const FGameFeatureComponentEntry& Entry) { return Entry.ActorClass.Get() == Controller->GeneratedClass; });
	FGameFeatureComponentEntry Entry;
	Entry.ActorClass = Controller->GeneratedClass;
	Entry.ComponentClass = Component->GeneratedClass;
	Entry.bServerComponent = true;
	Entry.bClientComponent = false;
	Add->ComponentList.Add(Entry);
	Experience->ActionSets.AddUnique(ActionSet);
	auto* ExperienceBP = Cast<UBlueprint>(Experience->GetClass()->ClassGeneratedBy);
	if (!ExperienceBP) return false;
	FBlueprintEditorUtils::MarkBlueprintAsModified(ExperienceBP);
	if (!SaveAsset(Component) || !SaveAsset(ActionSet) || !SaveAsset(ExperienceBP)) return false;
	return RemoveDirectBossBrain(Controller);
#else
	return false;
#endif
}

FString ULeeBossTreeLibrary::DescribeComponentActions(ULeeExperienceActionSet* ActionSet)
{
	FString Result;
#if WITH_EDITOR
	if (ActionSet)
		for (const auto& Action : ActionSet->Actions)
			if (const auto* Add = Cast<UGameFeatureAction_AddComponents>(Action))
				for (const auto& Entry : Add->ComponentList)
					Result += FString::Printf(TEXT("%s -> %s server=%d client=%d\n"), *Entry.ActorClass.ToString(), *Entry.ComponentClass.ToString(), Entry.bServerComponent, Entry.bClientComponent);
#endif
	return Result;
}
