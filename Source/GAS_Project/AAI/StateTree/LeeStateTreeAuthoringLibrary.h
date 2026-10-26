// Fill out your copyright notice in the Description page of Project Settings.
// 26.09.22 신규 작성 — StateTree 에셋을 C++로 자동 저작하기 위한 에디터 전용 라이브러리

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LeeStateTreeAuthoringLibrary.generated.h"

class UStateTree;
class UEnvQuery;

/**
 * Enemy 전투 StateTree를 생성할 때 쓰는 빌드 설정.
 * 어빌리티 태그를 C++에 하드코딩하지 않고 호출 측(파이썬/BP)에서 넘기도록 분리했다.
 */
USTRUCT(BlueprintType)
struct FLeeEnemyTreeBuildConfig
{
	GENERATED_BODY()

	/** 근접 + 원거리 겸용 아키타입이면 true, 근접 전용이면 false */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AI|Authoring")
	bool bCanRangedAttack = false;

	/** 약공격 어빌리티 태그 (Lee Activate Ability Task가 이 태그로 어빌리티를 찾는다) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AI|Authoring")
	FGameplayTag LightAttackAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AI|Authoring")
	FGameplayTag HeavyAttackAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AI|Authoring")
	FGameplayTag RangedAttackAbilityTag;

	/** 강공격 가중치를 올릴 타겟 상태 태그 (예: 그로기). 비어 있으면 해당 Consideration을 붙이지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AI|Authoring")
	FGameplayTag TargetVulnerableTag;

	/** 가드 스트래핑을 섞을지 여부. false면 일반 스트래핑만 사용한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AI|Authoring")
	bool bUseGuardStrafe = true;

	/**
	 * 원거리 아키타입(bCanRangedAttack=true)일 때 Close 티어의 스트래핑 반경.
	 * 근접형의 기본 반경(350, StrafeAroundTarget 자체 기본값)보다 훨씬 크게 둬서
	 * "원을 그리며 맴돈다"가 아니라 "거리를 벌리며 물러난다"처럼 보이게 한다.
	 * 근접 전용(bCanRangedAttack=false)에는 적용하지 않는다(기존 동작 유지).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AI|Authoring")
	float RangedCloseRetreatRadius = 900.f;

	/** 원거리 공격 재발사 최소 간격(초). 이 시간이 지나기 전에는 RangedAttack State에 진입하지 않는다(R7) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AI|Authoring")
	float RangedMinFireInterval = 2.0f;

	/**
	 * 26.10.21 추가 — 원거리 공격 GA의 쿨다운 태그 (예: Souls.Cooldown.Morgana.Ranged).
	 * 지정하면 이 태그가 있는 동안 RangedAttack State에 진입하지 않는다 (GA 쿨타임과 자동으로 맞춰진다).
	 * 비어 있으면 조건을 붙이지 않는다(기존 동작).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AI|Authoring")
	FGameplayTag RangedCooldownTag;

	/**
	 * 26.10.21 추가 — 원거리 공격을 못 쓰는 동안(쿨타임) 재배치할 지점을 고르는 EQS.
	 * 지정하면 Mid/Far 티어에 RangedAttack 다음 순서로 Reposition State를 만든다.
	 * 비어 있으면 Reposition을 만들지 않는다(기존 동작).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AI|Authoring")
	TObjectPtr<UEnvQuery> RangedRepositionQuery = nullptr;
};

/**
 * StateTree 에셋을 코드로 생성·배선·컴파일·저장하는 에디터 전용 라이브러리.
 * StateTreeEditorModule이 공개한 저작 API(AddChildState/AddTask/AddPropertyBinding)를 감싸
 * BlueprintCallable로 노출하므로, 에디터 파이썬에서 한 줄로 호출할 수 있다.
 *
 * 생성 정책: 기존 에셋을 절대 덮어쓰지 않는다. 같은 이름이 있으면 _v01, _v02 로
 * 새 버전을 만들어 손으로 튜닝한 내용이 사라지지 않게 한다.
 */
UCLASS()
class GAS_PROJECT_API ULeeStateTreeAuthoringLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Enemy 전투용 StateTree 에셋을 새 버전으로 생성한다.
	 * @param PackagePath    생성 위치 (예: "/Game/AI/StateTree")
	 * @param BaseAssetName  버전 접미사를 뺀 기본 이름 (예: "ST_Enemy_Melee")
	 * @param Config         아키타입·어빌리티 태그 설정
	 * @return 생성·컴파일·저장에 성공한 에셋. 실패하면 nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "Lee|AI|Authoring", meta = (DevelopmentOnly))
	static UStateTree* BuildEnemyCombatStateTree(const FString& PackagePath, const FString& BaseAssetName, const FLeeEnemyTreeBuildConfig& Config);

	/** BaseAssetName 기준으로 아직 쓰이지 않은 _vNN 이름을 찾아 반환한다 (덮어쓰기 방지용) */
	UFUNCTION(BlueprintPure, Category = "Lee|AI|Authoring", meta = (DevelopmentOnly))
	static FString MakeVersionedAssetName(const FString& PackagePath, const FString& BaseAssetName);

