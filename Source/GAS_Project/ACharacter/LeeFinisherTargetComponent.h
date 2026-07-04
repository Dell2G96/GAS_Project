// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameplayTagContainer.h"
#include "GAS_Project/AEquipment/LeeFinisherData.h"
#include "LeeFinisherTargetComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

/** 피니셔 프롬프트 표시 단계 */
UENUM(BlueprintType)
enum class ELeeFinisherPromptPhase : uint8
{
	Hidden		UMETA(DisplayName = "숨김"),
	Visible		UMETA(DisplayName = "표시 (5m 이내, 접근 유도)"),
	Executable	UMETA(DisplayName = "실행 가능 (2m 이내)"),
};

/**
 * 피니셔 프롬프트 UI 메시지.
 * 채널: MyTags::Souls::Message_Finisher_Prompt (GameplayMessageSubsystem, 로컬 브로드캐스트)
 */
USTRUCT(BlueprintType)
struct FLeeFinisherPromptMessage
{
	GENERATED_BODY()

	/** 프롬프트 대상 Enemy */
	UPROPERTY(BlueprintReadWrite, Category = "Lee|Finisher")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Lee|Finisher")
	ELeeFinisherType Type = ELeeFinisherType::Execution;

	UPROPERTY(BlueprintReadWrite, Category = "Lee|Finisher")
	ELeeFinisherPromptPhase Phase = ELeeFinisherPromptPhase::Hidden;
};

/**
 * Enemy에 부착하는 처형/암살 후보 판정 컴포넌트 (레거시 CEnemyBase 수정 회피용).
 * Enemy Blueprint에서 캡슐 아래에 추가한다.
 *
 * 역할:
 *  1) 판정 주기(EvalInterval)마다 로컬 플레이어와의 거리/각도를 직접 계산해 표시 단계(Hidden/Visible/Executable)를 산출.
 *     GA_Finisher::FindFinisherTarget()과 동일하게 "직접 거리 계산" 방식을 쓴다 — 콜리전 오버랩 이벤트에
 *     의존하지 않으므로 Player 쪽 콜리전 프리셋(WorldDynamic Block 등)의 영향을 받지 않는다.
 *  2) 바뀔 때만 GameplayMessageSubsystem으로 발행 → W_FinishPrompt가 수신 (표시 전용)
 *  3) [서버] LeeSoulsStatSet::OnOutOfStamina 수신 → GE_Groggy 적용 (그로기 진입)
 *
 * 실제 피니셔 실행 조건은 GA_Finisher가 서버에서 재검증하므로, 이 컴포넌트의 판정은 UI 표시에만 쓰인다.
 */
UCLASS(Blueprintable, ClassGroup = (Lee), meta = (BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeFinisherTargetComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	ULeeFinisherTargetComponent();

	/**
	 * 이 Enemy의 스켈레톤 식별 태그 (예: Souls.Skeleton.Goblin).
	 * GA_FinisherVictim이 이 태그로 ULeeFinisherVictimRegistry에서 피해자 몽타주를 조회한다.
	 * 같은 스켈레톤을 쓰는 Enemy BP가 여러 개여도 같은 태그를 지정하면 자동으로 동작한다.
	 */
	UFUNCTION(BlueprintPure, Category = "Lee|Finisher")
	FGameplayTag GetSkeletonTag() const { return SkeletonTag; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 이 Enemy의 스켈레톤 식별 태그. Enemy BP 클래스 디폴트에서 지정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher")
	FGameplayTag SkeletonTag;

	/** 암살/처형 가능 UI 표시 거리 (cm). 구체 반경으로 사용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Finisher", meta = (ClampMin = "0.0"))
	float PromptRadius = 500.0f;

	/** 실행 가능 거리 (cm). GA_Finisher의 ExecuteRadius와 같은 값을 유지할 것 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Finisher", meta = (ClampMin = "0.0"))
	float ExecuteRadius = 200.0f;

	/** 암살 허용 후방 각도 (도, 원뿔 전체 각). GA_Finisher의 BehindAngleDeg와 같은 값을 유지할 것 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Finisher", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float BehindAngleDeg = 120.0f;

	/** 판정 주기 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Finisher", meta = (ClampMin = "0.02"))
	float EvalInterval = 0.15f;

	/** [서버] 스태미나 고갈 시 적용할 그로기 GE. BP에서 GE_Groggy 지정 (Status.Groggy + Status.Vulnerable.Execution 부여) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Finisher")
	TSubclassOf<UGameplayEffect> GroggyEffect;

private:
	/** 오너 ASC의 SoulsStatSet 델리게이트 바인딩 (ASC 초기화 순서 때문에 다음 틱에 시도) */
	void BindToStaminaDelegate();

	/** [서버] 스태미나 0 도달 → GE_Groggy 적용 */
	void HandleOutOfStamina(AActor* EffectInstigator, AActor* EffectCauser,
		const struct FGameplayEffectSpec* EffectSpec, float EffectMagnitude, float OldValue, float NewValue);

	/** 타이머: 로컬 플레이어와의 거리/각도를 직접 계산해 표시 단계 재산출 (상시 실행, 오버랩 이벤트 불필요) */
	void EvaluatePrompt();

	/** 특정 플레이어에 대한 현재 표시 단계 계산 */
	void ComputePromptFor(const APawn* PlayerPawn, ELeeFinisherPromptPhase& OutPhase, ELeeFinisherType& OutType) const;

	/** 단계가 바뀌었을 때만 메시지 발행 */
	void BroadcastPrompt(ELeeFinisherPromptPhase Phase, ELeeFinisherType Type);

	UAbilitySystemComponent* GetOwnerASC() const;

	/** 마지막으로 발행한 표시 단계 (변화 감지용, 로컬 플레이어 기준) */
	ELeeFinisherPromptPhase LastPhase = ELeeFinisherPromptPhase::Hidden;
	ELeeFinisherType LastType = ELeeFinisherType::Execution;

	FTimerHandle EvalTimerHandle;
};
