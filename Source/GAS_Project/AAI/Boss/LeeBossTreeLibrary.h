#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BehaviorTree/Tasks/BTTask_RunStateTree.h"
#include "LeeBossTreeLibrary.generated.h"

class ULeeExperienceDefinition;
class ULeeExperienceActionSet;
class UStateTree;

UCLASS()
class GAS_PROJECT_API UBTDecorator_LeeBossKey : public UBTDecorator_Blackboard
{
	GENERATED_BODY()
public:
	void Configure(FName Key, bool bInteger, int32 Value);
};

UCLASS()
class GAS_PROJECT_API ULeeBossTreeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** 별도 버전 이름으로 BB/BT/페이즈 ST를 생성한다. 기존 에셋을 덮어쓰지 않는다. */
	UFUNCTION(BlueprintCallable, Category="Lee|Boss", meta=(DevelopmentOnly))
	static UBehaviorTree* BuildBossTrees(const FString& PackagePath, const FString& BaseName);

	/** 자식 분기가 유실된 보스 BT에 기존 페이즈 StateTree를 다시 연결한다. */
	UFUNCTION(BlueprintCallable, Category="Lee|Boss", meta=(DevelopmentOnly))
	static bool RepairEmptyBossTree(UBehaviorTree* Tree, UStateTree* Phase1, UStateTree* Phase2);

	/** 저장 후 다시 불러온 BT의 실행 분기 수를 확인한다. */
	UFUNCTION(BlueprintPure, Category="Lee|Boss", meta=(DevelopmentOnly))
	static int32 GetBossBranchCount(const UBehaviorTree* Tree);

	/** 기존 페이즈의 공격 설정을 유지하면서 근접 공격에 반응하는 가드를 추가한다. */
	UFUNCTION(BlueprintCallable, Category="Lee|Boss", meta=(DevelopmentOnly))
	static bool AddReactiveGuardToPhase(UStateTree* Tree);

	UFUNCTION(BlueprintCallable, Category="Lee|Boss", meta=(DevelopmentOnly))
	static bool RemoveDirectBossBrain(UBlueprint* Blueprint);
	UFUNCTION(BlueprintCallable, Category="Lee|Boss", meta=(DevelopmentOnly))
	static bool ConfigureBossExperience(UBlueprint* Controller, UBlueprint* Component, UBehaviorTree* Tree,
		ULeeExperienceDefinition* Experience, ULeeExperienceActionSet* ActionSet);
	UFUNCTION(BlueprintCallable, Category="Lee|Boss", meta=(DevelopmentOnly))
	static FString DescribeComponentActions(ULeeExperienceActionSet* ActionSet);
};