#pragma region 26.09.22 추가 - 파이썬 호출용 FName 버전
	/**
	 * 태그를 FName으로 받는 버전. 위 함수와 동작은 같다.
	 * 파이썬은 FGameplayTag를 문자열로 만들 수 없어(엔진 제약) 구조체 버전을 호출할 수 없다.
	 * 이 함수가 이름을 태그로 변환해 준 뒤 위 함수에 넘긴다.
	 * 비어 있는 이름은 "지정 안 함"으로 처리하고, 등록되지 않은 이름이면 경고 후 중단한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Lee|AI|Authoring", meta = (DevelopmentOnly))
	static UStateTree* BuildEnemyCombatStateTreeByTagNames(
		const FString& PackagePath,
		const FString& BaseAssetName,
		bool bCanRangedAttack,
		FName LightAttackTagName,
		FName HeavyAttackTagName,
		FName RangedAttackTagName,
		FName TargetVulnerableTagName,
		bool bUseGuardStrafe = true,
		float RangedCloseRetreatRadius = 900.f,
		float RangedMinFireInterval = 2.0f,
		FName RangedCooldownTagName = NAME_None,
		UEnvQuery* RangedRepositionQuery = nullptr);
#pragma endregion

#pragma region 26.10.21 추가 - 원거리 재배치용 EQS 자동 생성
	/**
	 * EQS의 Generator/Test 설정값을 전부 문자열로 만들어 로그에 찍고 반환한다.
	 * EQS 노드 속성은 Python에 노출되지 않아 C++ 리플렉션으로 읽는다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Lee|AI|Authoring", meta = (DevelopmentOnly))
	static FString DescribeEnvQuery(UEnvQuery* Query);

	/**
	 * 원거리 재배치용 EQS(Generator = Points: Circle)에 필터 2개를 추가하고 저장한다. 기존 테스트는 그대로 둔다.
	 * - 현재 위치(Querier)에서 MinMoveDistance 미만인 지점 제외 → 반드시 자리를 옮기게
	 * - 원 중심(공격 대상)까지 시야가 막힌 지점 제외 → 다음 사격이 가능하게 (지점은 SightHeightOffset 높이에서 검사)
	 * 같은 필터가 이미 있으면 다시 추가하지 않는다(여러 번 호출해도 안전).
	 */
	UFUNCTION(BlueprintCallable, Category = "Lee|AI|Authoring", meta = (DevelopmentOnly))
	static bool AddRangedRepositionFilters(UEnvQuery* Query, float MinMoveDistance = 300.f, float SightHeightOffset = 100.f);
#pragma endregion
};
